//
//  visualizer-source.cpp
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on 2026-11-23.
//

#include "visualizer-source.hpp"
#include "audio-ring.hpp"

#include <media-io/audio-resampler.h>
#include <obs-module.h>
#include <plugin-support.h>
#include <synesthesia-vibe/synesthesia-vibe.h>
#include <util/bmem.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/task.h>

#include <OpenGL/gl3.h>
#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <deque>
#include <mutex>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr const char *visualizer_source_id = "synesthesia_vibe_visualizer";
constexpr uint32_t default_width = 1920;
constexpr uint32_t default_height = 1080;
constexpr long long min_output_size = 128;
constexpr long long max_output_size = 8192;
constexpr long long full_render_scale = 100;
constexpr long long three_quarter_render_scale = 75;
constexpr long long half_render_scale = 50;
constexpr const char *setting_width = "width";
constexpr const char *setting_height = "height";
constexpr const char *setting_render_scale = "render_scale";
constexpr const char *setting_preset_folder = "preset_folder";
constexpr const char *setting_preset = "preset";
constexpr const char *setting_preset_hint = "preset_hint";
constexpr const char *setting_random_order = "random_order";
constexpr const char *setting_cycle_seconds = "cycle_seconds";
constexpr const char *setting_blend_seconds = "blend_seconds";
constexpr const char *setting_audio_source = "audio_source";
constexpr const char *setting_audio_mix_track = "audio_mix_track";
constexpr const char *audio_source_whole_mix = "whole_mix";
constexpr const char *group_presets = "presets_group";
constexpr const char *group_transitions = "transitions_group";
constexpr const char *group_audio = "audio_group";
constexpr const char *group_output = "output_group";
constexpr std::array<const char *, MAX_AUDIO_MIXES> mix_track_labels = {
	"MixTrack.Track1", "MixTrack.Track2", "MixTrack.Track3",
	"MixTrack.Track4", "MixTrack.Track5", "MixTrack.Track6"};
constexpr const char *hotkey_next_preset = "SynesthesiaVibe.NextPreset";
constexpr const char *hotkey_previous_preset = "SynesthesiaVibe.PreviousPreset";
constexpr const char *hotkey_random_preset = "SynesthesiaVibe.RandomPreset";
constexpr const char *hotkey_pause_auto_cycle = "SynesthesiaVibe.PauseAutoCycle";
constexpr const char *preset_extension = ".milk";
constexpr const char *opengl_framework_path = "/System/Library/Frameworks/OpenGL.framework/OpenGL";
constexpr uint32_t core_sample_rate = 44100;
constexpr size_t core_audio_frames = 2304;
constexpr size_t audio_ring_frames = 65536;
constexpr uint64_t audio_check_interval_ns = 1000000000;
constexpr uint64_t audio_silence_after_ns = 500000000;
constexpr long long default_audio_mix_track = 1;
constexpr long long default_cycle_seconds = 16;
constexpr long long max_cycle_seconds = 3600;
constexpr double default_blend_seconds = 2.7;
constexpr double max_blend_seconds = 30.0;
constexpr double max_step_seconds = 0.1;
constexpr double nanoseconds_per_second = 1e9;
constexpr double nanoseconds_per_millisecond = 1e6;
constexpr size_t preset_error_length = 1024;
constexpr size_t max_preset_history = 50;
constexpr size_t visualizer_hotkey_count = 4;

enum class preset_command { none, next, previous, random };

struct output_size {
	uint32_t width;
	uint32_t height;
};

