//
//  audio-ring.hpp
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on 2026-11-23.
//

#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

class audio_ring {
public:
	static constexpr size_t channels = 2;

	explicit audio_ring(size_t capacity_frames);

	size_t write(const float *interleaved, size_t frames);
	size_t write_silence(size_t frames);
	size_t read_newest(float *interleaved, size_t max_frames);

	audio_ring(const audio_ring &) = delete;
	audio_ring &operator=(const audio_ring &) = delete;

private:
	size_t store(const float *interleaved, size_t frames);

	const size_t capacity;
	const size_t mask;
	std::vector<float> samples;
	std::atomic<size_t> write_position{0};
	std::atomic<size_t> read_position{0};
};
