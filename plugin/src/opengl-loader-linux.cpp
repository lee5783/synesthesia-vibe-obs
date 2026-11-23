//
//  opengl-loader-linux.cpp
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on 2026-11-23.
//

#include "opengl-loader.hpp"

#include <obs-module.h>
#include <obs-nix-platform.h>
#include <plugin-support.h>

#include <EGL/egl.h>
#include <dlfcn.h>

#include <array>

namespace {

constexpr std::array<const char *, 2> opengl_library_names = {"libOpenGL.so.0", "libGL.so.1"};

const char *nix_platform_name(enum obs_nix_platform_type platform)
{
	switch (platform) {
	case OBS_NIX_PLATFORM_X11_EGL:
		return "X11 (EGL)";
	case OBS_NIX_PLATFORM_WAYLAND:
		return "Wayland (EGL)";
	default:
		return "unknown";
	}
}

void *opengl_library_proc(const char *name)
{
	static const std::array<void *, opengl_library_names.size()> libraries = [] {
		std::array<void *, opengl_library_names.size()> handles = {};
		for (size_t index = 0; index < opengl_library_names.size(); index++)
			handles[index] = dlopen(opengl_library_names[index], RTLD_LAZY | RTLD_LOCAL);
		return handles;
	}();

	for (void *library : libraries) {
		void *proc = library ? dlsym(library, name) : nullptr;
		if (proc)
			return proc;
	}
	return nullptr;
}

}

void *opengl_get_proc(const char *name)
{
	void *proc = reinterpret_cast<void *>(eglGetProcAddress(name));
	return proc ? proc : opengl_library_proc(name);
}

void opengl_log_platform(void)
{
	obs_log(LOG_INFO, "windowing platform: %s", nix_platform_name(obs_get_nix_platform()));
}
