import sys
p = sys.argv[1]
s = open(p).read()

def rep(a, b):
    global s
    assert s.count(a) == 1, a
    s = s.replace(a, b)

rep('\tbool audio_silenced = false;\n};\n',
    '\tbool audio_silenced = false;\n\tuint64_t debug_pushed_frames = 0;\n\tfloat debug_peak = 0.0f;\n};\n')
rep('\tconst size_t frames = visualizer->audio.read_newest(visualizer->audio_frames.data(), core_audio_frames);\n',
    '''\tconst size_t frames = visualizer->audio.read_newest(visualizer->audio_frames.data(), core_audio_frames);
	visualizer->debug_pushed_frames += frames;
	for (size_t i = 0; i < frames * audio_ring::channels; i++)
		visualizer->debug_peak = std::max(visualizer->debug_peak, std::abs(visualizer->audio_frames[i]));
''')
rep('\t\tvisualizer->audio_silenced = true;\n',
    '\t\tvisualizer->audio_silenced = true;\n\t\tobs_log(LOG_INFO, "[audio debug] no audio for 0.5 s: pushed %zu frames of silence", core_audio_frames);\n')
rep('void load_pending_preset(', '''void audio_debug_report(const sv_debug_info *info, void *user)
{
	visualizer_source *visualizer = static_cast<visualizer_source *>(user);
	obs_log(LOG_INFO, "[audio debug] bass %.3f mid %.3f treb %.3f | pushed %llu frames, peak %.3f, fps %.1f",
		info->bass, info->mid, info->treb, (unsigned long long)visualizer->debug_pushed_frames,
		visualizer->debug_peak, info->display_fps);
	visualizer->debug_pushed_frames = 0;
	visualizer->debug_peak = 0.0f;
}

void load_pending_preset(''')
rep('''		if (gl_status == SV_OK)
			visualizer->context = sv_create(&config);
''', '''		if (gl_status == SV_OK)
			visualizer->context = sv_create(&config);
		if (visualizer->context)
			sv_set_debug_callback(visualizer->context, audio_debug_report, visualizer);
''')
rep('#include <atomic>\n', '#include <atomic>\n#include <cmath>\n')
open(p, 'w').write(s)
