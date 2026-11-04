# -*- coding: utf-8 -*-
"""
generar_modelos.py
Genera TODOS los modelos 3D (.obj/.mtl/.png) de la carpeta Models/ y las imágenes de images/.
Se ejecuta una sola vez:   python tools/generar_modelos.py      (necesita:  pip install pillow numpy)
Después puedes abrir cualquier .obj en Blender y modificarlo: el programa solo carga los archivos.

Unidades: metros.  Convención de los modelos: frente = +Z, arriba = +Y.
"""
import math, os, random
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter
from mallas import *

FUENTE = None
for ruta in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", "C:/Windows/Fonts/arialbd.ttf",
             "C:/Windows/Fonts/arial.ttf", "/Library/Fonts/Arial Bold.ttf"):
    if os.path.exists(ruta):
        FUENTE = ruta
        break

def fuente(px):
    if FUENTE:
        return ImageFont.truetype(FUENTE, px)
    return ImageFont.load_default(size=px)

def texto_centrado(dib, caja, texto, px, color):
    f = fuente(px)
    x0, y0, x1, y1 = caja
    bb = dib.textbbox((0, 0), texto, font=f)
    w, h = bb[2] - bb[0], bb[3] - bb[1]
    dib.text((x0 + (x1 - x0 - w) / 2 - bb[0], y0 + (y1 - y0 - h) / 2 - bb[1]), texto, font=f, fill=color)


# =====================================================================================
#  CANCHA  (textura con las líneas reglamentarias dibujadas)
# =====================================================================================
PPM = 40            # píxeles por metro en la textura de la cancha
PISO_HX, PISO_HZ = 10.0, 20.0

def textura_cancha(tipo):
    W, H = int(2 * PISO_HX * PPM), int(2 * PISO_HZ * PPM)
    rng = np.random.RandomState(7)
    def capa(escala, amp):
        pq = rng.rand(H // escala + 2, W // escala + 2)
        im = Image.fromarray((pq * 255).astype("uint8")).resize((W, H), Image.BICUBIC)
        return (np.asarray(im, float) / 255.0 - 0.5) * amp
    zs = (np.arange(H)[:, None] - H / 2) / PPM
    xs = (np.arange(W)[None, :] - W / 2) / PPM
    if tipo == "arcilla":
        base = np.array([196, 98, 58], float)
        img = base[None, None, :] + (capa(40, 26) + capa(8, 14) + capa(2, 18))[:, :, None] * np.array([1.0, 0.8, 0.6])
    elif tipo == "pasto":
        base = np.array([58, 128, 46], float)
        franja = (np.floor(zs / 2.5) % 2) * 14 - 7
        img = base[None, None, :] + (capa(30, 14) + capa(3, 20))[:, :, None] * np.array([0.7, 1.0, 0.6]) + franja[:, :, None] * np.array([0.5, 1, 0.4])
    else:  # cemento
        dentro = (np.abs(xs) < 8.5) & (np.abs(zs) < 16.8)
        azul = np.array([40, 88, 160], float); verde = np.array([62, 122, 92], float)
        base = np.where(dentro[:, :, None], azul[None, None, :], verde[None, None, :])
        img = base + (capa(50, 10) + capa(3, 10))[:, :, None]
    im = Image.fromarray(np.clip(img, 0, 255).astype("uint8"))
    d = ImageDraw.Draw(im)
    blanco = (244, 244, 238)
    def rect(x0, x1, z0, z1):
        d.rectangle([W / 2 + x0 * PPM, H / 2 + z0 * PPM, W / 2 + x1 * PPM - 1, H / 2 + z1 * PPM - 1], fill=blanco)
    HL, HW, SW, SZ = 11.885, 5.485, 4.115, 6.40
    lw, bw = 0.07, 0.12
    rect(-HW, HW, HL - bw, HL); rect(-HW, HW, -HL, -HL + bw)             # líneas de fondo
    rect(HW - lw, HW, -HL, HL); rect(-HW, -HW + lw, -HL, HL)             # laterales de dobles
    rect(SW - lw, SW, -SZ, SZ); rect(-SW, -SW + lw, -SZ, SZ)             # laterales de sencillos
    rect(-SW, SW, SZ - lw, SZ); rect(-SW, SW, -SZ, -SZ + lw)             # líneas de saque
    rect(-lw / 2, lw / 2, -SZ, SZ)                                       # línea central de saque
    rect(-lw / 2, lw / 2, HL - bw - 0.12, HL - bw); rect(-lw / 2, lw / 2, -HL + bw, -HL + bw + 0.12)
    return im.filter(ImageFilter.GaussianBlur(0.5))


def modelo_cancha(tipo, nombre):
    a = Atlas()
    o = Obj(a)
    p = [(-PISO_HX, 0, -PISO_HZ), (PISO_HX, 0, -PISO_HZ), (PISO_HX, 0, PISO_HZ), (-PISO_HX, 0, PISO_HZ)]
    uv = [(0, 0), (1, 0), (1, 1), (0, 1)]
    o.quad_plano(p[0], p[3], p[2], p[1], [uv[0], uv[3], uv[2], uv[1]], salida=v3(0, 1, 0))
    print(" ", nombre, o.escribir("Cancha", nombre, textura_cancha(tipo)), "triángulos")


def modelo_piso():
    rng = np.random.RandomState(3)
    r = rng.randint(40, 90, (256, 256))
    im = Image.fromarray(np.stack([r, r, (r * 1.04).clip(0, 255)], -1).astype("uint8")).filter(ImageFilter.GaussianBlur(0.8))
    a = Atlas(); o = Obj(a); S = 300; R = 60
    p = [(-S, -0.04, -S), (S, -0.04, -S), (S, -0.04, S), (-S, -0.04, S)]
    o.quad_plano(p[0], p[3], p[2], p[1], [(0, 0), (0, R), (R, R), (R, 0)], salida=v3(0, 1, 0))
    print("  Piso", o.escribir("Piso", "Piso", im))


# =====================================================================================
#  RED + POSTES
# =====================================================================================
def modelo_red():
    img = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, 255, 17], fill=(245, 245, 245, 255))              # banda blanca superior
    for k in range(0, 256, 16):
        d.line([(k, 18), (k, 255)], fill=(25, 25, 28, 255), width=2)
    for k in range(26, 256, 10):
        d.line([(0, k), (255, k)], fill=(25, 25, 28, 255), width=2)
    a = Atlas(); o = Obj(a)
    PX, H0, H1 = 6.40, 0.914, 1.07
    n = 24
    pts = []
    for i in range(n + 1):
        x = -PX + 2 * PX * i / n
        top = H0 + (H1 - H0) * (x / PX) ** 2
        pts.append((x, top))
    for i in range(n):
        (x0, t0), (x1, t1) = pts[i], pts[i + 1]
        u0, u1 = (x0 + PX) / 0.64, (x1 + PX) / 0.64
        for cara in (1, -1):
            q = [(x0, 0.03, 0), (x1, 0.03, 0), (x1, t1, 0), (x0, t0, 0)]
            uv = [(u0, 1.0), (u1, 1.0), (u1, 0.0), (u0, 0.0)]
            o.quad_plano(*q, uv, salida=v3(0, 0, cara))
    print("  Red", o.escribir("Red", "Red", img))
    # postes
    a = Atlas(); o = Obj(a)
    oscuro = (40, 42, 48)
    for s in (-1, 1):
        o.cilindro((s * PX, 0, 0), (s * PX, H1 + 0.02, 0), 0.055, 0.045, oscuro, 10)
        o.caja((s * PX, 0.01, 0), (0.30, 0.02, 0.30), oscuro)
    o.caja((0, 0.46, 0), (0.05, 0.88, 0.012), (245, 245, 245))          # cinta central
    print("  Postes", o.escribir("Red", "Postes"))


