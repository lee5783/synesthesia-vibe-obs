import sys
p = sys.argv[1]
s = open(p).read()

def rep(a, b):
    global s
    assert s.count(a) == 1, a
    s = s.replace(a, b)

rep('#include <array>\n', '#include <array>\n#include <atomic>\n#include <pthread.h>\n')
rep('template<typename Call> void with_core_gl_state(Call call)\n{\n\tgl_state_guard guard;\n\tcall();\n}\n',
'''std::atomic<int> detector_in_flight{0};
std::atomic<uint64_t> detector_calls{0};
std::atomic<uint64_t> detector_overlaps{0};

struct detector_scope {
	explicit detector_scope(const char *where)
	{
		detector_calls.fetch_add(1);
		const int before = detector_in_flight.fetch_add(1);
		if (before != 0) {
			const uint64_t n = detector_overlaps.fetch_add(1) + 1;
			if (n <= 20)
				obs_log(LOG_WARNING, "[re-entrancy detector] overlap #%llu in %s (thread %p, %d already in flight)",
					(unsigned long long)n, where, (void *)pthread_self(), before);
		}
	}
	~detector_scope() { detector_in_flight.fetch_sub(1); }
};

void detector_report(const char *when)
{
	obs_log(LOG_INFO, "[re-entrancy detector] %s: %llu core calls, %llu overlaps", when,
		(unsigned long long)detector_calls.load(), (unsigned long long)detector_overlaps.load());
}

template<typename Call> void with_core_gl_state(Call call)
{
	detector_scope scope("with_core_gl_state");
	gl_state_guard guard;
	call();
}
''')
rep('''	if (!visualizer->context || frames == 0)
		return;
''', '''	if (!visualizer->context || frames == 0)
		return;
	detector_scope scope("push_synthetic_tone");
''')
rep('''	obs_enter_graphics();
	if (visualizer->context)
		with_core_gl_state''', '''	detector_report("destroy");
	obs_enter_graphics();
	if (visualizer->context)
		with_core_gl_state''')
open(p, 'w').write(s)
