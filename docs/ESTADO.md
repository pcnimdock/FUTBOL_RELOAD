# Estado del proyecto — LEER PRIMERO

Documento para **retomar el trabajo** (por ejemplo, después de que se cuelgue
la máquina virtual). Resume qué se quiere conseguir, dónde está todo, qué se
ha descubierto, qué está hecho y probado, y qué falta.

El detalle técnico de cada hallazgo está en
[equipaciones-y-ligas.md](equipaciones-y-ligas.md); aquí solo el resumen.

Última actualización: 9 de noviembre de 2026.

---

## 1. Objetivo

Ampliar el editor FUTBOL_RELOAD para PC Fútbol 4.5 y poder:

| Objetivo | Estado |
|---|---|
| Editar las **equipaciones** (camiseta, pantalón, medias) | ✅ Hecho y probado en el juego |
| Editar las **botas** | ✅ Hecho (por equipo; por jugador no existe en el juego) |
| Que no se confundan las equipaciones (**2ª / 3ª equipación**) | ✅ Hecho y probado en el juego (no hay 3ª) |
| Editar las **características físicas** de los jugadores | ✅ Ya existía en el editor (piel, pelo, altura, peso) |
| Cambiar **qué equipos** juegan en Primera y Segunda | ✅ Hecho y probado en el juego |
| Cambiar **cuántos equipos** hay en la liga | ⛔ Descartado por el usuario (9/11/2026): sería rehacer medio juego |
| Dibujo del kit de la ficha del equipo (`CAMISAS/*.DFG`) | ✅ Hecho y probado en el juego (paleta = foto del estadio o escudo) |
| Clasificados para Europa (1ª temporada) | ✅ Hecho y probado en el juego |

---

## 2. Dónde está cada cosa

| Qué | Dónde |
|---|---|
| **Repositorio bueno** (el que vale) | `/media/sf_ps1/PCFUTBOL/FUTBOL_RELOAD`, rama `mejoras-equipaciones` (no subida a GitHub) |
| Guía técnica detallada | `docs/equipaciones-y-ligas.md` |
| Este resumen | `docs/ESTADO.md` |
| Compilación | `build/` dentro del repositorio (no se versiona) |
| Qt completo (con QtSvg) | `~/Qt/6.8.3/gcc_64` |
| Juego original (no tocar) | `~/pcfutbol_trabajo/juego_original/Futbol45` |
| Juego para pruebas (se modifica y se restaura) | `~/pcfutbol_trabajo/juego_pruebas/Futbol45` |
| DOSBox local (sin instalar en el sistema) | `~/pcfutbol_trabajo/dosbox` (`start.sh`, `run.conf`) |
| Scripts para manejar DOSBox | `FUTBOL_RELOAD/tools/dosbox/` (copia en `~/pcfutbol_trabajo/tools`) |
| Copias del juego del usuario | `/media/sf_ps1/PCFUTBOL/PCFutbol45` (original) y `PRUEBA_PCF45` (pruebas) |
| Copia antigua del editor (ignorar) | `~/pcfutbol_trabajo/repo` — solo como referencia |

Nota: en `PRUEBA_PCF45/Futbol45` se restauró `PCFUTBOL.DAT` desde su `.bak`
el 9/11/2026; la versión parcheada (que rompía el juego) quedó como
`PCFUTBOL.DAT.roto`.

---

## 3. Cómo trabajar sin colgar la máquina

La máquina virtual tiene 5,8 GB de RAM y VS Code ya usa unos 3,5 GB. Se ha
colgado **dos veces**:

- 8/11/2026: `make -j4` con DOSBox abierto a la vez.
- 9/11/2026: dos sesiones de Claude trabajando en paralelo, cada una
  compilando y con sus programas abiertos.

Reglas:

1. **Un solo proceso pesado a la vez.** Comprobar antes con `free -m`.
2. Compilar así:
   ```
   cd /media/sf_ps1/PCFUTBOL/FUTBOL_RELOAD/build
   (ulimit -v 2500000; nice -n 19 make -j1)
   ```
3. Cerrar DOSBox antes de compilar:
   `pkill -f '[r]oot/usr/bin/dosbox'`. Los corchetes son necesarios: sin
   ellos, `pkill` también mata la propia terminal.
4. **No usar Ghidra** (Java consume demasiada memoria). Para desensamblar,
   usar `tools/le_plano.py` + `objdump` (ver §6).
5. `/tmp` se borra al reiniciar: no guardar nada importante ahí.
6. Tras un cuelgue, la carpeta compartida puede perder ficheros recién
   escritos. Revisar `git status` y recuperar con
   `git show HEAD:ruta > ruta` si falta algo. Hacer **commit a menudo**.
