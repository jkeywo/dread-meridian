import bpy
from pathlib import Path
p=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(p/'gunman-rigged.blend'))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
action=bpy.data.actions['T_Pose'];rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0]
bpy.context.scene.frame_start=1;bpy.context.scene.frame_end=2;bpy.context.scene.frame_set(1)
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(p/'smugglers-tpose.fbx'),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True)
