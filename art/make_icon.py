"""Draws crammy.ico: two sheets side by side, engraved in ink on red, to sit next to PdfCraft."""

from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

BIG = 1024
SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]

RED = (183, 2, 40)
CREAM = (239, 233, 220)
INK = (11, 11, 12)
GREY_INK = (125, 122, 116)

LEFT_SHEET = (170, 196, 498, 830)
RIGHT_SHEET = (526, 196, 854, 830)
FOLD = 92   # size of the turned-down corner on the right sheet


def blank():
    return Image.new("L", (BIG, BIG), 0)


def hatch(clip, spacing, width, slope=1):
    """Parallel diagonal lines inside clip, like the shading on an engraving."""
    lines = blank()
    pen = ImageDraw.Draw(lines)
    for start in range(-BIG, 2 * BIG, spacing):
        pen.line((start, 0, start + slope * BIG, BIG), fill=255, width=width)
    return ImageChops.multiply(lines, clip)


def ink(image, mask, colour=INK):
    image.paste(Image.new("RGBA", (BIG, BIG), colour), (0, 0), mask)


def sheet_outline(box, folded):
    """The outline of a sheet, with the top-right corner turned down when folded."""
    left, top, right, bottom = box
    if not folded:
        return [(left, top), (right, top), (right, bottom), (left, bottom)]
    return [(left, top), (right - FOLD, top), (right, top + FOLD), (right, bottom), (left, bottom)]


def polygon_mask(points, offset=(0, 0)):
    mask = blank()
    ImageDraw.Draw(mask).polygon([(x + offset[0], y + offset[1]) for x, y in points], fill=255)
    return mask


def draw(detailed):
    image = Image.new("RGBA", (BIG, BIG), (0, 0, 0, 0))
    tile = blank()
    ImageDraw.Draw(tile).rounded_rectangle((28, 28, 996, 996), radius=150, fill=255)
    ink(image, tile, RED)

    sheets = [sheet_outline(LEFT_SHEET, False), sheet_outline(RIGHT_SHEET, True)]

    if detailed:
        for points in sheets:
            ink(image, hatch(polygon_mask(points, (26, 30)), 22, 7))

    for points in sheets:
        ink(image, polygon_mask(points), CREAM)
        ImageDraw.Draw(image).line(points + [points[0]], fill=INK, width=18 if detailed else 34, joint="curve")

    # The turned-down corner, shaded with cross-hatching
    right, top = RIGHT_SHEET[2], RIGHT_SHEET[1]
    flap = [(right - FOLD, top), (right - FOLD, top + FOLD), (right, top + FOLD)]
    if detailed:
        flap_mask = polygon_mask(flap)
        ink(image, ImageChops.lighter(hatch(flap_mask, 18, 5), hatch(flap_mask, 18, 5, slope=-1)))
    ImageDraw.Draw(image).line(flap + [flap[0]], fill=INK, width=18 if detailed else 34, joint="curve")

    if not detailed:
        return image

    pen = ImageDraw.Draw(image)
    for y, length in ((300, 230), (380, 230), (460, 230), (540, 230), (620, 150)):
        pen.line((228, y, 228 + length, y), fill=INK, width=20)
        pen.ellipse((222, y - 10, 238, y + 10), fill=INK)

    band = blank()
    ImageDraw.Draw(band).rectangle((572, 330, 808, 404), fill=255)
    ink(image, hatch(band, 14, 6))
    for y, length in ((470, 236), (540, 236), (610, 236), (680, 160)):
        pen.line((572, y, 572 + length, y), fill=GREY_INK, width=14)

    return image


def main():
    here = Path(__file__).parent
    detailed = draw(True)
    simple = draw(False)
    frames = [(simple if size <= 24 else detailed).resize((size, size), Image.LANCZOS) for size in SIZES]
    frames[-1].save(here.parent / "crammy.ico", sizes=[(s, s) for s in SIZES], append_images=frames[:-1])


if __name__ == "__main__":
    main()
