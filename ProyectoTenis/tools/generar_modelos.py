#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Genera los modelos .OBJ/.MTL (y sus texturas PNG) que el proyecto carga con Assimp:
  raqueta, pelota, botella, toalla.
Uso:  python tools/generar_modelos.py      (requiere Pillow: pip install pillow)
Los archivos se escriben en ../Models. Puedes sustituirlos por modelos hechos en Blender
conservando los mismos nombres.
Unidades: metros. Raqueta: mango en el origen, apuntando a +Y, cara de cuerdas en el plano XY.
"""
import math, os, random
from PIL import Image, ImageDraw

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Models")
os.makedirs(OUT, exist_ok=True)


class Obj:
    def __init__(self, name, mtl):
        self.name, self.mtl = name, mtl
        self.v, self.vt, self.vn = [], [], []
        self.groups = []            # (material, [caras])
        self.cur = None

    def use(self, material):
        self.cur = (material, [])
        self.groups.append(self.cur)

    def vert(self, p, n, uv):
        self.v.append(p); self.vn.append(n); self.vt.append(uv)
        return len(self.v)          # indice 1-based (v, vt y vn comparten indice)

    def tri(self, a, b, c):
        self.cur[1].append((a, b, c))

    def quad(self, a, b, c, d):
        self.tri(a, b, c); self.tri(a, c, d)

    def write(self):
        with open(os.path.join(OUT, self.name + ".obj"), "w") as f:
            f.write("# Generado por tools/generar_modelos.py\nmtllib %s.mtl\n" % self.name)
            for p in self.v:  f.write("v %.5f %.5f %.5f\n" % p)
            for t in self.vt: f.write("vt %.5f %.5f\n" % t)
            for n in self.vn: f.write("vn %.5f %.5f %.5f\n" % n)
            for mat, faces in self.groups:
                f.write("usemtl %s\n" % mat)
                for (a, b, c) in faces:
                    f.write("f %d/%d/%d %d/%d/%d %d/%d/%d\n" % (a, a, a, b, b, b, c, c, c))
        with open(os.path.join(OUT, self.name + ".mtl"), "w") as f:
            f.write(self.mtl)


def norm(v):
    l = math.sqrt(sum(c * c for c in v)) or 1.0
    return tuple(c / l for c in v)

def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])

def add_tube(o, p0, p1, r0, r1, seg=10, cap=True):
    """Cilindro/cono entre dos puntos arbitrarios."""
    axis = norm(tuple(p1[i]-p0[i] for i in range(3)))
    ref = (0, 0, 1) if abs(axis[2]) < 0.9 else (1, 0, 0)
    u = norm(cross(axis, ref)); w = cross(axis, u)
    ring0, ring1 = [], []
    for i in range(seg + 1):
        a = 2 * math.pi * i / seg
        d = tuple(math.cos(a)*u[k] + math.sin(a)*w[k] for k in range(3))
        q0 = tuple(p0[k] + d[k]*r0 for k in range(3)); q1 = tuple(p1[k] + d[k]*r1 for k in range(3))
        ring0.append(o.vert(q0, d, (i/seg, 0.0))); ring1.append(o.vert(q1, d, (i/seg, 1.0)))
    for i in range(seg):
        o.quad(ring0[i], ring0[i+1], ring1[i+1], ring1[i])
    if cap:
        for (p, rr, sgn) in ((p0, r0, -1), (p1, r1, 1)):
            n = tuple(sgn*axis[k] for k in range(3))
            c = o.vert(p, n, (0.5, 0.5))
            vs = []
            for i in range(seg + 1):
                a = 2 * math.pi * i / seg
                q = tuple(p[k] + (math.cos(a)*u[k] + math.sin(a)*w[k])*rr for k in range(3))
                vs.append(o.vert(q, n, (0.5+0.5*math.cos(a), 0.5+0.5*math.sin(a))))
            for i in range(seg):
                if sgn > 0: o.tri(c, vs[i], vs[i+1])
                else:       o.tri(c, vs[i+1], vs[i])

def add_sphere(o, c, r, seg=24, rings=14):
    idx = []
    for j in range(rings + 1):
        th = math.pi * j / rings
        row = []
        for i in range(seg + 1):
            ph = 2 * math.pi * i / seg
            n = (math.sin(th)*math.cos(ph), math.cos(th), math.sin(th)*math.sin(ph))
            p = (c[0]+r*n[0], c[1]+r*n[1], c[2]+r*n[2])
            row.append(o.vert(p, n, (i/seg, 1.0 - j/rings)))
        idx.append(row)
    for j in range(rings):
        for i in range(seg):
            a, b, cc, d = idx[j][i], idx[j][i+1], idx[j+1][i+1], idx[j+1][i]
            o.quad(a, d, cc, b)


# ------------------------------------------------------------------ RAQUETA
def raqueta():
    o = Obj("raqueta", """# Raqueta de tenis
