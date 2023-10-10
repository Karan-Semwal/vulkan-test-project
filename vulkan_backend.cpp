#define GLFW_INCLUDE_VULKAN
#include "vulkan_backend.h"
#include <array>
#include <fstream>
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
	VkDebugUtilsMessageTypeFlagsEXT msgType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*)
{

	char pref[200];

	switch (msgType)
	{

	case 0x00000008:
		strcpy(pref,"[ADDR_BINDING:");
		break;
	case 0x00000004:
		strcpy(pref,"[PERFORMANCE:");
		break;
	case 0x00000002:
		strcpy(pref,"[VALIDATION:");
		break;
	case 0x00000001:
		strcpy(pref,"[GENERAL:");
		break;
	default:
		strcpy(pref,"[UNDEFINED:");
		break;
	};

	switch (msgSeverity)
	{
	case 0x00000001:
		strcat(pref,"VERBOSE:");
		break;
	case 0x00000100:
		strcat(pref,"WARNING:");
		break;
	case 0x00001000:
		strcat(pref,"ERROR:");
		break;
	case 0x00000010:
		strcat(pref,"INFO:");
		break;
	default:
		strcat(pref, "UNDEFINED:");
		break;
	};

	std::cout << pref << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void boilerPlate::imguiInit() {}
void boilerPlate::imguiDestroy() {}
void boilerPlate::drawUI(VkCommandBuffer*) {}
VkPipelineShaderStageCreateInfo boilerPlate::loadShader(const char* fileName,VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;

	std::ifstream file ( fileName , std::ios::ate | std::ios::binary );
	if( !file.is_open() ){
		std::cout << fileName << " shader file is missing\n"; 
	}

	size_t fileSize = (size_t)file.tellg();
	char* data = new char[fileSize];

	file.seekg(0,std::ios::beg);
	file.read(data, fileSize);
	file.close();
	assert(fileSize>0 && "shader size");

	VkShaderModule modul;
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = fileSize;
	createInfo.pCode =(uint32_t*)data;

	if ( vkCreateShaderModule (logicalDevice  , &createInfo , nullptr , &modul ) != VK_SUCCESS ) 
	{
		std::cout << fileName << " failure\n";
	}
	delete[] data;
	shaderStage.module = modul;
	shaderStage.pName = "main";
	assert(shaderStage.module != VK_NULL_HANDLE);
	return shaderStage;
}
void boilerPlate::LOG(VkResult res, const std::source_location location)
{
	std::string pref = "";
	if (res == 0)
	{
		pref = "\033[32m LOG\033[0m[";
	}
	else
	{
		pref = "\033[31m LOG\033[0m[";
		std::cout << pref << location.file_name() << ":" << location.function_name() << ":" << location.line() << ":" << res << "]" << std::endl;
	}

}
void boilerPlate::handleResizing(VkResult res)
{
	
	if( res != VK_ERROR_OUT_OF_DATE_KHR && res != VK_SUBOPTIMAL_KHR )
	{
		return;
	}
	generalInfo.setupSuccess = false;
	vkDeviceWaitIdle( logicalDevice );

	int tempHeight,tempWidth;
	glfwGetWindowSize( window, &tempWidth, &tempHeight );

	while( tempHeight == 0 || tempWidth == 0)
	{
		glfwGetWindowSize( window, &tempWidth, &tempHeight );
		glfwWaitEvents();
	}


	generalInfo.width = tempWidth;
	generalInfo.height = tempHeight;
	
	swapchainSetup();

	for( VkFramebuffer& frame : framebuffers )
	{
		vkDestroyFramebuffer( logicalDevice, frame, nullptr );
	}

	framebufferSetup();
	vkFreeCommandBuffers( logicalDevice, cmdPool, cmdBuffers.size(), cmdBuffers.data() );
	vkFreeCommandBuffers( logicalDevice, cmdPool, 1, &uploadBuffer );

	cmdBufferSetup();

	for( VkFence& fence : fences )
	{
		vkDestroyFence(logicalDevice,fence,nullptr);
	}
	vkDestroyFence(logicalDevice,uploadFence,nullptr);
	fenceSetup();
	generalInfo.setupSuccess = true;
}
int32_t boilerPlate::getQueueFamilyIndex( std::vector<VkQueueFamilyProperties> &props )
{
	for (uint8_t i = 0; i < props.size(); i++)
	{
		/**
		 * @attention remove this ugly thing later
		 */
		return i;
		VkBool32 presentSupported = VK_FALSE;
		if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupported) && (presentSupported == VK_TRUE))
			{
				return i;
			}
		}
	}
	throw std::runtime_error("Could not find a matching queue family index");
}
VkFence &boilerPlate::current(int)
{
	return fences[frameNumber % s_FRAME_OVERLAP];
}
VkCommandBuffer &boilerPlate::current(double)
{
	return cmdBuffers[frameNumber % s_FRAME_OVERLAP];
}
void boilerPlate::glfwInitialization()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER  , GLFW_TRUE);
	//glfwWindowHint(GLFW_DECORATED  , GLFW_FALSE);
	/** @note laggy resizing if collecting size & position from imgui window*/
	
	window = glfwCreateWindow(generalInfo.width, generalInfo.height, generalInfo.appName, nullptr, nullptr);

	if (!window)
	{
		glfwTerminate();
		exit(-1);
	}
	
}
void boilerPlate::run()
{
	if (!generalInfo.setupSuccess)
		return;

	// loop

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		uint32_t swapchainImageIndex;
		
		vkWaitForFences(logicalDevice, 1, &current(1), true, UINT64_MAX);

		handleResizing(vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, sync.sPresent, nullptr, &swapchainImageIndex));

		vkResetFences(logicalDevice, 1, &current(1));

		vkResetCommandBuffer(current(2.2), 0);

		VkCommandBufferBeginInfo cmdBeginInfo = info::bufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		vkBeginCommandBuffer(current(2.2), &cmdBeginInfo);

		VkRenderPassBeginInfo rpInfo{};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpInfo.renderPass = renderPass;
		rpInfo.renderArea = {0, 0, (uint32_t)generalInfo.width, (uint32_t)generalInfo.height};
		rpInfo.framebuffer = framebuffers[swapchainImageIndex];

		VkClearValue clearColor;
		clearColor.color = {0.0f, 0.0f, 0.0f};
		rpInfo.clearValueCount = 1;
		rpInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(current(2.2), &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		drawUI(&current(2.2));

		vkCmdEndRenderPass(current(2.2));
		vkEndCommandBuffer(current(2.2));

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submit = info::subInfo(&current(2.2));
		submit.pWaitDstStageMask = &waitStage;
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &sync.sPresent;
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &sync.sRender;

		vkQueueSubmit(graphicsQueue, 1, &submit, current(1));

		vkWaitForFences(logicalDevice, 1, &current(1), true, UINT64_MAX);

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.swapchainCount = 1;
		presentInfo.pImageIndices = &swapchainImageIndex;
		presentInfo.pWaitSemaphores = &sync.sRender;
		presentInfo.waitSemaphoreCount = 1;

		handleResizing(vkQueuePresentKHR(graphicsQueue, &presentInfo));

		frameNumber++;
	}
}
void boilerPlate::setup(bool validation, const char* appName)
{
	VALIDATION_ENABLED = validation;
	generalInfo.appName = appName;
	glfwInitialization();
	vulkanBoilerplate();
	imguiInit();
	generalInfo.setupSuccess = true;
}
void boilerPlate::cleanup()
{
	if (generalInfo.setupSuccess)
	{
		// kill window and gltf
		glfwDestroyWindow(window);
		glfwTerminate();

		vkWaitForFences(logicalDevice, 1, &current(1), true, UINT64_MAX);

		vkDeviceWaitIdle(logicalDevice);

		imguiDestroy();

		vkDestroyDescriptorPool(logicalDevice, imguiPool, nullptr);

		if (swapChain != VK_NULL_HANDLE)
		{
			for (uint8_t i = 0; i < imgCount; i++)
			{
				vkDestroyImageView  (logicalDevice, imageViews[i]   , nullptr);
				vkDestroyFramebuffer(logicalDevice, framebuffers[i] , nullptr);
			}
		}

		vkDestroySemaphore(logicalDevice, sync.sRender, nullptr);
		vkDestroySemaphore(logicalDevice, sync.sPresent, nullptr);

		for (auto &fence : fences)
		{
			vkDestroyFence(logicalDevice, fence, nullptr);
		}
		vkDestroyFence(logicalDevice, uploadFence, nullptr);
		
		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

		if (surface != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
			vkDestroySurfaceKHR(vulkInstance, surface, nullptr);
		}
		vkDestroyCommandPool(logicalDevice, cmdPool, nullptr);

		vkDestroyDevice(logicalDevice, nullptr);

		if (VALIDATION_ENABLED)
		{
			debuggerDestroyer(vulkInstance, debugger, nullptr);
		}
		vkDestroyInstance(vulkInstance, nullptr);
	}
}
VkResult boilerPlate::setupDebugger()
{
	PFN_vkCreateDebugUtilsMessengerEXT createDebugger =
		reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vulkInstance, "vkCreateDebugUtilsMessengerEXT"));
	debuggerDestroyer =
		reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(vulkInstance, "vkDestroyDebugUtilsMessengerEXT"));

	VkDebugUtilsMessengerCreateInfoEXT debugutilsmessengerInfo{};
	debugutilsmessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	debugutilsmessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

	debugutilsmessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
								  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;

	debugutilsmessengerInfo.pfnUserCallback = debugCallback;
	return createDebugger(vulkInstance, &debugutilsmessengerInfo, nullptr, &debugger);
}

