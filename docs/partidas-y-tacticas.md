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
| detrás | 8 bytes de estado: nº de jugadores humanos, **jornada actual** (la de las bajas), dos variables del bucle de liga y promoción, una del partido, otra sin uso visto, **competición en curso** (`nActualComp`) y otra del bucle |
| detrás | Calendario de Primera: 45 × 11 partidos × 4 bytes: local, visitante, goles local, goles visitante. Los goles valen 255 si no se ha jugado. Los equipos se numeran como `EQ9500nn`. |
| detrás | Calendario de Segunda: 38 × 10 × 4 bytes |
| detrás | Las cuatro copas (Copa del Rey, Recopa, UEFA y Copa de Europa): una cabecera de u16 y los cruces de 10 bytes. Ver «Copas» más abajo. La marca 5 lleva la tabla de clasificados para Europa (15 u16, la de `0xA21AC`) y 4 u16 más. |
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
3. Copas (ver «Copas» más abajo). Cada cruce ocupa 10 bytes.
   - Copa del Rey: 7 u16, la marca `1`, y 15 + 77 cruces.
   - Recopa: 15 u16, la marca `2`, y 31 cruces (32 equipos).
   - Copa de la UEFA: 23 u16, la marca `3`, y 63 cruces (64 equipos).
   - Copa de Europa: 6 u16, la marca `4`, 7 cruces (final, semifinales y
     cuartos) y 6 × 8 partidos de la fase de grupos.
4. La marca `5` y 19 u16.
5. Lista de traspasos: `0`, u16, double, u16, double y un jugador, repetido hasta
   `FFFF`.
6. Orden de los equipos: 22 + 20 u16.
7. Equipos: `1` + equipo, y `2` + jugador por cada jugador. Son 42 equipos más
   el 9900 (jugadores sueltos, «SEGUNDA B»), y la lista termina en `0`.
8. Cola. El editor la conserva tal cual y se lee entera hasta el último byte:
   - **goles** de cada división, partido a partido en orden de calendario:
     u16 equipo, u16 puesto del goleador en la alineación, u8 tipo, u8
     minuto. Termina en `55AA` al primer partido sin jugar;
   - **lista de jugadores en venta tal como se muestra:** u16 cuántos, y por
     cada uno texto nombre, u8, u8, u16 equipo, u16 (FFFF) y double precio;
   - **20 mánagers humanos:** texto nombre, u16 equipo, u16 **tipo de
     control** (1 = teclado) y las **5 teclas** (scancode y letra: O, F, P, V
     y Espacio por defecto).

**Equipo** (`0x24c07`). Los textos llevan un byte de longitud y van con XOR
0x61. Entre corchetes, la posición en la estructura en memoria.

| Campo | Significado |
|---|---|
| 4 textos | nombre largo, corto, mayúsculas y estadio |
| entrenador | u16, nombre y 2 u32 |
| u16 [0x1B8], u16 [0x1BC] | **posición del escudo** X e Y (iguales al DBC en los 42 equipos) |
| u16 [0x14] | **id** (`EQ9500nn`) |
| u16 [0x18] | **división** (0 Primera, 1 Segunda) |
| u16 [0x1C] | **nº de jugador humano** que lo lleva (FFFF = ordenador) |
| u16 [0x20] | **puesto en la clasificación** |
| u16 [0x24] | sin inicializar (valor al azar fijo por equipo; no se usa) |
| u32 [0x28] | **aforo** (igual al DBC en los 42 equipos) |
| texto + 176 bytes | nombre y **táctica** activa |
| 3 u16 [0x1A8…] | **entrenamiento** (defensa, medio, delantera) |
| u16 [0x1B4] | **cambios hechos en la alineación** esta semana («Cambios realizados: %d») |
| u16 [0x1C8] | **nº de jugadores** |
| 24 double [0x1CC…] | dinero: ver la tabla siguiente |
| u16 [0x28C] | **puesto en la jornada anterior** |
| u16 [0x290] | división (para ascensos y descensos) |
| u16 [0x294] | **semanas del crédito** («A devolver en N semanas») |
| u16 [0x298] | **jornada en que entró en números rojos** (aviso de quiebra a las 3 semanas) |
| u16 [0x29C], u16 [0x2A0] | marcas de la televisión (pasan a 1 tras la jornada; la Caja usa 0x2A0 para la TV) |
| 11 u16 [0x2A4…] | **emparejamientos** (marcajes de los 11 titulares; 255 = ninguno) |
| u16 [0x2D0], u16 [0x2D4] | posición en pantalla de un elemento arrastrable de «Emparejamientos» (100, 200) |

Los 24 double (identificados con las pantallas Caja y Decisiones):

