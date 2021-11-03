#ifdef __MINGW64__
    #define VK_USE_PLATFORM_WIN32_KHR

    #define COMPILER_NAME "MINGW"
#else
    #define COMPILER_NAME ""
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef __MINGW64__
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
#endif

#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX/UINT64_MAX
#include <algorithm> // Necessary for std::min/std::max
#include <fstream>

#ifdef NDEBUG
    const bool enableValidationLayers = false;

    const bool showDebugInfo = false;
#else
    const bool enableValidationLayers = true;

    const bool showDebugInfo = true;

    #include <iostream>

    using namespace std;
#endif

#include <figure2d_type_object.h>
#include <memory>

VkResult createDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
	        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (!func) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void destroyDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator = nullptr)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
	        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func) {
        func(instance, debugMessenger, pAllocator);
    }
}

class FigureDrawApplication
{
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    bool framebufferResized;

    uint32_t imageIndex;

    VertexArrays _vertexArrays;
    typedef std::vector<VkBuffer> VertexBuffers;
    VertexBuffers vertexBuffers;
    std::vector<VkDeviceMemory> vertexBufferMemoryBlocks;
    VkDeviceSize offset {0};
    std::vector<uint32_t> vertexArraySizes;

    IndexArrays _indexArrays;
    typedef std::vector<VkBuffer> IndexBuffers;
    IndexBuffers indexBuffers;
    std::vector<VkDeviceMemory> indexBufferMemoryBlocks;
    std::vector<uint32_t> indexArraySizes;

