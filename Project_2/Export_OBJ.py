import bpy
import bmesh

def export_custom_obj(filepath)
    obj = bpy.context.active_object
    mesh = obj.to_mesh()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces[])
    bm.verts.ensure_lookup_table()

    uv_layer = bm.loops.layers.uv.active
    normal_layer = bm.loops.layers.float_vector.get(normal)

    v_list = []
    vt_list = []
    vn_list = []
    face_indices = []

    v_map = {}
    vt_map = {}
    vn_map = {}

    def get_index(d, lst, val)
        if val in d
            return d[val]
        idx = len(lst) + 1
        d[val] = idx
        lst.append(val)
        return idx

    for face in bm.faces
        face_line = []
        for loop in face.loops
            v = loop.vert.co
            vt = loop[uv_layer].uv if uv_layer else (0.0, 0.0)
            vn = loop.vert.normal.normalized()

            vi = get_index(v_map, v_list, tuple(v))
            vti = get_index(vt_map, vt_list, tuple(vt))
            vni = get_index(vn_map, vn_list, tuple(vn))

            face_line.append((vi, vti, vni))
        face_indices.append(face_line)

    with open(filepath, 'w') as f
        f.write(#Custom OBJ Exportnn)

        f.write(#verticesn# format 'v' x y zn)
        for v in v_list
            f.write(fv {v[0].6f} {v[1].6f} {v[2].6f}n)

        f.write(n#UV coordinatesn# format 'vt' u vn)
        for vt in vt_list
            f.write(fvt {vt[0].6f} {vt[1].6f}n)

        f.write(n#normalsn# format 'vn' x y zn# should be normalizedn)
        for vn in vn_list
            f.write(fvn {vn[0].6f} {vn[1].6f} {vn[2].6f}n)

        f.write(n#faces (index data)n# format vertexUvcoordinatenormaln)
        for face in face_indices
            f.write(f  +  .join(f{vi}{vti}{vni} for vi, vti, vni in face) + n)

    bm.free()
    print(f✅ Exported to {filepath})

# Change path as needed
export_custom_obj(CUsersypersOneDriveDocumentsCOMP371COMP371saturn_ring.obj)
