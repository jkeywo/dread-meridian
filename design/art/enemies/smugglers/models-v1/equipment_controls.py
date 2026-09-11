"""Run this Blender text, move CTRL_wrist_r/l, then F3: Update Equipment Arms.
The operator updates the current equipped action at the current frame.
"""
import bpy,math
from mathutils import Vector,Matrix

class SMUGGLERS_OT_update_equipment_arms(bpy.types.Operator):
    bl_idname='pose.update_smuggler_equipment'
    bl_label='Update Equipment Arms'
    bl_options={'REGISTER','UNDO'}
    def execute(self,context):
        rig=next(o for o in context.scene.objects if o.type=='ARMATURE')
        ref={b.name:(rig.matrix_world@b.matrix_local).normalized() for b in rig.data.bones}
        for side,s in [('r',-1),('l',1)]:
            ctrl=bpy.data.objects.get('CTRL_wrist_'+side)
            if ctrl is None:continue
            a=rig.matrix_world@rig.pose.bones['upperarm_'+side].head;c=ctrl.matrix_world.translation
            a0=ref['upperarm_'+side].translation;b0=ref['lowerarm_'+side].translation;c0=ref['hand_'+side].translation
            l1=(b0-a0).length;l2=(c0-b0).length;d=c-a;distance=d.length
            if not abs(l1-l2)+.001<distance<l1+l2-.001:
                self.report({'ERROR'},'Wrist target is outside arm reach: '+side);return {'CANCELLED'}
            axis=d.normalized();pole=Vector((s*.65,-.12,1.1))-a;perp=(pole-axis*pole.dot(axis)).normalized()
            along=(l1*l1-l2*l2+distance*distance)/(2*distance);b=a+axis*along+perp*math.sqrt(max(0,l1*l1-along*along))
            for name,start,end,original in [('upperarm',a,b,b0-a0),('lowerarm',b,c,c0-b0)]:
                n=name+'_'+side;rotation=(original.rotation_difference(end-start).to_matrix()@ref[n].to_3x3()).to_quaternion()
                m=Matrix.LocRotScale(start,rotation,rig.matrix_world.to_scale());rig.pose.bones[n].matrix=rig.matrix_world.inverted()@m;context.view_layer.update()
            rig.pose.bones['hand_'+side].matrix=rig.matrix_world.inverted()@ctrl.matrix_world
            for limb in ['upperarm','lowerarm']:
                parent=limb+'_'+side
                for j in [1,2]:
                    n=f'{limb}_twist_{j:02d}_{side}';rig.pose.bones[n].matrix=rig.pose.bones[parent].matrix@rig.data.bones[parent].matrix_local.inverted()@rig.data.bones[n].matrix_local
            for pb in rig.pose.bones:
                if pb.name.endswith('_'+side) and pb.name.startswith(('upperarm','lowerarm','hand')):
                    for prop in ['location','rotation_quaternion','scale']:pb.keyframe_insert(data_path=prop,frame=context.scene.frame_current)
        context.view_layer.update();return {'FINISHED'}

if hasattr(bpy.types,'SMUGGLERS_OT_update_equipment_arms'):
    bpy.utils.unregister_class(bpy.types.SMUGGLERS_OT_update_equipment_arms)
bpy.utils.register_class(SMUGGLERS_OT_update_equipment_arms)
