import bpy,json,shutil,struct
from pathlib import Path
p=Path(__file__).resolve().parent
src=p.parent/'roster-3d-v1'
manifest=json.loads((src/'attachments.json').read_text())
manifest['schema_version']=2;manifest['status']='PROVISIONAL skinned Manny-compatible concept rigs'
manifest['transforms']['animation_note']='Character sockets are bone-parented. Native Unreal mesh sockets use the same stable names.'
manifest['transforms']['hand_pose']='Finger bones are skinned. Item-specific finger poses and two-hand IK are separate animation work.'
reports={}
for asset in manifest['characters']:
    bpy.ops.wm.open_mainfile(filepath=str(p/(asset+'-rigged.blend')))
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    rig.animation_data_create();rig.animation_data.action=bpy.data.actions['T_Pose'];rig.animation_data.action_slot=bpy.data.actions['T_Pose'].slots[0]
    bpy.context.scene.frame_set(1)
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.gltf(filepath=str(p/(asset+'-rigged.glb')),export_format='GLB',use_selection=True,export_extras=True,export_animations=True,export_animation_mode='ACTIONS',export_all_influences=True)
    # Useful opening view for animators; underlying reference bind pose is unchanged.
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=='VIEW_3D':
                area.spaces.active.region_3d.view_distance=3
                area.spaces.active.region_3d.view_location=(0,0,1)
                area.spaces.active.shading.color_type='MATERIAL'
    bpy.ops.wm.save_as_mainfile(filepath=str(p/(asset+'-rigged.blend')))
    manifest['characters'][asset]['asset']=asset+'-rigged.glb'
    raw=(p/(asset+'-rigged.glb')).read_bytes();size=struct.unpack_from('<I',raw,12)[0];d=json.loads(raw[20:20+size])
    assert len(d.get('skins',[]))==1
    assert len(d.get('animations',[]))==3,(asset,[a['name'] for a in d.get('animations',[])])
    assert all('JOINTS_0' in prim['attributes'] and 'WEIGHTS_0' in prim['attributes'] for m in d['meshes'] for prim in m['primitives'])
    reports[asset]={'skins':len(d['skins']),'joints':len(d['skins'][0]['joints']),'animations':[a['name'] for a in d['animations']],'second_weight_set':any('WEIGHTS_1' in prim['attributes'] for m in d['meshes'] for prim in m['primitives'])}
for item in manifest['items']:
    for ext in ['.blend','.glb']:shutil.copy2(src/(item+ext),p/(item+ext))
(p/'attachments.json').write_text(json.dumps(manifest,indent=2))
(p/'gltf-validation.json').write_text(json.dumps(reports,indent=2))
print('FINALIZE_COMPLETE')
