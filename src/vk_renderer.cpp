#include <si/vk_renderer.hpp>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <cstdint>
#include <numeric>
#include <limits>
#include <si/util.hpp>

namespace {
    const auto win_image_format = ::vk::Format::eB8G8R8A8Srgb;
    const auto win_color_space = ::vk::ColorSpaceKHR::eSrgbNonlinear;
}

si::vk::renderer::renderer(si::vk::gfx_device& device, ::vk::UniqueSurfaceKHR old_surface):
    device(device),
    surface(std::move(old_surface)),
    image_available(device.logical->createSemaphoreUnique({})),
    render_finished(device.logical->createSemaphoreUnique({})),
    in_flight(device.logical->createFenceUnique({::vk::FenceCreateFlagBits::eSignaled})) {

    static unsigned i = 1;
    spdlog::debug("i = {}", i);
    static std::uint32_t win_dim;
    win_dim = i++ * 200;
    
    ::vk::SurfaceCapabilitiesKHR caps = device.physical.getSurfaceCapabilitiesKHR(*surface);
    
    std::vector<::vk::SurfaceFormatKHR> formats = device.physical.getSurfaceFormatsKHR(*surface);
    spdlog::info("Available formats:");
    for (auto format : formats) { spdlog::info(" * {}/{}", to_string(format.format), to_string(format.colorSpace)); }
    
    std::vector<::vk::PresentModeKHR> modes = device.physical.getSurfacePresentModesKHR(*surface);
    spdlog::info("Available modes:");
    for (auto mode : modes) { spdlog::info(" * {}", to_string(mode)); }

    swapchain = device.logical->createSwapchainKHRUnique ({
        {},
        *surface,
        caps.minImageCount + 1,
        win_image_format, win_color_space, {win_dim, win_dim},
        1,
        ::vk::ImageUsageFlagBits::eColorAttachment,
        ::vk::SharingMode::eExclusive,
        0,
        nullptr,
        caps.currentTransform,
        ::vk::CompositeAlphaFlagBitsKHR::eOpaque,
        ::vk::PresentModeKHR::eMailbox,
        true,
        nullptr
    });

    images = device.logical->getSwapchainImagesKHR(*swapchain);
    image_views.reserve(images.size());
    for (::vk::Image& img : images) {
        image_views.push_back (
            device.logical->createImageViewUnique({
                {},
                img,
                ::vk::ImageViewType::e2D,
                win_image_format,
                {::vk::ComponentSwizzle::eIdentity, ::vk::ComponentSwizzle::eIdentity, ::vk::ComponentSwizzle::eIdentity, ::vk::ComponentSwizzle::eIdentity},
                {::vk::ImageAspectFlagBits::eColor,
                 0, 1,
                 0, 1
                }
            })
        );
    }

    framebuffers.reserve(image_views.size());
    for (::vk::UniqueImageView& view_ptr : image_views) {
        framebuffers.push_back (
            device.logical->createFramebufferUnique(::vk::FramebufferCreateInfo {
                {},
                *device.render_pass,
                1, &*view_ptr,
                win_dim, win_dim,
                1
            })
        );
    }

    command_buffers = device.logical->allocateCommandBuffersUnique({
        *device.graphics_command_pool,
        ::vk::CommandBufferLevel::ePrimary,
        static_cast<std::uint32_t>(framebuffers.size())
    });
    for (std::size_t i = 0; i < command_buffers.size(); i++) {
        const ::vk::Viewport viewport (0.0f, 0.0f, static_cast<float>(win_dim), static_cast<float>(win_dim), 0.0f, 1.0f);
        const ::vk::Rect2D scissor ({0, 0}, {static_cast<std::uint32_t>(win_dim), static_cast<std::uint32_t>(win_dim)});
    
        ::vk::CommandBuffer& cmd = *command_buffers[i];
        cmd.begin(::vk::CommandBufferBeginInfo {});
        ::vk::ClearValue clear_color {::vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}}};
        cmd.beginRenderPass (
            {
                *device.render_pass,
                *framebuffers[i],
                scissor,
                1,
                &clear_color
            },
            ::vk::SubpassContents::eInline
        );
        cmd.bindPipeline(::vk::PipelineBindPoint::eGraphics, *device.pipeline);
        cmd.setViewport(0, viewport);
        cmd.setScissor(0, scissor);
        cmd.draw(3, 1, 0, 0);
        cmd.endRenderPass();
        cmd.end();
    }
}

