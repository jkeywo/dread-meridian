import bpy,pathlib
root=pathlib.Path(__file__).resolve().parents[1]/'design/art/presentation/locomotion'
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(root/'M_Neutral_Run_Start_F_Rfoot.fbx'))
arm=next(o for o in bpy.data.objects if o.type=='ARMATURE')
arm.animation_data_clear();arm.data.pose_position='REST'
for b in arm.pose.bones:b.matrix_basis.identity()
mesh=bpy.data.meshes.new('RetargetReference');mesh.from_pydata([(0,0,0),(.01,0,0),(0,.01,0)],[],[(0,1,2)]);mesh.update()
o=bpy.data.objects.new('RetargetReference',mesh);bpy.context.collection.objects.link(o);o.parent=arm
g=o.vertex_groups.new(name=arm.data.bones[0].name);g.add([0,1,2],1,'REPLACE')
m=o.modifiers.new('Rig','ARMATURE');m.object=arm
bpy.ops.object.select_all(action='DESELECT');arm.select_set(True);o.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.export_scene.fbx(filepath=str(root/'SKM_UEFN_Mannequin.fbx'),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z')
print('REFERENCE_RIG_EXPORTED',len(arm.data.bones))
