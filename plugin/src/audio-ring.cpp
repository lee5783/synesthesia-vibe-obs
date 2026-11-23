//
//  audio-ring.cpp
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on 2026-11-23.
//

#include "audio-ring.hpp"

#include <algorithm>
#include <cstring>

namespace {

size_t next_power_of_two(size_t value)
{
	size_t result = 1;
	while (result < value)
		result <<= 1;
	return result;
}

}

audio_ring::audio_ring(size_t capacity_frames)
	: capacity(next_power_of_two(capacity_frames)),
	  mask(capacity - 1),
	  samples(capacity * channels, 0.0f)
{
}

size_t audio_ring::write(const float *interleaved, size_t frames)
{
	return interleaved ? store(interleaved, frames) : 0;
}

size_t audio_ring::write_silence(size_t frames)
{
	return store(nullptr, frames);
}

size_t audio_ring::store(const float *interleaved, size_t frames)
{
	const size_t write_at = write_position.load(std::memory_order_relaxed);
	const size_t read_at = read_position.load(std::memory_order_acquire);
	const size_t count = std::min(frames, capacity - (write_at - read_at));

	size_t done = 0;
	while (done < count) {
		const size_t offset = (write_at + done) & mask;
		const size_t chunk = std::min(count - done, capacity - offset);
		float *target = samples.data() + offset * channels;
		if (interleaved)
			std::memcpy(target, interleaved + done * channels, chunk * channels * sizeof(float));
		else
			std::fill(target, target + chunk * channels, 0.0f);
		done += chunk;
	}

	write_position.store(write_at + count, std::memory_order_release);
	return count;
}

size_t audio_ring::read_newest(float *interleaved, size_t max_frames)
{
	const size_t read_at = read_position.load(std::memory_order_relaxed);
	const size_t write_at = write_position.load(std::memory_order_acquire);
	const size_t count = std::min(write_at - read_at, max_frames);
	const size_t first = write_at - count;

	size_t done = 0;
	while (done < count) {
		const size_t offset = (first + done) & mask;
		const size_t chunk = std::min(count - done, capacity - offset);
		std::memcpy(interleaved + done * channels, samples.data() + offset * channels,
			    chunk * channels * sizeof(float));
		done += chunk;
	}

	read_position.store(write_at, std::memory_order_release);
	return count;
}
