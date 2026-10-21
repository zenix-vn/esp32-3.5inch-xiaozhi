"""Synthesises the sound files of the device (WAV, 8-bit unsigned mono, 16 kHz) into tools/sounds/ (git-ignored).

    python tools/gen_sounds.py [out_dir]            # default: tools/sounds
    python tools/upload_sounds.py [host]            # copies them to /sounds on the memory card through the web page

Three groups, all named after what the firmware plays (firmware/src/audio.h, sleep_app.h):
  * system:   tap, notify, ok, error, alarm, chirp, evolve, coin
  * dragon:   eat, bath, heal, sleep, hatch, pet, fire, flap, gem, gem_miss, gameover, win, achievement
  * ambient:  rain, stream, wind, storm, ocean, forest, fire_camp, lullaby, whitenoise  (seamless loops, 24-40 s)
Everything is generated from scratch (no samples), so there is nothing to license. Re-running overwrites the files.
"""
import os
import sys
import wave

import numpy as np

SR = 16000
RNG = np.random.default_rng(20261021)


# ---------------------------------------------------------------- basics
def t_(dur):
    return np.arange(int(SR * dur)) / SR


def env(n, attack=0.005, release=0.05):
    e = np.ones(n)
    a, r = max(1, int(SR * attack)), max(1, int(SR * release))
    e[:a] = np.linspace(0, 1, a)
    e[-r:] *= np.linspace(1, 0, r)
    return e


def bell(freq, dur, decay=6.0, amp=1.0, bright=1.0):
    """A struck bell / music-box note: a few inharmonic partials with exponential decay."""
    t = t_(dur)
    s = np.zeros_like(t)
    for ratio, a, d in ((1.0, 1.0, 1.0), (2.0, 0.45, 1.6), (2.76, 0.25 * bright, 2.2), (5.4, 0.12 * bright, 3.5)):
        s += a * np.sin(2 * np.pi * freq * ratio * t) * np.exp(-decay * d * t)
    return amp * s * env(len(t), 0.002, 0.01)


def tri(freq, dur, amp=1.0, rel=0.04):
    t = t_(dur)
    return amp * (2 / np.pi) * np.arcsin(np.sin(2 * np.pi * freq * t)) * env(len(t), 0.005, rel)


def sine(freq, dur, amp=1.0, decay=0.0, rel=0.02):
    t = t_(dur)
    return amp * np.sin(2 * np.pi * freq * t) * np.exp(-decay * t) * env(len(t), 0.003, rel)


def sweep(f0, f1, dur, amp=1.0, shape=1.0):
    t = t_(dur)
    k = (t / dur) ** shape
    f = f0 + (f1 - f0) * k
    ph = 2 * np.pi * np.cumsum(f) / SR
    return amp * np.sin(ph) * env(len(t), 0.004, 0.02)


def place(out, snd, at):
    i = int(SR * at)
    if i >= len(out):
        return
    j = min(len(out), i + len(snd))
    out[i:j] += snd[: j - i]


def seq(parts, total=None):
    """parts: [(start_seconds, signal)] -> one signal."""
    end = max(s + len(x) / SR for s, x in parts)
    out = np.zeros(int(SR * (total or end)) + 1)
    for s, x in parts:
        place(out, x, s)
    return out


def noise_band(n, lo=None, hi=None, slope=0.0):
    """White noise shaped in the frequency domain: 4th-order high-pass at lo, low-pass at hi, tilt f**slope."""
    w = RNG.standard_normal(n)
    f = np.fft.rfftfreq(n, 1 / SR)
    g = np.ones_like(f)
    if lo:
        g *= 1 / (1 + (lo / np.maximum(f, 1e-3)) ** 4)
    if hi:
        g *= 1 / (1 + (f / hi) ** 4)
    if slope:
        g *= (np.maximum(f, 20.0) / 1000.0) ** slope
    x = np.fft.irfft(np.fft.rfft(w) * g, n)
    return x / (np.std(x) + 1e-9)


def smooth_env(n, rate, depth=0.5):
    """Slowly wandering envelope in [1-depth, 1]; `rate` = about how many changes per second."""
    k = max(2, int(n / SR * rate) + 2)
    pts = RNG.random(k)
    x = np.interp(np.linspace(0, k - 1, n), np.arange(k), pts)
    x = 0.5 - 0.5 * np.cos(np.pi * x)  # smoothing
    x = (x - x.min()) / (x.max() - x.min() + 1e-9)
    return 1 - depth + depth * x


