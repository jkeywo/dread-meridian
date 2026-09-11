# Native game assets

`Maps/L_CombatSandbox.umap` is a native Unreal map created through the editor API.
It places the C++ greybox arena and selects the combat GameMode. Extend it in the
pinned editor; the generator preserves existing map edits. Input actions currently
live in C++; authored investigator/data assets come next.

Suggested first folders: Maps, Investigators, Input, Data, UI. Install Git LFS
before committing assets (`git lfs install`); `.gitattributes` covers uasset/umap.
