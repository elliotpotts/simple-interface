#ifndef SI_VK_RENDERER_HPP_INCLUDED
#define SI_VK_RENDERER_HPP_INCLUDED

#include <vulkan/vulkan.hpp>
#include <si/wl/display.hpp>
#include <si/wl/surface.hpp>
#include <vector>
#include <memory>

namespace si {
    namespace vk {
        struct gfx_device;
        struct renderer {
            gfx_device& device;
            ::vk::UniqueSurfaceKHR surface;
            ::vk::UniqueSemaphore image_available;
            ::vk::UniqueSemaphore render_finished;
            ::vk::UniqueFence in_flight;
            ::vk::UniqueSwapchainKHR swapchain;
            std::vector<::vk::Image> images;
            std::vector<::vk::UniqueImageView> image_views;
            std::vector<::vk::UniqueFramebuffer> framebuffers;
            std::vector<::vk::UniqueCommandBuffer> command_buffers;
            
            renderer(gfx_device& device, ::vk::UniqueSurfaceKHR surface);
            ~renderer();
            void draw();
        };
        struct gfx_device {
            ::vk::PhysicalDevice physical;
            ::vk::Queue graphics_q;
            std::uint32_t graphics_q_family_ix;
            ::vk::Queue present_q;
            std::uint32_t present_q_family_ix;
            ::vk::UniqueDevice logical;
            
            ::vk::UniquePipelineLayout pipeline_layout;
            ::vk::UniqueRenderPass render_pass;
            ::vk::UniquePipeline pipeline;

            ::vk::UniqueCommandPool graphics_command_pool;

            gfx_device(::vk::PhysicalDevice physical, ::vk::Queue graphics_q, std::uint32_t, ::vk::Queue present_q, std::uint32_t, ::vk::UniqueDevice logical);
            std::unique_ptr<renderer> make_renderer(::vk::UniqueSurfaceKHR);
        };
        struct root {
            ::vk::UniqueInstance instance;
            VkDebugReportCallbackEXT debug_reporter;
            std::vector<gfx_device> gfxs;
            root();
            std::unique_ptr<renderer> make_renderer(::wl::display&, ::wl::surface&);
        };
    }
    /*
    struct vk_iface {
        vk::PhysicalDevice phy;
        vk::Queue graphics_q;
        vk::Queue present_q;
        vk::UniqueDevice device;
        vk::UniqueSwapchainKHR swapchain;
    };

    struct vk_pipeline {
        vk::UniquePipelineLayout layout;
        vk::UniqueRenderPass render_pass;
        vk::UniquePipeline pipeline;
    };

    struct vk_renderer {
        vk::UniqueInstance instance;
        VkDebugReportCallbackEXT debug_callback;
        vk::UniqueSurfaceKHR surface;
        vk_iface iface;
        std::vector<vk::UniqueImageView> image_views;
        vk_pipeline pipeline;
        vk_renderer(::wl::display&, ::wl::surface&);
        };*/
}

#endif