struct visualizer_source {
	obs_source_t *source;
	sv_context *context = nullptr;
	gs_texrender_t *texrender = nullptr;
	std::atomic<output_size> size{output_size{default_width, default_height}};
	std::atomic<long long> render_scale{full_render_scale};
	output_size core_size = {0, 0};
	std::atomic<uint64_t> pending_nanoseconds{0};
	std::atomic<bool> frame_pending{false};
	os_task_queue_t *preset_worker = nullptr;
	std::mutex preset_mutex;
	std::string preset_folder;
	std::string preset_file;
	uint64_t preset_generation = 0;
	uint64_t preset_handled_generation = 0;
	bool preset_closing = false;
	std::string preset_target;
	std::string preset_cursor;
	sv_prepared_preset *ready_preset = nullptr;
	std::string ready_preset_path;
	float ready_preset_blend = 0.0f;
	bool cycle_requested = false;
	bool cycle_deferred = false;
	bool cycle_paused = false;
	float cycle_blend = 0.0f;
	long long cycle_seconds = -1;
	double blend_seconds = 0.0;
	bool random_order = false;
	bool transition_dirty = false;
	preset_command pending_command = preset_command::none;
	std::deque<std::string> preset_history;
	std::mt19937 preset_random{static_cast<std::mt19937::result_type>(os_gettime_ns())};
	std::array<obs_hotkey_id, visualizer_hotkey_count> hotkeys = {OBS_INVALID_HOTKEY_ID, OBS_INVALID_HOTKEY_ID,
								     OBS_INVALID_HOTKEY_ID, OBS_INVALID_HOTKEY_ID};
	std::mutex audio_mutex;
	std::string audio_source_uuid;
	size_t audio_mix_index = 0;
	bool audio_mix_connected = false;
	obs_weak_source_t *audio_weak_source = nullptr;
	audio_resampler_t *audio_resampler = nullptr;
	uint64_t audio_check_time = 0;
	bool audio_wait_logged = false;
	audio_ring audio{audio_ring_frames};
	std::array<float, core_audio_frames * audio_ring::channels> audio_frames = {};
	uint64_t last_audio_time = 0;
	bool audio_silenced = false;
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

double milliseconds_since(uint64_t start)
{
	return static_cast<double>(os_gettime_ns() - start) / nanoseconds_per_millisecond;
}

bool is_preset_file(const os_dirent *entry)
{
	const char *extension = os_get_path_extension(entry->d_name);
	return !entry->directory && entry->d_name[0] != '.' && extension && astrcmpi(extension, preset_extension) == 0;
}

std::vector<std::string> list_preset_files(const std::string &folder)
{
	std::vector<std::string> files;
	os_dir_t *dir = folder.empty() ? nullptr : os_opendir(folder.c_str());
	if (!dir)
		return files;

	for (os_dirent *entry = os_readdir(dir); entry; entry = os_readdir(dir)) {
		if (is_preset_file(entry))
			files.emplace_back(entry->d_name);
	}
	os_closedir(dir);

	std::sort(files.begin(), files.end(),
		  [](const std::string &a, const std::string &b) { return astrcmpi(a.c_str(), b.c_str()) < 0; });
	return files;
}

std::string preset_label(const std::string &file)
{
	const char *extension = os_get_path_extension(file.c_str());
	return extension ? std::string(file.c_str(), extension) : file;
}

std::string preset_path(const std::string &folder, const std::string &file)
{
	return folder + "/" + file;
}

std::string listed_or_first(const std::vector<std::string> &files, const std::string &file)
{
	return std::find(files.begin(), files.end(), file) != files.end() ? file : files.front();
}

void log_missing_presets(const std::string &folder)
{
	if (folder.empty())
		obs_log(LOG_WARNING, "preset folder not set; choose a folder of .milk presets");
	else if (!os_file_exists(folder.c_str()))
		obs_log(LOG_WARNING, "preset folder not found: %s", folder.c_str());
	else
		obs_log(LOG_WARNING, "preset folder has no .milk presets: %s", folder.c_str());
}

struct preset_job {
	std::string folder;
	std::string file;
	std::string target;
	uint64_t generation = 0;
	float blend_seconds = 0.0f;
	bool next = false;
	bool random = false;
	preset_command command = preset_command::none;
	std::string history_path;
};

bool cycle_request_servable(const visualizer_source *visualizer)
{
	return visualizer->cycle_requested && !visualizer->ready_preset;
}

void clear_cycle_request(visualizer_source *visualizer)
{
	visualizer->cycle_requested = false;
	visualizer->cycle_deferred = false;
}

std::string previous_from_history(visualizer_source *visualizer)
{
	std::deque<std::string> &history = visualizer->preset_history;
	while (!history.empty() &&
	       (history.back() == visualizer->preset_target || history.back() == visualizer->preset_cursor))
		history.pop_back();
	return history.empty() ? std::string() : history.back();
}

const char *preset_command_name(preset_command command)
{
	switch (command) {
	case preset_command::next:
		return "next";
	case preset_command::previous:
		return "previous";
	case preset_command::random:
		return "random";
	default:
		return "none";
	}
}

bool take_preset_job(visualizer_source *visualizer, preset_job &job)
{
	std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
	if (visualizer->preset_closing)
		return false;

	const bool pick = visualizer->preset_handled_generation != visualizer->preset_generation;
	if (!pick && !cycle_request_servable(visualizer))
		return false;

	job.folder = visualizer->preset_folder;
	job.file = visualizer->preset_file;
	job.target = visualizer->preset_target;
	job.generation = visualizer->preset_generation;
	job.blend_seconds = !pick                                ? visualizer->cycle_blend
			    : visualizer->preset_target.empty() ? 0.0f
								: static_cast<float>(visualizer->blend_seconds);
	job.next = !pick;
	job.random = visualizer->random_order;
	job.command = pick ? visualizer->pending_command : preset_command::none;
	if (job.command == preset_command::previous)
		job.history_path = previous_from_history(visualizer);
	visualizer->pending_command = preset_command::none;
	visualizer->preset_handled_generation = job.generation;
	return true;
}

bool switch_to_cycle_job(visualizer_source *visualizer, preset_job &job)
{
	std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
	if (visualizer->preset_closing || job.generation != visualizer->preset_generation ||
	    !cycle_request_servable(visualizer))
		return false;

	job.blend_seconds = visualizer->cycle_blend;
	job.next = true;
	job.random = visualizer->random_order;
	return true;
}

std::string sequential_preset_file(const std::vector<std::string> &files, const preset_job &job,
				   const std::string &cursor, bool backward = false)
{
	const auto find_path = [&](const std::string &path) {
		return std::find_if(files.begin(), files.end(),
				    [&](const std::string &file) { return preset_path(job.folder, file) == path; });
	};
	auto position = find_path(cursor);
	if (position == files.end())
		position = find_path(job.target);

	const size_t count = files.size();
	const size_t step = backward ? count - 1 : 1;
	size_t index = position == files.end() ? (backward ? count - 1 : 0)
					       : static_cast<size_t>(position - files.begin()) + step;
	if (count > 1 && preset_path(job.folder, files[index % count]) == job.target)
		index += step;
	return files[index % count];
}

std::string random_preset_file(visualizer_source *visualizer, const std::vector<std::string> &files,
			       const preset_job &job)
{
	std::vector<std::string> candidates;
	for (const std::string &file : files) {
		const std::string path = preset_path(job.folder, file);
		if (path != job.target && path != visualizer->preset_cursor)
			candidates.push_back(file);
	}
	if (candidates.empty()) {
		for (const std::string &file : files) {
			if (preset_path(job.folder, file) != job.target)
				candidates.push_back(file);
		}
	}
	if (candidates.empty())
		return files.front();

	std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
	return candidates[pick(visualizer->preset_random)];
}

std::string next_preset_file(visualizer_source *visualizer, const std::vector<std::string> &files,
			     const preset_job &job)
{
	return job.random ? random_preset_file(visualizer, files, job)
			  : sequential_preset_file(files, job, visualizer->preset_cursor);
}

std::string command_preset_path(visualizer_source *visualizer, const std::vector<std::string> &files,
				const preset_job &job)
{
	switch (job.command) {
	case preset_command::previous:
		return !job.history_path.empty()
			       ? job.history_path
			       : preset_path(job.folder,
					     sequential_preset_file(files, job, visualizer->preset_cursor, true));
	case preset_command::random:
		return preset_path(job.folder, random_preset_file(visualizer, files, job));
	default:
		return preset_path(job.folder, next_preset_file(visualizer, files, job));
	}
}

void prepare_requested_preset(void *data)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	preset_job job;
	if (!take_preset_job(visualizer, job))
		return;

