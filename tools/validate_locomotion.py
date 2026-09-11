import unreal,json,pathlib,math
root='/Game/DreadMeridian/Presentation/Animations'
m=json.loads(pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/locomotion/manifest.json').read_text())
rows=[]
def bone(a,t,b):
 p=unreal.AnimPoseExtensions.get_anim_pose_at_time(a,t,unreal.AnimPoseEvaluationOptions());assert unreal.AnimPoseExtensions.is_valid(p)
 return unreal.AnimPoseExtensions.get_bone_pose(p,b,unreal.AnimPoseSpaces.LOCAL)
for row in m['clips']:
 a=unreal.load_asset(row['path']);assert a and a.get_editor_property('skeleton')
 assert a.get_editor_property('force_root_lock') and not a.get_editor_property('enable_root_motion')
 name=a.get_name().replace('A_DM_Locomotion_','').removesuffix('_Rifle')
 full=unreal.load_asset(root+'/LocomotionSource/Retargeted/A_DM_Uncut_'+name);assert full
 start=.3 if 'Start' in name else .8 if 'Stop' in name else 7/30
 for b in ['pelvis','thigh_r','calf_r']:
  expected=bone(full,start,b);actual=bone(a,0,b)
  delta=(expected.translation-actual.translation).length()
  assert delta<.2,(a.get_name(),b,delta)
 if a.get_name().endswith('_Rifle'):
  idle=unreal.load_asset(root+'/A_DM_Idle_Rifle_Hip')
  expected=bone(idle,0,'upperarm_r');actual=bone(a,a.get_play_length()/2,'upperarm_r')
  assert (expected.translation-actual.translation).length()<.2
 rows.append({'clip':a.get_name(),'loaded':True,'window_pose_matches':True})
pathlib.Path('C:/Coding/dread-meridian/design/art/presentation/locomotion/validation.json').write_text(json.dumps(rows,indent=2))
unreal.log('LOCOMOTION_ASSET_VALIDATION_PASSED')
