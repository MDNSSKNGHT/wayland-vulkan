//
// https://docs.vulkan.org/tutorial/latest/00_introduction.html
//
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client-core.h>

#include "renderer_vk.h"

//
// macros
//
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

//
// constants
//
const char* const c_validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};
const size_t c_validation_layers_count = ARRAY_SIZE(c_validation_layers);
#ifdef RENDERER_VK_DEBUG
const bool c_enable_validation_layers = true;
#else
const bool c_enable_validation_layers = false;
#endif
const char* const c_enabled_extension_names[] = {
	"VK_EXT_debug_utils",
	"VK_KHR_surface",
	"VK_KHR_wayland_surface"
};
const size_t c_enabled_extension_count = ARRAY_SIZE(c_enabled_extension_names);

//
// global variables
// g_ to avoid confusion with
// local variables.
//
VkInstance g_vk_instance = VK_NULL_HANDLE;

//
// functions
//
static bool renderer_vk_check_validation_layer_support(void) {
	uint32_t layer_count = 0;
	VkLayerProperties *available_layers;

	vkEnumerateInstanceLayerProperties(&layer_count, NULL);
	available_layers = malloc(layer_count * sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	for (int i = 0; i < c_validation_layers_count; i++) {
		bool layer_found = false;
		const char *layer_name = c_validation_layers[i];

		for (VkLayerProperties *layer_properties = available_layers;
				layer_properties < &available_layers[layer_count];
				++layer_properties) {
			if (!strcmp(layer_name, layer_properties->layerName)) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found) {
			return false;
		}
	}

	free(available_layers);

	return true;
}

static void renderer_vk_init(void) {
	VkApplicationInfo vk_app_info = {0};
	VkInstanceCreateInfo vk_create_info = {0};
	VkResult vk_result;
	//uint32_t extension_count = 0;
	//VkExtensionProperties *extensions;

	if (c_enable_validation_layers && !renderer_vk_check_validation_layer_support()) {
		fprintf(stderr, "validation layers requested, but not available!");
		exit(EXIT_FAILURE);
	}

	vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vk_app_info.pApplicationName = "HelloTriangle";
	vk_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	vk_app_info.pEngineName = "NoEngine";
	vk_app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vk_app_info.apiVersion = VK_API_VERSION_1_0;

	//vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
	//extensions = calloc(extension_count, sizeof(VkExtensionProperties));
	//vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extensions);

	//printf("available extensions:\n");
	//for (int i = 0 ; i < extension_count; i++) {
	//	VkExtensionProperties extension = extensions[i];
	//	printf("\t%s\n", extension.extensionName);
	//}

	vk_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vk_create_info.pApplicationInfo = &vk_app_info;
	vk_create_info.enabledExtensionCount = c_enabled_extension_count;
	vk_create_info.ppEnabledExtensionNames = c_enabled_extension_names;
	if (c_enable_validation_layers) {
		vk_create_info.enabledLayerCount = c_validation_layers_count;
		vk_create_info.ppEnabledLayerNames = c_validation_layers;
	} else  {
		vk_create_info.enabledLayerCount = 0;
	}

	vk_result = vkCreateInstance(&vk_create_info, NULL, &g_vk_instance);
	if (vk_result != VK_SUCCESS) {
		fprintf(stderr, "failed to create instance!");
		exit(EXIT_FAILURE);
	}
}

static void renderer_vk_init_wayland(struct wl_display *display) {
}

static void renderer_vk_main_loop(void) {
}

static void renderer_vk_cleanup(void) {
	vkDestroyInstance(g_vk_instance, NULL);
}

void renderer_vk_run(void) {
	renderer_vk_init();
	renderer_vk_main_loop();
	renderer_vk_cleanup();
}
