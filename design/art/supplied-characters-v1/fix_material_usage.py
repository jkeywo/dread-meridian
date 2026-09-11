"""Enable skeletal shader permutations without reimporting meshes or textures."""
import json
from pathlib import Path
import unreal

P = Path(__file__).resolve().parent
for asset, spec in json.loads((P / 'manifest.json').read_text()).items():
    mesh = unreal.load_asset(spec['native_path'])
    assert mesh, asset
    for slot in mesh.get_editor_property('materials'):
        mat = slot.material_interface
        assert mat and 'M_Supplied_' in mat.get_name(), asset
        unreal.MaterialEditingLibrary.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        assert mat.get_editor_property('used_with_skeletal_mesh'), asset
        assert unreal.EditorAssetLibrary.save_loaded_asset(mat), asset
    unreal.log('FIXED_SUPPLIED_MATERIAL ' + asset)
unreal.log('SUPPLIED_MATERIAL_USAGE_COMPLETE')
