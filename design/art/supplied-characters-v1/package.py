import zipfile,json
from pathlib import Path
P=Path(__file__).resolve().parent
files=[]
for asset in json.loads((P/'manifest.json').read_text()):
    files.extend(P.glob(asset+'-rigged.*'))
    files.extend([P/f'{asset}-sockets.json',P/f'{asset}-tpose.png',P/f'{asset}-walk.png'])
files.extend((P/'textures').glob('*.png'))
files.extend(P/name for name in ['README.md','manifest.json','landmarks.json','blender-validation.json','source-inventory.json','unreal-validation.json','unreal-readback-validation.json'])
with zipfile.ZipFile(P/'supplied-character-rigs.zip','w',compression=zipfile.ZIP_DEFLATED,compresslevel=1) as z:
    for file in files:
        if file.is_file():z.write(file,file.relative_to(P))
with zipfile.ZipFile(P/'supplied-character-rigs.zip') as z:
    assert z.testzip() is None
    assert len([n for n in z.namelist() if n.endswith('-rigged.fbx')])==9
print('PACKAGED',len(files),(P/'supplied-character-rigs.zip').stat().st_size)
