"""Refresh only the two appearance-revised player skins; preserve other reports."""
import pathlib,json
p=pathlib.Path(__file__).resolve().parent
players=p.parents[2]/'investigators'/'roster-rigged-v1'
script=players/'import_unreal.py'
body=script.read_text(encoding='utf-8-sig').replace("['photographer','sapper','medium','smuggler']","['photographer','medium']")
body=body.replace('reports={}',"reports=json.loads((p/'unreal-validation.json').read_text())")
body=body.replace("task.destination_name='SKM_'+asset.title()","task.destination_name='SKM_'+asset.title()+'_Refined'")
exec(compile(body,str(script),'exec'),{'__file__':str(script),'__name__':'__main__'})
script=p/'verify_players_refined.py'
exec(compile(script.read_text(encoding='utf-8-sig'),str(script),'exec'),{'__file__':str(script),'__name__':'__main__'})
