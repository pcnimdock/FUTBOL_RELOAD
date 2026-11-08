#!/usr/bin/env python3
"""
ligas_pcf.py - Elegir qué equipos juegan en Primera y Segunda (PC Fútbol 4.5)

QUÉ HACE
  Los ejecutables del juego llevan dentro una tabla con los IDs de todos los
  equipos nacionales (el número de EQ95xxxx.DBC):

      puestos  0-21  -> Primera División (22 equipos)
      puestos 22-41  -> Segunda División (20 equipos)
      puestos 42-..  -> resto (Segunda B), hasta un 0xFFFF final

  Es la composición de las ligas al empezar una partida NUEVA (al final de
  cada temporada el propio juego reordena la tabla en memoria con los
  ascensos y descensos). Esta herramienta lee y modifica esa tabla en:

      PCMANAG.DAT   (modo mánager)        tabla única terminada en 0xFFFF
      PROMANAG.DAT  (modo ProManager)     igual que PCMANAG
      PCFUTBOL.DAT  (base de datos)       22 IDs, 0xFFFF, 20 IDs, 0xFFFF

  El NÚMERO de equipos (22 y 20) NO se puede cambiar con esta herramienta:
  está fijado en el código del juego (11/10 partidos por jornada, 42/38
  jornadas, estructuras de 22 puestos). Ver la sección "Tamaño de las ligas"
  de la wiki/README de esta herramienta.

USO
  python3 ligas_pcf.py listar      CARPETA_JUEGO
  python3 ligas_pcf.py intercambiar CARPETA_JUEGO ID_A ID_B
  python3 ligas_pcf.py fijar       CARPETA_JUEGO --primera 1,2,...(22) [--segunda ...(20)]

  CARPETA_JUEGO es la que contiene PCMANAG.DAT y DBDAT. Los IDs son el número
  del fichero sin el prefijo 95: EQ950004.DBC -> 4.

  Antes de escribir se crea una copia NOMBRE.DAT.bak (sólo la primera vez).
  Trabaja siempre sobre una copia del juego.

  Verificado con PC Fútbol 4.5. Sólo afecta a partidas nuevas: una liga
  grabada (TACTICS/LIGA.n) conserva su propia composición.
"""
import argparse
import os
import shutil
import struct
import sys

FIN = 0xFFFF
N_PRIMERA = 22
N_SEGUNDA = 20

# Tabla de 48 caracteres especiales del cifrado de los DBC (bbdd_futbol.cpp)
CARACTERES = [
    0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB,
    0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5, 0xC9, 0xE6, 0xC6, 0xF4,
    0xF6, 0xF2, 0xFB, 0xF9, 0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5,
    0x00, 0x00, 0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA,
    0xBF, 0x81, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB]


def descodificar(b):
    out = []
    for v in b:
        v ^= 0x61
        if v & 0x80:
            v &= 0x7F
            v = CARACTERES[v] if v < 48 else 0x20
        out.append(chr(v))
    return ''.join(out)


def info_equipo(juego, ident):
    """(nombre, tiene_bbdd, tiene_camiseta) o None si no existe el DBC."""
    ruta = os.path.join(juego, 'DBDAT', 'EQ95%04d.DBC' % ident)
    if not os.path.isfile(ruta):
        return None
    with open(ruta, 'rb') as f:
        d = f.read(0x200)
    tiene_bbdd = len(d) > 0x2A and d[0x2A] == 0
    nombre = '?'
    if len(d) > 0x2D:
        n = d[0x2C]
        nombre = descodificar(d[0x2D:0x2D + n]).strip()
    camiseta = os.path.isfile(os.path.join(juego, 'DBDAT', 'CAMISAS', 'EQ95%04d.DFG' % ident))
    return nombre, tiene_bbdd, camiseta


# ---------------------------------------------------------------------------
# Localización de la tabla en cada ejecutable
# ---------------------------------------------------------------------------
# Los equipos nacionales son EQ950001..EQ950199; los extranjeros van de
# EQ950201 en adelante (y tienen su propia tabla, que NO hay que tocar).
MAX_ID_NACIONAL = 200


