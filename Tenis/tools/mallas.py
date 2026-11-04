# -*- coding: utf-8 -*-
"""
mallas.py - Mini librería para generar modelos .OBJ/.MTL con textura (sin dependencias raras).

Cada modelo se escribe en  Models/<Carpeta>/<Nombre>.obj  (+ .mtl + .png), igual que los
modelos de clase: UN solo material con UNA textura.  Para los objetos de colores lisos la textura
es una "paleta" (cuadritos de color) y cada cara apunta al cuadrito que le toca.
"""
import math, os
import numpy as np
from PIL import Image

AQUI = os.path.dirname(os.path.abspath(__file__))
PROYECTO = os.path.abspath(os.path.join(AQUI, ".."))
MODELOS = os.path.join(PROYECTO, "Models")
IMAGENES = os.path.join(PROYECTO, "images")


def v3(*a): return np.array(a, dtype=float)
def norm(v):
    l = np.linalg.norm(v)
    return v / l if l > 1e-12 else v


def rot_x(g):
    c, s = math.cos(math.radians(g)), math.sin(math.radians(g))
    return np.array([[1, 0, 0], [0, c, -s], [0, s, c]])
def rot_y(g):
    c, s = math.cos(math.radians(g)), math.sin(math.radians(g))
    return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])
def rot_z(g):
    c, s = math.cos(math.radians(g)), math.sin(math.radians(g))
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])


class Atlas:
    """Textura del modelo: una zona superior opcional (imagen propia) + una paleta de 8x8 colores."""
    SW = 16
    def __init__(self, ancho=128, alto_sup=0, img_sup=None, alfa=False):
        self.w = max(ancho, 128)
        self.top_h = alto_sup
        self.h = alto_sup + 128
        self.sup = img_sup
        self.colores = []
        self.alfa = alfa

    def idx(self, rgb):
        rgb = tuple(int(round(c)) for c in rgb[:3])
        if rgb not in self.colores:
            if len(self.colores) >= 64:
                raise ValueError("paleta llena")
            self.colores.append(rgb)
        return self.colores.index(rgb)

    def uv(self, rgb):
        i = self.idx(rgb)
        col, fila = i % 8, i // 8
        return ((col * self.SW + self.SW / 2) / self.w, (self.top_h + fila * self.SW + self.SW / 2) / self.h)

    def rect_uv(self, x0, y0, x1, y1):
        """Rectángulo de la zona superior en coordenadas normalizadas de imagen (v hacia abajo)."""
        return (x0 / self.w, y0 / self.h, x1 / self.w, y1 / self.h)

    def imagen(self):
        modo = "RGBA" if self.alfa else "RGB"
        im = Image.new(modo, (self.w, self.h), (255, 255, 255, 255) if self.alfa else (255, 255, 255))
        if self.sup is not None:
            im.paste(self.sup.convert(modo), (0, 0))
        for i, c in enumerate(self.colores):
            col, fila = i % 8, i // 8
            x, y = col * self.SW, self.top_h + fila * self.SW
            sw = Image.new(modo, (self.SW, self.SW), c + ((255,) if self.alfa else ()))
            im.paste(sw, (x, y))
        return im


