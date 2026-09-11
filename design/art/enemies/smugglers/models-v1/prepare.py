"""Build the attachment-state manifest from the authored source locators."""
import json
from pathlib import Path
p=Path(__file__).resolve().parent
items={'gunman':['gunman-rifle'],'bruiser':['bruiser-truncheon'],'lookout':['lookout-binoculars','lookout-pistol'],'bomber':['bomber-grenade','bomber-satchel'],'gang-boss':['gang-boss-smg']}
stow={'gunman-rifle':'SOCKET_stow_back','bruiser-truncheon':'SOCKET_holster_hip_r','lookout-binoculars':'SOCKET_stow_chest','lookout-pistol':'SOCKET_holster_hip_r','bomber-grenade':'SOCKET_grenade_pouch','bomber-satchel':'SOCKET_holster_hip_r','gang-boss-smg':'SOCKET_stow_back'}
m={'schema_version':2,'status':'PROVISIONAL skinned concept assets','units':'metres','blender_axes':{'up':'+Z','character_forward':'-Y','character_right':'-X','item_forward':'+X'},'transforms':{'formula':'item_root_world = character_socket_world @ inverse(item_anchor_in_root)','unreal':'Use native mesh sockets and native prop anchors, in centimetres. Match anchor world transform to character socket world transform.','hand_pose':'Empty-hand T-pose. Finger animation and two-hand IK use the provided grip/support anchors.','animation_note':'Sockets follow stock Manny bones.'},'characters':{},'items':{}}
for role,props in items.items():
    def binding(item,held=False):
        b={'item':item,'socket':'SOCKET_hand_r' if held else stow[item],'anchor':'ANCHOR_grip_r' if held else 'ANCHOR_stow'}
        if held and 'ANCHOR_support_l' in json.loads((p/'source'/(item+'-locators.json')).read_text()):b['support_anchor']='ANCHOR_support_l'
        return b
    states={'stowed':[binding(item) for item in props]}
    for item in props:states[item+'_held']=[binding(i,i==item) for i in props]
    m['characters'][role]={'asset':role+'-rigged.glb','items':props,'states':states}
    for item in props:m['items'][item]={'asset':item+'.glb','anchors':list(json.loads((p/'source'/(item+'-locators.json')).read_text()))}
(p/'source'/'attachments.json').write_text(json.dumps(m,indent=2))
print('MANIFEST_COMPLETE')
