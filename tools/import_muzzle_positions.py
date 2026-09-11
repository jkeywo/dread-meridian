import unreal,json,pathlib
p=pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/props/effect-anchors.json')
for name,point in json.loads(p.read_text()).items():
 mesh=unreal.load_asset('/Game/DreadMeridian/Presentation/Props/'+name)
 assert unreal.DMArtRigLibrary.set_static_mesh_socket(mesh,'Muzzle',unreal.Vector(*point))
 unreal.EditorAssetLibrary.save_loaded_asset(mesh)
unreal.log('MUZZLE_SOCKETS_SAVED')
