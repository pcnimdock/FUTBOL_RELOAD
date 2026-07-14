#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
traductor_pcf.py — Prototipo del módulo de traducción para PC Fútbol 4.5
=========================================================================

Herramienta de línea de comandos que cubre las tres fuentes de texto
traducible del juego (ver MODULO_TRADUCTOR.md para la investigación):

  1. TEXTOS DE INTERFAZ (*.GRC)  — cifrados con XOR posicional.
       grc-extract  RESPCFUT.GRC textos.csv     -> CSV editable
       grc-inject   RESPCFUT.GRC textos.csv NUEVO.GRC

  2. IMÁGENES CON TEXTO INCRUSTADO (*.DAT en formato IFF/ILBM)
       ilbm-export  PCFTEX4.DAT  imagen.png     -> PNG editable (paleta 16c)
       ilbm-import  PCFTEX4.DAT  imagen.png NUEVO.DAT
     (mismo formato en COMMWIN.DAT, LGR.DAT, INFODAT.DAT; el resto de
      imágenes .EMP/.PAN/.DFG ya las cubre PCFutbolImagenes en el editor)

  3. FUENTE TIPOGRÁFICA (INFOFUT.FNT)
       fnt-dump     INFOFUT.FNT  fuente.png     -> atlas de glifos 1bpp
     Necesario para comprobar que la fuente contiene los caracteres del
     idioma destino (acentos, ñ/ç, etc.).

  4. TABLA DE CARACTERES DE LOS .DBC (nombres/comentarios de la BBDD)
       charset                                  -> imprime la tabla XOR 0x61

