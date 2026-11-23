//
//  obs-page.js
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on 2026-11-23.
//

const DIST_ENTRY = "./dist/synesthesia-vibe.js";
const MANIFEST_URL = "./dist/resources/manifest.json";
const CANVAS_ID = "visualizer";
const LOG_PREFIX = "obs-page:";
const DEFAULT_CYCLE_SECONDS = 30;
const DEFAULT_BLEND_SECONDS = 2;
const MAX_BLEND_SECONDS = 10;
const DEFAULT_FPS = 60;
const DEFAULT_AUDIO = "default";
const AUDIO_KEYWORDS = ["default", "none"];
const AUDIO_OFF = "none";
const AUDIO_INPUT_KIND = "audioinput";
const VIRTUAL_DEVICE_IDS = ["default", "communications"];
const AUDIO_WATCH_MS = 1000;
const GESTURE_EVENTS = ["pointerdown", "keydown"];

function warn(message) {
    console.warn(`${LOG_PREFIX} ${message}`);
}

function reportError(error) {
    console.error(`${LOG_PREFIX} ${error.message}`, error.cause ?? error);
}

function readNumber(params, name, fallback) {
    const raw = params.get(name);

    if (raw === null)
        return fallback;

    const value = raw.trim() === "" ? NaN : Number(raw);

    if (Number.isFinite(value))
        return value;

    warn(`${name}=${raw} is not a number, using ${fallback}`);
    return fallback;
}

function readNonNegative(params, name, fallback) {
    const value = readNumber(params, name, fallback);

    if (value >= 0)
        return value;

    warn(`${name}=${value} is negative, using ${fallback}`);
    return fallback;
}

function readClamped(params, name, fallback, max) {
    const value = readNumber(params, name, fallback);
    const clamped = Math.min(Math.max(value, 0), max);

    if (clamped !== value)
        warn(`${name}=${value} is outside 0-${max}, using ${clamped}`);

    return clamped;
}

function readAudio(params) {
    const raw = params.get("audio")?.trim() ?? "";

    if (raw === "")
        return DEFAULT_AUDIO;

    return AUDIO_KEYWORDS.includes(raw.toLowerCase()) ? raw.toLowerCase() : raw;
}

function readSettings(search) {
    const params = new URLSearchParams(search);

    return {
        preset: params.get("preset"),
        cycle: readNonNegative(params, "cycle", DEFAULT_CYCLE_SECONDS),
        blend: readClamped(params, "blend", DEFAULT_BLEND_SECONDS, MAX_BLEND_SECONDS),
        fps: readNonNegative(params, "fps", DEFAULT_FPS),
        audio: readAudio(params)
    };
}

async function stage(description, action) {
    try {
        return await action();
    } catch (error) {
        throw new Error(description, { cause: error });
    }
}

function importDist() {
    return stage(`could not load ${DIST_ENTRY}, run browser-source/fetch-dist.sh`, () => import(DIST_ENTRY));
}

async function loadPresets(sv, module) {
    const resources = await stage(`could not load ${MANIFEST_URL}`, () => new sv.ResourceLoader(module).load(MANIFEST_URL));

    if (resources.presets.length === 0)
        throw new Error(`${MANIFEST_URL} lists no presets`);

    return resources.presets;
}

function createView(sv, module, canvas, fps) {
    return stage("could not start the renderer", () => sv.SynesthesiaCanvas.create(canvas, module, {
        configuration: { maxFps: fps },
        onError: error => console.error(`${LOG_PREFIX} renderer error`, error)
    }));
}

function presetName(path) {
    return path.slice(path.lastIndexOf("/") + 1).replace(/\.milk$/, "");
}

function matchKey(text) {
    return text.toLowerCase().replace(/\+/g, " ").replace(/\s+/g, " ").trim();
}

function findMatch(candidates, keys, item, kind, nameOf) {
    const key = matchKey(item);
    const exact = keys.indexOf(key);

    if (exact !== -1)
        return candidates[exact];

    const matches = candidates.filter((candidate, index) => keys[index].includes(key));

    if (matches.length > 1)
        warn(`${kind} "${item}" matches ${matches.length} ${kind}s, using ${nameOf(matches[0])}`);

    return matches[0] ?? null;
}

function resolvePresets(presets, query) {
    if (query === null)
        return presets;

    const keys = presets.map(path => matchKey(presetName(path)));
    const items = query.split(",").map(item => item.trim()).filter(item => item !== "");
    const resolved = [];

    for (const item of items) {
        const path = findMatch(presets, keys, item, "preset", presetName);

        if (path === null)
            warn(`no preset matches "${item}", skipping it`);
        else
            resolved.push(path);
    }

    if (resolved.length === 0) {
        warn(`preset=${query} matched nothing, using all presets`);
        return presets;
    }

    return resolved;
}

async function loadPreset(view, path, blendSeconds) {
    const result = await stage(`could not load preset ${presetName(path)}`, () => view.loadPreset(path, blendSeconds));

    if (result !== null)
        console.info(`${LOG_PREFIX} preset ${presetName(path)}`);

    return result;
}

