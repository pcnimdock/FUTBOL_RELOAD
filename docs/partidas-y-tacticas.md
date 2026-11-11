# Tácticas y partidas guardadas (PC Fútbol 4.5, Liga Manager)

Analizado el 10/11/2026 sobre `PCMANAG.DAT` (desensamblado) y sobre ficheros
creados en el juego (DOSBox): una liga nueva con el Real Madrid, dos tácticas
grabadas y la partida guardada antes y después de la jornada 1.

## Dónde está cada cosa

| Qué | Fichero | Quién lo escribe |
|---|---|---|
| Táctica por defecto de cada equipo | Dentro de su `DBDAT/EQ95xxxx.DBC` (176 bytes) | El editor (base de datos) |
| Tácticas grabadas con «Grabar táctica» | `TACTICS/TACTIC.000` … `TACTIC.199` (244 bytes) | El juego |
| Partida de Liga Manager («Grabar Liga») | `TACTICS/LIGA.0` … `LIGA.9` (~120 KB) | El juego |
| Partida de Promanager | `TACTICS/PROLIGA.nnn` (una por liga, se sobrescribe al grabar) y la ficha del mánager en `PLAYERS/PLAYnnnn.DAT` | El juego (formato abajo) |
| Liga en curso al salir | `ACTLIG.DAT` (se borra al empezar otra liga) y `COMFILE.DAT` | El juego |

Las tácticas se editan dentro del juego (Entrenador → Tácticas) y se pueden
grabar y cargar desde allí. Manager y Promanager comparten `TACTIC.nnn`.

## Posiciones tácticas (común a DBC, `TACTIC.nnn` y partidas)

11 jugadores × 8 valores. En el DBC y en los ficheros son u16 LE; en memoria,
enteros de 32 bits.

| Valores | Significado |
|---|---|
| 0–3 | **Zona** del jugador: x, y, ancho, alto (rectángulo de la cuadrícula). Es la zona que el juego resalta al seleccionar un jugador en «Ver rival». En los 5.181 jugadores de los DBC de 4.0 y 4.5, la posición de defensa y la de ataque caen siempre dentro de la zona, y la zona dentro del campo. |
| 4–5 | Posición al **defender** (círculo en pantalla): columna, fila |
| 6–7 | Posición al **atacar** (flecha): columna, fila |

- Cuadrícula de 15 × 9: columna 0 = propia portería, columna 14 = la
  contraria; fila 0 = arriba, fila 8 = abajo.
- El portero es siempre `0,3,2,3, 0,4, 0,4` en los 236 DBC: zona de 2×3
  desde (0,3), y defensa y ataque en (0,4).
- Las 6 formaciones de los botones del juego (3-4-3, 3-5-2, 4-3-3, 4-4-2,
  5-3-2, 5-4-1) están en `PCMANAG.DAT` desde `0xA4FD0`, 6 × 0x160 bytes
  (11 × 8 int32). La 4-4-2 que graba el juego es idéntica a la tabla.

**Editor:** la ficha del equipo tiene el botón «Táctica…» (`dialog_tactica.*`,
`pcfutbol_tactica.*`). Comprobado en el juego el 10/11/2026:

- Se cambió con el editor la táctica del Real Madrid (5-3-2 con el 9 movido)
  y se empezó una liga nueva. «Ver rival» dibujaba las posiciones nuevas (en
  espejo, porque el rival ataca hacia la izquierda).
- Un `TACTIC.nnn` grabado por el editor aparece en «Cargar táctica» con su
  nombre y su autor, y se carga bien.

**En el DBC** el bloque son los primeros 176 bytes de lo que el editor llama
`cosas_bbdd`, justo después de `nombre_corto_mayusculas`. Lo tienen los 236
equipos. Por eso el comentario de `equipo.h` («202 bytes de estadísticas por
competición») no es exacto: 176 de esos 202 bytes son la táctica.

## `TACTICS/TACTIC.nnn` (244 bytes)

Lo escribe `PCMANAG.DAT` en `0x4bc60`–`0x4bdc0`.

