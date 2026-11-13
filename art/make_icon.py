"""Draws cramstation.ico: notes on the left, a highlighted page on the right."""

from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

BIG = 1024
SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]

BACKGROUND_TOP = (77, 141, 255)
BACKGROUND_BOTTOM = (88, 64, 214)
HIGHLIGHT_TOP = (255, 214, 92)
HIGHLIGHT_BOTTOM = (255, 160, 46)
INK = (70, 82, 160)
FAINT = (196, 204, 228)


def gradient(box_size, top, bottom):
    """A top-left to bottom-right blend between two colours."""
    down = Image.linear_gradient("L").resize( box_size )
    across = down.rotate( 90 ).transpose( Image.FLIP_LEFT_RIGHT )
    mask = ImageChops.add( down, across, scale=2 )
    return Image.composite( Image.new( "RGBA", box_size, bottom ), Image.new( "RGBA", box_size, top ), mask )


def shape(box, radius):
    mask = Image.new("L", (BIG, BIG), 0)
    ImageDraw.Draw(mask).rounded_rectangle(box, radius=radius, fill=255)
    return mask


def paint(image, mask, fill):
    layer = fill if isinstance(fill, Image.Image) else Image.new("RGBA", (BIG, BIG), fill)
    image.paste(layer, (0, 0), mask)


def shadow(image, mask, offset, blur, strength):
    soft = mask.filter(ImageFilter.GaussianBlur(blur)).point(lambda value: int(value * strength))
    moved = Image.new("L", (BIG, BIG), 0)
    moved.paste(soft, offset)
    image.paste(Image.new("RGBA", (BIG, BIG), (20, 18, 70)), (0, 0), moved)


def draw(detailed):
    image = Image.new("RGBA", (BIG, BIG), (0, 0, 0, 0))

    tile = shape((40, 40, 984, 984), 220)
    paint(image, tile, gradient((BIG, BIG), BACKGROUND_TOP, BACKGROUND_BOTTOM))

    # Soft light across the top of the tile
    sheen = Image.linear_gradient("L").resize((BIG, BIG)).point(lambda value: int(max(0, 150 - value) * 0.35))
    image.paste(Image.new("RGBA", (BIG, BIG), (255, 255, 255)), (0, 0), ImageChops.multiply(sheen, tile))

    left = shape((160, 190, 498, 834), 56)
    right = shape((526, 190, 864, 834), 56)
    for pane in (left, right):
        shadow(image, pane, (0, 26), 34, 0.45)
    paint(image, left, (255, 255, 255))
    paint(image, right, (255, 255, 255))

    if detailed:
        for y, width in ((300, 200), (400, 200), (500, 200), (600, 120)):
            paint(image, shape((226, y, 226 + width, y + 40), 20), INK)
        paint(image, shape((588, 288, 802, 368), 22), gradient((BIG, BIG), HIGHLIGHT_TOP, HIGHLIGHT_BOTTOM))
        for y in (430, 520, 610):
            paint(image, shape((592, y, 798, y + 36), 18), FAINT)
    else:
        paint(image, shape((566, 250, 824, 420), 40), gradient((BIG, BIG), HIGHLIGHT_TOP, HIGHLIGHT_BOTTOM))

    return image


def main():
    here = Path(__file__).parent
    detailed = draw(True)
    simple = draw(False)
    frames = [(simple if size <= 24 else detailed).resize((size, size), Image.LANCZOS) for size in SIZES]
    frames[-1].save(here.parent / "cramstation.ico", sizes=[(s, s) for s in SIZES], append_images=frames[:-1])


if __name__ == "__main__":
    main()
