"""Network client for the Choose-Your-Own-GPT receipt printer.

The device listens on TCP port 9100 and forwards every byte it receives to
its 58mm thermal printer. This module builds the printer's command bytes
(text styles, bitmaps) so you don't have to.

Stdlib only for text and glyphs; Pillow (``pip install pillow``) is needed
only for ``image()`` and ``render_text()``.

Example:

    from cyogpt_printer import Printer

    HEART = [
        ".##...##.",
        "####.####",
        "#########",
        ".#######.",
        "..#####..",
        "...###...",
        "....#....",
    ]

    with Printer("192.168.1.50") as p:
        p.justify("C").size("L").bold().text("Hello!").bold(False).size("S")
        p.glyph(HEART, scale=6, align="C")
        p.text("Sent from a Raspberry Pi")
        p.feed(3)

Nothing is sent until the ``with`` block exits (or ``send()`` is called).
"""

import socket

ESC = b"\x1b"
GS = b"\x1d"
DC2 = b"\x12"

PAPER_WIDTH_DOTS = 384  # 58mm printer, 8 dots/mm
LINE_WIDTH_CHARS = 32   # characters per line at size "S"
DEFAULT_PORT = 9100


class Printer:
    def __init__(self, host, port=DEFAULT_PORT, timeout=30.0, encoding="cp437"):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.encoding = encoding
        self._buf = bytearray()

    # ─── Sending ───────────────────────────────────────────────

    def send(self):
        """Send everything queued so far as one print job."""
        if not self._buf:
            return
        data = bytes(self._buf)
        self._buf.clear()
        with socket.create_connection((self.host, self.port), timeout=self.timeout) as s:
            s.sendall(data)
            s.shutdown(socket.SHUT_WR)
            # Wait for the device to finish and close its side.
            try:
                while s.recv(64):
                    pass
            except socket.timeout:
                pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        if exc_type is None:
            self.send()

    def raw(self, data):
        """Queue raw bytes (for commands this class doesn't wrap)."""
        self._buf += data
        return self

    # ─── Text ──────────────────────────────────────────────────

    def write(self, s):
        """Queue text without a trailing newline."""
        self._buf += s.encode(self.encoding, errors="replace")
        return self

    def text(self, s=""):
        """Queue a line of text. The printer wraps at 32 chars (size S)."""
        return self.write(s + "\n")

    def feed(self, lines=1):
        return self.raw(ESC + b"d" + bytes([max(0, min(lines, 255))]))

    def reset(self):
        """Restore power-on defaults (ESC @)."""
        return self.raw(ESC + b"@")

    # ─── Styles ────────────────────────────────────────────────

    def bold(self, on=True):
        return self.raw(ESC + b"E" + bytes([1 if on else 0]))

    def underline(self, weight=1):
        """0 = off, 1 = thin, 2 = thick."""
        return self.raw(ESC + b"-" + bytes([max(0, min(weight, 2))]))

    def inverse(self, on=True):
        return self.raw(GS + b"B" + bytes([1 if on else 0]))

    def justify(self, align="L"):
        """'L', 'C' or 'R'. Applies to text only; see ``align`` for bitmaps."""
        return self.raw(ESC + b"a" + bytes([{"L": 0, "C": 1, "R": 2}[align.upper()]]))

    def size(self, s="S"):
        """'S' normal, 'M' double height, 'L' double width and height."""
        return self.raw(GS + b"!" + bytes([{"S": 0x00, "M": 0x01, "L": 0x11}[s.upper()]]))

    def line_height(self, dots=30):
        return self.raw(ESC + b"3" + bytes([max(24, min(dots, 255))]))

    # ─── Bitmaps and glyphs ────────────────────────────────────

    def bitmap(self, width, height, data, align="L"):
        """Queue a 1-bit bitmap.

        ``data`` is row-major, (width + 7) // 8 bytes per row, MSB = leftmost
        pixel, 1 = black. ``align`` ('L'/'C'/'R') pads the image horizontally.
        """
        row_bytes = (width + 7) // 8
        if len(data) != row_bytes * height:
            raise ValueError("bitmap data is %d bytes, expected %d"
                             % (len(data), row_bytes * height))
        rows = [bytes(data[y * row_bytes:(y + 1) * row_bytes]) for y in range(height)]
        return self._rows(rows, row_bytes * 8, align)

    def glyph(self, rows, scale=1, align="L", on="#"):
        """Queue a glyph drawn as ASCII art.

        ``rows`` is a list of equal-ish length strings; characters in ``on``
        are black, anything else is white. ``scale`` enlarges each pixel.
        """
        width = max(len(r) for r in rows) * scale
        packed = []
        for r in rows:
            bits = []
            for ch in r:
                bits.extend([ch in on] * scale)
            row = _pack_bits(bits, width)
            packed.extend([row] * scale)
        return self._rows(packed, width, align)

    def image(self, img, align="C", max_width=PAPER_WIDTH_DOTS, dither=True):
        """Queue a Pillow image (or path), scaled down to fit and dithered."""
        from PIL import Image
        if not isinstance(img, Image.Image):
            img = Image.open(img)
        if img.mode in ("RGBA", "LA", "P"):
            img = img.convert("RGBA")
            bg = Image.new("RGBA", img.size, "white")
            bg.alpha_composite(img)
            img = bg
        img = img.convert("L")
        if img.width > max_width:
            img = img.resize((max_width, round(img.height * max_width / img.width)))
        img = img.convert("1", dither=Image.FLOYDSTEINBERG if dither else Image.NONE)
        return self._pil_1bit(img, align)

    def render_text(self, s, font_path=None, font_size=48, align="C"):
        """Render text (any Unicode, emoji fonts, symbols...) as a bitmap.

        Use this for glyphs the printer's built-in font doesn't have.
        ``font_path`` is a .ttf/.otf file, e.g.
        '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'.
        """
        from PIL import Image, ImageDraw, ImageFont
        font = (ImageFont.truetype(font_path, font_size) if font_path
                else ImageFont.load_default(font_size))
        left, top, right, bottom = ImageDraw.Draw(Image.new("1", (1, 1))).multiline_textbbox(
            (0, 0), s, font=font, align={"L": "left", "C": "center", "R": "right"}[align])
        img = Image.new("1", (min(right - left, PAPER_WIDTH_DOTS), bottom - top), 1)
        ImageDraw.Draw(img).multiline_text(
            (-left, -top), s, font=font, fill=0,
            align={"L": "left", "C": "center", "R": "right"}[align])
        return self._pil_1bit(img, align)

    # ─── Internals ─────────────────────────────────────────────

    def _pil_1bit(self, img, align):
        # Pillow mode "1": 0 = black. Printer: 1 = black.
        px = img.load()
        rows = [_pack_bits([px[x, y] == 0 for x in range(img.width)], img.width)
                for y in range(img.height)]
        return self._rows(rows, img.width, align)

    def _rows(self, rows, width, align):
        """Pad rows to the requested alignment and emit DC2 * chunks."""
        width = min(width, PAPER_WIDTH_DOTS)
        row_bytes = (width + 7) // 8
        pad_left = {"L": 0,
                    "C": (PAPER_WIDTH_DOTS // 8 - row_bytes) // 2,
                    "R": PAPER_WIDTH_DOTS // 8 - row_bytes}[align.upper()]
        out_bytes = row_bytes + pad_left
        rows = [b"\x00" * pad_left + r[:row_bytes].ljust(row_bytes, b"\x00") for r in rows]
        for start in range(0, len(rows), 255):
            chunk = rows[start:start + 255]
            self._buf += DC2 + b"*" + bytes([len(chunk), out_bytes])
            for r in chunk:
                self._buf += r
        return self


def _pack_bits(bits, width):
    out = bytearray((width + 7) // 8)
    for x, b in enumerate(bits[:width]):
        if b:
            out[x >> 3] |= 0x80 >> (x & 7)
    return bytes(out)


def main():
    import argparse
    import sys

    ap = argparse.ArgumentParser(description="Print to a Choose-Your-Own-GPT device.")
    ap.add_argument("host", help="device IP or hostname")
    ap.add_argument("text", nargs="?", help="text to print (default: read stdin)")
    ap.add_argument("--port", type=int, default=DEFAULT_PORT)
    ap.add_argument("--image", help="image file to print (needs Pillow)")
    ap.add_argument("--size", choices="SML", default="S")
    ap.add_argument("--center", action="store_true")
    ap.add_argument("--bold", action="store_true")
    args = ap.parse_args()

    with Printer(args.host, args.port) as p:
        if args.image:
            p.image(args.image)
        text = args.text if args.text is not None else (None if args.image else sys.stdin.read())
        if text:
            p.size(args.size).justify("C" if args.center else "L").bold(args.bold)
            p.text(text.rstrip("\n"))
        p.feed(3)


if __name__ == "__main__":
    main()
