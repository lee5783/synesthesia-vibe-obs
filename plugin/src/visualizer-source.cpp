//
//  visualizer-source.cpp
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on 2026-11-23.
//

#include "visualizer-source.hpp"

#include <obs-module.h>
#include <plugin-support.h>
#include <synesthesia-vibe/synesthesia-vibe.h>
#include <util/bmem.h>
#include <util/platform.h>

#include <OpenGL/gl3.h>
#include <dlfcn.h>

#include <array>
#include <cmath>
#include <mutex>
#include <string>

namespace {

constexpr const char *visualizer_source_id = "synesthesia_vibe_visualizer";
constexpr uint32_t visualizer_width = 1920;
constexpr uint32_t visualizer_height = 1080;
constexpr const char *setting_preset_path = "preset_path";
constexpr const char *default_preset_file = "presets/Rovastar - Fractopia (Upspoken Mix).milk";
constexpr const char *opengl_framework_path = "/System/Library/Frameworks/OpenGL.framework/OpenGL";
constexpr double tone_sample_rate = 44100.0;
constexpr int tone_channels = 2;

struct visualizer_source {
	obs_source_t *source;
	sv_context *context = nullptr;
	gs_texrender_t *texrender = nullptr;
	double pending_seconds = 0.0;
	bool frame_pending = false;
	double tone_time = 0.0;
	std::mutex preset_mutex;
	std::string pending_preset;
};

class gl_state_guard {
public:
	gl_state_guard()
	{
		glGetIntegerv(GL_CURRENT_PROGRAM, &program);
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertex_array);
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array_buffer);
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_framebuffer);
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_framebuffer);
		glGetIntegerv(GL_VIEWPORT, viewport.data());
		blend = glIsEnabled(GL_BLEND);
		glGetIntegerv(GL_BLEND_SRC_RGB, &blend_src_rgb);
		glGetIntegerv(GL_BLEND_DST_RGB, &blend_dst_rgb);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_src_alpha);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_dst_alpha);
		glGetIntegerv(GL_BLEND_EQUATION_RGB, &blend_equation_rgb);
		glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blend_equation_alpha);
		glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
		for (GLuint unit = 0; unit < GS_MAX_TEXTURES; unit++) {
			glActiveTexture(GL_TEXTURE0 + unit);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &textures_2d[unit]);
			glGetIntegerv(GL_TEXTURE_BINDING_3D, &textures_3d[unit]);
		}
		glActiveTexture(static_cast<GLenum>(active_texture));
	}

	~gl_state_guard()
	{
		for (GLuint unit = 0; unit < GS_MAX_TEXTURES; unit++) {
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textures_2d[unit]));
			glBindTexture(GL_TEXTURE_3D, static_cast<GLuint>(textures_3d[unit]));
		}
		glActiveTexture(static_cast<GLenum>(active_texture));
		if (blend)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);
		glBlendFuncSeparate(static_cast<GLenum>(blend_src_rgb), static_cast<GLenum>(blend_dst_rgb),
				    static_cast<GLenum>(blend_src_alpha), static_cast<GLenum>(blend_dst_alpha));
		glBlendEquationSeparate(static_cast<GLenum>(blend_equation_rgb),
					static_cast<GLenum>(blend_equation_alpha));
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read_framebuffer));
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw_framebuffer));
		glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(array_buffer));
		glBindVertexArray(static_cast<GLuint>(vertex_array));
		glUseProgram(static_cast<GLuint>(program));
	}

	gl_state_guard(const gl_state_guard &) = delete;
	gl_state_guard &operator=(const gl_state_guard &) = delete;

private:
	GLint program = 0;
	GLint vertex_array = 0;
	GLint array_buffer = 0;
	GLint draw_framebuffer = 0;
	GLint read_framebuffer = 0;
	std::array<GLint, 4> viewport = {};
	GLboolean blend = GL_FALSE;
	GLint blend_src_rgb = GL_ONE;
	GLint blend_dst_rgb = GL_ZERO;
	GLint blend_src_alpha = GL_ONE;
	GLint blend_dst_alpha = GL_ZERO;
	GLint blend_equation_rgb = GL_FUNC_ADD;
	GLint blend_equation_alpha = GL_FUNC_ADD;
	GLint active_texture = GL_TEXTURE0;
	std::array<GLint, GS_MAX_TEXTURES> textures_2d = {};
	std::array<GLint, GS_MAX_TEXTURES> textures_3d = {};
};

template<typename Call> void with_core_gl_state(Call call)
{
	gl_state_guard guard;
	call();
}

void *opengl_get_proc(const char *name)
{
	static void *const opengl = dlopen(opengl_framework_path, RTLD_LAZY | RTLD_LOCAL);
	return opengl ? dlsym(opengl, name) : nullptr;
}

std::string module_file_path(const char *file)
{
	char *path = obs_module_file(file);
	std::string result = path ? path : "";
	bfree(path);
	return result;
}

std::string preset_path_from_settings(obs_data_t *settings)
{
	const char *path = obs_data_get_string(settings, setting_preset_path);
	if (path && *path)
		return path;
	return module_file_path(default_preset_file);
}

