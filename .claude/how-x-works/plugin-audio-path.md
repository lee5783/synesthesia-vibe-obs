# Plugin audio path — from an OBS audio source or the mix to the core

> Part of the how-x-works corpus — see [`../HOW-X-WORKS.md`](../HOW-X-WORKS.md) for the index.
> Where it ends: [`plugin-render-frame.md`](plugin-render-frame.md), step 3.7. Thread table:
> `docs/architecture.md`. Limits (shared PCM buffer, stale PCM): `docs/concerns.md`.

How audio reaches `sv_push_pcm_float` without any `sv_*` call off the render thread. Citations are
`plugin/src/visualizer-source.cpp` unless noted.

## 0. The pieces

- **Setting** `audio_source`: a source UUID, `""` (None) or `whole_mix`; `audio_mix_track` 1–6
  (`audio_mix_index_from_settings`, `:820-825`, clamps and converts to a 0-based index).
- **Ring** `audio_ring` (`plugin/src/audio-ring.hpp`, `plugin/src/audio-ring.cpp`): 65,536
  interleaved stereo float frames (`audio_ring_frames`), one producer, one consumer.
- **`audio_mutex`**: serialises every attach, detach, connect, disconnect and resampler
  create/destroy, so at most one producer is registered.
- **Core format**: 44,100 Hz (`core_sample_rate`) interleaved float stereo; the core keeps 2304
  frames (`core_audio_frames`).

## 1. Choosing a producer

`visualizer_update` → `select_audio_source` (`:799-818`), under `audio_mutex`:

1. If the UUID is unchanged and this is not a track change in mix mode: store the index, return
   (re-applying identical settings logs nothing).
2. Otherwise remove **both** producers: `detach_audio_source` (`:708-723`), then
   `disconnect_audio_mix` (`:698-706`). The log always shows the removal before the next attach.
3. Store the new UUID and index, reset the 1 s check timer and the "logged" flag.
4. `whole_mix` → `connect_audio_mix` now. `""` → log `audio source: none`. A UUID is resolved
   lazily by the tick (step 2).

`update` runs on the graphics thread for settings changes (libobs defers it into the tick) and
directly inside `create` on a collection load.

## 2. Chosen source: lazy resolve, attach, re-check

`maintain_audio_source` (`:782-797`), called from every `video_tick`:

- `std::try_to_lock` on `audio_mutex`; returns if busy, if no UUID is set, or in mix mode.
- At most once per `audio_check_interval_ns` (1 s):
  - not attached → `resolve_audio_source` (`:754-765`): `obs_get_source_by_uuid`; if the source
    exists, is not removed and has `OBS_SOURCE_AUDIO`, attach; else log `audio source not found
    (…); checking again every second` once. This covers a collection where the visualizer loads
    before its audio source: "not found", then "attached" about 1 s later.
  - attached → `check_attached_audio_source` (`:767-780`): if the weak reference no longer gives an
    available source, log `audio source gone (…)` and detach.

`attach_audio_source` (`:725-747`): read OBS's output format (`audio_output_get_info`), create an
`audio_resampler` from it to 44.1 kHz float stereo (warn once and stay silent if that fails), keep
a weak reference, then `obs_source_add_audio_capture_callback(source, audio_capture, …)`. The
resampler exists before the callback is registered.

`detach_audio_source` (`:708-723`): if the weak reference still resolves,
`obs_source_remove_audio_capture_callback` (takes libobs's `audio_cb_mutex`, so the callback is not
running once it returns); release the weak reference; destroy the resampler.

## 3. Producer on the source's audio thread

`audio_capture` (`:660-673`) runs under libobs's `audio_cb_mutex`:

- `audio_resampler_resample` the incoming planar data; with `AUDIO_FORMAT_FLOAT` stereo,
  `output[0]` is one interleaved plane.
- muted → `audio.write_silence(frames)`; else `audio.write(output[0], frames)`.

It never calls `sv_*` and takes no plugin mutex.

## 4. Producer on the audio output thread (whole mix)

- `connect_audio_mix` (`:686-696`): `audio_output_connect(obs_get_audio(), mix_index,
  {44100, AUDIO_FORMAT_FLOAT, SPEAKERS_STEREO}, mix_capture, …)`. libobs does the conversion.
  Logs `audio mix connected: track N (44100 Hz stereo)` or a warning (not retried until the
  settings change).
- `mix_capture` (`:675-679`) runs under libobs's `input_mutex` and only calls `audio.write`. It fires
  about every 21 ms even when nothing is routed to the track, so the core gets real zeros.
- `disconnect_audio_mix` (`:698-706`): `audio_output_disconnect` on `obs_get_audio()` at
  disconnect time (NULL-safe after OBS's `stop_audio` at shutdown); it takes `input_mutex`, so the
  callback is not running once it returns.
- The mix is the stream mix of that track: muted and "Monitor only" sources are absent.

## 5. The ring

`plugin/src/audio-ring.cpp`:

- `store` (`:42-62`, producer): load own `write_position` relaxed and `read_position` with acquire;
  write at most `capacity - used` frames (new frames are dropped when full) in up to two masked
  chunks (copy, or zero-fill for silence); then `write_position` with release.
- `read_newest` (`:64-82`, consumer): load own `read_position` relaxed and `write_position` with
  acquire; copy the newest `min(available, max_frames)` frames (`first = write_at - count`); set
  `read_position = write_at` with release, discarding anything older.
- Every `video_render` holds the graphics lock, so the consumer side is serialised even when a
  websocket screenshot renders on another thread.

## 6. Consumer in the render

`push_captured_audio` (`:827-842`), called from `render_core_frame` right before
`sv_render_frame`:

- `read_newest(audio_frames, 2304)`; if any frames came back, `sv_push_pcm_float(ctx, frames, n,
  2)` and remember the time.
- If none came back for more than 0.5 s (`audio_silence_after_ns`) and silence was not pushed yet,
  push one block of 2304 zeros. The core never expires stale PCM through its public API, so
  without this the levels freeze at their last values after None, a lost source, or a paused /
  stopped source.
- A Media Source delivers about 12 chunks a second, so most renders find nothing new; that is
  fine, the core keeps its last 2304 frames.

## 7. Teardown

`visualizer_destroy` (`:1153-1174`) detaches and disconnects under `audio_mutex` before stopping the
worker and destroying the context. The ring is freed with the `visualizer_source`. Edge case
shared with libobs's own sidechain: if the source's last reference drops on another thread while
its callback runs, the weak reference fails and the remove is skipped (`docs/concerns.md`, N1).

## Edge cases

- All visualizers in one OBS share the core's process-global PCM buffer (C-006): an instance on
  None still reacts while another feeds the core.
- After an off-air gap the first render pushes up to ~1.5 s-old frames for one frame.
- A source deleted and re-created has a new UUID; the setting shows "(missing)"
  (`visualizer_get_properties`, `:1076-1078`).

**Analysis date:** 2026-11-23 (core `v0.7.0`, libobs 31.1.1, OBS 33.0.3).