# =====================================================================================
#  SILLA DEL JUEZ
# =====================================================================================
def modelo_silla_juez():
    a = Atlas(); o = Obj(a)
    metal, oscuro, azul = (205, 208, 212), (25, 28, 34), (20, 40, 115)
    for sx in (-1, 1):
        for sz in (-1, 1):
            o.cilindro((sx * 0.44, 0, sz * 0.44), (sx * 0.40, 2.30, sz * 0.40), 0.035, 0.035, metal, 8)
    for y in (0.75, 1.55):
        for s in (-1, 1):
            o.caja((0, y, s * 0.42), (0.84, 0.04, 0.04), metal)
            o.caja((s * 0.42, y, 0), (0.04, 0.04, 0.84), metal)
    o.caja((0, 2.30, 0), (0.98, 0.08, 0.98), oscuro)
    o.caja((0, 2.37, 0), (0.66, 0.06, 0.66), azul)
    o.caja((0, 2.78, -0.42), (0.70, 0.70, 0.07), azul, rot=rot_x(-8))
    for s in (-1, 1):
        o.caja((s * 0.40, 2.62, -0.02), (0.06, 0.05, 0.64), oscuro)
        o.cilindro((s * 0.40, 2.34, 0.22), (s * 0.40, 2.62, 0.22), 0.015, 0.015, metal, 6)
    o.caja((0, 1.92, 0.70), (0.80, 0.05, 0.36), oscuro)                      # estribo
    for s in (-1, 1):
        o.cilindro((s * 0.38, 1.90, 0.52), (s * 0.38, 2.28, 0.44), 0.02, 0.02, metal, 6)
        o.cilindro((s * 0.32, 0, -1.45), (s * 0.32, 2.28, -0.50), 0.025, 0.025, metal, 8)   # escalera
    for i in range(1, 7):
        t = i / 7.0
        y, z = 2.28 * t, -1.45 + 0.95 * t
        o.caja((0, y, z), (0.64, 0.035, 0.07), metal, rot=rot_x(-22))
    o.cilindro((0.40, 2.64, 0.28), (0.30, 2.92, 0.58), 0.012, 0.012, oscuro, 6)   # micrófono
    o.esfera((0.30, 2.93, 0.60), 0.035, oscuro, 8, 6)
    print("  SillaJuez", o.escribir("SillaJuez", "SillaJuez"))


# =====================================================================================
#  PERSONAJES POR PARTES (modelado jerárquico): torso, cabeza, brazo, antebrazo, muslo, pierna, pie
# =====================================================================================
OUTFITS = {
    "Rojo":  dict(skin=(232, 180, 144), hair=(60, 38, 22), shirt=(220, 40, 40), shorts=(245, 245, 245), trim=(255, 255, 255),
                  shoes=(245, 245, 245), socks=(245, 245, 245), cap=(250, 250, 250), gorra=True, banda=False),
    "Azul":  dict(skin=(160, 112, 80), hair=(22, 16, 12), shirt=(30, 100, 220), shorts=(20, 30, 92), trim=(255, 215, 30),
                  shoes=(235, 235, 90), socks=(245, 245, 245), cap=(30, 100, 220), gorra=False, banda=True),
    "Recogepelotas": dict(skin=(208, 158, 122), hair=(32, 24, 18), shirt=(28, 112, 82), shorts=(32, 36, 36), trim=(255, 255, 255),
                  shoes=(40, 40, 46), socks=(240, 240, 240), cap=(28, 112, 82), gorra=True, banda=False),
}

