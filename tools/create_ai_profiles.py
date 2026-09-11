"""Run through Unreal's Python commandlet; creates the AI profile data assets from the C++ defaults once.

Each /Game/DreadMeridian/AI/AIP_<Name> asset is a UDMAIProfile whose Weights start as DMUtilityAI::DefaultWeights
for that role or investigator kind. Existing assets are preserved so authored tuning is never overwritten.

Set DM_PROFILE_REFRESH=1 to re-apply the C++ defaults to assets that already exist. This is needed whenever
DefaultWeights gains an action or ability: UDMAIProfile::Resolve prefers the asset, so a stale one shadows the
new defaults and the bots quietly never see the addition. Refreshing costs nothing, because tuning results are
baked into DefaultWeights (see ApplyTunedDefaults) and experiments run through -DMAIWeights rather than the
asset; anything hand-edited in the asset itself would be lost.
"""
import os
import unreal

FOLDER = "/Game/DreadMeridian/AI"
NAMES = ["Gunman", "Bruiser", "Lookout", "Bomber", "GangBoss", "Sapper", "Photographer", "Medium", "Smuggler", "Raider"]

profile_class = unreal.load_class(None, "/Script/DreadMeridian.DMAIProfile")
if not profile_class:
    raise RuntimeError("UDMAIProfile is not available; build the DreadMeridian module first")
tools = unreal.AssetToolsHelpers.get_asset_tools()
refresh = os.environ.get("DM_PROFILE_REFRESH") == "1"
created, refreshed = [], []
for name in NAMES:
    asset_name = "AIP_" + name
    path = f"{FOLDER}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        if not refresh:
            unreal.log(f"{path} already exists; preserving authored tuning.")
            continue
        existing = unreal.EditorAssetLibrary.load_asset(path)
        existing.set_editor_property("profile_id", name)
        existing.apply_defaults()
        if not unreal.EditorAssetLibrary.save_asset(path):
            raise RuntimeError(f"Could not save {path}")
        refreshed.append(name)
        continue
    asset = None
    try:
        asset = tools.create_asset(asset_name, FOLDER, profile_class, None)
    except Exception as error:  # noqa: BLE001 - the factory-less path is best effort
        unreal.log_warning(f"create_asset without a factory failed for {asset_name}: {error}")
    if not asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", profile_class)
        asset = tools.create_asset(asset_name, FOLDER, profile_class, factory)
    if not asset:
        raise RuntimeError(f"Could not create {path}")
    asset.set_editor_property("profile_id", name)
    asset.apply_defaults()
    if not unreal.EditorAssetLibrary.save_asset(path):
        raise RuntimeError(f"Could not save {path}")
    created.append(name)
unreal.log(f"DREAD_AI_PROFILES_CREATED count={len(created)} names={','.join(created)}")
unreal.log(f"DREAD_AI_PROFILES_REFRESHED count={len(refreshed)} names={','.join(refreshed)}")
