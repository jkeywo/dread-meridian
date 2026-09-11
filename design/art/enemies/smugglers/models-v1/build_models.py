"""Authored 3D concept geometry from approved 2026-09-10 Smuggler designs.
Blender 5.0: --background --python build_models.py -- <asset>
Source modelling coordinates: metres, Z up, character faces -Y, item forward +X.
"""
import sys, json, math
from pathlib import Path
from mathutils import Matrix
HERE=Path(__file__).resolve().parent
HELPERS=HERE.parents[2]/'investigators'/'roster-3d-v1'
sys.path.insert(0,str(HELPERS))
import asset_common as c
import bpy
OUT=HERE/'source';OUT.mkdir(exist_ok=True)
c.OUT=OUT
asset=sys.argv[sys.argv.index('--')+1]
c.set_asset(asset)
CONFIG={
 'gunman':dict(hip=1.02,knee=.61,leg=.113,shoulder=1.43,sx=.205,elbow=.475,wrist=.73,head=1.68,neck=1.55,size=.96,palm=.7732,girth=.178),
 'bruiser':dict(hip=1.08,knee=.63,leg=.165,shoulder=1.56,sx=.29,elbow=.57,wrist=.86,head=1.82,neck=1.69,size=1.38,palm=.9221,girth=.264),
 'lookout':dict(hip=1.06,knee=.64,leg=.116,shoulder=1.43,sx=.202,elbow=.472,wrist=.72,head=1.67,neck=1.55,size=.86,palm=.7587,girth=.177),
 'bomber':dict(hip=.98,knee=.59,leg=.139,shoulder=1.36,sx=.23,elbow=.493,wrist=.733,head=1.60,neck=1.48,size=1.02,palm=.7789,girth=.215),
 'gang-boss':dict(hip=1.04,knee=.62,leg=.124,shoulder=1.47,sx=.226,elbow=.50,wrist=.76,head=1.72,neck=1.59,size=1.02,palm=.8059,girth=.196),
}
(HERE/'landmarks.json').write_text(json.dumps(CONFIG,indent=2))
ivory=c.material('Aged ivory cotton',(.79,.71,.53),.86,bump=.004)
red=c.material('Oxblood cloth',(.30,.029,.020),.81,bump=.003)
brown=c.material('Tobacco leather',(.28,.13,.056),.63,bump=.004)
green=c.material('Weathered olive',(.15,.19,.065),.83,bump=.004)
charcoal=c.material('Charcoal wool',(.07,.072,.064),.88,bump=.003)
walnut=c.material('Walnut stock',(.29,.11,.037),.57,bump=.002)
steel=c.material('Blued steel',(.062,.08,.087),.36,.78)
brass=c.material('Aged brass',(.43,.29,.105),.43,.72)

