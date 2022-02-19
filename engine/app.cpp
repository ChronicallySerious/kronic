#include "app.h"

#include <set>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <string>

#include "shaderc/shaderc.hpp"

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
	static const char* message_type_string[3] = {
		"General",
		"Validation",
		"Performance"
	};

	const char* current_message_type
	    = (message_type == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)  ? message_type_string[0]
	    : message_type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ? message_type_string[1]
	                                                                     : message_type_string[2];

	switch (message_severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		Log::warn("Vulkan: {}: {}", current_message_type, callback_data->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		Log::error("Vulkan: {}: {}", current_message_type, callback_data->pMessage);
		break;
	default:
	    // Log::info("Vulkan: {}: {}", current_message_type, callback_data->pMessage);
	    ;
	}

	return VK_FALSE;
}

void HelloTriangleApplication::_init_window()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void HelloTriangleApplication::_add_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debug_callback;
}

void HelloTriangleApplication::_create_instance()
{
	if (enable_validation_layers && !_check_validation_layer_support())
	{
		throw std::runtime_error("Validation layers requested, but not available");
	}

	VkApplicationInfo app_info {};

	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Hello Triangle";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "kronic";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	uint32_t extension_count = 0;
	CHECK_VULKAN(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));

	std::vector<VkExtensionProperties> extensions(extension_count);
	CHECK_VULKAN(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data()));

	std::vector<const char*> glfw_extensions = _get_glfw_required_extensions();

	for (auto& glfw_extension_name : glfw_extensions)
	{
		bool found = false;
		for (auto& extension : extensions)
		{
			if (strcmp(extension.extensionName, glfw_extension_name) == 0)
			{
				found = true;

				Log::info("Vulkan: Found support for extension required by GLFW: {}", glfw_extension_name);
				break;
			}
		}

		if (!found)
		{
			Log::warn("Vulkan: Extension required by GLFW not found: {}", glfw_extension_name);
		}
	}

	create_info.enabledExtensionCount = glfw_extensions.size();
	create_info.ppEnabledExtensionNames = glfw_extensions.data();

	create_info.enabledLayerCount = 0;

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info {};
	if (enable_validation_layers)
	{
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();

		_add_debug_messenger_create_info(debug_create_info);
		create_info.pNext = &debug_create_info;
	}
	else
	{
		create_info.enabledLayerCount = 0;
		create_info.pNext = nullptr;
	}

	CHECK_VULKAN(vkCreateInstance(&create_info, nullptr, &instance));
}