7. En la carpeta compartida, git necesita
   `git -c safe.directory=/media/sf_ps1/PCFUTBOL/FUTBOL_RELOAD ...`.

---

## 4. Lo descubierto (resumen)

### 4.1 Equipaciones — `PALETAS/EQUIPOS.DAT` (verificado en el juego)

- Bloques de 190 bytes, 2 por equipo (1ª y 2ª). El equipo *n* usa los
  bloques 2(*n*−1) y 2(*n*−1)+1.
- **Todo el bloque es una tabla de recoloreado** de los sprites del partido
  (`GRAFICOS/*.EMP`): el píxel del sprite con valor *v* se pinta con el color
  `bloque[v]`. La "textura" de 168 bytes empieza en el byte 8, así que
  posición de textura = *v* − 8.
- Zonas (posiciones de textura): **botas 7, 22, 23** · pelo 14 · **medias
  18–20** · **pantalón 9–13, 15, 63–71** · fijos 8, 17, 21 · resto camiseta.
  Bytes 1–6 piel, byte 7 sombra en el césped.
- La posición 7 es a la vez el **contorno del icono** de los menús y el
  **color de las botas**.
- **Botas por equipo, no por jugador**: el DBC no tiene campo de botas.
- Probado: Barça con botas rojas y pantalón amarillo; Madrid con pantalón
  negro, medias moradas y botas blancas.

### 4.2 Qué equipación se usa: choques de color (verificado en el juego)

- Bytes **185** (índice de paleta) y **187–189** (RGB) = **color
  representativo** del kit.
- `PCMANAG.DAT`/`PROMANAG.DAT` (función `0x74F3C` en `PCMANAG`) pasan a HSL
  el color de la 1ª equipación de los dos equipos. Si los tonos distan
  **menos de 60°**, el visitante usa la 2ª.
- Blanco, gris y negro no chocan con colores con tono, pero sí entre sí
  (blanco contra negro también).
- **No se vuelve a comprobar** con la 2ª equipación. **No existe 3ª
  equipación.**
- Decide ese color, no la camiseta pintada. Probado: Valencia–Madrid → el
  Madrid sale de morado (2ª). Con el color representativo del Madrid en
  rojo, los dos juegan de blanco.
- **El editor lo mantiene automáticamente**: al cambiar la camiseta
  recalcula el color si ya no se parece. Los kits sin tocar no cambian
  (comprobado con los 470). Probado en el juego: Valencia de rojo → el
  Madrid juega con la 1ª.
- El kit de cada equipo se busca en una tabla de 235 IDs dentro del
  ejecutable (`0xAE980` en `PCMANAG`, `0xA318C` en `PROMANAG`). Coincide con
  los `EQ*.DBC` ordenados. **Añadir equipos obligaría a ampliar esa tabla.**

### 4.3 Composición de las ligas (verificado en el juego)

- Las tablas de IDs de equipo (u32) están en `PCMANAG.DAT` y `PROMANAG.DAT`:
  la lista larga de 99 (22 + 20 + Segunda B) y la tabla de 42.
- **Solo se deben modificar esos dos.** `PCFUTBOL.DAT` tiene otra copia,
  pero va ligada al calendario real 95/96 (`JORNAD1.DBC` = Primera,
  `JORNAD2.DBC` = Segunda). Si se cambia, el juego falla al entrar en el
  amistoso con `RetEquipPtr: ERROR: Equipo no encontrado [id]`.
- Probado con Madrid ↔ Mallorca: el amistoso y una Liga Manager nueva usan
  la composición nueva.
- Probado con el Levante (sin ficha completa) en Primera: la liga arranca, su
  despacho se abre y sus partidos se juegan.
- Los clasificados para Europa salen de otra tabla, ya localizada y editable
  (ver §4.7).

### 4.7 Clasificados para Europa (verificado en el juego)

- Tabla `4, ids, FFFF, 3, ids, FFFF, 2, ids, FFFF, FFFF` (Copa de Europa,
  Recopa, UEFA) en `PCMANAG` (`0xA21AC`) y `PROMANAG` (`0x9638C`). Solo decide
  la 1ª temporada: después el juego la recalcula.
- Editor: apartado «Clasificados para Europa» en «Composición de ligas».
  Detalle en `equipaciones-y-ligas.md` §3.
- Línea de órdenes: `ligas_pcf.py europa JUEGO [--copa 4] [--recopa 3,2]
  [--uefa 19,1,6]` (sin opciones, solo muestra). Da los mismos bytes que el
  editor.