	const std::vector<std::string> files = list_preset_files(job.folder);
	if (files.empty()) {
		if (!job.next)
			log_missing_presets(job.folder);
		return;
	}

	std::string path;
	if (job.command != preset_command::none) {
		path = command_preset_path(visualizer, files, job);
		visualizer->preset_cursor = path;
		const char *origin = job.command != preset_command::previous ? ""
				     : job.history_path.empty()		 ? " (sorted order)"
									 : " (history)";
		obs_log(LOG_INFO, "hotkey picked the %s preset: %s%s", preset_command_name(job.command), path.c_str(),
			origin);
	} else if (!job.next) {
		path = preset_path(job.folder, listed_or_first(files, job.file));
		visualizer->preset_cursor = path;
		if (path == job.target && !switch_to_cycle_job(visualizer, job))
			return;
	}
	if (job.next) {
		path = preset_path(job.folder, next_preset_file(visualizer, files, job));
		visualizer->preset_cursor = path;
		obs_log(LOG_INFO, "auto-cycle picked the %s preset: %s", job.random ? "random" : "next", path.c_str());
	}

	sv_prepared_preset *prepared = nullptr;
	std::array<char, preset_error_length> error = {};
	const uint64_t start = os_gettime_ns();
	const sv_status status =
		sv_prepare_preset(visualizer->context, path.c_str(), &prepared, error.data(), error.size());
	const double milliseconds = milliseconds_since(start);
	if (status != SV_OK) {
		obs_log(LOG_WARNING, "preset failed to prepare: %s (%s)", path.c_str(), error.data());
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		clear_cycle_request(visualizer);
		return;
	}

	sv_prepared_preset *replaced = nullptr;
	bool superseded = false;
	{
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		superseded = visualizer->preset_closing || job.generation != visualizer->preset_generation;
		if (superseded) {
			replaced = prepared;
		} else {
			replaced = visualizer->ready_preset;
			visualizer->ready_preset = prepared;
			visualizer->ready_preset_path = path;
			visualizer->ready_preset_blend = job.blend_seconds;
			visualizer->preset_target = path;
			clear_cycle_request(visualizer);
		}
	}
	sv_prepared_preset_free(replaced);

	if (superseded)
		obs_log(LOG_INFO, "preset prepared but superseded, dropped: %s (%.1f ms)", path.c_str(), milliseconds);
	else
		obs_log(LOG_INFO, "preset prepared: %s (%.1f ms)", path.c_str(), milliseconds);
}

void request_preset(visualizer_source *visualizer, obs_data_t *settings)
{
	const char *folder = obs_data_get_string(settings, setting_preset_folder);
	const char *file = obs_data_get_string(settings, setting_preset);
	{
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		if (visualizer->preset_generation != 0 && visualizer->preset_folder == folder &&
		    visualizer->preset_file == file)
			return;
		if (visualizer->preset_folder != folder)
			visualizer->preset_history.clear();
		visualizer->preset_folder = folder;
		visualizer->preset_file = file;
		visualizer->pending_command = preset_command::none;
		visualizer->preset_generation++;
	}
	os_task_queue_queue_task(visualizer->preset_worker, prepare_requested_preset, visualizer);
}

void preset_requested(sv_context *, const sv_preset_request *request, void *data)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	bool paused = false;
	{
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		if (visualizer->preset_closing)
			return;
		paused = visualizer->cycle_paused;
		visualizer->cycle_requested = !paused;
		visualizer->cycle_deferred = paused;
		visualizer->cycle_blend = request->suggested_blend_seconds;
	}
	const char *reason = request->reason == SV_PRESET_REQUEST_HARD_CUT ? "hard cut" : "timer";
	if (paused) {
		obs_log(LOG_INFO, "auto-cycle request deferred while paused (%s)", reason);
		return;
	}
	obs_log(LOG_INFO, "auto-cycle requested a preset (%s, blend %.1f s)", reason, request->suggested_blend_seconds);
	os_task_queue_queue_task(visualizer->preset_worker, prepare_requested_preset, visualizer);
}

void request_preset_command(visualizer_source *visualizer, preset_command command)
{
	{
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		if (visualizer->preset_closing)
			return;
		visualizer->pending_command = command;
		visualizer->preset_generation++;
	}
	obs_log(LOG_INFO, "hotkey: %s preset", preset_command_name(command));
	os_task_queue_queue_task(visualizer->preset_worker, prepare_requested_preset, visualizer);
}