def loopify(s, fade=1.5):
    """Make the signal loop without a click: the first `fade` seconds are cross-faded with what follows the end."""
    X = int(SR * fade)
    n = len(s) - X
    r = s[:n].copy()
    w = np.linspace(0, 1, X)
    r[:X] = r[:X] * np.sin(w * np.pi / 2) + s[n : n + X] * np.cos(w * np.pi / 2)
    return r


def reverb(x, mix=0.25, decays=((0.0297, 0.45), (0.0371, 0.4), (0.0411, 0.35), (0.0437, 0.3))):
    out = x.copy()
    for d, g in decays:
        n = int(SR * d)
        y = np.zeros_like(x)
        for k in range(1, 7):
            if k * n < len(x):
                y[k * n :] += x[: len(x) - k * n] * (g ** k)
        out += mix * y
    return out


def finish(x, peak=0.85, rms=None, soft=False):
    x = x - np.mean(x)
    if rms:
        x = x * (rms / (np.sqrt(np.mean(x ** 2)) + 1e-9))
        x = np.tanh(x * 1.1) / 1.1
        peak_now = np.max(np.abs(x))
        x = x * (min(peak, 0.95) / peak_now) if peak_now > peak else x
    else:
        x = x * (peak / (np.max(np.abs(x)) + 1e-9))
    return x


def save(path, x):
    d = (RNG.random(len(x)) - RNG.random(len(x))) / 255.0  # TPDF dither: hides the 8-bit steps in quiet tails
    q = np.clip(np.round((x + d) * 127 + 128), 0, 255).astype(np.uint8)
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(1)
        w.setframerate(SR)
        w.writeframes(q.tobytes())


# ---------------------------------------------------------------- notes
def hz(midi):
    return 440.0 * 2 ** ((midi - 69) / 12)


# ---------------------------------------------------------------- system sounds
def s_tap():
    t = t_(0.05)
    x = np.sin(2 * np.pi * 2100 * t) * np.exp(-90 * t) + 0.6 * noise_band(len(t), 1500, 5000)[: len(t)] * np.exp(-140 * t)
    return finish(x, 0.55)


def s_notify():
    return finish(seq([(0, bell(hz(88), 0.5, 7)), (0.17, bell(hz(93), 0.7, 6))]), 0.8)


def s_ok():
    return finish(seq([(0, bell(hz(84), 0.3, 9)), (0.12, bell(hz(91), 0.45, 8))]), 0.75)


def s_error():
    a = tri(hz(67), 0.16, 1.0) + 0.3 * tri(hz(67) * 3, 0.16, 1.0)
    b = tri(hz(64), 0.3, 1.0, 0.1) + 0.3 * tri(hz(64) * 3, 0.3, 1.0, 0.1)
    return finish(seq([(0, a), (0.2, b * np.exp(-3 * t_(0.3)))]), 0.7)


def s_alarm():
    """Bell arpeggio, twice, then a pause: loops as the alarm / timer ring."""
    notes = [hz(m) for m in (84, 88, 91, 96)]
    parts = []
    for rep in range(2):
        for i, f in enumerate(notes):
            parts.append((rep * 1.1 + i * 0.16, bell(f, 0.9, 4.5, 1.0, 1.3)))
    parts.append((2.3, bell(hz(96), 0.9, 4.0, 0.6, 1.3)))
    return finish(seq(parts, 3.4), 0.95)


def s_chirp():
    a = sweep(900, 2000, 0.09)
    b = sweep(2000, 1100, 0.13)
    vib = 1 + 0.08 * np.sin(2 * np.pi * 38 * t_(0.22))
    x = np.concatenate([a, b])[: len(vib)] * vib
    x2 = sweep(1400, 2300, 0.07, 0.6)
    return finish(seq([(0, x), (0.2, x2)]), 0.8)


def s_evolve():
    parts = []
    for i, m in enumerate((72, 76, 79, 84)):
        parts.append((i * 0.12, bell(hz(m), 0.9, 3.5, 1.0, 1.2)))
    for m in (84, 88, 91):
        parts.append((0.5, bell(hz(m), 1.6, 2.2, 0.55, 1.4)))
    for k in range(10):
        parts.append((0.6 + k * 0.08, bell(hz(96 + (k % 4) * 2), 0.4, 8, 0.25)))
    return finish(seq(parts), 0.9)


