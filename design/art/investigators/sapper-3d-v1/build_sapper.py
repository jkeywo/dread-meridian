"""Rebuild the provisional Sapper concept model in Blender 5.0.

Run: blender --background --factory-startup --python build_sapper.py
The source illustration is a visual reference, not a texture or flat billboard.
"""
import bpy
import math
import random
from pathlib import Path
from mathutils import Vector

OUT = Path(__file__).resolve().parent
random.seed(710)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
character = bpy.data.collections.new('Sapper | character meshes')
scene.collection.children.link(character)
stage = bpy.data.collections.new('Presentation | not exported')
scene.collection.children.link(stage)

def move(obj, collection=character):
    for col in list(obj.users_collection):
        col.objects.unlink(obj)
    collection.objects.link(obj)
    return obj

def material(name, color, roughness=.65, metallic=0, bump=0):
    mat = bpy.data.materials.new(name)
    color=tuple(c*.60 for c in color) if name not in ['Studio floor','Pupils and mouth crease'] else color
    mat.diffuse_color = (*color, 1)
    mat.use_nodes = True
    nt = mat.node_tree
    bs = nt.nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value = (*color, 1)
    bs.inputs['Roughness'].default_value = roughness
    bs.inputs['Metallic'].default_value = metallic
    if bump:
        noise = nt.nodes.new('ShaderNodeTexNoise')
        noise.inputs['Scale'].default_value = 155
        noise.inputs['Detail'].default_value = 3
        node = nt.nodes.new('ShaderNodeBump')
        node.inputs['Strength'].default_value = .23
        node.inputs['Distance'].default_value = bump
        nt.links.new(noise.outputs['Fac'], node.inputs['Height'])
        nt.links.new(node.outputs['Normal'], bs.inputs['Normal'])
    return mat

coat = material('Worn olive wool', (.19,.205,.115), bump=.012)
edge = material('Raised wool seams', (.27,.28,.16), bump=.008)
darkcloth = material('Coat shadow and trousers', (.105,.125,.079), bump=.01)
scarf = material('Faded khaki scarf', (.29,.285,.18), bump=.007)
leather = material('Oiled brown leather', (.16,.073,.035), .48, bump=.007)
leatheredge = material('Leather edge wear', (.31,.177,.087), .6)
bootmat = material('Weathered boots', (.105,.071,.047), .58, bump=.009)
sole = material('Dark soles', (.042,.036,.025), .86)
skin = material('Weathered skin', (.49,.285,.165), .65, bump=.003)
skinlight = material('Knuckles and scar', (.57,.345,.215), .68)
lip = material('Muted lips and ear recess', (.30,.135,.087), .75)
hair = material('Iron grey hair', (.135,.15,.14), .85)
hairlight = material('Grey hair streaks', (.28,.30,.275), .85)
eye = material('Warm grey sclera', (.55,.535,.425), .43)
iris = material('Hazel iris', (.17,.13,.057), .4)
pupil = material('Pupils and mouth crease', (.022,.019,.013), .65)
metal = material('Worn gunmetal', (.075,.091,.097), .38,.8)
steel = material('Exposed steel edges', (.23,.27,.27), .32,.8)
brass = material('Oxidised brass', (.34,.245,.10), .45,.75)
wood = material('Carbine walnut', (.205,.089,.036), .49, bump=.005)

def finish(obj, name, mat, smooth=True):
    obj.name = name
    move(obj)
    if mat:
        obj.data.materials.append(mat)
    if smooth and obj.type == 'MESH':
        for p in obj.data.polygons:
            p.use_smooth = True
    return obj

def ell(name, loc, scale, mat, seg=24, rings=16):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=rings, location=loc)
    obj=finish(bpy.context.object,name,mat)
    obj.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return obj

def box(name, loc, scale, mat, bevel=.01, rotation=None):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    obj=finish(bpy.context.object,name,mat,False)
    obj.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if rotation:
        obj.rotation_euler=rotation
    if bevel:
        mod=obj.modifiers.new('Soft worn edges','BEVEL'); mod.width=bevel; mod.segments=3
        obj.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
    return obj

