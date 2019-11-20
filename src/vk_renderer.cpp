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
    ::vk::VertexInputAttributeDescription {0, 0, ::vk::Format::eR32G32Sfloat, offsetof(vertex, pos) },
    ::vk::VertexInputAttributeDescription {1, 0, ::vk::Format::eR32G32B32Sfloat, offsetof(vertex, color) },
    ::vk::VertexInputAttributeDescription {2, 0, ::vk::Format::eR32G32Sfloat, offsetof(vertex, uv) }
};
void si::vk::renderer::reset_descriptor_set_layout() { 
    descriptor_set_layout = device.logical->createDescriptorSetLayoutUnique({
        {},
        static_cast<std::uint32_t>(descriptor_set_layout_bindings.size()),
        descriptor_set_layout_bindings.data()
    });
}

void si::vk::renderer::reset_pipeline() {
    pipeline_layout = device.logical->createPipelineLayoutUnique(::vk::PipelineLayoutCreateInfo(
        {},
        1, std::to_address(descriptor_set_layout), // descriptor set layouts
        0, nullptr // push constant ranges
    ));
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
    const ::vk::PipelineVertexInputStateCreateInfo input_state (
        {},
        1, &vk::vertex::binding_description,
        vk::vertex::input_descriptions.size(), vk::vertex::input_descriptions.data()
    );
    const ::vk::PipelineInputAssemblyStateCreateInfo assembly_state ({}, ::vk::PrimitiveTopology::eTriangleList, false);
    const ::vk::PipelineViewportStateCreateInfo viewport_state ({}, 1, nullptr, 1, nullptr);
    const ::vk::PipelineRasterizationStateCreateInfo rasterization_state (
        {}, false, false,
        ::vk::PolygonMode::eFill,
        ::vk::CullModeFlagBits::eBack,
        ::vk::FrontFace::eCounterClockwise,
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
    render_pass = device.logical->createRenderPassUnique ({
            {},
            1, &colour_attachment,
            1, &subpass,
            1, &subpass_dependency
        });
    const std::array dynamic_states = {::vk::DynamicState::eViewport, ::vk::DynamicState::eScissor};
    const ::vk::PipelineDynamicStateCreateInfo dynamic_state {{}, dynamic_states.size(), dynamic_states.data() };
    pipeline = device.logical->createGraphicsPipelineUnique (
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
    swapchain = device.logical->createSwapchainKHRUnique ({
        {},
        *surface,
        caps.minImageCount + 1,
        win_image_format, win_color_space, swapchain_extent,
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
}
void si::vk::renderer::reset_swapchain_images() {
    swapchain_images = device.logical->getSwapchainImagesKHR(*swapchain);
    swapchain_image_views.clear();
    swapchain_image_views.reserve(swapchain_images.size());
    for (::vk::Image& img : swapchain_images) {
        swapchain_image_views.push_back (
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
    ::vk::UniqueBuffer buffer = logical->createBufferUnique(::vk::BufferCreateInfo {
        {},
        buffer_size,
        buffer_usage,
        sharing_mode
    });
    ::vk::MemoryRequirements memory_reqs = logical->getBufferMemoryRequirements(*buffer);
    std::uint32_t memory_type = find_memory_type_index(memory_reqs, memory_flags);
    ::vk::UniqueDeviceMemory buffer_memory = logical->allocateMemoryUnique (
        ::vk::MemoryAllocateInfo { memory_reqs.size, memory_type },
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
        // ubo pool
        ::vk::DescriptorPoolSize{}
        .setType(::vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(static_cast<std::uint32_t>(swapchain_images.size())),
        // sampler pool
        ::vk::DescriptorPoolSize{}
        .setType(::vk::DescriptorType::eCombinedImageSampler)
        .setDescriptorCount(static_cast<std::uint32_t>(swapchain_images.size()))
    };
    descriptor_pool = device.logical->createDescriptorPoolUnique( ::vk::DescriptorPoolCreateInfo {
        {},
        static_cast<std::uint32_t>(swapchain_images.size()),
        static_cast<std::uint32_t>(pool_sizes.size()),
        pool_sizes.data()
    });
}
void si::vk::renderer::reset_descriptor_sets() {
    std::vector<::vk::DescriptorSetLayout> descriptor_set_layouts(swapchain_images.size(), *descriptor_set_layout);
    descriptor_sets = device.logical->allocateDescriptorSets(::vk::DescriptorSetAllocateInfo {
        *descriptor_pool,
        static_cast<std::uint32_t>(swapchain_images.size()),
        descriptor_set_layouts.data()
    });
    for (std::size_t i = 0 ; i < swapchain_images.size(); i++) {
        auto buffer_info = ::vk::DescriptorBufferInfo { *uniform_buffers[i], 0, sizeof(uniform_buffer_object) };
        auto image_info = ::vk::DescriptorImageInfo()
            .setImageLayout(::vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(*texture_image_view)
            .setSampler(*texture_sampler);
        device.logical->updateDescriptorSets (
            std::array {
                // uniform buffer
                ::vk::WriteDescriptorSet{}
                     .setDstSet(descriptor_sets[i])
                     .setDstBinding(0)
                     .setDstArrayElement(0)
                     .setDescriptorCount(1)
                     .setDescriptorType(::vk::DescriptorType::eUniformBuffer)
                     .setPBufferInfo(&buffer_info),
                ::vk::WriteDescriptorSet{}
                     .setDstSet(descriptor_sets[i])
                     .setDstBinding(1)
                     .setDstArrayElement(0)
                     .setDescriptorCount(1)
                     .setDescriptorType(::vk::DescriptorType::eCombinedImageSampler)
                     .setPImageInfo(&image_info)
            },
            std::array<::vk::CopyDescriptorSet, 0> {}
        );
    }
}
#include <FreeImage.h>
//TODO: Make more robust
// image, start, end
std::tuple<std::unique_ptr<FIBITMAP, void(*)(FIBITMAP*)>, ::vk::Format, unsigned, unsigned, BYTE*, BYTE*> load_bitmap(std::string filepath) {
    spdlog::debug("Using FreeImage version {}", FreeImage_GetVersion());
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(filepath.c_str());
    if (!format) {
        throw std::runtime_error("Couldn't determine image format!");
    }
    FIBITMAP* bitmap = FreeImage_Load(format, filepath.c_str());
    if (!bitmap) {
        throw std::runtime_error("Couldn't load image!");
    }
    FREE_IMAGE_TYPE type = FreeImage_GetImageType(bitmap);
    FREE_IMAGE_COLOR_TYPE color_type = FreeImage_GetColorType(bitmap);
    unsigned bit_depth = FreeImage_GetBPP(bitmap);
    if (type != FIT_BITMAP || color_type != FIC_RGBALPHA || bit_depth != 32) {
        throw std::runtime_error("Image is either not rgb, not a bitmap or not 32 bits per pixel. Aborting.");
        //TODO: convert RGB to RGBA
    }
    BYTE* bitmap_begin = FreeImage_GetBits(bitmap);
    if (!bitmap_begin) {
        throw std::runtime_error("Image does not contain pixel data. Aborting.");
    }
    unsigned width = FreeImage_GetWidth(bitmap);
    unsigned height = FreeImage_GetHeight(bitmap);
    unsigned Bpp = bit_depth / ((sizeof(*bitmap_begin) * CHAR_BIT));
    BYTE* bitmap_end = bitmap_begin + width * height * Bpp;
    return std::make_tuple (
        std::unique_ptr<FIBITMAP, void(*)(FIBITMAP*)>{bitmap, &FreeImage_Unload},
        ::vk::Format::eR8G8B8A8Unorm,
        width, height,
        bitmap_begin, bitmap_end
    );
}
void si::vk::renderer::image_layout_stage0(::vk::Image& img, ::vk::Format format) {
    auto barrier = ::vk::ImageMemoryBarrier {
        ::vk::AccessFlags{}, ::vk::AccessFlagBits::eTransferWrite,
        ::vk::ImageLayout::eUndefined, ::vk::ImageLayout::eTransferDstOptimal,
        0, 0,
        img,
        ::vk::ImageSubresourceRange {
            ::vk::ImageAspectFlagBits::eColor,
            0, 1,
            0, 1
        }
    };
    run_oneshot(
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
        ::vk::AccessFlagBits::eTransferWrite, ::vk::AccessFlagBits::eShaderRead,
        ::vk::ImageLayout::eUndefined, ::vk::ImageLayout::eShaderReadOnlyOptimal,
        0, 0,
        img,
        ::vk::ImageSubresourceRange {
            ::vk::ImageAspectFlagBits::eColor,
            0, 1,
            0, 1
        }
    };
    run_oneshot(
        [&](::vk::CommandBuffer& cmd) {
            cmd.pipelineBarrier (
                ::vk::PipelineStageFlagBits::eTransfer, ::vk::PipelineStageFlagBits::eFragmentShader,
                ::vk::DependencyFlags{},
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }
    );
}
void si::vk::renderer::reset_texture_image(std::string filepath) {
    auto [bitmap, bitmap_format, bitmap_width, bitmap_height, bitmap_begin, bitmap_end] = load_bitmap(filepath);
    auto [staging_buffer, staging_buffer_memory, size] = stage(bitmap_begin, bitmap_end);
    texture_image = device.logical->createImageUnique (
        ::vk::ImageCreateInfo {}
        .setImageType(::vk::ImageType::e2D)
        .setFormat(bitmap_format)
        .setExtent({bitmap_width, bitmap_height, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(::vk::SampleCountFlagBits::e1)
        .setTiling(::vk::ImageTiling::eOptimal)
        .setUsage(::vk::ImageUsageFlagBits::eTransferDst | ::vk::ImageUsageFlagBits::eSampled)
        .setSharingMode(::vk::SharingMode::eExclusive)
        .setInitialLayout(::vk::ImageLayout::eUndefined)
    );
    ::vk::MemoryRequirements memory_reqs = device.logical->getImageMemoryRequirements(*texture_image);
    std::uint32_t memory_type = device.find_memory_type_index(memory_reqs, ::vk::MemoryPropertyFlagBits::eDeviceLocal);
    texture_image_memory = device.logical->allocateMemoryUnique (
        ::vk::MemoryAllocateInfo { memory_reqs.size, memory_type },
        nullptr
    );
    device.logical->bindImageMemory(*texture_image, *texture_image_memory, {});

    image_layout_stage0(*texture_image, bitmap_format);
    copy (
        *staging_buffer,
        *texture_image,
        ::vk::BufferImageCopy{}
            .setBufferOffset(0)
            .setBufferRowLength(0)
            .setBufferImageHeight(0)
            .setImageSubresource (::vk::ImageSubresourceLayers{}
                .setAspectMask(::vk::ImageAspectFlagBits::eColor)
                .setMipLevel(0)
                .setBaseArrayLayer(0)
                .setLayerCount(1)
            )
            .setImageOffset({0, 0, 0})
            .setImageExtent({bitmap_width, bitmap_height, 1})
    );
    image_layout_stage1(*texture_image, bitmap_format);
    texture_image_view = device.logical->createImageViewUnique (
        ::vk::ImageViewCreateInfo()
        .setImage(*texture_image)
        .setViewType(::vk::ImageViewType::e2D)
        .setFormat(bitmap_format)
        .setSubresourceRange (
            ::vk::ImageSubresourceRange()
            .setAspectMask(::vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
        )
    );
    texture_sampler = device.logical->createSamplerUnique (
        ::vk::SamplerCreateInfo()
        .setMagFilter(::vk::Filter::eLinear)
        .setMinFilter(::vk::Filter::eLinear)
        .setAddressModeU(::vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(::vk::SamplerAddressMode::eRepeat)
        .setAddressModeW(::vk::SamplerAddressMode::eRepeat)
        .setAnisotropyEnable(true)
        .setMaxAnisotropy(16)
        .setBorderColor(::vk::BorderColor::eIntOpaqueBlack)
        .setUnnormalizedCoordinates(false)
        .setCompareEnable(false)
        .setCompareOp(::vk::CompareOp::eAlways)
        .setMipmapMode(::vk::SamplerMipmapMode::eLinear)
        .setMipLodBias(0.0f)
        .setMinLod(0.0f)
        .setMaxLod(0.0f)
    );
}
void si::vk::renderer::reset_framebuffers(std::uint32_t width, std::uint32_t height) {
    framebuffers.clear();
    framebuffers.reserve(swapchain_image_views.size());
    for (::vk::UniqueImageView& view_ptr : swapchain_image_views) {
        framebuffers.push_back (
            device.logical->createFramebufferUnique(::vk::FramebufferCreateInfo {
                {},
                *render_pass,
                1, &*view_ptr,
                width, height,
                1
            })
        );
    }
}
void si::vk::renderer::reset_command_buffers(std::uint32_t width, std::uint32_t height) {
    command_buffers = device.logical->allocateCommandBuffersUnique({
        *graphics_command_pool,
        ::vk::CommandBufferLevel::ePrimary,
        static_cast<std::uint32_t>(framebuffers.size())
    });
    for (std::size_t i = 0; i < command_buffers.size(); i++) {
        const ::vk::Viewport viewport (0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
        const ::vk::Rect2D scissor ({0, 0}, {width, height});

        ::vk::CommandBuffer& cmd = *command_buffers[i];
        cmd.begin(::vk::CommandBufferBeginInfo {});
        ::vk::ClearValue clear_color {::vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}}};
        cmd.beginRenderPass (
            {
                *render_pass,
                *framebuffers[i],
                scissor,
                1,
                &clear_color
            },
            ::vk::SubpassContents::eInline
        );
        cmd.bindPipeline(::vk::PipelineBindPoint::eGraphics, *pipeline);
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
        .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), swapchain_extent.width / static_cast<float>(swapchain_extent.height), 0.1f, 10.0f)
    };
    ubo.proj[1][1] *= -1.0f; // glm's y clip-space axis is opposite to vulkans
    void* data = device.logical->mapMemory(*uniform_buffer_memories[ix], 0, sizeof(ubo));
    std::copy(&ubo, &ubo + 1, reinterpret_cast<uniform_buffer_object*>(data));
    device.logical->unmapMemory(*uniform_buffer_memories[ix]);
}
void si::vk::renderer::copy(::vk::Buffer src, ::vk::Buffer dst, ::vk::BufferCopy what) {
    run_oneshot(
        [&](::vk::CommandBuffer& cmd) {
            cmd.copyBuffer(src, dst, std::array{what});
        }
    );
}
void si::vk::renderer::copy(::vk::Buffer src, ::vk::Image dst, ::vk::BufferImageCopy what) {
    run_oneshot(
        [&](::vk::CommandBuffer& cmd) {
            cmd.copyBufferToImage(src, dst, ::vk::ImageLayout::eTransferDstOptimal, std::array{what});
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
    reset_texture_image("wintex.png");
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