def partes_jugador(nombre, c):
    carpeta = "Jugador/" + nombre
    def nuevo(): a = Atlas(); return Obj(a)
    # ---- Torso (origen = centro de la cadera) ----
    o = nuevo()
    o.caja((0, 0, 0), (0.32, 0.18, 0.21), c["shorts"])
    o.caja((0, 0.36, 0), (0.35, 0.55, 0.20), c["shirt"])
    o.caja((0, 0.50, 0), (0.40, 0.24, 0.22), c["shirt"])
    o.caja((0, 0.095, 0), (0.355, 0.035, 0.205), c["trim"])
    for s in (-1, 1):
        o.esfera((s * 0.205, 0.52, 0), 0.07, c["shirt"], 10, 7)
    o.cilindro((0, 0.60, 0), (0, 0.70, 0), 0.04, 0.04, c["skin"], 8)
    o.escribir(carpeta, "Torso")
    # ---- Cabeza (origen = base del cuello) ----
    o = nuevo()
    o.esfera((0, 0.115, 0.005), 1.0, c["skin"], 14, 10, esc=(0.100, 0.118, 0.108))
    for s in (-1, 1):
        o.esfera((s * 0.037, 0.127, 0.103), 0.013, (15, 15, 20), 6, 5)
    o.esfera((0, 0.100, 0.112), 1.0, tuple(int(x * 0.92) for x in c["skin"]), 6, 5, esc=(0.016, 0.02, 0.016))
    if c["gorra"]:
        o.esfera((0, 0.150, -0.004), 1.0, c["cap"], 12, 8, esc=(0.109, 0.082, 0.116))
        o.caja((0, 0.150, 0.108), (0.15, 0.014, 0.085), c["cap"], rot=rot_x(-8))
    else:
        o.esfera((0, 0.155, -0.012), 1.0, c["hair"], 12, 8, esc=(0.106, 0.088, 0.112))
        if c["banda"]:
            o.cilindro((0, 0.148, 0), (0, 0.174, 0), 0.112, 0.112, c["trim"], 14, tapas=False)
    o.escribir(carpeta, "Cabeza")
    # ---- Brazo (origen = hombro) ----
    o = nuevo()
    o.cilindro((0, 0, 0), (0, -0.29, 0), 0.043, 0.037, c["skin"], 8)
    o.cilindro((0, -0.01, 0), (0, -0.16, 0), 0.053, 0.053, c["shirt"], 8)
    o.escribir(carpeta, "Brazo")
    # ---- Antebrazo + mano (origen = codo) ----
    o = nuevo()
    o.esfera((0, 0, 0), 0.038, c["skin"], 8, 6)
    o.cilindro((0, 0, 0), (0, -0.26, 0), 0.037, 0.032, c["skin"], 8)
    o.cilindro((0, -0.20, 0), (0, -0.245, 0), 0.040, 0.040, c["trim"], 8)
    o.esfera((0, -0.30, 0), 1.0, c["skin"], 8, 6, esc=(0.044, 0.052, 0.040))
    o.escribir(carpeta, "Antebrazo")
    # ---- Muslo (origen = cadera) ----
    o = nuevo()
    o.cilindro((0, 0, 0), (0, -0.44, 0), 0.075, 0.058, c["skin"], 8)
    o.cilindro((0, 0.02, 0), (0, -0.22, 0), 0.084, 0.084, c["shorts"], 8)
    o.escribir(carpeta, "Muslo")
    # ---- Pierna (origen = rodilla) ----
    o = nuevo()
    o.esfera((0, 0, 0), 0.056, c["skin"], 8, 6)
    o.cilindro((0, 0, 0), (0, -0.43, 0), 0.056, 0.045, c["skin"], 8)
    o.cilindro((0, -0.23, 0), (0, -0.43, 0), 0.052, 0.050, c["socks"], 8)
    o.escribir(carpeta, "Pierna")
    # ---- Pie (origen = tobillo) ----
    o = nuevo()
    o.caja((0, -0.04, 0.05), (0.105, 0.075, 0.265), c["shoes"])
    o.caja((0, -0.0695, 0.05), (0.108, 0.016, 0.268), (30, 30, 34))
    o.escribir(carpeta, "Pie")
    print("  Jugador/%s: 7 partes" % nombre)


