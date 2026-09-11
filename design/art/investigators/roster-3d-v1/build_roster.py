"""Build one original character/prop, or upgrade Sapper with attachment nodes.

blender --background --factory-startup --python build_roster.py -- photographer
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import asset_common as c
import bpy
import math
from mathutils import Vector

asset=sys.argv[sys.argv.index('--')+1]
c.set_asset(asset)

def coat_tail(mat,trim):
    verts=[];faces=[];n=48
    for row,(z,rx,ry) in enumerate([(.27,.30,.19),(.55,.26,.17),(.82,.215,.14),(1.075,.153,.116)]):
        for i in range(n+1):
            a=-2.53+i*5.06/n
            fold=1+.035*math.cos(a*10)
            verts.append((rx*math.sin(a)*fold,.024+ry*math.cos(a)*fold,z+.018*math.sin(a*5) if row==0 else z))
    for row in range(3):
        for i in range(n):
            k=row*(n+1)+i;faces.append((k,k+1,k+n+2,k+n+1))
    mesh=bpy.data.meshes.new('Long sweeping coat tails');mesh.from_pydata(verts,[],faces);mesh.update()
    ob=bpy.data.objects.new('Split velvet coat tails',mesh);c.character.objects.link(ob);mesh.materials.append(mat)
    for poly in mesh.polygons:poly.use_smooth=True
    mod=ob.modifiers.new('Velvet thickness','SOLIDIFY');mod.thickness=.013
    mod=ob.modifiers.new('Flowing tailoring','SUBSURF');mod.levels=2
    for s in [-1,1]:
        c.tube('Embroidered tail edge',[(s*.09,-.072,1.065),(s*.14,-.112,.82),(s*.166,-.116,.55),(s*.173,-.13,.27)],.006,trim)
        for j in range(10):
            z=.32+j*.07;x=s*(.17-(z-.32)*.11)
            c.tube('Gold vine embroidery',[(x,-.135,z),(x+s*.018,-.135,z+.02),(x,-.134,z+.043)],.0028,trim,2)

def female(kind):
    medium=kind=='medium'
    cloth=c.material('Plum velvet' if medium else 'Ochre expedition canvas',(.19,.017,.074) if medium else (.43,.26,.083),.78,bump=.006)
    trim=c.material('Antique gold embroidery' if medium else 'Worn canvas edges',(.45,.28,.11) if medium else (.50,.33,.13),.58,.2 if medium else 0)
    trousers=c.material('Charcoal tailored trousers',(.047,.052,.050),.8,bump=.006)
    blouse=c.material('Ivory blouse',(.70,.63,.48),.8,bump=.003)
    skin=c.material('Medium skin' if medium else 'Photographer skin',(.62,.40,.28) if medium else (.39,.19,.08),.65,bump=.002)
    hair=c.material('Dark waved bob' if medium else 'Black curls',(.022,.019,.023),.75)
    silver=c.material('Silver forelock',(.55,.57,.52),.7)
    boots=c.material('Pointed black boots' if medium else 'Brown expedition boots',(.035,.03,.028) if medium else (.18,.072,.028),.55,bump=.003)
    legs=c.bootlegs(.113,1.025,.62,.34 if not medium else .27,trousers,boots,.93)
    if medium:
        for s in [-1,1]:
            c.ell('Pointed boot toe',(s*.113,-.166,.085),(.049,.070,.037),boots)
    legs.append(c.ell('Tailored trouser hips',(0,.025,.986),(.171,.12,.15),trousers))
    c.fuse(legs,'Continuous tailored trousers',.005)
    torso=[c.ell('Fitted jacket torso',(0,.018,1.18),(.174,.12,.226),cloth),c.ell('Shoulder line',(0,.022,1.365),(.221,.112,.072),cloth)]
    c.ell('Neck',(0,.005,1.479),(.043,.047,.082),skin)
    shoulder=1.365;wrist=.693
    for s in [-1,1]:
        torso.append(c.ell('Jacket shoulder',(s*.199,.022,shoulder),(.106,.102,.092),cloth))
        torso.append(c.bone('Jacket upper sleeve',(s*.20,.022,shoulder),(s*.467,.022,shoulder),.093,.074,cloth))
        if medium:
            torso.append(c.bone('Velvet forearm sleeve',(s*.445,.022,shoulder),(s*.66,.022,shoulder),.075,.072,cloth))
            c.bone('Lace cuff',(s*.65,.022,shoulder),(s*.697,.022,shoulder),.066,.07,blouse)
            for j in range(7):
                a=j*math.tau/7
                c.ell('Lace cuff scallop',(s*.69,.022+.065*math.cos(a),shoulder+.065*math.sin(a)),(.012,.012,.012),blouse,16,10)
        else:
            c.bone('Rolled shirt sleeve',(s*.438,.022,shoulder),(s*.486,.022,shoulder),.079,.076,blouse)
            for j in range(3):c.loop('Cuff roll',(s*(.442+j*.014),.022,shoulder),.075,trim,'X',.004)
            c.bone('Bare forearm',(s*.474,.022,shoulder),(s*.705,.022,shoulder),.052,.03,skin)
    c.fuse(torso,'Tailored '+kind+' jacket and sleeves',.005)
    c.ell('Ivory blouse front',(0,-.117,1.256),(.087,.028,.184),blouse)
    for s in [-1,1]:
        c.patch('Ivory shirt collar',[(s*.043,-.037,1.481),(s*.084,-.092,1.429),(s*.04,-.149,1.39),(s*.016,-.128,1.44)],blouse)
    for s in [-1,1]:
        c.patch('Jacket lapel',[(s*.073,-.113,1.439),(s*.156,-.10,1.368),(s*.103,-.15,1.21),(s*.052,-.143,1.31)],trim if not medium else cloth)
    if medium:
        coat_tail(cloth,trim)
        c.bone('Wide waist belt',(-.153,-.07,1.07),(.153,-.07,1.07),.021,.021,c.leather)
        c.buckle('Decorative waist buckle',0,-.151,1.066,.059,.044)
        c.loop('Pendant setting',(0,-.151,1.275),.022,c.brass,wire=.004)
        jewel=c.material('Garnet pendant',(.22,.013,.047),.28,.2)
        c.ell('Garnet pendant',(0,-.154,1.275),(.017,.008,.022),jewel)
        c.tube('Pendant chain',[(-.05,-.061,1.46),(-.04,-.134,1.38),(0,-.15,1.299),(.047,-.125,1.395),(.053,-.052,1.46)],.0026,c.brass)
        for s in [-1,1]:
            c.tube('Waist chain',[(s*.06,-.146,1.053),(s*.10,-.16,.952),(s*.148,-.126,1.045)],.002,c.brass)
            for j in range(3):
                c.tube('Gold tassel',[(s*(.122+j*.008),-.134,.946),(s*(.123+j*.008),-.142,.884)],.0024,trim)
    else:
        rust=c.material('Rust neck scarf',(.30,.058,.025),.85)
        c.patch('Knotted neck scarf',[(-.047,-.056,1.473),(.046,-.056,1.473),(.035,-.142,1.39),(.018,-.151,1.26),(-.029,-.14,1.382)],rust)
        c.ell('Scarf knot',(0,-.126,1.406),(.024,.019,.024),rust)
        for s in [-1,1]:
            for z in [1.26,1.062]:
                c.box('Field jacket pocket',(s*.123,-.104,z),(.081,.031,.074),cloth,.012)
                c.box('Pocket flap',(s*.123,-.126,z+.025),(.085,.014,.032),trim,.006)
                c.bone('Pocket button',(s*.123,-.137,z+.02),(s*.123,-.144,z+.02),.006,.006,c.brass,12)
        c.ribbon('Camera shoulder strap',[(-.141,.088,1.18),(-.151,.045,1.419),(-.135,-.105,1.4),(-.053,-.148,1.2),(.153,-.105,1.003)],.034,c.leather)
        c.buckle('Shoulder strap buckle',-.082,-.16,1.263,.031,.043)
        c.box('Film satchel',(.204,.016,.974),(.12,.116,.153),c.leather,.023)
        c.box('Satchel flap',(.206,-.048,1.014),(.125,.02,.065),c.leatheredge,.012)
        c.ribbon('Film satchel tab',[(.206,-.053,1.05),(.206,-.066,.942)],.021,c.leather)
        c.buckle('Film satchel buckle',.206,-.072,.989,.028,.033)
    palms=c.hands(wrist,shoulder,skin,.86)
    if medium:
        for s in [-1,1]:c.loop('Antique hand ring',(s*.80,.018,shoulder),.008,c.brass,'X',.002)
    c.face(kind,1.643,skin,hair,silver)
    # Keep the approved workwear/velvet costume, with a fitted waist and a
    # softer torso profile. Convert curves individually to retain editable parts.
    bpy.ops.object.select_all(action='DESELECT')
    parts=[ob for ob in c.character.objects if ob.type in {'MESH','CURVE'}]
    for ob in parts:ob.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.convert(target='MESH')
    for ob in c.character.objects:
        if ob.type!='MESH':continue
        mw=ob.matrix_world.copy();inv=mw.inverted()
        for v in ob.data.vertices:
            co=mw@v.co
            if .97<co.z<1.29 and abs(co.x)<.194:
                co.x*=1-(.16 if medium else .12)*math.exp(-((co.z-1.095)/.115)**2)
                if co.y<-.055:co.y-=.023*math.exp(-((co.z-1.22)/.065)**2)
                v.co=inv@co
    root=c.root_for(kind)
    c.character_sockets(root,palms,(.18,-.045,1.02),(.0,.184,1.22),(0,-.153,1.12),fx=medium)
    c.studio(1.81,2.08)
    c.save_export(kind,root,.24)

def smuggler():
    skin=c.material('Deep warm smuggler skin',(.225,.083,.043),.6,bump=.002)
    shirt=c.material('Faded blue work shirt',(.15,.23,.27),.85,bump=.007)
    shirtedge=c.material('Shirt turned cuffs',(.26,.33,.34),.8)
    trousers=c.material('Dark dockside trousers',(.054,.066,.065),.78,bump=.007)
    sash=c.material('Burgundy sash',(.27,.033,.034),.85,bump=.004)
    boots=c.material('Heavy dock boots',(.079,.049,.029),.62,bump=.005)
    beard=c.material('Short black beard',(.017,.014,.012),.83)
    legs=c.bootlegs(.164,1.13,.65,.23,trousers,boots,1.45)
    legs.append(c.ell('Broad trouser hips',(0,.038,1.057),(.259,.164,.168),trousers))
    c.fuse(legs,'Continuous dockside trousers',.006)
    torso=[c.ell('Heavy shirt torso',(0,.035,1.323),(.29,.182,.258),shirt),c.ell('Broad chest',(0,.037,1.509),(.321,.181,.138),shirt)]
    z=1.56;wrist=.867
    for s in [-1,1]:
        torso.append(c.ell('Powerful shoulder',(s*.282,.024,z),(.14,.153,.147),shirt))
        torso.append(c.bone('Short work shirt sleeve',(s*.29,.025,z),(s*.58,.025,z),.144,.116,shirt))
        c.bone('Rolled work cuff',(s*.566,.025,z),(s*.618,.025,z),.122,.116,shirtedge)
        parts=[c.bone('Muscular bare forearm',(s*.605,.025,z),(s*.887,.025,z),.099,.052,skin),c.ell('Forearm muscle',(s*.705,.025,z),(.104,.102,.094),skin)]
        c.fuse(parts,'Muscular forearm',.005)
        c.tube('Forearm tendon',[(s*.69,-.044,z+.057),(s*.78,-.016,z+.057),(s*.849,.009,z+.038)],.003,skin)
    c.fuse(torso,'Broad rolled-sleeve shirt',.006)
    c.ell('Strong neck',(0,.012,1.724),(.091,.089,.114),skin)
    c.ell('Open shirt chest',(0,-.13,1.577),(.082,.045,.124),skin)
    for s in [-1,1]:
        c.patch('Turned shirt collar',[(s*.085,-.134,1.70),(s*.151,-.116,1.65),(s*.098,-.195,1.47),(s*.057,-.172,1.61)],shirtedge)
        c.ribbon('Leather suspender',[(s*.219,.154,1.10),(s*.23,.115,1.61),(s*.222,-.063,1.662),(s*.187,-.184,1.41),(s*.175,-.167,1.126)],.043,c.leather)
        c.buckle('Suspender adjuster',s*.18,-.19,1.275,.04,.053)
    for i in range(6):
        zz=1.075+i*.016
        pts=[(.268*math.cos(a),.037+.179*math.sin(a),zz+.006*math.cos(3*a)) for a in [j*math.tau/48 for j in range(49)]]
        c.tube('Wrapped sash fold',pts,.014,sash)
    c.patch('Hanging sash tail',[(.215,-.087,1.16),(.28,-.042,1.115),(.291,-.093,.87),(.239,-.124,.80),(.223,-.11,1.03)],sash,.013)
    palms=c.hands(wrist,z,skin,1.38)
    c.face('smuggler',1.849,skin,beard,beard,1.16)
    root=c.root_for('smuggler')
    c.character_sockets(root,palms,(.28,-.026,1.10),(0,.235,1.36),(0,-.206,1.30))
    root['held_items']='None: bare-knuckle combat. Holster locators are reserves only.'
    c.studio(2.0,2.60)
    c.save_export('smuggler',root,.24)

def camera():
    black=c.material('Black camera leather',(.034,.03,.024),.75,bump=.003)
    bellows=c.material('Dark bellows rubber',(.048,.036,.026),.9)
    glass=c.material('Blue lens glass',(.022,.058,.069),.16,.35)
    c.box('Camera rear body',(-.058,0,0),(.04,.185,.135),black,.009)
    c.box('Folding baseboard',(.025,0,-.079),(.22,.20,.012),c.wood,.004)
    for i in range(9):
        x=-.03+i*.011
        c.box('Bellows accordion fold',(x,0,0),(.008,.139-i*.004,.112-i*.003),bellows,.005)
    c.box('Lens standard',(.075,0,0),(.013,.128,.101),c.metal,.006)
    c.bone('Lens brass mount',(.078,0,0),(.093,0,0),.045,.045,c.brass,40)
    c.bone('Lens barrel',(.091,0,0),(.137,0,0),.039,.032,c.metal,40)
    c.bone('Front optical glass',(.137,0,0),(.14,0,0),.027,.027,glass,40)
    c.loop('Lens rim',(.14,0,0),.031,c.brass,'X',.003)
    for side in [-1,1]:
        c.bone('Folding brace',(-.042,side*.092,-.063),(.069,side*.085,-.052),.0025,.0025,c.steel)
        c.bone('Side grip handle',(-.063,side*.106,-.042),(-.063,side*.106,.034),.013,.013,black)
    c.box('Shutter button',(-.044,-.074,.08),(.018,.018,.012),c.brass,.003)
    c.box('Viewfinder',(-.063,.038,.084),(.029,.036,.029),black,.004)
    c.bone('Flash stem',(-.04,-.12,.025),(-.04,-.12,.17),.009,.007,c.metal)
    # Concave reflector bowl faces +X along with the camera lens.
    verts=[(-.048,-.12,.201)];faces=[];n=48
    for radius,x in [(.02,-.04),(.043,-.027),(.066,-.010),(.075,.002)]:
        for i in range(n):
            a=i*math.tau/n;verts.append((x,-.12+radius*math.cos(a),.201+radius*math.sin(a)))
    for i in range(n):faces.append((0,1+i,1+(i+1)%n))
    for row in range(3):
        for i in range(n):
            a=1+row*n+i;b=1+row*n+(i+1)%n;faces.append((a,b,b+n,a+n))
    mesh=bpy.data.meshes.new('Flash reflector');mesh.from_pydata(verts,[],faces);mesh.update()
    ob=bpy.data.objects.new('Concave flash reflector',mesh);c.character.objects.link(ob);mesh.materials.append(c.steel)
    for p in mesh.polygons:p.use_smooth=True
    mod=ob.modifiers.new('Reflector thickness','SOLIDIFY');mod.thickness=.002
    c.loop('Flash reflector rim',(.002,-.12,.201),.075,c.brass,'X',.0025)
    bulb=c.material('Flashbulb pearl',(.7,.68,.52),.2)
    c.ell('Flashbulb',(-.009,-.12,.201),(.027,.016,.02),bulb)
    root=c.root_for('photographer-camera')
    c.empty('ANCHOR_grip_r',root,(-.063,-.106,-.01))
    c.empty('ANCHOR_support_l',root,(.028,.07,-.088))
    c.empty('ANCHOR_stow',root,(-.082,0,0))
    c.empty('ANCHOR_optical_axis',root,(.14,0,0),role='aim')
    c.studio(.35,.60,True)
    cam=bpy.context.scene.camera;cam.rotation_euler=(Vector((.015,-.015,.09))-cam.location).to_track_quat('-Z','Y').to_euler()
    c.save_export('photographer-camera',root,.65)

def rifle():
    # A continuous shaped walnut stock, with a straight precision-rifle barrel.
    profile=[(-.405,-.078),(-.405,.014),(-.29,.026),(-.22,.026),(-.17,.003),(-.11,-.006),(-.075,.015),(.205,.015),(.205,-.027),(-.05,-.027),(-.09,-.049),(-.13,-.053),(-.18,-.038),(-.23,-.066)]
    verts=[(x,y,z) for y in [-.023,.023] for x,z in profile];n=len(profile)
    faces=[tuple(range(n-1,-1,-1)),tuple(range(n,n*2))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    me=bpy.data.meshes.new('Single shaped stock');me.from_pydata(verts,[],faces);me.update()
    ob=bpy.data.objects.new('Walnut shoulder stock and fore-end',me);c.character.objects.link(ob);me.materials.append(c.wood)
    mod=ob.modifiers.new('Stock edge rounding','BEVEL');mod.width=.007;mod.segments=3
    ob.modifiers.new('Stock normals','WEIGHTED_NORMAL')
    c.box('Buttplate',(-.408,0,-.03),(.009,.05,.10),c.metal,.005)
    c.bone('Straight precision barrel',(-.04,0,.021),(.565,0,.021),.014,.008,c.metal,32)
    c.box('Receiver',(-.035,0,.022),(.14,.04,.04),c.metal,.008)
    c.bone('Muzzle',(.55,0,.021),(.571,0,.021),.011,.011,c.steel,24)
    c.bone('Bore',(.571,0,.021),(.572,0,.021),.0065,.0065,c.pupil,24)
    c.bone('Bolt handle',(-.07,-.015,.026),(-.071,-.059,.009),.005,.005,c.steel)
    c.ell('Bolt knob',(-.071,-.059,.009),(.012,.012,.012),c.metal,20,12)
    c.tube('Trigger guard',[(-.12,0,-.019),(-.117,0,-.068),(-.055,0,-.061),(-.048,0,-.02)],.0035,c.metal)
    c.tube('Trigger',[(-.097,0,-.023),(-.093,0,-.042)],.0025,c.steel)
    c.box('Rear sight',(.039,0,.043),(.02,.02,.015),c.metal,.003)
    c.box('Front sight',(.53,0,.032),(.01,.012,.026),c.metal,.002)
    c.bone('Front barrel band',(.175,0,.012),(.188,0,.012),.029,.029,c.metal)
    c.tube('Rifle sling',[(-.354,.019,-.063),(-.17,.03,-.166),(.20,.03,-.16),(.31,.019,.008)],.007,c.leather)
    c.bone('Sling front swivel',(.31,.005,.012),(.31,.019,.008),.004,.004,c.metal)
    root=c.root_for('photographer-rifle')
    c.empty('ANCHOR_grip_r',root,(-.125,0,-.035))
    c.empty('ANCHOR_support_l',root,(.16,0,-.031))
    c.empty('ANCHOR_stow',root,(0,-.029,0))
    c.empty('ANCHOR_muzzle',root,(.572,0,.021),role='aim')
    c.studio(.3,1.20,True);c.save_export('photographer-rifle',root,.65)

def spirit():
    spectral=c.material('Pale teal spectral form',(.20,.70,.57),.4)
    bs=spectral.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Emission Color'].default_value=(.08,.45,.32,1);bs.inputs['Emission Strength'].default_value=.7
    bs.inputs['Alpha'].default_value=.55
    spectral.surface_render_method='DITHERED'
    for strand in range(3):
        verts=[];faces=[];n=52;m=10
        for i in range(n):
            t=i/(n-1);a=t*math.tau*1.25+strand*2.1
            r=(.025+.025*math.sin(t*math.pi))*math.sin(t*math.pi*.9)
            center=Vector((r*math.cos(a),r*math.sin(a),t*.33))
            rad=(.012 if strand==0 else .007)*math.sin(math.pi*(.04+.94*t))
            for j in range(m):
                b=j*math.tau/m;verts.append(tuple(center+Vector((rad*math.cos(b),rad*math.sin(b),0))))
        for i in range(n-1):
            for j in range(m):
                k=i*m+j;faces.append((k,i*m+(j+1)%m,(i+1)*m+(j+1)%m,k+m))
        me=bpy.data.meshes.new('Spectral ribbon');me.from_pydata(verts,[],faces);me.update()
        ob=bpy.data.objects.new('Rising spirit wisp',me);c.character.objects.link(ob);me.materials.append(spectral)
        for p in me.polygons:p.use_smooth=True
    root=c.root_for('medium-spirit-wisp');root['inactive_behavior']='Hidden; not physically holstered.'
    c.empty('ANCHOR_emit',root,(0,0,0),role='effect')
    c.studio(.35,.50,True)
    cam=bpy.context.scene.camera;cam.location=(.7,-1,.5);cam.rotation_euler=(Vector((0,0,.165))-cam.location).to_track_quat('-Z','Y').to_euler()
    c.save_export('medium-spirit-wisp',root,.65)

def upgrade_sapper(gun=False):
    id='sapper-carbine' if gun else 'sapper'
    old='sapper-carbine' if gun else 'sapper-tpose'
    source=c.OUT/'source'/(old+'.blend')
    if not source.exists():source=c.OUT.parent/'sapper-3d-v2'/(old+'.blend')
    bpy.ops.wm.open_mainfile(filepath=str(source))
    c.character=bpy.data.collections['Carbine | separate asset' if gun else 'Sapper | character meshes']
    c.stage=bpy.data.collections['Presentation | not exported']
    c.character.name=id+' | editable geometry'
    root=bpy.data.objects['Carbine_Root' if gun else 'Sapper_Concept_Root'];root.name=id+'_root'
    if gun:
        c.empty('ANCHOR_grip_r',root,(-.045,0,-.027))
        c.empty('ANCHOR_support_l',root,(.16,0,-.025))
        c.empty('ANCHOR_stow',root,(0,-.03,0))
        c.empty('ANCHOR_muzzle',root,(.428,0,0),role='aim')
    else:
        c.character_sockets(root,{'r':(-.842,.025,1.437),'l':(.842,.025,1.437)},(.23,-.01,1.04),(0,.338,1.285),(0,-.182,1.19))
    c.save_export(id,root,.5 if gun else .14)

if asset in ['photographer','medium']:female(asset)
elif asset=='smuggler':smuggler()
elif asset=='photographer-camera':camera()
elif asset=='photographer-rifle':rifle()
elif asset=='medium-spirit-wisp':spirit()
elif asset=='sapper':upgrade_sapper()
elif asset=='sapper-carbine':upgrade_sapper(True)
else:raise ValueError(asset)