def tube(name, pts, radius, mat, res=3):
    cu=bpy.data.curves.new(name,'CURVE'); cu.dimensions='3D'; cu.resolution_u=10
    sp=cu.splines.new('BEZIER'); sp.bezier_points.add(len(pts)-1)
    for bp,co in zip(sp.bezier_points,pts):
        bp.co=co; bp.handle_left_type='AUTO'; bp.handle_right_type='AUTO'
    cu.bevel_depth=radius; cu.bevel_resolution=res; cu.use_fill_caps=True
    ob=bpy.data.objects.new(name,cu); character.objects.link(ob); cu.materials.append(mat)
    return ob

def bone(name, a, b, r1, r2, mat, vertices=20):
    a,b=Vector(a),Vector(b)
    bpy.ops.mesh.primitive_cone_add(vertices=vertices,radius1=r1,radius2=r2,depth=(b-a).length,location=(a+b)/2)
    ob=finish(bpy.context.object,name,mat)
    ob.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler()
    bevel=ob.modifiers.new('Soft edge','BEVEL'); bevel.width=min(r1,r2)*.23; bevel.segments=3
    return ob

def patch(name, verts, mat, thickness=.007, bevel=.006):
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],[list(range(len(verts)))]); mesh.update()
    ob=bpy.data.objects.new(name,mesh); character.objects.link(ob); mesh.materials.append(mat)
    sol=ob.modifiers.new('Material thickness','SOLIDIFY'); sol.thickness=thickness
    bev=ob.modifiers.new('Soft tailored edge','BEVEL'); bev.width=bevel; bev.segments=3
    return ob

def ribbon(name, pts, width, mat):
    verts=[]
    for x,y,z in pts:
        verts.extend([(x-width/2,y,z),(x+width/2,y,z)])
    faces=[(i*2,i*2+1,i*2+3,i*2+2) for i in range(len(pts)-1)]
    me=bpy.data.meshes.new(name); me.from_pydata(verts,[],faces); me.update()
    ob=bpy.data.objects.new(name,me); character.objects.link(ob); me.materials.append(mat)
    mod=ob.modifiers.new('Leather thickness','SOLIDIFY'); mod.thickness=.009
    mod=ob.modifiers.new('Rounded leather','BEVEL'); mod.width=.004; mod.segments=2
    return ob

def buckle(name,x,y,z,w=.044,h=.054):
    tube(name,[(x-w/2,y,z-h/2),(x-w/2,y,z+h/2),(x+w/2,y,z+h/2),(x+w/2,y,z-h/2),(x-w/2,y,z-h/2)],.004,brass,2)
    tube(name+' pin',[(x,y-.003,z-h*.35),(x,y-.003,z+h*.4)],.0026,brass,2)

# Boots, trouser volumes, and wound puttees.
for side,x in [('Left',-.155),('Right',.17)]:
    y=.035 if side=='Right' else -.025
    ell(side+' boot sole',(x,y-.065,.055),(.092,.172,.047),sole)
    ell(side+' boot toe',(x,y-.094,.105),(.087,.137,.072),bootmat)
    ell(side+' boot upper',(x,y+.012,.185),(.075,.086,.135),bootmat)
    box(side+' heel',(x,y+.095,.058),(.125,.095,.077),sole,.016)
    for i in range(7):
        z=.16+i*.019
        tube(side+' crossed lace A',[(x-.034,y-.072,z),(x+.034,y-.077,z+.017)],.0032,leatheredge,2)
        tube(side+' crossed lace B',[(x+.034,y-.072,z),(x-.034,y-.077,z+.017)],.0032,leatheredge,2)
    ankle=(x,y,.30); knee=(x+(.015 if x<0 else -.005),.045,.63); hip=(x*.86,.025,.97)
    bone(side+' calf trousers',ankle,knee,.064,.097,darkcloth)
    bone(side+' thigh trousers',knee,hip,.097,.125,darkcloth)
    ell(side+' knee',knee,(.099,.098,.108),darkcloth)
    for i in range(11):
        z=.275+i*.022; rad=.071+i*.0018
        pts=[(x+rad*math.cos(t),y+rad*math.sin(t),z+.009*math.cos(t)) for t in [j*2*math.pi/32 for j in range(33)]]
        tube(side+' puttee wrap',pts,.011,scarf,2)
    for j in range(4):
        z=.59+j*.055
        tube(side+' trouser fold',[(x-.068,-.030,z-.009),(x,-.061,z),(x+.067,-.024,z+.015)],.009,coat,2)

