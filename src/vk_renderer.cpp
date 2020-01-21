#include <si/vk_renderer.hpp>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <cstdint>
#include <numeric>
#include <limits>
#include <chrono>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <si/util.hpp>

namespace {
    const std::vector<const char*> exts_required = { "VK_KHR_swapchain" };
    const auto win_image_format = ::vk::Format::eB8G8R8A8Srgb;
    const auto win_color_space = ::vk::ColorSpaceKHR::eSrgbNonlinear;

    ::vk::UniqueShaderModule make_module(::vk::Device device, std::string filename) {
        auto code = si::file_contents(filename);
        if (code.size() % 4 != 0) {
            throw std::runtime_error("Code size must be multiple of 4.");
        }
        return device.createShaderModuleUnique( {{}, code.size(), reinterpret_cast<std::uint32_t*>(code.data())} );
    }
}

::vk::VertexInputBindingDescription si::vk::vertex::binding_description = {
    .binding = 0,
    .stride = sizeof(vertex),
    .inputRate = ::vk::VertexInputRate::eVertex
};
std::array<::vk::VertexInputAttributeDescription, 3> si::vk::vertex::input_descriptions = {
    ::vk::VertexInputAttributeDescription {
        .location = 0,
        .binding = 0,
        .format = ::vk::Format::eR32G32Sfloat,
        .offset = offsetof(vertex, pos)
    },
    ::vk::VertexInputAttributeDescription {
        .location = 1,
        .binding = 0,
        .format = ::vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(vertex, color)
    },
    ::vk::VertexInputAttributeDescription {
        .location = 2,
        .binding = 0,
        .format = ::vk::Format::eR32G32Sfloat,
        .offset = offsetof(vertex, uv)
    }
};
void si::vk::renderer::reset_descriptor_set_layout() { 
    descriptor_set_layout = device.logical->createDescriptorSetLayoutUnique (
        ::vk::DescriptorSetLayoutCreateInfo {
            .flags = {},
            .bindingCount = static_cast<std::uint32_t>(descriptor_set_layout_bindings.size()),
            .pBindings = descriptor_set_layout_bindings.data()
        }
    );
}

