# Plantillas, traspasos, CSV y temporada real

Detalle técnico de lo hecho el 9/11/2026. El resumen está en
[ESTADO.md](ESTADO.md), §4.8 a §4.10.

## Qué se puede hacer en el editor

| Botón (ventana principal) | Qué hace |
|---|---|
| **Traspasos…** | Dos plantillas lado a lado; se mueven jugadores con las flechas o con doble clic. Conservan foto y datos. |
| **Exportar a CSV (Excel)…** | Escribe `jugadores.csv`, `equipos.csv` y `LEEME_CSV.txt` (columnas y códigos). |
| **Importar CSV…** | Lee uno de los dos CSV editado, enseña un resumen y aplica los cambios en el editor (no escribe en el juego hasta pulsar Salvar). |
| **Comprobar plantillas** | Busca lo que puede colgar el juego. También se ejecuta solo al pulsar Salvar. |
| **Restaurar originales…** | Lista las copias `.bak` que dejan las partes del editor que escriben en el juego y devuelve los ficheros marcados a su estado original. |
| **Buscar equipo** | Ya existía el cuadro, pero no hacía nada; ahora filtra la tabla. |

Código: `pcfutbol_plantillas.*` (reglas), `pcfutbol_csv.*`,
`pcfutbol_temporada_real.*`, `dialog_traspasos.*` y `dialog_restaurar.*`.

## Reglas de las plantillas (sacadas de los DBC originales)

- El **puesto en la alineación** de cada plantilla va de 1 a N, sin huecos ni
  repetidos (en los 235 equipos de la 4.5 y los 234 de la 4.0).
- **Ningún equipo repite dorsal.**
- Plantillas de **16** jugadores (los extranjeros) a **33** (el máximo
  español). Por encima de 33 no se ha probado.
- El **puntero** del jugador da nombre a su foto (`MINIFOTO/J95xxxxx.DFG`) y
  es lo que usa el juego para identificarlo. Un jugador traspasado conserva
  el suyo; uno nuevo recibe el primero libre.
- **No hace falta foto**: 3.084 de los 4.151 jugadores no la tienen.
- Textos con **1 byte de longitud** (máximo 255). Las letras van con la tabla
  del juego (ASCII y á é í ó ú ñ ü ç…). Las demás (ć, ł, ø…) se cambian por
  la más parecida.
- 30 punteros están repetidos en dos equipos en el original. Son jugadores
  del filial, por ejemplo del Barcelona y el Barcelona B.

## Temporada real: jugadores que no pueden salir de su equipo (verificado en el juego)

**Síntoma.** Al dar de baja a Buyo (Real Madrid, puntero 1) o traspasar a
Lopetegi (Barcelona, puntero 23), el juego no arranca. `PCFUTBOL.DAT` (el
menú) muestra en bucle «NJugadorEquipo: ERROR: Jugador no encontrado.
Equipo: Real Madrid, Indice Jugador: 1».

**Causa.** `JORNAD1.DBC` (Primera 95/96) y `JORNAD2.DBC` (Segunda) guardan
los partidos de la temporada real con las alineaciones. Al arrancar, el menú
comprueba que cada jugador de esos partidos sigue en su equipo.

**Solución en el editor.** Al traspasar uno de estos jugadores se deja
también una **copia en su equipo**, que es lo que hace el juego original con
los del filial. No se pueden dar de baja. «Comprobar plantillas» marca como
grave cualquier jugador de esos partidos que falte. Son **786 jugadores** en
la 4.5, casi todos los de Primera; en el Barcelona, los 31.

**Probado en el juego (DOSBox):**

- Lopetegi traspasado con copia: el juego arranca y Lopetegi sale en la
  plantilla del Deportivo, con su foto y el escudo y la camiseta del
  Deportivo en la ficha.
- Boskovic, traspasado desde un equipo extranjero, sale en el Zaragoza.
- Cikovic, alta nueva por CSV, sale en el Zaragoza como «Nuevo fichaje», con
  1,85 m, delantero y nacido en 1980.
- El modo Liga Manager deja elegir el Zaragoza. No se llegó a jugar un
  partido porque el ratón de DOSBox no respondía en esa pantalla.

### Formato de `JORNAD1.DBC` / `JORNAD2.DBC`

Los dos ficheros se leen enteros, sin bytes sobrantes: JORNAD1 tiene 42
jornadas × 11 = 462 partidos y JORNAD2 38 × 10 = 380. No hay ninguna
referencia a un jugador que no exista.