# Continuous flared coat skirt, open in front, with a ragged hem.
steps=56
angles=[-.98*math.pi + i*(1.96*math.pi)/steps for i in range(steps+1)]
rings=[(.55,.29,.19),(.69,.27,.165),(.86,.235,.15),(1.055,.195,.132)]
verts=[]
for ri,(z,rx,ry) in enumerate(rings):
    for i,a in enumerate(angles):
        # a=0 is back, open seam at negative Y.
        fold=1+.045*math.cos(9*a)+.023*math.sin(17*a)
        zz=z+(random.uniform(-.021,.018) if ri==0 else 0)
        verts.append((rx*math.sin(a)*fold,ry*math.cos(a)*fold+.035,zz))
faces=[]
for j in range(len(rings)-1):
    for i in range(steps):
        n=steps+1; faces.append((j*n+i,j*n+i+1,(j+1)*n+i+1,(j+1)*n+i))
me=bpy.data.meshes.new('Tailored coat skirt mesh');me.from_pydata(verts,[],faces);me.update()
ob=bpy.data.objects.new('Continuous open greatcoat skirt',me);character.objects.link(ob);me.materials.append(coat)
for p in me.polygons:p.use_smooth=True
mod=ob.modifiers.new('Heavy wool thickness','SOLIDIFY');mod.thickness=.014
mod=ob.modifiers.new('Soft coat surface','SUBSURF');mod.levels=2
ell('Coat torso',(0,.035,1.225),(.245,.159,.29),coat)
ell('Upper shoulders',(0,.033,1.445),(.283,.153,.12),coat)
ell('Neck',(0,.001,1.584),(.065,.068,.105),skin)
for i in range(5):
    z=1.445+i*.025
    tube('Scarf layered fold',[(-.098,-.041,z+.035),(-.068,-.123,z),(.013,-.143,z-.009),(.091,-.068,z+.021),(.065,.083,z+.039)],.021,scarf)
patch('Left oversized lapel',[(-.218,-.082,1.48),(-.113,-.13,1.573),(-.071,-.184,1.48),(-.139,-.193,1.278),(-.245,-.093,1.403)],edge)
patch('Right oversized lapel',[(.207,-.089,1.484),(.107,-.129,1.572),(.072,-.183,1.463),(.111,-.195,1.282),(.231,-.099,1.392)],edge)
for x in [-.085,.085]:
    for z in [1.10,1.205,1.315]:
        bone('Greatcoat brass button',(x,-.134,z),(x,-.148,z),.013,.013,brass,16)
        tube('Button stitch',[(x-.004,-.151,z),(x+.004,-.151,z)],.0015,darkcloth,1)

# Posed sleeves: right hand up to hear the dead, left down on weapon.
armdata=[('Listening',(.248,.025,1.449),(.366,.011,1.265),(.313,-.079,1.61)),('Carbine',(-.242,.023,1.43),(-.308,-.006,1.203),(-.228,-.18,1.005))]
for name,shoulder,elbow,wrist in armdata:
    bone(name+' upper sleeve',elbow,shoulder,.10,.126,coat)
    ell(name+' sleeve elbow',elbow,(.112,.108,.104),coat)
    bone(name+' forearm sleeve',wrist,elbow,.078,.105,coat)
    a=Vector(wrist); b=Vector(elbow); d=(b-a).normalized()
    bone(name+' broad cuff',a-d*.012,a+d*.07,.086,.084,edge)
    for k,t in enumerate([.24,.39,.56,.72]):
        c=a.lerp(b,t)
        ell(name+' sleeve cloth fold',c,(.083+t*.018,.099,.019),coat,20,10).rotation_euler[1]=-.28 if name=='Carbine' else .35
    box(name+' cuff tab',(a.x,a.y-.083,a.z+.028),(.076,.013,.027),coat,.006)
    bone(name+' cuff button',(a.x,a.y-.089,a.z+.028),(a.x,a.y-.102,a.z+.028),.010,.010,brass,12)