def s_coin():
    a = sine(hz(83), 0.07, 1.0, 0, 0.01) + 0.4 * sine(hz(83) * 2, 0.07, 1.0, 0, 0.01)
    b = sine(hz(88), 0.4, 1.0, 9, 0.05) + 0.4 * sine(hz(88) * 2, 0.4, 1.0, 12, 0.05)
    return finish(seq([(0, a), (0.07, b)]), 0.8)


# ---------------------------------------------------------------- dragon game
def s_eat():
    parts = []
    for i in range(3):
        n = int(SR * 0.075)
        x = noise_band(n, 150, 1800) * np.hanning(n) ** 0.6
        x += 0.5 * np.sin(2 * np.pi * (180 - 30 * i) * t_(0.075)) * np.exp(-30 * t_(0.075))
        parts.append((i * 0.15, x))
    parts.append((0.5, sweep(260, 110, 0.12, 0.6)))
    return finish(seq(parts), 0.8)


def s_bath():
    total = 1.5
    out = np.zeros(int(SR * total))
    out += 0.5 * noise_band(len(out), 300, 2500) * np.exp(-6 * t_(total))
    for i in range(34):
        at = RNG.random() ** 1.3 * 1.3
        f0 = RNG.uniform(350, 900)
        d = RNG.uniform(0.025, 0.07)
        place(out, sweep(f0, f0 * RNG.uniform(1.8, 2.6), d, RNG.uniform(0.25, 0.7)) * np.exp(-14 * t_(d)), at)
    return finish(out, 0.8)


def s_heal():
    parts = []
    for i, m in enumerate((103, 100, 96, 91, 88)):
        parts.append((i * 0.07, bell(hz(m), 0.7, 5, 0.8, 1.5)))
    for m in (84, 88, 91):
        parts.append((0.4, bell(hz(m), 1.0, 3.5, 0.45)))
    return finish(seq(parts), 0.85)


def s_sleep():
    parts = [(i * 0.24, bell(hz(m), 1.1, 3.8, 1.0, 0.7)) for i, m in enumerate((88, 84, 81, 79))]
    x = seq(parts)
    return finish(reverb(x, 0.3) * np.linspace(1, 0.55, len(x)), 0.75)


def s_hatch():
    out = np.zeros(int(SR * 2.4))
    at = 0.0
    for i in range(7):
        n = int(SR * 0.025)
        click = noise_band(n, 800, 5000) * np.exp(-90 * t_(0.025))
        place(out, click * (0.5 + 0.1 * i), at)
        at += 0.17 - i * 0.017
    for k, m in enumerate((72, 76, 79, 84, 88, 91, 96, 100)):
        place(out, bell(hz(m), 0.9, 4, 0.7, 1.3), 1.15 + k * 0.07)
    for m in (84, 88, 91, 96):
        place(out, bell(hz(m), 1.2, 2.5, 0.4), 1.75)
    return finish(out, 0.9)


def s_pet():
    t = t_(0.95)
    saw = 2 * ((220 * t) % 1.0) - 1
    f = 0.55 + 0.45 * np.sin(2 * np.pi * 27 * t) ** 2
    env_ = np.sin(np.pi * np.minimum(t / 0.95, 1)) ** 0.7
    x = (saw * 0.6 + 0.4 * np.sin(2 * np.pi * 440 * t)) * f * env_
    x += seq([(0, sweep(1500, 2400, 0.08, 0.5))], 0.95)[: len(t)]
    return finish(x, 0.7)


def s_fire():
    t = t_(0.9)
    n = noise_band(len(t), 200, 3000)
    sweepf = np.sin(np.pi * t / 0.9)
    # a moving band: crossfade a low and a high band along a bump
    lowb = noise_band(len(t), 150, 700)
    highb = noise_band(len(t), 1200, 4500)
    x = lowb * (1 - sweepf) * 0.8 + n * 0.5 + highb * sweepf * 0.9
    growl = ((95 + 12 * np.sin(2 * np.pi * 7 * t)) * t) % 1.0
    x += 0.5 * (2 * growl - 1) * np.sin(np.pi * t / 0.9)
    return finish(x * np.sin(np.pi * np.minimum(t / 0.9, 1)) ** 0.6, 0.85)


def s_flap():
    t = t_(0.14)
    x = noise_band(len(t), 400, 2800) * np.sin(np.pi * t / 0.14) ** 2
    return finish(x, 0.55)