newmtl Marco
Kd 0.82 0.10 0.10
Ks 0.60 0.60 0.60
Ns 60
newmtl Grip
Kd 0.07 0.07 0.08
Ks 0.10 0.10 0.10
Ns 8
newmtl Cuerdas
Kd 1.0 1.0 1.0
Ks 0.15 0.15 0.15
Ns 20
map_Kd cuerdas.png
""")
    a, b, yc, rt = 0.130, 0.1925, 0.4925, 0.0105     # semiejes de la cabeza, centro y radio del tubo
    # Mango (grip) + pomo
    o.use("Grip")
    add_tube(o, (0, 0.0, 0), (0, 0.20, 0), 0.0155, 0.0135, 12)
    add_tube(o, (0, -0.012, 0), (0, 0.0, 0), 0.0185, 0.0155, 12)
    # Garganta en "Y"
    o.use("Marco")
    for s in (-1, 1):
        th = math.radians(-90 + s * 28)
        top = (a * math.cos(th), yc + b * math.sin(th), 0.0)
        add_tube(o, (s * 0.006, 0.20, 0), top, 0.0095, 0.0095, 8, cap=False)
    # Marco eliptico (toroide)
    seg, tube = 56, 10
    rows = []
    for i in range(seg + 1):
        th = 2 * math.pi * i / seg
        E = (a * math.cos(th), yc + b * math.sin(th))
        N2 = norm((b * math.cos(th), a * math.sin(th), 0.0))
        row = []
        for j in range(tube + 1):
            ph = 2 * math.pi * j / tube
            n = (N2[0]*math.cos(ph), N2[1]*math.cos(ph), math.sin(ph))
            p = (E[0] + rt*n[0], E[1] + rt*n[1], rt*n[2])
            row.append(o.vert(p, n, (i / seg, j / tube)))
        rows.append(row)
    for i in range(seg):
        for j in range(tube):
            o.quad(rows[i][j], rows[i+1][j], rows[i+1][j+1], rows[i][j+1])
    # Cuerdas: disco eliptico de dos caras con textura alpha
    o.use("Cuerdas")
    ia, ib = a - rt * 0.6, b - rt * 0.6
    cs = 36
    for sgn in (1, -1):
        c = o.vert((0, yc, 0.0015 * sgn), (0, 0, sgn), (0.5, 0.5))
        ring = []
        for i in range(cs + 1):
            th = 2 * math.pi * i / cs
            p = (ia * math.cos(th), yc + ib * math.sin(th), 0.0015 * sgn)
            ring.append(o.vert(p, (0, 0, sgn), (0.5 + 0.5*math.cos(th), 0.5 + 0.5*math.sin(th))))
        for i in range(cs):
            if sgn > 0: o.tri(c, ring[i], ring[i+1])
            else:       o.tri(c, ring[i+1], ring[i])
    o.write()
    # textura de cuerdas (RGBA)
    N = 256
    img = Image.new("RGBA", (N, N), (255, 255, 255, 0))
    d = ImageDraw.Draw(img)
    for k in range(0, N, 12):
        d.line([(k, 0), (k, N)], fill=(245, 245, 235, 255), width=2)
        d.line([(0, k), (N, k)], fill=(245, 245, 235, 255), width=2)
    img.save(os.path.join(OUT, "cuerdas.png"))


# ------------------------------------------------------------------ PELOTA
def pelota():
    o = Obj("pelota", """# Pelota de tenis
