#include <fmt/format.h>
#include <algorithm>
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
using namespace std::literals::string_literals;
#include <si/util.hpp>

#include <si/wl/display.hpp>
#include <si/wl/registry.hpp>
#include <si/wl/compositor.hpp>
#include <si/wl/shell.hpp>
#include <si/wl/shm.hpp>
#include <si/wl/seat.hpp>

#include <si/egl.hpp>
#include <si/wl/egl_window.hpp>
#include <si/shm_buffer.hpp>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <si/vk_renderer.hpp>

enum draw_method {
    draw_opengl,
    draw_gles,
    draw_vulkan,
    draw_shm
};
inline constexpr int win_width = 480;
inline constexpr int win_height = 360;
auto start_time = std::chrono::system_clock::now();

std::vector<VkLayerProperties> vk_available_layers() {
    std::uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> available(count);
    vkEnumerateInstanceLayerProperties(&count, available.data());
    return available;
}

std::vector<VkExtensionProperties> vk_available_extensions() {
    uint32_t count;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data());
    return available;
}

std::vector<VkPhysicalDevice> vk_physical_devices(VkInstance vk) {
    std::uint32_t count;
    if (VkResult result = vkEnumeratePhysicalDevices(vk, &count, nullptr); result != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("Couldn't enumerate physical devices: {}", result));
    } else if (count == 0) {
        throw std::runtime_error("No vulkan capable devices present.\n");
    } else {
        fmt::print("Found {} physical device(s)\n", count);
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(vk, &count, devices.data());
    return devices;
}

std::vector<VkExtensionProperties> vk_phy_available_extensions(VkPhysicalDevice device) {
    std::uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
    return extensions;
}

std::vector<VkQueueFamilyProperties> vk_phy_queue_families(VkPhysicalDevice device) {
    unsigned count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
    return families;
}

auto vk_get_phy_surface_capabilities(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
    
    std::uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, formats.data());

    std::uint32_t modes_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modes_count, nullptr);
    std::vector<VkPresentModeKHR> modes(modes_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modes_count, modes.data());

    return std::make_tuple(capabilities, formats, modes);
}

std::tuple<VkDevice, VkQueue, VkSwapchainKHR> vulkan_make_device_for_surface(VkInstance vk, VkSurfaceKHR surface) {    
    const VkPhysicalDevice phy = vk_physical_devices(vk)[0];

    fmt::print("Available device extensions:\n");
    for (auto ext : vk_phy_available_extensions(phy)) {
        fmt::print(" * {}\n", ext.extensionName);
    }
   
    // We know a priori that the first queue is fine for graphics & presentation
    // must call this to apease spec
    auto queues_avail = vk_phy_queue_families(phy);
    for (std::size_t i = 0; i < queues_avail.size(); i++) {
        VkBool32 supported;
        vkGetPhysicalDeviceSurfaceSupportKHR(phy, i, surface, &supported);
    }
    const float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_crinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = 0,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(phy, &features);

    std::vector<const char*> extensions = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo device_crinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_crinfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &features
    };
    
    VkDevice device;
    if (vkCreateDevice(phy, &device_crinfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    
    VkQueue queue;
    vkGetDeviceQueue(device, 0, 0, &queue);

    // Check swapchain caps
    auto [sc_caps, sc_formats, sc_modes] = vk_get_phy_surface_capabilities(phy, surface);
    fmt::print("Available formats:\n");
    for (auto format : sc_formats) {
        fmt::print(" * {}/{}\n", format.format, format.colorSpace);
    }
    fmt::print("Available modes:\n");
    for (auto mode : sc_modes) {
        fmt::print(" * {}\n", mode);
    }
    // We know a priori which format & mode we want
    VkSwapchainCreateInfoKHR sc_crinfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surface,
        .minImageCount = sc_caps.minImageCount + 1,
        .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {win_width, win_height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = sc_caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };
    VkSwapchainKHR swapchain;
    if (VkResult r = vkCreateSwapchainKHR(device, &sc_crinfo, nullptr, &swapchain); r != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("Couldn't create swapchain: {}", r));
    }

    return {device, queue, swapchain};
}

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
    fmt::print("{}\n", pMessage);
    return VK_FALSE;
}

