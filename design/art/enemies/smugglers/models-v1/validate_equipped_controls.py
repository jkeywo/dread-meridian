"""Reload saved scenes, move both targets 1cm, solve, verify, leave files unchanged."""
import bpy,json,math
from pathlib import Path
from mathutils import Vector
p=Path(__file__).resolve().parent;reports={}
exec(compile((p/'equipment_controls.py').read_text(),str(p/'equipment_controls.py'),'exec'))
for f in sorted((p/'equipped').glob('*-equipped.blend')):
    bpy.ops.wm.open_mainfile(filepath=str(f))
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    controls=[o for o in bpy.context.scene.objects if o.name.startswith('CTRL_wrist_')]
    assert controls
    for ctrl in controls:ctrl.location.z+=.01
    bpy.context.view_layer.update()
    result=bpy.ops.pose.update_smuggler_equipment();assert result=={'FINISHED'}
    errors={}
    for ctrl in controls:
        side=ctrl.name[-1];actual=rig.matrix_world@rig.pose.bones['hand_'+side].head
        error=(actual-ctrl.matrix_world.translation).length;assert error<.0001,(f.name,side,error);errors[side]=error
    mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH' and any(m.type=='ARMATURE' for m in o.modifiers));evaluated=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get());origin=rig.matrix_world@rig.pose.bones['pelvis'].head
    radius=max((evaluated.matrix_world@v.co-origin).length for v in evaluated.data.vertices);assert math.isfinite(radius) and radius<2
    reports[f.stem]={'moved_target_errors_m':errors,'skin_radius_m':radius,'passed':True}
assert len(reports)==7
(p/'equipped'/'controls-validation.json').write_text(json.dumps(reports,indent=2))
print('EQUIPPED_CONTROLS_VERIFIED')
