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

    // Zonas anatómicas de la camiseta (para pintarlas de golpe)
    enum class Zona { Sombra, Cuello, Ribete, MangaIzquierda, MangaDerecha,
                      Mangas, Hombros, Torso, Faldon, TodoLoVisible };

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

    static const int POS_SOMBRA = 7;
    static const quint8 COLOR_SOMBRA = 0x60;

private:
    static QByteArray cabeceraFija();
    static QByteArray cierreFijo();
};

#endif // PCFUTBOL_EQUIPACION_H