def head(q):
    female=asset in ['lookout','bomber'];z=q['head']+(.05 if female else 0);broad=asset in ['bruiser','bomber'];w=1.08 if asset=='bomber' else (1.20 if broad else 1.0)
    skin=c.material('Skin '+asset,{'gunman':(.68,.41,.25),'bruiser':(.74,.50,.35),'lookout':(.76,.46,.29),'bomber':(.61,.34,.19),'gang-boss':(.42,.22,.12)}[asset],.68,bump=.002)
    hair=c.material('Hair '+asset,{'gunman':(.034,.026,.020),'bruiser':(.065,.046,.029),'lookout':(.30,.073,.019),'bomber':(.039,.025,.018),'gang-boss':(.47,.49,.44)}[asset],.84)
    # Retain the face sculpt and eyelids; replace the investigator-specific hair.
    before=set(bpy.data.objects)
    if female:
        faceparts=[c.ell('Oval cranium',(0,.006,z),(.092*w,.083,.135),skin,40,28),
            c.ell('Tapered jaw',(0,-.024,z-.072),(.058*w,.054,.055),skin,32,24),
            c.ell('Rounded chin',(0,-.045,z-.115),(.033*w,.032,.019),skin,24,18),
            c.ell('Nose bridge',(0,-.080,z-.018),(.010*w,.022,.031),skin),
            c.ell('Nose tip',(0,-.102,z-.037),(.014*w,.018,.012),skin)]
        for s in [-1,1]:faceparts.append(c.ell('Soft cheekbone',(s*.060*w,-.057,z-.029),(.022,.018,.028),skin))
        c.fuse(faceparts,'Sculpted feminine face',.0028)
        lips=c.material('Muted rose lips',(.43,.15,.115),.7)
        eyes=c.material('Warm eye white',(.76,.72,.61),.42)
        iris=c.material('Hazel eyes',(.13,.16,.065),.43)
        for s in [-1,1]:
            x=s*.036*w
            c.ell('Ear',(s*.092*w,.001,z-.031),(.012,.019,.028),skin)
            c.ell('Almond eye',(x,-.079,z+.012),(.019,.006,.0085),eyes,24,16)
            c.ell('Hazel iris',(x,-.085,z+.012),(.006,.002,.007),iris,20,14)
            c.ell('Pupil',(x,-.087,z+.012),(.003,.001,.0045),c.pupil,16,10)
            c.ell('Eye highlight',(x-.002,-.088,z+.015),(.0014,.0008,.0014),eyes,12,8)
            c.tube('Upper eyelid',[(x-.019,-.078,z+.011),(x,-.086,z+.021),(x+.019,-.078,z+.014)],.0025,skin,2)
            c.tube('Fine lash line',[(x-.017,-.080,z+.014),(x,-.087,z+.020),(x+.017,-.08,z+.015)],.0012,hair,2)
            c.tube('Defined arched brow',[(s*.016,-.086,z+.039),(s*.039,-.083,z+.048),(s*.059,-.066,z+.037)],.0028,hair,2)
            c.ell('Nostril',(s*.009,-.115,z-.041),(.0027,.0015,.002),lips,12,8)
        c.tube('Cupid bow upper lip',[(-.025,-.081,z-.079),(-.009,-.095,z-.074),(0,-.097,z-.077),(.009,-.095,z-.074),(.025,-.081,z-.079)],.0037,lips,2)
        c.tube('Soft lower lip',[(-.023,-.083,z-.08),(0,-.098,z-.085),(.023,-.083,z-.08)],.0048,lips,2)
    else:c.face('smuggler',z,skin,hair,hair,wide=w)
    unwanted=('Short beard','Beard chin','Moustache','Crooked smile','Teeth in grin','Bob wave','Sculpted bob crest','Back of bob','Swept forelock','Gold earring','Plum earring')
    for ob in set(bpy.data.objects)-before:
        if ob.name.startswith(unwanted):bpy.data.objects.remove(ob,do_unlink=True)
    if not female:c.tube('Firm mouth',[(-.034,-.104,z-.083),(0,-.116,z-.086),(.035,-.102,z-.081)],.003,c.lip)
    if asset!='bruiser':
        c.ell('Hair back',(0,.052,z+.048),(.103*w,.062,.103),hair)
        c.ell('Hair crown',(0,.011,z+.117),(.100*w,.078,.042),hair)
        for i in range(10):
            x=-.081+i*.018
            c.tube('Combed hair',[(x,.047,z+.13),(x-.013,-.024,z+.146),(x-.02,-.077,z+.109)],.008,hair,2)
        if female:
            for s in [-1,1]:
                for i in range(5):
                    xx=s*(.079+i*.004)
                    c.tube('Loose face framing lock',[(xx,.025,z+.114),(xx+s*.012,-.028,z+.064),(xx+s*.008,-.032,z+.004),(xx+s*.021,.002,z-.056-i*.004)],.010 if asset=='lookout' else .008,hair,3)
                c.ell('Nape hair',(s*.067,.052,z-.039),(.038,.052,.065),hair)
    if asset in ['gunman','gang-boss']:
        for s in [-1,1]:c.tube('Narrow moustache',[(s*.004,-.119,z-.058),(s*.021,-.117,z-.060),(s*.041,-.101,z-.064)],.006,hair)
    if asset=='bruiser':
        for s in [-1,1]:
            c.ell('Mutton chop sideburn',(s*.105,-.033,z-.054),(.025,.044,.069),hair)
        c.tube('Eyebrow scar',[(.042,-.105,z+.07),(.051,-.106,z+.043)],.0035,ivory)
    if asset in ['gunman','lookout']:
        c.ell('Flat cap crown',(0,.006,z+.139),(.127,.111,.046),brown)
        c.ell('Flat cap peak',(0,-.099,z+.114),(.099,.071,.010),brown)
        for s in [-1,1]:c.tube('Cap seam',[(0,-.10,z+.15),(s*.045,0,z+.184),(s*.062,.073,z+.16)],.0024,c.leatheredge)
    if asset=='bomber':
        c.ell('Messy high hair bun',(0,.074,z+.17),(.060,.049,.051),hair)
        for i in range(6):c.loop('Coiled bun strand',(0,.067,z+.168+i*.006),.041-i*.004,hair,axis='Z',wire=.006)
        c.tube('Goggle strap',[(-.112,0,z+.09),(-.099,-.065,z+.117),(0,-.08,z+.126),(.099,-.065,z+.117),(.112,0,z+.09)],.009,brown)
        for s in [-1,1]:
            c.loop('Goggle rim',(s*.049,-.086,z+.128),.030,brass,wire=.005)
            c.bone('Smoked goggle lens',(s*.049,-.086,z+.128),(s*.049,-.091,z+.128),.026,.026,steel,32)
    if asset=='gang-boss':
        c.ell('Felt hat brim',(0,.01,z+.13),(.173,.145,.012),charcoal)
        c.ell('Fedora crown',(0,.022,z+.189),(.11,.092,.080),charcoal)
        c.ell('Hat band',(0,.02,z+.158),(.113,.095,.016),brown)
    if asset=='lookout':
        for s in [-1,1]:
            for i in range(5):c.ell('Freckle',(s*(.039+i*.008),-.093+i*.002,z-.026+(i%2)*.007),(.0019,.001,.0017),brown,8,6)
    return skin