void si::vk::renderer::reset_pipeline() {
    pipeline_layout = device.logical->createPipelineLayoutUnique (
        ::vk::PipelineLayoutCreateInfo {
            .flags = {},
            .setLayoutCount = 1,
            .pSetLayouts = std::to_address(descriptor_set_layout),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        }
    );
    std::vector<::vk::PipelineShaderStageCreateInfo> stages;
    std::vector<::vk::UniqueShaderModule> shaders;
    for (auto [stage_name, stage] : std::vector {std::make_pair("vert", ::vk::ShaderStageFlagBits::eVertex),
                                                 std::make_pair("frag", ::vk::ShaderStageFlagBits::eFragment)}) {
        shaders.push_back(make_module(*device.logical, fmt::format("build/{}.spv", stage_name)));
        stages.emplace_back (
            ::vk::PipelineShaderStageCreateFlags(),
            stage,
            *shaders.back(),
            "main"
        );
    }
    const ::vk::PipelineVertexInputStateCreateInfo input_state {
        .flags = {},
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vk::vertex::binding_description,
        .vertexAttributeDescriptionCount = vk::vertex::input_descriptions.size(),
        .pVertexAttributeDescriptions = vk::vertex::input_descriptions.data()
    };
    const ::vk::PipelineInputAssemblyStateCreateInfo assembly_state {
        .flags = {},
        .topology = ::vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false
    };
    const ::vk::PipelineViewportStateCreateInfo viewport_state {
        .flags = {},
        .viewportCount = 1,
        .pViewports = nullptr, // mutable viewport extension allows nullptr
        .scissorCount = 1,
        .pScissors = nullptr // mutable scissor extension allows nullptr
    };
    const ::vk::PipelineRasterizationStateCreateInfo rasterization_state {
        .flags = {},
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = ::vk::PolygonMode::eFill,
        .cullMode = ::vk::CullModeFlagBits::eBack,
        .frontFace = ::vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = false,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 0.0f
    };
    const ::vk::PipelineMultisampleStateCreateInfo multisample_state {
        .flags = {},
        .rasterizationSamples = ::vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = false,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false
    };
    const ::vk::PipelineColorBlendAttachmentState color_blend_attachment_state {
        .blendEnable = false,
        .srcColorBlendFactor = ::vk::BlendFactor::eOne,
        .dstColorBlendFactor = ::vk::BlendFactor::eZero,
        .colorBlendOp = ::vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = ::vk::BlendFactor::eZero,
        .dstAlphaBlendFactor = ::vk::BlendFactor::eZero,
        .alphaBlendOp = ::vk::BlendOp::eAdd,
        .colorWriteMask = ::vk::ColorComponentFlagBits::eR
                        | ::vk::ColorComponentFlagBits::eG
                        | ::vk::ColorComponentFlagBits::eB
                        | ::vk::ColorComponentFlagBits::eA,
    };
    const ::vk::PipelineColorBlendStateCreateInfo color_blend_state {
        .flags = {},
        .logicOpEnable = false,
        .logicOp = ::vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
    };
    const ::vk::AttachmentDescription colour_attachment {
        .flags = {},
        .format = ::vk::Format::eB8G8R8A8Srgb,
        .samples = ::vk::SampleCountFlagBits::e1,
        .loadOp = ::vk::AttachmentLoadOp::eClear,
        .storeOp = ::vk::AttachmentStoreOp::eStore,
        .stencilLoadOP = ::vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOP = ::vk::AttachmentStoreOp::eDontCare,
        .initialLayout = ::vk::ImageLayout::eUndefined,
        .finalLayout = ::vk::ImageLayout::ePresentSrcKHR
    };
    const ::vk::AttachmentReference colour_attachment_ref {
        .attachment = 0,
        .layout = ::vk::ImageLayout::eColorAttachmentOptimal
    };
    const ::vk::SubpassDescription subpass {
        .flags = ::vk::SubpassDescriptionFlags{},
        .pipelineBindPoint = ::vk::PipelineBindPoint::eGraphics,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colour_attachment_ref,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };
    const ::vk::SubpassDependency subpass_dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = ::vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = ::vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .srcAccessMask = {},
        .dstAccessMask = ::vk::AccessFlagBits::eColorAttachmentRead | ::vk::AccessFlagBits::eColorAttachmentWrite
    };
    render_pass = device.logical->createRenderPassUnique (
        ::vk::RenderPassCreateInfo {
            .flags = {},
            .attachmentCount = 1,
            .pAttachments = &colour_attachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &subpass_dependency
        }
    );
    const std::array dynamic_states = {::vk::DynamicState::eViewport, ::vk::DynamicState::eScissor};
    const ::vk::PipelineDynamicStateCreateInfo dynamic_state {
        .flags = {},
        .dynamicStateCount = dynamic_states.size(),
        .pDynamicStates = dynamic_states.data()
    };
    pipeline = device.logical->createGraphicsPipelineUnique (
        nullptr,
        ::vk::GraphicsPipelineCreateInfo {
            .flags = {},
            .stageCount = static_cast<std::uint32_t>(stages.size()),
            .pStages = stages.data(),
            .pVertexInputState = &input_state,
            .pInputAssemblyState = &assembly_state,
            .pTesselationState = nullptr,
            .pViewportState = &viewport_state,
            .pRasterizationState = &rasterization_state,
            .pMultisampleState = &multisample_state,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &color_blend_state,
            .pDynamicState = &dynamic_state,
            .layout = *pipeline_layout,
            .renderPass = *render_pass,
            .subPass = 0,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1
        }
    );
}
void si::vk::renderer::reset_swapchain(std::uint32_t width, std::uint32_t height) {
    ::vk::SurfaceCapabilitiesKHR caps = device.physical.getSurfaceCapabilitiesKHR(*surface);

    std::vector<::vk::SurfaceFormatKHR> formats = device.physical.getSurfaceFormatsKHR(*surface);
    spdlog::info("Available formats:");
    for (auto format : formats) { spdlog::info(" * {}/{}", to_string(format.format), to_string(format.colorSpace)); }

    std::vector<::vk::PresentModeKHR> modes = device.physical.getSurfacePresentModesKHR(*surface);
    spdlog::info("Available modes:");
    for (auto mode : modes) { spdlog::info(" * {}", to_string(mode)); }

    swapchain_extent = ::vk::Extent2D {width, height};
    swapchain = device.logical->createSwapchainKHRUnique (
        ::vk::SwapchainCreateInfoKHR { 
            .flags = {},
            .surface = *surface,
            .minImageCount = caps.minImageCount + 1,
            .imageFormat = win_image_format,
            .imageColorSpace = win_color_space,
            .imageExtent = swapchain_extent,
            .imageArrayLayers = 1,
            .imageUsage = ::vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = ::vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = caps.currentTransform,
            .compositeAlpha = ::vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = ::vk::PresentModeKHR::eMailbox,
            .clipped = true,
            .oldSwapchain = nullptr
        }
    );
}
void si::vk::renderer::reset_swapchain_images() {
    swapchain_images = device.logical->getSwapchainImagesKHR(*swapchain);
    swapchain_image_views.clear();
    swapchain_image_views.reserve(swapchain_images.size());
    for (::vk::Image& img : swapchain_images) {
        swapchain_image_views.push_back (
            device.logical->createImageViewUnique (
                ::vk::ImageViewCreateInfo {
                    .flags = {},
                    .image = img,
                    .viewType = ::vk::ImageViewType::e2D,
                    .format = win_image_format,
                    .components = ::vk::ComponentMapping {
                        .r = ::vk::ComponentSwizzle::eIdentity,
                        .g = ::vk::ComponentSwizzle::eIdentity,
                        .b = ::vk::ComponentSwizzle::eIdentity,
                        .a = ::vk::ComponentSwizzle::eIdentity
                    },
                    .subresourceRange = {
                        .aspectMask = ::vk::ImageAspectFlagBits::eColor,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                    }
                }
            )
        );
    }
}
std::uint32_t si::vk::gfx_device::find_memory_type_index(::vk::MemoryRequirements reqs, ::vk::MemoryPropertyFlags flags) {
    ::vk::PhysicalDeviceMemoryProperties mem_props = physical.getMemoryProperties();
    for (std::uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if (reqs.memoryTypeBits & (i << 1) // see if the memory is of the ith type
            && (mem_props.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }
    throw std::runtime_error("Can't find memory type satisfying given properties or requirements");
}
std::tuple<::vk::UniqueBuffer, ::vk::UniqueDeviceMemory> si::vk::gfx_device::make_buffer (
    ::vk::DeviceSize buffer_size,
    ::vk::BufferUsageFlags buffer_usage,
    ::vk::SharingMode sharing_mode,
    ::vk::MemoryPropertyFlags memory_flags
) {
    ::vk::UniqueBuffer buffer = logical->createBufferUnique (
        ::vk::BufferCreateInfo {
            .flags = {},
            .size = buffer_size,
            .usage = buffer_usage,
            .sharingMode = sharing_mode,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr
        }
    );
    ::vk::MemoryRequirements memory_reqs = logical->getBufferMemoryRequirements(*buffer);
    std::uint32_t memory_type = find_memory_type_index(memory_reqs, memory_flags);
    ::vk::UniqueDeviceMemory buffer_memory = logical->allocateMemoryUnique (
        ::vk::MemoryAllocateInfo {
            .allocationSize = memory_reqs.size,
            .memoryTypeIndex = memory_type
        },
        nullptr
    );
    logical->bindBufferMemory(*buffer, *buffer_memory, 0);
    return std::make_tuple(std::move(buffer), std::move(buffer_memory));
}
void si::vk::renderer::reset_vertex_buffer() {
    auto [staging_buffer, staging_buffer_memory, size] = stage(vertices.begin(), vertices.end());
    std::tie(vertex_buffer, vertex_buffer_memory) = device.make_buffer (
        size,
        ::vk::BufferUsageFlagBits::eVertexBuffer | ::vk::BufferUsageFlagBits::eTransferDst,
        ::vk::SharingMode::eExclusive,
        ::vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    copy(*staging_buffer, *vertex_buffer, {0, 0, size});
}
void si::vk::renderer::reset_index_buffer() {
    auto [staging_buffer, staging_buffer_memory, size] = stage(indices.begin(), indices.end());
    std::tie(index_buffer, index_buffer_memory) = device.make_buffer (
        size,
        ::vk::BufferUsageFlagBits::eIndexBuffer | ::vk::BufferUsageFlagBits::eTransferDst,
        ::vk::SharingMode::eExclusive,
        ::vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    copy(*staging_buffer, *index_buffer, {0, 0, size});
}
void si::vk::renderer::reset_uniform_buffers() {
    uniform_buffers.clear();
    uniform_buffers.resize(swapchain_images.size());
    uniform_buffer_memories.clear();
    uniform_buffer_memories.resize(swapchain_images.size());
    for (unsigned i = 0; i < swapchain_images.size(); i++) {
        std::tie(uniform_buffers[i], uniform_buffer_memories[i]) = device.make_buffer (
            ::vk::DeviceSize { sizeof(uniform_buffer_object) },
            ::vk::BufferUsageFlagBits::eUniformBuffer,
            ::vk::SharingMode::eExclusive,
            ::vk::MemoryPropertyFlagBits::eHostVisible | ::vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }
}
void si::vk::renderer::reset_descriptor_pool() {
    auto pool_sizes = std::array {
        // UBO pool
        ::vk::DescriptorPoolSize {
            .type = ::vk::DescriptorType::eUniformBuffer,
            .descriptorCount = static_cast<std::uint32_t>(swapchain_images.size())
        },
        // Sampler pool
        ::vk::DescriptorPoolSize {
            .type = ::vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = static_cast<std::uint32_t>(swapchain_images.size())
        }
    };
    descriptor_pool = device.logical->createDescriptorPoolUnique (
        ::vk::DescriptorPoolCreateInfo {
            .flags = {},
            .maxSets = static_cast<std::uint32_t>(swapchain_images.size()),
            .poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data()
        }
    );
}
void si::vk::renderer::reset_descriptor_sets() {
    std::vector<::vk::DescriptorSetLayout> descriptor_set_layouts(swapchain_images.size(), *descriptor_set_layout);
    descriptor_sets = device.logical->allocateDescriptorSets (
        ::vk::DescriptorSetAllocateInfo {
            .descriptorPool = *descriptor_pool,
            .descriptorSetCount = static_cast<std::uint32_t>(swapchain_images.size()),
            .pSetLayouts = descriptor_set_layouts.data()
        }
    );
    for (std::size_t i = 0 ; i < swapchain_images.size(); i++) {
        auto buffer_info = ::vk::DescriptorBufferInfo {
            .buffer = *uniform_buffers[i],
            .offset = 0,
            .range = sizeof(uniform_buffer_object)
        };
        auto image_info = ::vk::DescriptorImageInfo {
            .sampler = *texture_sampler,
            .imageView = *texture_image_view,
            .imageLayout = ::vk::ImageLayout::eShaderReadOnlyOptimal
        };
        device.logical->updateDescriptorSets (
            std::array {
                ::vk::WriteDescriptorSet {
                     .dstset = descriptor_sets[i],
                     .dstBinding = 0,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = ::vk::DescriptorType::eUniformBuffer,
                     .pImageInfo = nullptr,
                     .pBufferInfo = &buffer_info,
                     .pTexelBufferView = nullptr
                },
                ::vk::WriteDescriptorSet {
                     .dstSet = descriptor_sets[i],
                     .dstBinding = 1,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = ::vk::DescriptorType::eCombinedImageSampler,
                     .pImageInfo = &image_info,
                     .pBufferInfo = nullptr,
                     .pTexelBufferView = nullptr
                 }
            },
            std::array<::vk::CopyDescriptorSet, 0> {}
        );
    }
}

#include <FreeImage.h>
//TODO: Make more robust
struct bitmap {
    const unsigned width;
    const unsigned height;
    const unsigned depth;
    std::vector<unsigned char> data;
    const ::vk::Format format = ::vk::Format::eB8G8R8A8Srgb;
    const unsigned char* begin() const {
        return std::to_address(data.begin());
    }
    const unsigned char* end() const {
        return std::to_address(data.end());
    }
};
bitmap load_bitmap(std::string filepath) {
    spdlog::debug("Using FreeImage version {}", FreeImage_GetVersion());
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(filepath.c_str());
    if (!format) {
        throw std::runtime_error("Couldn't determine image format!");
    } else {
        const char* format_mime = FreeImage_GetFIFMimeType(format);
        spdlog::info("Loading {} as {}", filepath, format_mime);
    }
    std::unique_ptr<FIBITMAP, decltype(&FreeImage_Unload)> bitmap{FreeImage_Load(format, filepath.c_str()), FreeImage_Unload};
    if (!bitmap) {
        bitmap.release();
        throw std::runtime_error("Couldn't load image!");
    }
    const unsigned width = FreeImage_GetWidth(bitmap.get());
    const unsigned height = FreeImage_GetHeight(bitmap.get());
    const unsigned depth = FreeImage_GetBPP(bitmap.get());
    std::vector<unsigned char> raw(width * height * depth);
    const unsigned pitch = FreeImage_GetPitch(bitmap.get());
    FreeImage_ConvertToRawBits(raw.data(), bitmap.get(), pitch, depth, FI_RGBA_BLUE_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_RED_MASK, FALSE);
    return {width, height, depth, std::move(raw)};
}
void si::vk::renderer::image_layout_stage0(::vk::Image& img, ::vk::Format format) {
    auto barrier = ::vk::ImageMemoryBarrier {
        .srcAccessMask = ::vk::AccessFlags{},
        .dstAccessMask = ::vk::AccessFlagBits::eTransferWrite,
        .oldLayout = ::vk::ImageLayout::eUndefined,
        .newLayout = ::vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = 0,
        .dstQueueFamilyIndex = 0,
        .image = img,
        .subresourceRange = ::vk::ImageSubresourceRange {
            .aspectMask = ::vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    run_oneshot (
        [&](::vk::CommandBuffer& cmd) {
            cmd.pipelineBarrier (
                ::vk::PipelineStageFlagBits::eTopOfPipe, ::vk::PipelineStageFlagBits::eTransfer,
                ::vk::DependencyFlags{},
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }
    );
}
void si::vk::renderer::image_layout_stage1(::vk::Image& img, ::vk::Format format) {
    auto barrier = ::vk::ImageMemoryBarrier {
        .srcAccessMask = ::vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = ::vk::AccessFlagBits::eShaderRead,
        .oldLayout = ::vk::ImageLayout::eTransferDstOptimal,
        .newLayout = ::vk::ImageLayout::eShaderReadOnlyOptimal,
        .srcQueueFamilyIndex = 0,
        .dstQueueFamilyIndex = 0,
        .image = img,
        .subresourceRange = ::vk::ImageSubresourceRange {
            .aspectMask = ::vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    run_oneshot([&](::vk::CommandBuffer& cmd) {
        cmd.pipelineBarrier (
            ::vk::PipelineStageFlagBits::eTransfer, ::vk::PipelineStageFlagBits::eFragmentShader,
            ::vk::DependencyFlags{},
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    });
}
void si::vk::renderer::reset_texture_image(std::string filepath) {
    auto bmp = load_bitmap(filepath);
    auto [staging_buffer, staging_buffer_memory, size] = stage(bmp.begin(), bmp.end());
    texture_image = device.logical->createImageUnique (
        ::vk::ImageCreateInfo {
            .flags = {},
            .imageType = ::vk::ImageType::e2D,
            .format = bmp.format,
            .extent = {bmp.width, bmp.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = ::vk::SampleCountFlagBits::e1,
            .tiling = ::vk::ImageTiling::eOptimal,
            .usage = ::vk::ImageUsageFlagBits::eTransferDst | ::vk::ImageUsageFlagBits::eSampled,
            .sharingMode = ::vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = ::vk::ImageLayout::eUndefined
        }
    );
    ::vk::MemoryRequirements memory_reqs = device.logical->getImageMemoryRequirements(*texture_image);
    std::uint32_t memory_type = device.find_memory_type_index(memory_reqs, ::vk::MemoryPropertyFlagBits::eDeviceLocal);
    texture_image_memory = device.logical->allocateMemoryUnique (
        ::vk::MemoryAllocateInfo {
            .allocationSize = memory_reqs.size,
            .memoryTypeIndex = memory_type
        },
        nullptr
    );
    device.logical->bindImageMemory(*texture_image, *texture_image_memory, {});
    image_layout_stage0(*texture_image, bmp.format);
    copy (
        *staging_buffer,
        *texture_image,
        ::vk::BufferImageCopy {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = ::vk::ImageSubresourceLayers {
                .aspectMask = ::vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {bmp.width, bmp.height, 1},
        }
    );
    image_layout_stage1(*texture_image, bmp.format);
    texture_image_view = device.logical->createImageViewUnique (
        ::vk::ImageViewCreateInfo {
            .flags = {},
            .image = *texture_image,
            .viewType = ::vk::ImageViewType::e2D,
            .format = bmp.format,
            .components = ::vk::ComponentMapping{},
            .subresourceRange = ::vk::ImageSubresourceRange {
                .aspectMask = ::vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        }
    );
    texture_sampler = device.logical->createSamplerUnique (
        ::vk::SamplerCreateInfo {
            .flags = ::vk::SamplerCreateFlags{},
            .magFilter = ::vk::Filter::eLinear,
            .minFilter = ::vk::Filter::eLinear,
            .mipmapMode = ::vk::SamplerMipmapMode::eLinear,
            .addressModeU = ::vk::SamplerAddressMode::eRepeat,
            .addressModeV = ::vk::SamplerAddressMode::eRepeat,
            .addressModeW = ::vk::SamplerAddressMode::eRepeat,
            .mipLodBias = 0.0f,
            .anisotropyEnable = true,
            .maxAnisotropy = 16,
            .compareEnable = false,
            .compareOp = ::vk::CompareOp::eAlways,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = ::vk::BorderColor::eIntOpaqueBlack,
            .unnormalizedCoordinates = false
        }
    );
}
void si::vk::renderer::reset_framebuffers(std::uint32_t width, std::uint32_t height) {
    framebuffers.clear();
    framebuffers.reserve(swapchain_image_views.size());
    for (::vk::UniqueImageView& view_ptr : swapchain_image_views) {
        framebuffers.push_back (
            device.logical->createFramebufferUnique (
                ::vk::FramebufferCreateInfo {
                    .flags = {},
                    .renderPass = *render_pass,
                    .attachmentCount = 1,
                    .pAttachments = &*view_ptr,
                    .width = width,
                    .height = height,
                    .layers = 1
                }
            )
        );
    }
}
void si::vk::renderer::reset_command_buffers(std::uint32_t width, std::uint32_t height) {
    command_buffers = device.logical->allocateCommandBuffersUnique (
        ::vk::CommandBufferAllocateInfo {
            .commandPool = *graphics_command_pool,
            .level = ::vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = static_cast<std::uint32_t>(framebuffers.size())
        }
    );
    for (std::size_t i = 0; i < command_buffers.size(); i++) {
        ::vk::CommandBuffer& cmd = *command_buffers[i];
        cmd.begin(::vk::CommandBufferBeginInfo {});
        const ::vk::ClearValue clear_color {
            .color = ::vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 0.0f}}
        };
        const ::vk::Rect2D scissor ({0, 0}, {width, height});
        cmd.beginRenderPass (
            ::vk::RenderPassBeginInfo {
                .renderPass = *render_pass,
                .framebuffer = *framebuffers[i],
                .renderArea = scissor,
                .clearValueCount = 1,
                .pClearValues = &clear_color
            },
            ::vk::SubpassContents::eInline
        );
        cmd.bindPipeline(::vk::PipelineBindPoint::eGraphics, *pipeline);
        const auto viewport = ::vk::Viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        cmd.setViewport(0, viewport);
        cmd.setScissor(0, scissor);
        std::array<::vk::Buffer, 1> buffers = { *vertex_buffer };
        std::array<::vk::DeviceSize, 1> offsets = { 0 };
        cmd.bindVertexBuffers(0, 1, buffers.data(), offsets.data());
        cmd.bindIndexBuffer(*index_buffer, ::vk::DeviceSize {0}, ::vk::IndexType::eUint16);
        cmd.bindDescriptorSets(::vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr);
        cmd.drawIndexed(indices.size(), 1, 0, 0, 0);
        cmd.endRenderPass();
        cmd.end();
    }
}
void si::vk::renderer::update_uniform_buffers(unsigned ix) {
    using clock = std::chrono::high_resolution_clock;
    static auto start = clock::now();
    auto now = clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(now - start).count();
    uniform_buffer_object ubo = {
        .model = glm::mat4{1.0f}, //glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::mat4{1.0f}, //glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::mat4{1.0f}, //glm::perspective(glm::radians(45.0f), swapchain_extent.width / static_cast<float>(swapchain_extent.height), 0.1f, 10.0f)
    };
    ubo.proj[1][1] *= -1.0f; // glm's y clip-space axis is opposite to vulkans
    void* data = device.logical->mapMemory(*uniform_buffer_memories[ix], 0, sizeof(ubo));
    std::copy(&ubo, &ubo + 1, reinterpret_cast<uniform_buffer_object*>(data));
    device.logical->unmapMemory(*uniform_buffer_memories[ix]);
}
void si::vk::renderer::copy(::vk::Buffer src, ::vk::Buffer dst, ::vk::BufferCopy what) {
    run_oneshot(
        [&](::vk::CommandBuffer& cmd) {
            cmd.copyBuffer(src, dst, what);
        }
    );
}
void si::vk::renderer::copy(::vk::Buffer src, ::vk::Image dst, ::vk::BufferImageCopy what) {
    run_oneshot(
        [&](::vk::CommandBuffer& cmd) {
            cmd.copyBufferToImage(src, dst, ::vk::ImageLayout::eTransferDstOptimal, what);
        }
    );
}
si::vk::renderer::renderer(si::vk::gfx_device& device, ::vk::UniqueSurfaceKHR old_surface, std::uint32_t width, std::uint32_t height):
    device(device),
    swapchain_image_available(device.logical->createSemaphoreUnique({})),
    render_finished(device.logical->createSemaphoreUnique({})),
    in_flight(device.logical->createFenceUnique({::vk::FenceCreateFlagBits::eSignaled})),
    surface(std::move(old_surface)) {
    reset_descriptor_set_layout();
    reset_pipeline();
    graphics_command_pool = device.logical->createCommandPoolUnique({{}, device.graphics_q_family_ix});
    reset_swapchain(width, height);
    reset_swapchain_images();
    reset_framebuffers(width, height);
    reset_vertex_buffer();
    reset_index_buffer();
    reset_uniform_buffers();
    reset_descriptor_pool();
    reset_texture_image("wintex2.png");
    reset_descriptor_sets();
    reset_command_buffers(width, height);
}

void si::vk::renderer::resize(std::uint32_t width, std::uint32_t height) {
    device.logical->waitIdle();
    reset_swapchain(width, height);
    reset_swapchain_images();
    reset_framebuffers(width, height);
    reset_uniform_buffers();
    reset_descriptor_pool();
    reset_descriptor_sets();
    reset_command_buffers(width, height);
}

si::vk::renderer::~renderer() {
    device.logical->waitIdle();
}

void si::vk::renderer::draw() {
    device.logical->waitForFences(*in_flight, true, std::numeric_limits<std::uint64_t>::max());
    device.logical->resetFences(*in_flight);
    auto [result, swapchain_image_ix] = device.logical->acquireNextImageKHR (
        *swapchain,
        std::numeric_limits<std::uint64_t>::max(),
        *swapchain_image_available,
        nullptr
    );
    update_uniform_buffers(swapchain_image_ix);
    ::vk::PipelineStageFlags pipeline_stage = ::vk::PipelineStageFlagBits::eColorAttachmentOutput;
    device.graphics_q.submit (
        ::vk::SubmitInfo {
            1, &*swapchain_image_available,
            &pipeline_stage,
            1, &*command_buffers[swapchain_image_ix],
            1, &*render_finished
        },
        *in_flight
    );
    device.present_q.presentKHR (
        ::vk::PresentInfoKHR {
            1, &*render_finished,
            1, &*swapchain,
            &swapchain_image_ix
        }
    );
}

si::vk::gfx_device::gfx_device(::vk::PhysicalDevice physical, ::vk::Queue graphics_q, std::uint32_t graphics_q_family_ix, ::vk::Queue present_q, std::uint32_t present_q_family_ix, ::vk::UniqueDevice device):
    physical(physical),
    graphics_q(graphics_q),
    graphics_q_family_ix(graphics_q_family_ix),
    present_q(present_q),
    present_q_family_ix(present_q_family_ix),
    logical(std::move(device)) {
}

std::unique_ptr<si::vk::renderer> si::vk::gfx_device::make_renderer(::vk::UniqueSurfaceKHR surface, std::uint32_t width, std::uint32_t height) {
    return std::make_unique<si::vk::renderer>(*this, std::move(surface), width, height);
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
std::unique_ptr<si::vk::renderer> si::vk::root::make_renderer(::wl::display& display, ::wl::surface& surface, std::uint32_t width, std::uint32_t height) {
    // Optimistically make a vulkan surface
    ::vk::UniqueSurfaceKHR vk_surface = instance->createWaylandSurfaceKHRUnique ({
        {}, static_cast<wl_display*>(display), static_cast<wl_surface*>(surface)
    });
    auto gfx_it = std::find_if(gfxs.begin(), gfxs.end(), [&](const si::vk::gfx_device& gfx) {
        return gfx.physical.getSurfaceSupportKHR(gfx.present_q_family_ix, *vk_surface);
    });
    if (gfx_it != gfxs.end()) {
        return gfx_it->make_renderer(std::move(vk_surface), width, height);
    } else {
        for (::vk::PhysicalDevice& physical : instance->enumeratePhysicalDevices()) {
            auto props = physical.getProperties();
            spdlog::debug("Device max viewports: {} up to {}x{}", props.limits.maxViewports, props.limits.maxViewportDimensions[0], props.limits.maxViewportDimensions[1]);
            std::vector<::vk::ExtensionProperties> exts_avail = physical.enumerateDeviceExtensionProperties();
            //TODO: actually check if the required extensions are available
            //TODO: check for anisotropy support
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
                    return created.make_renderer(std::move(vk_surface), width, height);
                }
            }
        }
        throw std::runtime_error("Can't create a renderer for supplied wayland display/surface");
    }
}
