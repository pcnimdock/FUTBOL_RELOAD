#ifndef PCFUTBOL_EXTRAS_H
#define PCFUTBOL_EXTRAS_H

/*
 * pcfutbol_extras.h / .cpp
 *
 * Los formatos de PC Fútbol 4.5 que faltaban por soportar en el editor.
 * Se engancha a los mismos botones "Exportar imágenes" / "Importar imágenes".
 *
 * ---------------------------------------------------------------------------
 * 1) SPRITES .EMP  (GRAFICOS/*.EMP — 155 ficheros, 5.100+ frames)
 * ---------------------------------------------------------------------------
 *   Cabecera (16 bytes):
 *     +0 u8   nº de entradas de la tabla de animación
 *     +1 u8   0x41 ('A') = firma. 0x00 en la variante antigua (SENTRAPO.EMP),
 *             que usa descriptores de 6 bytes en vez de 8.
 *     +2 u16  nº de secuencias
 *     +8 u16  tamaño del fichero en párrafos (x16)
 *   Tabla de animación: N entradas de 4 bytes (no hace falta para los píxeles).
 *   Frames consecutivos, cada uno con descriptor de 8 bytes:
 *     +0 u16  bytes por fila + 1   -> ancho = 4 * (valor - 1)
 *     +2 u16  alto
 *     +4 u16  tamaño de un plano   (= bytesPorFila * alto)
 *     +6 u16  0
 *     +8 píxeles: 4 PLANOS Mode-X consecutivos.
 *        pixel(x,y) = datos[(x % 4) * tamPlano + y * bytesPorFila + (x / 4)]
 *   Índice 0 de la paleta = transparente.
 *
 * ---------------------------------------------------------------------------
 * 2) FONDO POR TILES  (GRAFICOS/FON_*.BLO + GRAFICOS/MAP_*.BIN)
 * ---------------------------------------------------------------------------
 *   .BLO: tileset. Tiles de 16x16 px, 256 bytes cada uno, sin cabecera,
 *         en el mismo formato planar Mode-X que los .EMP.
 *   .BIN: mapa de 128 x 48 tiles (= 2048 x 768 px), 6144 entradas u16:
 *         bits 0-14 = índice del tile ; bit 15 = espejo horizontal.
 *
 * ---------------------------------------------------------------------------
 * 3) TEXTOS DE INTERFAZ  (*.GRC — "GFX Resource file", Dinamic Multimedia)
 * ---------------------------------------------------------------------------
 *   Cabecera ASCII terminada en 0x1A. Después, objetos de interfaz; los que
 *   llevan texto van precedidos de 12 bytes (6 x u16):
 *     tipo, param1, param2, x, y, longitud   y luego el texto CIFRADO.
 *
 *   CIFRADO:  claro[i] = cifrado[i] XOR ((0xC0 + longitud + i) & 0xFF)
 *
 *   Al importar, el texto nuevo se rellena con espacios o se recorta a la
 *   longitud original: así el fichero conserva su tamaño y todos los offsets
 *   siguen siendo válidos.
 *
 * ---------------------------------------------------------------------------
 * 4) SONIDOS  (SONIDOS/*.RAW)
 * ---------------------------------------------------------------------------
 *   PCM crudo: 8 bits SIN signo, mono, sin cabecera (el silencio es 0x80).
 *   Frecuencia: 11.025 Hz. No viene escrita en ningún sitio (el ejecutable
 *   calcula la constante de tiempo del DSP al vuelo), se dedujo del contenido:
 *   el pico espectral de SILBATO.RAW y SILBATOF.RAW cae en 0,296-0,304
 *   ciclos/muestra, que a 11.025 Hz son 3.269 y 3.353 Hz — justo el tono de un
 *   silbato de árbitro. A 8.000 Hz sonaría demasiado grave y a 22.050 Hz
 *   demasiado agudo.
 *
 *   Al exportar se envuelven en un WAV (sin tocar las muestras). Al importar
 *   se acepta cualquier WAV PCM (8 o 16 bits, mono o estéreo, cualquier
 *   frecuencia): se pasa a mono, se remuestrea a 11.025 Hz y se guarda a 8
 *   bits sin signo.
 */