def partes_juez():
    carpeta = "Juez"
    traje, camisa, piel, pelo = (22, 32, 90), (245, 245, 245), (218, 168, 132), (170, 170, 175)
    a = Atlas(); o = Obj(a)
    # cuerpo sentado (origen = cadera sobre el cojín)
    o.caja((0, 0, 0), (0.32, 0.18, 0.21), traje)
    o.caja((0, 0.36, 0), (0.35, 0.55, 0.20), camisa)
    o.caja((0, 0.50, 0), (0.40, 0.24, 0.22), camisa)
    o.caja((0, 0.43, 0.115), (0.07, 0.30, 0.01), (20, 40, 120))           # corbata
    for s in (-1, 1):
        o.esfera((s * 0.205, 0.52, 0), 0.07, camisa, 10, 7)
        o.cilindro((s * 0.095, -0.04, 0), (s * 0.095, -0.04, 0.44), 0.075, 0.062, traje, 8)       # muslos
        o.cilindro((s * 0.095, -0.04, 0.44), (s * 0.095, -0.47, 0.46), 0.060, 0.050, traje, 8)    # piernas
        o.caja((s * 0.095, -0.51, 0.51), (0.105, 0.075, 0.265), (25, 25, 28))
    o.cilindro((0, 0.60, 0), (0, 0.70, 0), 0.04, 0.04, piel, 8)
    o.escribir(carpeta, "CuerpoJuez")
    a = Atlas(); o = Obj(a)
    o.esfera((0, 0.115, 0.005), 1.0, piel, 14, 10, esc=(0.100, 0.118, 0.108))
    for s in (-1, 1):
        o.esfera((s * 0.037, 0.127, 0.103), 0.013, (15, 15, 20), 6, 5)
    o.esfera((0, 0.155, -0.012), 1.0, pelo, 12, 8, esc=(0.106, 0.088, 0.112))
    o.escribir(carpeta, "CabezaJuez")
    a = Atlas(); o = Obj(a)             # brazo con el antebrazo ya doblado hacia el frente (origen = hombro)
    o.cilindro((0, 0, 0), (0, -0.29, 0), 0.043, 0.037, camisa, 8)
    o.esfera((0, -0.29, 0), 0.038, piel, 8, 6)
    o.cilindro((0, -0.29, 0), (0, -0.29, 0.26), 0.037, 0.032, piel, 8)
    o.esfera((0, -0.29, 0.30), 1.0, piel, 8, 6, esc=(0.044, 0.040, 0.052))
    o.escribir(carpeta, "BrazoJuez")
    print("  Juez: 3 partes")


# =====================================================================================
#  RAQUETA y PELOTA
# =====================================================================================
def modelo_raqueta():
    cuerdas = Image.new("RGBA", (128, 128), (255, 255, 255, 0))
    d = ImageDraw.Draw(cuerdas)
    for k in range(0, 128, 8):
        d.line([(k, 0), (k, 127)], fill=(245, 245, 235, 255), width=2)
        d.line([(0, k), (127, k)], fill=(245, 245, 235, 255), width=2)
    a = Atlas(128, 128, cuerdas, alfa=True); o = Obj(a)
    marco, grip = (205, 28, 28), (20, 20, 24)
    aa, bb, yc, rt = 0.130, 0.1925, 0.4925, 0.0105
    o.cilindro((0, -0.012, 0), (0, 0.0, 0), 0.0185, 0.0155, grip, 10)
    o.cilindro((0, 0.0, 0), (0, 0.20, 0), 0.0155, 0.0135, grip, 10)
    for s in (-1, 1):
        th = math.radians(-90 + s * 28)
        o.cilindro((s * 0.006, 0.20, 0), (aa * math.cos(th), yc + bb * math.sin(th), 0), 0.0095, 0.0095, marco, 6, tapas=False)
    # marco elíptico (toroide)
    seg, tubo = 40, 8
    t = a.uv(marco)
    anillos = []
    for i in range(seg + 1):
        th = 2 * math.pi * i / seg
        E = v3(aa * math.cos(th), yc + bb * math.sin(th), 0)
        N2 = norm(v3(bb * math.cos(th), aa * math.sin(th), 0))
        fila = []
        for j in range(tubo + 1):
            ph = 2 * math.pi * j / tubo
            n = v3(N2[0] * math.cos(ph), N2[1] * math.cos(ph), math.sin(ph))
            fila.append((E + n * rt, n))
        anillos.append(fila)
    for i in range(seg):
        for j in range(tubo):
            A, B, C, D = anillos[i][j], anillos[i + 1][j], anillos[i + 1][j + 1], anillos[i][j + 1]
            for tri in ((A, B, C), (A, C, D)):
                pts = [x[0] for x in tri]; ns = [x[1] for x in tri]
                if np.dot(np.cross(pts[1] - pts[0], pts[2] - pts[0]), sum(ns)) < 0:
                    pts = [pts[0], pts[2], pts[1]]; ns = [ns[0], ns[2], ns[1]]
                o.tri(pts, [t, t, t], ns)
    # cuerdas: disco elíptico de dos caras (usa la zona superior del atlas con transparencia)
    ia, ib = aa - rt * 0.6, bb - rt * 0.6
    cs = 28
    for cara in (1, -1):
        for i in range(cs):
            a0, a1 = 2 * math.pi * i / cs, 2 * math.pi * (i + 1) / cs
            P0 = v3(0, yc, 0.0015 * cara)
            P1 = v3(ia * math.cos(a0), yc + ib * math.sin(a0), 0.0015 * cara)
            P2 = v3(ia * math.cos(a1), yc + ib * math.sin(a1), 0.0015 * cara)
            f = lambda p: (0.5 + 0.5 * (p[0] / ia), (0.5 - 0.5 * ((p[1] - yc) / ib)) * 128.0 / a.h)
            o.quad_plano(P0, P1, P2, P2, [f(P0), f(P1), f(P2), f(P2)], salida=v3(0, 0, cara))
    print("  Raqueta", o.escribir("Raqueta", "Raqueta"))


def modelo_pelota():
    rng = np.random.RandomState(3)
    W, H = 256, 128
    im = Image.new("RGB", (W, H), (208, 232, 54))
    px = np.asarray(im, float) + rng.randint(-12, 12, (H, W, 1))
    im = Image.fromarray(px.clip(0, 255).astype("uint8"))
    d = ImageDraw.Draw(im)
    for sgn, off in ((1, 0.0), (-1, 0.5)):
        pts = [(i, (0.5 + sgn * 0.22 * math.sin(2 * math.pi * (i / W + off))) * H) for i in range(0, W + 1, 2)]
        d.line(pts, fill=(246, 246, 240), width=5)
    a = Atlas(256, 128, im); o = Obj(a)
    seg, anillos = 20, 12
    def uvf(i, j, x):
        d_ = x[2]
        ph = math.atan2(d_[2], d_[0]) / (2 * math.pi) % 1.0
        th = math.acos(max(-1, min(1, d_[1]))) / math.pi
        return (ph, th * 128.0 / a.h)
    o.esfera((0, 0.0335, 0), 0.0335, (255, 255, 255), seg, anillos, uv_func=uvf)
    print("  Pelota", o.escribir("Pelota", "Pelota"))


