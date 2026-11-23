//
//  opengl-loader-macos.cpp
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on 2026-11-23.
//

#include "opengl-loader.hpp"

#include <dlfcn.h>

namespace {

constexpr const char *opengl_framework_path = "/System/Library/Frameworks/OpenGL.framework/OpenGL";

}

void *opengl_get_proc(const char *name)
{
	static void *const opengl = dlopen(opengl_framework_path, RTLD_LAZY | RTLD_LOCAL);
	return opengl ? dlsym(opengl, name) : nullptr;
}

void opengl_log_platform(void)
{
}
