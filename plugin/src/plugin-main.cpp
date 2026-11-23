//
//  plugin-main.cpp
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on 2026-11-23.
//

#include <obs-module.h>
#include <plugin-support.h>

#include "visualizer-source.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace {

bool renderer_is_opengl(void)
{
	obs_enter_graphics();
	const int device_type = gs_get_device_type();
	const char *device_name = gs_get_device_name();
	obs_leave_graphics();

	if (device_type != GS_DEVICE_OPENGL) {
		obs_log(LOG_WARNING,
			"SynesthesiaVibe Visualizer needs the OpenGL renderer; current renderer is %s (device type %d). "
			"The source is not registered.",
			device_name ? device_name : "unavailable", device_type);
		return false;
	}

	obs_log(LOG_INFO, "renderer check passed (%s, device type %d)", device_name ? device_name : "unavailable",
		device_type);
	return true;
}

}

bool obs_module_load(void)
{
	if (!renderer_is_opengl())
		return true;
	visualizer_source_register();
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
