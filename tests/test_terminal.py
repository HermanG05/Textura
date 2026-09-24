import fcntl
import os
import select
import signal
import struct
import subprocess
import tempfile
import termios
import time
from pathlib import Path


class Terminal:
    def __init__(self, filename, term="xterm"):
        self.master, slave = os.openpty()
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 80, 0, 0))
        self.process = subprocess.Popen(
            [str(Path(__file__).resolve().parents[1] / "Textura"), str(filename)],
            stdin=slave,
            stdout=slave,
            stderr=slave,
            env={**os.environ, "TERM": term},
        )
        os.close(slave)
        output = self.read()
        deadline = time.monotonic() + 3
        while b"Help" not in output and time.monotonic() < deadline:
            output += self.read()
        assert b"Help" in output
        self.initial_output = output

    def read(self):
        output = bytearray()
        deadline = time.monotonic() + 0.15
        while time.monotonic() < deadline:
            if select.select([self.master], [], [], 0.02)[0]:
                try:
                    output.extend(os.read(self.master, 65536))
                except OSError:
                    break
        return bytes(output)

    def send(self, text):
        os.write(self.master, text)
        return self.read()

    def resize(self, rows, cols):
        fcntl.ioctl(self.master, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))
        self.process.send_signal(signal.SIGWINCH)
        self.read()

    def close(self):
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait(timeout=3)
        os.close(self.master)


with tempfile.TemporaryDirectory(prefix="textura-test-") as directory:
    filename = Path(directory) / "sample.txt"
    original = b"  alpha alpha\n\tbeta\nlast"
    filename.write_bytes(original)
    terminal = Terminal(filename)
    try:
        terminal.send(b"\x06")
        terminal.send(b"beta\r")
        terminal.send(b"\x12")
        terminal.send(b"\r")
        terminal.send(b"BETA\r")
        terminal.send(b"\x1a")
        terminal.send(b"\x19")
        terminal.send(b"\x07")
        terminal.send(b"3\r")
        terminal.send(b"\x05!\x13")
        assert filename.read_bytes() == b"  alpha alpha\n\tBETA\nlast!"
        terminal.send(b"\x01\x1b[3~")
        terminal.send(b"\x13")
        assert filename.read_bytes() == b"  alpha alpha\n\tBETA\nast!"
        terminal.send(b"\x1a\x13")
        assert filename.read_bytes() == b"  alpha alpha\n\tBETA\nlast!"
        terminal.send(b"X")
        terminal.send(b"\x11")
        assert terminal.process.poll() is None
        terminal.send(b"\x11")
        assert terminal.process.wait(timeout=3) == 0
        assert filename.read_bytes() == b"  alpha alpha\n\tBETA\nlast!"
    finally:
        terminal.close()

    filename.write_bytes(b"  text")
    terminal = Terminal(filename)
    try:
        terminal.send(b"\x05\rnext\x13")
        assert filename.read_bytes() == b"  text\n  next"
        terminal.send(b"\x06")
        terminal.send(b"next\x1b")
        terminal.resize(3, 10)
        terminal.send(b"\x07")
        terminal.send(b"1\x1b")
        terminal.resize(12, 40)
        terminal.send(b"\x1bOP")
        terminal.send(b" ")
        terminal.send(b"\x13\x11")
        assert terminal.process.wait(timeout=3) == 0
        assert filename.read_bytes() == b"  text\n  next"
    finally:
        terminal.close()

    filename.write_bytes(b"start" + b" " * 200 + b"end\n")
    terminal = Terminal(filename)
    try:
        terminal.send(b"\x05!\x13")
        assert filename.read_bytes() == b"start" + b" " * 200 + b"end!\n"
        terminal.resize(8, 24)
        terminal.send(b"\x01")
        terminal.send(b"\x06end\r")
        terminal.send(b"X\x13\x11")
        assert terminal.process.wait(timeout=3) == 0
        assert filename.read_bytes() == b"start" + b" " * 200 + b"Xend!\n"
    finally:
        terminal.close()

    for extension, content in [
        ("c", b"int main() { return 42; }\n"),
        ("cpp", b"class Example { int value = 42; };\n"),
        ("java", b"public class Example { int value = 42; }\n"),
        ("py", b"def example():\n    return 42\n"),
    ]:
        filename = Path(directory) / ("syntax." + extension)
        filename.write_bytes(content)
        terminal = Terminal(filename)
        try:
            assert b"36m" in terminal.initial_output
            assert b"35m" in terminal.initial_output
            terminal.send(b"\x06return\r")
            terminal.resize(8, 32)
            terminal.send(b"\x13\x11")
            assert terminal.process.wait(timeout=3) == 0
            assert filename.read_bytes() == content
        finally:
            terminal.close()

    filename = Path(directory) / "extensionless"
    content = b"def greet(user):\n    return f\"Hello {user.name}\"\n"
    filename.write_bytes(content)
    terminal = Terminal(filename, "xterm-256color")
    try:
        output = b""
        for language in range(5):
            output += terminal.send(b"\x1bOR")
        assert b"38;5;176m" in output
        assert b"38;5;117m" in output
        output = terminal.send(b"\x1bOS")
        assert b"38;5;90m" in output
        assert b"38;5;25m" in output
        terminal.send(b"\x13\x11")
        assert terminal.process.wait(timeout=3) == 0
        assert filename.read_bytes() == content
    finally:
        terminal.close()

    terminal = Terminal(filename, "vt100")
    try:
        assert b"36m" not in terminal.initial_output
        terminal.send(b"\x13\x11")
        assert terminal.process.wait(timeout=3) == 0
        assert filename.read_bytes() == content
    finally:
        terminal.close()

print("Terminal integration tests passed")