def s_gem():
    return finish(bell(hz(96), 0.4, 9, 1.0, 1.0) + 0.5 * bell(hz(103), 0.4, 11), 0.8)


def s_gem_miss():
    t = t_(0.2)
    f = 150 * np.exp(-6 * t) + 60
    x = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-14 * t) + 0.3 * noise_band(len(t), 300, 1500) * np.exp(-50 * t)
    return finish(x, 0.7)


def s_gameover():
    parts = [(0.0, tri(hz(72), 0.22)), (0.22, tri(hz(71), 0.22)), (0.44, tri(hz(69), 0.22)), (0.66, tri(hz(65), 0.7, 1.0, 0.25) * np.exp(-2 * t_(0.7)))]
    x = seq(parts)
    return finish(x + 0.3 * np.sin(2 * np.pi * hz(53) * t_(len(x) / SR))[: len(x)] * np.exp(-2 * t_(len(x) / SR))[: len(x)], 0.75)


def s_win():
    parts = [(i * 0.085, bell(hz(m), 0.7, 5, 0.9, 1.2)) for i, m in enumerate((72, 76, 79, 84, 88))]
    for m in (84, 88, 91):
        parts.append((0.5, bell(hz(m), 1.2, 2.8, 0.55, 1.3)))
    return finish(seq(parts), 0.9)


def s_achievement():
    parts = [(i * 0.1, bell(hz(m), 0.8, 4.5, 0.9, 1.4)) for i, m in enumerate((79, 84, 88, 91))]
    for k in range(8):
        parts.append((0.5 + k * 0.06, bell(hz(100 + (k % 3) * 3), 0.35, 9, 0.3)))
    parts.append((0.45, bell(hz(96), 1.0, 3, 0.5)))
    return finish(seq(parts), 0.9)


# ---------------------------------------------------------------- ambient loops
def a_rain(dur=32):
    n = int(SR * dur)
    x = noise_band(n, 600, 6500, slope=0.35) * smooth_env(n, 0.5, 0.18)
    x += 0.5 * noise_band(n, 2500, 7000) * smooth_env(n, 1.2, 0.3)
    drops = np.zeros(n)
    for _ in range(int(dur * 45)):
        at = RNG.random() * dur
        d = RNG.uniform(0.002, 0.007)
        m = int(SR * d)
        c = noise_band(m, 2500, 7500) * np.exp(-np.linspace(0, 7, m))
        place(drops, c * RNG.uniform(0.3, 1.6), at)
    for _ in range(int(dur * 5)):  # fatter drops
        at = RNG.random() * dur
        m = int(SR * 0.012)
        f = RNG.uniform(900, 1700)
        place(drops, np.sin(2 * np.pi * f * t_(0.012)) * np.exp(-np.linspace(0, 6, m)) * RNG.uniform(0.3, 0.8), at)
    return loopify(finish(x * 0.8 + drops * 0.8, rms=0.13), 2.0)


def a_stream(dur=32):
    n = int(SR * dur)
    x = noise_band(n, 250, 3800, slope=0.15) * smooth_env(n, 0.8, 0.25)
    bub = np.zeros(n)
    for _ in range(int(dur * 38)):
        at = RNG.random() * dur
        d = RNG.uniform(0.02, 0.07)
        f0 = RNG.uniform(350, 1100)
        place(bub, sweep(f0, f0 * RNG.uniform(1.6, 2.8), d, RNG.uniform(0.15, 0.6)) * np.exp(-9 * t_(d)), at)
    flow = noise_band(n, 1200, 5200) * smooth_env(n, 2.5, 0.5)
    return loopify(finish(x * 0.9 + bub * 0.8 + flow * 0.25, rms=0.13), 2.0)


def a_wind(dur=36):
    n = int(SR * dur)
    low = noise_band(n, 60, 350) * smooth_env(n, 0.22, 0.75)
    mid = noise_band(n, 250, 1100) * smooth_env(n, 0.3, 0.8)
    high = noise_band(n, 900, 2800) * smooth_env(n, 0.4, 0.85)
    g = smooth_env(n, 0.18, 0.9)
    whistle = np.zeros(n)
    t = np.arange(n) / SR
    f = 1150 + 220 * np.sin(2 * np.pi * 0.11 * t) + 80 * np.sin(2 * np.pi * 0.37 * t)
    whistle = 0.5 * np.sin(2 * np.pi * np.cumsum(f) / SR) * (g ** 3) * smooth_env(n, 0.3, 0.9)
    return loopify(finish((low * 1.1 + mid * 0.8 + high * 0.45) * (0.4 + 0.6 * g) + whistle * 0.12, rms=0.15), 2.5)


