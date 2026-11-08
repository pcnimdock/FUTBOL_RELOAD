# Equipaciones del partido, botas y composición de las ligas (PC Fútbol 4.5)

Ingeniería inversa hecha en noviembre de 2026 sobre PC Fútbol 4.5. Todo lo
marcado como **verificado** se ha comprobado con los ficheros del juego; lo
marcado como **pendiente** no se ha probado dentro del juego (no había DOSBox).

Código relacionado:

| Fichero | Qué hace |
|---|---|
| `FUTBOL_RELOAD/pcfutbol_equipacion.{h,cpp}` | Zonas del jugador, relleno con degradado, guardado seguro |
| `FUTBOL_RELOAD/dialog_equipacion.{h,cpp}` | Editor con vista previa del jugador en el partido |
| `FUTBOL_RELOAD/tools/ligas_pcf.py` | Elegir los equipos de Primera y Segunda |
| `FUTBOL_RELOAD/tools/le_a_binario.py` | Convierte un ejecutable LE (DOS/4GW) en imágenes planas para desensamblar |

---

## 1. `PALETAS/EQUIPOS.DAT` es una tabla de recoloreado (verificado)

Cada bloque de 190 bytes no es sólo el icono de la camiseta: es una **tabla de
consulta (LUT) para los sprites de jugador** de `GRAFICOS/*.EMP`. Al dibujar un
jugador, el píxel del sprite con índice `i` se pinta con el color `bloque[i]`
de `GRAFICOS/PALETA.BBM`.

Eso explica la "cabecera" y el "cierre" que antes se creían fijos:

| Bytes del bloque | Índices del sprite | Contenido |
|---|---|---|
| `0` | 0 | transparente (no se usa) |
| `1..6` = `"QRSTUV"` | 1–6 | `0x51..0x56`: rampa de **piel** |
| `7` = `">"` | 7 | `0x3E`: sombra del jugador sobre el césped |
| `8..175` | 8–175 | la "textura" de 168 bytes: posición `p` = índice `p+8` |
| `176..189` | 176–189 | índices altos casi sin uso; 185 y 187–189 **varían por equipo** |

Comprobación: pintando los sprites `CORRER/CHUT/PASE.EMP` con el bloque del
Barça salen azulgranas; con el del Zaragoza, blanco con pantalón azul; con la
2ª del Madrid, morado.

### Zonas del jugador (posiciones de la textura)

Obtenidas cruzando la altura media de cada índice dentro del sprite con la
familia de color en los 470 kits, y verificadas pintando cada zona de un
color en varios sprites:

| Zona | Posiciones | Observaciones |
|---|---|---|
| **Botas** | 7, 22, 23 | `0x60/0x70/0x73` en los 470 kits: botas negras |
| **Pelo** | 14 | `0x60` en los 470 kits (probablemente el juego lo cambia por jugador) |
| **Medias** | 18, 19, 20 | |
| **Pantalón** | 9–13, 15, 63–71 | |
| Detalles fijos | 8, 17, 21 | negro `0x60` |
| **Camiseta** | todas las demás | las rayas viven en las filas 3–6 |

- La antigua "sombra" protegida (posición 7) es el **color de las botas**. El
  icono de 20×20 la usa además como contorno, así que cambiar las botas
  también cambia el contorno del icono.
- **Las botas son por equipo**, no por jugador: en el DBC no hay ningún campo
  de botas por jugador.
- Las paletas `PALETAS/PALETA.BBM` y `GRAFICOS/PALETA.BBM` sólo difieren en
  `0x99..0x9D`: azul marino en el icono, **morado en el partido**.

### Errores corregidos en el editor

- "Aplicar patrón" rellenaba las 168 posiciones: pantalón, medias, botas y
  pelo acababan del color de la camiseta. Ahora sólo toca la camiseta.
- Al guardar, `bloqueDesdeTextura()` sustituía los 14 bytes finales por una
  constante inventada y se perdían los bytes 185–189 de cada equipo. Ahora
  se reescriben sólo los 168 bytes de textura (`escribirTexturaEquipacion`).

---

## 2. Dibujos de kit `DBDAT/CAMISAS/EQ95xxxx.DFG` (parcial)

Son imágenes de la "familia A" (cabecera `01 47 00 25 00 04 00 00`: 71×37,
4 planos), con las medias, la camiseta y el pantalón. El exportador de imágenes
existente ya las trata.

**Pendiente:** su paleta de 16 colores no es fija. Los índices 0, 5, 6 y 12–15
son siempre negro, azul, rojo y grises/blanco, pero 1–4 y 7–11 cambian por
equipo (el 1 es morado en el Valladolid y negro en el Mallorca; el 8, verde en
el Betis). El juego carga esos colores en tiempo de ejecución; falta averiguar
de dónde.

---

## 3. Composición de las ligas (verificado en los ficheros)

