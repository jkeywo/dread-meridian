"""Load exported assets and snap matching anchor/socket frames in Blender."""
import bpy
from mathutils import Matrix
from pathlib import Path

DIRECTORY=Path(__file__).resolve().parent

def import_asset(asset):
    before=set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=str(DIRECTORY/(asset+'.glb')))
    objects=set(bpy.data.objects)-before
    root=next(ob for ob in objects if ob.name==asset+'_root' or ob.name.startswith(asset+'_root.'))
    nodes={ob.get('attachment_id',ob.name):ob for ob in objects if ob.type=='EMPTY'}
    return {'root':root,'nodes':nodes,'objects':objects}

def snap(character,item,socket_name,anchor_name):
    bpy.context.view_layer.update()
    socket=character['nodes'][socket_name]
    anchor=item['nodes'][anchor_name]
    root=item['root']
    anchor_in_root=root.matrix_world.inverted() @ anchor.matrix_world
    root.parent=socket
    root.matrix_parent_inverse=Matrix.Identity(4)
    root.matrix_basis=anchor_in_root.inverted()
    bpy.context.view_layer.update()
    difference=socket.matrix_world.inverted() @ anchor.matrix_world
    error=max(abs(difference[r][col]-(1 if r==col else 0)) for r in range(4) for col in range(4))
    assert error<1e-5,(socket_name,anchor_name,error)
    return error

def set_visible(item,visible):
    for ob in item['objects']:
        ob.hide_render=not visible
        ob.hide_set(not visible)
