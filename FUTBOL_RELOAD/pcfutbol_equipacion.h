#ifndef PCFUTBOL_EQUIPACION_H
#define PCFUTBOL_EQUIPACION_H

/*
 * pcfutbol_equipacion.h / .cpp
 *
 * Editor de equipaciones (camisetas) para PC Fútbol 4.0/4.5.
 *
 * FORMATO DE PALETAS/EQUIPOS.DAT (verificado):
 *  - 190 bytes por equipación, 2 equipaciones por equipo (1ª y 2ª)
 *  - offset 0      : 0x90                      (marcador fijo)
 *  - offset 1-7    : "QRSTUV>" (ASCII)         (marcador fijo)
 *  - offset 8-175  : 168 bytes = textura (24 x 7), índices de PALETA.BBM
 *  - offset 176-189: 14 bytes de cierre fijo
 *  - offset del bloque = 190 * (2*(n-1) + [0=1ª,1=2ª]), n = posición 1-based
 *    del equipo entre los EQ*.DBC ordenados por ID, excluyendo EQ959900/01.
 *
 * MAPA DE SPRITE (calibrado contra el juego real, 100% resuelto):
 *  El icono de camiseta es de 20x20 y sólo muestra 37 de las 168 posiciones
 *  de la textura. La correspondencia píxel->posición está en mapaSprite[].
 *  Anatomía descubierta cruzando el mapa con los 470 kits reales:
 *    - posición   7  -> SOMBRA negra (0x60 en los 470 kits, sin excepción)
 *    - posición  16  -> CUELLO
 *    - posición  15  -> RIBETE inferior
 *    - posiciones 4,27,28,31 / 25,29,30,32 -> MANGA izquierda / derecha
 *    - posiciones 72,77 -> HOMBROS
 *    - filas 4,5,6 de la textura -> TORSO
 *    - posiciones 1,34,43,47,50,52 -> FALDÓN (parte baja)
 *
 * TABLA DE RECOLOREADO DEL PARTIDO (verificado 2026-11):
 *  El bloque de 190 bytes es en realidad una TABLA (LUT) para los sprites
 *  de jugador de GRAFICOS/*.EMP: el píxel del sprite con índice i se pinta
 *  con el color bloque[i] (paleta GRAFICOS/PALETA.BBM). Por eso:
 *    - bloque[1..6] = 0x51..0x56 ("QRSTUV") -> rampa de PIEL
 *    - bloque[7]    = 0x3E (">")             -> sombra del jugador en el césped
 *    - bloque[8..175] = la "textura": posición p de la textura = índice p+8
 *    - bloque[176..189] = índices altos, apenas usados por los sprites; los
 *      bytes 187..189 llevan además un color RGB propio de cada kit.
 *  Zonas del jugador (posiciones de textura), comprobadas pintando cada zona
 *  de un color en CORRER/CHUT/ENTRADA/CABCHI.EMP:
 *    - BOTAS    : 7, 22, 23   (0x60/0x70/0x73 en los 470 kits: botas negras)
 *    - PELO     : 14          (0x60 en los 470 kits)
 *    - MEDIAS   : 18, 19, 20
 *    - PANTALÓN : 9-13, 15, 63-71
 *    - fijos    : 8, 17, 21   (detalles negros, 0x60)
 *    - CAMISETA : todas las demás
 */

#include <QString>
#include <QByteArray>
#include <QImage>
#include <QVector>
#include <QList>

class PCFutbolEquipacion
{
public:
    enum class Patron { Liso, RayasVerticales, FranjasHorizontales };

    // Zonas anatómicas (para pintarlas de golpe).
    //  - Las primeras son zonas del ICONO de 20x20 (sólo camiseta).
    //  - Camiseta/Pantalon/Medias/Botas/Pelo son zonas del JUGADOR en el
    //    partido (ver "TABLA DE RECOLOREADO" más abajo).
    //  - Sombra se conserva por compatibilidad: es la posición 7, que en
    //    realidad es el color principal de las BOTAS (y el icono la usa
    //    además como contorno).
    enum class Zona { Sombra, Cuello, Ribete, MangaIzquierda, MangaDerecha,
                      Mangas, Hombros, Torso, Faldon, TodoLoVisible,
                      Camiseta, Pantalon, Medias, Botas, Pelo };

    struct ColorEquipacion {
        QString nombre;
        QVector<quint8> rampa; // índices de paleta, de oscuro a claro
    };

    // Colores validados contra la paleta real de PALETA.BBM
    static QList<ColorEquipacion> coloresDisponibles();