| Offset | Tamaño | Contenido |
|---|---|---|
| 0x00 | 24 | Nombre de la táctica, terminado en 0, **XOR 0x6A** (p. ej. «Real Madrid», «Táctica 4-4-2») |
| 0x18 | 16 | Nombre del mánager, XOR 0x6A |
| 0x28 | 12 | Fecha: año, mes y día (u32 cada uno) |
| 0x34 | 16 | Hora: centésimas, segundos, minutos y hora (u32 cada uno) |
| 0x44 | 176 | Posiciones tácticas (tabla de arriba) |

Al grabar se usa el primer número libre de 000 a 199.

## Partida guardada `TACTICS/LIGA.n` (Liga Manager)

Un único fichero con toda la liga. Lo escriben `0x13600` (cabecera) y
`0x101e0` (estado). Las posiciones dependen de la longitud del nombre del
jugador; con «prueba» (6 letras) el calendario empieza en 0x65 y los equipos
hacia 0x1DF0.

| Offset | Contenido |
|---|---|
| 0x00 | Fecha y hora de grabación: 7 × u16 (año, mes, día, centésimas, segundos, minutos, hora). La ventana de cargar muestra el año como «año − 1900» (sale «126»). |
| 0x0E | Tipo de liga (0 = Manager) |
| 0x0F | Nombre del jugador: longitud + texto XOR 0x61 (como en los DBC) |
| detrás | Jornada actual (1 byte) |
| detrás | Texto de copyright del juego, 70 bytes |
| detrás | 8 bytes de estado (jornada, competición…) |
| detrás | Calendario de Primera: 45 × 11 partidos × 4 bytes: local, visitante, goles local, goles visitante. Los goles valen 255 si no se ha jugado. Los equipos se numeran como `EQ9500nn`. |
| detrás | Calendario de Segunda: 38 × 10 × 4 bytes |
| detrás | Copa del Rey, Copa de Europa, Recopa y UEFA (eliminatorias), clasificaciones y estadísticas. Crece al jugar (+418 bytes tras la jornada 1). |
| detrás | **Los 42 equipos completos**, más un bloque «SEGUNDA B» (de 2,2 a 3,8 KB cada uno) |

Cada equipo de la partida lleva:

- nombres, estadio, aforo, presidente y demás datos del club;
- el entrenador;
- **la táctica activa** (nombre + 11 × 8 valores);
- los niveles de entrenamiento;
- la plantilla con el **estado de cada jugador**:
  - puntero (la misma identidad que en el DBC) y nombre corto y largo, con XOR 0x61;
  - medias (VE, RE, AG, CA…);
  - partidos, goles y tarjetas, sueldo y valores en coma flotante (forma, moral…; sin identificar).

No lleva los comentarios largos ni las fotos: esos siguen saliendo del DBC.

Después de la jornada 1, `LIGA.1` tenía los resultados en el calendario, la
jornada en la cabecera y 1.028 bytes más (zona de copas y estadísticas, y el
bloque final).

### Formato completo (`PCMANAG.DAT`, verificado leyendo dos partidas enteras)

Tras la cabecera (ver tabla), lo que escribe `0x101e0`:

1. 70 bytes de copyright y 8 bytes de estado.
2. Calendarios: Primera, 45 × 11 × 4 bytes; Segunda, 38 × 10 × 4 bytes.
3. Copas. Cada cruce ocupa 10 bytes: 2 × u16 y 6 × u8.
   - Copa del Rey: 7 u16, la marca `1`, y 15 + 77 cruces.
   - Copa de Europa: 15 u16, la marca `2`, y 31 cruces.
   - Recopa: 23 u16, la marca `3`, y 63 cruces.
   - UEFA: 6 u16, la marca `4`, y 7 + 48 cruces.
4. La marca `5` y 19 u16.
5. Lista de traspasos: `0`, u16, double, u16, double y un jugador, repetido hasta
   `FFFF`.
