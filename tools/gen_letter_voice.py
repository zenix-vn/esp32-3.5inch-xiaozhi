"""Records the voice of the kids' alphabet game (firmware/src/kids.h) with a macOS Vietnamese neural voice.

    python3 tools/gen_letter_voice.py              # makes tools/letters/*.wav (macOS only: tools/tts_vi.swift, afconvert)

The voice is a downloadable one: System Settings > Accessibility > Spoken Content > System Voice > Manage Voices >
Vietnamese. `swift tools/tts_vi.swift --list` shows the installed ones; VOICE below picks one (the compact "Linh"
said e / ê as ơ).
    python3 tools/upload_sounds.py voi.local tools/letters

16 kHz 16-bit mono WAV for /sounds on the memory card. Per letter:
    chu_<x>.wav   the letter's name ("chữ bờ")
    hoc_<x>.wav   the name and the example word ("chữ bờ... bé"): Học chữ, when a letter is shown or tapped
    tim_<x>.wav   "Con hãy tìm chữ bờ": the question in Tìm chữ
    hoa_<x>.wav   "Đây là chữ bờ viết hoa. Con tìm chữ bờ viết thường nhé": the question in Chữ hoa - chữ thường
    sai_<x>.wav   "Đây là chữ bờ. Con thử lại nhé": a wrong pick names the letter the child chose
<x> is the letter in Telex style: aw = ă, aa = â, dd = đ, ee = ê, oo = ô, ow = ơ, uw = ư.
And once: dung_1..3.wav (praise for a right answer), ket_1..3.wav (end of a round: great, good, try again),
chao_mung.wav (the welcome when the app opens).
"""
import os
import subprocess
import tempfile
import wave

import numpy as np

# file name, how the letter is said (the way Vietnamese kindergartens teach it), example word (as in kids.h)
LETTERS = [
    ('a', 'a', 'áo'), ('aw', 'á', 'ăn'), ('aa', 'ớ', 'ấm'), ('b', 'bờ', 'bé'), ('c', 'cờ', 'cá'), ('d', 'dờ', 'dê'),
    ('dd', 'đờ', 'đèn'), ('e', 'e', 'em'), ('ee', 'ê', 'ếch'), ('g', 'gờ', 'gà'), ('h', 'hờ', 'hoa'), ('i', 'i', 'bi'),
    ('k', 'ca', 'kẹo'), ('l', 'lờ', 'lá'), ('m', 'mờ', 'mèo'), ('n', 'nờ', 'nơ'), ('o', 'o', 'ong'), ('oo', 'ô', 'ô tô'),
    ('ow', 'ơ', 'ớt'), ('p', 'pờ', 'pin'), ('q', 'quờ', 'quả'), ('r', 'rờ', 'rùa'), ('s', 'sờ', 'sao'), ('t', 'tờ', 'táo'),
    ('u', 'u', 'ủng'), ('uw', 'ư', 'sư tử'), ('v', 'vờ', 'voi'), ('x', 'xờ', 'xe'), ('y', 'i dài', 'y tá'),
]
VOICE = 'com.apple.ttsbundle.gryphon-neural_vi-VN-B_vi-VN_premium'  # "Voice 2"; vi-VN-C ("Voice 3") is the other one
RATE = 0.42  # 0.5 is the normal speed: a little slower for a 5-year-old
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'letters')
LOUD_RMS = 0.30  # the speech is raised to this level (the voice comes out ~0.13) and soft-limited: the speaker is small
TTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'tts_vi.swift')


def texts():
    """(file name, text) for every recording."""
    for name, said, word in LETTERS:
        # "chữ" in front: alone the voice flattens the huyền tone (mờ, hờ, gờ came out as mơ, hơ, gơ)
        yield f'chu_{name}', f'chữ {said}'
        yield f'hoc_{name}', f'chữ {said}... {word}'
        yield f'tim_{name}', f'Con hãy tìm chữ {said}'
        yield f'hoa_{name}', f'Đây là chữ {said} viết hoa. Con tìm chữ {said} viết thường nhé'
        yield f'sai_{name}', f'Đây là chữ {said}. Con thử lại nhé'
    for i, text in enumerate(['Đúng rồi!', 'Giỏi quá!', 'Con giỏi lắm!'], 1):
        yield f'dung_{i}', text
    for i, text in enumerate(['Hoan hô! Con giỏi quá!', 'Con làm tốt lắm!', 'Cố lên nào, mình chơi lại nhé!'], 1):
        yield f'ket_{i}', text
    yield 'chao_mung', 'Chào mừng Voi con đã quay lại với lớp học. Hãy chăm học để nhận được nhiều kim cương nhé!'


def louder(path):
    """Raises a 16-bit mono WAV to LOUD_RMS (measured where there is speech) and rounds the peaks off with tanh."""
    with wave.open(path) as w:
        params = w.getparams()
        x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float32) / 32768
    voiced = x[np.abs(x) > 0.02]
    rms = np.sqrt(np.mean(voiced ** 2)) if voiced.size else 0
    if rms > 0:
        x = np.tanh(x * (LOUD_RMS / rms) * 1.2) / np.tanh(1.2)
        x = x * (0.97 / max(0.97, np.max(np.abs(x))))
    with wave.open(path, 'wb') as w:
        w.setparams(params)
        w.writeframes((x * 32767).astype('<i2').tobytes())


def main():
    os.makedirs(OUT, exist_ok=True)
    items = list(texts())
    with tempfile.TemporaryDirectory() as tmp:
        jobs = os.path.join(tmp, 'jobs.tsv')
        with open(jobs, 'w', encoding='utf-8') as f:
            for name, text in items:
                f.write(f'{os.path.join(tmp, name)}.caf\t{text}\n')
        subprocess.run(['swift', TTS, VOICE, str(RATE), jobs], check=True)
        for name, _ in items:  # 16 kHz 16-bit mono for the device
            subprocess.run(['afconvert', '-f', 'WAVE', '-d', 'LEI16@16000', '-c', '1', os.path.join(tmp, name + '.caf'),
                            os.path.join(OUT, name + '.wav')], check=True)
            louder(os.path.join(OUT, name + '.wav'))
    total = sum(os.path.getsize(os.path.join(OUT, f)) for f in os.listdir(OUT))
    print(f'\n{len(os.listdir(OUT))} files, {total // 1024} KB in {OUT}')


if __name__ == '__main__':
    main()
