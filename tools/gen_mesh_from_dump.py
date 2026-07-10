#!/usr/bin/env python3
"""Generate a Build<Name>Mesh() .hpp/.cpp pair from a dump_glb.py dump.

This is the last step of the Blender round-trip (see blender-mesh-pipeline
memory): once a dump's triangles are already colored (either real per-vertex
COLOR_0, or a per-primitive material color from dump_glb.py's materials
fallback), there's nothing left to *classify* -- every triangle already
carries its final color -- so this mechanically emits one PushTri() call per
triangle instead of a hand-transcribed literal array. Matches this codebase's
established convention (see JetMesh.cpp/VehicleMesh.cpp): a Build<Name>Mesh()
function made of PushTri() calls, not a baked flat data blob.

Usage:
    python tools/gen_mesh_from_dump.py <dump.txt> <MeshName> [out_dir]

Writes <out_dir>/<MeshName>Mesh.hpp and .cpp (out_dir defaults to src/effects).
"""
import re
import sys
import os

TRI_RE = re.compile(r"^TRI (\d+)")
VERT_RE = re.compile(
    r"^\s*v(\d)\s+([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s+\|\s+([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s+\|\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)"
)


def parse_dump(path):
    triangles = []
    current = None
    with open(path) as f:
        for line in f:
            m = TRI_RE.match(line)
            if m:
                if current:
                    triangles.append(current)
                current = []
                continue
            m = VERT_RE.match(line)
            if m and current is not None:
                _, x, y, z, nx, ny, nz, r, g, b, a = m.groups()
                current.append({
                    "pos": (float(x), float(y), float(z)),
                    "color": (int(r), int(g), int(b), int(a)),
                })
    if current:
        triangles.append(current)
    return triangles


def emit(mesh_name, triangles, out_dir):
    hpp_name = f"{mesh_name}Mesh.hpp"
    cpp_name = f"{mesh_name}Mesh.cpp"
    guard = f"{mesh_name.upper()}_MESH_HPP"

    hpp = f"""#ifndef {guard}
#define {guard}

#include "raylib.h"

// Hand-built mesh baked from a Blender round-trip (see blender-mesh-pipeline
// memory / export_glb.cpp + tools/dump_glb.py) -- one PushTri() call per
// triangle, generated mechanically from the exported .glb's already-resolved
// per-triangle colors (vertex-paint COLOR_0 or per-material baseColorFactor,
// whichever the source file used), not hand-classified into named parts.
Mesh Build{mesh_name}Mesh();

#endif // {guard}
"""

    lines = []
    lines.append(f'#include "{hpp_name}"')
    lines.append('#include "MeshBuilders.hpp"')
    lines.append("#include <cstring>")
    lines.append("#include <vector>")
    lines.append("")
    lines.append(f"Mesh Build{mesh_name}Mesh() {{")
    lines.append("    std::vector<float> verts, normals, uvs;")
    lines.append("    std::vector<unsigned char> colors;")
    lines.append("")
    for tri in triangles:
        if len(tri) != 3:
            continue
        a, b, c = tri
        col = a["color"]
        ax, ay, az = a["pos"]
        bx, by, bz = b["pos"]
        cx, cy, cz = c["pos"]
        lines.append(
            f"    PushTri(verts, normals, uvs, colors, "
            f"{{{ax:.5f}f, {ay:.5f}f, {az:.5f}f}}, "
            f"{{{bx:.5f}f, {by:.5f}f, {bz:.5f}f}}, "
            f"{{{cx:.5f}f, {cy:.5f}f, {cz:.5f}f}}, "
            f"Color{{{col[0]}, {col[1]}, {col[2]}, {col[3]}}});"
        )
    lines.append("")
    lines.append("    Mesh mesh = {};")
    lines.append("    mesh.triangleCount = (int)(verts.size() / 9);")
    lines.append("    mesh.vertexCount = mesh.triangleCount * 3;")
    lines.append("    mesh.vertices = (float*)RL_MALLOC(verts.size() * sizeof(float));")
    lines.append("    mesh.normals = (float*)RL_MALLOC(normals.size() * sizeof(float));")
    lines.append("    mesh.texcoords = (float*)RL_MALLOC(uvs.size() * sizeof(float));")
    lines.append("    mesh.colors = (unsigned char*)RL_MALLOC(colors.size() * sizeof(unsigned char));")
    lines.append("    memcpy(mesh.vertices, verts.data(), verts.size() * sizeof(float));")
    lines.append("    memcpy(mesh.normals, normals.data(), normals.size() * sizeof(float));")
    lines.append("    memcpy(mesh.texcoords, uvs.data(), uvs.size() * sizeof(float));")
    lines.append("    memcpy(mesh.colors, colors.data(), colors.size() * sizeof(unsigned char));")
    lines.append("    UploadMesh(&mesh, false);")
    lines.append("    return mesh;")
    lines.append("}")
    lines.append("")
    cpp = "\n".join(lines)

    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, hpp_name), "w") as f:
        f.write(hpp)
    with open(os.path.join(out_dir, cpp_name), "w") as f:
        f.write(cpp)
    print(f"wrote {out_dir}/{hpp_name} and {cpp_name} ({len(triangles)} triangles)")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    dump_path = sys.argv[1]
    mesh_name = sys.argv[2]
    out_dir = sys.argv[3] if len(sys.argv) > 3 else "src/effects"
    tris = parse_dump(dump_path)
    emit(mesh_name, tris, out_dir)
