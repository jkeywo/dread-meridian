"""Assign materials after all FBX imports have completed and assets are reloaded."""
import unreal,json
from pathlib import Path
P=Path(__file__).resolve().parent
for asset,spec in json.loads((P/'manifest.json').read_text()).items():
    mesh=unreal.load_asset(spec['native_path']);mat=unreal.load_asset(spec['native_path'].rsplit('/',1)[0]+'/M_'+asset)
    assert mesh and mat,asset
    prop='materials' if spec['humanoid'] else 'static_materials'
    slots=mesh.get_editor_property(prop)
    if not slots:slots=[unreal.SkeletalMaterial() if spec['humanoid'] else unreal.StaticMaterial()]
    for i,slot in enumerate(slots):
        slot.set_editor_property('material_interface',mat);slots[i]=slot
    mesh.set_editor_property(prop,slots)
    assert all(s.material_interface==mat for s in mesh.get_editor_property(prop)),asset
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False),asset
