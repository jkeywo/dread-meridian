import zipfile,json
from pathlib import Path
p=Path(__file__).resolve().parent
archive=p/'investigators-manny-rigged.zip'
files=[f for f in p.iterdir() if f.is_file() and f.suffix in {'.blend','.fbx','.glb','.json','.md','.py','.png'} and not f.name.endswith(('-idle.png','-tpose.png','-walk.png')) and f.name!='diagnose_axes.py']
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for f in sorted(files):z.write(f,f.name)
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    for a in ['sapper','photographer','medium','smuggler']:
        for ext in ['.blend','.fbx','.glb']:assert a+'-rigged'+ext in z.namelist()
print('PACKAGE_COMPLETE',archive.stat().st_size,len(files))
