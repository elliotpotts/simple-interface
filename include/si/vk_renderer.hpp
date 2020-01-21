#ifndef SI_VK_RENDERER_HPP_INCLUDED
#define SI_VK_RENDERER_HPP_INCLUDED

#include <vulkan/vulkan.hpp>
#include <si/wl/display.hpp>
#include <si/wl/surface.hpp>
#include <vector>
#include <array>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>

namespace si {
    namespace vk {
        struct gfx_device;
        struct renderer;
        struct root {
            ::vk::UniqueInstance instance;
            VkDebugReportCallbackEXT debug_reporter;
            std::vector<gfx_device> gfxs;
            root();
            std::unique_ptr<renderer> make_renderer(::wl::display&, ::wl::surface&, std::uint32_t width, std::uint32_t height);
        };
        struct gfx_device {
            ::vk::PhysicalDevice physical;
            ::vk::Queue graphics_q;
            std::uint32_t graphics_q_family_ix;
            ::vk::Queue present_q;
            std::uint32_t present_q_family_ix;
            ::vk::UniqueDevice logical;

            gfx_device(::vk::PhysicalDevice physical, ::vk::Queue graphics_q, std::uint32_t graphics_q_ix, ::vk::Queue present_q, std::uint32_t present_q_ix, ::vk::UniqueDevice logical);
            std::unique_ptr<renderer> make_renderer(::vk::UniqueSurfaceKHR, std::uint32_t width, std::uint32_t height);
            std::uint32_t find_memory_type_index(::vk::MemoryRequirements reqs, ::vk::MemoryPropertyFlags flags);
            std::tuple<::vk::UniqueBuffer, ::vk::UniqueDeviceMemory> make_buffer(::vk::DeviceSize buffer_size, ::vk::BufferUsageFlags buffer_usage, ::vk::SharingMode sharing_mode, ::vk::MemoryPropertyFlags memory_flags);
        };
        struct vertex {
            glm::vec2 pos;
            glm::vec3 color;
            glm::vec2 uv;
            static ::vk::VertexInputBindingDescription binding_description;
            static std::array<::vk::VertexInputAttributeDescription, 3> input_descriptions;
        };
        struct uniform_buffer_object {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };
        struct renderer {
            gfx_device& device;
            ::vk::UniqueSemaphore swapchain_image_available;
            ::vk::UniqueSemaphore render_finished;
            ::vk::UniqueFence in_flight;
            ::vk::UniqueSurfaceKHR surface;
            //TODO: Get formats from surface
            //::vk::SurfaceFormatKHR surface_format;
            //::vk::PresentModeKHR surface_present_mode;

