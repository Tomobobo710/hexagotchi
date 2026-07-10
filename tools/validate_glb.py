#!/usr/bin/env python3
"""Headlessly check a .glb for geometry problems before baking it into a mesh.

This exists so "does this exported mesh actually work" doesn't require
opening the game or Blender -- it reuses dump_glb.py's parsing (same
COLOR_0/material-color handling) and flags the kinds of problems that cause
symptoms like a triangle rendering black/missing/inside-out:
  - Zero-area (degenerate) triangles: two or more corners coincide, or all
    three corners are collinear -- these contribute a zero-length normal and
    can z-fight or vanish depending on the renderer.
  - NaN/Inf in any position or normal component.
  - Zero-length normals (either exported degenerate, or a normal of exactly
    (0,0,0) some exporters emit for a broken triangle).
  - Out-of-range indices (an index referencing a vertex past the end of the
    POSITION accessor -- exporter bug or truncated buffer).
  - Triangles with an absurd edge-length ratio (a sliver -- not necessarily
    wrong, but worth flagging since a hand-built mesh usually shouldn't have
    them; reported separately from hard errors, doesn't fail validation).

Usage:
    python tools/validate_glb.py path/to/file.glb
Exit code 0 if clean, 1 if any hard problems were found (prints a report
either way).
"""
import math
import sys

from dump_glb import load_glb, read_accessor, material_base_color

EPS_AREA = 1e-9
EPS_COINCIDENT = 1e-6


def is_finite3(v):
    return all(math.isfinite(c) for c in v)


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def length(v):
    return math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])


def dist(a, b):
    return length(sub(a, b))


def validate_primitive(gltf, bin_data, prim, prim_index, tri_offset, errors, warnings):
    attrs = prim["attributes"]
    positions = read_accessor(gltf, bin_data, attrs["POSITION"])
    normals = read_accessor(gltf, bin_data, attrs["NORMAL"]) if "NORMAL" in attrs else None
    material_color = material_base_color(gltf, prim.get("material"))
    has_color_source = "COLOR_0" in attrs or material_color is not None

    if not has_color_source:
        warnings.append(f"primitive {prim_index}: no COLOR_0 and no material -- color will be undefined (WHITE)")

    vcount = len(positions)
    if "indices" in prim:
        indices = read_accessor(gltf, bin_data, prim["indices"])
        indices = [i[0] for i in indices]
    else:
        indices = list(range(vcount))

    if len(indices) % 3 != 0:
        errors.append(f"primitive {prim_index}: index count {len(indices)} not a multiple of 3")
        return len(indices) // 3

    for tri_i in range(len(indices) // 3):
        tri_id = tri_offset + tri_i
        vi = indices[tri_i * 3: tri_i * 3 + 3]

        for v in vi:
            if v < 0 or v >= vcount:
                errors.append(f"TRI {tri_id} (primitive {prim_index}): index {v} out of range (vertex count {vcount})")
                continue

        if any(v < 0 or v >= vcount for v in vi):
            continue  # can't inspect geometry for an out-of-range triangle

        a, b, c = positions[vi[0]], positions[vi[1]], positions[vi[2]]

        if not (is_finite3(a) and is_finite3(b) and is_finite3(c)):
            errors.append(f"TRI {tri_id} (primitive {prim_index}): NaN/Inf in vertex position")
            continue

        # Coincident corners (two verts at the same point, within tolerance).
        d_ab, d_bc, d_ca = dist(a, b), dist(b, c), dist(c, a)
        if min(d_ab, d_bc, d_ca) < EPS_COINCIDENT:
            errors.append(
                f"TRI {tri_id} (primitive {prim_index}): degenerate -- two corners coincide "
                f"(edge lengths {d_ab:.6f}, {d_bc:.6f}, {d_ca:.6f})"
            )
            continue

        # Zero-area check via cross product magnitude.
        cr = cross(sub(b, a), sub(c, a))
        area2 = length(cr)
        if area2 < EPS_AREA:
            errors.append(f"TRI {tri_id} (primitive {prim_index}): zero-area (collinear corners)")
            continue

        # Sliver warning: longest edge much bigger than the triangle's altitude.
        longest_edge = max(d_ab, d_bc, d_ca)
        altitude = area2 / longest_edge if longest_edge > 0 else 0.0
        if longest_edge > 0 and altitude / longest_edge < 0.01:
            warnings.append(
                f"TRI {tri_id} (primitive {prim_index}): sliver triangle "
                f"(longest edge {longest_edge:.5f}, altitude {altitude:.7f})"
            )

        if normals:
            for corner, v in enumerate(vi):
                n = normals[v]
                if not is_finite3(n):
                    errors.append(f"TRI {tri_id} (primitive {prim_index}) v{corner}: NaN/Inf normal")
                elif length(n) < 1e-4:
                    errors.append(f"TRI {tri_id} (primitive {prim_index}) v{corner}: zero-length normal")

    return len(indices) // 3


def validate(path):
    gltf, bin_data = load_glb(path)
    if "meshes" not in gltf or not gltf["meshes"]:
        print(f"ERROR: {path} has no meshes")
        return 1

    errors = []
    warnings = []
    total_tris = 0
    for mesh in gltf["meshes"]:
        for prim_index, prim in enumerate(mesh["primitives"]):
            if "POSITION" not in prim.get("attributes", {}):
                errors.append(f"primitive {prim_index}: missing POSITION attribute")
                continue
            total_tris += validate_primitive(gltf, bin_data, prim, prim_index, total_tris, errors, warnings)

    print(f"{path}: {total_tris} triangle(s) checked")
    if warnings:
        print(f"\n{len(warnings)} warning(s):")
        for w in warnings:
            print(f"  WARN  {w}")
    if errors:
        print(f"\n{len(errors)} error(s):")
        for e in errors:
            print(f"  ERROR  {e}")
        return 1

    print("no hard errors found")
    return 0


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    sys.exit(validate(sys.argv[1]))
