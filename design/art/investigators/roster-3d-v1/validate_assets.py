"""Validate standalone exports and every configured attachment state."""
import bpy
import json
import struct
import sys
import math
from pathlib import Path
out=Path(__file__).resolve().parent
sys.path.insert(0,str(out))
from attachment_tools import import_asset,snap,set_visible
manifest=json.loads((out/'attachments.json').read_text())
reports=[];placements=[]
allassets=list(manifest['characters'])+list(manifest['items'])
for asset in allassets:
    data=(out/(asset+'.glb')).read_bytes()
    magic,version,length=struct.unpack_from('<III',data)
    assert magic==0x46546c67 and version==2 and length==len(data)
    n,kind=struct.unpack_from('<II',data,12);assert kind==0x4e4f534a
    gltf=json.loads(data[20:20+n])
    nodes=gltf['nodes'];names=[node.get('name') for node in nodes]
    required=manifest['required_character_nodes'] if asset in manifest['characters'] else manifest['items'][asset]['anchors']
    assert set(required)<=set(names),(asset,'missing locator',set(required)-set(names))
    assert len(names)==len(set(names)),(asset,'duplicate node names')
    assert asset+'_root' in names
    assert not gltf.get('skins') and not gltf.get('animations') and not gltf.get('cameras')
    assert not gltf.get('extensions',{}).get('KHR_lights_punctual')
    assert len(gltf['meshes'])==1
    parent={child:i for i,node in enumerate(nodes) for child in node.get('children',[])}
    root=names.index(asset+'_root')
    for locator in required:
        i=names.index(locator)
        assert 'mesh' not in nodes[i]
        seen=set()
        while i!=root:
            assert i not in seen and i in parent
            seen.add(i);i=parent[i]
    triangles=sum(gltf['accessors'][p['indices']]['count']//3 for m in gltf['meshes'] for p in m['primitives'])
    bpy.ops.wm.read_factory_settings(use_empty=True)
    loaded=import_asset(asset)
    if asset in manifest['characters']:
        right=loaded['nodes']['SOCKET_hand_r'].matrix_world.translation
        left=loaded['nodes']['SOCKET_hand_l'].matrix_world.translation
        assert right.x<-.65 and left.x>.65
        assert abs(right.x+left.x)<1e-5 and abs(right.z-left.z)<1e-5
        assert right.z>1.3
        for locator in required:
            assert all(math.isfinite(v) for row in loaded['nodes'][locator].matrix_world for v in row)
    reports.append({'asset':asset,'glb_import_verified':True,'triangles':triangles,'materials':len(gltf.get('materials',[])),'attachment_nodes':required,'single_geometry_mesh':True,'rigged':False})

for char,spec in manifest['characters'].items():
    for state,mounts in spec['states'].items():
        bpy.ops.wm.read_factory_settings(use_empty=True)
        character=import_asset(char)
        used=set()
        for mount in mounts:
            assert mount['item'] in spec['items']
            item=import_asset(mount['item'])
            if mount.get('visibility')=='hidden':
                set_visible(item,False)
                assert all(ob.hide_render for ob in item['objects'])
                placements.append({'character':char,'state':state,'item':mount['item'],'visibility':'hidden'})
                continue
            assert mount['socket'] not in used
            used.add(mount['socket'])
            error=snap(character,item,mount['socket'],mount['anchor'])
            if 'support_anchor' in mount:assert mount['support_anchor'] in item['nodes']
            placements.append({'character':char,'state':state,'item':mount['item'],'socket':mount['socket'],'anchor':mount['anchor'],'matrix_alignment_error':error,'item_root_world_blender':[list(row) for row in item['root'].matrix_world]})
result={'assets':reports,'placements':placements,'states_checked':sum(len(c['states']) for c in manifest['characters'].values()),'status':'passed','limitations':['Static locators only; no skeletal rig or grip animation.','No Unreal import or runtime attachment system tested.']}
(out/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
print('ROSTER_VALIDATION_PASSED',len(reports),'assets',len(placements),'placements')