### 4.4 Características físicas de los jugadores

- Ya estaban en el DBC y en el editor (ficha del jugador): **piel** (0
  blanca, 1 negra), **pelo** (0 rubio, 1 castaño claro, 2 negro, 3 gris,
  4 pelirrojo, 5 castaño oscuro), **altura** y **peso**.
- Altura y peso solo existen en equipos **con ficha completa** (byte `0x2A`
  del DBC = 0). Los de Segunda B no tienen esos campos.
- **Verificado en el juego:** el pelo y la piel del sprite salen de la ficha
  de cada jugador, no de `EQUIPOS.DAT` (allí el pelo, posición 14, vale `0x60`
  en todos los kits). Con pelo 4 en varios titulares del Madrid salen
  pelirrojos, y con piel 1 Laudrup sale con la piel oscura.
- En el DBC, tras los dos nombres del jugador: dorsal, estado de fichaje,
  demarcación, 5 demarcaciones secundarias, puesto en la alineación, `00`,
  **extranjero, piel, pelo**, posición en el campo.

### 4.5 Dibujo del kit — `DBDAT/CAMISAS/EQ95xxxx.DFG` (verificado en el juego)

- Solo 50 equipos. Familia A: cabecera de 8 bytes y 4 planos por fila. Sale
  en la ficha del equipo de la Base de datos, junto al botón «Plantilla».
- **Paleta = la de toda la ficha:** la de la foto del estadio
  (`BIGCAMP/ES95xxxx.DFG`) o, si no hay foto, la del escudo grande
  (`BIGESC/EQ95xxxx.DFG`). Colores **1, 2 y 8** propios del equipo; 9–11 de la
  foto; el resto, fijos.
- Editor: botón «Dibujo en la ficha del equipo…» en el editor de
  equipaciones. Detalle en `equipaciones-y-ligas.md` §2.

### 4.6 Otros datos útiles

- `JORNAD1/2.DBC`: calendario y crónicas de la temporada real. Empieza con
  `u16 jornada, u16 local, u16 visitante`; los registros son de longitud
  variable (textos con XOR 0x61). Formato completo sin descifrar.
- `LIGA.DBC`: histórico de clasificaciones, no la estructura de la liga.
- Código del juego: ejecutables LE de DOS/4GW. `PCFUTBOL.DAT` es el menú,
  la base de datos y el seguimiento; `PCMANAG.DAT` los modos de liga y el
  amistoso; `PROMANAG.DAT` el Promanager. `pcFut4.exe` es solo el lanzador.

---

## 5. Hecho en el editor (rama `mejoras-equipaciones`)

Commits principales, del más reciente al más antiguo:

| Commit | Qué |
|---|---|
| `44c1dbf` | Ligas: probado un equipo sin ficha en Primera |
| `83da4b0` | Ligas: no modificar `PCFUTBOL.DAT` (rompía el juego); ofrece restaurarlo |
| `c0e8349` | Equipaciones: color representativo y «Comprobar choques» |
| `fa2dcb8` y anteriores | Ventanas para pantallas pequeñas, tema oscuro, ventana «Composición de ligas», `ligas_pcf.py`, pantalón/medias/botas con vista previa del partido, guardado que no estropea los bytes 176–189 |

Errores del editor original corregidos:

- Guardar estropeaba los bytes 176–189 de cada kit, que incluyen el color
  representativo.
- «Aplicar patrón» pintaba también pantalón, medias y botas.

---

## 6. Cómo probar y analizar

### Probar en el juego (DOSBox en Linux)

```
~/pcfutbol_trabajo/dosbox/start.sh pcFut4 &          # abre el juego
python3 FUTBOL_RELOAD/tools/dosbox/dbx.py shot c.png  # captura
python3 FUTBOL_RELOAD/tools/dosbox/dbx.py keys "5,sleep6,Return"
python3 FUTBOL_RELOAD/tools/dosbox/empujar.py CX CY DX DY click
```

- DOSBox se instaló sin `sudo`: `apt-get download` de `dosbox`,
  `libsdl1.2debian`, `libsdl-sound1.2`, `libsdl-net1.2`, `libsdl2-2.0-0` y
  `libmikmod3`, después `dpkg -x` en una carpeta y enlace
  `libSDL-1.2.so.0 → libSDL-1.2.so.1.2.68`. `start.sh` fija
  `LD_LIBRARY_PATH`. Configuración en `tools/dosbox/run.conf.ejemplo`
  (**`autolock=false` es imprescindible**).
