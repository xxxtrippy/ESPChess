#!/usr/bin/env python3
"""Beauty renders: black case + translucent keycaps lit by piece colour,
with a bloom/glow post-process. Starting position, white to move."""
import os, numpy as np, trimesh, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
from PIL import Image, ImageFilter

HERE=os.path.dirname(__file__); PROJ=os.path.join(HERE,"..","project"); OUT=os.path.join(HERE,"..")
PITCH=19.05; C0=14.8; N=9

# piece colours (handoff code)
PC={'P':(0,255,0),'N':(160,0,255),'R':(255,140,0),'B':(0,80,255),'Q':(255,0,0),'K':(255,255,0)}
BACK=['R','N','B','Q','K','B','N','R']

LIGHT=np.array([0.35,-0.5,0.9]); LIGHT/=np.linalg.norm(LIGHT)
def shade(mesh,rgb,emissive):
    d=np.clip(mesh.face_normals@LIGHT,0,1)
    s=(0.72+0.28*d) if emissive else (0.33+0.67*d)
    return np.clip(s[:,None]*(np.array(rgb)/255.0)[None,:],0,1)

def facecolors(mesh,rgb,alpha,emissive=False):
    fc=shade(mesh,rgb,emissive)
    return np.column_stack([fc,np.full(len(fc),alpha)])

base=trimesh.load(os.path.join(PROJ,"base_enclosure.stl"))
top =trimesh.load(os.path.join(PROJ,"top_plate.stl"))
cx=(base.bounds[0][0]+base.bounds[1][0])/2; cy=(base.bounds[0][1]+base.bounds[1][1])/2
PCB_OFF=cx-91  # OUTER/2 - PCB/2 ; square centres share this offset

topA=top.copy(); topA.apply_translation([0,0,16.6])   # seat plate on walls
PLATE_TOP=16.6+7.5                                     # assembled plate top z

def cap_mesh(xc,yc):
    # stepped translucent keycap: lower 18x18x3.5 + upper 16x16x4
    lo=trimesh.creation.box(extents=[18,18,3.5]); lo.apply_translation([xc,yc,PLATE_TOP+1.75])
    hi=trimesh.creation.box(extents=[16,16,4.5]); hi.apply_translation([xc,yc,PLATE_TOP+3.5+2.25])
    return lo,hi

def cell_color(r,c):
    # returns (rgb, emissive) for physical (row r, col c).
    # Beauty render shows both sides at full colour to showcase the code
    # (in-game the waiting side renders at 50%).
    if 1<=r<=8 and 1<=c<=8:
        file=c-1
        if r in (8,1): return PC[BACK[file]],True        # back ranks
        if r in (7,2): return PC['P'],True               # pawn ranks
        return (52,58,72),False                          # empty square (smoky translucent)
    if (r,c)==(8,0): return (0,210,0),True               # White CLK (active)
    if (r,c)==(0,8): return (0,70,0),True                # Black CLK (idle)
    if (r,c)==(0,0): return (90,90,104),True             # M button faint
    return (40,45,56),False                              # eval segments / unlit

def render(out, elev, azim, title, w=12, h=10):
    fig=plt.figure(figsize=(w,h),dpi=170); fig.patch.set_facecolor("#05070c")
    ax=fig.add_subplot(111,projection="3d"); ax.set_facecolor("#05070c")
    CASE=(28,30,38)
    tri_list=[]; col_list=[]
    def push(mesh,rgb,alpha,emis=False):
        tri_list.append(mesh.vertices[mesh.faces]); col_list.append(facecolors(mesh,rgb,alpha,emis))
    push(base,CASE,1.0); push(topA,CASE,1.0)
    for r in range(N):
        for c in range(N):
            xc=C0+c*PITCH+PCB_OFF; yc=C0+r*PITCH+PCB_OFF
            rgb,emis=cell_color(r,c); lo,hi=cap_mesh(xc,yc)
            a=0.96 if emis else 0.9
            push(lo,rgb,a,emis); push(hi,rgb,a,emis)
    tris=np.vstack(tri_list); cols=np.vstack(col_list)
    pc=Poly3DCollection(tris,facecolors=cols,edgecolors="none",linewidths=0)
    pc.set_zsort("average"); ax.add_collection3d(pc)
    span=(base.bounds[1][:2]-base.bounds[0][:2]).max()/2*1.01
    zmax=PLATE_TOP+9
    ax.set_xlim(cx-span,cx+span); ax.set_ylim(cy-span,cy+span); ax.set_zlim(0,zmax)
    ax.set_box_aspect((2*span,2*span,zmax))      # true proportions, fills frame
    ax.view_init(elev=elev,azim=azim); ax.set_axis_off()
    if title: fig.suptitle(title,color="#dfe8f5",fontsize=15,y=0.9,weight="bold")
    tmp=out+".tmp.png"
    plt.savefig(tmp,facecolor="#05070c",bbox_inches="tight",pad_inches=0.15); plt.close(fig)
    bloom(tmp,out)

def bloom(src,dst):
    img=Image.open(src).convert("RGB"); arr=np.asarray(img).astype(np.float32)
    lum=arr.max(2)
    mask=np.clip((lum-120)/135,0,1)[...,None]
    bright=Image.fromarray((arr*mask).astype(np.uint8))
    acc=np.zeros_like(arr)
    for rad,inten in [(5,0.6),(12,0.5),(26,0.4),(48,0.3)]:
        b=np.asarray(bright.filter(ImageFilter.GaussianBlur(rad))).astype(np.float32)
        acc+=b*inten
    out=255-(255-arr)*(255-np.clip(acc,0,255))/255   # screen blend
    Image.fromarray(np.clip(out,0,255).astype(np.uint8)).save(dst)
    os.remove(src); print("wrote",os.path.basename(dst))

render(os.path.join(OUT,"beauty_hero.png"),   elev=28, azim=-52, title="")
render(os.path.join(OUT,"beauty_topdown.png"),elev=72, azim=-90, title="")
render(os.path.join(OUT,"beauty_angle.png"),  elev=20, azim=-30, title="")