def _ids_validos(vals, distintos=True):
    if not all(1 <= v < MAX_ID_NACIONAL for v in vals):
        return False
    return len(set(vals)) == len(vals) if distintos else True


def buscar_tabla_manager(d):
    """Tabla única: N IDs nacionales seguidos de 0xFFFF (60 <= N <= 200).
    Los 42 primeros (Primera + Segunda) son distintos; en el resto el juego
    original repite algún ID (94 y 95 aparecen dos veces)."""
    encontrados = []
    i = 0
    lim = len(d) - 4 * 61
    while i < lim:
        # filtro rápido: los 42 primeros han de ser IDs válidos
        v0 = struct.unpack_from('<I', d, i)[0]
        if 1 <= v0 < MAX_ID_NACIONAL:
            vals = struct.unpack_from('<%dI' % 42, d, i)
            if _ids_validos(vals):
                j = i + 4 * 42
                lista = list(vals)
                while j + 4 <= len(d) and len(lista) <= 200:
                    v = struct.unpack_from('<I', d, j)[0]
                    if v == FIN:
                        break
                    lista.append(v)
                    j += 4
                if 60 <= len(lista) <= 200 and j + 4 <= len(d) and \
                        struct.unpack_from('<I', d, j)[0] == FIN and \
                        _ids_validos(lista, distintos=False):
                    encontrados.append((i, lista))
                    i = j
        i += 1
    return encontrados


def buscar_tabla_pcfutbol(d):
    """22 IDs, 0xFFFF, 20 IDs, 0xFFFF."""
    encontrados = []
    tam = 4 * (N_PRIMERA + 1 + N_SEGUNDA + 1)
    for i in range(0, len(d) - tam):
        if struct.unpack_from('<I', d, i + 4 * N_PRIMERA)[0] != FIN:
            continue
        if struct.unpack_from('<I', d, i + 4 * (N_PRIMERA + 1 + N_SEGUNDA))[0] != FIN:
            continue
        p = struct.unpack_from('<%dI' % N_PRIMERA, d, i)
        s = struct.unpack_from('<%dI' % N_SEGUNDA, d, i + 4 * (N_PRIMERA + 1))
        if _ids_validos(list(p) + list(s)):
            encontrados.append((i, list(p) + list(s)))
    return encontrados


EJECUTABLES = [
    ('PCMANAG.DAT', 'manager'),
    ('PROMANAG.DAT', 'manager'),
    ('PCFUTBOL.DAT', 'pcfutbol'),
]


class Tabla:
    def __init__(self, ruta, tipo, offset, ids):
        self.ruta, self.tipo, self.offset, self.ids = ruta, tipo, offset, ids

    @property
    def primera(self):
        return self.ids[:N_PRIMERA]

    @property
    def segunda(self):
        return self.ids[N_PRIMERA:N_PRIMERA + N_SEGUNDA]

    def bytes_nuevos(self, ids):
        if self.tipo == 'manager':
            return struct.pack('<%dI' % len(ids), *ids)
        p, s = ids[:N_PRIMERA], ids[N_PRIMERA:N_PRIMERA + N_SEGUNDA]
        return struct.pack('<%dI' % (N_PRIMERA + 1 + N_SEGUNDA),
                           *(list(p) + [FIN] + list(s)))


def leer_tablas(juego):
    tablas = []
    for nombre, tipo in EJECUTABLES:
        ruta = os.path.join(juego, nombre)
        if not os.path.isfile(ruta):
            continue
        with open(ruta, 'rb') as f:
            d = f.read()
        enc = buscar_tabla_manager(d) if tipo == 'manager' else buscar_tabla_pcfutbol(d)
        if len(enc) != 1:
            sys.exit('ERROR: en %s se esperaba 1 tabla de equipos y se han encontrado %d. '
                     'No se toca nada.' % (nombre, len(enc)))
        off, ids = enc[0]
        tablas.append(Tabla(ruta, tipo, off, ids))
    if not tablas:
        sys.exit('ERROR: no se encuentra PCMANAG.DAT / PROMANAG.DAT / PCFUTBOL.DAT en ' + juego)
    return tablas


