#!/usr/bin/env python3
"""3D-printable TOP switch-plate and flat BOTTOM plate for the 9x9 chessboard
PCB sandwich.  top plate -> PCB -> bottom plate, held by M3 standoffs.

Dimensions match ChessboardPCB.kicad_pcb: 182x182 mm, 19.05 mm key pitch,
9x9 centred (first/last switch centre 14.8 / 167.2), M3 holes at the 4 corners
(5,5),(177,5),(5,177),(177,177).
"""
import os
import numpy as np
import trimesh
from trimesh.creation import extrude_polygon
from shapely.geometry import Polygon, Point, box as sbox
from shapely.ops import unary_union

OUT = os.path.join(os.path.dirname(__file__), "..", "project")
os.makedirs(OUT, exist_ok=True)

# ---- shared dimensions ----
EDGE   = 182.0
PITCH  = 19.05
N      = 9
C0     = 14.8                       # centre of first switch from edge
MX_CUT = 14.0                       # MX plate cutout (web hole)
RELIEF = 15.6                       # clip-relief opening (below the web)
WEB_T  = 1.5                        # latch web thickness (MX spec)
REL_T  = 1.5                        # relief layer thickness
PLATE_T= WEB_T + REL_T             # 3.0 total plate
LEG_H  = 3.5                        # top-plate corner leg = plate->PCB gap (switch-set)
LEG_D  = 7.0
M3     = 3.4                        # M3 screw clearance diameter
CORNERS= [(5,5),(EDGE-5,5),(5,EDGE-5),(EDGE-5,EDGE-5)]

def centres():
    return [(C0+c*PITCH, C0+r*PITCH) for r in range(N) for c in range(N)]

def sq(cx, cy, s):
    h=s/2.0
    return sbox(cx-h, cy-h, cx+h, cy+h)

def circ(cx, cy, d):
    return Point(cx, cy).buffer(d/2.0, resolution=48)

# ========================================================= TOP PLATE
def build_top():
    outer = sbox(0,0,EDGE,EDGE)
    corner_holes = [circ(x,y,M3) for x,y in CORNERS]
    # web layer: 14mm cutouts, top 1.5mm
    web_poly = Polygon(outer.exterior.coords,
                       [c.exterior.coords[::-1] for c in
                        ([sq(cx,cy,MX_CUT) for cx,cy in centres()] + corner_holes)])
    web = extrude_polygon(web_poly, WEB_T)
    web.apply_translation([0,0,LEG_H+REL_T])          # sits on top
    # relief layer: 15.6mm openings, bottom 1.5mm
    rel_poly = Polygon(outer.exterior.coords,
                       [c.exterior.coords[::-1] for c in
                        ([sq(cx,cy,RELIEF) for cx,cy in centres()] + corner_holes)])
    rel = extrude_polygon(rel_poly, REL_T)
    rel.apply_translation([0,0,LEG_H])
    # 4 corner legs (ring: outer LEG_D minus M3 clearance), height LEG_H
    legs=[]
    for x,y in CORNERS:
        ring = Polygon(circ(x,y,LEG_D).exterior.coords,
                       [circ(x,y,M3).exterior.coords[::-1]])
        leg = extrude_polygon(ring, LEG_H)
        legs.append(leg)
    parts=[web,rel]+legs
    mesh = trimesh.boolean.union(parts)
    return mesh

# ========================================================= BOTTOM PLATE
def build_bottom():
    BT=3.0
    outer = sbox(0,0,EDGE,EDGE)
    holes=[circ(x,y,M3) for x,y in CORNERS]
    # cable exit slot on one edge (centered, 24 x 10) + a few vents
    slot = sbox(EDGE/2-12, -0.1, EDGE/2+12, 9.9)
    vents=[]
    for gx in range(1,9):
        for gy in range(1,9):
            vents.append(circ(C0+ (gx-0.5+0.5)*0, 0, 0))  # placeholder, unused
    cutters=[h for h in holes]+[slot]
    poly = Polygon(outer.exterior.coords,
                   [c.exterior.coords[::-1] for c in cutters])
    mesh = extrude_polygon(poly, BT)
    return mesh

top = build_top()
bot = build_bottom()

for name,m in [("top_plate",top),("bottom_plate",bot)]:
    m.merge_vertices(); m.update_faces(m.nondegenerate_faces()); m.fix_normals()
    p=os.path.join(OUT, name+".stl")
    m.export(p)
    print(f"{name}: watertight={m.is_watertight} vol={m.volume/1000:.1f}cm3 "
          f"bbox={np.round(m.extents,1)} -> {os.path.basename(p)}")