class Obj:
    def __init__(self, atlas):
        self.a = atlas
        self.v, self.vt, self.vn, self.f = [], [], [], []

    # ---- triángulo con normales explícitas -------------------------------------------------
    def tri(self, p, uv, n):
        base = len(self.v)
        for k in range(3):
            self.v.append(tuple(p[k])); self.vt.append((uv[k][0], 1.0 - uv[k][1])); self.vn.append(tuple(n[k]))
        self.f.append((base + 1, base + 2, base + 3))

    def quad_plano(self, p0, p1, p2, p3, uv, salida=None):
        """Quad plano (normal por cara). Se orienta para que la normal apunte a 'salida' si se da."""
        p = [np.array(x, float) for x in (p0, p1, p2, p3)]
        n = np.cross(p[1] - p[0], p[2] - p[0])
        if np.linalg.norm(n) < 1e-12:
            n = np.cross(p[2] - p[0], p[3] - p[0])
        n = norm(n)
        if salida is not None and np.dot(n, salida) < 0:
            p = [p[0], p[3], p[2], p[1]]
            uv = [uv[0], uv[3], uv[2], uv[1]]
            n = -n
        self.tri([p[0], p[1], p[2]], [uv[0], uv[1], uv[2]], [n, n, n])
        self.tri([p[0], p[2], p[3]], [uv[0], uv[2], uv[3]], [n, n, n])

    # ---- primitivas ---------------------------------------------------------------------
    FACES = {
        "+x": (v3(1, 0, 0), v3(0, 0, -1), v3(0, 1, 0)), "-x": (v3(-1, 0, 0), v3(0, 0, 1), v3(0, 1, 0)),
        "+y": (v3(0, 1, 0), v3(1, 0, 0), v3(0, 0, -1)), "-y": (v3(0, -1, 0), v3(1, 0, 0), v3(0, 0, 1)),
        "+z": (v3(0, 0, 1), v3(1, 0, 0), v3(0, 1, 0)), "-z": (v3(0, 0, -1), v3(-1, 0, 0), v3(0, 1, 0)),
    }

    def caja(self, c, tam, color=(200, 200, 200), rot=None, caras=None, colores=None):
        """Caja centrada en c. caras: {'+z': (u0,v0,u1,v1)} usa una zona de la imagen en esa cara.
        colores: {'+y': (r,g,b)} para pintar caras con otro color."""
        c = np.array(c, float); h = np.array(tam, float) / 2.0
        R = np.eye(3) if rot is None else rot
        for nombre, (n, u, v) in self.FACES.items():
            hn, hu, hv = abs(n) @ h, abs(u) @ h, abs(v) @ h
            ctr = n * hn
            esq = [ctr - u * hu - v * hv, ctr + u * hu - v * hv, ctr + u * hu + v * hv, ctr - u * hu + v * hv]
            esq = [c + R @ e for e in esq]
            if caras and nombre in caras:
                u0, v0, u1, v1 = caras[nombre]
                uv = [(u0, v1), (u1, v1), (u1, v0), (u0, v0)]
            else:
                col = colores[nombre] if (colores and nombre in colores) else color
                t = self.a.uv(col)
                uv = [t, t, t, t]
            self.quad_plano(esq[0], esq[1], esq[2], esq[3], uv, salida=R @ n)

    def cilindro(self, p0, p1, r0, r1, color=(200, 200, 200), seg=12, tapas=True):
        p0 = np.array(p0, float); p1 = np.array(p1, float)
        eje = norm(p1 - p0)
        ref = v3(0, 0, 1) if abs(eje[2]) < 0.9 else v3(1, 0, 0)
        u = norm(np.cross(eje, ref)); w = np.cross(eje, u)
        t = self.a.uv(color); uvs = [t, t, t]
        for i in range(seg):
            a0, a1 = 2 * math.pi * i / seg, 2 * math.pi * (i + 1) / seg
            d0 = math.cos(a0) * u + math.sin(a0) * w; d1 = math.cos(a1) * u + math.sin(a1) * w
            a, b, c, d = p0 + d0 * r0, p0 + d1 * r0, p1 + d1 * r1, p1 + d0 * r1
            for tri_p, tri_n in (([a, b, c], [d0, d1, d1]), ([a, c, d], [d0, d1, d0])):
                n = np.cross(tri_p[1] - tri_p[0], tri_p[2] - tri_p[0])
                if np.dot(n, (d0 + d1)) < 0:
                    tri_p = [tri_p[0], tri_p[2], tri_p[1]]; tri_n = [tri_n[0], tri_n[2], tri_n[1]]
                self.tri(tri_p, uvs, tri_n)
        if tapas:
            for (cen, rr, sgn) in ((p0, r0, -1.0), (p1, r1, 1.0)):
                if rr <= 1e-6:
                    continue
                nn = eje * sgn
                for i in range(seg):
                    a0, a1 = 2 * math.pi * i / seg, 2 * math.pi * (i + 1) / seg
                    q0 = cen + (math.cos(a0) * u + math.sin(a0) * w) * rr
                    q1 = cen + (math.cos(a1) * u + math.sin(a1) * w) * rr
                    pts = [cen, q0, q1]
                    if np.dot(np.cross(pts[1] - pts[0], pts[2] - pts[0]), nn) < 0:
                        pts = [cen, q1, q0]
                    self.tri(pts, uvs, [nn, nn, nn])

    def esfera(self, c, r, color=(200, 200, 200), seg=12, anillos=8, esc=(1, 1, 1), uv_func=None):
        c = np.array(c, float); esc = np.array(esc, float)
        def P(i, j):
            th, ph = math.pi * j / anillos, 2 * math.pi * i / seg
            d = v3(math.sin(th) * math.cos(ph), math.cos(th), math.sin(th) * math.sin(ph))
            return c + d * r * esc, norm(d / esc), d
        t = self.a.uv(color)
        for j in range(anillos):
            for i in range(seg):
                a, b, cc, d = P(i, j), P(i + 1, j), P(i + 1, j + 1), P(i, j + 1)
                quads = []
                if j != 0: quads.append((a, d, b))
                if j != anillos - 1: quads.append((b, d, cc))
                for q in quads:
                    pts = [q[0][0], q[1][0], q[2][0]]
                    ns = [q[0][1], q[1][1], q[2][1]]
                    if np.dot(np.cross(pts[1] - pts[0], pts[2] - pts[0]), sum(x[2] for x in q)) < 0:
                        pts = [pts[0], pts[2], pts[1]]; ns = [ns[0], ns[2], ns[1]]; q = (q[0], q[2], q[1])
                    if uv_func:
                        uvs = [uv_func(i, j, x) for x in q]
                    else:
                        uvs = [t, t, t]
                    self.tri(pts, uvs, ns)

    # ---- escritura -----------------------------------------------------------------------
    def escribir(self, carpeta, nombre, imagen=None):
        d = os.path.join(MODELOS, carpeta)
        os.makedirs(d, exist_ok=True)
        (imagen if imagen is not None else self.a.imagen()).save(os.path.join(d, nombre + ".png"))
        with open(os.path.join(d, nombre + ".mtl"), "w") as f:
            f.write("# %s\nnewmtl material\nKd 1.0 1.0 1.0\nKs 0.0 0.0 0.0\nNs 8\nmap_Kd %s.png\n" % (nombre, nombre))
        with open(os.path.join(d, nombre + ".obj"), "w") as f:
            f.write("# Generado por tools/generar_modelos.py\nmtllib %s.mtl\no %s\n" % (nombre, nombre))
            for p in self.v: f.write("v %.5f %.5f %.5f\n" % p)
            for t in self.vt: f.write("vt %.5f %.5f\n" % t)
            for n in self.vn: f.write("vn %.5f %.5f %.5f\n" % n)
            f.write("usemtl material\n")
            for a, b, c in self.f:
                f.write("f %d/%d/%d %d/%d/%d %d/%d/%d\n" % (a, a, a, b, b, b, c, c, c))
        return len(self.f)