# Face built from a shaped continuous volume with fused facial planes.
headparts=[]
def headell(name,loc,scale):
    ob=ell(name,loc,scale,skin,32,24);headparts.append(ob);return ob
headell('Cranium',(0,.002,1.761),(.114,.095,.146))
headell('Long square jaw',(0,-.035,1.667),(.085,.073,.084))
headell('Chin',(0,-.068,1.613),(.057,.044,.035))
for s in [-1,1]:
    headell('High cheekbone',(s*.068,-.071,1.715),(.029,.024,.038))
    ob=headell('Heavy brow plane',(s*.048,-.084,1.796),(.054,.034,.021));ob.rotation_euler[1]=s*.20
headell('Nose bridge',(-.003,-.10,1.738),(.023,.035,.056))
headell('Broken nose tip',(-.009,-.13,1.71),(.029,.032,.022))
for s in [-1,1]: headell('Nose wing',(s*.021,-.118,1.702),(.014,.02,.014))
bpy.ops.object.select_all(action='DESELECT')
for obj in headparts:obj.select_set(True)
bpy.context.view_layer.objects.active=headparts[0]
bpy.ops.object.join();head=bpy.context.object;head.name='Sculpted head | fused facial planes'
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
rem=head.modifiers.new('Fused head volumes','REMESH');rem.mode='VOXEL';rem.voxel_size=.0035;rem.use_smooth_shade=True
bpy.ops.object.modifier_apply(modifier=rem.name)
sm=head.modifiers.new('Sculpt smoothing','SMOOTH');sm.factor=.65;sm.iterations=4
bpy.ops.object.modifier_apply(modifier=sm.name)
sub=head.modifiers.new('Face finish','SUBSURF');sub.levels=1
for s in [-1,1]:
    ell('Ear',(s*.115,.002,1.735),(.027,.025,.046),skin)
    ell('Ear concha',(s*.127,-.018,1.737),(.012,.009,.029),lip)
    tube('Ear inner ridge',[(s*.126,-.028,1.715),(s*.137,-.028,1.743),(s*.127,-.022,1.763)],.005,skinlight)
    ell('Recessed eye socket',(s*.047,-.094,1.766),(.034,.018,.021),lip)
    ell('Eye',(s*.047,-.105,1.768),(.024,.012,.009),eye)
    ell('Hazel iris',(s*.047-.006,-.118,1.771),(.009,.003,.010),iris,20,12)
    ell('Pupil',(s*.047-.006,-.121,1.771),(.004,.0016,.006),pupil,16,10)
    tube('Upper eyelid',[(s*.047-.026,-.107,1.768),(s*.047,-.117,1.775),(s*.047+.025,-.106,1.771)],.0045,skin)
    tube('Lower eyelid',[(s*.047-.024,-.104,1.764),(s*.047,-.115,1.760),(s*.047+.023,-.105,1.765)],.0045,skin)
    tube('Heavy grey eyebrow',[(s*.022,-.116,1.789),(s*.048,-.113,1.806),(s*.086,-.089,1.798)],.008,hair)
    for j in range(3):
        tube('Weathered cheek crease',[(s*.078,-.079,1.744-j*.012),(s*.086,-.073,1.733-j*.012)],.0009,lip,1)
    ell('Nostril',(s*.018-.004,-.142,1.698),(.006,.003,.0045),lip,16,8)
    tube('Nasolabial crease',[(s*.027,-.117,1.70),(s*.035,-.111,1.68),(s*.043,-.10,1.659)],.0011,lip)