    VkClearValue clearColor {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
	    return graphicsFamily.has_value() &&
		   presentFamily.has_value();
	}
    };

    struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
    };

    GLFWwindow* window;

    const char* TITLE = "Vulkan Triangle Demo";

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    int width = 0;
    int height = 0;

    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    void initVulkan() {
        createInstance();
	setupDebugMessenger();
	createSurface();

	pickPhysicalDevice();
	createLogicalDevice();

	createSwapChain();
	createImageViews();

	createRenderPass();
	createGraphicsPipeline();

	createFramebuffers();

	createCommandPool();

	createVertexBuffers(_vertexArrays);
	createIndexBuffers(_indexArrays);

	createCommandBuffers();

	createSyncObjects();
    }

    bool checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
	    bool layerFound = false;

	    for (const auto& layerProperties : availableLayers) {
	        if (strcmp(layerName, layerProperties.layerName) == 0) {
	            layerFound = true;
		    break;
		}
	    }

	    if (!layerFound)
	        return false;
	}

	return true;
    }

    std::vector<const char*> getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

	if (strcmp(COMPILER_NAME, "MINGW") == 0)
	    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional
    }

    void setupDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(createInfo);

        if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	    throw std::runtime_error("Failed to set up debug messenger.");
    }

    void createInstance() {
        VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = TITLE;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (showDebugInfo) {
	    uint32_t availableExtensionCount = 0;
	    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);

	    std::cout << availableExtensionCount << " extensions supported" << std::endl;

            std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);

            vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount,
			                           availableExtensions.data());

	    std::cout << "Available extensions:" << std::endl;
            for (const auto& availableExtension : availableExtensions) {
	        std::cout << '\t' << availableExtension.extensionName << std::endl;
	    }
	}

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;

	if (enableValidationLayers) {
	    if (!checkValidationLayerSupport())
                throw std::runtime_error("Validation layers requested, but not available.");

	    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	    createInfo.ppEnabledLayerNames = validationLayers.data();

	    populateDebugMessengerCreateInfo(debugCreateInfo);
	    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        }

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	    throw std::runtime_error("Failed to create Vulkan instance.");
    }

    void createSurface() {
	if (strcmp(COMPILER_NAME, "MINGW") == 0) {
	    VkWin32SurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hwnd = glfwGetWin32Window(window);
            createInfo.hinstance = GetModuleHandle(nullptr);

	    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
                throw std::runtime_error("Failed to create Win32 window surface.");

	    return;
	}

        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	    throw std::runtime_error("Failed to create window surface.");
    }

    void pickPhysicalDevice(const bool& findBestCandidate = false) {
        uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	    throw std::runtime_error("Failed to find GPUs with Vulkan support.");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	if (findBestCandidate) {
	    // Use an ordered map to automatically sort candidates by increasing score
            std::multimap<int, VkPhysicalDevice> candidates;

            for (const auto& device : devices) {
	        int score = rateDeviceSuitability(device);
                candidates.insert(std::make_pair(score, device));
            }

            // Check if the best candidate is suitable at all
            if (candidates.rbegin()->first == 0)
                throw std::runtime_error("Failed to find a suitable GPU.");

	    physicalDevice = candidates.rbegin()->second;
	} else {
            for (const auto& device : devices) {
                if (isDeviceSuitable(device)) {
                    physicalDevice = device;
                    break;
                }
            }

            if (physicalDevice == VK_NULL_HANDLE)
                throw std::runtime_error("Failed to find a suitable GPU.");
	}
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviveExtensionSupport(device);

        bool swapChainAdequate = false;

	if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviveExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
	    details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
	    details.presentModes.resize(presentModeCount);
	    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

	return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
	    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
	        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
	        return availableFormat;
	    }
	}

	return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
	        return availablePresentMode;
	    }
	}

	return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width,
			                capabilities.minImageExtent.width,
					capabilities.maxImageExtent.width);

        actualExtent.height = std::clamp(actualExtent.height,
			                 capabilities.minImageExtent.height,
					 capabilities.maxImageExtent.height);

        return actualExtent;
    }

    void createSwapChain() {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 &&
	    imageCount > swapChainSupport.capabilities.maxImageCount) { 
	    imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
	    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
	    throw std::runtime_error("Failed to create swap chain.");

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
    }

    void cleanupSwapChain() { 
        for (const auto& framebuffer : swapChainFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);

	vkFreeCommandBuffers(device, commandPool,
			     static_cast<uint32_t>(commandBuffers.size()),
			     commandBuffers.data());

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);

	for (const auto& imageView : swapChainImageViews)
	    vkDestroyImageView(device, imageView, nullptr);

	vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void recreateSwapChain() {
	glfwGetFramebufferSize(window, &width, &height);

	while (width == 0 || height == 0) {
	    glfwGetFramebufferSize(window, &width, &height);
	    glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandBuffers();
    }

    void createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = swapChainImageFormat;

	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	for (size_t i = 0; i < swapChainImages.size(); ++i) {    
	    createInfo.image = swapChainImages[i];

	    if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
	        throw std::runtime_error("Failed to create image views.");
	}
    }

    void createRenderPass() {
	VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass.");
    }

    void createGraphicsPipeline() {
	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderCreateInfo{};
	vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderCreateInfo.module = vertShaderModule;
	vertShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderCreateInfo{};
	fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderCreateInfo.module = fragShaderModule;
	fragShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderCreateInfo, fragShaderCreateInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
		                              VK_COLOR_COMPONENT_G_BIT |
					      VK_COLOR_COMPONENT_B_BIT |
					      VK_COLOR_COMPONENT_A_BIT;

        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

	/*
	VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

	VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;
	*/

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	    throw std::runtime_error("Failed to create pipeline layout.");

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStages;

        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizer;
        pipelineCreateInfo.pMultisampleState = &multisampling;
        pipelineCreateInfo.pDepthStencilState = nullptr; // Optional
        pipelineCreateInfo.pColorBlendState = &colorBlending;
        pipelineCreateInfo.pDynamicState = nullptr; // Optional

	pipelineCreateInfo.layout = pipelineLayout;

	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;

        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineCreateInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
				      &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
	    throw std::runtime_error("Failed to create graphics pipeline.");

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
            VkImageView attachments[] = {swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.pAttachments = attachments;
            framebufferCreateInfo.width = swapChainExtent.width;
            framebufferCreateInfo.height = swapChainExtent.height;
            framebufferCreateInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer.");
        }
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
		      VkMemoryPropertyFlags properties,
		      VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	    throw std::runtime_error("Failed to create buffer.");

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memoryRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits,
			                           properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	    throw std::runtime_error("Failed to allocate buffer memory.");

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	    VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0; // Optional
	    copyRegion.dstOffset = 0; // Optional
	    copyRegion.size = size;
	    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void createVertexBuffer(const VertexArray& vertexArray,
		            VkBuffer& vertexBuffer,
		            VkDeviceMemory& vertexBufferMemory)
    {
	if (vertexArray.empty()) return;

	VkDeviceSize bufferSize = sizeof(vertexArray[0]) * vertexArray.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	    memcpy(data, vertexArray.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     vertexBuffer, vertexBufferMemory);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createVertexBuffers(const VertexArrays& vertexArrays) {
	if (vertexArrays.empty()) return;

	size_t vertexArraysSize = vertexArrays.size();
	
	vertexBuffers.resize(vertexArraysSize);
	vertexBufferMemoryBlocks.resize(vertexArraysSize);

	for (size_t i = 0; i < vertexArraysSize; ++i) {
	    createVertexBuffer(vertexArrays[i], vertexBuffers[i], vertexBufferMemoryBlocks[i]);

	    uint32_t vertexArraySize = static_cast<uint32_t>(vertexArrays[i].size());
	    vertexArraySizes.emplace_back(vertexArraySize);
	}
    }

    void destroyVertexBuffers() { 
	if (vertexBuffers.empty()) return;

	for (size_t i = 0; i < vertexBuffers.size(); ++i) {
	    vkDestroyBuffer(device, vertexBuffers[i], nullptr);
	    vkFreeMemory(device, vertexBufferMemoryBlocks[i], nullptr);
	}
    }

    void createIndexBuffer(const IndexArray& indexArray,
		           VkBuffer& indexBuffer,
		           VkDeviceMemory& indexBufferMemory)
    {
	if (indexArray.empty()) return;

	VkDeviceSize bufferSize = sizeof(indexArray[0]) * indexArray.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	    memcpy(data, indexArray.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffers(const IndexArrays& indexArrays) {
	if (indexArrays.empty()) return;

	size_t indexArraysSize = indexArrays.size();
	
	indexBuffers.resize(indexArraysSize);
	indexBufferMemoryBlocks.resize(indexArraysSize);

	for (size_t i = 0; i < indexArraysSize; ++i) {
	    createIndexBuffer(indexArrays[i], indexBuffers[i], indexBufferMemoryBlocks[i]);

	    uint32_t indexArraySize = static_cast<uint32_t>(indexArrays[i].size());
	    indexArraySizes.emplace_back(indexArraySize);
	}
    }

    void destroyIndexBuffers() {
	if (indexBuffers.empty()) return;

	for (size_t i = 0; i < indexBuffers.size(); ++i) {
	    vkDestroyBuffer(device, indexBuffers[i], nullptr);
	    vkFreeMemory(device, indexBufferMemoryBlocks[i], nullptr);
	}
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	    if ((typeFilter & (1 << i)) &&
		(memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;

        throw std::runtime_error("Failed to find suitable memory type.");
    }

    void createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	commandPoolCreateInfo.flags = 0; // Optional

	if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
	    throw std::runtime_error("Failed to create command pool.");
    }
    
    void createCommandBuffers()
    {
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS)
	    throw std::runtime_error("Failed to allocate command buffers.");

	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0; // Optional
	commandBufferBeginInfo.pInheritanceInfo = nullptr; // Optional

	VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;        

        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

	auto commandBufferIt = commandBuffers.cbegin();

	for (const auto& commandBuffer : commandBuffers) {
	    if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
		throw std::runtime_error("Failed to begin recording command buffer.");

            renderPassInfo.framebuffer = swapChainFramebuffers[std::distance(commandBuffers.cbegin(), commandBufferIt)];

	    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		if (!vertexBuffers.empty()) {
		    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		    for (size_t buffer_idx = 0; buffer_idx < vertexBuffers.size(); ++buffer_idx) {
	                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers[buffer_idx], &offset);

		        if (indexArraySizes[buffer_idx] == 0) {
		            vkCmdDraw(commandBuffer, vertexArraySizes[buffer_idx], 1, 0, 0);
			} else {
	                    vkCmdBindIndexBuffer(commandBuffer, indexBuffers[buffer_idx], 0, VK_INDEX_TYPE_UINT16);

                            vkCmdDrawIndexed(commandBuffer, indexArraySizes[buffer_idx], 1, 0, 0, 0);
		        }
		    }
		}

	    vkCmdEndRenderPass(commandBuffer);

	    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	        throw std::runtime_error("Failed to record command buffer.");

	    ++commandBufferIt;
	}
    }

    void createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapChainImageViews.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
	        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
		vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
	        throw std::runtime_error("Failed to create synchronization objects for a frame.");
    }

    void destroySyncObjects() {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
	    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
	    vkDestroyFence(device, inFlightFences[i], nullptr);
	}
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = static_cast<uint32_t>(code.size());
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;

	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	    throw std::runtime_error("Failed to create shader module.");

	return shaderModule;
    }

    static std::vector<char> readFile(const std::string& fileName) {
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	    throw std::runtime_error("Failed to open file.");

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
    }

    bool isDeviceSupportGeometryShader(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
	       deviceFeatures.geometryShader;
    }

    int rateDeviceSuitability(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        int score = 0;

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        // Application can't function without geometry shaders
        if (!deviceFeatures.geometryShader) return 0;

        return score;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	VkBool32 presentSupport = VK_FALSE;

	for (const auto& queueFamily : queueFamilies) {
	    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
	        indices.graphicsFamily = i;
	    }

	    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

	    if (presentSupport) {
	        indices.presentFamily = i;
	    }

	    if (indices.isComplete())
		break;

	    ++i;
	}

        return indices;
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
	    indices.graphicsFamily.value(),
	    indices.presentFamily.value()
	};

        float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

	for (uint32_t queueFamily : uniqueQueueFamilies) {
            queueCreateInfo.queueFamilyIndex = queueFamily;

	    queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	createInfo.enabledLayerCount = 0;
	if (enableValidationLayers) {
	    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	    createInfo.ppEnabledLayerNames = validationLayers.data();
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
	    throw std::runtime_error("Failed to create logical device.");

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void initWindow() {
        glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<FigureDrawApplication*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
	    glfwPollEvents();
	    drawFrame();
	}

	vkDeviceWaitIdle(device);
    }

    void drawFrame() {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
			                        imageAvailableSemaphores[currentFrame],
						VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
	    recreateSwapChain();
	    return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	   throw std::runtime_error("Failed to acquire swap chain image.");

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	    vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

	// Mark the image as now being in use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer.");

	VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR ||
	    result == VK_SUBOPTIMAL_KHR ||
	    framebufferResized)
	{
	    framebufferResized = false;
	    recreateSwapChain();
	} else if (result != VK_SUCCESS)
	    throw std::runtime_error("Failed to present swap chain image.");

	vkQueueWaitIdle(presentQueue);
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanup() {
	cleanupSwapChain();

	destroyIndexBuffers();
	destroyVertexBuffers();

	destroySyncObjects();

	vkDestroyCommandPool(device, commandPool, nullptr);

	vkDestroyDevice(device, nullptr);

	if (enableValidationLayers) {
            destroyDebugUtilsMessengerEXT(instance, debugMessenger);
        }

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

	glfwTerminate();
    }

public:
    void draw(Figure2D& figure, const IndexArray& indices = {})
    {
	VertexArray figureVertexArray = figure.draw();
	uint32_t vertexQuantity = static_cast<uint32_t>(figureVertexArray.size());
	uint32_t indexQuantity = static_cast<uint32_t>(indices.size());

	_vertexArrays.emplace_back(figureVertexArray);
	vertexArraySizes.emplace_back(vertexQuantity);
	_indexArrays.emplace_back(indices);
	indexArraySizes.emplace_back(indexQuantity);
    }

    void draw(std::unique_ptr<Figure2D>&& figure,
	      const IndexArray& indices = {})
    {
	VertexArray figureVertexArray = figure->draw();
	uint32_t vertexQuantity = static_cast<uint32_t>(figureVertexArray.size());
	uint32_t indexQuantity = static_cast<uint32_t>(indices.size());

	_vertexArrays.emplace_back(figureVertexArray);
	vertexArraySizes.emplace_back(vertexQuantity);
	_indexArrays.emplace_back(indices);
	indexArraySizes.emplace_back(indexQuantity);
    }

    void draw(std::vector<Figure2D>& figures,
	      const IndexArrays& indexArrays = {})
    {
	for (size_t i = 0; i < figures.size(); ++i)
	    draw(figures[i], indexArrays[i]);
    }

    void draw(std::vector<std::unique_ptr<Figure2D>>& figures,
	      const IndexArrays& indexArrays = {})
    {
	for (size_t i = 0; i < figures.size(); ++i)
	    draw(std::move(figures[i]), indexArrays[i]);
    }

    int run() {
        try {
            initWindow();
            initVulkan();
            mainLoop();
            cleanup();
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
};
