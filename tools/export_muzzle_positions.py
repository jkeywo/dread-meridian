import bpy,json,pathlib
p=pathlib.Path('C:/Coding/dread-meridian/design/art/investigators/roster-rigged-v1');out={}
for item,name in [('sapper-carbine','SapperCarbine'),('photographer-rifle','PhotographerRifle'),('photographer-camera','PhotographerCamera')]:
 bpy.ops.wm.open_mainfile(filepath=str(p/(item+'.blend')))
 grip=bpy.data.objects['ANCHOR_grip_r'];effect=bpy.data.objects.get('ANCHOR_muzzle') or bpy.data.objects['ANCHOR_optical_axis']
 v=grip.matrix_world.inverted()@effect.matrix_world.translation
 out['SM_'+name+'_Held']=[v.x*100,-v.y*100,v.z*100]
pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/props/effect-anchors.json').write_text(json.dumps(out,indent=2))