Sin dependencias externas salvo Pillow para PNG (pip install pillow).
"""

import argparse
import csv
import struct
import sys

# ---------------------------------------------------------------------------
# 1) *.GRC — "GFX Resource file" (Dinamic Multimedia, Carlos Abril)
# ---------------------------------------------------------------------------
# Cabecera ASCII terminada en 0x1A. Los objetos de interfaz con texto van
# precedidos de 12 bytes (6 x u16 LE): tipo, param1, param2, x, y, longitud.
# El texto va CIFRADO:  claro[i] = cifrado[i] XOR ((0xC0 + longitud + i) & 0xFF)
# Para reinyectar sin romper offsets, el texto nuevo se recorta o rellena
# con espacios hasta la longitud original.

def _grc_decrypt(raw: bytes) -> bytes:
    n = len(raw)
    return bytes(b ^ ((0xC0 + n + i) & 0xFF) for i, b in enumerate(raw))


def _grc_encrypt(raw: bytes) -> bytes:
    return _grc_decrypt(raw)  # XOR es involutivo


# Caracteres válidos en un texto de interfaz del juego (CP850 parcial)
_GRC_OK = set(range(0x20, 0x7F)) | {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5,
                                    0x81, 0x82, 0x8A, 0x8D, 0x95, 0x97,
                                    0xD1, 0xF1}


def grc_scan(data: bytes):
    """Devuelve [(offset_texto, longitud, texto_claro), ...]."""
    out = []
    i = data.index(b"\x1a") + 1
    end = len(data)
    while i < end - 12:
        ln = struct.unpack_from("<H", data, i + 10)[0]
        if 0 < ln < 80 and i + 12 + ln <= end:
            dec = _grc_decrypt(data[i + 12:i + 12 + ln])
            if all(c in _GRC_OK for c in dec):
                out.append((i + 12, ln, dec.decode("cp850")))
                i += 12 + ln
                continue
        i += 1
    return out


def cmd_grc_extract(args):
    data = open(args.grc, "rb").read()
    filas = grc_scan(data)
    with open(args.csv, "w", newline="", encoding="utf-8-sig") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(["offset", "longitud", "texto_original", "traduccion"])
        for off, ln, txt in filas:
            w.writerow([hex(off), ln, txt, ""])
    print(f"{len(filas)} textos extraídos a {args.csv}")


def cmd_grc_inject(args):
    data = bytearray(open(args.grc, "rb").read())
    cambios = 0
    with open(args.csv, encoding="utf-8-sig") as f:
        for fila in csv.DictReader(f, delimiter=";"):
            nuevo = (fila.get("traduccion") or "").strip()
            if not nuevo:
                continue
            off = int(fila["offset"], 16)
            ln = int(fila["longitud"])
            enc = nuevo.encode("cp850", errors="replace")[:ln].ljust(ln, b" ")
            data[off:off + ln] = _grc_encrypt(enc)
            cambios += 1
    open(args.salida, "wb").write(data)
    print(f"{cambios} textos reinyectados en {args.salida} "
          f"(tamaño idéntico: {len(data)} bytes)")


# ---------------------------------------------------------------------------
# 2) *.DAT — imágenes IFF/ILBM (Deluxe Paint) con texto "pintado"
# ---------------------------------------------------------------------------

def _ilbm_chunks(data: bytes):
    pos = 12
    while pos < len(data) - 8:
        cid = data[pos:pos + 4]
        ln = struct.unpack(">I", data[pos + 4:pos + 8])[0]
        yield cid, pos, data[pos + 8:pos + 8 + ln]
        pos += 8 + ln + (ln & 1)


def _unpack_byterun1(body: bytes) -> bytes:
    out = bytearray()
    i = 0
    while i < len(body):
        n = body[i]; i += 1
        if n < 128:
            out += body[i:i + n + 1]; i += n + 1
        elif n > 128:
            out += bytes([body[i]]) * (257 - n); i += 1
    return bytes(out)


def _pack_byterun1(row: bytes) -> bytes:
    """Compresión ByteRun1 fila a fila (suficiente y compatible)."""
    out = bytearray()
    i, n = 0, len(row)
    while i < n:
        run = 1
        while i + run < n and run < 128 and row[i + run] == row[i]:
            run += 1
        if run >= 3:
            out += bytes([257 - run, row[i]]); i += run
        else:
            j = i + 1
            while (j < n and j - i < 128 and
                   not (j + 2 < n and row[j] == row[j + 1] == row[j + 2])):
                j += 1
            out += bytes([j - i - 1]) + row[i:j]; i = j
    return bytes(out)


def ilbm_decode(data: bytes):
    bmhd = cmap = body = None
    for cid, _, chunk in _ilbm_chunks(data):
        if cid == b"BMHD": bmhd = chunk
        elif cid == b"CMAP": cmap = chunk
        elif cid == b"BODY": body = chunk
    w, h, _, _, planes, _, comp = struct.unpack(">HHHHBBB", bmhd[:11])
    if comp == 1:
        body = _unpack_byterun1(body)
    rowbytes = ((w + 15) // 16) * 2
    pix = bytearray(w * h)
    for y in range(h):
        base = y * rowbytes * planes
        for x in range(w):
            v = 0
            for p in range(planes):
                if body[base + p * rowbytes + x // 8] >> (7 - x % 8) & 1:
                    v |= 1 << p
            pix[y * w + x] = v
    return w, h, planes, cmap, bytes(pix)


def cmd_ilbm_export(args):
    from PIL import Image
    w, h, planes, cmap, pix = ilbm_decode(open(args.dat, "rb").read())
    img = Image.frombytes("P", (w, h), pix)
    img.putpalette(list(cmap) + [0] * (768 - len(cmap)))
    img.save(args.png)
    print(f"{args.dat}: {w}x{h}, {planes} planos, "
          f"{len(cmap)//3} colores -> {args.png}")


def cmd_ilbm_import(args):
    from PIL import Image
    original = open(args.dat, "rb").read()
    w, h, planes, cmap, _ = ilbm_decode(original)
    img = Image.open(args.png)
    if img.size != (w, h):
        sys.exit(f"ERROR: la imagen debe ser {w}x{h}, es {img.size}")
    if img.mode != "P":
        # Cuantizar a la paleta original del juego (¡no inventar colores!)
        pal_img = Image.new("P", (1, 1))
        pal_img.putpalette(list(cmap) + [0] * (768 - len(cmap)))
        img = img.convert("RGB").quantize(palette=pal_img)
    pix = img.tobytes()

    rowbytes = ((w + 15) // 16) * 2
    filas = bytearray()
    for y in range(h):
        planos = bytearray(rowbytes * planes)
        for x in range(w):
            v = pix[y * w + x]
            for p in range(planes):
                if v >> p & 1:
                    planos[p * rowbytes + x // 8] |= 0x80 >> (x % 8)
        filas += _pack_byterun1(bytes(planos))
    nuevo_body = bytes(filas)

    out = bytearray()
    for cid, pos, chunk in _ilbm_chunks(original):
        if cid == b"BODY":
            out = bytearray(original[:pos])
            out += b"BODY" + struct.pack(">I", len(nuevo_body)) + nuevo_body
            if len(nuevo_body) & 1:
                out += b"\x00"
            resto = pos + 8 + len(chunk) + (len(chunk) & 1)
            out += original[resto:]
            break
    struct.pack_into(">I", out, 4, len(out) - 8)  # tamaño FORM
    open(args.salida, "wb").write(out)
    print(f"Imagen reinyectada en {args.salida} ({len(out)} bytes)")


# ---------------------------------------------------------------------------
# 3) INFOFUT.FNT — fuente de mapa de bits 1bpp
# ---------------------------------------------------------------------------
# El .FNT contiene VARIAS fuentes concatenadas. Cada bloque empieza con
# u32 LE = tamaño del bloque, seguido de una cabecera de métricas y, en
# el offset 0x1D0 del bloque, los glifos consecutivos. Registro de glifo
# (12 bytes, 6 x u16 LE):
#   código, reservado(0), alto_de_línea, ancho_px, nº_de_filas, avance
# Bitmap: nº_de_filas * ceil(ancho/8) bytes, 1 bit por píxel, MSB primero.
# El volcado a PNG permite auditar qué caracteres existen antes de traducir
# a idiomas con otros diacríticos. INFOFUT.FNT trae Latin-1 casi completo
# (á é í ó ú ñ ç ü ¿ ¡ ...), así que la mayoría de idiomas europeos
# occidentales no necesitan tocar la fuente.

def _fnt_parse_block(d, base):
    glifos = []
    i = base + 0x1D0
    while i + 12 <= len(d):
        code, res, _alto, ancho, filas, _av = struct.unpack_from("<6H", d, i)
        if not (0x20 <= code <= 0x1FF) or res != 0:
            break
        bpr = max(1, (ancho + 7) // 8)
        i += 12
        glifos.append((code, ancho, filas, bpr, d[i:i + filas * bpr]))
        i += filas * bpr
    return glifos


def cmd_fnt_dump(args):
    from PIL import Image
    d = open(args.fnt, "rb").read()
    glifos = []
    base = 0
    fuente = 0
    while base + 4 < len(d):
        tam = struct.unpack_from("<I", d, base)[0]
        if tam < 0x1D0 or base + tam > len(d) + 16:
            break
        bloque = _fnt_parse_block(d, base)
        if not bloque:
            break
        fuente += 1
        print(f"  fuente #{fuente}: {len(bloque)} glifos "
              f"(bloque de {tam} bytes en {hex(base)})")
        glifos += bloque
        base += tam + 4
    if not glifos:
        sys.exit("No se han reconocido glifos: revisar offsets de cabecera.")

    celda_w = max(g[1] for g in glifos) + 2
    celda_h = max(g[2] for g in glifos) + 10
    cols = 16
    filas = (len(glifos) + cols - 1) // cols
    img = Image.new("L", (cols * celda_w, filas * celda_h), 255)
    px = img.load()
    for n, (code, w, h, bpr, bits) in enumerate(glifos):
        ox, oy = (n % cols) * celda_w, (n // cols) * celda_h
        for y in range(h):
            for x in range(w):
                if y * bpr + x // 8 < len(bits) and \
                   bits[y * bpr + x // 8] >> (7 - x % 8) & 1:
                    px[ox + x + 1, oy + y + 1] = 0
    img.save(args.png)
    codes = "".join(chr(g[0]) for g in glifos)
    print(f"{len(glifos)} glifos -> {args.png}")
    print("Caracteres presentes:", codes)


# ---------------------------------------------------------------------------
# 4) Tabla de caracteres de los .DBC
# ---------------------------------------------------------------------------

_DBC_TABLA = [0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB,
              0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5, 0xC9, 0xE6, 0xC6, 0xF4,
              0xF6, 0xF2, 0xFB, 0xF9, 0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5,
              0x00, 0x00, 0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA,
              0xBF, 0x81, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB]


def cmd_charset(_args):
    print("Codificación .DBC:  byte_archivo = byte_claro XOR 0x61")
    print("Si (byte XOR 0x61) tiene el bit 7, es un índice en esta tabla")
    print("de caracteres extendidos (Latin-1):\n")
    for i, c in enumerate(_DBC_TABLA):
        ch = chr(c) if c else "·"
        print(f"  [{i:2d}] 0x{c:02X} {ch}", end="\n" if i % 6 == 5 else "   ")
    print()


# ---------------------------------------------------------------------------

def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("grc-extract"); s.add_argument("grc"); s.add_argument("csv")
    s.set_defaults(fn=cmd_grc_extract)
    s = sub.add_parser("grc-inject"); s.add_argument("grc"); s.add_argument("csv")
    s.add_argument("salida"); s.set_defaults(fn=cmd_grc_inject)
    s = sub.add_parser("ilbm-export"); s.add_argument("dat"); s.add_argument("png")
    s.set_defaults(fn=cmd_ilbm_export)
    s = sub.add_parser("ilbm-import"); s.add_argument("dat"); s.add_argument("png")
    s.add_argument("salida"); s.set_defaults(fn=cmd_ilbm_import)
    s = sub.add_parser("fnt-dump"); s.add_argument("fnt"); s.add_argument("png")
    s.set_defaults(fn=cmd_fnt_dump)
    s = sub.add_parser("charset"); s.set_defaults(fn=cmd_charset)

    args = p.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