void toggle_auto_cycle_pause(visualizer_source *visualizer)
{
	bool paused = false;
	bool serve_deferred = false;
	{
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		if (visualizer->preset_closing)
			return;
		visualizer->cycle_paused = !visualizer->cycle_paused;
		paused = visualizer->cycle_paused;
		serve_deferred = !paused && visualizer->cycle_deferred;
		if (serve_deferred) {
			visualizer->cycle_deferred = false;
			visualizer->cycle_requested = true;
		}
	}
	if (paused) {
		obs_log(LOG_INFO, "hotkey: auto-cycle paused");
		return;
	}
	obs_log(LOG_INFO, "hotkey: auto-cycle resumed%s", serve_deferred ? ", serving the deferred request" : "");
	if (serve_deferred)
		os_task_queue_queue_task(visualizer->preset_worker, prepare_requested_preset, visualizer);
}

void next_preset_hotkey(void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (pressed)
		request_preset_command(static_cast<visualizer_source *>(data), preset_command::next);
}

void previous_preset_hotkey(void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (pressed)
		request_preset_command(static_cast<visualizer_source *>(data), preset_command::previous);
}

void random_preset_hotkey(void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (pressed)
		request_preset_command(static_cast<visualizer_source *>(data), preset_command::random);
}

void pause_auto_cycle_hotkey(void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (pressed)
		toggle_auto_cycle_pause(static_cast<visualizer_source *>(data));
}

void register_hotkeys(visualizer_source *visualizer)
{
	obs_source_t *source = visualizer->source;
	visualizer->hotkeys = {
		obs_hotkey_register_source(source, hotkey_next_preset, obs_module_text("Hotkey.NextPreset"),
					   next_preset_hotkey, visualizer),
		obs_hotkey_register_source(source, hotkey_previous_preset, obs_module_text("Hotkey.PreviousPreset"),
					   previous_preset_hotkey, visualizer),
		obs_hotkey_register_source(source, hotkey_random_preset, obs_module_text("Hotkey.RandomPreset"),
					   random_preset_hotkey, visualizer),
		obs_hotkey_register_source(source, hotkey_pause_auto_cycle, obs_module_text("Hotkey.PauseAutoCycle"),
					   pause_auto_cycle_hotkey, visualizer),
	};
}

void unregister_hotkeys(visualizer_source *visualizer)
{
	for (obs_hotkey_id &hotkey : visualizer->hotkeys) {
		obs_hotkey_unregister(hotkey);
		hotkey = OBS_INVALID_HOTKEY_ID;
	}
}

void configure_auto_cycle(visualizer_source *visualizer, obs_data_t *settings)
{
	const long long cycle = std::clamp(obs_data_get_int(settings, setting_cycle_seconds), 0LL, max_cycle_seconds);
	const double blend = std::clamp(obs_data_get_double(settings, setting_blend_seconds), 0.0, max_blend_seconds);
	const bool random = obs_data_get_bool(settings, setting_random_order);
	std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
	if (visualizer->cycle_seconds == cycle && visualizer->blend_seconds == blend && visualizer->random_order == random)
		return;

	visualizer->cycle_seconds = cycle;
	visualizer->blend_seconds = blend;
	visualizer->random_order = random;
	visualizer->transition_dirty = true;
}

void stop_preset_worker(visualizer_source *visualizer)
{
	{
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		visualizer->preset_closing = true;
	}
	if (visualizer->preset_worker) {
		const uint64_t start = os_gettime_ns();
		os_task_queue_destroy(visualizer->preset_worker);
		visualizer->preset_worker = nullptr;
		obs_log(LOG_INFO, "preset worker stopped (%.1f ms)", milliseconds_since(start));
	}
	sv_prepared_preset_free(visualizer->ready_preset);
	visualizer->ready_preset = nullptr;
}

void audio_capture(void *data, obs_source_t *, const struct audio_data *audio_data, bool muted)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	uint8_t *output[MAX_AV_PLANES] = {};
	uint32_t frames = 0;
	uint64_t ts_offset = 0;
	if (!audio_resampler_resample(visualizer->audio_resampler, output, &frames, &ts_offset, audio_data->data,
				      audio_data->frames))
		return;
	if (muted)
		visualizer->audio.write_silence(frames);
	else
		visualizer->audio.write(reinterpret_cast<const float *>(output[0]), frames);
}

void mix_capture(void *data, size_t, struct audio_data *audio_data)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	visualizer->audio.write(reinterpret_cast<const float *>(audio_data->data[0]), audio_data->frames);
}

bool whole_mix_selected(const visualizer_source *visualizer)
{
	return visualizer->audio_source_uuid == audio_source_whole_mix;
}

void connect_audio_mix(visualizer_source *visualizer)
{
	const audio_convert_info conversion = {core_sample_rate, AUDIO_FORMAT_FLOAT, SPEAKERS_STEREO, false};
	visualizer->audio_mix_connected = audio_output_connect(obs_get_audio(), visualizer->audio_mix_index,
							       &conversion, mix_capture, visualizer);
	if (visualizer->audio_mix_connected)
		obs_log(LOG_INFO, "audio mix connected: track %zu (%u Hz stereo)", visualizer->audio_mix_index + 1,
			core_sample_rate);
	else
		obs_log(LOG_WARNING, "audio mix could not be connected: track %zu", visualizer->audio_mix_index + 1);
}

void disconnect_audio_mix(visualizer_source *visualizer)
{
	if (!visualizer->audio_mix_connected)
		return;

	audio_output_disconnect(obs_get_audio(), visualizer->audio_mix_index, mix_capture, visualizer);
	visualizer->audio_mix_connected = false;
	obs_log(LOG_INFO, "audio mix disconnected: track %zu", visualizer->audio_mix_index + 1);
}