for j in range(3):
    z=1.83+j*.015
    tube('Forehead wrinkle',[(-.055,-.070,z),(-.025,-.083,z+.006),(.031,-.08,z+.004),(.055,-.067,z-.004)],.0007,skin,2)
tube('Stern mouth',[(-.042,-.104,1.651),(-.014,-.118,1.657),(.009,-.119,1.655),(.039,-.103,1.647)],.004,lip)
tube('Lower lip',[(-.029,-.113,1.646),(.001,-.121,1.641),(.029,-.11,1.643)],.006,skin)
tube('Chin crease',[(-.026,-.105,1.625),(.005,-.113,1.622),(.03,-.104,1.625)],.002,lip)
tube('Cheek scar',[(-.071,-.097,1.731),(-.065,-.105,1.703),(-.06,-.101,1.686)],.0025,skinlight)

# Close-cropped hair cap with directional swept tufts, and coarse stubble.
verts=[];faces=[];n=48;nr=10
for j in range(nr+1):
    for i in range(n):
        a=2*math.pi*i/n
        front=max(0,-math.sin(a))
        theta=(j/nr)*(1.54-.69*front+.022*math.sin(19*a)+.016*math.cos(31*a))
        verts.append((.116*math.sin(theta)*math.cos(a),.003+.099*math.sin(theta)*math.sin(a),1.768+.144*math.cos(theta)))
for j in range(nr):
    for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
me=bpy.data.meshes.new('Hair cap');me.from_pydata(verts,[],faces);me.update()
ob=bpy.data.objects.new('Short swept iron-grey hair',me);character.objects.link(ob);me.materials.append(hair)
for p in me.polygons:p.use_smooth=True
for i in range(360):
    a=random.uniform(0,math.pi*2)
    front=max(0,-math.sin(a))
    theta=random.uniform(.05,1.46-.67*front)
    points=[]
    for j in range(4):
        t=theta+j*.022;ang=a+j*.023
        points.append((.117*math.sin(t)*math.cos(ang),.003+.101*math.sin(t)*math.sin(ang),1.768+.146*math.cos(t)))
    tube('Short swept hair strand',points,.0014,hairlight if i%4==0 else hair,1)

# Listening hand: palm faces forward, four separated articulated fingers.
ell('Listening hand palm',(.291,-.078,1.669),(.044,.021,.061),skin)
for i,(x,height) in enumerate([(.258,1.752),(.279,1.775),(.301,1.767),(.323,1.748)]):
    start=(x,-.078,1.692);mid=(x-.006,-.084,height-.025);tip=(x-.012,-.097,height)
    bone('Listening finger proximal',start,mid,.010,.0087,skin,14)
    ell('Listening knuckle',mid,(.010,.010,.010),skinlight,16,10)
    bone('Listening fingertip',mid,tip,.009,.007,skin,14)
    ell('Listening finger pad',tip,(.0075,.007,.008),skin,16,10)
bone('Listening thumb',(.262,-.08,1.65),(.242,-.113,1.68),.015,.011,skin)
bone('Listening thumb tip',(.242,-.113,1.68),(.244,-.12,1.7),.011,.008,skin)
tube('Palm crease',[(.269,-.1,1.684),(.288,-.103,1.668),(.314,-.095,1.671)],.0018,lip,1)
ell('Carbine hand palm',(-.21,-.212,.989),(.046,.027,.04),skin)
for i in range(4):
    x=-.245+i*.022
    tube('Carbine curled finger',[(x,-.227,1.004),(x+.01,-.253,.982),(x+.018,-.24,.963)],.010,skin)
    ell('Carbine knuckle',(x,-.24,.993),(.011,.012,.011),skinlight,16,10)
bone('Carbine thumb',(-.171,-.212,1.017),(-.158,-.243,.991),.014,.011,skin)

