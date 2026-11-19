#!/usr/bin/env python3
"""Full enclosure for the 9x9 chessboard PCB.

Assembly (one unit drops into the base and screws down):
  TOP PLATE (switches snap in) -- PCB soldered to switch pins -- 4x M3 screws
  through the plate corners + PCB into heat-set inserts in the base posts.

Stack (z, mm, base floor top = 3):
  floor 0..3 | battery bay 3..15 (12mm) | PCB 15..16.6 | switch gap 16.6..20.1
  | top plate rests on wall top at 20.1, plate 20.1..24.1
"""
import os, numpy as np, trimesh
from trimesh.creation import extrude_polygon, box, cylinder
from shapely.geometry import Polygon, Point, box as sbox

OUT=os.path.join(os.path.dirname(__file__),"..","project"); os.makedirs(OUT,exist_ok=True)

# ---- parameters ----
PCB      = 182.0
PITCH    = 19.05
N        = 9
C0_PCB   = 14.8              # first switch centre in PCB space
WALL     = 5.0
FLOOR    = 3.0
CLEAR    = 0.5              # per-side clearance around PCB inside the tray
INNER    = PCB + 2*CLEAR    # 183
OUTER    = INNER + 2*WALL   # 189
PCB_OFF  = WALL + CLEAR     # PCB (0,0) -> base coord (3.5,3.5)

BATTERY  = 12.0            # gap below PCB  (floor_top -> PCB bottom)
PCB_T    = 1.6
SW_GAP   = 3.5            # PCB top -> plate bottom (switch-set); = corner leg height
PLATE_T  = 4.0
WEB_T    = 1.5           # MX latch web (top of plate)
REL_T    = PLATE_T-WEB_T # relief below web (2.5)

MX_CUT   = 14.0
RELIEF   = 15.6
POST_D   = 8.0
M3       = 3.4           # screw clearance
INSERT_D = 4.0           # heat-set insert pilot hole (M3 brass, ~4.0mm)
INSERT_H = 6.0
CB_D     = 6.0           # counterbore for M3 socket head
CB_DEPTH = 2.5

# z levels
FLOOR_TOP = FLOOR
POST_TOP  = FLOOR + BATTERY          # 15  (PCB bottom)
PCB_TOP   = POST_TOP + PCB_T         # 16.6
WALL_TOP  = PCB_TOP + SW_GAP         # 20.1 (plate rests here)

# corner screw/post positions, base coords
HOLES_PCB = [(5,5),(PCB-5,5),(5,PCB-5),(PCB-5,PCB-5)]
HOLES = [(x+PCB_OFF, y+PCB_OFF) for x,y in HOLES_PCB]
# switch centres, base coords
CENTRES = [(C0_PCB+c*PITCH+PCB_OFF, C0_PCB+r*PITCH+PCB_OFF)
           for r in range(N) for c in range(N)]

def sq(cx,cy,s): h=s/2; return sbox(cx-h,cy-h,cx+h,cy+h)
def ci(cx,cy,d): return Point(cx,cy).buffer(d/2,resolution=48)

# =================================================== TOP PLATE (cap)
def build_top():
    outer=sbox(0,0,OUTER,OUTER)
    screw=[ci(x,y,M3) for x,y in HOLES]
    web_holes=[sq(cx,cy,MX_CUT) for cx,cy in CENTRES]+screw
    rel_holes=[sq(cx,cy,RELIEF) for cx,cy in CENTRES]+screw
    web=extrude_polygon(Polygon(outer.exterior.coords,[h.exterior.coords[::-1] for h in web_holes]),WEB_T)
    web.apply_translation([0,0,SW_GAP+REL_T])   # top layer z: 3.5+2.5 .. 3.5+2.5+1.5
    rel=extrude_polygon(Polygon(outer.exterior.coords,[h.exterior.coords[::-1] for h in rel_holes]),REL_T)
    rel.apply_translation([0,0,SW_GAP])          # z 3.5..6.0
    legs=[]
    for x,y in HOLES:
        ring=Polygon(ci(x,y,POST_D-1).exterior.coords,[ci(x,y,M3).exterior.coords[::-1]])
        legs.append(extrude_polygon(ring,SW_GAP))  # z 0..3.5
    parts=[web,rel]+legs
    mesh=trimesh.boolean.union(parts)
    # counterbores for screw heads (from plate top downward)
    top_z=SW_GAP+REL_T+WEB_T
    cbs=[]
    for x,y in HOLES:
        c=cylinder(radius=CB_D/2,height=CB_DEPTH+0.2)
        c.apply_translation([x,y,top_z-CB_DEPTH/2+0.1]); cbs.append(c)
    mesh=trimesh.boolean.difference([mesh]+cbs)
    # shift so plate bottom (legs) sits at z=0 for printing
    mesh.apply_translation([0,0,0])
    return mesh

# =================================================== BASE (floor+walls+posts)
def build_base():
    # outer solid
    outer=box(extents=[OUTER,OUTER,WALL_TOP]); outer.apply_translation([OUTER/2,OUTER/2,WALL_TOP/2])
    # cavity (leaves floor + 4 walls)
    cav=box(extents=[INNER,INNER,WALL_TOP]); cav.apply_translation([OUTER/2,OUTER/2,FLOOR+WALL_TOP/2])
    base=trimesh.boolean.difference([outer,cav])
    # corner posts up to PCB bottom
    posts=[]
    for x,y in HOLES:
        p=cylinder(radius=POST_D/2,height=BATTERY); p.apply_translation([x,y,FLOOR+BATTERY/2]); posts.append(p)
    base=trimesh.boolean.union([base]+posts)
    # subtract insert pilot holes (from post top down) + cable slot
    cuts=[]
    for x,y in HOLES:
        h=cylinder(radius=INSERT_D/2,height=INSERT_H+0.2); h.apply_translation([x,y,POST_TOP-INSERT_H/2+0.1]); cuts.append(h)
    # 12mm round port through the front wall, centred side-to-side and top-to-bottom
    import trimesh.transformations as tf
    port=cylinder(radius=6.0, height=WALL+4)          # 12mm dia, pierces the 5mm wall
    port.apply_transform(tf.rotation_matrix(np.pi/2,[1,0,0]))   # z-axis -> y-axis
    port.apply_translation([OUTER/2, WALL/2, WALL_TOP/2])
    cuts.append(port)
    base=trimesh.boolean.difference([base]+cuts)
    return base

top=build_top(); base=build_base()
for name,m in [("top_plate",top),("base_enclosure",base)]:
    m.merge_vertices(); m.update_faces(m.nondegenerate_faces()); m.fix_normals()
    p=os.path.join(OUT,name+".stl"); m.export(p)
    print(f"{name}: watertight={m.is_watertight} vol={m.volume/1000:.1f}cm3 "
          f"bbox={np.round(m.extents,1)}")
print(f"z-levels: floor_top={FLOOR_TOP} battery_to={POST_TOP} pcb_top={PCB_TOP} wall_top={WALL_TOP} assembled_H={WALL_TOP+PLATE_T}")
print(f"outer footprint={OUTER}x{OUTER}mm  inserts@{[tuple(round(v,1) for v in h) for h in HOLES]}")