void detach_audio_source(visualizer_source *visualizer)
{
	if (!visualizer->audio_weak_source)
		return;

	obs_source_t *source = obs_weak_source_get_source(visualizer->audio_weak_source);
	if (source) {
		obs_source_remove_audio_capture_callback(source, audio_capture, visualizer);
		obs_log(LOG_INFO, "audio source detached: %s", obs_source_get_name(source));
	}
	obs_weak_source_release(visualizer->audio_weak_source);
	visualizer->audio_weak_source = nullptr;
	audio_resampler_destroy(visualizer->audio_resampler);
	visualizer->audio_resampler = nullptr;
	obs_source_release(source);
}

void attach_audio_source(visualizer_source *visualizer, obs_source_t *source)
{
	const audio_output_info *output_info = audio_output_get_info(obs_get_audio());
	if (!output_info)
		return;

	const resample_info from = {output_info->samples_per_sec, output_info->format, output_info->speakers};
	const resample_info to = {core_sample_rate, AUDIO_FORMAT_FLOAT, SPEAKERS_STEREO};
	visualizer->audio_resampler = audio_resampler_create(&to, &from);
	if (!visualizer->audio_resampler) {
		if (!visualizer->audio_wait_logged)
			obs_log(LOG_WARNING, "audio resampler could not be created (%u Hz to %u Hz)",
				output_info->samples_per_sec, core_sample_rate);
		visualizer->audio_wait_logged = true;
		return;
	}

	visualizer->audio_weak_source = obs_source_get_weak_source(source);
	visualizer->audio_wait_logged = false;
	obs_source_add_audio_capture_callback(source, audio_capture, visualizer);
	obs_log(LOG_INFO, "audio source attached: %s (%u Hz to %u Hz stereo)", obs_source_get_name(source),
		output_info->samples_per_sec, core_sample_rate);
}

bool audio_source_available(obs_source_t *source)
{
	return source && !obs_source_removed(source) && (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO);
}

void resolve_audio_source(visualizer_source *visualizer)
{
	obs_source_t *source = obs_get_source_by_uuid(visualizer->audio_source_uuid.c_str());
	if (audio_source_available(source)) {
		attach_audio_source(visualizer, source);
	} else if (!visualizer->audio_wait_logged) {
		obs_log(LOG_INFO, "audio source not found (%s); checking again every second",
			visualizer->audio_source_uuid.c_str());
		visualizer->audio_wait_logged = true;
	}
	obs_source_release(source);
}

void check_attached_audio_source(visualizer_source *visualizer)
{
	obs_source_t *source = obs_weak_source_get_source(visualizer->audio_weak_source);
	if (audio_source_available(source)) {
		obs_source_release(source);
		return;
	}

	obs_log(LOG_INFO, "audio source gone (%s); audio is silent until it returns",
		visualizer->audio_source_uuid.c_str());
	detach_audio_source(visualizer);
	visualizer->audio_wait_logged = true;
	obs_source_release(source);
}

void maintain_audio_source(visualizer_source *visualizer)
{
	std::unique_lock<std::mutex> lock(visualizer->audio_mutex, std::try_to_lock);
	if (!lock.owns_lock() || visualizer->audio_source_uuid.empty() || whole_mix_selected(visualizer))
		return;

	const uint64_t now = os_gettime_ns();
	if (visualizer->audio_check_time != 0 && now - visualizer->audio_check_time < audio_check_interval_ns)
		return;
	visualizer->audio_check_time = now;

	if (visualizer->audio_weak_source)
		check_attached_audio_source(visualizer);
	else
		resolve_audio_source(visualizer);
}

void select_audio_source(visualizer_source *visualizer, const char *uuid, size_t mix_index)
{
	std::lock_guard<std::mutex> lock(visualizer->audio_mutex);
	const bool mix_index_changed = visualizer->audio_mix_index != mix_index;
	if (visualizer->audio_source_uuid == uuid && !(whole_mix_selected(visualizer) && mix_index_changed)) {
		visualizer->audio_mix_index = mix_index;
		return;
	}

	detach_audio_source(visualizer);
	disconnect_audio_mix(visualizer);
	visualizer->audio_source_uuid = uuid;
	visualizer->audio_mix_index = mix_index;
	visualizer->audio_check_time = 0;
	visualizer->audio_wait_logged = false;
	if (whole_mix_selected(visualizer))
		connect_audio_mix(visualizer);
	else if (visualizer->audio_source_uuid.empty())
		obs_log(LOG_INFO, "audio source: none");
}

size_t audio_mix_index_from_settings(obs_data_t *settings)
{
	const long long track = std::clamp(obs_data_get_int(settings, setting_audio_mix_track), 1LL,
					   static_cast<long long>(MAX_AUDIO_MIXES));
	return static_cast<size_t>(track - 1);
}

void push_captured_audio(visualizer_source *visualizer)
{
	const size_t frames = visualizer->audio.read_newest(visualizer->audio_frames.data(), core_audio_frames);
	const uint64_t now = os_gettime_ns();
	if (frames > 0) {
		sv_push_pcm_float(visualizer->context, visualizer->audio_frames.data(), frames,
				  static_cast<int>(audio_ring::channels));
		visualizer->last_audio_time = now;
		visualizer->audio_silenced = false;
	} else if (!visualizer->audio_silenced && now - visualizer->last_audio_time > audio_silence_after_ns) {
		visualizer->audio_frames.fill(0.0f);
		sv_push_pcm_float(visualizer->context, visualizer->audio_frames.data(), core_audio_frames,
				  static_cast<int>(audio_ring::channels));
		visualizer->audio_silenced = true;
	}
}