def comprobar_coherencia(tablas):
    ref = tablas[0]
    ok = True
    for t in tablas[1:]:
        if t.primera != ref.primera or t.segunda != ref.segunda:
            print('AVISO: %s no tiene la misma Primera/Segunda que %s'
                  % (os.path.basename(t.ruta), os.path.basename(ref.ruta)))
            ok = False
        if t.tipo == 'manager' and ref.tipo == 'manager' and sorted(t.ids) != sorted(ref.ids):
            print('AVISO: %s y %s no contienen los mismos equipos'
                  % (os.path.basename(t.ruta), os.path.basename(ref.ruta)))
            ok = False
    return ok


def describir(juego, ident):
    inf = info_equipo(juego, ident)
    if inf is None:
        return '%4d  (sin EQ95%04d.DBC)' % (ident, ident)
    nombre, bbdd, cam = inf
    notas = []
    if not bbdd:
        notas.append('sin ficha completa')
    if not cam:
        notas.append('sin dibujo de camiseta')
    return '%4d  %-28s %s' % (ident, nombre, ('[' + ', '.join(notas) + ']') if notas else '')


# ---------------------------------------------------------------------------
# Órdenes
# ---------------------------------------------------------------------------
def cmd_listar(a):
    tablas = leer_tablas(a.juego)
    for t in tablas:
        print('%-13s tabla en el offset 0x%X (%d equipos)'
              % (os.path.basename(t.ruta), t.offset, len(t.ids)))
    comprobar_coherencia(tablas)
    ref = tablas[0]
    print('\nPRIMERA DIVISIÓN (%d)' % N_PRIMERA)
    for i in ref.primera:
        print('  ' + describir(a.juego, i))
    print('\nSEGUNDA DIVISIÓN (%d)' % N_SEGUNDA)
    for i in ref.segunda:
        print('  ' + describir(a.juego, i))
    resto = [t for t in ref.ids[N_PRIMERA + N_SEGUNDA:]]
    if resto and not a.corto:
        print('\nRESTO (Segunda B y otros, %d)' % len(resto))
        for i in resto:
            print('  ' + describir(a.juego, i))


def escribir(tablas, nuevas_por_tabla, juego):
    for t, ids in zip(tablas, nuevas_por_tabla):
        bak = t.ruta + '.bak'
        if not os.path.exists(bak):
            shutil.copy2(t.ruta, bak)
        nuevo = t.bytes_nuevos(ids)
        with open(t.ruta, 'r+b') as f:
            f.seek(t.offset)
            f.write(nuevo)
        # verificación
        with open(t.ruta, 'rb') as f:
            f.seek(t.offset)
            if f.read(len(nuevo)) != nuevo:
                sys.exit('ERROR: la verificación de %s ha fallado. Restaura %s' % (t.ruta, bak))
        print('Escrito %s (copia de seguridad: %s)' % (os.path.basename(t.ruta), os.path.basename(bak)))


def validar_division(juego, ids, etiqueta):
    errores = []
    for i in ids:
        inf = info_equipo(juego, i)
        if inf is None:
            errores.append('el equipo %d no tiene DBDAT/EQ95%04d.DBC' % (i, i))
        elif not inf[1]:
            print('AVISO: %d (%s) va a %s sin ficha completa de club; '
                  'puede dar problemas en el juego.' % (i, inf[0], etiqueta))
        elif not inf[2]:
            print('AVISO: %d (%s) va a %s y no tiene DBDAT/CAMISAS/EQ95%04d.DFG.'
                  % (i, inf[0], etiqueta, i))
    return errores