6. Orden de los equipos: 22 + 20 u16.
7. Equipos: `1` + equipo, y `2` + jugador por cada jugador. Son 42 equipos más
   el 9900 (jugadores sueltos, «SEGUNDA B»), y la lista termina en `0`.
8. Cola: goleadores (`0x3a4c4`), una lista de registros y los 20 mánagers
   humanos. El editor la conserva tal cual.

**Equipo** (`0x24c07`). Los textos llevan un byte de longitud y van con XOR 0x61.

- Cuatro textos: nombre largo, nombre corto, mayúsculas y estadio.
- Entrenador: u16, texto y 2 × u32.
- 2 u16, el id (`EQ9500nn`), 4 u16 y un u32. Uno de esos u16 es el puesto en la
  clasificación.
- Nombre de la táctica y la táctica (176 bytes).
- **Entrenamiento**: 3 u16 (defensa, medio, delantera) y 2 u16 más (el último
  es el número de jugadores).
- 24 double: [0] presupuesto, **[1] caja**, [2] ¿?, [6] 100.000,
  [7] 2.500, y [13] y [14] la taquilla y las vallas del último partido en casa.
- 6 u16, 11 u16 (sin identificar; no son la alineación) y 2 u16.

**Jugador** (`0x322e1`). Casi todo es copia de su ficha del DBC; se cruzaron
1.027 jugadores y coinciden todos:

- puntero, nombre corto y nombre largo (Latin-1 con XOR 0x61);
- u16 **equipo de procedencia** (si es 9900, otro texto) y u16 **equipo
  actual**. Al fichar, el juego deja en el primero el equipo de donde viene;
- 7 u8: posición (0 POR … 3 DEL), demarcación, extranjero, piel, pelo, dorsal
  y **puesto en la alineación** (de 1 a N sin huecos);
- 20 u8: 5 competiciones × [`Incidencias`, `JornIncid`, `NJornIncid`,
  `NAmarillas`]. Son los nombres que dan los mensajes de error de las
  funciones `0x31bac`, `0x31c28`, `0x31ca4` y `0x31d20`. Incidencias 1 =
  lesión (Raúl y Palacios, con cruz en el juego). Márquez, con tarjeta roja,
  tenía `0,0,1`. En muchos jugadores sanos hay valores sin sentido (restos de
  memoria), así que no se interpretan; los cuatro a 0 es un jugador limpio;
- 2 u8;
- **double: precio** (la columna PRECIO de «Vender»; precio del DBC × 1 millón);
- **4 u8: VE, RE, AG, CA** y **4 u8: aptitudes de técnica, remate, pase y tiro**
  (0/1, las mismas del DBC);
- **u8: energía (EN)**. La media ME la calcula el juego.

Reglas que cumple la partida en sus 1.060 jugadores:

- equipo actual = id del equipo;
- alineación de 1 a N sin huecos;
- el equipo guarda su número de jugadores (5.º u16 tras el entrenamiento);
- plantillas de 20 a 33 jugadores.

Los jugadores en venta tienen **una copia** en la lista de traspasos, con su
equipo. Los goles de la cola no usan el puntero del jugador.

### Reglas del entrenamiento (importante)

El código de las flechas (`0x4c6b7`) solo deja cada barra entre **12 y 43**, y
subir solo si la suma × 3,2 < 240, es decir, **como mucho 75 entre las tres**.
Todos los equipos empiezan en 25/25/25, así que en el juego solo se puede
redistribuir. Con 50/0/10 el juego **se cuelga al abrir Tácticas** (probado).
Además, el cuelgue borra `COMFILE.DAT` y el juego ya no arranca hasta reponerlo.

### Bajas: cómo cuenta el juego las jornadas

Lo hace `PCMANAG.DAT` en `0x33263` (pantalla) y `0x37d32` (cada jornada):

- **Jornadas que faltan** = `NJornIncid + 1 − (jornada actual − JornIncid)`.
  La jornada actual es el 2.º de los 8 bytes de estado.
- Si ya se ha cumplido, `Incidencias` pasa a 0. Mientras `Incidencias` sea 0
  no hay baja, aunque los otros bytes tengan basura.