void remember_committed_preset(visualizer_source *visualizer, const std::string &path)
{
	std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
	std::deque<std::string> &history = visualizer->preset_history;
	if (!history.empty() && history.back() == path)
		return;
	history.push_back(path);
	if (history.size() > max_preset_history)
		history.pop_front();
}

void commit_ready_preset(visualizer_source *visualizer)
{
	sv_prepared_preset *prepared = nullptr;
	std::string path;
	float blend_seconds = 0.0f;
	{
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		prepared = visualizer->ready_preset;
		visualizer->ready_preset = nullptr;
		path.swap(visualizer->ready_preset_path);
		blend_seconds = visualizer->ready_preset_blend;
		if (prepared)
			clear_cycle_request(visualizer);
	}
	if (!prepared)
		return;

	sv_status status = SV_ERR_GL;
	const uint64_t start = os_gettime_ns();
	with_core_gl_state([&] { status = sv_commit_preset(visualizer->context, prepared, blend_seconds); });
	const double milliseconds = milliseconds_since(start);
	sv_prepared_preset_free(prepared);

	if (status == SV_OK) {
		remember_committed_preset(visualizer, path);
		obs_log(LOG_INFO, "preset committed: %s (blend %.1f s, %.1f ms)", path.c_str(), blend_seconds,
			milliseconds);
		return;
	}

	obs_log(LOG_WARNING, "preset failed to commit: %s (%s)", path.c_str(), sv_last_error(visualizer->context));
	std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
	if (visualizer->preset_target == path)
		visualizer->preset_target.clear();
}

void apply_transition_config(visualizer_source *visualizer)
{
	sv_transition_config config = {};
	bool random = false;
	{
		std::lock_guard<std::mutex> lock(visualizer->preset_mutex);
		if (!visualizer->transition_dirty)
			return;
		visualizer->transition_dirty = false;
		config.soft_cut_seconds = static_cast<float>(visualizer->blend_seconds);
		config.preset_duration = static_cast<float>(visualizer->cycle_seconds);
		random = visualizer->random_order;
	}
	config.preset_duration_rand = 0.0f;
	config.hard_cuts_enabled = false;
	with_core_gl_state([&] { sv_set_transition_config(visualizer->context, &config); });

	if (config.preset_duration > 0.0f)
		obs_log(LOG_INFO, "auto-cycle: every %.0f s, blend %.1f s, %s order", config.preset_duration,
			config.soft_cut_seconds, random ? "random" : "sequential");
	else
		obs_log(LOG_INFO, "auto-cycle: off, blend %.1f s", config.soft_cut_seconds);
}

uint32_t scaled_dimension(uint32_t dimension, long long scale)
{
	const double scaled = static_cast<double>(dimension) * static_cast<double>(scale) /
			      static_cast<double>(full_render_scale);
	return std::max<uint32_t>(1, static_cast<uint32_t>(std::lround(scaled)));
}

output_size core_target_size(const visualizer_source *visualizer)
{
	const output_size size = visualizer->size.load();
	const long long scale = visualizer->render_scale.load();
	return {scaled_dimension(size.width, scale), scaled_dimension(size.height, scale)};
}

bool same_size(const output_size &a, const output_size &b)
{
	return a.width == b.width && a.height == b.height;
}

void configure_output_size(visualizer_source *visualizer, obs_data_t *settings)
{
	const auto dimension = [settings](const char *name) {
		return static_cast<uint32_t>(std::clamp(obs_data_get_int(settings, name), min_output_size, max_output_size));
	};
	visualizer->size.store({dimension(setting_width), dimension(setting_height)});
	visualizer->render_scale.store(
		std::clamp(obs_data_get_int(settings, setting_render_scale), half_render_scale, full_render_scale));
}

output_size resize_core(visualizer_source *visualizer)
{
	const output_size target = core_target_size(visualizer);
	if (same_size(target, visualizer->core_size))
		return target;

	const uint64_t start = os_gettime_ns();
	with_core_gl_state([&] {
		sv_resize(visualizer->context, static_cast<int>(target.width), static_cast<int>(target.height));
	});
	const double milliseconds = milliseconds_since(start);
	visualizer->core_size = target;

	const output_size size = visualizer->size.load();
	obs_log(LOG_INFO, "core resized to %ux%u (source %ux%u, render scale %lld %%, %.1f ms)", target.width,
		target.height, size.width, size.height, visualizer->render_scale.load(), milliseconds);
	return target;
}

const char *visualizer_get_name(void *)
{
	return obs_module_text("SynesthesiaVibeVisualizer");
}

bool add_audio_source_entry(void *data, obs_source_t *source)
{
	if (audio_source_available(source))
		obs_property_list_add_string(static_cast<obs_property_t *>(data), obs_source_get_name(source),
					     obs_source_get_uuid(source));
	return true;
}

bool audio_source_listed(obs_property_t *list, const std::string &uuid)
{
	const size_t count = obs_property_list_item_count(list);
	for (size_t i = 0; i < count; i++) {
		if (uuid == obs_property_list_item_string(list, i))
			return true;
	}
	return false;
}

bool audio_source_modified(obs_properties_t *properties, obs_property_t *, obs_data_t *settings)
{
	const bool whole_mix = strcmp(obs_data_get_string(settings, setting_audio_source), audio_source_whole_mix) == 0;
	obs_property_set_visible(obs_properties_get(properties, setting_audio_mix_track), whole_mix);
	return true;
}

