# Equipaciones del partido, botas y composición de las ligas (PC Fútbol 4.5)

Ingeniería inversa hecha en noviembre de 2026 sobre PC Fútbol 4.5. Todo lo
marcado como **verificado** se ha comprobado con los ficheros del juego; lo
marcado como **pendiente** no se ha probado dentro del juego (no había DOSBox).

Código relacionado:

| Fichero | Qué hace |
|---|---|
| `FUTBOL_RELOAD/pcfutbol_equipacion.{h,cpp}` | Zonas del jugador, relleno con degradado, guardado seguro |
| `FUTBOL_RELOAD/dialog_equipacion.{h,cpp}` | Editor con vista previa del jugador en el partido |
| `FUTBOL_RELOAD/pcfutbol_ligas.{h,cpp}` | Localiza y modifica las tablas de composición de ligas |
| `FUTBOL_RELOAD/dialog_ligas.{h,cpp}` | Ventana «Composición de ligas» del editor |
| `FUTBOL_RELOAD/tools/ligas_pcf.py` | Lo mismo por línea de comandos |
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

### Choques de color: cuándo se usa la 2ª equipación (verificado en el juego)

Los bytes **185** (índice de paleta) y **187–189** (su RGB, 0–255) son el
**color representativo** del kit. `PCMANAG.DAT` (función en `0x74F3C` de la
imagen cargada con base `0x10000`) y `PROMANAG.DAT` cargan la 1ª equipación
de los dos equipos, pasan ese RGB a HSL y miden la distancia de **tono**:

- menos de 60° → choque: el visitante (salvo un indicador interno que hace
  cambiar al otro equipo) carga su **2ª equipación** (kit 2*i*+1);
- un color sin tono (blanco, gris, negro: H = 0 y S = 0) no choca con uno
  que sí lo tiene, pero **dos colores sin tono siempre chocan** (blanco
  contra negro incluido); la luminosidad no cuenta;
- **no se vuelve a comprobar** con la 2ª: si también choca, se juega así;
- **no existen terceras equipaciones** (2 kits por equipo en `EQUIPOS.DAT`).

Lo que decide es ese color, no la camiseta pintada. Comprobado en DOSBox:
Valencia–Madrid (los dos blancos) → el Madrid sale con la 2ª, morada; con el
color representativo del Madrid cambiado a rojo y la camiseta igual, los dos
juegan de blanco.

El kit de cada equipo se busca en una tabla de 235 números de equipo dentro
del ejecutable (`0xAE980` en `PCMANAG.DAT`, `0xA318C` en `PROMANAG.DAT`, en la
imagen cargada); en la 4.5 coincide con los `EQ*.DBC` ordenados.

En el editor, apartado **«Color para detectar choques»**: muestra el color,
avisa si ya no se parece a la camiseta, lo calcula desde la camiseta o lo
toma de la paleta, y «Aplicar patrón» lo recalcula solo. **«Comprobar
choques…»** aplica la regla del juego contra todos los equipos y marca los
partidos en los que la 2ª equipación tampoco evita el choque.

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

Los ejecutables llevan **varias copias** de la composición, como tablas de
IDs de equipo (el número de `EQ95xxxx.DBC`) en u32 little-endian:

| Ejecutable | Fichero (4.5) | Memoria | Formato |
|---|---|---|---|
| `PCMANAG.DAT` | `0xA80DC` | `0xA22DC` | lista larga: 22 Primera + 20 Segunda + 57 Segunda B, fin `0xFFFF` |
| `PCMANAG.DAT` | `0xA9924` | `0xA3B24` | tabla de 42 (Primera + Segunda) |
| `PROMANAG.DAT` | `0xAAABC` | `0x964BC` | lista larga |
| `PROMANAG.DAT` | `0xAC7A0` | `0x981A0` | tabla de 42 (aquí Albacete y Valladolid van al revés) |
| `PCFUTBOL.DAT` | `0x58344` | `0x54344` | 22 IDs, `0xFFFF`, 20 IDs, `0xFFFF` |

- La lista larga está ordenada por la clasificación de la temporada anterior
  (Real Madrid, Deportivo, Betis...). En Segunda B, Ávila y Manchego aparecen
  dos veces: por eso un intercambio debe sustituir **todas** las apariciones.
- La tabla de 42 se recorre con bucles `cmp ...,0xA8` (42×4) y se rellena
  desde el fichero al cargar una liga grabada.
- No se sabe cuál de las copias manda al empezar partida, así que las
  herramientas modifican **todas**. (La primera versión de `ligas_pcf.py` sólo
  tocaba la lista larga; corregido.)

- El código accede a ella como `tabla[división*22 + puesto]` y aborta si el
  índice pasa de 41.
- Es la composición de una partida **nueva**: al final de cada temporada el
  juego reescribe la tabla en memoria con ascensos y descensos.
- `PCMANAG.DAT`: emparejamientos de la primera jornada en `0xA3C14`
  (memoria): 11 pares de Primera y 10 de Segunda, índices de puesto. El resto
  del calendario se genera rotando.

### En el editor: botón «Composición de ligas»

Tres listas (Primera, Segunda, Segunda B). Se elige un equipo en una y otro
en otra y se pulsa «Intercambiar». Al guardar se modifican los tres
ejecutables con copia `.bak`. Las tablas se localizan por contenido, así que
funciona también con PC Fútbol 4.0 (verificado en lectura: allí la lista
larga tiene 96 equipos).

El editor y `ligas_pcf.py` se han validado uno contra otro: el mismo
intercambio (Ávila ↔ Albacete) produce ficheros idénticos byte a byte.

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