si::vk::renderer::~renderer() {
    //TODO: should we reset the command buffer instead of waiting for it to finish?
    device.logical->waitIdle();
}

void si::vk::renderer::draw() {
    device.logical->waitForFences(*in_flight, true, std::numeric_limits<std::uint64_t>::max());
    device.logical->resetFences(*in_flight);
    auto [result, image_ix] = device.logical->acquireNextImageKHR (
        *swapchain,
        std::numeric_limits<std::uint64_t>::max(),
        *image_available,
        nullptr
    );
    ::vk::PipelineStageFlags pipeline_stage = ::vk::PipelineStageFlagBits::eColorAttachmentOutput;
    device.graphics_q.submit (
        ::vk::SubmitInfo {
            1, &*image_available,
            &pipeline_stage,
            1, &*command_buffers[image_ix],
            1, &*render_finished
        },
        *in_flight
    );
    device.present_q.presentKHR (
        ::vk::PresentInfoKHR {
            1, &*render_finished,
            1, &*swapchain,
            &image_ix
        }
    );
}

namespace {
    ::vk::UniqueShaderModule make_module(::vk::Device device, std::string filename) {
        auto code = si::file_contents(filename);
        if (code.size() % 4 != 0) {
            throw std::runtime_error("Code size must be multiple of 4.");
        }
        return device.createShaderModuleUnique( {{}, code.size(), reinterpret_cast<std::uint32_t*>(code.data())} );
    }
    const std::vector<const char*> exts_required = { "VK_KHR_swapchain" };
}

si::vk::gfx_device::gfx_device(::vk::PhysicalDevice physical, ::vk::Queue graphics_q, std::uint32_t graphics_q_family_ix, ::vk::Queue present_q, std::uint32_t present_q_family_ix, ::vk::UniqueDevice device):
    physical(physical),
    graphics_q(graphics_q),
    graphics_q_family_ix(graphics_q_family_ix),
    present_q(present_q),
    present_q_family_ix(present_q_family_ix),
    logical(std::move(device)),
    pipeline_layout(logical->createPipelineLayoutUnique({
        {},
        0, nullptr,
        0, nullptr})
    ) {
    std::vector<::vk::PipelineShaderStageCreateInfo> stages;
    std::vector<::vk::UniqueShaderModule> shaders;
    for (auto [stage_name, stage] : std::vector {std::make_pair("vert", ::vk::ShaderStageFlagBits::eVertex),
                                                 std::make_pair("frag", ::vk::ShaderStageFlagBits::eFragment)}) {
        shaders.push_back(make_module(*logical, fmt::format("build/{}.spv", stage_name)));
        stages.emplace_back (
            ::vk::PipelineShaderStageCreateFlags(),
            stage,
            *shaders.back(),
            "main"
        );
    }
    const ::vk::PipelineVertexInputStateCreateInfo input_state ({}, 0, nullptr, 0, nullptr);
    const ::vk::PipelineInputAssemblyStateCreateInfo assembly_state ({}, ::vk::PrimitiveTopology::eTriangleList, false);
    const ::vk::PipelineViewportStateCreateInfo viewport_state ({}, 1, nullptr, 1, nullptr);
    const ::vk::PipelineRasterizationStateCreateInfo rasterization_state (
        {}, false, false,
        ::vk::PolygonMode::eFill,
        ::vk::CullModeFlagBits::eBack,
        ::vk::FrontFace::eClockwise,
        false, 0.0f, 0.0f, 0.0f);
    const ::vk::PipelineMultisampleStateCreateInfo multisample_state ({}, ::vk::SampleCountFlagBits::e1, false, 1.0f, nullptr, false, false);
    const ::vk::PipelineColorBlendAttachmentState color_blend_attachment_state (
        false,
        ::vk::BlendFactor::eOne, ::vk::BlendFactor::eZero, ::vk::BlendOp::eAdd,
        ::vk::BlendFactor::eOne, ::vk::BlendFactor::eZero, ::vk::BlendOp::eAdd,
        ::vk::ColorComponentFlagBits::eR | ::vk::ColorComponentFlagBits::eG | ::vk::ColorComponentFlagBits::eB | ::vk::ColorComponentFlagBits::eA
    );
    const ::vk::PipelineColorBlendStateCreateInfo color_blend_state ({}, false, ::vk::LogicOp::eCopy, 1, &color_blend_attachment_state, {0.0f, 0.0f, 0.0f, 0.0f});
    const ::vk::AttachmentDescription colour_attachment (
        {},
        ::vk::Format::eB8G8R8A8Srgb, ::vk::SampleCountFlagBits::e1,
        ::vk::AttachmentLoadOp::eClear, ::vk::AttachmentStoreOp::eStore,
        ::vk::AttachmentLoadOp::eDontCare, ::vk::AttachmentStoreOp::eDontCare,
        ::vk::ImageLayout::eUndefined, ::vk::ImageLayout::ePresentSrcKHR);
    const ::vk::AttachmentReference colour_attachment_ref (0, ::vk::ImageLayout::eColorAttachmentOptimal);
    const ::vk::SubpassDescription subpass (
        ::vk::SubpassDescriptionFlags{}, ::vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &colour_attachment_ref,
        nullptr,
        nullptr,
        0, nullptr);
    const ::vk::SubpassDependency subpass_dependency (
        VK_SUBPASS_EXTERNAL, 0,
        ::vk::PipelineStageFlagBits::eColorAttachmentOutput,
        ::vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        ::vk::AccessFlagBits::eColorAttachmentRead | ::vk::AccessFlagBits::eColorAttachmentWrite);
    render_pass = logical->createRenderPassUnique ({
            {},
            1, &colour_attachment,
            1, &subpass,
            1, &subpass_dependency 
        });
    const std::array dynamic_states = {::vk::DynamicState::eViewport, ::vk::DynamicState::eScissor};
    const ::vk::PipelineDynamicStateCreateInfo dynamic_state {{}, dynamic_states.size(), dynamic_states.data() };
    pipeline = logical->createGraphicsPipelineUnique (
        nullptr,
        ::vk::GraphicsPipelineCreateInfo (
            {},
            static_cast<std::uint32_t>(stages.size()),
            stages.data(),
            &input_state,
            &assembly_state,
            nullptr, //tesselation
            &viewport_state,
            &rasterization_state,
            &multisample_state,
            nullptr, //depth
            &color_blend_state,
            &dynamic_state,
            *pipeline_layout,
            *render_pass,
            0,
            nullptr,
            -1));
    graphics_command_pool = logical->createCommandPoolUnique({{}, graphics_q_family_ix});
}

