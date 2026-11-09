#ifndef PCFUTBOL_IMAGENES_H
#define PCFUTBOL_IMAGENES_H

/*
 * pcfutbol_imagenes.h / .cpp
 *
 * Port a C++/Qt de las herramientas Python "extraer_imagenes.py" /
 * "reinyectar_imagenes.py" / "pcfutbol_gfx.py" desarrolladas para PC
 * Fútbol 4.0/4.5. Ver PCFUTBOL_formato_tecnico.md para el detalle de
 * cada formato.
 *
 * Uso típico desde MainWindow:
 *
 *   PCFutbolImagenes::ResultadoExportacion r =
 *       PCFutbolImagenes::exportarImagenes(carpetaJuego, carpetaSalida);
 *
 *   PCFutbolImagenes::ResultadoImportacion r2 =
 *       PCFutbolImagenes::importarImagenes(carpetaOriginal, carpetaImagenes, carpetaNueva);
 */

#include <QString>
#include <QStringList>
#include <QImage>
#include <QByteArray>
#include <QVector>
#include <QRgb>
#include <QPair>
#include <functional>

class PCFutbolImagenes
{
public:
    // Callback de progreso. Se llama con (hechos, total, detalle). Si devuelve
    // false, la operación se aborta de forma limpia (deja lo ya escrito y
    // añade un aviso "cancelado" al resultado). Puede ser nullptr.
    using CallbackProgreso = std::function<bool(int hechos, int total,
                                                 const QString &detalle)>;
    // --- Resultados ---------------------------------------------------
    struct ResultadoExportacion {
        int ficherosEscaneados = 0;
        int ficherosConImagen = 0;
        int imagenesExtraidas = 0;
        QString carpetaSalida;
        QString manifestPath;
        QStringList errores;
    };

    struct ResultadoImportacion {
        int ficherosReescritos = 0;
        int imagenesReinyectadas = 0;
        QString carpetaNueva;
        QStringList errores;
    };

    // --- Punto de entrada principal (para los dos botones) ------------
    static ResultadoExportacion exportarImagenes(const QString &carpetaJuego,
                                                  const QString &carpetaSalida,
                                                  CallbackProgreso avance = nullptr);

    static ResultadoImportacion importarImagenes(const QString &carpetaOriginal,
                                                  const QString &carpetaImagenes,
                                                  const QString &carpetaNueva,
                                                  CallbackProgreso avance = nullptr);

    // --- Clasificación de ficheros --------------------------------------
    enum class Familia { A, B, C, Ninguna };
    static Familia clasificarFichero(const QString &rutaRelativa);

    // --- Familia A (cabecera corta, paleta fija externa) ----------------
    struct ImagenEncontrada { int offset; int ancho; int alto; };
    static QVector<ImagenEncontrada> buscarImagenesFamiliaA(const QByteArray &data,
                                                             int minDim = 4, int maxDim = 700);
    static QImage decodeFamiliaA(const QByteArray &data, int offset, int ancho, int alto,
                                  const QVector<QRgb> &paleta);
    static QByteArray encodeFamiliaA(const QImage &imagen, int ancho, int alto,
                                      const QVector<QRgb> &paleta);

    // Formato mixto (MINIFOTO/MINIESC): puede ser familia A o B según tamaño
    struct FormatoDetectado { Familia familia; int offset; int ancho; int alto; bool valido; };
    static FormatoDetectado detectarFormatoUnaImagen(const QByteArray &data, int wFallback, int hFallback);

    // --- Familia B (cabecera larga, paleta local embebida) --------------
    static bool leerDimensionesFamiliaB(const QByteArray &data, int offset, int *ancho, int *alto);
    static QImage decodeFamiliaB(const QByteArray &data, int ancho, int alto, int offset = 0);
    static QByteArray encodeFamiliaB(const QImage &imagen, int ancho, int alto);

    // --- Familia C (ESCUDOS, entrelazado por columnas) -------------------
    static const int ESCUDOS_ANCHO = 52;
    static const int ESCUDOS_ALTO = 65;
    static const int ESCUDOS_CABECERA = 0x18;
    static const int ESCUDOS_CIERRE = 4;
    static QImage decodeEscudosSim(const QByteArray &data, int offset = 0);
    static QByteArray encodeEscudosSim(const QImage &imagen, const QByteArray &cabeceraOriginal);

    // --- Paletas REALES embebidas en los GFX*.DAT ------------------------
    // Los GFX llevan su propia tabla de paletas de 16 colores. Cada registro:
    //     00 10  +  16 x RGB (48 bytes)  +  FF
    // Los valores son VGA de 6 bits expandidos a 8 (v*4|3), asi que todos los
    // componentes cumplen v==0 || v%4==3: es la prueba que distingue una
    // paleta de verdad de una coincidencia dentro de los datos de una imagen.
    // El fichero NO dice que paleta usa cada imagen (lo elige el codigo del
    // juego), por eso el indice viaja en el manifest y se puede editar.
    static QVector<QVector<QRgb>> leerPaletasGFX(const QByteArray &data);
    static bool esFicheroGFX(const QString &nombreFichero);

    // --- Paletas conocidas ----------------------------------------------
    static QVector<QRgb> paletaMinifoto();
    static QVector<QRgb> paletaMiniesc();
    static QVector<QRgb> paletaEscudosSim();       // 256 colores
    static QVector<QRgb> paletaBannerCompartidaGFX02(); // 16 colores, offsets 380000-530000 de GFX02.DAT

    // Excepciones de paleta por (nombreFichero, offset) o por rango.
    // Devuelve la paleta a usar y una etiqueta para guardar en el manifest.
    static QPair<QVector<QRgb>, QString> obtenerPaletaFamiliaA(const QString &nombreFichero, int offset);
    static QVector<QRgb> paletaPorEtiqueta(const QString &etiqueta);

    // Versiones que miran la tabla de paletas embebida en el propio fichero.
    static QPair<QVector<QRgb>, QString> obtenerPaletaFamiliaA(const QString &nombreFichero, int offset,
                                                               const QByteArray &data);
    static QVector<QRgb> paletaPorEtiqueta(const QString &etiqueta, const QByteArray &data);
    // Con "equipo:<id>" (CAMISAS y cintas) hace falta la carpeta del juego
    static QVector<QRgb> paletaPorEtiqueta(const QString &etiqueta, const QByteArray &data,
                                           const QString &carpetaJuego);
    static QVector<QRgb> paletaEquipo(const QString &carpetaJuego, const QString &id);
    // Paleta de cada imagen de GFX05 ("pal:FICHERO:offset") o vacío si no se sabe
    static QString etiquetaGFX05(int offset, int tamFichero);

private:
    static QByteArray leerFichero(const QString &ruta, bool *ok = nullptr);
    static bool escribirFichero(const QString &ruta, const QByteArray &data);
    static QImage cuantizarAPaleta(const QImage &origen, int ancho, int alto, const QVector<QRgb> &paleta);
};

#endif // PCFUTBOL_IMAGENES_H
