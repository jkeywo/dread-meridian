import bpy
from pathlib import Path
from mathutils import Vector
P=Path(__file__).resolve().parents[1];out=P/'Saved/SmugglerAnimationProbe'
for name in ['KB_Idle_1','KB_p_Jab_R_1','KB_Superpunch']:
    bpy.ops.wm.open_mainfile(filepath=str(P/'design/art/supplied-characters-v1/smuggler-rigged.blend'))
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(out/(name+'.fbx')),automatic_bone_orientation=False)
    imported=list(set(bpy.data.objects)-before);other=next(o for o in imported if o.type=='ARMATURE');action=other.animation_data.action
    slot=other.animation_data.action_slot
    rig.animation_data.action=action;rig.animation_data.action_slot=slot
    for ob in imported:bpy.data.objects.remove(ob,do_unlink=True)
    scene=bpy.context.scene;scene.frame_set(int((action.frame_range[0]+action.frame_range[1])*.5));scene.render.engine='CYCLES';scene.cycles.samples=12
    scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.3,.3,.3,1)
    bpy.ops.object.camera_add(location=(2,-6,2));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=2.8;scene.camera=cam
    for loc,power in [((2,-4,4),350),((-3,-2,2),200)]:
        bpy.ops.object.light_add(type='AREA',location=loc);bpy.context.object.data.energy=power;bpy.context.object.data.size=3
    scene.render.resolution_x=700;scene.render.resolution_y=700;scene.render.resolution_percentage=100;scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
