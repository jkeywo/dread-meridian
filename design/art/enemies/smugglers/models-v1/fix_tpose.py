"""Align lower-arm/hand positions exactly; retain the stock reference bind pose."""
import bpy
from pathlib import Path
from mathutils import Vector
p=Path(__file__).resolve().parent
players=p.parents[2]/'investigators'/'roster-rigged-v1'
for asset,folder in [(a,p) for a in ['gunman','bruiser','lookout','bomber','gang-boss']]+[(a,players) for a in ['photographer','medium']]:
    bpy.ops.wm.open_mainfile(filepath=str(folder/(asset+'-rigged.blend')))
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    action=bpy.data.actions['T_Pose'];rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0];bpy.context.scene.frame_set(1)
    for side,s in [('l',1),('r',-1)]:
        for parent,name in [('upperarm','lowerarm'),('lowerarm','hand')]:
            par=rig.pose.bones[parent+'_'+side];pb=rig.pose.bones[name+'_'+side]
            length=(rig.data.bones[name+'_'+side].head_local-rig.data.bones[parent+'_'+side].head_local).length
            m=pb.matrix.copy();m.translation=par.head+Vector((s*length,0,0));pb.matrix=m
            bpy.context.view_layer.update();pb.keyframe_insert(data_path='location',frame=1);pb.keyframe_insert(data_path='rotation_quaternion',frame=1)
    bpy.context.scene.frame_set(1);bpy.context.view_layer.update()
    for side in ['l','r']:
        d=rig.pose.bones['hand_'+side].head-rig.pose.bones['upperarm_'+side].head
        assert abs(d.y)<.002 and abs(d.z)<.002,(asset,side,str(d))
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.wm.save_as_mainfile(filepath=str(folder/(asset+'-rigged.blend')))
print('TPOSE_ALIGNMENT_PASSED')
