#include <si/vk_renderer.hpp>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <cstdint>
#include <numeric>
#include <si/util.hpp>

namespace {
    const std::uint32_t app_version = 0;
    const std::uint32_t engine_version = 0;
    const vk::ApplicationInfo app_info { "si test", app_version, "No Engine", engine_version, VK_API_VERSION_1_1 };

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

    vk::UniqueSurfaceKHR make_surface(vk::Instance vk, wl::display& dpy, wl::surface& sfc) {
        return vk.createWaylandSurfaceKHRUnique ({
            {},
            static_cast<wl_display*>(dpy),
            static_cast<wl_surface*>(sfc)
        });
    }

    vk::UniqueShaderModule make_module(vk::Device device, std::string filename) {
        auto code = si::file_contents(filename);
        if (code.size() % 4 != 0) {
            throw std::runtime_error("Code size must be multiple of 4.");
        }
        return device.createShaderModuleUnique( {{}, code.size(), reinterpret_cast<std::uint32_t*>(code.data())} );
    }

    std::vector<vk::UniqueImageView> make_image_views(vk::Device device, vk::SwapchainKHR swapchain) {
        std::vector<vk::UniqueImageView> views;
        for (vk::Image image : device.getSwapchainImagesKHR(swapchain)) {
            views.push_back(device.createImageViewUnique({
                        {},
                        image, vk::ImageViewType::e2D, vk::Format::eB8G8R8A8Srgb,
                        {vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
                         vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity},
                        {vk::ImageAspectFlagBits::eColor,
                         0, 1,
                         0, 1}
                    })
            );
        }
        return views;
    }
}

si::vk_iface make_iface(vk::Instance vk, vk::SurfaceKHR surface) {
    std::vector<const char*> exts_required = { "VK_KHR_swapchain" };
    for (vk::PhysicalDevice& phy : vk.enumeratePhysicalDevices()) {
        std::vector<vk::ExtensionProperties> exts_avail = phy.enumerateDeviceExtensionProperties();
        //TODO: actually check if the required extensions are available
        if (true) {
            std::vector<vk::QueueFamilyProperties> queue_families = phy.getQueueFamilyProperties();
            std::vector<vk::DeviceQueueCreateInfo> queue_infos;
            const float queue_priority = 0.0f;
            std::optional<std::uint32_t> graphics_q_ix;
            std::optional<std::uint32_t> present_q_ix;
            for (std::size_t i = 0; i < queue_families.size(); i++) {
                queue_infos.push_back({{}, static_cast<std::uint32_t>(i), 0, &queue_priority});
                if (queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                    graphics_q_ix = i;
                    queue_infos[i].queueCount = 1;
                    spdlog::info("Found graphics queue family");
                }
                if (phy.getSurfaceSupportKHR(i, surface)) {
                    present_q_ix = i;
                    queue_infos[i].queueCount = 1;
                    spdlog::info("Found present queue family");
                }
            }
            if (graphics_q_ix && present_q_ix) {
                vk::PhysicalDeviceFeatures features = phy.getFeatures();
                vk::DeviceCreateInfo device_info (
                    {},
                    static_cast<std::uint32_t>(queue_infos.size()), queue_infos.data(),
                    0, nullptr, // device layers are deprecated
                    static_cast<std::uint32_t>(exts_required.size()), exts_required.data(),
                    &features
                );
                vk::UniqueDevice device = phy.createDeviceUnique(device_info);
                vk::Queue graphics_q = device->getQueue(*graphics_q_ix, 0);
                vk::Queue present_q = device->getQueue(*present_q_ix, 0);

                vk::SurfaceCapabilitiesKHR capabilities = phy.getSurfaceCapabilitiesKHR(surface);
                std::vector<vk::SurfaceFormatKHR> formats = phy.getSurfaceFormatsKHR(surface);
                std::vector<vk::PresentModeKHR> modes = phy.getSurfacePresentModesKHR(surface);

                if (std::find_if(formats.begin(), formats.end(),
                                 [](auto& fmt) {
                                     return fmt.format == vk::Format::eB8G8R8A8Srgb
                                         && fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                                 }) != formats.end()
                    && std::find(modes.begin(), modes.end(), vk::PresentModeKHR::eMailbox) != modes.end()) {
                    vk::UniqueSwapchainKHR swapchain = device->createSwapchainKHRUnique({
                            {},
                            surface,
                            capabilities.minImageCount + 1,
                            vk::Format::eB8G8R8A8Srgb,
                            vk::ColorSpaceKHR::eSrgbNonlinear,
                            {480, 360},
                            1,
                            vk::ImageUsageFlagBits::eColorAttachment,
                            vk::SharingMode::eExclusive,
                            0,
                            nullptr,
                            capabilities.currentTransform,
                            vk::CompositeAlphaFlagBitsKHR::eOpaque,
                            vk::PresentModeKHR::eMailbox,
                            true,
                            nullptr
                        });
                    return {phy, graphics_q, present_q, std::move(device), std::move(swapchain)};
                }
            }
        }
    }
    throw std::runtime_error("No suitable physical devices!");
}

