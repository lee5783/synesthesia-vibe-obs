//
//  opengl-loader.hpp
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on 2026-11-23.
//

#pragma once

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

void *opengl_get_proc(const char *name);
void opengl_log_platform(void);
