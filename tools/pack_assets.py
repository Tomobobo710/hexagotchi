#!/usr/bin/env python3
"""
pack_assets.py - Build assets.rres from the assets/ directory.

This script generates an rres file (https://github.com/raysan5/rres)
containing all PNG images from the assets/ directory.

Usage:
    python3 pack_assets.py              pack assets/ -> assets.rres
    python3 pack_assets.py <dir> <out>  pack <dir> -> <out>
"""

import struct
import sys
import os
from pathlib import Path


def compute_crc32(data: bytes) -> int:
    """Compute CRC32 checksum (matches rres' rresComputeCRC32)."""
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320
            else:
                crc >>= 1
    return crc ^ 0xFFFFFFFF


def load_png_pixels(filepath: str) -> tuple:
    """
    Load PNG image and extract raw pixel data.
    Returns (width, height, format, pixel_data).

    We convert to RGBA format (8 bits per channel) for compatibility with raylib.
    """
    from PIL import Image

    img = Image.open(filepath)

    # Convert to RGBA to ensure consistent format
    if img.mode != 'RGBA':
        img = img.convert('RGBA')

    width = img.width
    height = img.height
    pixel_format = 7  # RRES_PIXELFORMAT_UNCOMP_R8G8B8A8 (matches raylib PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)

    # Get raw pixel data (RGBA format)
    pixel_data = img.tobytes()

    return width, height, pixel_format, pixel_data


def build_image_chunk_buffer(width: int, height: int, pixel_format: int, raw_data: bytes) -> bytes:
    """
    Build the IMGE chunk data buffer.
    Format: propCount (4 bytes) + props[4] (16 bytes) + raw pixel data
    Total: 20 + raw_data_size
    """
    prop_count = 4
    props = struct.pack('<IIII', width, height, pixel_format, 1)  # 4 mipmap levels

    # propCount (4 bytes) + props[] (16 bytes) + raw pixel data
    return struct.pack('<I', prop_count) + props + raw_data


def find_pngs_recursive(root: str) -> list:
    """Find all PNG files recursively in root directory."""
    pngs = []
    for dirpath, dirnames, filenames in os.walk(root):
        for filename in filenames:
            if filename.lower().endswith('.png'):
                pngs.append(os.path.join(dirpath, filename))
    return sorted(pngs)


def to_relative_key(full_path: str, root: str) -> str:
    """Convert full path to relative key."""
    rel = os.path.relpath(full_path, root)
    return rel.replace(os.sep, '/')


def pack_assets(assets_dir: str, output_path: str):
    """Pack all PNG assets into an rres file."""

    # Find all PNG files
    pngs = find_pngs_recursive(assets_dir)
    if not pngs:
        print(f"No PNGs found under {assets_dir}")
        return False

    print(f"Found {len(pngs)} PNG files")

    # Open output file
    with open(output_path, 'wb') as rres_file:
        # Write file header (16 bytes)
        # id[4] = 'rres', version = 100, chunkCount, cdOffset = 0, reserved = 0
        header = struct.pack('<4sHHII', b'rres', 100, len(pngs), 0, 0)
        rres_file.write(header)

        # Track current position after header
        current_pos = 16

        # Process each PNG
        packed = 0
        skipped = 0
        for full_path in pngs:
            try:
                width, height, pixel_format, raw_data = load_png_pixels(full_path)
            except Exception as e:
                print(f"  Skipping (failed to load): {full_path} - {e}")
                skipped += 1
                continue

            # Calculate expected pixel data size
            # RGBA = 4 bytes per pixel
            expected_size = width * height * 4
            if len(raw_data) != expected_size:
                print(f"  Skipping (size mismatch): {full_path} - expected {expected_size}, got {len(raw_data)}")
                skipped += 1
                continue

            # Build the image chunk data buffer
            chunk_data = build_image_chunk_buffer(width, height, pixel_format, raw_data)
            buf_size = len(chunk_data)

            # Compute relative key for ID
            key = to_relative_key(full_path, assets_dir)

            # Compute CRC32 for ID
            chunk_id = compute_crc32(key.encode('utf-8'))

            # Compute CRC32 for chunk data (propCount + props + data)
            chunk_crc = compute_crc32(chunk_data)

            # Write chunk info (32 bytes)
            # type[4] = 'IMGE', id, compType = 0 (none), cipherType = 0 (none),
            # flags = 0, packedSize, baseSize, nextOffset = 0, reserved = 0, crc32
            chunk_info = struct.pack('<4sIBBHIIIII',
                b'IMGE',           # type
                chunk_id,           # id
                0,                  # compType (RRES_COMP_NONE)
                0,                  # cipherType (RRES_CIPHER_NONE)
                0,                  # flags
                buf_size,           # packedSize
                buf_size,           # baseSize
                0,                  # nextOffset
                0,                  # reserved
                chunk_crc           # crc32
            )

            rres_file.write(chunk_info)
            rres_file.write(chunk_data)

            packed += 1

        print(f"Wrote {output_path}: {packed} resources, {skipped} skipped")
        return packed > 0


def main():
    assets_dir = sys.argv[1] if len(sys.argv) > 1 else "assets"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "assets.rres"

    if not os.path.isdir(assets_dir):
        print(f"Error: {assets_dir} is not a directory")
        sys.exit(1)

    success = pack_assets(assets_dir, output_path)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
