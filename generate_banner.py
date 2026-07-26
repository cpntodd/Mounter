#!/usr/bin/env python3
"""Generate Mounter banner GIF matching cpntodd brand theme."""

import math, os, sys

W, H = 1200, 400
FRAMES = 24
DELAY = 10

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Install Pillow: pip install Pillow")
    sys.exit(1)

# ── cpntodd brand palette ───────────────────────────────────
BG         = (30, 30, 30)        # #1e1e1e
SURFACE    = (37, 37, 38)        # #252526
ACCENT     = (240, 144, 64)      # #f09040
ACCENT_LIGHT = (245, 166, 35)    # #f5a623
ACCENT_DARK  = (204, 119, 34)    # #cc7722
TEXT       = (212, 212, 212)     # #d4d4d4
HEADING    = (255, 255, 255)     # #ffffff
MUTED      = (157, 157, 157)     # #9d9d9d
BORDER     = (62, 62, 66)        # #3e3e42
CODE_BG    = (45, 45, 45)        # #2d2d2d

OUT_DIR = "assets"
os.makedirs(OUT_DIR, exist_ok=True)

# ── Find fonts ──────────────────────────────────────────────
def find_font(names, size):
    for name in names:
        for base in ["/usr/share/fonts", "/usr/local/share/fonts"]:
            for root, dirs, files in os.walk(base):
                for f in files:
                    if name.lower() in f.lower() and f.endswith(('.ttf', '.otf')):
                        return ImageFont.truetype(os.path.join(root, f), size)
    return ImageFont.load_default()

title_font = find_font(["DejaVuSans-Bold", "DejaVuSans"], 64)
mono_font  = find_font(["DejaVuSansMono", "DejaVuSans"], 18)
small_font = find_font(["DejaVuSansMono", "DejaVuSans"], 12)

frames = []
for frame in range(FRAMES):
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)

    t = frame / FRAMES

    # ── Dark gradient background ────────────────────────────
    for y in range(H):
        ratio = y / H
        r = int(10 + 8 * ratio)
        g = int(10 + 6 * ratio)
        b = int(12 + 8 * ratio)
        draw.line([(0, y), (W, y)], fill=(r, g, b))

    # ── Subtle grid ─────────────────────────────────────────
    for x in range(0, W, 40):
        draw.line([(x, 0), (x, H)], fill=(28, 28, 32), width=1)
    for y in range(0, H, 40):
        draw.line([(0, y), (W, y)], fill=(28, 28, 32), width=1)

    # ── Circuit traces ──────────────────────────────────────
    traces = [
        ((0, 200), (180, 200), (180, 80), (280, 80)),
        ((W, 160), (W-200, 160), (W-200, 240), (W-320, 240)),
        ((0, 280), (120, 280), (120, 320), (200, 320)),
        ((W, 300), (W-250, 300), (W-250, 100), (W-400, 100)),
    ]
    for i, trace in enumerate(traces):
        points = trace
        alpha = int(80 + 40 * math.sin(t * math.pi * 2 + i))
        for j in range(len(points) - 1):
            draw.line([points[j], points[j+1]], fill=(ACCENT[0], ACCENT[1], ACCENT[2], min(255, alpha)), width=1)

    # ── Glow behind title ───────────────────────────────────
    pulse = (math.sin(t * math.pi / 12) + 1) / 2
    for r in range(100, 0, -3):
        alpha = int(25 * (1 - r / 100) * (0.6 + 0.4 * pulse))
        if alpha > 0:
            draw.ellipse(
                [W//2 - r, H//2 - 50 - r//3, W//2 + r, H//2 - 50 + r//3],
                fill=(ACCENT[0], ACCENT[1], ACCENT[2], min(255, alpha))
            )

    # ── Surface card behind title ────────────────────────────
    card_margin = 60
    card_x1, card_y1 = card_margin, H//2 - 70
    card_x2, card_y2 = W - card_margin, H//2 + 40
    draw.rounded_rectangle(
        [card_x1, card_y1, card_x2, card_y2],
        radius=8, fill=SURFACE, outline=BORDER, width=1
    )

    # ── Terminal-style prefix ───────────────────────────────
    prefix = "cpntodd@mounter:~$ "
    px, py = card_x1 + 20, card_y1 + 14
    draw.text((px, py), prefix, fill=ACCENT, font=small_font)

    # ── Title ───────────────────────────────────────────────
    title = "MOUNTER"
    bbox = draw.textbbox((0, 0), title, font=title_font)
    tw = bbox[2] - bbox[0]
    tx = (W - tw) // 2
    ty = H // 2 - 54
    draw.text((tx + 2, ty + 1), title, fill=(0, 0, 0), font=title_font)
    draw.text((tx, ty), title, fill=HEADING, font=title_font)

    # ── Subtitle ────────────────────────────────────────────
    subtitle = "SMB / CIFS  ·  Mount GUI  ·  Debian Linux"
    bbox = draw.textbbox((0, 0), subtitle, font=mono_font)
    sw = bbox[2] - bbox[0]
    sx = (W - sw) // 2
    sy = H // 2 + 10
    draw.text((sx, sy), subtitle, fill=MUTED, font=mono_font)

    # ── Accent line below subtitle ──────────────────────────
    line_y = H // 2 + 36
    line_w = 200
    draw.line([(W//2 - line_w//2, line_y), (W//2 + line_w//2, line_y)], fill=ACCENT, width=2)

    # ── Floating terminal-style particles ───────────────────
    chars = "01▮▯▓▒░◆◇○●"
    for i in range(10):
        px = int(card_x1 + 30 + (card_x2 - card_x1 - 60) * ((i * 73 + frame * 7) % 100) / 100)
        py = int(card_y1 + 12 + (card_y2 - card_y1 - 24) * ((i * 37 + frame * 11) % 100) / 100)
        alpha = int(100 * (0.4 + 0.6 * abs(math.sin(t * 0.7 + i))))
        ch = chars[(i + frame) % len(chars)]
        draw.text((px, py), ch, fill=(ACCENT[0], ACCENT[1], ACCENT[2], min(255, alpha)), font=small_font)

    # ── Bottom status bar ───────────────────────────────────
    bar_h = 24
    draw.rectangle([0, H - bar_h, W, H], fill=SURFACE)
    draw.line([(0, H - bar_h), (W, H - bar_h)], fill=BORDER)
    status = "v0.1.0  ·  github.com/cpntodd/Mounter  ·  GPL-3.0"
    bbox = draw.textbbox((0, 0), status, font=small_font)
    sw = bbox[2] - bbox[0]
    draw.text(((W - sw)//2, H - bar_h + 6), status, fill=MUTED, font=small_font)

    frames.append(img)

# ── Save ────────────────────────────────────────────────────
output = os.path.join(OUT_DIR, "banner.gif")
frames[0].save(output, save_all=True, append_images=frames[1:],
               duration=DELAY, loop=0, optimize=False)
print(f"Banner saved: {output} ({len(frames)} frames)")