def a_storm(dur=40):
    n = int(SR * dur)
    r = noise_band(n, 500, 6500, slope=0.4) * smooth_env(n, 0.6, 0.15)
    for _ in range(int(dur * 70)):
        at = RNG.random() * dur
        m = int(SR * RNG.uniform(0.002, 0.007))
        place(r, noise_band(m, 2500, 7500) * np.exp(-np.linspace(0, 7, m)) * RNG.uniform(0.6, 2.2), at)
    wind = (noise_band(n, 60, 400) * 1.1 + noise_band(n, 300, 1200) * 0.7) * smooth_env(n, 0.25, 0.8)
    out = r * 0.7 + wind * 0.8
    for at, amp, tail in ((6.5, 1.0, 5.0), (17.0, 0.75, 4.0), (29.5, 1.2, 6.0)):
        m = int(SR * tail)
        tt = np.arange(m) / SR
        crack = noise_band(m, 500, 4000) * np.exp(-30 * tt)
        rumble = noise_band(m, 35, 180) * (1 - np.exp(-tt / 0.08)) * np.exp(-tt / (tail / 3.0))
        rumble *= 1 + 0.5 * np.sin(2 * np.pi * 3.1 * tt)
        place(out, (crack * 0.9 + rumble * 2.2) * amp, at)
    return loopify(finish(out, rms=0.17), 2.5)


def a_ocean(dur=40):
    n = int(SR * dur)
    t = np.arange(n) / SR
    out = np.zeros(n)
    period = 9.5
    for k in range(5):
        at = 1.0 + k * period + RNG.uniform(-0.6, 0.6)
        m = int(SR * 8.5)
        tt = np.arange(m) / SR
        swell = np.sin(np.pi * np.clip(tt / 4.5, 0, 1)) ** 2 * np.exp(-np.maximum(tt - 4.5, 0) / 1.8)
        body = noise_band(m, 90, 1400) * swell
        foam = noise_band(m, 1500, 6000) * (np.clip((tt - 3.6) / 1.2, 0, 1) * np.exp(-np.maximum(tt - 4.8, 0) / 1.5)) * 0.7
        place(out, (body * 1.1 + foam) * RNG.uniform(0.8, 1.15), at)
    out += 0.25 * noise_band(n, 80, 900) * smooth_env(n, 0.3, 0.5)
    return loopify(finish(out, rms=0.16), 3.0)


def a_forest(dur=36):
    n = int(SR * dur)
    t = np.arange(n) / SR
    out = 0.35 * noise_band(n, 80, 600, slope=0.2) * smooth_env(n, 0.2, 0.7)
    leaves = noise_band(n, 2500, 6500) * smooth_env(n, 0.35, 0.9) ** 2
    out += 0.12 * leaves
    for carrier, rate, phase in ((4300, 3.1, 0.0), (4650, 2.6, 0.9), (4050, 3.7, 0.4)):
        trill = 0.5 * (1 + np.sign(np.sin(2 * np.pi * (rate * t + phase))))  # chirps / gaps
        pulses = 0.5 * (1 + np.sign(np.sin(2 * np.pi * 36 * t)))
        gate = trill * pulses * smooth_env(n, 0.15, 0.6)
        out += 0.17 * np.sin(2 * np.pi * carrier * t + 6 * np.sin(2 * np.pi * 36 * t)) * gate
    for at in (9.0, 24.5):  # an owl, far away
        for k, (dt, f0) in enumerate(((0.0, 420), (0.55, 400))):
            h = sweep(f0, f0 * 0.93, 0.4, 0.6) * np.hanning(int(SR * 0.4))
            place(out, reverb(h, 0.4), at + dt)
    return loopify(finish(out, rms=0.12), 2.5)