```
fichero  = { [u16 número de jornada] partido... }
partido  = u16 local, u16 visitante        (n -> DBDAT/EQ95000n.DBC)
           u8 goles local, u8 goles visitante
           4 bytes (byte desconocido + día, mes, año: 02 09 5f = 2/9/1995;
                    alguno no sigue este patrón)
           texto árbitro, u32 público, texto estado del campo, texto tiempo,
           texto hora, texto largo crónica
           2 × bloque de equipo (local y visitante)
bloque   = u16 equipo
           12 × (u16 jugador, u16 sustituto o FFFF, u16 minuto)
               (11 titulares + 1 casilla para el suplente que a su vez es
                sustituido; si no hay, FFFF FFFF 0000)
           sucesos: (u16 tipo, u16 jugador, u16 valor) hasta tipo 0x0063
               (tipos vistos: 0x21, 0x22, 0x2C, 0x43, 0x4D…; un suceso puede
                ser de un jugador del otro equipo, p. ej. gol en propia)
           texto largo declaraciones
texto       = u8 longitud + letras (XOR 0x61)
texto largo = u8 N + (N+1) textos
```

El número de jornada solo va delante del primer partido de cada jornada. El
lector distingue los dos casos intentando leer un partido completo, en el
que los dos bloques de equipo deben coincidir con local y visitante.

### PC Fútbol 4.0

- La 4.0 salió con muy pocas jornadas jugadas: su `JORNAD1.DBC` ocupa 22 KB
  frente a 496 KB, y su `JORNAD2.DBC` 3,7 KB.
- `JORNAD2` de la 4.0 usa otra variante: «sin sustituto» es `FF 00`, y los
  jugadores van numerados 1 a 11 en vez de por puntero.
- El lector se valida solo: si más de un 2 % de los jugadores no existen en
  su equipo, descarta ese fichero y lo dice en la barra de estado, en vez de
  dar errores falsos. Resultado:
  - 4.0: JORNAD1 aceptado (388 jugadores protegidos), JORNAD2 descartado y
    0 problemas en el juego sin tocar;
  - 4.5: sin cambios (786 jugadores, 0 problemas).
- `JORNAD2` de la 4.0 es **relleno**: textos «X», 0 espectadores, ningún
  suceso, y en cada equipo los jugadores 1 a 11 con `FF 00` de sustituto.
  Esos números no son punteros de esos equipos, así que descartarlo es
  correcto. Si fueran puestos en la alineación, también están a salvo,
  porque el editor mantiene la alineación de 1 a N.
- El `PCFUTBOL.DAT` de la 4.0 tiene el mismo mensaje «NJugadorEquipo …
  Jugador no encontrado», así que la protección de JORNAD1 hace falta
  igual.
- **Probado en el juego (DOSBox, 9/11/2026):**
  - Sin Buyo, la 4.0 también se bloquea al arrancar con «Jugador no
    encontrado — Real Madrid, Índice Jugador: 1». La comprobación va antes
    de la pantalla del escudo.
  - Con Lopetegi traspasado dejando copia y un alta nueva, arranca.
    Lopetegi sale en el Deportivo y «Prueba40» en el Zaragoza, los dos como
    «Nuevo fichaje».
  - La copia del zip pide el escudo del manual. El usuario indicó pulsar
    siempre el primero de la primera fila; a veces hay que repetir porque
    la pregunta cambia.
  - El menú de la 4.0 dice «Actualizado hasta la jornada 02»: por eso sus
    JORNAD son tan pequeños.
- Comprobado sin abrir el juego:
  - las 234 fichas y equipaciones se abren;
  - EQUIPOS.DAT tiene 468 bloques (234 × 2);
  - PCMANAG y PROMANAG de la 4.0 llevan la lista ordenada de IDs que usa
    el editor para numerar los kits;
  - se encuentran las tablas de ligas y Europa;
  - hay 44 dibujos CAMISAS, con el mismo formato;
  - CSV de ida y vuelta: 0 cambios;
  - «Comprobar plantillas» en la 4.0 sin tocar: 0 problemas.
- Copias para probar: `~/pcfutbol_trabajo/juego40_original` y
  `juego40_pruebas`, con `dosbox/start40.sh FUTBOL4` y `run40.conf`.

