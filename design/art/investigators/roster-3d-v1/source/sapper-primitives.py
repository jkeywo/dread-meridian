"""Rebuild the provisional Sapper concept model in Blender 5.0.

Run: blender --background --factory-startup --python build_sapper.py
The source illustration is a visual reference, not a texture or flat billboard.
"""
import bpy
import math
import random
from pathlib import Path
from mathutils import Vector

OUT = Path(__file__).resolve().parent
random.seed(710)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
character = bpy.data.collections.new('Sapper | character meshes')
scene.collection.children.link(character)
stage = bpy.data.collections.new('Presentation | not exported')
scene.collection.children.link(stage)

def move(obj, collection=character):
    for col in list(obj.users_collection):
        col.objects.unlink(obj)
    collection.objects.link(obj)
    return obj

def material(name, color, roughness=.65, metallic=0, bump=0):
    mat = bpy.data.materials.new(name)
    color=tuple(c*.60 for c in color) if name not in ['Studio floor','Pupils and mouth crease'] else color
    mat.diffuse_color = (*color, 1)
    mat.use_nodes = True
    nt = mat.node_tree
    bs = nt.nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value = (*color, 1)
    bs.inputs['Roughness'].default_value = roughness
    bs.inputs['Metallic'].default_value = metallic
    if bump:
        noise = nt.nodes.new('ShaderNodeTexNoise')
        noise.inputs['Scale'].default_value = 155
        noise.inputs['Detail'].default_value = 3
        node = nt.nodes.new('ShaderNodeBump')
        node.inputs['Strength'].default_value = .23
        node.inputs['Distance'].default_value = bump
        nt.links.new(noise.outputs['Fac'], node.inputs['Height'])
        nt.links.new(node.outputs['Normal'], bs.inputs['Normal'])
    return mat

coat = material('Worn olive wool', (.19,.205,.115), bump=.012)
edge = material('Raised wool seams', (.27,.28,.16), bump=.008)
darkcloth = material('Coat shadow and trousers', (.105,.125,.079), bump=.01)
scarf = material('Faded khaki scarf', (.29,.285,.18), bump=.007)
leather = material('Oiled brown leather', (.16,.073,.035), .48, bump=.007)
leatheredge = material('Leather edge wear', (.31,.177,.087), .6)
bootmat = material('Weathered boots', (.105,.071,.047), .58, bump=.009)
sole = material('Dark soles', (.042,.036,.025), .86)
skin = material('Weathered skin', (.49,.285,.165), .65, bump=.003)
skinlight = material('Knuckles and scar', (.57,.345,.215), .68)
lip = material('Muted lips and ear recess', (.30,.135,.087), .75)
hair = material('Iron grey hair', (.135,.15,.14), .85)
hairlight = material('Grey hair streaks', (.28,.30,.275), .85)
eye = material('Warm grey sclera', (.55,.535,.425), .43)
iris = material('Hazel iris', (.17,.13,.057), .4)
pupil = material('Pupils and mouth crease', (.022,.019,.013), .65)
metal = material('Worn gunmetal', (.075,.091,.097), .38,.8)
steel = material('Exposed steel edges', (.23,.27,.27), .32,.8)
brass = material('Oxidised brass', (.34,.245,.10), .45,.75)
wood = material('Carbine walnut', (.205,.089,.036), .49, bump=.005)

def finish(obj, name, mat, smooth=True):
    obj.name = name
    move(obj)
    if mat:
        obj.data.materials.append(mat)
    if smooth and obj.type == 'MESH':
        for p in obj.data.polygons:
            p.use_smooth = True
    return obj

def ell(name, loc, scale, mat, seg=24, rings=16):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=rings, location=loc)
    obj=finish(bpy.context.object,name,mat)
    obj.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return obj

def box(name, loc, scale, mat, bevel=.01, rotation=None):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    obj=finish(bpy.context.object,name,mat,False)
    obj.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if rotation:
        obj.rotation_euler=rotation
    if bevel:
        mod=obj.modifiers.new('Soft worn edges','BEVEL'); mod.width=bevel; mod.segments=3
        obj.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
    return obj

def tube(name, pts, radius, mat, res=3):
    cu=bpy.data.curves.new(name,'CURVE'); cu.dimensions='3D'; cu.resolution_u=10
    sp=cu.splines.new('BEZIER'); sp.bezier_points.add(len(pts)-1)
    for bp,co in zip(sp.bezier_points,pts):
        bp.co=co; bp.handle_left_type='AUTO'; bp.handle_right_type='AUTO'
    cu.bevel_depth=radius; cu.bevel_resolution=res; cu.use_fill_caps=True
    ob=bpy.data.objects.new(name,cu); character.objects.link(ob); cu.materials.append(mat)
    return ob

def bone(name, a, b, r1, r2, mat, vertices=20):
    a,b=Vector(a),Vector(b)
    bpy.ops.mesh.primitive_cone_add(vertices=vertices,radius1=r1,radius2=r2,depth=(b-a).length,location=(a+b)/2)
    ob=finish(bpy.context.object,name,mat)
    ob.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler()
    bevel=ob.modifiers.new('Soft edge','BEVEL'); bevel.width=min(r1,r2)*.23; bevel.segments=3
    return ob

def patch(name, verts, mat, thickness=.007, bevel=.006):
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],[list(range(len(verts)))]); mesh.update()
    ob=bpy.data.objects.new(name,mesh); character.objects.link(ob); mesh.materials.append(mat)
    sol=ob.modifiers.new('Material thickness','SOLIDIFY'); sol.thickness=thickness
    bev=ob.modifiers.new('Soft tailored edge','BEVEL'); bev.width=bevel; bev.segments=3
    return ob

def ribbon(name, pts, width, mat):
    verts=[]
    for x,y,z in pts:
        verts.extend([(x-width/2,y,z),(x+width/2,y,z)])
    faces=[(i*2,i*2+1,i*2+3,i*2+2) for i in range(len(pts)-1)]
    me=bpy.data.meshes.new(name); me.from_pydata(verts,[],faces); me.update()
    ob=bpy.data.objects.new(name,me); character.objects.link(ob); me.materials.append(mat)
    mod=ob.modifiers.new('Leather thickness','SOLIDIFY'); mod.thickness=.009
    mod=ob.modifiers.new('Rounded leather','BEVEL'); mod.width=.004; mod.segments=2
    return ob

def buckle(name,x,y,z,w=.044,h=.054):
    tube(name,[(x-w/2,y,z-h/2),(x-w/2,y,z+h/2),(x+w/2,y,z+h/2),(x+w/2,y,z-h/2),(x-w/2,y,z-h/2)],.004,brass,2)
    tube(name+' pin',[(x,y-.003,z-h*.35),(x,y-.003,z+h*.4)],.0026,brass,2)