- Con 5 amarillas en liga (2 en copa) pone `Incidencias` = 4 (7 en copa),
  una jornada de sanción, y resta las amarillas.
- Tipos: 1 = lesión (cruz roja); los demás, sanciones.

Comprobado: Raúl (1,1,1) y Palacios (1,0,2) salen con «2» en el juego, y el
editor da 2.

### Estrellas Mundiales (bloque 9900)

Son las estrellas extranjeras (Batistuta, Baggio, Ronaldo…). Su procedencia
es 9900 seguida del **nombre de su club en texto** (Fiorentina, A.C. Milan…).
Cuando un equipo ficha a una estrella, conserva esa procedencia y su equipo
actual pasa a ser el nuevo. Maldini está además en la lista de traspasos.

### Promanager (`TACTICS/PROLIGA.nnn`)

Formato sacado de `PROMANAG.DAT` y comprobado leyendo una partida entera
(43 bloques):

- **Cabecera:** fecha (7 u16), **8 huecos de mánager** (u16 nº y su nombre,
  o FFFF si está vacío) y la jornada.
- **Igual que Liga Manager:** copyright, estado, calendarios, copas y
  marca 5 con 19 u16.
- **No hay lista de traspasos.** Viene el orden de los equipos y después los
  equipos (`1` + equipo, `2` + jugador…, `0`) y la cola.
- **Equipo** (`0x26453`):
  - 5 textos (con el presidente);
  - u16 id, u16, 3 u8, 3 u16 y 2 u32;
  - nombre de la táctica y táctica;
  - u16 y u16 **número de jugadores**;
  - entrenador (u16 + texto);
  - u16: si vale 1 siguen **48 registros de 84 bytes** (solo en el equipo
    del mánager humano);
  - 7 double: [0] presupuesto y **[1] caja**;
  - 9 u8, 11 u16 y 4 u16.
  - No guarda entrenamiento.
- **Jugador** (`0x36173`):
  - puntero, nombres y u16 de procedencia (texto si es 9900);
  - 7 u8 (posición, …);
  - 30 u8: extranjero, piel, pelo, dorsal, **alineación** y 5 grupos de
    incidencias, que empiezan en +8, +14, +18, +22 y +26 (en +13, el minuto
    de la última tarjeta);
  - u16 **equipo actual**, u16 y u16 **nº de orden** (de 1 a N);
  - double precio, double y u32;
  - 11 u8: VE, RE, AG y CA; 4 aptitudes; **MO, EF y EN** (moral,
    efectividad y energía, las columnas de la Alineación del Promanager).
- **Cuidado al probar:** el juego no cierra el fichero hasta que se acepta el
  mensaje «La liga actual ha sido grabada». Si se cierra DOSBox antes, queda a
  medias (múltiplo de 4 KB) y al cargarla el juego se cuelga.

**Ficha del mánager** (`PLAYERS/PLAYnnnn.DAT`, 83 bytes):

- u16 y el copyright;
- el nombre con XOR 0xAA, en un campo de 20 bytes;
- la **clave**, con XOR 0xC4, 0xC5, 0xC6… según la posición, terminada en un
  0 sin cifrar. «abc» se guarda como `a5 a7 a5 00`. La del mánager «prueba»
  que trae el juego es «w».

### Editor de partidas (botón «Partida guardada…»)

`pcfutbol_partida.*` y `dialog_partida.*`. Abre partidas de **Liga Manager y
de Promanager** (detecta cuál es). La primera vez deja copia `.bak` y,
antes de escribir, comprueba que la partida se puede volver a leer.

- **En su sitio** (el fichero no cambia de tamaño):
  - de cada equipo: caja, entrenamiento (con las reglas de abajo) y táctica;
  - de cada jugador: VE, RE, AG, CA, EN y precio;
  - **«Quitar lesión y sanción»**, del jugador o de todo el equipo: pone a 0 los
    20 bytes de incidencias, y también los de su copia en la lista de
    traspasos.