bool preset_folder_modified(obs_properties_t *properties, obs_property_t *, obs_data_t *settings)
{
	obs_property_t *list = obs_properties_get(properties, setting_preset);
	obs_property_list_clear(list);
	const std::vector<std::string> files = list_preset_files(obs_data_get_string(settings, setting_preset_folder));
	if (!files.empty())
		obs_property_list_add_string(list, obs_module_text("Preset.First"), "");
	for (const std::string &file : files)
		obs_property_list_add_string(list, preset_label(file).c_str(), file.c_str());

	const bool empty = files.empty();
	obs_property_set_visible(list, !empty);
	obs_property_set_visible(obs_properties_get(properties, setting_preset_hint), empty);
	return true;
}

obs_properties_t *visualizer_get_properties(void *data)
{
	obs_properties_t *properties = obs_properties_create();

	obs_properties_t *presets = obs_properties_create();
	obs_property_t *folder = obs_properties_add_path(presets, setting_preset_folder,
							 obs_module_text("PresetFolder"), OBS_PATH_DIRECTORY, nullptr,
							 nullptr);
	obs_property_set_modified_callback(folder, preset_folder_modified);
	obs_properties_add_list(presets, setting_preset, obs_module_text("Preset"), OBS_COMBO_TYPE_LIST,
				OBS_COMBO_FORMAT_STRING);
	obs_property_t *hint = obs_properties_add_text(presets, setting_preset_hint,
						       obs_module_text("PresetFolder.Hint"), OBS_TEXT_INFO);
	obs_property_text_set_info_type(hint, OBS_TEXT_INFO_WARNING);
	obs_properties_add_group(properties, group_presets, obs_module_text("Group.Presets"), OBS_GROUP_NORMAL,
				 presets);

	obs_properties_t *transitions = obs_properties_create();
	obs_property_t *cycle = obs_properties_add_int(transitions, setting_cycle_seconds,
						       obs_module_text("CycleSeconds"), 0,
						       static_cast<int>(max_cycle_seconds), 1);
	obs_property_int_set_suffix(cycle, obs_module_text("Unit.Seconds"));
	obs_property_t *blend = obs_properties_add_float(transitions, setting_blend_seconds,
							 obs_module_text("BlendSeconds"), 0.0, max_blend_seconds, 0.1);
	obs_property_float_set_suffix(blend, obs_module_text("Unit.Seconds"));
	obs_properties_add_bool(transitions, setting_random_order, obs_module_text("RandomOrder"));
	obs_properties_add_group(properties, group_transitions, obs_module_text("Group.Transitions"), OBS_GROUP_NORMAL,
				 transitions);

	obs_properties_t *audio = obs_properties_create();
	obs_property_t *audio_list = obs_properties_add_list(audio, setting_audio_source,
							     obs_module_text("AudioSource"), OBS_COMBO_TYPE_LIST,
							     OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(audio_list, obs_module_text("AudioSource.None"), "");
	obs_property_list_add_string(audio_list, obs_module_text("AudioSource.WholeMix"), audio_source_whole_mix);
	obs_enum_sources(add_audio_source_entry, audio_list);
	obs_property_set_modified_callback(audio_list, audio_source_modified);
	obs_property_t *mix_track = obs_properties_add_list(audio, setting_audio_mix_track, obs_module_text("MixTrack"),
							    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	for (size_t i = 0; i < mix_track_labels.size(); i++)
		obs_property_list_add_int(mix_track, obs_module_text(mix_track_labels[i]),
					  static_cast<long long>(i + 1));
	obs_properties_add_group(properties, group_audio, obs_module_text("Group.Audio"), OBS_GROUP_NORMAL, audio);

	obs_properties_t *output = obs_properties_create();
	obs_property_t *width = obs_properties_add_int(output, setting_width, obs_module_text("Width"),
						       static_cast<int>(min_output_size),
						       static_cast<int>(max_output_size), 1);
	obs_property_int_set_suffix(width, obs_module_text("Unit.Pixels"));
	obs_property_t *height = obs_properties_add_int(output, setting_height, obs_module_text("Height"),
							static_cast<int>(min_output_size),
							static_cast<int>(max_output_size), 1);
	obs_property_int_set_suffix(height, obs_module_text("Unit.Pixels"));
	obs_property_t *scale = obs_properties_add_list(output, setting_render_scale, obs_module_text("RenderScale"),
							OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(scale, obs_module_text("RenderScale.Full"), full_render_scale);
	obs_property_list_add_int(scale, obs_module_text("RenderScale.ThreeQuarters"), three_quarter_render_scale);
	obs_property_list_add_int(scale, obs_module_text("RenderScale.Half"), half_render_scale);
	obs_properties_add_group(properties, group_output, obs_module_text("Group.Output"), OBS_GROUP_NORMAL, output);

	if (data) {
		visualizer_source *visualizer = static_cast<visualizer_source *>(data);
		std::string uuid;
		{
			std::lock_guard<std::mutex> lock(visualizer->audio_mutex);
			uuid = visualizer->audio_source_uuid;
		}
		if (!uuid.empty() && !audio_source_listed(audio_list, uuid))
			obs_property_list_insert_string(audio_list, 1, obs_module_text("AudioSource.Missing"),
							uuid.c_str());
	}
	return properties;
}

void visualizer_update(void *data, obs_data_t *settings)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	select_audio_source(visualizer, obs_data_get_string(settings, setting_audio_source),
			    audio_mix_index_from_settings(settings));
	configure_auto_cycle(visualizer, settings);
	request_preset(visualizer, settings);
	configure_output_size(visualizer, settings);
}

void visualizer_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, setting_preset_folder, "");
	obs_data_set_default_string(settings, setting_preset, "");
	obs_data_set_default_int(settings, setting_cycle_seconds, default_cycle_seconds);
	obs_data_set_default_double(settings, setting_blend_seconds, default_blend_seconds);
	obs_data_set_default_bool(settings, setting_random_order, false);
	obs_data_set_default_string(settings, setting_audio_source, "");
	obs_data_set_default_int(settings, setting_audio_mix_track, default_audio_mix_track);
	obs_data_set_default_int(settings, setting_width, default_width);
	obs_data_set_default_int(settings, setting_height, default_height);
	obs_data_set_default_int(settings, setting_render_scale, full_render_scale);
}