| nº | Significado | nº | Significado |
|---|---|---|---|
| 0 | presupuesto | 12 | compra de jugador (semana anterior) |
| 1 | **caja** | 13 | Liga: taquilla |
| 2 | caja al empezar la semana | 14 | Liga: publicidad (vallas) |
| 3 | crédito concedido | 15 | Copa de Europa: taquilla |
| 4 | deuda pendiente (intereses = d4 × 12 / 5.200) | 16 | Copa de Europa: publicidad |
| 5 | devolución semanal | 17 | Copa de Europa: ingresos por punto |
| 6 | **precio por valla** | 18-20 | Copa del Rey: taquilla, publicidad, TV |
| 7 | **precio de la entrada** (taquilla = espectadores × d7) | 21-23 | UEFA y Recopa: taquilla, publicidad, TV |
| 8, 9 | primas pagadas (por victoria / por empate) | | |
| 10 | venta de jugador | | |
| 11 | compra de jugador (semana) | | |

- Las **primas que se fijan en «Decisiones» no se graban** en Liga Manager: al
  cargar vuelven a 0 (comprobado).
- La Caja calcula su primer pago como (presupuesto / 1.000)^¼ × 36 × un
  campo entero de la estructura (no se ha comprobado cuál).

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
- 2 u8: **tarjeta en el último partido** (1 amarilla, 2 roja) y **minuto**. Al
  cerrar la jornada, una roja crea la incidencia de tipo 3 y una amarilla suma
  una a NAmarillas;
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
- Tipos (llamadas a `0x318fc`): 0 ninguna, 1 lesión (cruz roja), 2 y 3
  expulsión, 4 sanción por 5 amarillas en liga, 7 sanción por 2 amarillas en
  copa.

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
  - u16 id y u16 sin inicializar;
  - 3 u8: **división**, **objetivo de la temporada** y **nº de orden en su
    división**. Objetivos (tabla de `PROMANAG.DAT`): 0 Título de Liga,
    1 Entrar en la UEFA, 2 Zona tranquila (1ª), 3 Permanencia (1ª),
    4 Ascenso a Primera, 5 Zona tranquila (2ª), 6 Permanencia (2ª). Coincide
    con la pantalla de ofertas (Villarreal 5; Écija, Eibar, Sestao y
    Getafe 6);
  - 3 u16: **nº de mánager humano** (FFFF = ordenador), **puesto en la
    clasificación** y uno sin inicializar;
  - 2 u32: **aforo** y **socios** (iguales al DBC en los 42 equipos);
  - nombre de la táctica y táctica;
  - u16 y u16 **número de jugadores**;
  - entrenador (u16 + texto);
  - u16: si vale 1 siguen 48 × 84 bytes con el **historial semanal de la Caja**
    del mánager humano: 48 semanas × 21 `float` ([0] caja al empezar la
    semana, [7] fichas, [9] taquilla, [10] vallas…);
  - 7 double: presupuesto, **caja**, **precio de la entrada**, **precio por
    valla**, **prima por objetivo**, **prima por puntos** y **precio de la
    ampliación del estadio**. Las primas sí se graban en Promanager
    (comprobado en «Decisiones»);
  - 9 u8:
    - puesto anterior y división;
    - **semanas del crédito**;
    - **semanas de la obra del estadio**, **semanas que faltan** (distinto de
      0 = obra en marcha; dinero restante = precio / semanas × faltan) y
      **ampliación elegida** (0, 1, 2 = 4.000, 8.000 o 12.000 asientos);
    - jornada en que entró en números rojos;
    - dos marcas de la televisión;
  - 11 u16 de **emparejamientos**, y 4 u16: posición del elemento de
    emparejamientos (100, 200) y **posición del escudo** X e Y.
  - No guarda entrenamiento.
- **Jugador** (`0x36173`):
  - puntero, nombres y u16 de procedencia (texto si es 9900);
  - 7 u8: posición, **demarcación − 1** y las **5 demarcaciones secundarias
    − 1** (98 = ninguna). Comprobado en los 1.027 jugadores;
  - 30 u8:
    - [0] **demarcación en la que juega** (la asignada en la alineación);
    - [1] extranjero, [2] piel, [3] pelo, [4] dorsal, [5] **alineación**;
    - [6] partidos jugados;
    - [7] siempre 255;
    - 5 grupos de incidencias en [8], [14], [18], [22] y [26], y en [12]-[13]
      la **tarjeta del último partido y su minuto**;
  - u16 **equipo actual**, u16 (siempre FFFF) y u16 **nº de orden** (de 1 a N);
  - double precio, double (siempre 0 en las partidas vistas) y u32 que crece
    con el precio (unas 574 veces el precio en millones). No se ha confirmado
    qué es: la suma de la plantilla no da las fichas de la Caja con un factor
    limpio;
  - 11 u8: VE, RE, AG y CA; 4 aptitudes; **MO, EF y EN**.
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

## Copas (Liga Manager)

Comprobado con 11 partidas grabadas a lo largo de una temporada jugada de
forma automática con el Real Madrid (jornadas 3 a 29, hasta las finales). Los ids de equipo son los de la base de datos: los españoles
son 1 Barcelona, 2 Deportivo, 3 Zaragoza, 4 Real Madrid…, y los extranjeros
llevan el país en las centenas (852, 2102…).

