#!/usr/bin/env python3
"""Shaded 3D isometric renders of the enclosure STLs (base, top plate, assembly)."""
import os, numpy as np, trimesh, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

HERE=os.path.dirname(__file__); PROJ=os.path.join(HERE,"..","project"); OUT=os.path.join(HERE,"..")
LIGHT=np.array([0.4,-0.5,0.85]); LIGHT=LIGHT/np.linalg.norm(LIGHT)

def shade(mesh, base_rgb):
    n=mesh.face_normals
    d=np.clip(n@LIGHT,0,1)
    s=0.5+0.5*d                     # ambient 0.5 so nothing is crushed black
    base=np.array(base_rgb)/255.0
    return np.clip(s[:,None]*base[None,:],0,1)

def add_mesh(ax, mesh, color, alpha=1.0):
    tris=mesh.vertices[mesh.faces]
    fc=shade(mesh,color)
    pc=Poly3DCollection(tris, facecolors=np.column_stack([fc,np.full(len(fc),alpha)]),
                        edgecolors="none", linewidths=0)
    pc.set_sort_zpos(None)
    ax.add_collection3d(pc)

def setup(ax, bounds, elev=32, azim=-54):
    (xmin,ymin,zmin),(xmax,ymax,zmax)=bounds
    ax.set_xlim(xmin,xmax); ax.set_ylim(ymin,ymax); ax.set_zlim(zmin,zmax)
    ax.set_box_aspect((xmax-xmin, ymax-ymin, (zmax-zmin)))
    ax.view_init(elev=elev, azim=azim)
    ax.set_axis_off()

def frame(bounds_meshes):
    allv=np.vstack([m.vertices for m,_ ,*_ in bounds_meshes])
    mn=allv.min(0); mx=allv.max(0)
    # pad z a touch
    return (mn, mx)

def render(meshes, out, title, elev=32, azim=-54, zexag=1.0):
    fig=plt.figure(figsize=(10,8.4),dpi=160); fig.patch.set_facecolor("#0d1320")
    ax=fig.add_subplot(111,projection="3d"); ax.set_facecolor("#0d1320")
    disp=[]
    for m,c,*rest in meshes:
        alpha=rest[0] if rest else 1.0
        mm=m.copy()
        if zexag!=1.0: mm.vertices[:,2]*=zexag
        add_mesh(ax,mm,c,alpha); disp.append((mm,c))
    allv=np.vstack([m.vertices for m,_ in disp])
    mn=allv.min(0); mx=allv.max(0)
    ctr=(mn+mx)/2; sp=(mx[:2]-mn[:2]).max()/2*1.04
    ax.set_xlim(ctr[0]-sp,ctr[0]+sp); ax.set_ylim(ctr[1]-sp,ctr[1]+sp)
    ax.set_zlim(mn[2],mx[2])
    zr=max(mx[2]-mn[2],1e-3)
    ax.set_box_aspect((2*sp, 2*sp, zr))   # true proportions, fills frame
    ax.view_init(elev=elev,azim=azim); ax.set_axis_off()
    fig.suptitle(title,color="#e8eef5",fontsize=15,y=0.86,weight="bold")
    plt.savefig(out,facecolor="#0d1320",bbox_inches="tight",pad_inches=0.1); plt.close(fig)
    print("wrote",os.path.basename(out))

base=trimesh.load(os.path.join(PROJ,"base_enclosure.stl"))
top =trimesh.load(os.path.join(PROJ,"top_plate.stl"))
print("faces: base",len(base.faces),"top",len(top.faces))

C_BASE=(104,128,170); C_TOP=(86,102,134); C_PCB=(22,140,66)

# PCB slab (182x182x1.6) at z 15..16.6, centred in 189 footprint
pcb=trimesh.creation.box(extents=[182,182,1.6]); pcb.apply_translation([94.5,94.5,15.8])

# --- separate pieces (true proportions) ---
render([(base,C_BASE)], os.path.join(OUT,"render_base.png"),
       "BASE  —  floor + walls + corner posts + 12 mm battery bay", elev=30, azim=-52)
render([(top,C_TOP)], os.path.join(OUT,"render_topplate.png"),
       "TOP PLATE  —  snap-in switch cutouts + corner legs", elev=32, azim=-52)

# --- assembled (top plate lifted to rest on walls) ---
topA=top.copy(); topA.apply_translation([0,0,16.6])   # legs bottom -> PCB top
render([(base,C_BASE),(pcb,C_PCB),(topA,C_TOP)], os.path.join(OUT,"render_assembled.png"),
       "ASSEMBLED  —  base + PCB + top plate", elev=24, azim=-52)

# --- exploded ---
pcbE=pcb.copy();  pcbE.apply_translation([0,0,26])
topE=top.copy();  topE.apply_translation([0,0,60])
render([(base,C_BASE),(pcbE,C_PCB),(topE,C_TOP)], os.path.join(OUT,"render_exploded.png"),
       "EXPLODED  —  top plate · PCB · base", elev=16, azim=-52)