VkInstance init_vulkan() {
    fmt::print("Available layers:\n");
    for (auto& layer : vk_available_layers()) {
        fmt::print(" * {}\n", layer.layerName);
    }
    fmt::print("Available extensions:\n");
    for (const auto& extension : vk_available_extensions()) {
        fmt::print(" * {}\n", extension.extensionName);
    }
    
    VkApplicationInfo appinfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "si test",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_0
    };
    std::vector<const char*> layers = { "VK_LAYER_LUNARG_standard_validation" };
    std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_wayland_surface", "VK_EXT_debug_utils", "VK_EXT_debug_report" };
    VkInstanceCreateInfo crinfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appinfo,
        .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };
    VkInstance vk;
    if (VkResult result = vkCreateInstance(&crinfo, nullptr, &vk); result != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("Couldn't create vulkan instance: {}", result));
    } else {
        fmt::print("Created vulkan instance\n");
    }

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
        fmt::print("Created debug reporter\n");
    }
    return vk;
}

VkSurfaceKHR vk_create_surface(VkInstance vk, wl::display& dpy, wl::surface& sfc) {
    VkWaylandSurfaceCreateInfoKHR vk_wl_crinfo = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .display = static_cast<wl_display*>(dpy),
        .surface = static_cast<wl_surface*>(sfc)
    };
    VkSurfaceKHR vk_surface;
    if (VkResult r = vkCreateWaylandSurfaceKHR(vk, &vk_wl_crinfo, nullptr, &vk_surface); r != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("Could not create surface: {}", r));
    }
    return vk_surface;
}

std::vector<VkImage> vk_sc_images(VkDevice device, VkSwapchainKHR sc) {
    std::uint32_t count;
    vkGetSwapchainImagesKHR(device, sc, &count, nullptr);
    std::vector<VkImage> images;
    vkGetSwapchainImagesKHR(device, sc, &count, images.data());
    return images;
}

std::vector<VkImageView> vk_make_image_views(VkDevice device, std::vector<VkImage>& images) {
    std::vector<VkImageView> views;
    for (auto img : images) {
        VkImageViewCreateInfo crinfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = img,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        views.emplace_back();
        if (VkResult r = vkCreateImageView(device, &crinfo, nullptr, &views.back()); r != VK_SUCCESS) {
            throw std::runtime_error(fmt::format("Couldn't create image view: {}", r));
        }
    }
    return views;
}

VkShaderModule vk_make_module(VkDevice device, std::string fname) {
    auto code = si::file_contents(fname);
    if (code.size() % 4 != 0) {
        throw std::runtime_error("Code size must be multiple of 4.");
    }
    VkShaderModuleCreateInfo crinfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<std::uint32_t*>(code.data())
    };
    VkShaderModule module;
    if (VkResult r = vkCreateShaderModule(device, &crinfo, nullptr, &module); r != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("Couldn't create shader module: {}", r));
    } else {
        return module;
    }
}

VkPipeline vk_make_pipeline(VkDevice device) {
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    
    for (auto [stage_name, stage] : std::vector {std::make_pair("vert", VK_SHADER_STAGE_VERTEX_BIT),
                                                 std::make_pair("frag", VK_SHADER_STAGE_FRAGMENT_BIT)}) {
        stages.push_back ({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = stage,
            .module = vk_make_module(device, fmt::format("build/{}.spv", stage_name)),
            .pName = "main"
        });
    }
    VkPipelineVertexInputStateCreateInfo input_crinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };
    VkPipelineInputAssemblyStateCreateInfo assemble_crinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(win_width),
        .height = static_cast<float>(win_height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {win_width, win_height}
    };
    VkPipelineViewportStateCreateInfo viewport_crinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    VkPipelineRasterizationStateCreateInfo rasteriser_crinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f
    };
    VkPipelineMultisampleStateCreateInfo multisampling_crinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    VkPipelineColorBlendAttachmentState blend_attachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
    };
    VkPipelineLayoutCreateInfo layout_crinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };
    VkPipelineLayout layout;
    if (VkResult r = vkCreatePipelineLayout(device, &layout_crinfo, nullptr, &layout); r != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("Couldn't create pipeline layout: {}", r));
    }
    
    VkAttachmentDescription colour_attachment = {
        .flags = 0,
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    VkAttachmentReference colour_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    VkSubpassDescription subpass = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colour_attachment_ref,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };
    VkRenderPassCreateInfo pass_crinfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colour_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };
    VkRenderPass pass;
    if (VkResult r = vkCreateRenderPass(device, &pass_crinfo, nullptr, &pass); r != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("Couldn't create render pass: {}", r));
    }

    VkGraphicsPipelineCreateInfo pipeline_crinfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<std::uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &input_crinfo,
        .pInputAssemblyState = &assemble_crinfo,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_crinfo,
        .pRasterizationState = &rasteriser_crinfo,
        .pMultisampleState = &multisampling_crinfo,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blending,
        .pDynamicState = nullptr,
        .layout = layout,
        .renderPass = pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };
    VkPipeline pipeline;
    if (VkResult r = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_crinfo, nullptr, &pipeline); r != VK_SUCCESS) {
        throw std::runtime_error(fmt::format("Couldn't create pipeline: {}", r));
    }
    return pipeline;
}

