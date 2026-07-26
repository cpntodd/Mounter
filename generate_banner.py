#!/usr/bin/env python3
"""Generate Mounter banner GIF — clean, subtle animations."""

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
BG         = (30, 30, 30)
SURFACE    = (37, 37, 38)
ACCENT     = (240, 144, 64)
TEXT       = (212, 212, 212)
HEADING    = (255, 255, 255)
MUTED      = (157, 157, 157)
BORDER     = (62, 62, 66)

OUT_DIR = "assets"
os.makedirs(OUT_DIR, exist_ok=True)

def find_font(names, size):
    for name in names:
        for base in ["/usr/share/fonts", "/usr/local/share/fonts"]:
            for root, dirs, files in os.walk(base):
                for f in files:
                    if name.lower() in f.lower() and f.endswith(('.ttf', '.otf')):
                        return ImageFont.truetype(os.path.join(root, f), size)
    return ImageFont.load_default()

title_font = find_font(["DejaVuSans-Bold", "DejaVuSans"], 64)
mono_font  = find_font(["DejaVuSansMono", "DejaVuSans"], 17)
small_font = find_font(["DejaVuSansMono", "DejaVuSans"], 12)

frames = []
for frame in range(FRAMES):
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)
    t = frame / FRAMES

    # ── Gradient background ─────────────────────────────────
    for y in range(H):
        ratio = y / H
        r = int(10 + 6 * ratio)
        g = int(10 + 4 * ratio)
        b = int(12 + 6 * ratio)
        draw.line([(0, y), (W, y)], fill=(r, g, b))

    # ── Subtle grid ─────────────────────────────────────────
    for x in range(0, W, 50):
        draw.line([(x, 0), (x, H)], fill=(26, 26, 30), width=1)
    for y in range(0, H, 50):
        draw.line([(0, y), (W, y)], fill=(26, 26, 30), width=1)

    # ── Subtle pulsing glow behind title ────────────────────
    pulse = (math.sin(t * math.pi / 12) + 1) / 2
    for r in range(80, 0, -4):
        alpha = int(10 * (1 - r / 80) * (0.7 + 0.3 * pulse))
        if alpha > 0:
            draw.ellipse(
                [W//2 - r, H//2 - 48 - r//3, W//2 + r, H//2 - 48 + r//3],
                fill=(ACCENT[0], ACCENT[1], ACCENT[2], min(255, alpha))
            )

    # ── Surface card ────────────────────────────────────────
    card_margin = 80
    card_x1 = card_margin
    card_y1 = H//2 - 72
    card_x2 = W - card_margin
    card_y2 = H//2 + 40
    draw.rounded_rectangle(
        [card_x1, card_y1, card_x2, card_y2],
        radius=8, fill=SURFACE, outline=BORDER, width=1
    )

    # ── Terminal prefix ─────────────────────────────────────
    prefix = "cpntodd@mounter:~$"
    px, py = card_x1 + 24, card_y1 + 16
    draw.text((px, py), prefix, fill=ACCENT, font=small_font)

    # ── Title ───────────────────────────────────────────────
    title = "MOUNTER"
    bbox = draw.textbbox((0, 0), title, font=title_font)
    tw = bbox[2] - bbox[0]
    tx = (W - tw) // 2
    ty = H // 2 - 56
    draw.text((tx + 1, ty + 1), title, fill=(0, 0, 0), font=title_font)
    draw.text((tx, ty), title, fill=HEADING, font=title_font)

    # ── Subtitle ────────────────────────────────────────────
    subtitle = "SMB / CIFS Mount GUI for Debian Linux"
    bbox = draw.textbbox((0, 0), subtitle, font=mono_font)
    sw = bbox[2] - bbox[0]
    sx = (W - sw) // 2
    sy = H // 2 + 8
    draw.text((sx, sy), subtitle, fill=MUTED, font=mono_font)

    # ── Accent underline ────────────────────────────────────
    line_y = H // 2 + 34
    line_w = 180
    draw.line(
        [(W//2 - line_w//2, line_y), (W//2 + line_w//2, line_y)],
        fill=ACCENT, width=2
    )

    # ── Bottom status bar ───────────────────────────────────
    bar_h = 22
    draw.rectangle([0, H - bar_h, W, H], fill=SURFACE)
    draw.line([(0, H - bar_h), (W, H - bar_h)], fill=BORDER)
    status = "v0.1.0  ·  github.com/cpntodd/Mounter  ·  GPL-3.0"
    bbox = draw.textbbox((0, 0), status, font=small_font)
    sw = bbox[2] - bbox[0]
    draw.text(((W - sw)//2, H - bar_h + 5), status, fill=MUTED, font=small_font)

    frames.append(img)

output = os.path.join(OUT_DIR, "banner.gif")
frames[0].save(output, save_all=True, append_images=frames[1:],
               duration=DELAY, loop=0, optimize=False)
print(f"Banner saved: {output} ({len(frames)} frames)")
