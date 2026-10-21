"""Copies the generated sound files to /sounds on the memory card through the device's web page.

    python tools/gen_sounds.py                         # makes tools/sounds/*.wav
    python tools/upload_sounds.py [host] [dir]         # default host voi.local, dir tools/sounds

The device must be on the same network. Existing files with the same name are overwritten.
"""
import os
import sys
import urllib.request
import uuid

host = sys.argv[1] if len(sys.argv) > 1 else 'voi.local'
src = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sounds')
files = sorted(f for f in os.listdir(src) if f.lower().endswith('.wav'))
for i, name in enumerate(files, 1):
    data = open(os.path.join(src, name), 'rb').read()
    b = uuid.uuid4().hex
    body = (f'--{b}\r\nContent-Disposition: form-data; name="f"; filename="{name}"\r\nContent-Type: audio/wav\r\n\r\n').encode() + data + f'\r\n--{b}--\r\n'.encode()
    req = urllib.request.Request(f'http://{host}/api/sd/upload?dir=/sounds', data=body, method='POST', headers={'Content-Type': f'multipart/form-data; boundary={b}'})
    with urllib.request.urlopen(req, timeout=120) as r:
        print(f'[{i}/{len(files)}] {name:18s} {len(data) // 1024:4d} KB  {r.read().decode()}')