# Belt, diagonal leather harness, buckles, and cartridge pouches.
for x in [-.165,.161]:
    ribbon('Shoulder webbing',[(x,.113,1.06),(x,.166,1.30),(x,.09,1.52),(x,-.055,1.535),(x*.8,-.173,1.37),(x*.6,-.171,1.17),(x*.45,-.148,1.049)],.047,leather)
    buckle('Harness buckle',x*.82,-.186,1.345)
    for j in range(5):
        ell('Strap punched hole',(x*.69,-.183,1.21+j*.018),(.002,.0015,.002),sole,12,8)
pts=[(.218*math.sin(a),.036+.154*math.cos(a),1.06) for a in [i*math.pi*2/64 for i in range(65)]]
tube('Leather waist belt',pts,.028,leather)
buckle('Waist buckle',.023,-.136,1.06,.057,.045)
for x in [-.153,-.061]:
    box('Cartridge pouch',(x,-.163,1.094),(.077,.073,.105),leather,.014)
    box('Cartridge flap',(x,-.208,1.121),(.078,.013,.056),leatheredge,.011)
    bone('Cartridge pouch stud',(x,-.219,1.104),(x,-.226,1.104),.005,.005,brass,12)
    for side in [-1,1]:
        tube('Pouch edge piping',[(x+side*.033,-.205,1.13),(x+side*.033,-.205,1.054)],.0024,leatheredge,1)
box('Demolition satchel',(.25,-.048,1.009),(.182,.145,.214),coat,.032)
box('Demolition satchel flap',(.252,-.13,1.069),(.181,.024,.088),edge,.016)
for x in [.198,.304]:
    ribbon('Satchel straps',[(x,-.091,1.13),(x,-.153,1.097),(x,-.153,.951)],.024,leather)
    buckle('Satchel strap buckle',x,-.161,1.03,.031,.04)
bone('Wire spool axle',(.297,-.064,.875),(.297,-.064,.978),.045,.045,wood)
for z in [.87,.982]:
    bone('Wire spool flange',(.297,-.064,z),(.297,-.064,z+.007),.065,.065,metal)
for i in range(18):
    z=.885+i*.0048
    tube('Wound demolition wire',[(.297+.049*math.cos(a),-.064+.049*math.sin(a),z) for a in [j*2*math.pi/24 for j in range(25)]],.0024,steel,1)
tube('Loose wire end',[(.345,-.069,.9),(.373,-.081,.876),(.365,-.131,.852)],.0024,metal)
bone('Plunger handle stem',(.294,.028,1.105),(.294,.028,1.232),.009,.009,metal)
bone('Plunger T handle',(.26,.028,1.232),(.332,.028,1.232),.012,.012,wood)
box('Scrounged tool backpack',(0,.198,1.292),(.31,.135,.31),darkcloth,.04)
box('Backpack flap',(0,.273,1.415),(.315,.032,.092),coat,.024)
for x in [-.09,.09]:
    ribbon('Pack retaining strap',[(x,.262,1.47),(x,.288,1.30),(x,.268,1.15)],.033,leather)
    buckle('Pack buckle',x,.295,1.294)
for s in [-1,1]:
    box('Shoulder epaulette',(s*.224,.012,1.524),(.092,.082,.017),edge,.009)
    bone('Epaulette button',(s*.193,-.001,1.533),(s*.193,-.001,1.54),.011,.011,brass,12)

# One mechanically coherent carbine along a single diagonal local axis.
gun_origin=Vector((-.145,-.244,1.007))
axis=Vector((.57,-.01,-.822)).normalized()
cross=Vector((.822,0,.57)).normalized()
front=Vector((0,-1,0))
def gp(t,u=0,v=0):return gun_origin+axis*t+cross*u+front*v
def gunbox(name,t,length,width,depth,mat,u=0,v=0):
    ob=box(name,gp(t,u,v),(width,depth,length),mat,.006)
    ob.rotation_euler=axis.to_track_quat('Z','Y').to_euler();return ob