newmtl Fieltro
Kd 1.0 1.0 1.0
Ks 0.05 0.05 0.05
Ns 4
map_Kd pelota.png
""")
    o.use("Fieltro")
    add_sphere(o, (0, 0.0335, 0), 0.0335, 28, 16)        # apoyada sobre y=0
    o.write()
    W, H = 512, 256
    img = Image.new("RGB", (W, H), (208, 232, 54))
    px = img.load()
    random.seed(3)
    for y in range(H):                                    # fieltro: ruido fino
        for x in range(W):
            n = random.randint(-14, 14)
            r, g, b = px[x, y]
            px[x, y] = (max(0, min(255, r+n)), max(0, min(255, g+n)), max(0, min(255, b+n//2)))
    d = ImageDraw.Draw(img)
    for sgn, off in ((1, 0.0), (-1, 0.5)):               # costura (dos lobulos)
        pts = []
        for i in range(0, W + 1, 2):
            u = i / W
            v = 0.5 + sgn * 0.22 * math.sin(2 * math.pi * (u + off))
            pts.append((i, v * H))
        d.line(pts, fill=(246, 246, 240), width=7)
    img.save(os.path.join(OUT, "pelota.png"))


# ------------------------------------------------------------------ BOTELLA
def botella():
    o = Obj("botella", """# Botella de agua
newmtl Plastico
Kd 0.50 0.78 0.95
Ks 0.80 0.80 0.80
Ns 90
newmtl Tapa
Kd 0.95 0.95 0.98
Ks 0.30 0.30 0.30
Ns 20
newmtl Etiqueta
Kd 0.10 0.30 0.75
Ks 0.10 0.10 0.10
Ns 10
""")
    o.use("Plastico")
    add_tube(o, (0, 0, 0), (0, 0.140, 0), 0.034, 0.034, 20)
    add_tube(o, (0, 0.140, 0), (0, 0.175, 0), 0.034, 0.013, 20, cap=False)
    add_tube(o, (0, 0.175, 0), (0, 0.190, 0), 0.013, 0.013, 20, cap=False)
    o.use("Etiqueta")
    add_tube(o, (0, 0.045, 0), (0, 0.105, 0), 0.0345, 0.0345, 20, cap=False)
    o.use("Tapa")
    add_tube(o, (0, 0.190, 0), (0, 0.205, 0), 0.0155, 0.0155, 16)
    o.write()


# ------------------------------------------------------------------ TOALLA
def toalla():
    o = Obj("toalla", """# Toalla colgada
newmtl Tela
Kd 1.0 1.0 1.0
Ks 0.0 0.0 0.0
Ns 1
map_Kd toalla.png
""")
    o.use("Tela")
    # Perfil (z,y) de la tela colgada sobre un respaldo (eje del respaldo: x):
    # cuelga 0.40 por delante y 0.55 por detras.
    prof = []
    steps = 14
    for i in range(steps + 1):                      # cara delantera, de abajo hacia arriba
        t = i / steps
        prof.append((0.012, -0.40 + 0.40 * t))
    for i in range(1, 9):                           # arco superior
        ang = math.pi * i / 8
        prof.append((0.012 * math.cos(ang), 0.012 * math.sin(ang)))
    for i in range(1, steps + 1):                   # cara trasera, hacia abajo
        t = i / steps
        prof.append((-0.012, -0.55 * t))
    W, NX = 0.46, 10
    for side in (1, -1):                            # dos caras para ver la tela por ambos lados
        grid = []
        for j, (pz, py) in enumerate(prof):
            row = []
            nz = 1.0 if pz > 0 else -1.0
            for i in range(NX + 1):
                x = -W / 2 + W * i / NX
                row.append(o.vert((x, py, pz + 0.0012 * side * nz),
                                  (0, 0, nz * side), (i / NX, j / (len(prof) - 1))))
            grid.append(row)
        for j in range(len(prof) - 1):
            for i in range(NX):
                a, b, c, d = grid[j][i], grid[j][i+1], grid[j+1][i+1], grid[j+1][i]
                if side > 0: o.quad(a, b, c, d)
                else:        o.quad(a, d, c, b)
    o.write()
    img = Image.new("RGB", (128, 256), (245, 245, 240))
    d = ImageDraw.Draw(img)
    for y0 in (14, 22, 232, 240):
        d.rectangle([0, y0, 127, y0 + 5], fill=(30, 90, 190))
    img.save(os.path.join(OUT, "toalla.png"))


if __name__ == "__main__":
    raqueta(); pelota(); botella(); toalla()
    print("Modelos generados en", os.path.abspath(OUT))
