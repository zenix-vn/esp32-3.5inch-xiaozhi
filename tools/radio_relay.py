"""Radio relay for Zin100: re-streams the VOV channels as plain HTTP the board can play.

    python tools/radio_relay.py [--port 8000]

The VOV streams are HLS over HTTPS at 256 kbit/s and need a Referer header. The ESP32 (no PSRAM) has too little
heap to keep a TLS session open at that rate next to its UI, so this script, running on any PC on the same
network, pulls a channel with ffmpeg and serves it as AAC-LC, mono, 16 kHz, 40 kbit/s, in ADTS frames over HTTP:

    GET /vov1 /vov2 /vov3 /vov5 /vovgthn /vovgthcm   -> endless audio/aac stream (one ffmpeg per listener)
    GET /                                            -> channel list (JSON)

On the board, point the Radio app at this PC once (kept in flash; an empty value goes back to direct HTTPS):
    curl -X POST "http://<board>/api/radio?relay=<this PC's IP>:8000"
Needs ffmpeg (winget install Gyan.FFmpeg); set FFMPEG=<path> if it is not on PATH.
"""
import argparse
import glob
import json
import os
import shutil
import socket
import subprocess
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

CHANNELS = {
    'vov1': 'vov1vov5Vietnamese',
    'vov2': 'vov2',
    'vov3': 'vov3',
    'vov5': 'vov247vov5English',
    'vovgthn': 'vovGTHN',
    'vovgthcm': 'vovGTHCM',
}
URL = 'https://stream.vovlive.vn/vovlive/{}.sdp_aac/playlist.m3u8'
PREBUF = 8 * 40000 // 8  # bytes: ~8 s of the 40 kbit/s output collected before sending starts
REFERER = 'https://vovlive.vn/'


def find_ffmpeg():
    exe = os.environ.get('FFMPEG') or shutil.which('ffmpeg')
    if exe:
        return exe
    # winget installs it here; PATH only knows it in shells started after the install
    hits = glob.glob(os.path.expandvars(r'%LOCALAPPDATA%\Microsoft\WinGet\Packages\Gyan.FFmpeg*\*\bin\ffmpeg.exe'))
    return hits[0] if hits else None


FFMPEG = find_ffmpeg()


class Relay(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.0'  # no length: the stream ends when either side closes

    def log_message(self, fmt, *args):
        sys.stderr.write('%s %s\n' % (self.client_address[0], fmt % args))

    def do_GET(self):
        name = self.path.strip('/').lower()
        if not name:
            body = json.dumps(sorted(CHANNELS)).encode()
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if name not in CHANNELS:
            self.send_error(404, 'unknown channel')
            return
        cmd = [FFMPEG, '-hide_banner', '-loglevel', 'error',
               '-headers', 'Referer: %s\r\n' % REFERER,
               '-live_start_index', '-3',  # 3 segments back: ~30 s in hand right away for the jitter buffer
               '-i', URL.format(CHANNELS[name]),
               '-vn', '-ac', '1', '-ar', '16000', '-c:a', 'aac', '-b:a', '40k',
               '-flush_packets', '1', '-f', 'adts', 'pipe:1']  # each frame out at once (5 KB/s fills buffers slowly)
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stdin=subprocess.DEVNULL)
        self.log_message('start %s (ffmpeg pid %d)', name, proc.pid)
        # Jitter buffer: ffmpeg follows the live playlist and hands out audio in bursts, some of them slower than real
        # time, while the board keeps only half a second. Collect PREBUF first, then send as it comes.
        buf, cond, done = bytearray(), threading.Condition(), [False]

        def pump():
            while True:
                chunk = proc.stdout.read1(4096)
                with cond:
                    if not chunk:
                        done[0] = True
                    else:
                        buf.extend(chunk)
                    cond.notify()
                if not chunk:
                    return

        threading.Thread(target=pump, daemon=True).start()
        try:
            self.send_response(200)
            self.send_header('Content-Type', 'audio/aac')
            self.send_header('Cache-Control', 'no-cache')
            self.end_headers()
            with cond:
                cond.wait_for(lambda: len(buf) >= PREBUF or done[0], timeout=20)
            while True:
                with cond:
                    cond.wait_for(lambda: buf or done[0], timeout=30)
                    if not buf:
                        break
                    chunk = bytes(buf)
                    del buf[:]
                self.wfile.write(chunk)  # blocks while the board's TCP window is full: that paces the sending
        except (ConnectionError, socket.timeout, OSError):
            pass
        finally:
            proc.kill()
            proc.wait()
            self.log_message('stop %s', name)


def local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('10.255.255.255', 1))
        return s.getsockname()[0]
    except OSError:
        return '127.0.0.1'
    finally:
        s.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--port', type=int, default=8000)
    a = ap.parse_args()
    if not FFMPEG:
        sys.exit('ffmpeg not found: winget install Gyan.FFmpeg, or set FFMPEG=<path to ffmpeg.exe>')
    srv = ThreadingHTTPServer(('0.0.0.0', a.port), Relay)
    srv.daemon_threads = True
    print('Zin100 radio relay on http://%s:%d  (ffmpeg: %s)' % (local_ip(), a.port, FFMPEG), flush=True)
    print('Point the board at it once:  curl -X POST "http://<board>/api/radio?relay=%s:%d"' % (local_ip(), a.port),
          flush=True)
    srv.serve_forever()


if __name__ == '__main__':
    main()