gunbox('Walnut buttstock',-.155,.235,.065,.043,wood)
gunbox('Buttplate',-.274,.012,.071,.049,metal)
gunbox('Carbine receiver',.009,.14,.048,.042,metal)
gunbox('Walnut fore-end',.145,.195,.041,.039,wood)
bone('Straight steel barrel',gp(.085),gp(.413),.014,.010,metal)
bone('Muzzle collar',gp(.404),gp(.426),.015,.015,steel)
bone('Dark muzzle bore',gp(.426),gp(.428),.0085,.0085,pupil)
gunbox('Box magazine',.022,.052,.042,.035,metal,u=-.068)
gunbox('Receiver side plate',.021,.093,.027,.006,steel,v=.024)
tube('Bolt handle',[gp(-.023,.01,.022),gp(-.023,.027,.048)],.006,metal)
ell('Bolt knob',gp(-.023,.027,.048),(.011,.011,.011),steel,16,10)
tube('Trigger guard',[gp(-.055,-.018),gp(-.027,-.051),gp(.002,-.046),gp(.014,-.018)],.004,metal,2)
tube('Trigger',[gp(-.035,-.019),gp(-.028,-.035)],.0025,steel,1)
gunbox('Front sight',.377,.018,.023,.014,metal,u=.022)
gunbox('Rear sight',.042,.022,.02,.016,steel,u=.027)
tube('Hanging rifle sling',[gp(-.234,0,.018),(-.10,-.287,.779),(.16,-.279,.645),gp(.296,0,.018)],.009,leather)

# Visible coat seams and restrained sculpted garment folds.
for side in [-1,1]:
    tube('Front coat skirt seam',[(side*.025,-.134,1.04),(side*.072,-.14,.86),(side*.119,-.151,.69),(side*.156,-.144,.563)],.004,edge)
    for i in range(4):
        x=side*(.115+i*.034)
        tube('Long coat fold',[(x*.69,.11,1.016),(x,.166,.831),(x*1.17,.172,.60)],.009,coat)
    box('Coat lower pocket',(side*.186,-.107,.816),(.102,.024,.041),edge,.009,rotation=(0,side*.14,0))
    for j in range(3):
        tube('Sergeant chevron',[(side*.289,-.081,1.409-j*.016),(side*.319,-.085,1.395-j*.016),(side*.341,-.069,1.411-j*.016)],.005,scarf,2)
tube('Rear coat center seam',[(0,.198,1.025),(0,.209,.828),(0,.235,.56)],.004,edge)

# Fuse the upper coat and sleeves to remove primitive intersection seams.
upper_names=['Coat torso','Upper shoulders','Listening upper sleeve','Listening sleeve elbow','Listening forearm sleeve','Carbine upper sleeve','Carbine sleeve elbow','Carbine forearm sleeve']
bpy.ops.object.select_all(action='DESELECT')
upper=[]
for name in upper_names:
    ob=bpy.data.objects[name]
    bpy.context.view_layer.objects.active=ob
    for mod in list(ob.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    upper.append(ob)
for ob in upper:ob.select_set(True)
bpy.context.view_layer.objects.active=upper[0]
bpy.ops.object.join()
ob=bpy.context.object;ob.name='Continuous upper greatcoat and posed sleeves'
mod=ob.modifiers.new('Fused wool silhouette','REMESH');mod.mode='VOXEL';mod.voxel_size=.006;mod.use_smooth_shade=True
bpy.ops.object.modifier_apply(modifier=mod.name)
mod=ob.modifiers.new('Sculpted cloth transitions','SMOOTH');mod.factor=.8;mod.iterations=5
bpy.ops.object.modifier_apply(modifier=mod.name)

# Bake modifiers to an export copy; keep original editable geometry in .blend.
root=bpy.data.objects.new('Sapper_Concept_Root',None);character.objects.link(root)
for ob in list(character.objects):
    if ob!=root:ob.parent=root
root['status']='PROVISIONAL stylized static concept; not rigged or production game-ready'
root['reference']='../2026-09-10-v1/sapper-full.png'
root['height_m']=1.92
root['generator']='Local Blender Python modeling; no image-to-3D reconstruction service'

# Studio lighting and two render views.
floor_mat=material('Studio floor',(.036,.048,.043),.9)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,.001))
floor=move(bpy.context.object,stage);floor.name='Studio ground';floor.data.materials.append(floor_mat)
world=bpy.data.worlds.new('Dark green studio') if not scene.world else scene.world
scene.world=world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.07,.09,.08,1);world.node_tree.nodes['Background'].inputs[1].default_value=.4
def aim(obj,target):obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
def light(name,loc,power,color,size):
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.color=color;data.shape='DISK';data.size=size
    ob=bpy.data.objects.new(name,data);stage.objects.link(ob);ob.location=loc;aim(ob,(0,0,1));return ob