#include <QString>
#include <functional>
#include <QStringList>
#include <QByteArray>
#include <QImage>
#include <QVector>
#include <QRgb>

class PCFutbolExtras
{
public:
    struct Resultado {
        int spritesExportados = 0;   // frames .EMP
        int spritesImportados = 0;
        int fondosExportados = 0;    // FON_*.BLO + MAP_*.BIN
        int fondosImportados = 0;
        int textosExportados = 0;    // cadenas .GRC
        int textosImportados = 0;
        int sonidosExportados = 0;   // SONIDOS/*.RAW
        int sonidosImportados = 0;
        QStringList errores;
    };

    // --- Paleta ------------------------------------------------------------
    // Lee un PALETA.BBM (IFF, chunk CMAP). OJO: GRAFICOS/PALETA.BBM y
    // PALETAS/PALETA.BBM NO son la misma paleta.
    static QVector<QRgb> leerPaletaBBM(const QString &ruta, bool *ok = nullptr);

    // --- Sprites .EMP ------------------------------------------------------
    struct FrameEMP {
        int offset = 0;     // offset del descriptor
        int datos = 0;      // offset de los píxeles
        int ancho = 0;
        int alto = 0;
        int bytesPorFila = 0;
        int tamPlano = 0;
    };
    static QVector<FrameEMP> parsearEMP(const QByteArray &d);
    static QImage decodificarFrame(const QByteArray &d, const FrameEMP &f,
                                   const QVector<QRgb> &paleta);
    static bool codificarFrame(QByteArray &d, const FrameEMP &f, const QImage &img,
                               const QVector<QRgb> &paleta);

    // --- Fondo por tiles ---------------------------------------------------
    static const int MAPA_ANCHO = 128;
    static const int MAPA_ALTO = 48;
    static const int TILE = 16;
    static QImage montarFondo(const QByteArray &blo, const QByteArray &mapa,
                              const QVector<QRgb> &paleta);
    // Reconstruye tileset + mapa a partir de la imagen. Reaprovecha tiles
    // repetidos y espejados igual que el original.
    static bool desmontarFondo(const QImage &img, const QVector<QRgb> &paleta,
                               int tilesOriginales, QByteArray *blo, QByteArray *mapa,
                               QString *aviso);

    // --- Textos .GRC -------------------------------------------------------
    struct TextoGRC {
        int offset = 0;   // offset del texto cifrado
        int tipo = 0;
        int x = 0, y = 0;
        int longitud = 0;
        QString texto;
    };
    static QVector<TextoGRC> leerTextosGRC(const QByteArray &d);
    static void escribirTextoGRC(QByteArray &d, const TextoGRC &t, const QString &nuevo);

    // --- Sonidos .RAW ------------------------------------------------------
    static const int FRECUENCIA_RAW = 11025;
    static QByteArray rawAWav(const QByteArray &raw, int frecuencia = FRECUENCIA_RAW);
    // Devuelve las muestras a 8 bits sin signo y 11.025 Hz, listas para el .RAW
    static QByteArray wavARaw(const QByteArray &wav, QString *error);

    static void exportarSonidos(const QString &carpetaJuego, const QString &carpetaSalida,
                                Resultado &r);
    static void importarSonidos(const QString &carpetaNueva, const QString &carpetaSonidos,
                                Resultado &r);

    // --- Orquestación (lo que llaman los dos botones) ----------------------
    // 'avance' recibe (hechos, total, detalle) y devuelve false para cancelar.
    using CallbackProgreso = std::function<bool(int hechos, int total,
                                                 const QString &detalle)>;

    static void exportarTodo(const QString &carpetaJuego, const QString &carpetaSalida,
                             Resultado &r, CallbackProgreso avance = nullptr);
    static void importarTodo(const QString &carpetaNueva, const QString &carpetaImagenes,
                             Resultado &r, CallbackProgreso avance = nullptr);

private:
    static int indiceMasCercano(QRgb c, const QVector<QRgb> &paleta);
};

#endif // PCFUTBOL_EXTRAS_H
