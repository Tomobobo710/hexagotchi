"""Remove white-matte fringing from cutout PNGs with semi-transparent edges.

Usage: python tools/defringe.py <in.png> [out.png]

Assumes the source was cut out against a white background, so edge pixels
are blended toward white in proportion to their transparency (color = true*a
+ white*(1-a)). Un-premultiplies against white to recover the true edge
color, then writes the same alpha back out.
"""
import sys
from PIL import Image

def defringe(path_in, path_out, matte=(255, 255, 255)):
    im = Image.open(path_in).convert("RGBA")
    px = im.load()
    w, h = im.size
    mr, mg, mb = matte
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a == 0:
                px[x, y] = (0, 0, 0, 0)
                continue
            if a == 255:
                continue
            af = a / 255.0
            # Unpremultiply: observed = true*af + matte*(1-af) -> true = (observed - matte*(1-af)) / af
            nr = (r - mr * (1 - af)) / af
            ng = (g - mg * (1 - af)) / af
            nb = (b - mb * (1 - af)) / af
            clamp = lambda v: max(0, min(255, int(round(v))))
            px[x, y] = (clamp(nr), clamp(ng), clamp(nb), a)
    im.save(path_out)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    src = sys.argv[1]
    dst = sys.argv[2] if len(sys.argv) > 2 else src
    defringe(src, dst)
    print(f"defringed: {src} -> {dst}")