light('Large warm key',(-3,-4,5),450,(1,.83,.65),4)
light('Soft frontal fill',(2,-3,2.7),170,(.77,.87,1),3)
light('Teal rim',(2,2,3.1),550,(.36,.78,.72),2.5)
light('Hair light',(-1,1,4),280,(1,.89,.67),2)
camdata=bpy.data.cameras.new('Portrait camera');cam=bpy.data.objects.new('Portrait camera',camdata);stage.objects.link(cam)
cam.location=(2.8,-6,2.65);aim(cam,(0,0,.98));camdata.type='ORTHO';camdata.ortho_scale=2.28;scene.camera=cam
scene.render.engine='CYCLES';scene.cycles.samples=48
scene.cycles.use_denoising=True
scene.render.resolution_x=1100;scene.render.resolution_y=1400;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.view_settings.view_transform='AgX'
scene.render.film_transparent=False
scene.render.image_settings.color_mode='RGBA'

# Export geometry only, triangulating evaluated copies for portable GLB shading.
bpy.ops.object.select_all(action='DESELECT')
for ob in character.objects:ob.select_set(True)
bpy.context.view_layer.objects.active=root
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_location=(0,0,.99)
            area.spaces.active.region_3d.view_distance=3.3
            area.spaces.active.region_3d.view_rotation=cam.rotation_euler.to_quaternion()
            area.spaces.active.shading.color_type='MATERIAL'
            area.spaces.active.clip_start=.01
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'sapper-concept.blend'))
# Join evaluated geometry into a compact static export, retaining editable originals.
bpy.ops.object.select_all(action='DESELECT')
parts=[ob for ob in character.objects if ob.type in {'MESH','CURVE'}]
for ob in parts:ob.select_set(True)
bpy.context.view_layer.objects.active=parts[0]
bpy.ops.object.convert(target='MESH')
bpy.ops.object.join()
export_mesh=bpy.context.object;export_mesh.name='Sapper_Static_Concept'
mod=export_mesh.modifiers.new('Portable concept mesh reduction','DECIMATE');mod.ratio=.14
bpy.ops.object.modifier_apply(modifier=mod.name)
root.select_set(True)
bpy.ops.export_scene.gltf(filepath=str(OUT/'sapper-concept.glb'),export_format='GLB',use_selection=True,export_apply=True,export_cameras=False,export_lights=False,export_extras=True)
bpy.ops.wm.open_mainfile(filepath=str(OUT/'sapper-concept.blend'))
scene=bpy.context.scene;cam=scene.camera;camdata=cam.data
scene.render.filepath=str(OUT/'sapper-preview.png');bpy.ops.render.render(write_still=True)
cam.location=(1.35,-4,2.07);aim(cam,(0,-.02,1.55));camdata.ortho_scale=.88
scene.render.resolution_x=1200;scene.render.resolution_y=1200
scene.render.filepath=str(OUT/'sapper-detail.png');bpy.ops.render.render(write_still=True)
cam.location=(-3,5,2.6);aim(cam,(0,0,.98));camdata.ortho_scale=2.27
scene.render.resolution_x=1100;scene.render.resolution_y=1400
scene.render.filepath=str(OUT/'sapper-back.png');bpy.ops.render.render(write_still=True)
print('SAPPER_DELIVERABLES_COMPLETE',OUT)
