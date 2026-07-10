#!/usr/bin/env python3
"""Dump a .glb's POSITION/NORMAL/COLOR_0 data as a triangle-grouped text file.

This is the general, reusable reader half of the Blender round-trip
described in export_glb.cpp: that tool writes a vertex-colored .glb, Tom
reshapes/repaints it in Blender, exports it back out, and this script turns
the returned .glb into a flat text dump that's actually usable for
classifying which triangles are which part (see memory: "the classification
step is not automatable in general", this just removes the "re-write a
byte parser every time" tax on top of that).

Handles both of the shapes a returning .glb can have:
  - Non-indexed triangle list (what export_glb.cpp itself writes): every 3
    consecutive vertices in POSITION are one triangle.
  - Indexed triangle list (what Blender's exporter normally produces): an
    "indices" accessor selects 3 vertices per triangle from the attribute
    arrays, and vertices are commonly split/duplicated at UV/normal seams.

Also handles per-material color (not just per-vertex COLOR_0): if a mesh has
no COLOR_0 attribute at all, this falls back to each primitive's assigned
material's pbrMetallicRoughness.baseColorFactor -- this is what you get when
Tom paints with Blender's Material/Face-assign workflow (separate material
slots + baseColorFactor) instead of vertex-paint. Blender splits a
materials-painted mesh into one primitive per material, so every triangle in
a given primitive shares that primitive's material color.

Usage:
    python tools/dump_glb.py path/to/file.glb [output.txt]

Output format, one block per triangle:
    TRI <index>
      v0  x y z  |  nx ny nz  |  r g b a
      v1  x y z  |  nx ny nz  |  r g b a
      v2  x y z  |  nx ny nz  |  r g b a
      color: r g b a  (mode: uniform|mixed)

"color" is the triangle's color if all 3 corners match (the common case --
this codebase's meshes are flat-colored per triangle), or a note that the
3 corners differ (rare, usually a seam Blender introduced) with all 3 shown
above for manual inspection.
"""
import json
import struct
import sys


def load_glb(path):
    with open(path, "rb") as f:
        data = f.read()

    magic, version, length = struct.unpack_from("<III", data, 0)
    if magic != 0x46546C67:
        raise ValueError(f"{path} is not a .glb (bad magic)")

    offset = 12
    json_chunk = None
    bin_chunk = None
    while offset < length:
        chunk_len, chunk_type = struct.unpack_from("<II", data, offset)
        chunk_data = data[offset + 8: offset + 8 + chunk_len]
        if chunk_type == 0x4E4F534A:  # "JSON"
            json_chunk = json.loads(chunk_data.decode("utf-8"))
        elif chunk_type == 0x004E4942:  # "BIN\0"
            bin_chunk = chunk_data
        offset += 8 + chunk_len

    if json_chunk is None or bin_chunk is None:
        raise ValueError(f"{path}: missing JSON or BIN chunk")
    return json_chunk, bin_chunk


COMPONENT_FORMATS = {
    5120: ("b", 1),  # BYTE
    5121: ("B", 1),  # UNSIGNED_BYTE
    5122: ("h", 2),  # SHORT
    5123: ("H", 2),  # UNSIGNED_SHORT
    5125: ("I", 4),  # UNSIGNED_INT
    5126: ("f", 4),  # FLOAT
}

TYPE_COUNTS = {
    "SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4,
    "MAT2": 4, "MAT3": 9, "MAT4": 16,
}


def read_accessor(gltf, bin_data, accessor_index):
    accessor = gltf["accessors"][accessor_index]
    view = gltf["bufferViews"][accessor["bufferView"]]
    view_offset = view.get("byteOffset", 0)
    accessor_offset = accessor.get("byteOffset", 0)
    start = view_offset + accessor_offset

    fmt_char, comp_size = COMPONENT_FORMATS[accessor["componentType"]]
    count_per_elem = TYPE_COUNTS[accessor["type"]]
    count = accessor["count"]
    normalized = accessor.get("normalized", False)

    stride = view.get("byteStride", comp_size * count_per_elem)

    values = []
    for i in range(count):
        elem_offset = start + i * stride
        elem = struct.unpack_from("<" + fmt_char * count_per_elem, bin_data, elem_offset)
        if normalized and fmt_char == "B":
            elem = tuple(v / 255.0 for v in elem)
        elif normalized and fmt_char == "H":
            elem = tuple(v / 65535.0 for v in elem)
        values.append(elem)
    return values