def character():
    q=CONFIG[asset];h=q['hip'];z=q['shoulder'];g=q['girth'];sx=q['sx'];el=q['elbow'];wr=q['wrist']
    skin=head(q)
    cloth={'gunman':charcoal,'bruiser':brown,'lookout':green,'bomber':brown,'gang-boss':c.material('Bottle green greatcoat',(.026,.085,.059),.83,bump=.003)}[asset]
    trouser={'gunman':brown,'bruiser':green,'lookout':charcoal,'bomber':green,'gang-boss':red}[asset]
    chunk={'gunman':.94,'bruiser':1.36,'lookout':.93,'bomber':1.15,'gang-boss':1.02}[asset]
    legs=c.bootlegs(q['leg'],h,q['knee'],.34 if asset=='lookout' else .26,trouser,brown if asset!='gang-boss' else charcoal,chunk)
    legs.append(c.ell('Trouser hips',(0,.025,h-.06),(g,.13,h*.15),trouser))
    c.fuse(legs,'Continuous tailored trousers',.006)
    for s in [-1,1]:
        for j in range(3):
            zz=q['knee']-.07+j*.07
            c.tube('Trouser fold',[(s*q['leg']-.05,-.058,zz),(s*q['leg'],-.064,zz-.009),(s*q['leg']+.05,-.054,zz+.011)],.004,trouser)
    torso=[c.ell('Jacket body',(0,.022,(h+z)/2),(g,.137,(z-h)/2+.08),cloth),c.ell('Shoulder bridge',(0,.025,z-.016),(sx+.016,.111,.085),cloth)]
    c.ell('Neck',(0,.012,q['neck']),(.044 if asset in ['lookout','bomber'] else .054*chunk,.047 if asset in ['lookout','bomber'] else .058,.082),skin)
    shirt=red if asset=='gang-boss' else ivory
    c.patch('Waistcoat front',[(-g*.63,-.119,h+.035),(-g*.60,-.127,z-.037),(0,-.15,z-.11),(g*.60,-.127,z-.037),(g*.63,-.119,h+.035),(0,-.149,h-.012)],green if asset=='bomber' else shirt,.01)
    if asset in ['gunman','lookout']:
        vest=charcoal if asset=='gunman' else c.material('Mustard waistcoat',(.51,.31,.065),.8)
        for s in [-1,1]:c.patch('Tailored vest panel',[(s*.012,-.155,h+.02),(s*.012,-.156,z-.20),(s*.087,-.122,z-.036),(s*.146,-.1,z-.12),(s*.14,-.126,h+.02),(s*.045,-.15,h-.008)],vest)
    for s in [-1,1]:
        if asset=='bruiser':
            arm=[c.ell('Deltoid',(s*sx,.025,z),(.14,.125,.116),skin),c.bone('Upper arm',(s*sx,.025,z),(s*el,.025,z),.106,.077,skin),c.ell('Biceps',(s*(sx+el)/2,.017,z-.013),(.18,.096,.09),skin),c.bone('Forearm',(s*el,.025,z),(s*wr,.025,z),.086,.037,skin)]
            c.fuse(arm,'Bare muscular arm '+str(s),.005)
        else:
            sleevec=ivory if asset=='gunman' else cloth
            arm=[c.ell('Sleeve shoulder',(s*sx,.025,z),(.101,.093,.088),sleevec),c.bone('Upper sleeve',(s*sx,.025,z),(s*el,.025,z),.09,.069,sleevec)]
            end=wr-.018 if asset=='gang-boss' else el+.035
            arm.append(c.bone('Sleeve cuff',(s*(el-.02),.025,z),(s*end,.025,z),.071,.050 if asset=='gang-boss' else .072,sleevec))
            c.fuse(arm,'Sleeve '+str(s),.005)
            if asset!='gang-boss':
                c.bone('Rolled sleeve band',(s*(el+.005),.025,z),(s*(el+.05),.025,z),.076,.076,sleevec)
                c.bone('Exposed forearm',(s*(el+.03),.025,z),(s*wr,.025,z),.062,.030,skin)
    c.fuse(torso,'Fitted outer garment',.005)
    palms=c.hands(wr,z,brown if asset=='bomber' else skin,q['size'])
    c.ell('Leather belt',(0,.023,h+.016),(g+.011,.143,.025),brown)
    c.buckle('Belt buckle',0,-.129,h+.018,.050,.035)
    for i in range(5):c.ell('Brass front button',(0,-.163,h+.07+i*(z-h-.15)/5),(.005,.004,.005),brass,12,8)
    for s in [-1,1]:
        c.patch('Collar',[(s*.004,-.066,q['neck']-.018),(s*.055,-.05,q['neck']+.012),(s*.10,-.115,z-.014),(s*.058,-.144,z-.088)],ivory)
        if asset not in ['gunman','bomber']:
            c.patch('Lapel',[(s*.102,-.102,z+.007),(s*.152,-.10,z-.06),(s*.102,-.143,z-.104),(s*.12,-.14,z-.15),(s*.035,-.161,z-.27)],cloth)
        c.box('Jacket patch pocket',(s*g*.67,-.114,h+.10),(.075,.021,.073),cloth,.007)
    c.ell('Neck cloth knot',(0,-.084,q['neck']-.041),(.027,.026,.025),red)
    if asset in ['lookout','bomber']:
        c.tube('Wrapped neckerchief',[(-.052,.015,q['neck']+.004),(-.047,-.044,q['neck']-.005),(0,-.079,q['neck']-.034),(.047,-.044,q['neck']-.005),(.052,.015,q['neck']+.004)],.013,red)
        c.patch('Short scarf ends',[(-.022,-.108,q['neck']-.036),(.025,-.112,q['neck']-.041),(.039,-.15,z-.10),(0,-.158,z-.085),(-.024,-.145,z-.12)],red)
    else:c.patch('Neck cloth ends',[(-.02,-.113,q['neck']-.041),(.025,-.116,q['neck']-.046),(.052,-.153,z-.18),(0,-.160,z-.145),(-.037,-.149,z-.20)],red)
    if asset=='gunman':
        for s in [-1,1]:
            for i in range(5):
                x=s*(.041+i*.019)
                c.tube('Waistcoat pinstripe',[(x,-.157,h+.048),(x,-.148,z-.21),(x,-.12,z-.07)],.0009,c.leatheredge,1)
        for i in range(6):c.bone('Belt cartridge',(.055+i*.018,-.137,h+.003),(.055+i*.018,-.137,h+.061),.005,.005,brass,12)
    if asset=='bomber':
        for s in [-1,1]:c.ribbon('Dungaree strap',[(s*.074,-.161,h+.12),(s*.082,-.146,z-.20),(s*.10,-.075,z+.012),(s*.11,.05,z+.048)],.031,green)
        c.patch('Canvas elbow patch',[(.49,-.058,z-.05),(.56,-.055,z-.036),(.56,-.056,z+.045),(.49,-.059,z+.05)],c.leatheredge)
    if asset=='gang-boss':
        # Split front and back coat panels deform with the legs; cloth simulation is separate.
        n=36;verts=[];faces=[]
        for zz,rx,ry in [(h-.65,g*1.28,.18),(h-.35,g*1.14,.16),(h+.08,g,.139)]:
            for i in range(n+1):
                a=-2.5+i*5/n;verts.append((rx*math.sin(a),.022+ry*math.cos(a),zz+.008*math.cos(8*a)))
        for j in range(2):
            for i in range(n):k=j*(n+1)+i;faces.append((k,k+1,k+n+2,k+n+1))
        me=bpy.data.meshes.new('Coat tails');me.from_pydata(verts,[],faces);me.materials.append(cloth)
        ob=bpy.data.objects.new('Split long coat',me);c.character.objects.link(ob);mod=ob.modifiers.new('Coat thickness','SOLIDIFY');mod.thickness=.012
        c.tube('Watch chain',[(.01,-.167,h+.23),(.068,-.18,h+.17),(.126,-.144,h+.22)],.0026,brass)
    root=c.root_for(asset)
    c.character_sockets(root,palms,(g+.06,.025,h), (0,.215,z-.18), (0,-.225,z-.16))
    # The satchel itself is external; provide a grenade pocket point on the body.
    if asset=='bomber':
        ob=c.empty('SOCKET_grenade_pouch',root,(-g-.15,-.012,h+.025),c.frame((0,-1,0)));ob['future_bone']='pelvis'
    return root