- **Reescribiendo el fichero:** se rehace a partir de los registros leídos.
  Sin cambios sale idéntico byte a byte.
  - **Cambiar nombre** (corto hasta 16 y largo hasta 40, letras de Latin-1).
  - **Traspasar** a otro equipo de la liga, como un fichaje del juego:
    - procedencia = origen y equipo actual = destino;
    - va el último de la alineación del destino y se renumera la del origen;
    - si su dorsal está ocupado, recibe el primero libre;
    - se actualiza el número de jugadores de los dos equipos;
    - si está en venta, su copia en la lista de traspasos pasa al destino;
    - límites: origen con al menos 16 jugadores y destino con 33 como mucho.
      Se pueden fichar Estrellas Mundiales (conservan su club de
      procedencia), pero no mandar jugadores allí; el juego tampoco lo hace.
  - **Añadir jugador:** con identidad, dorsal y puesto libres, sin bajas, o
    copiando sus datos de un jugador de la base de datos del editor (el
    precio de la partida es el del DBC × 1 millón). En Promanager se clona un
    jugador del equipo para los campos sin identificar.
- En Promanager no hay entrenamiento ni lista de traspasos.

Comprobado en el juego (10 y 11/11/2026):

- la caja de 2.000 y de 3.000 millones aparece en «Caja»;
- VE 99 y EN 50 de Buyo y CA 99 de Hierro aparecen en «Alineación»;
- el precio de 123.456.789 aparece en «Vender» como «123 m.»;
- la táctica 5-4-1 y el entrenamiento 30/20/25 aparecen en «Tácticas»;
- «Buyazo» aparece en la alineación;
- Hierro, traspasado al Rayo, ya no sale en el Real Madrid y los demás suben
  un puesto;
- Begiristain, traspasado al Rayo estando en venta, sale en «Comprar» como
  «Rayo», y comprarlo desde el juego funciona («Jugador comprado»);
- se jugó la jornada y el juego volvió a grabar la partida con todos los
  cambios y las reglas intactas;
- Raúl, lesionado (cruz «2»), queda sano tras «Quitar lesión y sanción», y la
  media del equipo sube de 75 a 82.

Comprobado también en el juego (11/11/2026):

- **Liga Manager:**
  - Batistuta, fichado de las Estrellas, y un jugador nuevo («Novato»)
    aparecen en la plantilla con sus medias;
  - se juega la jornada y el juego los vuelve a grabar bien;
  - Raúl, lesionado, queda sano tras «Quitar lesión y sanción».
- **Promanager (Villarreal):**
  - caja de 999 millones en «Caja»;
  - VE 99 en la Alineación, «Renombrado» en el puesto 13;
  - Hierro traído del Real Madrid y «Novato» en los puestos 23 y 24, con su
    energía y medias;
  - se jugó la jornada y el juego grabó la partida con todo intacto;
  - quitar la baja a Puche II elimina el icono y el aviso de bajas.

### Consecuencia para el editor (comprobado en el juego)

Se cambió «Buyo» por «Bayo» solo dentro de `LIGA.0` y se cargó la partida:
la alineación mostraba «Bayo». **El juego toma los equipos y jugadores de la
partida guardada, no de los DBC.** Por tanto:

- Lo que se cambie con el editor en los DBC (plantillas, traspasos, medias,
  tácticas por defecto) solo vale para **ligas nuevas**. Una partida ya
  empezada no lo ve.
- Para tocar una partida en curso habría que editar `LIGA.n` directamente.
  Los nombres son de longitud variable, así que cambiarlos de largo mueve
  todo lo que va detrás; los valores numéricos se pueden cambiar en su sitio.

## Pendiente

- Identificar los otros double del equipo.
- En Promanager: el significado de los 48 registros del mánager, la
  demarcación y el resto de bytes del jugador.
- Un editor de la ficha del mánager (ver o cambiar la clave); el formato ya
  se conoce.
- Promanager: `PROLIGA.nnn` y `PLAYERS/PLAYnnnn.DAT`, por probar.
- Ficheros de muestra en `~/pcfutbol_trabajo/test3/partidas/` (fuera del repositorio).
