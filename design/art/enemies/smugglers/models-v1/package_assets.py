import zipfile,json,hashlib,shutil
from pathlib import Path
p=Path(__file__).resolve().parent
manifest=json.loads((p/'attachments.json').read_text())
for item in manifest['items']:shutil.copy2(p/'source'/(item+'.fbx'),p/(item+'.fbx'))
required=[]
for asset in manifest['characters']:
    for ext in ['.blend','.fbx','.glb']:required.append(asset+'-rigged'+ext)
for item in manifest['items']:
    for ext in ['.blend','.fbx','.glb']:required.append(item+ext)
checks={}
for name in required:
    f=p/name;assert f.exists() and f.stat().st_size>1000
    checks[name]={'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()}
(p/'delivery-files.json').write_text(json.dumps(checks,indent=2))
archive=p/'smugglers-manny-rigged.zip'
files=[f for f in p.rglob('*') if f.is_file() and f.suffix in {'.blend','.fbx','.glb','.json','.md','.py','.png'} and '__pycache__' not in f.parts]
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for f in sorted(files):z.write(f,str(f.relative_to(p)))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert all(name in z.namelist() for name in required)
print('PACKAGE_COMPLETE',len(required),'models',archive.stat().st_size,'bytes')