- Ratón: cada pantalla del juego tiene su propia escala vertical y algunas
  limitan la zona del cursor, así que los clics absolutos fallan. Lo fiable
  es `empujar.py`: se mira en la captura dónde está el cursor del juego
  (CX,CY) y se desplaza (DX,DY). Teclas útiles: `5` amistoso, `8` Liga
  Manager, `Return` para avanzar, letra subrayada de cada botón.
- Si otra ventana tapa la de DOSBox, las capturas salen congeladas y los
  clics se pierden. `dbx.py` la sube al frente.
- **Al terminar, restaurar** `juego_pruebas` desde `juego_original` y
  comprobarlo con
  `diff -rq juego_original/Futbol45 juego_pruebas/Futbol45`.

### Probar el editor sin interfaz

Programas de prueba en `~/pcfutbol_trabajo/test2/` (`prueba.cpp` para
equipaciones y `prueba_ligas.cpp` para ligas). Se enlazan con `build/*.o`
(todos menos `main.o`), abren el diálogo, llaman a sus slots con
`QMetaObject::invokeMethod`, aceptan los `QMessageBox` con un `QTimer` y
guardan capturas.

### Desensamblar

```
python3 FUTBOL_RELOAD/tools/le_plano.py PCMANAG.DAT pcmanag.bin
python3 FUTBOL_RELOAD/tools/buscar_refs.py pcmanag.bin 10000 "%s:EQUIPOS.DAT"
objdump -D -b binary -m i386 --adjust-vma=0x10000 --start-address=0x73d28 --stop-address=0x74600 pcmanag.bin
```

Todas las direcciones de esta documentación son de esa imagen (base
`0x10000`). `tools/le_a_binario.py` produce lo mismo separado por objetos.

---

## 7. Lo que falta, por orden de prioridad

Prioridad actual: no queda nada importante; ver §7.4 (menores).

### 7.1 Cambiar el número de equipos de la liga — DESCARTADO

El usuario lo descartó el 9/11/2026 («sería como hacer todo el juego otra vez»).
**No retomarlo** salvo que lo pida expresamente. Se conserva el análisis por si acaso:


Análisis previo en `equipaciones-y-ligas.md` §4. En resumen:

- Está fijado en el código de `PCMANAG.DAT` y `PROMANAG.DAT`: 11/10
  partidos por jornada, 42/38 jornadas, estructuras de 22 puestos
  (`imul ...,0x16`) y calendario de `0x1734` bytes por división. Hay listas
  de direcciones concretas en esa sección.
- Realista: **Primera a 20** o **Segunda a 22**. Más de 22 desbordaría
  estructuras.
- Hay que tocar también: la tabla de emparejamientos de la jornada 1
  (`0xA3C14`, 11 + 10 pares), el número de ascensos y descensos, la tabla de
  42 (`cmp ...,0xA8`) y el límite de índice 41.
- `PCFUTBOL.DAT` (base de datos y seguimiento de la liga real) tiene su
  propio 22/20 ligado a `JORNAD*.DBC`. Lo más prudente es **no tocarlo** y
  cambiar solo los modos de liga.
- Riesgo: Copa y competiciones europeas se juegan entre jornadas concretas.
  Con menos jornadas, alguna ronda podría no jugarse nunca.
- Plan:
  1. Localizar todos los sitios en `PCMANAG.DAT`.
  2. Parche mínimo para Primera a 20 en una copia.
  3. Probar en DOSBox una **temporada completa** en modo «Resultado».
  4. Si funciona, lo mismo en `PROMANAG.DAT`.
  5. Herramienta y botón en el editor.

### 7.2 Paleta de los dibujos `CAMISAS/*.DFG` — HECHO (9/11/2026)

Ver §4.5.

### 7.3 Clasificados para competiciones europeas — HECHO (9/11/2026)

Ver §4.7. Lo que sigue es la nota original.

Localizar la tabla de equipos que van a Copa de Europa, Recopa y UEFA en
`PCMANAG`/`PROMANAG`, para que cuadre con una composición cambiada. Buscar
por los IDs del Madrid (4), Zaragoza (3), Deportivo (2), Betis (19),
Barcelona (1) y Sevilla (6), cerca de las cadenas "COPA DE EUROPA" y
"RECOPA".

### 7.4 Menores

- ~~Comprobar si el pelo lo cambia el juego por jugador~~: hecho, sí (§4.4).
- Descifrar `JORNAD*.DBC`. Solo haría falta para cambiar la composición
  también en la base de datos o el seguimiento de la liga real.
- ~~Europa en `ligas_pcf.py`~~: hecho (orden `europa`, §4.7).
- Subir la rama a GitHub o abrir PR: solo cuando el usuario lo pida.