std::vector<const char*> HelloTriangleApplication::_get_glfw_required_extensions()
{
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (enable_validation_layers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool HelloTriangleApplication::_check_validation_layer_support()
{
	uint32_t layer_count = 0;
	CHECK_VULKAN(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));

	std::vector<VkLayerProperties> available_layers(layer_count);
	CHECK_VULKAN(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()));

	for (const char* layer_name : validation_layers)
	{
		bool layer_found = false;

		for (const auto& layer_properties : available_layers)
		{
			if (strcmp(layer_name, layer_properties.layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
		{
			Log::error("Vulkan: Could not find layer support: {}", layer_name);
			return false;
		}
	}

	return true;
}

void HelloTriangleApplication::_setup_debug_messenger()
{
	if (!enable_validation_layers)
	{
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	_add_debug_messenger_create_info(create_info);

	CHECK_VULKAN_EXT(vkCreateDebugUtilsMessengerEXT, instance, &create_info, nullptr, &debug_messenger);
}

void HelloTriangleApplication::_init_vulkan()
{
	_create_instance();
	_setup_debug_messenger();
	_create_surface();
	_pick_physical_device();
	_create_logical_device();
	_create_swap_chain();
	_create_render_pass();
	_create_graphics_pipeline();
}

void HelloTriangleApplication::_main_loop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

void HelloTriangleApplication::_clean_up()
{
	if (enable_validation_layers)
	{
		VULKAN_EXT(vkDestroyDebugUtilsMessengerEXT, instance, debug_messenger, nullptr);
	}

	vkDestroyPipeline(device, graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
	vkDestroyRenderPass(device, render_pass, nullptr);

	for (auto& image_view : swap_chain_image_views)
	{
		vkDestroyImageView(device, image_view, nullptr);
	}

	vkDestroySwapchainKHR(device, swap_chain, nullptr);
	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}

void HelloTriangleApplication::_pick_physical_device()
{
	uint32_t device_count = 0;
	CHECK_VULKAN(vkEnumeratePhysicalDevices(instance, &device_count, nullptr));
	if (!device_count)
	{
		throw std::runtime_error("Failed to find a GPU with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	CHECK_VULKAN(vkEnumeratePhysicalDevices(instance, &device_count, devices.data()));

	uint32_t current_device_score = 0;

	for (const auto& device : devices)
	{
		uint32_t score = _calculate_device_score(device);
		if (score > current_device_score)
		{
			physical_device = device;
			current_device_score = score;
		}
	}

	if (physical_device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to find a suitable GPU");
	}
}

uint32_t HelloTriangleApplication::_calculate_device_score(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);

	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(device, &device_features);

	bool swap_chain_adequate = false;
	bool extensions_supported = _check_device_extension_support(device);

	if (extensions_supported)
	{
		SwapChainSupportDetails swap_chain_support = _query_swap_chain_support(device);
		swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();

		if (!swap_chain_adequate)
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}

	QueueFamilyIndices indices = _find_queue_families(device);

	// We only accept GPUs which support our required Vulkan queue families
	if (!indices.is_complete())
	{
		return 0;
	}

	// Application can't function without geometry shaders
	if (!device_features.geometryShader)
	{
		return 0;
	}

	uint32_t score = 0;

	// Discrete GPUs have a major performance advantage over non-discrete GPUs
	score += 1000 * static_cast<uint32_t>(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

	// Maximum possible size of textures affects graphics quality
	score += static_cast<uint32_t>(device_properties.limits.maxImageDimension2D);

	return score;
}

HelloTriangleApplication::QueueFamilyIndices HelloTriangleApplication::_find_queue_families(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto& queue_family : queue_families)
	{
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics_family = i;
		}

		VkBool32 present_support = false;
		CHECK_VULKAN(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support));
		if (present_support)
		{
			indices.present_family = i;
		}

		if (indices.is_complete())
		{
			break;
		}

		i++;
	}

	return indices;
}

void HelloTriangleApplication::_create_logical_device()
{
	QueueFamilyIndices indices = _find_queue_families(physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos {};
	std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families)
	{
		VkDeviceQueueCreateInfo queue_create_info {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	VkDeviceCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());

	VkPhysicalDeviceFeatures device_features {};
	create_info.pEnabledFeatures = &device_features;

	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	if (enable_validation_layers)
	{
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	}
	else
	{
		create_info.enabledLayerCount = 0;
	}

	CHECK_VULKAN(vkCreateDevice(physical_device, &create_info, nullptr, &device));

	vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
	vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}

void HelloTriangleApplication::_create_surface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface");
	}
}

bool HelloTriangleApplication::_check_device_extension_support(VkPhysicalDevice device)
{
	uint32_t extension_count;
	CHECK_VULKAN(vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr));

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	CHECK_VULKAN(vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data()));

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	for (const auto& extension : available_extensions)
	{
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

HelloTriangleApplication::SwapChainSupportDetails HelloTriangleApplication::_query_swap_chain_support(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	CHECK_VULKAN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities));

	uint32_t format_count = 0;
	CHECK_VULKAN(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr));

	if (format_count != 0)
	{
		details.formats.resize(format_count);
		CHECK_VULKAN(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data()));
	}

	uint32_t present_mode_count = 0;
	CHECK_VULKAN(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr));

	if (present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		CHECK_VULKAN(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data()));
	}

	return details;
}

VkSurfaceFormatKHR HelloTriangleApplication::_choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	for (const auto& available_format : available_formats)
	{
		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return available_format;
		}
	}

	return available_formats.front();
}

VkPresentModeKHR HelloTriangleApplication::_choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	for (const auto& available_present_mode : available_present_modes)
	{
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return available_present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::_choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}

	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actual_extent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
	};

	actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actual_extent;
}

