"""
generate_icon.py
----------------
Generates a minimal valid 16x16 .ico file so the resource compiler
never fails due to a missing icon.  Run once before the first build:

    python resources/generate_icon.py
"""
import struct, os

def make_ico(path: str) -> None:
    # BMP header fields for a 16x16, 32-bit icon inside an ICO container
    width = height = 16
    bpp   = 32
    row_bytes = width * 4                        # BGRA
    xor_size  = row_bytes * height               # colour mask
    and_size  = 0                                # omit AND mask (all opaque)
    img_size  = 40 + xor_size                   # BITMAPINFOHEADER + pixels

    # BITMAPINFOHEADER
    bih = struct.pack('<IiiHHIIiiII',
        40,            # biSize
        width,         # biWidth
        height * 2,    # biHeight (×2 for ICO convention)
        1,             # biPlanes
        bpp,           # biBitCount
        0,             # biCompression (BI_RGB)
        xor_size,      # biSizeImage
        0, 0, 0, 0)    # resolution / clr table

    # 16×16 pixels – a simple blue square (BGRA)
    pixels = b'\x88\x44\x00\xFF' * (width * height)   # blue-ish

    # ICO file header
    ico_header = struct.pack('<HHH', 0, 1, 1)  # reserved, type=1 (ICO), count=1

    # Directory entry
    img_offset = 6 + 16   # right after header + one dir entry
    dir_entry  = struct.pack('<BBBBHHII',
        width, height, 0, 0,    # w, h, colour count, reserved
        1, bpp,                  # planes, bit count
        img_size,                # size of image data
        img_offset)              # offset in file

    os.makedirs(os.path.dirname(path) or '.', exist_ok=True)
    with open(path, 'wb') as f:
        f.write(ico_header + dir_entry + bih + pixels)
    print(f"Generated {path}  ({os.path.getsize(path)} bytes)")

if __name__ == '__main__':
    make_ico(os.path.join(os.path.dirname(__file__), 'app.ico'))
