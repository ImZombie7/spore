#!/usr/bin/env python3
import re
import selectors
import subprocess
import sys
import time


CSI_RE = re.compile(rb"\x1b\[[0-9;=?]*[A-Za-z]")
INTERESTING_RE = re.compile(rb"(\[spore\][^\r\n]*|\[cell [0-9]+\][^\r\n]*|\[kernel\] lower sync fault[^\r\n]*)")
TRIGGER = b"[spore] stdin demo: child blocking on read(0)"


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: run_qemu_input.py QEMU [ARGS...]", file=sys.stderr)
        return 2

    proc = subprocess.Popen(
        sys.argv[1:],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    out = bytearray()
    selector = selectors.DefaultSelector()
    selector.register(proc.stdout, selectors.EVENT_READ)
    sent = False
    deadline = time.monotonic() + 30
    while proc.poll() is None:
        if time.monotonic() > deadline:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()
            break
        events = selector.select(timeout=0.05)
        for key, _ in events:
            chunk = key.fileobj.read1(256)
            if not chunk:
                continue
            out += chunk
            clean = CSI_RE.sub(b"", bytes(out))
            if not sent and TRIGGER in clean and proc.stdin is not None:
                proc.stdin.write(b"z\n")
                proc.stdin.flush()
                sent = True

    if proc.stdout is not None:
        out += proc.stdout.read()
    clean = CSI_RE.sub(b"", bytes(out))
    matches = INTERESTING_RE.findall(clean)
    if matches:
        for match in matches:
            print(match.decode("utf-8", errors="replace"))
    else:
        sys.stdout.buffer.write(clean)
        if clean and not clean.endswith(b"\n"):
            print()
    return proc.returncode if proc.returncode is not None else 124


if __name__ == "__main__":
    raise SystemExit(main())