def prop():
    anchors={};stock=walnut
    if asset in ['gunman-rifle','gang-boss-smg','lookout-pistol']:
        pistol=asset=='lookout-pistol';smg=asset=='gang-boss-smg'
        if pistol:
            c.box('Pistol slide',(.06,0,.029),(.185,.031,.040),steel,.004)
            c.box('Pistol grip',(-.014,0,-.026),(.044,.035,.081),brown,.004,rotation=(0,.20,0))
            c.bone('Straight barrel',(.03,0,.033),(.16,0,.033),.008,.008,steel)
            muzzle=.164;grip=(-.014,0,-.024);support=None
        else:
            c.patch('Walnut buttstock',[(-.46,0,.036),(-.29,0,.04),(-.12,0,.009),(-.07,0,-.018),(-.13,0,-.06),(-.43,0,-.11),(-.46,0,-.08)],stock,.048,.008)
            c.box('Receiver',(.0,0,.02),(.22,.052,.058),steel,.004)
            c.bone('Perfectly straight barrel',(.09,0,.031),(.55 if not smg else .43,0,.031),.012,.012,steel,32)
            muzzle=.55 if not smg else .43
            c.box('Walnut fore-end',(.215,0,-.005),(.21,.041,.042),stock,.010)
            grip=(-.113,0,-.024);support=(.205,0,-.015)
            if smg:
                c.box('Pistol grip',(-.04,0,-.06),(.037,.043,.094),stock,.008,rotation=(0,-.22,0));grip=(-.04,0,-.056)
                c.bone('Drum magazine',(.10,-.049,-.085),(.10,.049,-.085),.075,.075,steel,48)
                for i in range(10):c.loop('Barrel cooling fin',(.29+i*.009,0,.031),.017,steel,axis='X',wire=.003)
            else:
                c.bone('Tubular magazine',(.1,0,.005),(.505,0,.005),.008,.008,steel)
                c.tube('Lever loop',[(-.10,-.007,-.027),(-.078,-.007,-.089),(.026,-.007,-.079),(.055,-.007,-.02)],.0045,steel)
        c.tube('Trigger guard',[(-.025,-.007,-.018),(-.007,-.007,-.057),(.047,-.007,-.05),(.051,-.007,-.019)],.003,steel)
        c.box('Front sight',(muzzle-.025,0,.047),(.012,.008,.024),steel,.002)
        anchors={'ANCHOR_grip_r':grip,'ANCHOR_stow':(0,0,0),'ANCHOR_muzzle':(muzzle,0,.031)}
        if support:anchors['ANCHOR_support_l']=support
    elif asset=='bruiser-truncheon':
        c.bone('Weighted wooden club',(-.15,0,0),(.36,0,0),.028,.046,stock,32)
        c.bone('Leather grip',(-.19,0,0),(-.055,0,0),.022,.024,brown,32)
        for i in range(9):c.loop('Grip wrap',(-.18+i*.014,0,0),.023,c.leatheredge,axis='X',wire=.0018)
        anchors={'ANCHOR_grip_r':(-.115,0,0),'ANCHOR_stow':(-.14,0,0)}
    elif asset=='lookout-binoculars':
        for s in [-1,1]:
            c.bone('Binocular barrel',(-.05,s*.057,0),(.087,s*.057,0),.024,.038,steel,32)
            c.loop('Brass objective rim',(.086,s*.057,0),.037,brass,axis='X',wire=.004)
            c.bone('Objective glass',(.087,s*.057,0),(.090,s*.057,0),.032,.032,c.material('Smoky green glass',(.021,.082,.065),.14,.4),32)
        c.box('Binocular hinge',(-.025,0,0),(.028,.072,.023),brass,.005)
        anchors={'ANCHOR_grip_r':(0,-.062,0),'ANCHOR_support_l':(0,.062,0),'ANCHOR_stow':(0,0,0),'ANCHOR_optical_axis':(.09,0,0)}
    elif asset=='bomber-grenade':
        c.ell('Closed grenade body',(0,0,0),(.031,.031,.046),green,32,24)
        for z in [-.025,-.012,0,.012,.025]:c.loop('Cast groove',(0,0,z),.029*(1-(z/.060)**2),steel,axis='Z',wire=.0015)
        c.box('Closed lever',(.008,0,.050),(.045,.011,.009),steel,.002)
        c.loop('Safety ring',(-.014,0,.057),.013,brass,wire=.0018)
        anchors={'ANCHOR_grip_r':(0,0,0),'ANCHOR_stow':(0,0,.035)}
    elif asset=='bomber-satchel':
        c.box('Leather satchel',(0,0,-.08),(.19,.125,.23),brown,.022)
        c.box('Folded bag flap',(0,-.066,-.011),(.197,.024,.096),brown,.012)
        for s in [-1,1]:
            c.ribbon('Bag fastening',[(s*.055,-.083,.021),(s*.055,-.083,-.14)],.018,c.leatheredge)
            c.buckle('Bag buckle',s*.055,-.09,-.091,.024,.030)
        c.tube('Satchel handle',[(-.055,0,.04),(-.046,0,.097),(.046,0,.097),(.055,0,.04)],.008,brown)
        anchors={'ANCHOR_grip_r':(0,0,.094),'ANCHOR_stow':(0,.074,0),'ANCHOR_grenade_pocket':(-.042,0,.035)}
    else:raise ValueError(asset)
    root=c.root_for(asset)
    for name,pos in anchors.items():
        rot=None
        if asset in ['bomber-satchel','bomber-grenade']:rot=c.frame((0,-1,0))
        if name=='ANCHOR_stow' and asset in ['bruiser-truncheon','lookout-pistol','lookout-binoculars']:
            rot=c.frame((0,0,1),(-1,0,0))
        c.empty(name,root,pos,rotation=rot,role='item_anchor')
    return root