**Las marcas no van en el orden de la tabla de clasificados.** Lo confirman
los equipos españoles de 1995-96 que aparecen en cada bloque:

| Marca | Competición | Españoles | Código en la tabla de `0xA21AC` |
|---|---|---|---|
| 1 | Copa del Rey | todos | — |
| 2 | Recopa | Zaragoza (3), Deportivo (2) | 3 |
| 3 | Copa de la UEFA | Betis (19), Barcelona (1), Sevilla (6) | 2 |
| 4 | Copa de Europa (Liga de Campeones) | Real Madrid (4) | 4 |

**Cruce** (10 bytes): u16 equipo 1, u16 equipo 2, y 6 u8:

| byte | Significado |
|---|---|
| 0 | goles del equipo 1 en la ida; **+128 si es el que pasa** |
| 1 | goles del equipo 1 en la vuelta (255 = sin jugar) |
| 2 | goles del equipo 2 en la ida; **+128 si es el que pasa** |
| 3 | goles del equipo 2 en la vuelta (255 = sin jugar) |
| 4, 5 | siempre 0 en todo lo visto; por el orden en que se escriben, probablemente los penaltis |

Un cruce sorteado y sin jugar lleva `255 255 255 255 0 0`; con la ida
jugada, por ejemplo, `2 255 1 255 0 0` (2-1). Al resolverse, el que pasa
lleva el bit alto en su byte de ida: `130 0 1 0` es 2+0 contra 1+0, pasa
el equipo 1; `1 0 129 2` es 1+0 contra 1+2, pasa el equipo 2.

El equipo 1 juega la ida en casa. Comprobado en los 180 cruces resueltos
de una temporada: el bit va siempre al que suma más goles, y en caso de
empate al que marcó más fuera (`129 1 0 2` es 2-2 y pasa el equipo 1 por su
gol en la vuelta). No salió ningún empate también en goles fuera, así que
no se ha visto cómo se guardan los penaltis.

La final se guarda como un cruce más, pero es a partido único: la
cabecera la cuenta como una ronda.

Las rondas van **de la final hacia atrás**: el primer cruce es la final,
luego 2 semifinales, 4 cuartos, etc. La primera ronda que se juega es la
última del bloque. En la Copa de Europa, los 6 × 8 partidos son las 6
jornadas de la fase de grupos (4 grupos de 4, 8 partidos por jornada), con
la jornada 1 al final. Usan solo la ida (bytes 1 y 3 a 255).

**Cabeceras.** Recopa, UEFA y Copa de Europa empiezan igual:

| u16 | Significado |
|---|---|
| 0 | cruces con la ida jugada y la vuelta pendiente (0 si no hay ronda a medias) |
| 1 | rondas terminadas (en la Copa de Europa cuenta también las 6 jornadas de grupos: 8 = grupos, cuartos y semifinales) |
| 2 | 1 entre la ida y la vuelta, 0 si no |
| 3 | equipos que siguen vivos (32 → 16 → 8 en la Recopa) |
| 4, 5 | 0 |

Ejemplos de la Recopa: `16 0 1 32` tras la ida de la primera ronda,
`0 1 0 16` al terminarla, `8 1 1 16` tras la ida de la segunda, y
`0 2 0 8` al terminarla.

- Recopa y UEFA siguen con una lista de ids de equipo terminada en `FFFF`:
  los equipos de las ligas de la base de datos con plaza directa (8 y 16;
  incluye a los españoles). No cambia durante la temporada.
- La Copa del Rey lleva un u16 más al principio. Vale 6, que es la ronda de
  entrada (la de 44 cruces, la sexta empezando por la final). Detrás vienen
  los mismos 4 campos (`40 0 1 80` tras la primera ida, `0 1 0 40` al
  acabarla) y otro 0.
- Antes de que empiece una copa, su cabecera está entera a 0.

## Pendiente

- Identificar los otros double del equipo.
- **Sin identificar (no afectan a la edición):**
  - Liga Manager: 5 de los 8 bytes de estado de la cabecera (variables internas
    del bucle de liga y del partido);
  - los bytes 4 y 5 de cada cruce de copa (siempre 0; no ha salido ningún
    cruce que se decida por penaltis) y el primer u16 de
    la cabecera de la Copa de Europa en la jornada 3 (vale 2);
  - Promanager: el u32 y el segundo double del jugador, y u16 y bytes
    siempre fijos (FFFF, 255, 0) que no cambian en las partidas vistas;
  - campos sin inicializar (valores al azar fijos por equipo): Liga Manager
    [0x24]; Promanager el 2.º u16 y el 3.º de los 3 u16 del equipo.
- Un editor de la ficha del mánager (ver o cambiar la clave); el formato ya
  se conoce.
- Promanager: `PROLIGA.nnn` y `PLAYERS/PLAYnnnn.DAT`, por probar.
- Ficheros de muestra en `~/pcfutbol_trabajo/test3/partidas/` (fuera del repositorio).