## Textos largos (biografía y declaraciones del entrenador, comentario del jugador)

Se guardan como **líneas de pantalla ya partidas por palabras**, de 68 a 74
letras y terminadas en espacio. `\` es salto de línea y el código `0xF9`
empieza párrafo; el juego lo pinta como viñeta «•» y el editor lo muestra
como `¶`.

**Error del editor original (corregido).** Juntaba las líneas al leer y las
volvía a partir en trozos de 16 a 25 letras cortando palabras. Además perdía
el `0xF9`. Cualquier «Salvar» alteraba así 42 equipos.

**Ahora:**

- Se guarda el bloque original y, si el texto no se ha editado, se escribe
  byte a byte.
- Si se ha editado, se parte por palabras con 70 letras de ancho. El juego
  vuelve a ajustar el texto a su ventana, como se comprobó con el comentario
  de prueba de Lopetegi.

## Otras correcciones de errores que venían del editor original

- **Último nombre del club** («Atlético de Madrid», «LOGROÑES»): las letras
  acentuadas van en Latin-1 directo (é = `0xE9 ^ 0x61`), no con la tabla.
  Antes se convertían en espacio. Ahora se leen y escriben bien
  (`codificar_caracter_directo`).
- **Guardar sin cambios**: los 235 DBC de la 4.5 salen idénticos byte a
  byte; antes cambiaban 42. En la 4.0 queda un byte distinto en `EQ951053`
  (Widzew Łódź), un código raro que antes pasaba a espacio y ahora a «à».
- **Fotos y escudos**: al aceptar la ficha del jugador o del equipo se
  volvían a convertir siempre desde el botón, aunque no se hubieran tocado.
  Si se cancelaba el cuadro de abrir imagen, se metía una imagen vacía.
  Ahora solo se convierten si se ha elegido una imagen válida.
- **Añadir jugador** en la ficha del equipo: escribía el número en la
  *posición en el campo* (0-3) en vez de en el *puesto en la alineación*, y
  este quedaba sin poner. **Quitar jugador** sin selección cerraba el editor.
  Ahora quitar renumera la alineación.
- **Doble clic en la lista de jugadores** buscaba el jugador por el
  principio del nombre y podía abrir otro. Las tablas se quedaban con datos
  viejos tras editar. El doble clic en la tabla de equipos perdía los
  punteros nuevos.
- **Fecha de nacimiento 0/0** (Markovic): la ficha la cambiaba a 1/1.
- **Versión**: la 4.5 se detectaba como 4.0. La pista que usaba el editor
  (`EQ959901.DBC`) no existe; las dos versiones llaman a las Estrellas
  `EQ959900.DBC`. Ahora la 4.5 se reconoce por `INFOFUT.EXE`,
  `ACTUALIZ.EXE` o `pcFut4.exe`.
- **Importar sonidos** escribía sobre el juego sin copia. Ahora deja `.bak`.

## CSV

- Se escribe con `;` y UTF-8 con BOM. Se lee con `;`, `,` o tabulador, en
  UTF-8 o Windows-1252 (el «CSV» de Excel).
- **Celda vacía = no cambiar.** Se pueden borrar columnas enteras.
- `jugador_id` = puntero. Cambiar `equipo_id` es un traspaso; una fila con
  `jugador_id` vacío es un alta. Si falta la fila de un jugador, se pregunta
  si darlo de baja.
- Los códigos (posición, demarcación, piel, pelo, estado) admiten el número
  o el nombre. La altura se puede poner en cm o en metros («1,85»).
- Si el dorsal ya está ocupado en el equipo de destino, se pone el primero
  libre y se avisa.
- **Comprobado:** exportar e importar sin tocar nada da 0 cambios. Un CSV
  editado con traspaso, alta, baja, letras raras, una fila errónea y un
  fichero de Excel en Windows-1252 con comas da el resultado esperado.

## Pruebas

En `~/pcfutbol_trabajo/test3/`:

- `prueba_csv.cpp` (modos `comprobar`, `exportar`, `importar`, `guardar`,
  `volcar`, `dialogos`, `escudos`);
- `prueba_trasp.cpp`;
- `py/jornad4.py` (lector de JORNAD en Python).

**Cuidado:** los modos `dialogos` y `escudos`, con el código antiguo,
generaron gigas de avisos de Qt y llenaron el disco. Hay que pasar siempre
la salida por `grep` o `head`, nunca a un fichero.
