#!/usr/bin/env python3
"""Generate an animated banner GIF for Mounter — cpntodd brand style."""

import subprocess, sys, os, math, struct, zlib

W, H = 1200, 400
FRAMES = 24
DELAY = 10  # 100ms

# Check if Pillow is available
try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Pillow not installed. Install with: pip install Pillow")
    print("Generating static fallback PNG instead...")
    has_pil = False
else:
    has_pil = True

OUT_DIR = "assets"
os.makedirs(OUT_DIR, exist_ok=True)

# ── cpntodd brand colors ────────────────────────────────────
BG         = (30, 30, 30)
ACCENT     = (240, 144, 64)
ACCENT_LIGHT = (245, 166, 35)
ACCENT_DARK  = (204, 119, 34)
TEXT       = (212, 212, 212)
HEADING    = (255, 255, 255)
MUTED      = (157, 157, 157)
BORDER     = (62, 62, 66)

if has_pil:
    frames = []
    for frame in range(FRAMES):
        img = Image.new("RGB", (W, H), BG)
        draw = ImageDraw.Draw(img)

        # Gradient background
        for y in range(H):
            t = y / H
            r = int(BG[0] * (1 - t) + 10 * t)
            g = int(BG[1] * (1 - t) + 10 * t)
            b = int(BG[2] * (1 - t) + 12 * t)
            draw.line([(0, y), (W, y)], fill=(r, g, b))

        # Grid pattern
        for x in range(0, W, 40):
            draw.line([(x, 0), (x, H)], fill=(40, 40, 44), width=1)
        for y in range(0, H, 40):
            draw.line([(0, y), (W, y)], fill=(40, 40, 44), width=1)

        # Pulsing glow at center
        pulse = (math.sin(frame * math.pi / 12) + 1) / 2  # 0..1
        glow_r = int(ACCENT[0] * 0.15 * (0.5 + pulse * 0.5))
        for r in range(120, 0, -2):
            alpha = int(glow_r * (1 - r / 120))
            if alpha > 0:
                draw.ellipse(
                    [W//2 - r, H//2 - r//3, W//2 + r, H//2 + r//3],
                    fill=(ACCENT[0], ACCENT[1], ACCENT[2], alpha),
                    outline=None
                )

        # Title text
        try:
            title_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 72)
        except:
            title_font = ImageFont.load_default()

        title = "MOUNTER"
        bbox = draw.textbbox((0, 0), title, font=title_font)
        tw = bbox[2] - bbox[0]
        tx = (W - tw) // 2
        ty = H // 2 - 60

        # Text shadow
        draw.text((tx + 2, ty + 2), title, fill=(0, 0, 0), font=title_font)
        draw.text((tx, ty), title, fill=ACCENT, font=title_font)

        # Subtitle
        try:
            sub_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 22)
        except:
            sub_font = ImageFont.load_default()

        subtitle = "SMB/CIFS Mount GUI for Linux"
        bbox = draw.textbbox((0, 0), subtitle, font=sub_font)
        sw = bbox[2] - bbox[0]
        sx = (W - sw) // 2
        sy = H // 2 + 20
        draw.text((sx, sy), subtitle, fill=MUTED, font=sub_font)

        # Floating particles
        for i in range(12):
            px = int(W * 0.2 + W * 0.6 * ((i * 73 + frame * 13) % 100) / 100)
            py = int(H * 0.1 + H * 0.8 * ((i * 37 + frame * 7) % 100) / 100)
            alpha = int(128 * (0.5 + 0.5 * math.sin(frame * 0.3 + i)))
            size = 2 + (i % 3)
            draw.ellipse([px, py, px + size, py + size],
                        fill=(ACCENT[0], ACCENT[1], ACCENT[2], min(255, alpha)))

        # Bottom accent line
        draw.line([(W//4, H - 20), (3*W//4, H - 20)], fill=ACCENT, width=2)

        frames.append(img)

    # Save animated GIF
    output_path = os.path.join(OUT_DIR, "banner.gif")
    frames[0].save(
        output_path, save_all=True, append_images=frames[1:],
        duration=DELAY, loop=0, optimize=False
    )
    print(f"Banner saved to {output_path} ({len(frames)} frames)")

else:
    # Fallback: generate a simple PNG
    output_path = os.path.join(OUT_DIR, "banner.png")
    # Use ImageMagick if available
    try:
        subprocess.run([
            "convert", "-size", f"{W}x{H}", "xc:#1e1e1e",
            "-fill", "#252526", "-draw", "rectangle 0,0 1200,400",
            "-font", "DejaVu-Sans-Bold", "-pointsize", "72",
            "-fill", "#f09040", "-gravity", "center",
            "-annotate", "+0-30", "MOUNTER",
            "-font", "DejaVu-Sans", "-pointsize", "22",
            "-fill", "#9d9d9d",
            "-annotate", "+0+30", "SMB/CIFS Mount GUI for Linux",
            "-fill", "#f09040", "-draw", "line 300,380 900,380",
            output_path
        ], check=True)
        print(f"Banner saved to {output_path}")
    except:
        print("Install ImageMagick or Pillow to generate banner: apt install imagemagick")
        print("Skipping banner generation.")

