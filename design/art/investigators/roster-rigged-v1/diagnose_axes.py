import bpy,json
from pathlib import Path
from mathutils import Matrix,Quaternion,Vector
p=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(p/'manny-reference.blend'))
a=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
d=json.loads((p/'manny-data.json').read_text())
C=Matrix(((0,-1,0,0),(-1,0,0,0),(0,0,1,0),(0,0,0,1)))
def globals_of(rows):
    g={}
    for n,ref in d['bones'].items():
        t=rows[n];q=t['q'];m=Quaternion((q[3],*q[:3])).to_matrix().to_4x4();m.translation=Vector(t['t'])/100
        g[n]=g[ref['parent']]@m if ref['parent'] in g else m
    return {n:C@m@C for n,m in g.items()}
g=globals_of(d['bones']); anim=globals_of(d['animations']['idle']['frames'][0])
for n in ['pelvis','upperarm_l','hand_l','head','foot_l']:
    b=a.matrix_world@a.data.bones[n].matrix_local
    b=b.normalized()
    print(n,'REF',list(g[n].translation),'ANIM',list(anim[n].translation),'FBX',list(b.translation))
    print('K REF',g[n].inverted()@b,'K ANIM',anim[n].inverted()@b)