function createPlaylist(view, presets) {
    let index = -1;

    return async function loadNext(blendSeconds) {
        for (let attempt = 0; attempt < presets.length; attempt++) {
            index = (index + 1) % presets.length;

            try {
                return await loadPreset(view, presets[index], blendSeconds);
            } catch (error) {
                reportError(error);
            }
        }

        throw new Error("no preset in the list could be loaded");
    };
}

function startCycling(sv, view, loadNext, cycleSeconds, blendSeconds) {
    view.setTransitionConfig({
        softCutSeconds: blendSeconds,
        presetDuration: cycleSeconds,
        presetDurationRand: 0,
        hardCutsEnabled: false
    });
    view.setBlendPattern(sv.SynesthesiaContext.BLEND_PATTERN_RANDOM);
    view.onPresetRequest = request => {
        console.info(`${LOG_PREFIX} preset request (${request.reason})`);
        loadNext(request.reason === "hardCut" ? 0 : request.suggestedBlendSeconds).catch(reportError);
    };
}

function logSettings(settings, presets, cycleSeconds) {
    const names = presets.map(presetName).join(" | ");
    const cycle = cycleSeconds === 0 ? "off" : `${cycleSeconds}s`;
    const fps = settings.fps === 0 ? "uncapped" : settings.fps;

    console.info(`${LOG_PREFIX} settings cycle=${cycle} blend=${settings.blend}s fps=${fps} audio=${settings.audio} presets=${names}`);
}

async function unlockDeviceLabels() {
    try {
        const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
        stream.getTracks().forEach(track => track.stop());
    } catch (error) {
        throw new Error(`audio unavailable (${error.name}: ${error.message}), running without audio`, { cause: error });
    }
}

async function listAudioInputs() {
    const devices = await navigator.mediaDevices.enumerateDevices();

    return devices.filter(device => device.kind === AUDIO_INPUT_KIND);
}

function chooseDevice(inputs, audio) {
    const devices = inputs.filter(device => !VIRTUAL_DEVICE_IDS.includes(device.deviceId));
    const labels = devices.map(device => device.label).join(" | ");
    const fallback = {
        label: inputs.find(device => device.deviceId === DEFAULT_AUDIO)?.label ?? devices[0]?.label ?? "the default input",
        constraints: {}
    };

    console.info(`${LOG_PREFIX} audio inputs ${labels}`);

    if (audio === DEFAULT_AUDIO)
        return fallback;

    const keys = devices.map(device => matchKey(device.label));
    const device = findMatch(devices, keys, audio, "audio input", match => match.label);

    if (device !== null)
        return { label: device.label, constraints: { deviceId: { exact: device.deviceId } } };

    warn(`no audio input matches "${audio}", using ${fallback.label}; available: ${labels}`);
    return fallback;
}

function startsSuspended() {
    const probe = new AudioContext();
    const suspended = probe.state === "suspended";

    probe.close();
    return suspended;
}

function nextGesture() {
    return new Promise(resolve => {
        const listeners = new AbortController();
        const done = () => {
            listeners.abort();
            resolve();
        };

        for (const type of GESTURE_EVENTS)
            window.addEventListener(type, done, { capture: true, signal: listeners.signal });
    });
}

function watchAudioInput(input, label) {
    const timer = setInterval(() => {
        if (input.isRunning)
            return;

        clearInterval(timer);
        warn(`audio input ${label} stopped, running without audio; reload the page to retry`);
    }, AUDIO_WATCH_MS);
}

async function startAudio(sv, view, audio) {
    if (audio === AUDIO_OFF) {
        console.info(`${LOG_PREFIX} audio off (audio=none)`);
        return;
    }

    if (!sv.AudioInput.isMicrophoneSupported()) {
        warn("audio capture needs getUserMedia, an AudioWorklet and a secure context (https or localhost), running without audio");
        return;
    }

    await unlockDeviceLabels();
    const device = chooseDevice(await listAudioInputs(), audio);

    if (startsSuspended()) {
        console.info(`${LOG_PREFIX} the AudioContext is suspended by the autoplay policy, audio starts on the first click or keypress`);
        await nextGesture();
    }

    const input = await stage(`could not start audio from ${device.label}, running without audio`, () => sv.AudioInput.fromMicrophone(view, device.constraints));

    console.info(`${LOG_PREFIX} audio ${device.label} at ${input.inputSampleRate} Hz`);
    watchAudioInput(input, device.label);
}

async function main() {
    const settings = readSettings(location.search);
    const canvas = document.getElementById(CANVAS_ID);
    const sv = await importDist();
    const module = await stage("could not load the synesthesia module", () => sv.loadSynesthesiaModule());
    const presets = resolvePresets(await loadPresets(sv, module), settings.preset);
    const cycleSeconds = presets.length > 1 ? settings.cycle : 0;
    const view = await createView(sv, module, canvas, settings.fps);
    const loadNext = createPlaylist(view, presets);

    logSettings(settings, presets, cycleSeconds);
    startCycling(sv, view, loadNext, cycleSeconds, settings.blend);
    await loadNext(0);
    await startAudio(sv, view, settings.audio);
}

main().catch(reportError);
