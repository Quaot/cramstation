"""Draws cramstation.ico: notes on the left, a highlighted page on the right."""

from pathlib import Path

from PIL import Image, ImageDraw

BACKGROUND = (43, 58, 85)
PAPER = (255, 255, 255)
INK = (43, 58, 85)
HIGHLIGHT = (232, 185, 49)

BIG = 1024
SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]


def draw(detailed):
    image = Image.new("RGBA", (BIG, BIG), (0, 0, 0, 0))
    pen = ImageDraw.Draw(image)
    pen.rounded_rectangle((32, 32, 992, 992), radius=200, fill=BACKGROUND)

    left = (150, 170, 494, 854)
    right = (530, 170, 874, 854)
    pen.rounded_rectangle(left, radius=48, fill=PAPER)
    pen.rounded_rectangle(right, radius=48, fill=PAPER)

    if detailed:
        for y in (290, 400, 510, 620):
            width = 210 if y != 620 else 130
            pen.rounded_rectangle((214, y, 214 + width, y + 44), radius=22, fill=INK)
        pen.rounded_rectangle((590, 280, 814, 360), radius=20, fill=HIGHLIGHT)
        for y in (430, 530, 630):
            pen.rounded_rectangle((594, y, 810, y + 40), radius=20, fill=(190, 198, 212))
    else:
        pen.rounded_rectangle((560, 230, 844, 400), radius=36, fill=HIGHLIGHT)

    return image


def main():
    here = Path(__file__).parent
    detailed = draw(True)
    simple = draw(False)
    frames = [(simple if size <= 24 else detailed).resize((size, size), Image.LANCZOS) for size in SIZES]
    frames[-1].save(here.parent / "cramstation.ico", sizes=[(s, s) for s in SIZES], append_images=frames[:-1])


if __name__ == "__main__":
    main()