void push_synthetic_tone(visualizer_source *visualizer, float seconds)
{
	const size_t frames = static_cast<size_t>(seconds * tone_sample_rate);
	if (!visualizer->context || frames == 0)
		return;

	static std::array<float, 8192> samples;
	const size_t chunk_frames = samples.size() / tone_channels;
	size_t remaining = frames;
	while (remaining > 0) {
		const size_t count = remaining < chunk_frames ? remaining : chunk_frames;
		for (size_t i = 0; i < count; i++) {
			const double t = visualizer->tone_time + static_cast<double>(i) / tone_sample_rate;
			const double pulse = std::pow(0.5 + 0.5 * std::sin(2.0 * M_PI * 2.0 * t), 4.0);
			const double value = pulse * (0.8 * std::sin(2.0 * M_PI * 60.0 * t) +
						      0.2 * std::sin(2.0 * M_PI * 880.0 * t));
			samples[i * tone_channels] = static_cast<float>(value);
			samples[i * tone_channels + 1] = static_cast<float>(value);
		}
		sv_push_pcm_float(visualizer->context, samples.data(), count, tone_channels);
		visualizer->tone_time += static_cast<double>(count) / tone_sample_rate;
		remaining -= count;
	}
}

void load_pending_preset(visualizer_source *visualizer)
{
	std::string path;
	{
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		path.swap(visualizer->pending_preset);
	}
	if (path.empty())
		return;

	sv_status status = SV_ERR_GL;
	with_core_gl_state([&] { status = sv_load_preset(visualizer->context, path.c_str(), 0.0f); });
	if (status == SV_OK)
		obs_log(LOG_INFO, "preset loaded: %s", path.c_str());
	else
		obs_log(LOG_WARNING, "preset failed to load: %s (%s)", path.c_str(),
			sv_last_error(visualizer->context));
}

const char *visualizer_get_name(void *)
{
	return obs_module_text("SynesthesiaVibeVisualizer");
}

void visualizer_update(void *data, obs_data_t *settings)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
	visualizer->pending_preset = preset_path_from_settings(settings);
}

void *visualizer_create(obs_data_t *settings, obs_source_t *source)
{
	visualizer_source *visualizer = new visualizer_source{source};

	const char *data_path = obs_get_module_data_path(obs_current_module());
	const std::string resource_path = data_path ? data_path : "";
	char *cache_path = obs_module_config_path("cache");
	if (cache_path)
		os_mkdirs(cache_path);

	sv_config config = {};
	config.width = static_cast<int>(visualizer_width);
	config.height = static_cast<int>(visualizer_height);
	config.resource_path = resource_path.c_str();
	config.cache_path = cache_path;

	obs_enter_graphics();
	sv_status gl_status = SV_ERR_GL;
	with_core_gl_state([&] {
		gl_status = sv_init_gl(opengl_get_proc);
		if (gl_status == SV_OK)
			visualizer->context = sv_create(&config);
	});
	visualizer->texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	obs_leave_graphics();

	bfree(cache_path);

	if (visualizer->context)
		obs_log(LOG_INFO, "sv_init_gl and sv_create succeeded (%ux%u)", visualizer_width, visualizer_height);
	else
		obs_log(LOG_WARNING, "core setup failed (sv_init_gl status %d, %s)", gl_status,
			sv_last_error(nullptr));

	visualizer_update(visualizer, settings);
	return visualizer;
}

void visualizer_destroy(void *data)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);

	obs_enter_graphics();
	if (visualizer->context)
		with_core_gl_state([&] { sv_destroy(visualizer->context); });
	gs_texrender_destroy(visualizer->texrender);
	obs_leave_graphics();

	delete visualizer;
}

uint32_t visualizer_get_width(void *)
{
	return visualizer_width;
}

uint32_t visualizer_get_height(void *)
{
	return visualizer_height;
}

void visualizer_video_tick(void *data, float seconds)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	visualizer->pending_seconds += seconds;
	visualizer->frame_pending = true;
	push_synthetic_tone(visualizer, seconds);
}

void render_core_frame(visualizer_source *visualizer)
{
	load_pending_preset(visualizer);

	gs_texrender_reset(visualizer->texrender);
	if (!gs_texrender_begin(visualizer->texrender, visualizer_width, visualizer_height))
		return;

	const bool previous_srgb = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(false);

	GLint target_fbo = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &target_fbo);
	with_core_gl_state([&] {
		sv_render_frame(visualizer->context, visualizer->pending_seconds, static_cast<uint32_t>(target_fbo));
	});

	gs_enable_framebuffer_srgb(previous_srgb);
	gs_texrender_end(visualizer->texrender);

	visualizer->pending_seconds = 0.0;
	visualizer->frame_pending = false;
}

void visualizer_video_render(void *data, gs_effect_t *)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	if (!visualizer->context || !visualizer->texrender)
		return;

	if (visualizer->frame_pending)
		render_core_frame(visualizer);

	gs_texture_t *texture = gs_texrender_get_texture(visualizer->texrender);
	if (!texture)
		return;

	const bool linear_srgb = gs_get_linear_srgb();
	const bool previous_srgb = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(linear_srgb);
	gs_blend_state_push();
	gs_enable_blending(false);

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	if (linear_srgb)
		gs_effect_set_texture_srgb(image, texture);
	else
		gs_effect_set_texture(image, texture);

	while (gs_effect_loop(effect, "Draw"))
		gs_draw_sprite(texture, GS_FLIP_V, 0, 0);

	gs_blend_state_pop();
	gs_enable_framebuffer_srgb(previous_srgb);
}

}

void visualizer_source_register(void)
{
	obs_source_info info = {};
	info.id = visualizer_source_id;
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
	info.get_name = visualizer_get_name;
	info.create = visualizer_create;
	info.destroy = visualizer_destroy;
	info.update = visualizer_update;
	info.get_width = visualizer_get_width;
	info.get_height = visualizer_get_height;
	info.video_tick = visualizer_video_tick;
	info.video_render = visualizer_video_render;
	obs_register_source(&info);
}