    // --- Generación de texturas ---------------------------------------------
    // Genera los 168 bytes de textura a partir de un patrón y 1-2 colores.
    // Si preservarSombra es true (recomendado), la posición 7 se fuerza a 0x60
    // para que la camiseta conserve su contorno negro como en el juego.
    static QByteArray generarTextura(Patron patron,
                                      const ColorEquipacion &colorA,
                                      const ColorEquipacion &colorB,
                                      int anchoRaya = 4,
                                      bool preservarSombra = true);

    static QByteArray generarBloqueEquipacion(Patron patron,
                                               const ColorEquipacion &colorA,
                                               const ColorEquipacion &colorB,
                                               int anchoRaya = 4);

    // Envuelve una textura de 168 bytes en su bloque de 190
    static QByteArray bloqueDesdeTextura(const QByteArray &textura168);

    // --- Edición a nivel de píxel -------------------------------------------
    // Posición (0-167) de la textura que el juego pinta en el píxel (sx,sy)
    // del sprite de 20x20. Devuelve -1 si ese píxel es fondo.
    static int posicionTextura(int sx, int sy);

    // Pinta el píxel (sx,sy): escribe idxPaleta en la posición de textura
    // correspondiente. Si protegerSombra es true no toca la posición 7.
    // Devuelve true si la textura ha cambiado.
    static bool pintarPixel(QByteArray &textura168, int sx, int sy,
                            quint8 idxPaleta, bool protegerSombra = true);

    // Índice de paleta que hay actualmente en el píxel (sx,sy). -1 si es fondo.
    static int colorEnPixel(const QByteArray &textura168, int sx, int sy);

    // Posiciones de textura (0-167) que forman una zona anatómica
    static QVector<int> posicionesZona(Zona zona);

    // Rellena una zona entera con un índice de paleta (respeta la sombra)
    static void pintarZona(QByteArray &textura168, Zona zona, quint8 idxPaleta,
                           bool protegerSombra = true);

    // Fuerza el contorno negro del juego
    static void restaurarSombra(QByteArray &textura168);

    // --- Zonas del jugador en el partido ---------------------------------
    // true si la posición pertenece a la camiseta (no a pantalón, medias,
    // botas, pelo ni a los detalles negros fijos).
    static bool esPosicionCamiseta(int pos);

    // Copia en "destino" sólo las posiciones de camiseta de "origen".
    // Sirve para aplicar un patrón sin pisar pantalón, medias ni botas.
    static void copiarSoloCamiseta(QByteArray &destino, const QByteArray &origen);

    // Rellena una zona con una rampa de color conservando el sombreado:
    // cada posición recibe un tono más claro u oscuro según lo claro u
    // oscuro que fuera su color anterior dentro de la zona.
    static void pintarZonaRampa(QByteArray &textura168, Zona zona,
                                const QVector<quint8> &rampa);

    // Rampa (de coloresDisponibles) que contiene el índice; vacía si ninguna.
    static QVector<quint8> rampaDeIndice(quint8 idx);

    // Tabla de recoloreado de 190 entradas (cabecera + textura + cierre)
    // para pintar los sprites del partido. "cierre" puede venir vacío.
    static QByteArray tablaRecoloreado(const QByteArray &textura168,
                                       const QByteArray &bloqueOriginal = QByteArray());

    // Escribe SÓLO los 168 bytes de textura de un kit (offset+8), sin tocar
    // la cabecera ni los 14 bytes finales, que varían por equipo.
    static bool escribirTexturaEquipacion(const QString &equiposDatPath, int n, bool primera,
                                          const QByteArray &textura168);

    // --- Vista previa --------------------------------------------------------
    // Dibuja el sprite real de 20x20 ampliado x escala (nearest-neighbor).
    static QImage previsualizar(const QByteArray &textura168, int escala = 10);

    // --- Lectura/escritura directa de PALETAS/EQUIPOS.DAT --------------------
    static int kitOffset(int n, bool primera);
    static QByteArray leerBloqueEquipacion(const QString &equiposDatPath, int n, bool primera, bool *ok = nullptr);
    static bool escribirBloqueEquipacion(const QString &equiposDatPath, int n, bool primera, const QByteArray &bloque190);

    static const int ANCHO = 24;
    static const int ALTO = 7;
    static const int TAM_TEXTURA = 168; // ANCHO*ALTO
    static const int TAM_BLOQUE = 190;

    static const int SPRITE_W = 20;
    static const int SPRITE_H = 20;

    static const int POS_SOMBRA = 7;   // = color principal de las BOTAS
    static const quint8 COLOR_SOMBRA = 0x60;

private:
    static QByteArray cabeceraFija();
    static QByteArray cierreFijo();
};

#endif // PCFUTBOL_EQUIPACION_H
