#ifndef SI_VK_RENDERER_HPP_INCLUDED
#define SI_VK_RENDERER_HPP_INCLUDED

#include <vulkan/vulkan.hpp>
#include <si/wl/display.hpp>
#include <si/wl/surface.hpp>
#include <vector>
#include <memory>
#include <cstdint>

namespace si {
    namespace vk {
        struct gfx_device;
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

            ::vk::UniqueSwapchainKHR swapchain;

            std::vector<::vk::Image> images;
            std::vector<::vk::UniqueImageView> image_views;

            std::vector<::vk::UniqueFramebuffer> framebuffers;

            std::vector<::vk::UniqueCommandBuffer> command_buffers;

            void create_pipeline();
            void create_swapchain(std::uint32_t width, std::uint32_t height);
            void create_images();
            void create_framebuffers(std::uint32_t width, std::uint32_t height);
            void create_command_buffers(std::uint32_t width, std::uint32_t height);

            renderer(gfx_device& device, ::vk::UniqueSurfaceKHR surface, std::uint32_t width, std::uint32_t height);
            ~renderer();
            void draw();
            void resize(std::uint32_t width, std::uint32_t height);
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
        };
        struct root {
            ::vk::UniqueInstance instance;
            VkDebugReportCallbackEXT debug_reporter;
            std::vector<gfx_device> gfxs;
            root();
            std::unique_ptr<renderer> make_renderer(::wl::display&, ::wl::surface&, std::uint32_t width, std::uint32_t height);
        };
    }
}

#endif