std::tuple<VkInstance, VkDevice, VkQueue, VkSwapchainKHR, VkPipeline> make_vulkan(wl::display& dpy, wl::surface& sfc) {
    VkInstance vk = init_vulkan();
    VkSurfaceKHR surface = vk_create_surface(vk, dpy, sfc);
    auto [device, queue, swapchain] = vulkan_make_device_for_surface(vk, surface);
    std::vector<VkImage> images = vk_sc_images(device, swapchain);
    std::vector<VkImageView> views = vk_make_image_views(device, images);
    auto pipeline = vk_make_pipeline(device);
    return std::make_tuple(vk, device, queue, swapchain, pipeline);
}

int main() {
    wl::display my_display;
    wl::registry my_registry = my_display.make_registry();
    //my_display.dispatch(); // TODO: Figure out when I need to dispatch/roundtrip
    my_display.roundtrip();

    auto& my_compositor = my_registry.get<wl::compositor>();
    auto& my_seat = my_registry.get<si::wl::seat>();
    auto my_ptr = my_seat.pointer();
    auto my_keyboard = my_seat.keyboard();
    auto& my_shell = my_registry.get<wl::shell>();

    wl::surface my_surface = my_compositor.make_surface();   
    wl::shell_surface my_shell_surface = my_shell.make_surface(my_surface);
    my_shell_surface.set_toplevel();

    draw_method draw_with = draw_vulkan;
    if (draw_with == draw_vulkan) {
        si::vk::root vk;
        auto r = vk.make_renderer(my_display, my_surface);
        //si::vk_renderer vk(my_display, my_surface);
    } else if (draw_with == draw_opengl) {
        EGLDisplay egl_display = my_display.egl();
        auto [egl_context, egl_config] = init_egl(egl_display);
        describe_egl_config(egl_display, egl_config);
        wl::egl_window egl_window(egl_display, egl_config, egl_context, my_surface, win_width, win_height);

        /* Paint on the freshly created rendering context */
        if (eglMakeCurrent(egl_display, egl_window.surface(), egl_window.surface(), egl_context)) {
            fmt::print("egl made current.\n");
        } else {
            egl_throw();
        }

        for (unsigned i = 0; i < 1; i++) {
            glClearColor(1.0, 1.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
            glFlush();          
            if (eglSwapBuffers(egl_display, egl_window.surface())) {
                fmt::print("egl buffer swapped\n");
            } else {
                egl_throw();
            }
        }
    } else if (draw_with == draw_shm) {
        auto& my_shm = my_registry.get<wl::shm>();
        shm_buffer my_buffer {my_shm, win_width, win_height};
        my_surface.attach(my_buffer.buffer, 0, 0);

        std::fill (static_cast<std::uint32_t*>(my_buffer.pixels.get_address()),
                   static_cast<std::uint32_t*>(my_buffer.pixels.get_address()) + win_width * win_height,
                   (255 << 24) | (255 << 16) | (255 << 8) | (0 << 0)
        );

        my_surface.commit();
    } else {
        fmt::print("unknown draw method\n");
    }
    while (my_display.dispatch() != -1) {
    }
    fmt::print("finished\n");
    return 0;
}