def cmd_intercambiar(a):
    tablas = leer_tablas(a.juego)
    comprobar_coherencia(tablas)
    nuevas = []
    for t in tablas:
        ids = list(t.ids)
        if a.id_a not in ids or a.id_b not in ids:
            if t.tipo == 'pcfutbol':
                # La tabla de PCFUTBOL sólo tiene Primera y Segunda: si uno de los
                # dos viene del resto, se sustituye directamente.
                if a.id_a in ids:
                    ids[ids.index(a.id_a)] = a.id_b
                elif a.id_b in ids:
                    ids[ids.index(a.id_b)] = a.id_a
                nuevas.append(ids)
                continue
            sys.exit('ERROR: %d o %d no está en la tabla de %s. Los IDs válidos son los de '
                     '"listar".' % (a.id_a, a.id_b, os.path.basename(t.ruta)))
        ia, ib = ids.index(a.id_a), ids.index(a.id_b)
        ids[ia], ids[ib] = ids[ib], ids[ia]
        nuevas.append(ids)
    ref = nuevas[0]
    errores = validar_division(a.juego, ref[:N_PRIMERA], 'Primera') + \
        validar_division(a.juego, ref[N_PRIMERA:N_PRIMERA + N_SEGUNDA], 'Segunda')
    if errores:
        sys.exit('ERROR:\n  ' + '\n  '.join(errores))
    escribir(tablas, nuevas, a.juego)
    print('Hecho: %s <-> %s' % (describir(a.juego, a.id_a).strip(), describir(a.juego, a.id_b).strip()))


def cmd_fijar(a):
    tablas = leer_tablas(a.juego)
    comprobar_coherencia(tablas)
    ref = tablas[0]
    primera = [int(x) for x in a.primera.split(',')] if a.primera else list(ref.primera)
    segunda = [int(x) for x in a.segunda.split(',')] if a.segunda else list(ref.segunda)
    if len(primera) != N_PRIMERA or len(segunda) != N_SEGUNDA:
        sys.exit('ERROR: Primera necesita exactamente %d equipos y Segunda %d '
                 '(el número está fijado en el código del juego).' % (N_PRIMERA, N_SEGUNDA))
    elegidos = primera + segunda
    if len(set(elegidos)) != len(elegidos):
        sys.exit('ERROR: hay equipos repetidos.')
    errores = validar_division(a.juego, primera, 'Primera') + validar_division(a.juego, segunda, 'Segunda')
    if errores:
        sys.exit('ERROR:\n  ' + '\n  '.join(errores))
    nuevas = []
    for t in tablas:
        if t.tipo == 'manager':
            faltan = [i for i in elegidos if i not in t.ids]
            if faltan:
                sys.exit('ERROR: %s no contiene los equipos %s; sólo se pueden usar los IDs '
                         'que aparecen en "listar".' % (os.path.basename(t.ruta), faltan))
            # el resto conserva su orden; los desplazados de Primera/Segunda
            # pasan al principio del resto
            resto = [i for i in t.ids if i not in elegidos]
            nuevas.append(elegidos + resto)
        else:
            nuevas.append(elegidos)
    escribir(tablas, nuevas, a.juego)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest='orden', required=True)
    s = sub.add_parser('listar', help='muestra Primera, Segunda y el resto')
    s.add_argument('juego')
    s.add_argument('--corto', action='store_true', help='no listar el resto (Segunda B)')
    s.set_defaults(f=cmd_listar)
    s = sub.add_parser('intercambiar', help='intercambia la posición de dos equipos')
    s.add_argument('juego')
    s.add_argument('id_a', type=int)
    s.add_argument('id_b', type=int)
    s.set_defaults(f=cmd_intercambiar)
    s = sub.add_parser('fijar', help='define Primera (22) y/o Segunda (20) completas')
    s.add_argument('juego')
    s.add_argument('--primera', help='22 IDs separados por comas')
    s.add_argument('--segunda', help='20 IDs separados por comas')
    s.set_defaults(f=cmd_fijar)
    a = p.parse_args()
    a.f(a)


if __name__ == '__main__':
    main()