def a_fire_camp(dur=30):
    n = int(SR * dur)
    base = noise_band(n, 40, 500) * smooth_env(n, 0.6, 0.6)
    hiss = noise_band(n, 2500, 6000) * 0.18 * smooth_env(n, 1.0, 0.7)
    out = base * 0.8 + hiss
    for _ in range(int(dur * 22)):
        at = RNG.random() * dur
        m = int(SR * RNG.uniform(0.003, 0.016))
        lo, hi = RNG.uniform(600, 1500), RNG.uniform(2500, 6500)
        place(out, noise_band(m, lo, hi) * np.exp(-np.linspace(0, 6, m)) * RNG.uniform(0.5, 3.0), at)
    for _ in range(int(dur * 0.5)):  # a loud pop now and then
        at = RNG.random() * dur
        m = int(SR * 0.02)
        place(out, noise_band(m, 700, 4500) * np.exp(-np.linspace(0, 5, m)) * RNG.uniform(3, 5), at)
    return loopify(finish(out, rms=0.13), 2.0)


def a_lullaby(dur_bars=16):
    """Original music-box lullaby in C pentatonic (3/4, 66 BPM), 16 bars, loops."""
    beat = 60 / 66
    # (midi, beats); 0 = rest. Two 8-bar phrases, the second one answers the first.
    mel = [(76, 1), (79, 1), (81, 1), (79, 2), (76, 1), (74, 1), (76, 1), (79, 1), (76, 3),
           (74, 1), (76, 1), (79, 1), (81, 2), (84, 1), (81, 1), (79, 1), (76, 1), (79, 3),
           (84, 1), (81, 1), (79, 1), (76, 2), (74, 1), (72, 1), (74, 1), (76, 1), (72, 3),
           (79, 1), (76, 1), (74, 1), (72, 2), (74, 1), (76, 2), (74, 1), (72, 3)]
    bass = [48, 45, 41, 43, 48, 45, 41, 43, 48, 45, 41, 43, 48, 45, 41, 48]
    total = dur_bars * 3 * beat + 3.0
    out = np.zeros(int(SR * total))
    at = 0.0
    for m, b in mel:
        if m:
            place(out, bell(hz(m), 2.2, 2.6, 0.8, 0.9), at)
            place(out, bell(hz(m) * 1.003, 2.2, 2.8, 0.25, 0.4), at)  # a slightly detuned copy: the music-box shimmer
        at += b * beat
    for i, b in enumerate(bass):
        bt = i * 3 * beat
        place(out, bell(hz(b + 12), 3.0, 1.8, 0.6, 0.3), bt)
        for k, step in enumerate((7, 12)):
            place(out, bell(hz(b + 12 + step), 1.8, 3.5, 0.28, 0.3), bt + (k + 1) * beat)
    out = reverb(out, 0.45)
    loop_len = int(SR * dur_bars * 3 * beat)
    tail = out[loop_len:]
    out = out[:loop_len].copy()
    out[: len(tail)] += tail[: len(out)]  # the ring-out of the last bar wraps onto the start: seamless
    return finish(out, rms=0.1, peak=0.8)


def a_whitenoise(dur=20):
    n = int(SR * dur)
    x = noise_band(n, 60, 5500, slope=-0.5) * smooth_env(n, 0.15, 0.08)
    return loopify(finish(x, rms=0.12), 1.5)


SOUNDS = {
    'tap': s_tap, 'notify': s_notify, 'ok': s_ok, 'error': s_error, 'alarm': s_alarm, 'chirp': s_chirp, 'evolve': s_evolve, 'coin': s_coin,
    'eat': s_eat, 'bath': s_bath, 'heal': s_heal, 'sleep': s_sleep, 'hatch': s_hatch, 'pet': s_pet, 'fire': s_fire, 'flap': s_flap,
    'gem': s_gem, 'gem_miss': s_gem_miss, 'gameover': s_gameover, 'win': s_win, 'achievement': s_achievement,
    'rain': a_rain, 'stream': a_stream, 'wind': a_wind, 'storm': a_storm, 'ocean': a_ocean, 'forest': a_forest, 'fire_camp': a_fire_camp,
    'lullaby': a_lullaby, 'whitenoise': a_whitenoise,
}


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sounds')
    os.makedirs(out, exist_ok=True)
    total = 0
    for name, fn in SOUNDS.items():
        x = fn()
        save(os.path.join(out, name + '.wav'), x)
        size = os.path.getsize(os.path.join(out, name + '.wav'))
        total += size
        print(f'{name:12s} {len(x) / SR:5.1f} s  {size // 1024:4d} KB  peak {np.max(np.abs(x)):.2f}')
    print(f'{len(SOUNDS)} files, {total // 1024} KB')


if __name__ == '__main__':
    main()
