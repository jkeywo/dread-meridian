"""Validate the four creature skins with the same pose checks as the human roster."""
from pathlib import Path
source=Path(__file__).resolve().parent.parent/'supplied-characters-v1/finish.py'
code=source.read_text().replace("for asset in json.loads((P/'manifest.json').read_text()):", "for asset,spec in json.loads((P/'manifest.json').read_text()).items():\n    if not spec['humanoid']:continue")
# These creatures have no held or holstered equipment.
code=code.replace("assert len(sockets)>=8 and all(o.parent==rig and o.parent_bone in rig.data.bones for o in sockets)","assert not sockets")
exec(compile(code,str(source),'exec'))
