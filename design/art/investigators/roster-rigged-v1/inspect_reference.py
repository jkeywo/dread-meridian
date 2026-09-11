import bpy, json
from pathlib import Path
p=Path(__file__).resolve().parent
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(p/'manny-idle.fbx'),automatic_bone_orientation=False)
a=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
report={'armature':a.name,'matrix':[list(r) for r in a.matrix_world],'bones':{b.name:{'parent':b.parent.name if b.parent else None,'head':list(a.matrix_world@b.head_local),'tail':list(a.matrix_world@b.tail_local)} for b in a.data.bones}}
(p/'reference-bones.json').write_text(json.dumps(report,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(p/'manny-reference.blend'))
print('REFERENCE_INSPECTED',len(a.data.bones))