def to_byte_color(color_tuple, was_normalized):
    # COLOR_0 can be VEC3 or VEC4, float or normalized ubyte/ushort -- always
    # surface as 0-255 ints in the dump since that's what PushTri()/raylib
    # Color expects.
    vals = list(color_tuple)
    if len(vals) == 3:
        vals.append(1.0 if was_normalized or isinstance(vals[0], float) else 255)
    if all(isinstance(v, float) for v in vals):
        vals = [round(v * 255) for v in vals]
    return tuple(int(v) for v in vals)


def material_base_color(gltf, material_index):
    if material_index is None or "materials" not in gltf:
        return None
    material = gltf["materials"][material_index]
    pbr = material.get("pbrMetallicRoughness", {})
    factor = pbr.get("baseColorFactor", [1.0, 1.0, 1.0, 1.0])
    return tuple(round(c * 255) for c in factor), material.get("name", f"material[{material_index}]")


def dump_primitive(gltf, bin_data, prim, prim_index, tri_offset):
    attrs = prim["attributes"]
    positions = read_accessor(gltf, bin_data, attrs["POSITION"])
    normals = read_accessor(gltf, bin_data, attrs["NORMAL"]) if "NORMAL" in attrs else None

    material_color = material_base_color(gltf, prim.get("material"))

    colors = None
    if "COLOR_0" in attrs:
        color_accessor = gltf["accessors"][attrs["COLOR_0"]]
        was_normalized = color_accessor.get("normalized", False) or color_accessor["componentType"] == 5126
        colors_raw = read_accessor(gltf, bin_data, attrs["COLOR_0"])
        colors = [to_byte_color(c, was_normalized) for c in colors_raw]
    elif material_color is None:
        raise ValueError(
            f"primitive {prim_index}: no COLOR_0 attribute and no assigned material -- "
            "nothing to determine this primitive's color from"
        )

    if "indices" in prim:
        indices = read_accessor(gltf, bin_data, prim["indices"])
        indices = [i[0] for i in indices]
    else:
        indices = list(range(len(positions)))

    if len(indices) % 3 != 0:
        raise ValueError(f"primitive {prim_index}: index count {len(indices)} is not a multiple of 3")

    lines = []
    if material_color is not None:
        mc, mname = material_color
        lines.append(f"# primitive {prim_index}: material \"{mname}\" baseColorFactor -> {mc[0]} {mc[1]} {mc[2]} {mc[3]}")
    lines.append(f"# primitive {prim_index}: {len(positions)} vertices, {len(indices) // 3} triangles, indexed={'indices' in prim}")
    lines.append("")

    for tri_i in range(len(indices) // 3):
        vi = indices[tri_i * 3: tri_i * 3 + 3]
        lines.append(f"TRI {tri_offset + tri_i}  (primitive {prim_index})")
        corner_colors = []
        for corner, v in enumerate(vi):
            p = positions[v]
            n = normals[v] if normals else (0.0, 0.0, 0.0)
            c = colors[v] if colors is not None else material_color[0]
            corner_colors.append(c)
            lines.append(
                f"  v{corner}  {p[0]:.5f} {p[1]:.5f} {p[2]:.5f}  |  "
                f"{n[0]:.5f} {n[1]:.5f} {n[2]:.5f}  |  {c[0]} {c[1]} {c[2]} {c[3]}"
            )
        if colors is None:
            c = material_color[0]
            lines.append(f"  color: {c[0]} {c[1]} {c[2]} {c[3]}  (mode: material \"{material_color[1]}\")")
        elif corner_colors[0] == corner_colors[1] == corner_colors[2]:
            c = corner_colors[0]
            lines.append(f"  color: {c[0]} {c[1]} {c[2]} {c[3]}  (mode: uniform)")
        else:
            lines.append(f"  color: MIXED -- corners differ, see v0/v1/v2 colors above  (mode: mixed)")
        lines.append("")

    return lines, len(indices) // 3


def dump(path, out_path):
    gltf, bin_data = load_glb(path)
    mesh = gltf["meshes"][0]
    primitives = mesh["primitives"]

    lines = [f"# {path}", f"# {len(primitives)} primitive(s)", ""]
    total_tris = 0
    for prim_index, prim in enumerate(primitives):
        prim_lines, tri_count = dump_primitive(gltf, bin_data, prim, prim_index, total_tris)
        lines.extend(prim_lines)
        total_tris += tri_count

    text = "\n".join(lines)
    if out_path:
        with open(out_path, "w") as f:
            f.write(text)
        print(f"wrote {out_path} ({total_tris} triangles across {len(primitives)} primitive(s))")
    else:
        print(text)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    glb_path = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else glb_path.rsplit(".", 1)[0] + "_dump.txt"
    dump(glb_path, out)
