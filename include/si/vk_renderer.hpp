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
            std::tuple<::vk::UniqueBuffer, ::vk::UniqueDeviceMemory> make_buffer(::vk::DeviceSize buffer_size, ::vk::BufferUsageFlags buffer_usage, ::vk::SharingMode sharing_mode, ::vk::MemoryPropertyFlags memory_flags);
        };
        struct vertex {
            glm::vec2 pos;
            glm::vec3 color;
            static ::vk::VertexInputBindingDescription binding_description;
            static std::array<::vk::VertexInputAttributeDescription, 2> input_descriptions;
        };
        struct renderer {
            gfx_device& device;
            ::vk::UniqueSemaphore image_available;
            ::vk::UniqueSemaphore render_finished;
            ::vk::UniqueFence in_flight;
            ::vk::UniqueSurfaceKHR surface;
            //TODO: Get formats from surface
            //::vk::SurfaceFormatKHR surface_format;
            //::vk::PresentModeKHR surface_present_mode;

            //TODO: Move pipeline into gfx_device - maybe?
            ::vk::UniquePipelineLayout pipeline_layout;
            ::vk::UniqueRenderPass render_pass;
            ::vk::UniquePipeline pipeline;
            ::vk::UniqueCommandPool graphics_command_pool;
            ::vk::UniqueBuffer vertex_buffer;
            ::vk::UniqueDeviceMemory vertex_buffer_memory;
            ::vk::UniqueBuffer index_buffer;
            ::vk::UniqueDeviceMemory index_buffer_memory;
            ::vk::UniqueSwapchainKHR swapchain;
            std::vector<::vk::Image> images;
            std::vector<::vk::UniqueImageView> image_views;
            std::vector<::vk::UniqueFramebuffer> framebuffers;
            std::vector<::vk::UniqueCommandBuffer> command_buffers;
            const std::vector<vertex> vertices = {
                {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
            };
            const std::vector<uint16_t> indices = {
                0, 1, 2                
            };

            void create_pipeline();
            void create_swapchain(std::uint32_t width, std::uint32_t height);
            void create_images();
            void create_framebuffers(std::uint32_t width, std::uint32_t height);
            void create_vertex_buffer();
            void create_index_buffer();
            void create_command_buffers(std::uint32_t width, std::uint32_t height);

            void buffer_copy(::vk::Buffer src, ::vk::Buffer dst, ::vk::BufferCopy what);

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