Los tres ejecutables llevan una tabla de IDs de equipo (el número de
`EQ95xxxx.DBC`) en u32 little-endian:

| Ejecutable | Offset en el fichero (4.5 original) | Formato |
|---|---|---|
| `PCMANAG.DAT` | `0xA80DC` (memoria `0xA3B24`) | 22 Primera + 20 Segunda + resto, fin `0xFFFF` |
| `PROMANAG.DAT` | `0xAAABC` | igual |
| `PCFUTBOL.DAT` | `0x58344` | 22 IDs, `0xFFFF`, 20 IDs, `0xFFFF` |

- El código accede a ella como `tabla[división*22 + puesto]` y aborta si el
  índice pasa de 41.
- Es la composición de una partida **nueva**: al final de cada temporada el
  juego reescribe la tabla en memoria con ascensos y descensos.
- `PCMANAG.DAT`: emparejamientos de la primera jornada en `0xA3C14`
  (memoria): 11 pares de Primera y 10 de Segunda, índices de puesto. El resto
  del calendario se genera rotando.

### Herramienta `ligas_pcf.py`

```
python3 FUTBOL_RELOAD/tools/ligas_pcf.py listar       JUEGO [--corto]
python3 FUTBOL_RELOAD/tools/ligas_pcf.py intercambiar JUEGO 13 23
python3 FUTBOL_RELOAD/tools/ligas_pcf.py fijar        JUEGO --primera id,...(22) --segunda id,...(20)
```

Localiza la tabla por su estructura (no por offsets fijos), modifica los tres
ejecutables a la vez, hace una copia `.bak` la primera vez y verifica lo
escrito. Avisa si un equipo sin ficha completa o sin dibujo de camiseta va a
Primera o Segunda. Probado sobre una copia: sólo cambian bytes dentro de la
tabla. **Pendiente** probar una partida nueva en el juego.

Como no se añaden ni quitan `.DBC`, la numeración de `EQUIPOS.DAT` no se ve
afectada.

---

## 4. Tamaño de las ligas: por qué no se cambia todavía

El número de equipos está fijado en el código de `PCMANAG.DAT` (y lo mismo en
`PROMANAG.DAT`). Variables en memoria:

| Dirección | Significado |
|---|---|
| `0xBEFF8` | jornada actual |
| `0xBF004` | partidos por jornada: `10 + (división == 0)` → 11 / 10 |
| `0xBF008` | jornadas de la división: 42 (`0x2A`) / 38 (`0x26`); en un sitio 45 (`0x2D`) |

Sitios en `PCMANAG.DAT` (direcciones de memoria tras `le_a_binario.py`):

- **Partidos por jornada** (`sete al; movzx; add eax,0xa`): `10389, 116f3,
  3836a, 3a3d9, 3a53a, 3a960, 3c988, 3d457, 3d6f6, 48d12`.
- **Jornadas** (`mov ... ,0x2a/0x2d/0x26`): `10397, 1039e, 11701, 11708,
  3837c, 38383, 39e56, 39e5d, 3a3e8, 3a3ef, 3a549, 3a550, 3a69b, 3a973,
  48841, 4884d`.
- **Comparaciones con la jornada actual**: `3498a, 3499c, 34a4d, 34a71,
  3594e, 35960, 35a11, 35a35, 369d0, 369e2, 36ac4, 36ba0, 3c116, 3c14d,
  3c18f, 3c1c9, 3c1d2`.
- **Estructuras de tamaño fijo**: 76 `imul ...,0x16` (22 puestos por
  división); calendario de `0x1734` bytes por división (45 jornadas × `0x84`,
  y `0x84` = 11 partidos × 12 bytes).

Conclusiones:

- **Más de 22 equipos en Primera:** desbordaría estructuras de tamaño fijo.
  No es realista.
- **Segunda a 22 equipos:** cabe en las estructuras, pero hay que cambiar el
  límite de 41, los 38/42 y los 10/11, y la tabla de emparejamientos.
- **Primera a 20 equipos:** hay que tocar todos los sitios de arriba, dar una
  tabla de emparejamientos válida para 20 y revisar cuántos descienden.
- **Riesgo principal:** la Copa y las competiciones europeas se juegan entre
  jornadas de liga concretas ("la Copa comienza después de la jornada 8").
  Con menos jornadas, alguna ronda podría no jugarse nunca. Hay que probar
  **temporadas completas** en DOSBox antes de dar un parche por bueno.

### Cómo reproducir el desensamblado

```
python3 FUTBOL_RELOAD/tools/le_a_binario.py PCMANAG.DAT salida/
objdump -D -b binary -m i386 -M intel --adjust-vma=0x10000 salida/obj1.bin > code.asm
```

El objeto 1 (código y cadenas) se carga en `0x10000` y el 2 (datos) en
`0xA0000`. En esta máquina virtual hay que trabajar con poca memoria: un
proceso pesado cada vez (`make -j1`).