si::vk_pipeline make_pipeline(vk::Device device) {
    vk::UniquePipelineLayout layout = device.createPipelineLayoutUnique({
            {},
            0, nullptr,
            0, nullptr});
    
    std::vector<vk::PipelineShaderStageCreateInfo> stages;
    std::vector<vk::UniqueShaderModule> shaders;
    for (auto [stage_name, stage] : std::vector {std::make_pair("vert", vk::ShaderStageFlagBits::eVertex),
                                                 std::make_pair("frag", vk::ShaderStageFlagBits::eFragment)}) {
        shaders.push_back(make_module(device, fmt::format("build/{}.spv", stage_name)));
        stages.emplace_back (
            vk::PipelineShaderStageCreateFlags(),
            stage,
            *shaders.back(),
            "main"
        );
    }
    const vk::PipelineVertexInputStateCreateInfo input_state ({}, 0, nullptr, 0, nullptr);
    const vk::PipelineInputAssemblyStateCreateInfo assembly_state ({}, vk::PrimitiveTopology::eTriangleList, false);
    const vk::Viewport viewport (0.0f, 0.0f, 360.0f, 480.0f, 0.0f, 1.0f);
    const vk::Rect2D scissor ({0, 0}, {360, 480});
    const vk::PipelineViewportStateCreateInfo viewport_state ({}, 1, &viewport, 1, &scissor);
    const vk::PipelineRasterizationStateCreateInfo rasterization_state (
        {}, false, false,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        false, 0.0f, 0.0f, 0.0f);
    const vk::PipelineMultisampleStateCreateInfo multisample_state (
        {}, vk::SampleCountFlagBits::e1, false, 1.0f, nullptr, false, false);
    const vk::PipelineColorBlendAttachmentState color_blend_attachment_state (
        false,
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );
    const vk::PipelineColorBlendStateCreateInfo color_blend_state (
        {}, false, vk::LogicOp::eCopy, 1, &color_blend_attachment_state, {0.0f, 0.0f, 0.0f, 0.0f});
    const vk::AttachmentDescription colour_attachment (
        {},
        vk::Format::eB8G8R8A8Srgb, vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
    const vk::AttachmentReference colour_attachment_ref (0, vk::ImageLayout::eColorAttachmentOptimal);
    const vk::SubpassDescription subpass (
        vk::SubpassDescriptionFlags{}, vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &colour_attachment_ref,
        nullptr,
        nullptr,
        0, nullptr);
    vk::UniqueRenderPass pass = device.createRenderPassUnique ({
            {},
            1, &colour_attachment,
            1, &subpass
        });
    vk::UniquePipeline pipeline = device.createGraphicsPipelineUnique (
        nullptr,
        vk::GraphicsPipelineCreateInfo (
            {},
            static_cast<std::uint32_t>(stages.size()),
            stages.data(),
            &input_state,
            &assembly_state,
            nullptr,
            &viewport_state,
            &rasterization_state,
            &multisample_state,
            nullptr,
            &color_blend_state,
            nullptr,
            *layout,
            *pass,
            0,
            nullptr,
            -1));
    return si::vk_pipeline {std::move(layout), std::move(pass), std::move(pipeline)};
}

si::vk_renderer::vk_renderer(wl::display& dpy, wl::surface& sfc):
    instance(make_instance()),
    debug_callback(attach_debug_reporter(static_cast<VkInstance>(*instance))),
    surface(make_surface(*instance, dpy, sfc)),
    iface(make_iface(*instance, *surface)),
    image_views(make_image_views(*iface.device, *iface.swapchain)),
    pipeline(make_pipeline(*iface.device)) {
}