            //TODO: Move pipeline into gfx_device - maybe?
            std::array<::vk::DescriptorSetLayoutBinding, 2> descriptor_set_layout_bindings = std::array {
                // ubo binding
                ::vk::DescriptorSetLayoutBinding()
                .setBinding(0)
                .setDescriptorType(::vk::DescriptorType::eUniformBuffer)
                .setDescriptorCount(1)
                .setStageFlags(::vk::ShaderStageFlagBits::eVertex)
                .setPImmutableSamplers(nullptr),
                // sampler binding
                ::vk::DescriptorSetLayoutBinding()
                .setBinding(1)
                .setDescriptorType(::vk::DescriptorType::eCombinedImageSampler)
                .setDescriptorCount(1)
                .setStageFlags(::vk::ShaderStageFlagBits::eFragment)
                .setPImmutableSamplers(nullptr)
            };
            ::vk::UniqueDescriptorSetLayout descriptor_set_layout;
            ::vk::UniquePipelineLayout pipeline_layout;
            ::vk::UniqueRenderPass render_pass;
            ::vk::UniquePipeline pipeline;
            ::vk::UniqueCommandPool graphics_command_pool;
            ::vk::UniqueBuffer vertex_buffer;
            ::vk::UniqueDeviceMemory vertex_buffer_memory;
            ::vk::UniqueBuffer index_buffer;
            ::vk::UniqueDeviceMemory index_buffer_memory;
            std::vector<::vk::UniqueBuffer> uniform_buffers;
            std::vector<::vk::UniqueDeviceMemory> uniform_buffer_memories;
            ::vk::UniqueDescriptorPool descriptor_pool;
            std::vector<::vk::DescriptorSet> descriptor_sets; // not unique because they'll be destroyed along with the above pool.
            ::vk::UniqueImage texture_image;
            ::vk::UniqueDeviceMemory texture_image_memory;
            ::vk::UniqueImageView texture_image_view;
            ::vk::UniqueSampler texture_sampler;
            ::vk::Extent2D swapchain_extent;
            ::vk::UniqueSwapchainKHR swapchain;
            std::vector<::vk::Image> swapchain_images;
            std::vector<::vk::UniqueImageView> swapchain_image_views;
            std::vector<::vk::UniqueFramebuffer> framebuffers;
            std::vector<::vk::UniqueCommandBuffer> command_buffers;
            const std::vector<vertex> vertices = {
                {{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                {{ 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{ 1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-1.0f,  1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
            };
            const std::vector<uint16_t> indices = {
                0, 1, 2, 2, 3, 0
            };

            void reset_descriptor_set_layout();
            void reset_pipeline();
            void reset_swapchain(std::uint32_t width, std::uint32_t height);
            void reset_swapchain_images();
            void reset_framebuffers(std::uint32_t width, std::uint32_t height);
            void reset_vertex_buffer();
            void reset_index_buffer();
            void reset_uniform_buffers();
            void reset_descriptor_pool();
            void reset_descriptor_sets();
            void reset_texture_image(std::string filepath);
            void reset_command_buffers(std::uint32_t width, std::uint32_t height);

            void update_uniform_buffers(unsigned ix);

            template<typename F>
            void run_oneshot(F&& f) {
                std::vector<::vk::UniqueCommandBuffer> cmds = device.logical->allocateCommandBuffersUnique({
                    *graphics_command_pool,
                    ::vk::CommandBufferLevel::ePrimary,
                    1
                });
                ::vk::CommandBuffer& cmd = *cmds.front();
                cmd.begin(::vk::CommandBufferBeginInfo {});
                f(cmd);
                cmd.end();
                device.graphics_q.submit (
                    ::vk::SubmitInfo {
                        0, nullptr,
                        nullptr,
                        1, &cmd,
                        0, nullptr
                   },
                    nullptr
                );
                device.graphics_q.waitIdle();
            }
            void copy(::vk::Buffer src, ::vk::Buffer dst, ::vk::BufferCopy what);
            void copy(::vk::Buffer src, ::vk::Image dst, ::vk::BufferImageCopy what);
            // These are poorly named and thought out [temporary] helper functions
            void image_layout_stage0(::vk::Image& img, ::vk::Format format);
            void image_layout_stage1(::vk::Image& img, ::vk::Format format);

            template<typename It>
            std::tuple<::vk::UniqueBuffer, ::vk::UniqueDeviceMemory, std::size_t> stage(const It begin, const It end) {
                using T = typename std::iterator_traits<It>::value_type;
                std::size_t size = sizeof(T) * std::distance(begin, end);
                auto [buffer, buffer_memory] = device.make_buffer (
                    size,
                    ::vk::BufferUsageFlagBits::eTransferSrc,
                    ::vk::SharingMode::eExclusive,
                    ::vk::MemoryPropertyFlagBits::eHostVisible | ::vk::MemoryPropertyFlagBits::eHostCoherent
                );
                void* data = device.logical->mapMemory(*buffer_memory, 0, size);
                std::copy(begin, end, reinterpret_cast<T*>(data));
                device.logical->unmapMemory(*buffer_memory);
                return std::make_tuple(std::move(buffer), std::move(buffer_memory), size);
            }

            renderer(gfx_device& device, ::vk::UniqueSurfaceKHR surface, std::uint32_t width, std::uint32_t height);
            ~renderer();
            void draw();
            void resize(std::uint32_t width, std::uint32_t height);
        };
    }
}

#endif
