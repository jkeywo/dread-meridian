import pathlib
p=pathlib.Path(__file__).resolve().parent
players=p.parents[2]/'investigators'/'roster-rigged-v1'
script=players/'verify_unreal.py'
body=script.read_text(encoding='utf-8-sig')
body=body.replace("+'/SKM_'+asset.title()","+'/SKM_'+asset.title()+('_Refined' if asset in ['photographer','medium'] else '')")
exec(compile(body,str(script),'exec'),{'__file__':str(script),'__name__':'__main__'})
