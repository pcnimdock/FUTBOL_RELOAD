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
| Partida de Promanager | `TACTICS/PROLIGA.nnn` y la ficha del mánager en `PLAYERS/PLAYnnnn.DAT` | El juego (por las cadenas de `PROMANAG.DAT`; sin probar) |
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

- Identificar los campos del estado del jugador (los valores en coma
  flotante, los contadores) y el bloque de dinero/caja del equipo del
  jugador. El método es grabar, cambiar una sola cosa en el juego y comparar.
- Promanager: `PROLIGA.nnn` y `PLAYERS/PLAYnnnn.DAT`, por probar.
- Ficheros de muestra en `~/pcfutbol_trabajo/test3/partidas/` (fuera del repositorio).