std::unique_ptr<si::vk::renderer> si::vk::gfx_device::make_renderer(::vk::UniqueSurfaceKHR surface) {
    return std::make_unique<si::vk::renderer>(*this, std::move(surface));
}

namespace {
    const std::uint32_t app_version = VK_MAKE_VERSION(0, 1, 0);
    const std::uint32_t engine_version = VK_MAKE_VERSION(0, 1, 0);
    const vk::ApplicationInfo app_info { "si-test", app_version, "No Engine", engine_version, VK_API_VERSION_1_1 };

    VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_output(
        VkDebugReportFlagsEXT       flags,
        VkDebugReportObjectTypeEXT  objectType,
        uint64_t                    object,
        size_t                      location,
        int32_t                     messageCode,
        const char*                 pLayerPrefix,
        const char*                 pMessage,
        void*                       pUserData
    )   {
        if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
            spdlog::info(pMessage);
        } else if (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)) {
            spdlog::warn(pMessage);
        } else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            spdlog::error(pMessage);
        } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
            spdlog::debug(pMessage);
        };
        return VK_FALSE;
    }
    
    vk::UniqueInstance make_instance() {
        spdlog::info("Available instance layers:");
        for (auto& layer : vk::enumerateInstanceLayerProperties()) {
            spdlog::info(" * {}", layer.layerName);
        }
        spdlog::info("Available instance extensions:");
        for (const auto& extension : vk::enumerateInstanceExtensionProperties()) {
            spdlog::info(" * {}", extension.extensionName);
        }
        
        auto layers = std::vector { "VK_LAYER_LUNARG_standard_validation" };
        auto extensions = std::vector { "VK_KHR_surface", "VK_KHR_wayland_surface", "VK_EXT_debug_utils", "VK_EXT_debug_report" };
        vk::UniqueInstance vk = vk::createInstanceUnique ({
            {},
            &app_info,
            static_cast<std::uint32_t>(layers.size()), layers.data(),
            static_cast<std::uint32_t>(extensions.size()), extensions.data()
        });
        spdlog::debug("Successfully created vulkan instance");
        return vk;
    }
    VkDebugReportCallbackEXT attach_debug_reporter(VkInstance vk) {        
        auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(vk, "vkCreateDebugReportCallbackEXT"));
        VkDebugReportCallbackCreateInfoEXT debug_callback_crinfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
            .pfnCallback = vk_debug_output,
            .pUserData = nullptr
        };
        VkDebugReportCallbackEXT callback;
        if (VkResult r = vkCreateDebugReportCallbackEXT(vk, &debug_callback_crinfo, nullptr, &callback); r != VK_SUCCESS) {
            throw std::runtime_error(fmt::format("Couldn't create debug callback: {}", r));
        } else {
            spdlog::debug("Successfully created debug reporter");
        }
        return callback;
    }
}
si::vk::root::root(): instance(make_instance()), debug_reporter(attach_debug_reporter(*instance)) {
}
std::unique_ptr<si::vk::renderer> si::vk::root::make_renderer(::wl::display& display, ::wl::surface& surface) {
    // Optimistically make a vulkan surface
    ::vk::UniqueSurfaceKHR vk_surface = instance->createWaylandSurfaceKHRUnique ({
        {}, static_cast<wl_display*>(display), static_cast<wl_surface*>(surface)
    });
    auto gfx_it = std::find_if(gfxs.begin(), gfxs.end(), [&](const si::vk::gfx_device& gfx) {
        return gfx.physical.getSurfaceSupportKHR(gfx.present_q_family_ix, *vk_surface);
    });
    if (gfx_it != gfxs.end()) {
        return gfx_it->make_renderer(std::move(vk_surface));
    } else {
        for (::vk::PhysicalDevice& physical : instance->enumeratePhysicalDevices()) {
            auto props = physical.getProperties();
            spdlog::debug("Device max viewports: {} up to {}x{}", props.limits.maxViewports, props.limits.maxViewportDimensions[0], props.limits.maxViewportDimensions[1]);
            std::vector<::vk::ExtensionProperties> exts_avail = physical.enumerateDeviceExtensionProperties();
            //TODO: actually check if the required extensions are available
            if (true) {
                std::vector<::vk::QueueFamilyProperties> queue_families = physical.getQueueFamilyProperties();
                std::vector<::vk::DeviceQueueCreateInfo> queue_infos;
                const float queue_priority = 0.0f;
                std::optional<std::uint32_t> graphics_q_ix;
                std::optional<std::uint32_t> present_q_ix;
                for (std::size_t i = 0; i < queue_families.size(); i++) {
                    queue_infos.push_back({{}, static_cast<std::uint32_t>(i), 0, &queue_priority});
                    if (queue_families[i].queueFlags & ::vk::QueueFlagBits::eGraphics) {
                        graphics_q_ix = i;
                        queue_infos[i].queueCount = 1;
                        spdlog::info("Found graphics queue family");
                    }
                    if (physical.getSurfaceSupportKHR(i, *vk_surface)) {
                        present_q_ix = i;
                        queue_infos[i].queueCount = 1;
                        spdlog::info("Found present queue family");
                    }
                }
                if (graphics_q_ix && present_q_ix) {
                    ::vk::PhysicalDeviceFeatures features = physical.getFeatures();
                    ::vk::DeviceCreateInfo device_info (
                        {},
                        static_cast<std::uint32_t>(queue_infos.size()), queue_infos.data(),
                        0, nullptr, // device layers are deprecated
                        static_cast<std::uint32_t>(exts_required.size()), exts_required.data(),
                        &features
                    );
                    ::vk::UniqueDevice logical = physical.createDeviceUnique(device_info);
                    ::vk::Queue graphics_q = logical->getQueue(*graphics_q_ix, 0);
                    ::vk::Queue present_q = logical->getQueue(*present_q_ix, 0);
                    gfx_device& created = gfxs.emplace_back(physical, graphics_q, *graphics_q_ix, present_q, *present_q_ix, std::move(logical));
                    return created.make_renderer(std::move(vk_surface));
                }
            }
        }
        throw std::runtime_error("Can't create a renderer for supplied wayland display/surface");
    }
}
