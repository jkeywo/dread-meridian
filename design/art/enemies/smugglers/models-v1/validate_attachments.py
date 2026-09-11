"""Reload actual GLBs; exercise every attachment state on each animated rig."""
import bpy,json,math
from pathlib import Path
from mathutils import Matrix
p=Path(__file__).resolve().parent
manifest=json.loads((p/'attachments.json').read_text());report={}
for asset,spec in manifest['characters'].items():
    bpy.ops.wm.open_mainfile(filepath=str(p/(asset+'-rigged.blend')))
    rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    sockets={o.get('attachment_id'):o for o in bpy.context.scene.objects if o.type=='EMPTY'}
    items={}
    for item in spec['items']:
        before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=str(p/(item+'.glb')))
        obs=list(set(bpy.data.objects)-before);root=next(o for o in obs if o.parent is None)
        anchors={o.get('attachment_id'):o for o in obs if o.get('attachment_id')}
        assert set(manifest['items'][item]['anchors'])==set(anchors)
        items[item]=(root,anchors)
    cases=[]
    for state,bindings in spec['states'].items():
        for b in bindings:
            root,anchors=items[b['item']];anchor=anchors[b['anchor']]
            bpy.context.view_layer.update();local=root.matrix_world.inverted()@anchor.matrix_world
            root.parent=sockets[b['socket']];root.matrix_parent_inverse=Matrix.Identity(4);root.matrix_basis=local.inverted()
            if 'support_anchor' in b:assert b['support_anchor'] in anchors
        maxerror=0
        for action_name in ['T_Pose','Manny_Idle','Manny_Walk']:
            action=bpy.data.actions[action_name];rig.animation_data.action=action;rig.animation_data.action_slot=action.slots[0]
            for frame in [1,int(action.frame_range[1]/2),int(action.frame_range[1])]:
                bpy.context.scene.frame_set(max(1,frame));bpy.context.view_layer.update()
                assert rig.location.length<.001,'Presentation offset in portable rig'
                for b in bindings:
                    root,anchors=items[b['item']];a=anchors[b['anchor']].matrix_world;s=sockets[b['socket']].matrix_world
                    error=max(abs(a[i][j]-s[i][j]) for i in range(4) for j in range(4));maxerror=max(maxerror,error)
                    assert error<1e-4,(asset,state,b['item'],error)
        cases.append({'state':state,'max_anchor_matrix_error':maxerror})
    report[asset]={'states':cases,'passed':True}
(p/'attachment-validation.json').write_text(json.dumps(report,indent=2))
print('ATTACHMENT_VALIDATION_PASSED')