VkCommandBufferAllocateInfo info::commandBufferAllocate(VkCommandPool pool, uint32_t count, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	inf.pNext = nullptr;

	inf.commandBufferCount = count;
	inf.commandPool = pool;
	inf.level = level;

	return inf;
}
void boilerPlate::fastSubmit(std::function<void(VkCommandBuffer cmd)> &&function)
{
	/** @attention replace with already created fence/cmb*/

	VkSubmitInfo             submit    = info::subInfo(&uploadBuffer);
	VkCommandBufferBeginInfo beginInfo = info::bufferBeginInfo(0);
	LOG(vkResetFences(logicalDevice,1,&uploadFence));
	LOG(vkResetCommandBuffer(uploadBuffer,0));

	LOG(vkBeginCommandBuffer(uploadBuffer, &beginInfo));

	function(uploadBuffer);
	
	LOG(vkEndCommandBuffer(uploadBuffer));

	LOG(vkQueueSubmit(graphicsQueue, 1, &submit, uploadFence));
	LOG(vkWaitForFences(logicalDevice, 1, &uploadFence, VK_TRUE, UINT64_MAX));
}
void boilerPlate::vulkanBoilerplate()
{
	// get rquired glfw extensions
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	VkApplicationInfo appInf{};
	appInf.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	appInf.apiVersion = VK_API_VERSION_1_0;
	appInf.pEngineName = nullptr;
	appInf.engineVersion = 0;
	appInf.applicationVersion = 0;

	std::vector<const char*> instanceExtensions; // = { VK_KHR_SURFACE_EXTENSION_NAME };
	// Get extensions supported by the instance and store for later use
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	std::vector<const char*> supportedInstanceExtensions;

	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
		{
			for (VkExtensionProperties extension : extensions)
			{

				supportedInstanceExtensions.push_back(extension.extensionName);
			}
		}
	}
	// Enabled requested instance extensions
	for (const char* enabledExtension : requiredExtensions)
	{
		// Output message if requested extension is not available
		if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) == supportedInstanceExtensions.end())
		{
			std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
		}
		instanceExtensions.push_back(enabledExtension);
	}

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = NULL;
	instanceInfo.pApplicationInfo = &appInf;

	if (VALIDATION_ENABLED)
	{
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		// Check if this layer is available at instance level
		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
		bool validationLayerPresent = false;
		for (VkLayerProperties layer : instanceLayerProperties)
		{
			if (strcmp(layer.layerName, validationLayerName) == 0)
			{
				validationLayerPresent = true;
				break;
			}
		}
		if (validationLayerPresent)
		{
			instanceInfo.ppEnabledLayerNames = &validationLayerName;
			instanceInfo.enabledLayerCount = 1;
		}
		else
		{
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}
	instanceInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
	instanceInfo.ppEnabledExtensionNames = instanceExtensions.data();
	LOG(vkCreateInstance(&instanceInfo, nullptr, &vulkInstance));

	LOG(glfwCreateWindowSurface(vulkInstance, window, nullptr, &surface));

	if (VALIDATION_ENABLED)
	{
		setupDebugger();
	}

	uint32_t deviceCount;

	vkEnumeratePhysicalDevices(vulkInstance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vulkInstance, &deviceCount, devices.data());

	// usually it is the first listed GPU we need
	physicalDevice = devices[0];

	// Memory properties are used regularly for creating all kinds of buffers
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProps);

	uint32_t queueFamilyCount = 0;
	uint32_t graphIndex = 0;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	for (uint8_t i = 0; i < queueFamilyCount; i++)
	{
		if (getQueueFamilyIndex(queueFamilyProperties) != -1)
		{
			graphIndex = i;
			break;
		}
	}

	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &swapchainDetails.formatCount, nullptr);
	
	
	// vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice , surface , &swapchainDetails.pModesCount , nullptr );

	swapchainDetails.surfaceFormats.resize(swapchainDetails.formatCount);
	// swapchainDetails.presentModes.resize(swapchainDetails.pModesCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &swapchainDetails.formatCount, swapchainDetails.surfaceFormats.data());
	// vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &swapchainDetails.pModesCount, swapchainDetails.presentModes.data());

	const float defaultQueuePriority(0.0f);

	// Graphics queue
	VkDeviceQueueCreateInfo queueInfo{};
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &defaultQueuePriority;
	queueInfo.queueFamilyIndex = graphIndex;

	const std::vector<const char*> enabledDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.pEnabledFeatures = nullptr;
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();

	LOG(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &logicalDevice));

	VkCommandPoolCreateInfo commandpoolInfo{};
	commandpoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandpoolInfo.queueFamilyIndex = queueInfo.queueFamilyIndex;
	commandpoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	LOG(vkCreateCommandPool(logicalDevice, &commandpoolInfo, nullptr, &cmdPool));

	vkGetDeviceQueue(logicalDevice, queueInfo.queueFamilyIndex, 0, &graphicsQueue);

	colorFormat = swapchainDetails.surfaceFormats[0].format;
	colorSpace = swapchainDetails.surfaceFormats[0].colorSpace;
	std::vector<VkFormat> preferredImageFormats = {
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_A8B8G8R8_UNORM_PACK32};

	for (auto &availableFormat : swapchainDetails.surfaceFormats)
	{
		if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format) != preferredImageFormats.end())
		{
			colorFormat = availableFormat.format;
			colorSpace = availableFormat.colorSpace;
			break;
		}
	}
	// color attachment
	VkAttachmentDescription attachment{};
	attachment.flags = 0;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.format = colorFormat;

	// color reference
	VkAttachmentReference reference{};
	reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	reference.attachment = 0;

	// subpass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &reference;
	subpass.pDepthStencilAttachment = nullptr;

	// color dependency
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;

	VkRenderPassCreateInfo renderpassInfo = {};
	renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpassInfo.attachmentCount = 1;
	renderpassInfo.dependencyCount = 1;
	renderpassInfo.pDependencies = &dependency;
	renderpassInfo.pAttachments = &attachment;
	renderpassInfo.subpassCount = 1;
	renderpassInfo.pSubpasses = &subpass;

	LOG(vkCreateRenderPass(logicalDevice, &renderpassInfo, nullptr, &renderPass));

	swapchainSetup();
	framebufferSetup();
	fenceSetup();
	// create synchronization structures
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	LOG(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &sync.sPresent));
	LOG(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &sync.sRender));

	cmdBufferSetup();
}
void boilerPlate::fenceSetup()
{
	VkFenceCreateInfo fenceInfo = info::fence(VK_FENCE_CREATE_SIGNALED_BIT);
	for (auto &fence : fences)
	{
		LOG(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
	}
	LOG(vkCreateFence(logicalDevice,&fenceInfo,nullptr,&uploadFence));
}
void boilerPlate::cmdBufferSetup()
{
	VkCommandBufferAllocateInfo cmdAllocInfo = info::commandBufferAllocate(cmdPool, cmdBuffers.size(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	LOG(vkAllocateCommandBuffers(logicalDevice, &cmdAllocInfo, cmdBuffers.data()));

	cmdAllocInfo.commandBufferCount = 1;
	LOG(vkAllocateCommandBuffers(logicalDevice, &cmdAllocInfo, &uploadBuffer));
}
void boilerPlate::swapchainSetup()
{
	VkSwapchainKHR oldswapChain = swapChain;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapchainDetails.capabilities);
	// Determine the number of images
	uint8_t desiredNumberOfSwapchainImages = std::max(swapchainDetails.capabilities.minImageCount, 3u);

	if ((swapchainDetails.capabilities.maxImageCount > 0) && (desiredNumberOfSwapchainImages > swapchainDetails.capabilities.maxImageCount))
	{
		desiredNumberOfSwapchainImages = swapchainDetails.capabilities.maxImageCount;
	}

	// Find the transformation of the surface
	VkSurfaceTransformFlagsKHR preTransform;
	if (swapchainDetails.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		// We prefer a non-rotated transform
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = swapchainDetails.capabilities.currentTransform;
	}

	// Find a supported composite alpha format (not all devices support alpha opaque)
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// Simply select the first composite alpha format available
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};

	for (auto &compositeAlphaFlag : compositeAlphaFlags)
	{
		if (swapchainDetails.capabilities.supportedCompositeAlpha & compositeAlphaFlag)
		{
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}
	VkSwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

	swapchainInfo.clipped 				= VK_TRUE;
	swapchainInfo.surface 			    = surface;
	swapchainInfo.imageUsage 			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.imageFormat 			= colorFormat;
	swapchainInfo.presentMode 			= VK_PRESENT_MODE_FIFO_KHR;
	swapchainInfo.imageExtent 			= {generalInfo.width, generalInfo.height};
	swapchainInfo.oldSwapchain 		    = oldswapChain;
	swapchainInfo.preTransform 			= (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainInfo.minImageCount 		= desiredNumberOfSwapchainImages;
	swapchainInfo.compositeAlpha 		= compositeAlpha;
	swapchainInfo.imageColorSpace 		= colorSpace;
	swapchainInfo.imageArrayLayers 		= 1;
	swapchainInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.queueFamilyIndexCount = 0;

	// Enable transfer source on swap chain images if supported
	if (swapchainDetails.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		swapchainInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (swapchainDetails.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	{
		swapchainInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	LOG(vkCreateSwapchainKHR(logicalDevice, &swapchainInfo, nullptr, &swapChain));

	if(oldswapChain != VK_NULL_HANDLE)
	{
		for( uint8_t i = 0 ; i < imgCount ; i++)
		{
			vkDestroyImageView(logicalDevice,imageViews[i],nullptr);
		}
		vkDestroySwapchainKHR(logicalDevice,oldswapChain,nullptr);
	}

	LOG(vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imgCount, NULL));

	images.resize(imgCount);
	imageViews.resize(imgCount);
	LOG(vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imgCount, images.data()));

}
void boilerPlate::framebufferSetup()
{
	framebuffers.resize(imgCount);
	for (uint8_t i = 0; i < imgCount; i++)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = colorFormat;
		viewInfo.image = images[i];
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

		LOG(vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageViews[i]));

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &imageViews[i];

		framebufferInfo.width = generalInfo.width;
		framebufferInfo.height = generalInfo.height;
		framebufferInfo.layers = 1;

		LOG(vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &framebuffers[i]));
	}
}
VkCommandBufferBeginInfo info::bufferBeginInfo(VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	inf.pNext = nullptr;

	inf.pInheritanceInfo = nullptr;
	inf.flags = flags;

	return inf;
}
VkSubmitInfo info::subInfo(VkCommandBuffer* cmd)
{
	VkSubmitInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	inf.pNext = nullptr;

	inf.signalSemaphoreCount = 0;
	inf.waitSemaphoreCount = 0;
	inf.commandBufferCount = 1;
	inf.pWaitDstStageMask = nullptr;
	inf.pSignalSemaphores = nullptr;
	inf.pWaitSemaphores = nullptr;
	inf.pCommandBuffers = cmd;

	return inf;
}
VkFenceCreateInfo info::fence(VkFenceCreateFlags flags)
{
	VkFenceCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	inf.pNext = nullptr;

	inf.flags = flags;

	return inf;
}
 VkDescriptorSetLayoutBinding info::descriptorSetLayoutBinding(VkDescriptorType type,VkShaderStageFlags flags, uint32_t binding,uint32_t descriptorCount)
{
	VkDescriptorSetLayoutBinding inf{};
	
	inf.binding   	    = binding;
	inf.stageFlags      = flags;
	inf.descriptorType  = type;
	inf.descriptorCount = descriptorCount;

	return inf;
}
 VkDescriptorSetLayoutCreateInfo info::descriptorSetLayout(const VkDescriptorSetLayoutBinding* bindings,uint32_t bindingCount,VkDescriptorSetLayoutCreateFlags flags)
{
	VkDescriptorSetLayoutCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	inf.pNext = nullptr;

	inf.flags		 = flags;
	inf.pBindings    = bindings;
	inf.bindingCount = bindingCount;

	return inf;
}
 VkPipelineInputAssemblyStateCreateInfo info::input_assembly_state( VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags,VkBool32 primitiveRestartEnable)
{
	VkPipelineInputAssemblyStateCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inf.pNext = nullptr;

	inf.flags 				   = flags;
	inf.topology 			   = topology;
	inf.primitiveRestartEnable = primitiveRestartEnable;

	return inf;
}
 VkPipelineRasterizationStateCreateInfo info::rasterization_state( VkPolygonMode polygonMode,VkCullModeFlags cullMode,VkFrontFace frontFace,VkPipelineRasterizationStateCreateFlags flags )
{
	VkPipelineRasterizationStateCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	inf.pNext = nullptr;

	inf.rasterizerDiscardEnable = VK_FALSE;
	inf.depthBiasConstantFactor = 0.0f;
	inf.depthBiasSlopeFactor    = 0.0f;
	inf.depthClampEnable 		= VK_FALSE;
	inf.depthBiasEnable 		= VK_FALSE;
	inf.depthBiasClamp 			= 0.0f;
	inf.polygonMode 			= polygonMode;
	inf.lineWidth 				= 1.0f;
	inf.frontFace				= VK_FRONT_FACE_CLOCKWISE;
	inf.cullMode 				= VK_CULL_MODE_NONE;

	return inf;
}
VkPipelineMultisampleStateCreateInfo info::multisampling_state(VkSampleCountFlagBits rasterizationSamples,VkPipelineMultisampleStateCreateFlags flags)
{
	VkPipelineMultisampleStateCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	inf.pNext = nullptr;

	inf.rasterizationSamples  = rasterizationSamples;
	inf.flags 		          = flags;

	return inf;
}
VkPipelineDynamicStateCreateInfo info::dynamic_state(const VkDynamicState * pDynamicStates,uint32_t dynamicStateCount,VkPipelineDynamicStateCreateFlags flags)
{
	VkPipelineDynamicStateCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	inf.pNext = nullptr;

	inf.flags 			  = flags;
	inf.pDynamicStates    = pDynamicStates;
	inf.dynamicStateCount = dynamicStateCount;
	
	return inf;
}
 VkPipelineVertexInputStateCreateInfo info::vertex_input_state(uint32_t vACount , uint32_t vBCount ,
const VkVertexInputAttributeDescription *vAttrDesk,const VkVertexInputBindingDescription *vBindDesk,
VkPipelineVertexInputStateCreateFlags flags) {

	VkPipelineVertexInputStateCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	inf.pNext = nullptr;


	inf.vertexAttributeDescriptionCount = vACount;
	inf.vertexBindingDescriptionCount   = vBCount;
	inf.pVertexAttributeDescriptions    = vAttrDesk;
	inf.pVertexBindingDescriptions      = vBindDesk;
	inf.flags 							= flags;

	return inf;
}
 VkPipelineColorBlendStateCreateInfo info::color_blend_state(int32_t attachmentCount,const VkPipelineColorBlendAttachmentState * pAttachments)
{
	VkPipelineColorBlendStateCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	
	inf.logicOp 	    = VK_LOGIC_OP_COPY;
	inf.pAttachments 	= pAttachments;
	inf.logicOpEnable   = VK_FALSE;
	inf.attachmentCount = attachmentCount;

	return inf;
}
 VkPipelineDepthStencilStateCreateInfo  info::depth_stencil(VkBool32 depthTestEnable,VkBool32 depthWriteEnable,VkCompareOp depthCompareOp)
{
	VkPipelineDepthStencilStateCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	inf.pNext = nullptr;

	inf.back.compareOp	  = VK_COMPARE_OP_ALWAYS;
	inf.depthCompareOp    = depthCompareOp;
	inf.depthTestEnable   = depthTestEnable;
	inf.depthWriteEnable  = depthWriteEnable;

	return inf;
}
 VkPipelineViewportStateCreateInfo info::viewport_state(const VkViewport* viewport,const VkRect2D* scissor,uint32_t viewportCount,uint32_t scissorCount,VkPipelineViewportStateCreateFlags flags)
{
	VkPipelineViewportStateCreateInfo inf{};
	inf.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	inf.pNext = nullptr;

	inf.flags 		  = flags;
	inf.pScissors 	  = scissor;
	inf.pViewports 	  = viewport;
	inf.scissorCount  = scissorCount;
	inf.viewportCount = viewportCount;

	return inf;
}
 VkVertexInputAttributeDescription info::vertex_input_attr(VkFormat format,uint32_t binding,uint32_t location,uint32_t offset)
{
	VkVertexInputAttributeDescription inf{};
	
	inf.format   = format;
	inf.offset   = offset;
	inf.binding  = binding;
	inf.location = location;

	return inf;
}
 VkPipelineColorBlendAttachmentState info::color_blend_attachment_state(VkColorComponentFlags colorWriteMask,VkBool32 blendEnable) 
{
	VkPipelineColorBlendAttachmentState inf{};
	inf.colorWriteMask 				 =  colorWriteMask;
	inf.blendEnable = blendEnable;
	return inf;
}
 VkVertexInputBindingDescription info::vertex_input_bind(uint32_t stride,uint32_t binding,VkVertexInputRate inputRate)
{
	VkVertexInputBindingDescription inf{};
	
	inf.stride    = stride;
	inf.binding   = binding;
	inf.inputRate = inputRate;

	return inf;
}
 VkGraphicsPipelineCreateInfo info::graphicsPipeline(VkPipelineLayout layout,VkRenderPass renderPass,VkPipelineCreateFlags flags)
{
	VkGraphicsPipelineCreateInfo inf {};
	inf.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	inf.flags 			   = flags;
	inf.layout 		       = layout;
	inf.renderPass 		   = renderPass;
	inf.basePipelineIndex  = -1;
	inf.basePipelineHandle = VK_NULL_HANDLE;
	
	return inf;
}