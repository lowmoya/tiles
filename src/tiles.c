
#include<vulkan/vulkan.h>
#include<GLFW/glfw3.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

/* Definitions. */

#define WINDOW_DEFAULT_WIDTH 800
#define WINDOW_DEFAULT_HEIGHT 600
#define WINDOW_TITLE "Tiles"

#define error(m) fputs("ERR | " m, stderr)
#define warn(m) fputs("WRN | " m , stderr)

/* Declarations. */
int initVulkan();
void deinitVulkan();

/* Global variables. */
struct {
	GLFWwindow * window;
	VkSurfaceKHR surface;
	VkInstance instance;
	VkDevice device;
	VkQueue graphics_queue;
	VkQueue presentation_queue;
} v;

/* Function definitions. */
int
initVulkan()
{
	/* Create window. */
	if (!glfwInit()) {
		error("Failed to load GLFW");
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	v.window = glfwCreateWindow(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT,
			WINDOW_TITLE, 0, 0);


	/* Create Vulkan instance. */
	uint32_t instance_extension_count;
	char const ** instance_extensions = glfwGetRequiredInstanceExtensions(&instance_extension_count);
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = WINDOW_TITLE,
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};
	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = instance_extension_count,
		.ppEnabledExtensionNames = instance_extensions,
#ifdef DEBUG
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = (char const * const []){"VK_LAYER_KHRONOS_validation"},
#endif
	};
	if (vkCreateInstance(&create_info, 0, &v.instance) != VK_SUCCESS) {
		error("Failed to create a Vulkan instance.");
		return 1;
	}

	/* Get surface. */
	if (glfwCreateWindowSurface(v.instance, v.window, 0, &v.surface) != VK_SUCCESS) {
		error("Failed to create a window surface for Vulkan.");
		return 1;
	}


	/* Pick a physical device and grab its queue information */
	uint32_t device_count;
	vkEnumeratePhysicalDevices(v.instance, &device_count, 0);
	VkPhysicalDevice * devices = malloc(device_count * sizeof(VkPhysicalDevice)); 
	vkEnumeratePhysicalDevices(v.instance, &device_count, devices);

	struct {
		int32_t index;
		int32_t score;
		int32_t queue_map;
		uint32_t graphics_queue;
		uint32_t presentation_queue;
	} selected_device = {
		.index = -1
	};

#define REQUIRED_EXTENSION_COUNT 1
	char const * const required_extensions[REQUIRED_EXTENSION_COUNT] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	for (int32_t i = 0; i < device_count; ++i) {
		// Get queue family information
		uint32_t family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, 0);
		VkQueueFamilyProperties * families = malloc(family_count * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, families);
		int32_t queue_map = 0;
		uint32_t graphics_queue;
		uint32_t presentation_queue;

		for (int32_t j = 0; j < family_count; ++j) {
			if ((families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					&& ((queue_map & 3) != 3 || graphics_queue != presentation_queue)) {
				graphics_queue = j;
				queue_map |= 1;
			}

			int32_t present_support = 0;
			vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, v.surface, &present_support);
			if (present_support
					&& ((queue_map & 3) != 3 || graphics_queue != presentation_queue)) {
				presentation_queue = j;
				queue_map |= 2;
			}
		}
		free(families);
		if (queue_map != 3)
			continue;

		// Check extension information
		uint32_t supported_extension_count;
		vkEnumerateDeviceExtensionProperties(devices[i], 0, &supported_extension_count, 0);
		VkExtensionProperties * supported_extensions =
			malloc(supported_extension_count * sizeof(VkExtensionProperties));
		vkEnumerateDeviceExtensionProperties(devices[i], 0, &supported_extension_count,
				supported_extensions);
		int8_t extensions_present = 1;
		for (int32_t ri = 0; ri < REQUIRED_EXTENSION_COUNT; ++ri) {
			int8_t found = 0;
			for (int32_t si = 0; si < supported_extension_count; ++si) {
				if (!strcmp(required_extensions[ri], supported_extensions[si].extensionName)) {
					found = 1;
					break;
				}
			}
			if (!found) {
				extensions_present = 0;
				break;
			}
		}

		free(supported_extensions);
		if (!extensions_present)
			continue;

		// Check swapchain details
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[i], v.surface, &capabilities);

		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], v.surface, &format_count, NULL);
		VkSurfaceFormatKHR * formats = malloc(format_count * sizeof(VkSurfaceFormatKHR));
		vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], v.surface, &format_count, formats);

		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], v.surface, &present_mode_count, 0);
		VkPresentModeKHR * present_modes = malloc(present_mode_count * sizeof(VkPresentModeKHR));
		vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], v.surface, &present_mode_count,
				present_modes);

		free(formats);
		free(present_modes);
		if (!format_count || !present_mode_count)
			continue;
		

		// Rating
		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceFeatures(devices[i], &device_features);
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(devices[i], &device_properties);

		int score =
			300 * (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);


		if (score > selected_device.score) {
			selected_device.index = i;
			selected_device.score = score;
			selected_device.queue_map = queue_map;
			selected_device.graphics_queue = graphics_queue;
			selected_device.presentation_queue = presentation_queue;
		}
	}
	if (selected_device.index == -1) {
		error("Failed to find a suitable graphics card");
		return 1;
	}
	VkPhysicalDevice device = devices[selected_device.index];
	free(devices);

	/* Set up queues for logical device. */
	uint32_t unique_queue_count = 2;
	uint32_t unique_queues[2] = {
		selected_device.graphics_queue,
		selected_device.presentation_queue
	};
	for (int32_t i = 1; i < unique_queue_count; ++i) {
		for (int32_t j = 0; j < i; ++j)  {
			if (unique_queues[j] == unique_queues[i]) {
				unique_queues[i--] = unique_queues[--unique_queue_count];
				break;
			}
		}
	}

	VkDeviceQueueCreateInfo * queue_create_infos =
		malloc(unique_queue_count * sizeof(VkDeviceQueueCreateInfo));
	float prio = 1.0;
	for (int32_t i = 0; i < unique_queue_count; ++i) {
		queue_create_infos[i] = (VkDeviceQueueCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = unique_queues[i],
			.queueCount = 1,
			.pQueuePriorities = &prio,
		};
	}
	
	/* Create logical device. */
	VkDeviceCreateInfo device_info = (VkDeviceCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = unique_queue_count,
		.pQueueCreateInfos = queue_create_infos,
		.enabledExtensionCount = REQUIRED_EXTENSION_COUNT,
		.ppEnabledExtensionNames = required_extensions,
#ifdef DEBUG
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = (char const * const []){"VK_LAYER_KHRONOS_validation"}
#endif
	};
	if (vkCreateDevice(device, &device_info, 0, &v.device) 
			!= VK_SUCCESS) {
		error("Failed to create Vulkan logical device.");
		return 1;
	}
	free(queue_create_infos);

	vkGetDeviceQueue(v.device, selected_device.graphics_queue, 0, &v.graphics_queue);
	vkGetDeviceQueue(v.device, selected_device.presentation_queue, 0, &v.presentation_queue);

	return 0;
}

void
deinitVulkan()
{
	vkDestroyDevice(v.device, 0);
	vkDestroySurfaceKHR(v.instance, v.surface, 0);
	vkDestroyInstance(v.instance, 0);
	glfwDestroyWindow(v.window);
	glfwTerminate();
}

int
main()
{
	int res;
	if ((res = initVulkan()))
		return res;

	while (!glfwWindowShouldClose(v.window)) {
		glfwSwapBuffers(v.window);
		glfwPollEvents();
	}

	deinitVulkan();
}