void HelloTriangleApplication::_create_swap_chain()
{
	SwapChainSupportDetails swap_chain_support = _query_swap_chain_support(physical_device);

	VkSurfaceFormatKHR surface_format = _choose_swap_surface_format(swap_chain_support.formats);
	VkPresentModeKHR present_mode = _choose_swap_present_mode(swap_chain_support.present_modes);
	VkExtent2D extent = _choose_swap_extent(swap_chain_support.capabilities);

	uint32_t image_count = std::clamp(swap_chain_support.capabilities.minImageCount + 1, swap_chain_support.capabilities.minImageCount, swap_chain_support.capabilities.maxImageCount);

	VkSwapchainCreateInfoKHR create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1; // Always = 1 unless making a stereoscopic app
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = _find_queue_families(physical_device);
	uint32_t queue_family_indices[] = {
		indices.graphics_family.value(),
		indices.present_family.value(),
	};

	if (indices.graphics_family != indices.present_family)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swap chain");
	}

	CHECK_VULKAN(vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr));
	swap_chain_images.resize(image_count);
	CHECK_VULKAN(vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data()));

	swap_chain_image_format = surface_format.format;
	swap_chain_extent = extent;
}

void HelloTriangleApplication::_create_image_views()
{
	swap_chain_image_views.resize(swap_chain_images.size());

	for (int i = 0; i < swap_chain_images.size(); i++)
	{
		VkImageViewCreateInfo create_info {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = swap_chain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = swap_chain_image_format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &create_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image views");
		}
	}
}

void HelloTriangleApplication::_create_graphics_pipeline()
{
	VkShaderModule vert_shader_module = _compile_shader("engine/shaders/shader.vert", ShaderType::Vertex);
	VkShaderModule frag_shader_module = _compile_shader("engine/shaders/shader.frag", ShaderType::Fragment);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[2] = { vert_shader_stage_info, frag_shader_stage_info };

	// Input Assembly
	VkPipelineVertexInputStateCreateInfo vertex_input_info {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 0;
	vertex_input_info.pVertexBindingDescriptions = nullptr;
	vertex_input_info.vertexAttributeDescriptionCount = 0;
	vertex_input_info.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo input_assembly {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	// Viewport
	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swap_chain_extent.width;
	viewport.height = (float)swap_chain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor {};
	scissor.offset = { 0, 0 };
	scissor.extent = swap_chain_extent;

	VkPipelineViewportStateCreateInfo viewport_state {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Color blending
	VkPipelineColorBlendAttachmentState color_blend_attachment {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	// Dynamic State
	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamic_state {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = 2;
	dynamic_state.pDynamicStates = dynamic_states;

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 0;
	pipeline_layout_info.pSetLayouts = nullptr;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipeline_info {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = nullptr;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = &dynamic_state;
	pipeline_info.layout = pipeline_layout;
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	// Destroy
	vkDestroyShaderModule(device, frag_shader_module, nullptr);
	vkDestroyShaderModule(device, vert_shader_module, nullptr);
}

VkShaderModule HelloTriangleApplication::_compile_shader(const char* path, ShaderType type)
{
	std::vector<char> buffer;
	{
		std::ifstream shader_file { path, std::ios::ate | std::ios::binary };
		if (!shader_file.is_open())
		{
			throw std::runtime_error("Failed to open shader");
		}

		size_t file_size = shader_file.tellg();
		shader_file.seekg(0);
		buffer.resize(file_size);
		shader_file.read(buffer.data(), file_size);
	}

	const shaderc::Compiler compiler;

	shaderc::CompileOptions options;
	options.SetOptimizationLevel(shaderc_optimization_level_performance);

	shaderc_shader_kind kind = shaderc_glsl_default_vertex_shader;
	switch (type)
	{
	case ShaderType::Vertex:
		kind = shaderc_glsl_vertex_shader;
		break;
	case ShaderType::Fragment:
		kind = shaderc_glsl_fragment_shader;
		break;
	}

	const shaderc::SpvCompilationResult compilation_result = compiler.CompileGlslToSpv(std::string(buffer.begin(), buffer.end()), kind, path, options);

	if (compilation_result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		Log::error("Vulkan: Shader compilation error: {}", compilation_result.GetErrorMessage());
		throw std::runtime_error("Could not compile shader");
	}

	VkShaderModuleCreateInfo create_info {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = sizeof(uint32_t) * (compilation_result.end() - compilation_result.begin());
	create_info.pCode = reinterpret_cast<const uint32_t*>(compilation_result.begin());

	VkShaderModule shader_module;
	if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module");
	}

	return shader_module;
}

void HelloTriangleApplication::_create_render_pass()
{
	VkAttachmentDescription color_attachment {};
	color_attachment.format = swap_chain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkRenderPassCreateInfo render_pass_info {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;

	if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass");
	}
}
