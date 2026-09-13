"""Reuse the validated Manny skinning pipeline with this set's source and landmarks."""
from pathlib import Path
source=Path(__file__).resolve().parent.parent/'supplied-characters-v1/build_rig.py'
exec(compile(source.read_text(),str(source),'exec'))
