#ifndef SI_VK_RENDERER_HPP_INCLUDED
#define SI_VK_RENDERER_HPP_INCLUDED

#include <vulkan/vulkan.hpp>
#include <si/wl/display.hpp>
#include <si/wl/surface.hpp>
#include <vector>

namespace si {
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
    };
}

#endif