root=character() if asset in CONFIG else prop()
c.studio(height=2.12,width=2.6,item=asset not in CONFIG)
# Save editable multi-part source, then convert the actual delivery mesh.
bpy.context.view_layer.update()
locators={o.name:{'matrix_local_blender':[list(r) for r in o.matrix_local],'role':o.get('role'),'future_bone':o.get('future_bone')} for o in c.character.objects if o.type=='EMPTY' and o!=root}
for ob in c.character.objects:
    if ob.name in locators:
        flip=Matrix.Diagonal((1,-1,1,1));q=(flip@ob.matrix_local@flip).to_quaternion()
        locators[ob.name]['unreal_quaternion_xyzw']=[q.x,q.y,q.z,q.w]
(OUT/(asset+'-locators.json')).write_text(json.dumps(locators,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(asset+'.blend')))
bpy.ops.object.select_all(action='DESELECT')
parts=[o for o in c.character.objects if o.type in {'MESH','CURVE'}]
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.convert(target='MESH');bpy.ops.object.join()
mesh=bpy.context.object;mesh.name=asset.replace('-','_')+'_mesh'
if asset in ['lookout','bomber']:
    # Tailor the clothing silhouette while retaining the reference's workwear.
    q=CONFIG[asset];h=q['hip'];z=q['shoulder'];mw=mesh.matrix_world;inv=mw.inverted()
    for v in mesh.data.vertices:
        co=mw@v.co;zz=co.z
        if h-.10<zz<z-.085 and abs(co.x)<q['girth']*1.45:
            waist=math.exp(-((zz-(h+.045))/.14)**2)
            co.x*=1-(.15 if asset=='lookout' else .105)*waist
            chest=math.exp(-((zz-(z-.19))/.09)**2)
            if co.y<-.055:co.y-=(.022 if asset=='lookout' else .029)*chest
            v.co=inv@co
mod=mesh.modifiers.new('Concept reduction','DECIMATE');mod.ratio=.34 if asset in CONFIG else .65;bpy.ops.object.modifier_apply(modifier=mod.name)
for o in c.character.objects:o.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(OUT/(asset+'.glb')),export_format='GLB',use_selection=True,export_apply=True,export_extras=True)
if asset not in CONFIG:
    bpy.ops.object.select_all(action='DESELECT');mesh.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(island_margin=.01);bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.export_scene.fbx(filepath=str(OUT/(asset+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,mesh_smooth_type='FACE')
    mats={}
    for mat in mesh.data.materials:
        n=mat.node_tree.nodes.get('Principled BSDF');mats[mat.name]={'color':list(n.inputs['Base Color'].default_value),'roughness':n.inputs['Roughness'].default_value,'metallic':n.inputs['Metallic'].default_value}
    (OUT/(asset+'-materials.json')).write_text(json.dumps(mats,indent=2))
print('MODEL_COMPLETE',asset,len(mesh.data.vertices),len(mesh.data.polygons))
