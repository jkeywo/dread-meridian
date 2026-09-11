# Third-party content

Every non-original asset in `Content/` is recorded here with its source, licence
and package root. Placeholder content carries an explicit removal condition; a
placeholder is never counted as finished art in captures or omissions.

Existing entry: Manny/Quinn template content under `/Game/Characters`, copied
from the installed Unreal 5.8.2 distribution and Epic-licensed. See
[characters](characters.md).

## Requested for acquisition

None of the rows below are in `Content/` yet. Acquisition needs an Epic account
sign-in on Fab, so it is a manual step; see the procedure underneath.

| Pack | Source | Licence | Format | Destination root | Role |
|---|---|---|---|---|---|
| Game Animation Sample | [Fab](https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016) (Epic Games) | Standard | Complete project | `/Game/ThirdParty/GameAnimationSample` | 500+ animations retargetable to the UE5 Mannequin skeleton; replaces stock idle/jog only |
| Niagara Examples Pack | [Fab](https://www.fab.com/listings/0e188eca-4e54-4fb2-a9ed-d8b8a565e600) (Epic Games) | Standard | Asset package, UE 5.7 | `/Game/ThirdParty/NiagaraExamples` | Reference VFX for telegraphs, hazards and persistent ground effects |
| Old West VOL 7 - Foliage | [Fab](https://www.fab.com/listings/78a6b75f-4687-4ba6-ab4b-d863d0af9491) (Dekogon Studios) | Standard | UE5 only; confirm an Unreal Engine build is offered, not UEFN only | `/Game/ThirdParty/OldWestVol7` | 20 foliage meshes with imposters for swamp and village dressing |
| Photorealistic Swamp Cypress Roots | [Fab](https://www.fab.com/listings/770f0ddb-2e52-4bd7-9fa8-deaf33466765) (EntropyArchives) | Standard | FBX + JPG, 47 MB zip | `/Game/ThirdParty/SwampCypress` | One scanned cypress knee, 70k tris, Nanite-intended, no LODs and no UE material |

Stylized Lake Village was requested but is **not usable here**. Its only listed
format is UEFN, it is built for the UEFN scene graph prefab system and it carries
a referenced asset, so it belongs to the Fortnite ecosystem rather than a
standalone Unreal project. A different fishing-hut and jetty source is needed.

## Status of each pack

All four are placeholders for the swamp MVP look described in GDD section 11.
None satisfies the art direction. They exist to let mechanics be built and tested
against readable silhouettes and real animation before bespoke art exists.

Removal condition: each row leaves `Content/` when the corresponding bespoke art
lands, or when the slice it supports is cut. The Cypress and Old West rows may
survive into shipped content because they are period-neutral natural forms; the
Game Animation Sample and Niagara Examples rows are learning and prototyping
sources whose individual assets should be replaced, not shipped wholesale.

## Acquisition procedure

1. Sign in to [fab.com](https://www.fab.com) with the Epic account and press
   **Add to My Library** on each listing above.
2. For the two asset packages, use the Epic Games Launcher library and
   **Add to project**, selecting Dread Meridian. The launcher only offers engine
   versions the publisher declared, which may be below the pinned 5.8.2; take the
   highest offered version and let the editor upconvert, then recompile shaders.
3. For the Game Animation Sample, which is a complete project, install it as its
   own project and migrate only the animation and retargeting assets into
   `/Game/ThirdParty/GameAnimationSample`. Do not migrate its maps or character
   blueprints; they carry their own locomotion implementation that would compete
   with ours.
4. For the Cypress FBX, import the mesh and its four textures into
   `/Game/ThirdParty/SwampCypress` and author a material. It arrives with no LODs
   and no collision, so both must be set before it is placed in a playable map.
5. Commit through Git LFS. `.gitattributes` covers `.uasset`, `.umap`, `.fbx`,
   `.tga`, `.jpg` and `.wav`.

## Repository size

`Content/` is currently 126 MB. The Game Animation Sample is by a wide margin the
largest of these and its animation set alone runs to a few gigabytes. Migrate
selectively and check the delta before committing; if the repository grows past
roughly a gigabyte of LFS objects, move third-party content to a submodule rather
than continuing to add to this history.