def modelo_sombra():
    N = 64
    y, x = np.mgrid[0:N, 0:N]
    r = np.sqrt((x - N / 2 + 0.5) ** 2 + (y - N / 2 + 0.5) ** 2) / (N / 2)
    alfa = (np.clip(1 - r, 0, 1) ** 1.2 * 150).astype("uint8")
    im = Image.fromarray(np.stack([np.zeros((N, N), "uint8")] * 3 + [alfa], -1), "RGBA")
    a = Atlas(); o = Obj(a)
    p = [(-0.5, 0, -0.5), (0.5, 0, -0.5), (0.5, 0, 0.5), (-0.5, 0, 0.5)]
    o.quad_plano(p[0], p[3], p[2], p[1], [(0, 0), (0, 1), (1, 1), (1, 0)], salida=v3(0, 1, 0))
    print("  Sombra", o.escribir("Sombra", "Sombra", im))


# =====================================================================================
#  GRADAS, PÚBLICO, VALLAS
# =====================================================================================
GR_ANCHO, GR_FILAS, GR_PROF, GR_ALZA = 8.0, 10, 0.85, 0.42
GR_ASIENTOS = 16
PASO = GR_ANCHO / GR_ASIENTOS            # 0.5 m entre asientos

def modelo_grada(nombre, paleta):
    a = Atlas(); o = Obj(a)
    top, riser, muro = (186, 186, 192), (140, 140, 150), (110, 112, 120)
    for r in range(GR_FILAS):
        alto = (r + 1) * GR_ALZA
        z0 = r * GR_PROF
        o.caja((0, alto / 2, z0 + GR_PROF / 2), (GR_ANCHO, alto, GR_PROF), riser, colores={"+y": top})
        for k in range(GR_ASIENTOS):
            x = -GR_ANCHO / 2 + PASO / 2 + k * PASO
            col = paleta[((k // 4) + (r // 2)) % len(paleta)]
            o.caja((x, alto + 0.42, z0 + 0.58), (0.44, 0.06, 0.40), col)
            o.caja((x, alto + 0.68, z0 + 0.81), (0.44, 0.46, 0.06), col)
            o.caja((x, alto + 0.20, z0 + 0.58), (0.10, 0.40, 0.10), (50, 50, 56))
    total = GR_FILAS * GR_PROF; alto = GR_FILAS * GR_ALZA
    o.caja((0, (alto + 1.2) / 2, total + 0.15), (GR_ANCHO, alto + 1.2, 0.30), muro)
    o.caja((0, 0.45, -0.18), (GR_ANCHO, 0.9, 0.20), (150, 154, 162))
    print("  Grada", nombre, o.escribir("Gradas", nombre))


def persona(o, x, rng, mirar=-1.0):
    """Espectador sentado. Origen = centro del cojín. Mira hacia 'mirar' (-Z = hacia la cancha)."""
    camisas = [(235, 235, 238), (30, 70, 200), (210, 40, 40), (245, 195, 30), (30, 140, 80), (40, 40, 48),
               (240, 120, 30), (150, 50, 150), (80, 170, 220), (190, 190, 120)]
    pieles = [(244, 200, 168), (205, 150, 112), (125, 85, 60)]
    pelos = [(40, 28, 20), (20, 18, 16), (150, 110, 60), (200, 200, 200)]
    sh, sk, hr = camisas[rng.randint(0, len(camisas))], pieles[rng.randint(0, 3)], pelos[rng.randint(0, 4)]
    pant = (34, 42, 74)
    m = mirar
    for s in (-1, 1):
        o.caja((x + s * 0.11, 0.065, m * 0.21), (0.14, 0.13, 0.44), pant)
        o.caja((x + s * 0.11, -0.20, m * 0.41), (0.12, 0.42, 0.12), pant)
        o.caja((x + s * 0.11, -0.43, m * 0.47), (0.12, 0.06, 0.24), (22, 22, 24))
        o.caja((x + s * 0.25, 0.34, m * 0.04), (0.09, 0.40, 0.10), sh)
    o.caja((x, 0.36, 0), (0.40, 0.54, 0.24), sh)
    o.esfera((x, 0.80, m * 0.02), 1.0, sk, 8, 6, esc=(0.115, 0.115, 0.115))
    o.esfera((x, 0.86, -m * 0.02), 1.0, hr, 8, 5, esc=(0.108, 0.100, 0.108))


def modelo_publico():
    for variante in range(4):
        rng = np.random.RandomState(100 + variante)
        a = Atlas(); o = Obj(a)
        n = 0
        for k in range(GR_ASIENTOS):
            if rng.rand() < 0.85:
                persona(o, -GR_ANCHO / 2 + PASO / 2 + k * PASO, rng)
                n += 1
        o.escribir("Publico", "Fila%d" % (variante + 1))
    print("  Publico: 4 filas")


ANUNCIOS = [
    [("GRAFICA 3D", (22, 70, 170), (255, 255, 255)), ("TENIS", (14, 14, 18), (255, 214, 30)),
     ("SMASH", (200, 30, 36), (255, 255, 255)), ("AQUAFIT", (0, 140, 148), (255, 255, 255))],
    [("OPENGL", (72, 26, 116), (255, 255, 255)), ("RACKETPRO", (240, 128, 14), (14, 14, 18)),
     ("ACE", (14, 116, 46), (255, 255, 255)), ("TIEBREAK", (10, 26, 76), (255, 255, 255))],
    [("FI UNAM", (176, 140, 20), (14, 14, 18)), ("GOLPE", (220, 220, 224), (20, 20, 120)),
     ("DEUCE", (150, 24, 24), (255, 255, 255)), ("SAQUE", (20, 90, 160), (255, 255, 255))],
]

def modelo_vallas():
    for v, paneles in enumerate(ANUNCIOS):
        im = Image.new("RGB", (1024, 384), (255, 255, 255))
        d = ImageDraw.Draw(im)
        for i, (txt, bg, fg) in enumerate(paneles):
            d.rectangle([i * 256, 0, i * 256 + 255, 255], fill=bg)
            d.rectangle([i * 256, 0, i * 256 + 255, 10], fill=(255, 255, 255))
            d.rectangle([i * 256, 0, i * 256 + 3, 255], fill=(255, 255, 255))
            px = 56
            while d.textlength(txt, font=fuente(px)) > 230:
                px -= 2
            texto_centrado(d, (i * 256, 10, i * 256 + 256, 256), txt, px, fg)
        a = Atlas(1024, 256, im.crop((0, 0, 1024, 256))); o = Obj(a)
        o.caja((0, 0.5, 0), (4.0, 1.0, 0.25), (28, 66, 46),
               caras={"+z": a.rect_uv(0, 0, 1024, 256)})
        o.escribir("Vallas", "Valla%d" % (v + 1))
    print("  Vallas: 3 anuncios")


# =====================================================================================
#  MARCADOR (pantalla + estructura) y dígitos
# =====================================================================================
PANEL_W, PANEL_H, PX_M = 9.0, 4.5, 128

def modelo_marcador():
    W, H = int(PANEL_W * PX_M), int(PANEL_H * PX_M)
    im = Image.new("RGB", (W, H), (6, 14, 40))
    d = ImageDraw.Draw(im)
    d.rectangle([0, 0, W - 1, H - 1], outline=(60, 200, 255), width=8)
    texto_centrado(d, (0, 14, int(4.2 * PX_M), int(1.0 * PX_M)), "TENIS 3D", 96, (255, 214, 30))
    for cx, t in ((4.75, "S1"), (5.75, "S2"), (6.75, "S3"), (8.0, "PTS")):
        texto_centrado(d, (int((cx - 0.5) * PX_M), 14, int((cx + 0.5) * PX_M), int(1.0 * PX_M)), t, 56, (140, 170, 210))
    d.line([(20, int(1.1 * PX_M)), (W - 20, int(1.1 * PX_M))], fill=(60, 200, 255), width=5)
    nombres = ["ROJAS", "GARCIA"]
    for i, nom in enumerate(nombres):
        cy = [1.95, 3.65][i]
        texto_centrado(d, (int(0.8 * PX_M), int((cy - 0.7) * PX_M), int(4.3 * PX_M), int((cy + 0.7) * PX_M)), nom, 104, (255, 255, 255))
        for cx in (4.75, 5.75, 6.75):
            d.rectangle([int((cx - 0.42) * PX_M), int((cy - 0.62) * PX_M), int((cx + 0.42) * PX_M), int((cy + 0.62) * PX_M)], fill=(10, 24, 60), outline=(40, 80, 140), width=3)
        d.rectangle([int((8.0 - 0.62) * PX_M), int((cy - 0.62) * PX_M), int((8.0 + 0.62) * PX_M), int((cy + 0.62) * PX_M)], fill=(10, 24, 60), outline=(40, 80, 140), width=3)
    a = Atlas(W, H, im); o = Obj(a)
    acero = (60, 62, 70)
    for s in (-1, 1):
        o.cilindro((s * 5.2, -9.6, -0.5), (s * 5.2, 3.0, -0.5), 0.30, 0.26, acero, 10)
        o.caja((s * 5.2, -9.35, -0.5), (1.4, 0.5, 1.4), (110, 112, 120))
    o.caja((0, 0, -0.30), (9.8, 5.3, 0.5), acero)
    o.caja((0, 2.9, -0.35), (10.0, 0.30, 0.7), (230, 230, 235))
    pu = a.rect_uv(0, 0, W, H)
    o.caja((0, 0, 0.0), (PANEL_W, PANEL_H, 0.02), (6, 14, 40), caras={"+z": pu})
    print("  Marcador", o.escribir("Marcador", "Marcador"))


def imagenes_marcador():
    d_ = os.path.join(IMAGENES, "Marcador")
    os.makedirs(d_, exist_ok=True)
    oro = (255, 214, 30, 255)
    def pngtxt(nombre, txt, w, h, px, color=oro):
        im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        dd = ImageDraw.Draw(im)
        texto_centrado(dd, (0, 0, w, h), txt, px, color)
        im.save(os.path.join(d_, nombre + ".png"))
    for n in range(10):
        pngtxt("d%d" % n, str(n), 128, 192, 170)
    for nombre, txt in (("p0", "0"), ("p15", "15"), ("p30", "30"), ("p40", "40"), ("pAD", "AD")):
        pngtxt(nombre, txt, 256, 192, 170)
    im = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
    ImageDraw.Draw(im).ellipse([16, 16, 112, 112], fill=(214, 240, 60, 255), outline=(255, 255, 255, 255), width=6)
    im.save(os.path.join(d_, "saque.png"))
    print("  imágenes del marcador")


# =====================================================================================
#  TORRES DE LUZ, TÚNEL, ZONA DE DESCANSO
# =====================================================================================
def modelo_torre():
    a = Atlas(); o = Obj(a)
    gris = (150, 154, 162)
    o.caja((0, 0.3, 0), (2.4, 0.6, 2.4), (115, 118, 126))
    o.cilindro((0, 0, 0), (0, 31.0, 0), 0.55, 0.32, gris, 12)
    o.caja((0, 30.8, 0), (1.2, 0.4, 1.2), (60, 62, 70))
    print("  Torre", o.escribir("Torre", "Torre"))
    for estado, col in (("Encendidas", (255, 250, 225)), ("Apagadas", (95, 98, 105))):
        a = Atlas(); o = Obj(a)
        o.caja((0, 0, -0.35), (7.4, 5.4, 0.30), (45, 47, 54))
        for fila in range(4):
            for c in range(3):
                x, y = (c - 1) * 2.2, (fila - 1.5) * 1.15
                o.caja((x, y, -0.05), (1.7, 1.0, 0.30), (25, 26, 30))
                o.caja((x, y, 0.12), (1.45, 0.82, 0.06), col)
        o.escribir("Torre", "Lamparas" + estado)
    print("  Lámparas")


def modelo_tunel():
    sup = Image.new("RGB", (512, 128), (0, 0, 0))
    d = ImageDraw.Draw(sup)
    d.rectangle([0, 0, 511, 63], fill=(8, 70, 36)); texto_centrado(d, (0, 0, 512, 64), "VESTIDORES", 46, (255, 255, 255))
    d.rectangle([0, 64, 511, 127], fill=(14, 14, 18)); texto_centrado(d, (0, 64, 512, 128), "ENTRADA JUGADORES", 40, (255, 214, 30))
    a = Atlas(512, 128, sup); o = Obj(a)
    pared, piso, luz, puerta = (205, 205, 210), (86, 88, 100), (255, 248, 225), (84, 54, 32)
    hw, wt, h, L = 2.0, 0.30, 2.8, 10.5          # el túnel se extiende hacia +Z desde la boca (z=0)
    o.caja((0, 0.01, L / 2), (2 * hw, 0.02, L), piso)
    for s in (-1, 1):
        o.caja((s * (hw + wt / 2), h / 2, L / 2), (wt, h, L), pared)
    o.caja((0, h + 0.15, L / 2), (2 * hw + 2 * wt, 0.30, L), pared)
    o.caja((0, h / 2, L + 0.15), (2 * hw, h, 0.30), pared)
    o.caja((0, h + 0.70, -0.175), (2 * hw + 1.2, 0.80, 0.35), (20, 70, 44))                        # dintel
    o.caja((0, h + 0.70, -0.36), (4.4, 0.70, 0.02), (0, 0, 0), caras={"-z": a.rect_uv(0, 64, 512, 128)})
    for i in range(3):
        o.caja((0, h - 0.03, 3.2 + i * 3.2), (1.6, 0.05, 0.35), luz)
    for s in (-1, 1):
        o.caja((s * 1.05, 1.05, L - 0.02), (1.0, 2.1, 0.08), puerta)
    o.caja((0, 2.35, L - 0.04), (2.8, 0.45, 0.02), (0, 0, 0), caras={"-z": a.rect_uv(0, 0, 512, 64)})
    print("  Tunel", o.escribir("Tunel", "Tunel"))


def modelo_descanso(nombre, color_silla, color_sombrilla):
    a = Atlas(); o = Obj(a)
    metal, blanco = (170, 174, 180), (245, 245, 245)
    o.caja((0, 0.46, 0), (0.52, 0.06, 0.50), color_silla)                          # silla (frente +Z)
    o.caja((0, 0.80, -0.23), (0.52, 0.62, 0.05), color_silla, rot=rot_x(-8))
    for sx in (-1, 1):
        for sz in (-1, 1):
            o.cilindro((sx * 0.22, 0, sz * 0.21), (sx * 0.22, 0.45, sz * 0.21), 0.017, 0.017, metal, 6)
    o.cilindro((0.95, 0.52, 0), (0.95, 0.56, 0), 0.30, 0.30, blanco, 16)             # mesita
    o.cilindro((0.95, 0, 0), (0.95, 0.53, 0), 0.03, 0.03, metal, 8)
    o.cilindro((0.95, 0.56, 0.06), (0.95, 0.76, 0.06), 0.034, 0.034, (130, 200, 245), 10)   # botella
    o.cilindro((0.95, 0.76, 0.06), (0.95, 0.80, 0.06), 0.016, 0.016, blanco, 8)
    o.caja((0.70, 0.575, -0.10), (0.26, 0.03, 0.18), (30, 90, 190))                 # toalla sobre la mesa
    o.caja((-0.55, 0.21, 0.80), (0.62, 0.42, 0.40), (24, 80, 190))                  # hielera
    o.caja((-0.55, 0.45, 0.80), (0.64, 0.06, 0.42), blanco)
    o.caja((0, 0.98, -0.28), (0.46, 0.55, 0.02), (245, 245, 245), rot=rot_x(-8))    # toalla colgada
    o.cilindro((-0.85, 0, -0.1), (-0.85, 2.45, -0.1), 0.025, 0.025, (220, 220, 224), 8)         # sombrilla
    o.cilindro((-0.85, 2.05, -0.1), (-0.85, 2.50, -0.1), 1.55, 0.05, color_sombrilla, 16)
    o.cilindro((-0.85, 2.05, -0.1), (-0.85, 2.13, -0.1), 1.58, 1.55, blanco, 16)
    print("  Descanso", nombre, o.escribir("Descanso", nombre))


# =====================================================================================
#  CIELO (cubemap día / noche)
# =====================================================================================
def ruido3(p, rng_lat):
    """Ruido de valor 3D con interpolación trilineal (p: N x 3)."""
    n = rng_lat.shape[0]
    pi = np.floor(p).astype(int); f = p - pi
    f = f * f * (3 - 2 * f)
    res = 0
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                w = (f[:, 0] if dx else 1 - f[:, 0]) * (f[:, 1] if dy else 1 - f[:, 1]) * (f[:, 2] if dz else 1 - f[:, 2])
                res = res + w * rng_lat[(pi[:, 0] + dx) % n, (pi[:, 1] + dy) % n, (pi[:, 2] + dz) % n]
    return res

def generar_cielos(tam=1024):
    rng = np.random.RandomState(11)
    lat = rng.rand(24, 24, 24)
    caras = ["px", "nx", "py", "ny", "pz", "nz"]
    d_ = os.path.join(IMAGENES, "Skybox")
    os.makedirs(d_, exist_ok=True)
    s = (np.arange(tam) + 0.5) / tam * 2 - 1
    S, Tt = np.meshgrid(s, s)             # S: columna, Tt: fila (hacia abajo)
    for nombre in caras:
        if nombre == "px": D = np.stack([np.ones_like(S), -Tt, -S], -1)
        elif nombre == "nx": D = np.stack([-np.ones_like(S), -Tt, S], -1)
        elif nombre == "py": D = np.stack([S, np.ones_like(S), Tt], -1)
        elif nombre == "ny": D = np.stack([S, -np.ones_like(S), -Tt], -1)
        elif nombre == "pz": D = np.stack([S, -Tt, np.ones_like(S)], -1)
        else: D = np.stack([-S, -Tt, -np.ones_like(S)], -1)
        D = D / np.linalg.norm(D, axis=-1, keepdims=True)
        flat = D.reshape(-1, 3)
        y = D[..., 1]
        for modo in ("dia", "noche"):
            if modo == "dia":
                zen, hor, sue = np.array([52, 112, 214.]), np.array([186, 214, 244.]), np.array([150, 160, 160.])
                t = np.clip(y, 0, 1) ** 0.5
                col = hor[None, None, :] * (1 - t[..., None]) + zen[None, None, :] * t[..., None]
                col = np.where((y < 0)[..., None], sue[None, None, :] * (1 + y[..., None] * 0.6), col)
                nz = (ruido3(flat * 2.4 + 5, lat) * 0.55 + ruido3(flat * 5.0 + 9, lat) * 0.3 + ruido3(flat * 10.0, lat) * 0.15).reshape(tam, tam)
                nube = np.clip((nz - 0.52) / 0.18, 0, 1) * np.clip(y * 6 + 0.4, 0, 1)
                col = col * (1 - nube[..., None] * 0.85) + np.array([250, 250, 252.]) * (nube[..., None] * 0.85)
                sol = np.clip(np.sum(D * norm(v3(0.3, 0.8, 0.5)), -1), 0, 1) ** 400
                col = col + sol[..., None] * np.array([255, 240, 200.])
            else:
                zen, hor, sue = np.array([4, 8, 28.]), np.array([22, 34, 74.]), np.array([8, 10, 18.])
                t = np.clip(y, 0, 1) ** 0.5
                col = hor[None, None, :] * (1 - t[..., None]) + zen[None, None, :] * t[..., None]
                col = np.where((y < 0)[..., None], sue[None, None, :], col)
                q = np.floor(flat * 120).astype(int)
                h = ((q[:, 0] * 73856093) ^ (q[:, 1] * 19349663) ^ (q[:, 2] * 83492791)) & 0xFFFF
                est = ((h / 65535.0) > 0.9985).reshape(tam, tam) & (y > 0)
                col = col + est[..., None] * 150.0
                luna = np.clip(np.sum(D * norm(v3(-0.4, 0.55, 0.3)), -1), 0, 1) ** 900
                col = col + luna[..., None] * np.array([235, 240, 255.]) * 0.9
            Image.fromarray(np.clip(col, 0, 255).astype("uint8")).save(os.path.join(d_, "%s_%s.jpg" % (modo, nombre)), quality=92)
    print("  cielos día/noche")


if __name__ == "__main__":
    print("Generando modelos en", MODELOS)
    for t, n in (("arcilla", "CanchaArcilla"), ("pasto", "CanchaPasto"), ("cemento", "CanchaCemento")):
        modelo_cancha(t, n)
    modelo_piso(); modelo_red(); modelo_silla_juez()
    for nom, c in OUTFITS.items():
        partes_jugador(nom, c)
    partes_juez(); modelo_raqueta(); modelo_pelota(); modelo_sombra()
    modelo_grada("GradaAzul", [(30, 70, 200), (240, 240, 245), (14, 36, 110)])
    modelo_grada("GradaRoja", [(190, 36, 36), (240, 240, 245), (110, 14, 14)])
    modelo_publico(); modelo_vallas(); modelo_marcador(); imagenes_marcador()
    modelo_torre(); modelo_tunel()
    modelo_descanso("DescansoRojo", (200, 34, 34), (226, 34, 34))
    modelo_descanso("DescansoAzul", (30, 80, 200), (30, 80, 220))
    generar_cielos()
    print("Listo.")