void *visualizer_create(obs_data_t *settings, obs_source_t *source)
{
	visualizer_source *visualizer = new visualizer_source{source};

	const char *data_path = obs_get_module_data_path(obs_current_module());
	const std::string resource_path = data_path ? data_path : "";
	char *cache_path = obs_module_config_path("cache");
	if (cache_path)
		os_mkdirs(cache_path);

	configure_output_size(visualizer, settings);
	const output_size core_size = core_target_size(visualizer);

	sv_config config = {};
	config.width = static_cast<int>(core_size.width);
	config.height = static_cast<int>(core_size.height);
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

	if (visualizer->context) {
		visualizer->preset_worker = os_task_queue_create();
		visualizer->core_size = core_size;
		sv_set_preset_request_callback(visualizer->context, preset_requested, visualizer);
		obs_log(LOG_INFO, "sv_init_gl and sv_create succeeded (%ux%u)", core_size.width, core_size.height);
	} else {
		obs_log(LOG_WARNING, "core setup failed (sv_init_gl status %d, %s)", gl_status,
			sv_last_error(nullptr));
	}

	visualizer_update(visualizer, settings);
	register_hotkeys(visualizer);
	return visualizer;
}

void visualizer_destroy(void *data)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);

	unregister_hotkeys(visualizer);

	{
		std::lock_guard<std::mutex> lock(visualizer->audio_mutex);
		detach_audio_source(visualizer);
		disconnect_audio_mix(visualizer);
	}

	stop_preset_worker(visualizer);

	obs_enter_graphics();
	if (visualizer->context)
		with_core_gl_state([&] { sv_destroy(visualizer->context); });
	gs_texrender_destroy(visualizer->texrender);
	obs_leave_graphics();

	delete visualizer;
}

uint32_t visualizer_get_width(void *data)
{
	return static_cast<visualizer_source *>(data)->size.load().width;
}

uint32_t visualizer_get_height(void *data)
{
	return static_cast<visualizer_source *>(data)->size.load().height;
}

void visualizer_video_tick(void *data, float seconds)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	if (seconds > 0.0f)
		visualizer->pending_nanoseconds.fetch_add(
			static_cast<uint64_t>(static_cast<double>(seconds) * nanoseconds_per_second));
	visualizer->frame_pending.store(true);
	maintain_audio_source(visualizer);
}

void visualizer_reset_pending_delta(void *data)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	visualizer->pending_nanoseconds.store(0);
}

double take_pending_seconds(visualizer_source *visualizer)
{
	const uint64_t nanoseconds = visualizer->pending_nanoseconds.exchange(0);
	return std::min(static_cast<double>(nanoseconds) / nanoseconds_per_second, max_step_seconds);
}

void render_core_frame(visualizer_source *visualizer, double seconds)
{
	apply_transition_config(visualizer);
	commit_ready_preset(visualizer);

	const output_size core_size = resize_core(visualizer);
	gs_texrender_reset(visualizer->texrender);
	if (!gs_texrender_begin(visualizer->texrender, core_size.width, core_size.height))
		return;

	const bool previous_srgb = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(false);

	GLint target_fbo = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &target_fbo);
	push_captured_audio(visualizer);
	with_core_gl_state([&] {
		sv_render_frame(visualizer->context, seconds, static_cast<uint32_t>(target_fbo));
	});

	gs_enable_framebuffer_srgb(previous_srgb);
	gs_texrender_end(visualizer->texrender);
}

void visualizer_video_render(void *data, gs_effect_t *)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(data);
	if (!visualizer->context || !visualizer->texrender)
		return;

	if (visualizer->frame_pending.exchange(false))
		render_core_frame(visualizer, take_pending_seconds(visualizer));

	gs_texture_t *texture = gs_texrender_get_texture(visualizer->texrender);
	if (!texture)
		return;

	const output_size size = visualizer->size.load();

	const bool linear_srgb = gs_get_linear_srgb();
	const bool previous_srgb = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(linear_srgb);
	gs_blend_state_push();
	gs_enable_blending(false);

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_OPAQUE);
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	if (linear_srgb)
		gs_effect_set_texture_srgb(image, texture);
	else
		gs_effect_set_texture(image, texture);

	while (gs_effect_loop(effect, "Draw"))
		gs_draw_sprite(texture, GS_FLIP_V, size.width, size.height);

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
	info.get_defaults = visualizer_get_defaults;
	info.get_properties = visualizer_get_properties;
	info.get_width = visualizer_get_width;
	info.get_height = visualizer_get_height;
	info.video_tick = visualizer_video_tick;
	info.video_render = visualizer_video_render;
	info.show = visualizer_reset_pending_delta;
	info.activate = visualizer_reset_pending_delta;
	obs_register_source(&info);
}
