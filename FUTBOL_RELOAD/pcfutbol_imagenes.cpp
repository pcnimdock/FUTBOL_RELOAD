#include "pcfutbol_imagenes.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QObject>
#include <QTextStream>
#include <QDebug>
#include <QMap>
#include <QSet>
#include <cmath>

// ==========================================================================
// Clasificación de ficheros
// ==========================================================================

PCFutbolImagenes::Familia PCFutbolImagenes::clasificarFichero(const QString &rutaRelativa)
{
    const QString up = rutaRelativa.toUpper();
    if (up.contains("ESCUDOS") && !up.contains("DBDAT"))
        return Familia::C;
    static const QStringList carpetasB = {"BIGFOTO", "BIGENTR", "BIGESC", "BIGCAMP"};
    for (const QString &clave : carpetasB) {
        if (up.contains(clave))
            return Familia::B;
    }
    return Familia::A;
}

// ==========================================================================
// Familia A -- cabecera corta (8 bytes), sin paleta embebida
//   offset 0   : 0x01
//   offset 1-2 : ancho (u16 LE)
//   offset 3-4 : alto (u16 LE)
//   offset 5-7 : 0x04 0x00 0x00 (4 planos, reservado)
//   offset 8-. : pixeles, 4 planos entrelazados por fila
// ==========================================================================

QVector<PCFutbolImagenes::ImagenEncontrada> PCFutbolImagenes::buscarImagenesFamiliaA(
    const QByteArray &data, int minDim, int maxDim)
{
    QVector<ImagenEncontrada> resultados;
    const int n = data.size();
    const unsigned char *d = reinterpret_cast<const unsigned char*>(data.constData());

    int i = 0;
    while (i < n - 8) {
        if (d[i] == 0x01 && d[i+5] == 0x04 && d[i+6] == 0 && d[i+7] == 0) {
            int w = d[i+1] | (d[i+2] << 8);
            int h = d[i+3] | (d[i+4] << 8);
            if (w >= minDim && w <= maxDim && h >= minDim && h <= maxDim) {
                int rowbytes = (w + 7) / 8;
                int needed = rowbytes * h * 4;
                if (i + 8 + needed <= n) {
                    resultados.append({i, w, h});
                    i += 8 + needed;
                    continue;
                }
            }
        }
        i++;
    }
    return resultados;
}

QImage PCFutbolImagenes::decodeFamiliaA(const QByteArray &data, int offset, int ancho, int alto,
                                        const QVector<QRgb> &paleta)
{
    QImage img(ancho, alto, QImage::Format_Indexed8);
    QVector<QRgb> tabla = paleta;
    while (tabla.size() < 16) tabla.append(qRgb(0, 0, 0));
    img.setColorTable(tabla);

    const int rowbytes = (ancho + 7) / 8;
    const unsigned char *d = reinterpret_cast<const unsigned char*>(data.constData()) + offset + 8;

    int pos = 0;
    for (int y = 0; y < alto; y++) {
        const unsigned char *planos[4];
        for (int p = 0; p < 4; p++) {
            planos[p] = d + pos;
            pos += rowbytes;
        }
        for (int x = 0; x < ancho; x++) {
            int byteI = x / 8;
            int bitI = 7 - (x % 8);
            int idx = 0;
            for (int p = 0; p < 4; p++) {
                idx |= ((planos[p][byteI] >> bitI) & 1) << p;
            }
            img.setPixel(x, y, idx);
        }
    }
    return img;
}

// Cuantiza una imagen arbitraria a una paleta fija de hasta 16 colores
// (vecino más próximo en espacio RGB, sin dithering).
QImage PCFutbolImagenes::cuantizarAPaleta(const QImage &origen, int ancho, int alto,
                                          const QVector<QRgb> &paleta)
{
    QImage escalada = origen.convertToFormat(QImage::Format_RGB32)
                             .scaled(ancho, alto, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage salida(ancho, alto, QImage::Format_Indexed8);
    QVector<QRgb> tabla = paleta;
    while (tabla.size() < 16) tabla.append(qRgb(0, 0, 0));
    salida.setColorTable(tabla);

    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            QRgb px = escalada.pixel(x, y);
            int r = qRed(px), g = qGreen(px), b = qBlue(px);
            int mejorIdx = 0;
            long mejorDist = -1;
            for (int i = 0; i < tabla.size() && i < 16; i++) {
                int dr = r - qRed(tabla[i]);
                int dg = g - qGreen(tabla[i]);
                int db = b - qBlue(tabla[i]);
                long dist = long(dr)*dr + long(dg)*dg + long(db)*db;
                if (mejorDist < 0 || dist < mejorDist) {
                    mejorDist = dist;
                    mejorIdx = i;
                }
            }
            salida.setPixel(x, y, mejorIdx);
        }
    }
    return salida;
}

QByteArray PCFutbolImagenes::encodeFamiliaA(const QImage &imagen, int ancho, int alto,
                                            const QVector<QRgb> &paleta)
{
    QImage quant = cuantizarAPaleta(imagen, ancho, alto, paleta);
    const int rowbytes = (ancho + 7) / 8;
    QByteArray salida;
    salida.resize(rowbytes * alto * 4);
    salida.fill(0);
    unsigned char *out = reinterpret_cast<unsigned char*>(salida.data());

    int pos = 0;
    for (int y = 0; y < alto; y++) {
        unsigned char *planos[4];
        for (int p = 0; p < 4; p++) {
            planos[p] = out + pos;
            pos += rowbytes;
        }
        for (int x = 0; x < ancho; x++) {
            int idx = quant.pixelIndex(x, y);
            int byteI = x / 8;
            int bitI = 7 - (x % 8);
            for (int p = 0; p < 4; p++) {
                if ((idx >> p) & 1)
                    planos[p][byteI] |= (1 << bitI);
            }
        }
    }
    return salida;
}

// --- Formato mixto MINIFOTO/MINIESC --------------------------------------

PCFutbolImagenes::FormatoDetectado PCFutbolImagenes::detectarFormatoUnaImagen(
    const QByteArray &data, int wFallback, int hFallback)
{
    FormatoDetectado r{Familia::Ninguna, 0, 0, 0, false};
    if (data.size() < 8) return r;
    const unsigned char *d = reinterpret_cast<const unsigned char*>(data.constData());

    if (d[0] == 0x01 && d[5] == 0x04 && d[6] == 0 && d[7] == 0) {
        r.familia = Familia::A;
        r.offset = 0;
        r.ancho = d[1] | (d[2] << 8);
        r.alto = d[3] | (d[4] << 8);
        r.valido = true;
        return r;
    }
    if (d[0] == 0x00 && d[1] == 0x10) {
        int w, h;
        if (leerDimensionesFamiliaB(data, 0, &w, &h)) {
            r.ancho = w; r.alto = h;
        } else {
            r.ancho = wFallback; r.alto = hFallback;
        }
        r.familia = Familia::B;
        r.offset = 0;
        r.valido = true;
        return r;
    }
    return r;
}

// ==========================================================================
// Familia B -- cabecera larga (58 bytes), paleta local embebida
//   offset 0-1   : 2 bytes fijos (0x00 0x10)
//   offset 2-49  : paleta local de 16 colores x 3 bytes RGB (48 bytes)
//   offset 50-57 : mini-cabecera "01 w w h h 04 00 00" (¡preservar al escribir!)
//   offset 58-.  : pixeles, 4 planos entrelazados por fila
// ==========================================================================

bool PCFutbolImagenes::leerDimensionesFamiliaB(const QByteArray &data, int offset, int *ancho, int *alto)
{
    if (data.size() < offset + 58) return false;
    const unsigned char *d = reinterpret_cast<const unsigned char*>(data.constData()) + offset + 50;
    if (d[0] != 0x01) return false;
    *ancho = d[1] | (d[2] << 8);
    *alto = d[3] | (d[4] << 8);
    return true;
}

QImage PCFutbolImagenes::decodeFamiliaB(const QByteArray &data, int ancho, int alto, int offset)
{
    QImage img(ancho, alto, QImage::Format_Indexed8);
    QVector<QRgb> tabla;
    const unsigned char *pal = reinterpret_cast<const unsigned char*>(data.constData()) + offset + 2;
    for (int i = 0; i < 16; i++) {
        tabla.append(qRgb(pal[i*3], pal[i*3+1], pal[i*3+2]));
    }
    img.setColorTable(tabla);

    const int rowbytes = (ancho + 7) / 8;
    const unsigned char *d = reinterpret_cast<const unsigned char*>(data.constData()) + offset + 58;

    int pos = 0;
    for (int y = 0; y < alto; y++) {
        const unsigned char *planos[4];
        for (int p = 0; p < 4; p++) {
            planos[p] = d + pos;
            pos += rowbytes;
        }
        for (int x = 0; x < ancho; x++) {
            int byteI = x / 8;
            int bitI = 7 - (x % 8);
            int idx = 0;
            for (int p = 0; p < 4; p++) {
                idx |= ((planos[p][byteI] >> bitI) & 1) << p;
            }
            img.setPixel(x, y, idx);
        }
    }
    return img;
}

QByteArray PCFutbolImagenes::encodeFamiliaB(const QImage &imagen, int ancho, int alto)
{
    // Derivar una paleta local de 16 colores a partir de la imagen de entrada
    // (cuantización simple: usamos QImage::convertToFormat con
    // Format_Indexed8 y su algoritmo de cuantización por defecto de Qt).
    QImage escalada = imagen.convertToFormat(QImage::Format_RGB32)
                             .scaled(ancho, alto, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage quant = escalada.convertToFormat(QImage::Format_Indexed8);
    QVector<QRgb> tabla = quant.colorTable();
    while (tabla.size() < 16) tabla.append(qRgb(0, 0, 0));
    if (tabla.size() > 16) tabla.resize(16); // Qt puede generar mas de 16; nos quedamos con las 16 primeras

    // Recuantizar contra esas 16 exactas para que pixelIndex() sea 0-15 real
    QImage final16(ancho, alto, QImage::Format_Indexed8);
    final16.setColorTable(tabla);
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            QRgb px = escalada.pixel(x, y);
            int r = qRed(px), g = qGreen(px), b = qBlue(px);
            int mejorIdx = 0; long mejorDist = -1;
            for (int i = 0; i < tabla.size(); i++) {
                int dr = r-qRed(tabla[i]), dg = g-qGreen(tabla[i]), db = b-qBlue(tabla[i]);
                long dist = long(dr)*dr+long(dg)*dg+long(db)*db;
                if (mejorDist<0 || dist<mejorDist) { mejorDist=dist; mejorIdx=i; }
            }
            final16.setPixel(x, y, mejorIdx);
        }
    }

    QByteArray salida;
    salida.append(char(0x00));
    salida.append(char(0x10));
    for (int i = 0; i < 16; i++) {
        salida.append(char(qRed(tabla[i])));
        salida.append(char(qGreen(tabla[i])));
        salida.append(char(qBlue(tabla[i])));
    }
    // Mini-cabecera: "01 ancho_lo ancho_hi alto_lo alto_hi 04 00 00"
    // (¡IMPORTANTE! ver PCFUTBOL_formato_tecnico.md seccion 4bis -- si esto
    // se pone a cero el fichero queda corrupto para el juego)
    salida.append(char(0x01));
    salida.append(char(ancho & 0xFF));
    salida.append(char((ancho >> 8) & 0xFF));
    salida.append(char(alto & 0xFF));
    salida.append(char((alto >> 8) & 0xFF));
    salida.append(char(0x04));
    salida.append(char(0x00));
    salida.append(char(0x00));

    const int rowbytes = (ancho + 7) / 8;
    QByteArray cuerpo;
    cuerpo.resize(rowbytes * alto * 4);
    cuerpo.fill(0);
    unsigned char *out = reinterpret_cast<unsigned char*>(cuerpo.data());
    int pos = 0;
    for (int y = 0; y < alto; y++) {
        unsigned char *planos[4];
        for (int p = 0; p < 4; p++) { planos[p] = out + pos; pos += rowbytes; }
        for (int x = 0; x < ancho; x++) {
            int idx = final16.pixelIndex(x, y);
            int byteI = x/8, bitI = 7-(x%8);
            for (int p = 0; p < 4; p++) {
                if ((idx >> p) & 1) planos[p][byteI] |= (1 << bitI);
            }
        }
    }
    salida.append(cuerpo);
    salida.append(char(0xFF)); // byte de cierre, igual que en los originales
    return salida;
}

// ==========================================================================
// Familia C -- .../ESCUDOS/*.EMP (escudo del simulador de partidos)
//   offset 0-23  : cabecera de 24 bytes (0x18), preservar tal cual
//   offset 24-.. : pixeles, tamano FIJO 52x65, paleta directa de 256
//                  colores propia, en 4 PASADAS entrelazadas por columna
//   cierre       : 4 bytes a 0x00
// ==========================================================================

QImage PCFutbolImagenes::decodeEscudosSim(const QByteArray &data, int offset)
{
    const int w = ESCUDOS_ANCHO, h = ESCUDOS_ALTO;
    QImage img(w, h, QImage::Format_Indexed8);
    QVector<QRgb> tabla = paletaEscudosSim();
    img.setColorTable(tabla);

    const unsigned char *d = reinterpret_cast<const unsigned char*>(data.constData());
    int pos = offset + ESCUDOS_CABECERA;

    for (int pasada = 0; pasada < 4; pasada++) {
        for (int fila = 0; fila < h; fila++) {
            int columna = pasada;
            while (columna < w) {
                img.setPixel(columna, fila, d[pos]);
                pos++;
                columna += 4;
            }
        }
    }
    return img;
}

QByteArray PCFutbolImagenes::encodeEscudosSim(const QImage &imagen, const QByteArray &cabeceraOriginal)
{
    const int w = ESCUDOS_ANCHO, h = ESCUDOS_ALTO;
    QVector<QRgb> paleta = paletaEscudosSim();

    QImage escalada = imagen.convertToFormat(QImage::Format_RGB32)
                             .scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QByteArray salida;
    salida.append(cabeceraOriginal.left(ESCUDOS_CABECERA));
    while (salida.size() < ESCUDOS_CABECERA) salida.append(char(0));

    for (int pasada = 0; pasada < 4; pasada++) {
        for (int fila = 0; fila < h; fila++) {
            int columna = pasada;
            while (columna < w) {
                QRgb px = escalada.pixel(columna, fila);
                int r = qRed(px), g = qGreen(px), b = qBlue(px);
                int mejorIdx = 0; long mejorDist = -1;
                for (int i = 0; i < paleta.size(); i++) {
                    int dr = r-qRed(paleta[i]), dg = g-qGreen(paleta[i]), db = b-qBlue(paleta[i]);
                    long dist = long(dr)*dr+long(dg)*dg+long(db)*db;
                    if (mejorDist<0 || dist<mejorDist) { mejorDist=dist; mejorIdx=i; }
                }
                salida.append(char(mejorIdx));
                columna += 4;
            }
        }
    }
    for (int i = 0; i < ESCUDOS_CIERRE; i++) salida.append(char(0));
    return salida;
}

// ==========================================================================
// Paletas
// ==========================================================================

QVector<QRgb> PCFutbolImagenes::paletaMinifoto()
{
    static const QVector<QRgb> pal = {
        qRgb(0,0,0), qRgb(0x18,0x6d,0x14), qRgb(0x86,0xaa,0x1c), qRgb(0xa6,0xb6,0xcb),
        qRgb(0xf7,0xdb,0x18), qRgb(0x00,0x10,0xe3), qRgb(0xf7,0x00,0x00), qRgb(0x61,0x8e,0xb6),
        qRgb(0x59,0x24,0x00), qRgb(0x9a,0x61,0x24), qRgb(0xdb,0x92,0x24), qRgb(0xff,0xd7,0x86),
        qRgb(0x69,0x69,0x69), qRgb(0x9a,0x9a,0x9a), qRgb(0xc7,0xc7,0xc7), qRgb(0xf7,0xf7,0xf7)
    };
    return pal;
}

QVector<QRgb> PCFutbolImagenes::paletaMiniesc()
{
    static const QVector<QRgb> pal = {
        qRgb(0,0,0), qRgb(0x18,0x6d,0x14), qRgb(0x86,0xaa,0x1c), qRgb(0xae,0xbe,0xd3),
        qRgb(0xff,0xe3,0x20), qRgb(0x00,0x00,0xff), qRgb(0xef,0x20,0x00), qRgb(0x69,0x96,0xbe),
        qRgb(0x6d,0xa6,0xaa), qRgb(0x00,0x49,0x75), qRgb(0xa2,0xd3,0x24), qRgb(0x00,0x59,0x61),
        qRgb(0x71,0x71,0x71), qRgb(0xa2,0xa2,0xa2), qRgb(0xcf,0xcf,0xcf), qRgb(0xff,0xff,0xff)
    };
    return pal;
}

QVector<QRgb> PCFutbolImagenes::paletaBannerCompartidaGFX02()
{
    // Paleta compartida en GFX02.DAT offsets 380000-530000 (banner "PC
    // FUTBOL", logo Dinamic Multimedia y varias imagenes mas de esa zona)
    // -- inferida por votacion contra capturas reales, ver seccion
    // 4quater de PCFUTBOL_formato_tecnico.md
    // Es EXACTAMENTE la paleta numero 8 de la tabla embebida en GFX02.DAT.
    // La version anterior estaba inferida por votacion y tenia un error de
    // +-2 por canal, porque los valores reales son VGA de 6 bits expandidos
    // (v*4|3): 151 y no 150, 199 y no 199... 
    static const QVector<QRgb> pal = {
        qRgb(0,0,0),     qRgb(151,99,0),   qRgb(199,143,0), qRgb(247,183,0),
        qRgb(255,207,11), qRgb(255,239,19), qRgb(103,151,219), qRgb(223,23,0),
        qRgb(15,63,215), qRgb(39,79,0),    qRgb(91,127,0),  qRgb(67,67,67),
        qRgb(107,107,107), qRgb(147,147,147), qRgb(203,203,203), qRgb(255,255,255)
    };
    return pal;
}

QVector<QRgb> PCFutbolImagenes::paletaEscudosSim()
{
    static const QVector<QRgb> pal = {
    qRgb(0,0,0), qRgb(0,0,0), qRgb(4,69,28), qRgb(4,73,32), qRgb(4,77,32), qRgb(4,77,32),
    qRgb(4,81,36), qRgb(4,81,36), qRgb(4,85,36), qRgb(0,85,36), qRgb(0,89,40), qRgb(0,89,40),
    qRgb(0,93,40), qRgb(0,97,44), qRgb(0,97,44), qRgb(0,101,44), qRgb(0,101,44), qRgb(0,105,48),
    qRgb(0,105,48), qRgb(0,109,48), qRgb(0,109,52), qRgb(0,113,52), qRgb(0,117,52), qRgb(0,117,52),
    qRgb(0,121,56), qRgb(0,134,60), qRgb(0,134,60), qRgb(0,138,65), qRgb(20,146,77), qRgb(36,154,89),
    qRgb(56,162,105), qRgb(73,170,117), qRgb(93,178,134), qRgb(109,186,146), qRgb(130,199,162), qRgb(146,207,174),
    qRgb(166,215,186), qRgb(182,223,203), qRgb(203,231,215), qRgb(219,239,231), qRgb(239,247,243), qRgb(251,251,251),
    qRgb(8,89,36), qRgb(12,85,36), qRgb(20,81,32), qRgb(24,81,28), qRgb(32,77,28), qRgb(36,73,24),
    qRgb(44,73,20), qRgb(48,69,20), qRgb(56,65,16), qRgb(60,65,12), qRgb(69,60,12), qRgb(77,56,8),
    qRgb(85,52,4), qRgb(97,48,0), qRgb(4,40,20), qRgb(4,44,20), qRgb(4,44,20), qRgb(4,48,24),
    qRgb(4,48,24), qRgb(4,52,24), qRgb(4,52,24), qRgb(4,56,28), qRgb(4,56,28), qRgb(4,60,28),
    qRgb(4,60,28), qRgb(4,65,28), qRgb(8,65,32), qRgb(8,65,32), qRgb(8,69,36), qRgb(8,69,36),
    qRgb(8,73,36), qRgb(8,73,36), qRgb(8,77,36), qRgb(8,77,40), qRgb(8,77,40), qRgb(0,81,40),
    qRgb(4,85,44), qRgb(8,85,44), qRgb(8,89,44), qRgb(223,162,93), qRgb(199,134,65), qRgb(174,105,44),
    qRgb(146,81,28), qRgb(121,56,12), qRgb(97,40,4), qRgb(142,101,60), qRgb(125,85,40), qRgb(109,69,28),
    qRgb(93,52,20), qRgb(77,36,8), qRgb(60,28,4), qRgb(4,69,28), qRgb(134,134,251), qRgb(134,134,251),
    qRgb(0,0,0), qRgb(8,8,8), qRgb(16,16,16), qRgb(24,24,24), qRgb(32,32,32), qRgb(40,40,40),
    qRgb(48,48,48), qRgb(56,56,56), qRgb(65,65,65), qRgb(73,73,73), qRgb(85,85,85), qRgb(93,93,93),
    qRgb(101,101,101), qRgb(109,109,109), qRgb(117,117,117), qRgb(125,125,125), qRgb(134,134,134), qRgb(142,142,142),
    qRgb(150,150,150), qRgb(158,158,158), qRgb(166,166,166), qRgb(174,174,174), qRgb(182,182,182), qRgb(190,190,190),
    qRgb(199,199,199), qRgb(207,207,207), qRgb(215,215,215), qRgb(223,223,223), qRgb(231,231,231), qRgb(239,239,239),
    qRgb(247,247,247), qRgb(251,251,251), qRgb(77,0,0), qRgb(117,0,0), qRgb(154,0,0), qRgb(195,0,0),
    qRgb(231,0,0), qRgb(134,89,4), qRgb(166,130,4), qRgb(195,174,4), qRgb(227,215,4), qRgb(251,251,0),
    qRgb(0,0,97), qRgb(0,0,134), qRgb(0,0,174), qRgb(0,0,211), qRgb(0,0,247), qRgb(0,65,0),
    qRgb(0,93,0), qRgb(0,117,0), qRgb(0,146,0), qRgb(0,174,0), qRgb(8,93,142), qRgb(20,117,170),
    qRgb(36,146,199), qRgb(48,170,227), qRgb(60,199,251), qRgb(32,32,65), qRgb(36,40,81), qRgb(44,48,97),
    qRgb(52,60,125), qRgb(56,65,138), qRgb(60,0,4), qRgb(77,0,8), qRgb(93,0,12), qRgb(113,0,16),
    qRgb(130,0,20), qRgb(16,16,16), qRgb(24,24,24), qRgb(36,36,36), qRgb(44,44,44), qRgb(56,56,56),
    qRgb(0,20,28), qRgb(0,28,40), qRgb(0,32,52), qRgb(0,40,65), qRgb(0,48,77), qRgb(146,36,0),
    qRgb(174,56,4), qRgb(203,77,8), qRgb(231,97,12), qRgb(251,117,16), qRgb(4,138,138), qRgb(4,166,166),
    qRgb(4,199,199), qRgb(0,227,227), qRgb(0,251,251), qRgb(0,73,48), qRgb(0,97,60), qRgb(0,121,69),
    qRgb(0,146,81), qRgb(0,166,93), qRgb(130,0,81), qRgb(158,0,101), qRgb(190,0,121), qRgb(219,0,142),
    qRgb(247,0,162), qRgb(138,24,134), qRgb(162,40,162), qRgb(190,56,186), qRgb(215,73,215), qRgb(243,89,243),
    qRgb(166,182,203), qRgb(247,0,0), qRgb(105,105,105), qRgb(154,154,154), qRgb(199,199,199), qRgb(247,247,247),
    qRgb(247,219,24), qRgb(0,0,247), qRgb(97,142,182), qRgb(134,170,28), qRgb(251,227,32), qRgb(0,0,251),
    qRgb(251,0,0), qRgb(162,162,162), qRgb(207,207,207), qRgb(251,251,251), qRgb(174,190,211), qRgb(105,150,190),
    qRgb(113,113,113), qRgb(251,0,251), qRgb(24,105,20), qRgb(251,195,195), qRgb(170,203,65), qRgb(117,0,142),
    qRgb(247,154,0), qRgb(243,150,150), qRgb(162,207,0), qRgb(150,243,20), qRgb(0,81,44), qRgb(0,97,48),
    qRgb(0,121,52), qRgb(0,134,56), qRgb(0,138,56), qRgb(65,166,105), qRgb(130,199,158), qRgb(4,113,48),
    qRgb(12,109,44), qRgb(16,101,40), qRgb(24,97,36), qRgb(32,89,36), qRgb(36,85,32), qRgb(44,77,28),
    qRgb(52,73,24), qRgb(56,65,20), qRgb(65,52,0), qRgb(85,69,0), qRgb(134,109,0), qRgb(170,134,0),
    qRgb(48,12,32), qRgb(65,16,44), qRgb(77,20,52), qRgb(97,24,69), qRgb(24,24,24), qRgb(40,40,40),
    qRgb(48,48,48), qRgb(65,65,65), qRgb(0,52,0), qRgb(134,134,251)
    };
    return pal;
}

bool PCFutbolImagenes::esFicheroGFX(const QString &nombreFichero)
{
    return nombreFichero.startsWith("GFX", Qt::CaseInsensitive) &&
           nombreFichero.endsWith(".DAT", Qt::CaseInsensitive);
}

QVector<QVector<QRgb>> PCFutbolImagenes::leerPaletasGFX(const QByteArray &data)
{
    QVector<QVector<QRgb>> paletas;
    const unsigned char *d = reinterpret_cast<const unsigned char *>(data.constData());
    const int n = data.size();

    int o = 0;
    while (o + 51 <= n) {
        if (d[o] == 0x00 && d[o+1] == 0x10 && d[o+50] == 0xFF) {
            // Prueba de validez: son valores VGA de 6 bits expandidos a 8
            // bits (v*4|3), luego todos cumplen v == 0 || v % 4 == 3.
            bool vga = true;
            bool algo = false;
            for (int i = 2; i < 50 && vga; i++) {
                const unsigned char v = d[o+i];
                if (v != 0 && (v % 4) != 3) vga = false;
                if (v != 0) algo = true;
            }
            if (vga && algo) {
                QVector<QRgb> pal;
                for (int i = 0; i < 16; i++)
                    pal.append(qRgb(d[o+2+i*3], d[o+3+i*3], d[o+4+i*3]));

                // Dos filtros mas contra falsos positivos (dentro de los datos
                // de una imagen aparecen secuencias que pasan la prueba VGA):
                // toda paleta real empieza en negro y tiene variedad de color.
                const bool empiezaEnNegro = (pal.at(0) == qRgb(0, 0, 0));
                const int distintos = QSet<QRgb>(pal.begin(), pal.end()).size();
                if (empiezaEnNegro && distintos >= 10) {
                    paletas.append(pal);
                    o += 51;
                    continue;
                }
            }
        }
        o++;
    }
    return paletas;
}

QPair<QVector<QRgb>, QString> PCFutbolImagenes::obtenerPaletaFamiliaA(const QString &nombreFichero,
                                                                      int offset,
                                                                      const QByteArray &data)
{
    if (esFicheroGFX(nombreFichero)) {
        const QVector<QVector<QRgb>> paletas = leerPaletasGFX(data);
        if (!paletas.isEmpty()) {
            // El fichero no dice que paleta toca a cada imagen: lo decide el
            // codigo del juego. Por defecto la 0; la unica excepcion conocida
            // es la zona del banner de GFX02, que usa la 8. El indice va al
            // manifest y se puede cambiar a mano y reimportar.
            int idx = 0;
            if (nombreFichero.compare("GFX02.DAT", Qt::CaseInsensitive) == 0 &&
                offset >= 380000 && offset <= 530000 && paletas.size() > 8) {
                idx = 8;
            }
            // GFX04 (solo lo usa PROMANAG.DAT). Sacado del código del juego:
            // cada pantalla fija su paleta con call 0x56220(fichero, n*51) y
            // se ha seguido qué pantalla dibuja cada imagen (9/11/2026).
            // Igual en 4.0 y 4.5 (el fichero es idéntico).
            if (nombreFichero.compare("GFX04.DAT", Qt::CaseInsensitive) == 0 &&
                data.size() == 244815 && paletas.size() >= 10) {
                static const QSet<int> paleta5 = {   // estadio y sus obras
                    16802, 38411, 60020, 81629, 103238, 124847, 146456, 168065, 189674,
                    211283, 215388, 219493, 220802, 222111, 223760, 225409, 225646,
                    225883, 226144, 226405};
                if (offset == 1224) idx = 6;                         // trofeo grande
                else if (paleta5.contains(offset)) idx = 5;
                else if (offset >= 240333 && offset <= 244566) idx = 9;   // mini-campos
            }
            return {paletas.at(idx), QString("gfx:%1").arg(idx)};
        }
    }
    return obtenerPaletaFamiliaA(nombreFichero, offset);
}

// Dibujos de la ficha del equipo (CAMISAS, CINTAUP, CINTADW): no llevan
// paleta; la ficha usa la de la foto del estadio (BIGCAMP/ES<id>.DFG) o, si
// no hay, la del escudo grande (BIGESC/EQ<id>.DFG). Verificado en el juego.
QVector<QRgb> PCFutbolImagenes::paletaEquipo(const QString &carpetaJuego, const QString &id)
{
    for (const auto &par : {qMakePair(QString("BIGCAMP"), QString("ES")), qMakePair(QString("BIGESC"), QString("EQ"))}) {
        QDir dbdat(carpetaJuego + "/DBDAT");
        for (const QString &sub : dbdat.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (sub.compare(par.first, Qt::CaseInsensitive) != 0) continue;
            QDir dir(dbdat.absoluteFilePath(sub));
            for (const QString &f : dir.entryList(QDir::Files)) {
                if (f.compare(par.second + id + ".DFG", Qt::CaseInsensitive) != 0) continue;
                bool ok = false;
                const QByteArray d = leerFichero(dir.absoluteFilePath(f), &ok);
                if (!ok || d.size() < 50 || quint8(d.at(0)) != 0x00 || quint8(d.at(1)) != 0x10) continue;
                QVector<QRgb> pal;
                for (int i = 0; i < 16; i++)
                    pal.append(qRgb(quint8(d.at(2 + 3 * i)), quint8(d.at(3 + 3 * i)), quint8(d.at(4 + 3 * i))));
                return pal;
            }
        }
    }
    return paletaMinifoto();
}

QVector<QRgb> PCFutbolImagenes::paletaPorEtiqueta(const QString &etiqueta, const QByteArray &data,
                                                 const QString &carpetaJuego)
{
    if (etiqueta.startsWith("equipo:"))
        return paletaEquipo(carpetaJuego, etiqueta.mid(7));
    return paletaPorEtiqueta(etiqueta, data);
}

QVector<QRgb> PCFutbolImagenes::paletaPorEtiqueta(const QString &etiqueta, const QByteArray &data)
{
    if (etiqueta.startsWith("gfx:")) {
        const QVector<QVector<QRgb>> paletas = leerPaletasGFX(data);
        bool ok = false;
        const int idx = etiqueta.mid(4).toInt(&ok);
        if (ok && idx >= 0 && idx < paletas.size())
            return paletas.at(idx);
    }
    return paletaPorEtiqueta(etiqueta);
}

QPair<QVector<QRgb>, QString> PCFutbolImagenes::obtenerPaletaFamiliaA(const QString &nombreFichero, int offset)
{
    if (nombreFichero.compare("GFX02.DAT", Qt::CaseInsensitive) == 0 &&
        offset >= 380000 && offset <= 530000) {
        return {paletaBannerCompartidaGFX02(), QString("rango:GFX02.DAT:380000-530000")};
    }
    if (nombreFichero.compare("GFX05.DAT", Qt::CaseInsensitive) == 0 && offset == 620769) {
        return {paletaBannerCompartidaGFX02(), QString("especial:GFX05.DAT:620769")};
    }
    return {paletaMinifoto(), QString("minifoto")};
}

QVector<QRgb> PCFutbolImagenes::paletaPorEtiqueta(const QString &etiqueta)
{
    if (etiqueta.startsWith("rango:") || etiqueta.startsWith("especial:"))
        return paletaBannerCompartidaGFX02();
    if (etiqueta == "miniesc")
        return paletaMiniesc();
    return paletaMinifoto();
}

// ==========================================================================
// Utilidades de fichero
// ==========================================================================

QByteArray PCFutbolImagenes::leerFichero(const QString &ruta, bool *ok)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly)) {
        if (ok) *ok = false;
        return QByteArray();
    }
    QByteArray data = f.readAll();
    f.close();
    if (ok) *ok = true;
    return data;
}

bool PCFutbolImagenes::escribirFichero(const QString &ruta, const QByteArray &data)
{
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(data);
    f.close();
    return true;
}

// Carpetas de formato mixto (MINIFOTO/MINIESC): (clave, ancho_fallback, alto_fallback)
static const QVector<QPair<QString, QPair<int,int>>> CARPETAS_FORMATO_MIXTO = {
    {"MINIFOTO", {24, 24}},
    {"MINIESC",  {48, 64}},
};

// ==========================================================================
// EXPORTAR -- boton 1
// ==========================================================================

PCFutbolImagenes::ResultadoExportacion PCFutbolImagenes::exportarImagenes(
    const QString &carpetaJuego, const QString &carpetaSalida,
    CallbackProgreso avance)
{
    ResultadoExportacion r;
    r.carpetaSalida = carpetaSalida;

    // Primera pasada, sólo para saber cuántos ficheros hay y poder dar un
    // porcentaje real de progreso (es un simple recorrido de directorios).
    int totalFicheros = 0;
    {
        QDirIterator itc(carpetaJuego, QDir::Files, QDirIterator::Subdirectories);
        while (itc.hasNext()) { itc.next(); totalFicheros++; }
    }

    QDir().mkpath(carpetaSalida);
    QFile manifest(carpetaSalida + "/manifest.csv");
    if (!manifest.open(QIODevice::WriteOnly | QIODevice::Text)) {
        r.errores << "No se pudo crear manifest.csv";
        return r;
    }
    r.manifestPath = carpetaSalida + "/manifest.csv";
    QTextStream out(&manifest);
    out << "ruta_original,ruta_png,familia,offset,ancho,alto,paleta\n";

    static const QStringList extensionesIgnorar = {"EXE", "BAT", "INI", "TXT", "ZIP"};

    QDirIterator it(carpetaJuego, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString rutaAbs = it.next();
        QString rutaRel = QDir(carpetaJuego).relativeFilePath(rutaAbs);
        r.ficherosEscaneados++;

        if (avance && !avance(r.ficherosEscaneados, totalFicheros, rutaRel)) {
            r.errores << "Exportación cancelada por el usuario.";
            manifest.close();
            return r;
        }

        QString ext = QFileInfo(rutaAbs).suffix().toUpper();
        if (extensionesIgnorar.contains(ext)) continue;

        bool ok;
        QByteArray data = leerFichero(rutaAbs, &ok);
        if (!ok || data.size() < 20) continue;

        QString carpetaDestino = QFileInfo(carpetaSalida + "/" + rutaRel).path();
        QString stem = QFileInfo(rutaRel).completeBaseName();
        QString nombreFichero = QFileInfo(rutaRel).fileName();

        Familia familia = clasificarFichero(rutaRel);

        if (familia == Familia::B) {
            int w, h;
            if (!leerDimensionesFamiliaB(data, 0, &w, &h)) continue;
            QDir().mkpath(carpetaDestino);
            QImage img = decodeFamiliaB(data, w, h);
            QString nombrePng = stem + ".png";
            img.save(carpetaDestino + "/" + nombrePng);
            out << rutaRel << "," << QDir(carpetaSalida).relativeFilePath(carpetaDestino + "/" + nombrePng)
                << ",B,0," << w << "," << h << ",\n";
            r.ficherosConImagen++;
            r.imagenesExtraidas++;
            continue;
        }

        if (familia == Familia::C) {
            int minimo = ESCUDOS_CABECERA + 4*ESCUDOS_ALTO*(ESCUDOS_ANCHO/4) + ESCUDOS_CIERRE;
            if (data.size() < minimo) continue;
            QDir().mkpath(carpetaDestino);
            QImage img = decodeEscudosSim(data);
            QString nombrePng = stem + ".png";
            img.save(carpetaDestino + "/" + nombrePng);
            QString cabeceraHex = data.left(ESCUDOS_CABECERA).toHex();
            out << rutaRel << "," << QDir(carpetaSalida).relativeFilePath(carpetaDestino + "/" + nombrePng)
                << ",C,0," << ESCUDOS_ANCHO << "," << ESCUDOS_ALTO << ",cabecera:" << cabeceraHex << "\n";
            r.ficherosConImagen++;
            r.imagenesExtraidas++;
            continue;
        }

        // Carpetas de formato mixto MINIFOTO/MINIESC
        bool procesadoMixto = false;
        QString up = rutaRel.toUpper();
        for (const auto &par : CARPETAS_FORMATO_MIXTO) {
            if (up.contains(par.first)) {
                FormatoDetectado res = detectarFormatoUnaImagen(data, par.second.first, par.second.second);
                if (!res.valido) { procesadoMixto = true; break; }
                QDir().mkpath(carpetaDestino);
                QString nombrePng = stem + ".png";
                QString etiqueta;
                if (res.familia == Familia::A) {
                    QVector<QRgb> pal = (par.first == "MINIFOTO") ? paletaMinifoto() : paletaMiniesc();
                    QImage img = decodeFamiliaA(data, res.offset, res.ancho, res.alto, pal);
                    img.save(carpetaDestino + "/" + nombrePng);
                    etiqueta = (par.first == "MINIFOTO") ? "minifoto" : "miniesc";
                    out << rutaRel << "," << QDir(carpetaSalida).relativeFilePath(carpetaDestino + "/" + nombrePng)
                        << ",A," << res.offset << "," << res.ancho << "," << res.alto << "," << etiqueta << "\n";
                } else {
                    QImage img = decodeFamiliaB(data, res.ancho, res.alto, res.offset);
                    img.save(carpetaDestino + "/" + nombrePng);
                    out << rutaRel << "," << QDir(carpetaSalida).relativeFilePath(carpetaDestino + "/" + nombrePng)
                        << ",B," << res.offset << "," << res.ancho << "," << res.alto << ",\n";
                }
                r.ficherosConImagen++;
                r.imagenesExtraidas++;
                procesadoMixto = true;
                break;
            }
        }
        if (procesadoMixto) continue;

        // GRAFICOS/*.EMP son las animaciones del partido (otro formato, de
        // 256 colores): el escáner de la familia A sólo encontraba basura.
        if (up.startsWith("GRAFICOS/")) continue;

        // Dibujos de la ficha del equipo: paleta del propio equipo
        QString idEquipo;
        if ((up.contains("/CAMISAS/") || up.contains("/CINTAUP/") || up.contains("/CINTADW/")) &&
            stem.size() == 8 && stem.startsWith("EQ", Qt::CaseInsensitive))
            idEquipo = stem.mid(2);

        // Familia A general: escanear el fichero entero
        QVector<ImagenEncontrada> imagenes = buscarImagenesFamiliaA(data);
        if (imagenes.isEmpty()) continue;

        QDir().mkpath(carpetaDestino);
        bool multiple = imagenes.size() > 1;
        for (int idx = 0; idx < imagenes.size(); idx++) {
            const ImagenEncontrada &im = imagenes[idx];
            auto parPaleta = idEquipo.isEmpty()
                ? obtenerPaletaFamiliaA(nombreFichero, im.offset, data)
                : qMakePair(paletaEquipo(carpetaJuego, idEquipo), "equipo:" + idEquipo);
            QImage img = decodeFamiliaA(data, im.offset, im.ancho, im.alto, parPaleta.first);
            QString nombrePng = multiple
                ? QString("%1_%2.png").arg(stem).arg(idx, 4, 10, QChar('0'))
                : (stem + ".png");
            img.save(carpetaDestino + "/" + nombrePng);
            out << rutaRel << "," << QDir(carpetaSalida).relativeFilePath(carpetaDestino + "/" + nombrePng)
                << ",A," << im.offset << "," << im.ancho << "," << im.alto << "," << parPaleta.second << "\n";
        }
        r.ficherosConImagen++;
        r.imagenesExtraidas += imagenes.size();
    }

    manifest.close();
    return r;
}

// ==========================================================================
// IMPORTAR -- boton 2
// ==========================================================================

// Copia recursiva de una carpeta completa (Qt no trae una funcion de una
// sola linea para esto).
static bool copiarCarpetaRecursiva(const QString &origen, const QString &destino)
{
    QDir dirOrigen(origen);
    if (!dirOrigen.exists()) return false;
    QDir().mkpath(destino);

    const auto entradas = dirOrigen.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : entradas) {
        QString destinoRuta = destino + "/" + info.fileName();
        if (info.isDir()) {
            if (!copiarCarpetaRecursiva(info.filePath(), destinoRuta)) return false;
        } else {
            QFile::copy(info.filePath(), destinoRuta);
        }
    }
    return true;
}

// Estructura auxiliar: una fila del manifest.csv
struct FilaManifest {
    QString rutaOriginal, rutaPng, familia, paleta;
    int offset, ancho, alto;
};

static QVector<FilaManifest> leerManifest(const QString &manifestPath, bool *ok)
{
    QVector<FilaManifest> filas;
    QFile f(manifestPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) { if (ok) *ok = false; return filas; }
    QTextStream in(&f);
    QString cabecera = in.readLine(); // descartar cabecera
    while (!in.atEnd()) {
        QString linea = in.readLine();
        QStringList campos = linea.split(",");
        if (campos.size() < 7) continue;
        FilaManifest fila;
        fila.rutaOriginal = campos[0];
        fila.rutaPng = campos[1];
        fila.familia = campos[2];
        fila.offset = campos[3].toInt();
        fila.ancho = campos[4].toInt();
        fila.alto = campos[5].toInt();
        // la paleta puede contener ":" pero no "," asi que campos[6..] es solo un campo
        fila.paleta = campos.mid(6).join(",");
        filas.append(fila);
    }
    if (ok) *ok = true;
    return filas;
}

PCFutbolImagenes::ResultadoImportacion PCFutbolImagenes::importarImagenes(
    const QString &carpetaOriginal, const QString &carpetaImagenes, const QString &carpetaNueva,
    CallbackProgreso avance)
{
    ResultadoImportacion r;
    r.carpetaNueva = carpetaNueva;

    QString manifestPath = carpetaImagenes + "/manifest.csv";
    if (!QFile::exists(manifestPath)) {
        r.errores << "No existe manifest.csv en " + carpetaImagenes;
        return r;
    }
    if (QDir(carpetaNueva).exists() && !QDir(carpetaNueva).isEmpty()) {
        r.errores << "La carpeta de salida ya existe y no esta vacia: " + carpetaNueva;
        return r;
    }

    if (avance) avance(0, 0, QObject::tr("Copiando el juego original…"));

    if (!copiarCarpetaRecursiva(carpetaOriginal, carpetaNueva)) {
        r.errores << "No se pudo copiar " + carpetaOriginal + " a " + carpetaNueva;
        return r;
    }

    bool ok;
    QVector<FilaManifest> filas = leerManifest(manifestPath, &ok);
    if (!ok) {
        r.errores << "No se pudo leer manifest.csv";
        return r;
    }

    // Agrupar filas por fichero original (un GFX puede tener varias imagenes)
    QMap<QString, QVector<FilaManifest>> porFichero;
    for (const FilaManifest &fila : filas)
        porFichero[fila.rutaOriginal].append(fila);

    int hechos = 0;
    const int totalFich = porFichero.size();
    for (auto it = porFichero.constBegin(); it != porFichero.constEnd(); ++it) {
        const QString &rutaOriginal = it.key();
        const QVector<FilaManifest> &filasFichero = it.value();

        hechos++;
        if (avance && !avance(hechos, totalFich, rutaOriginal)) {
            r.errores << "Importación cancelada por el usuario.";
            return r;
        }
        QString rutaDestino = carpetaNueva + "/" + rutaOriginal;
        if (!QFile::exists(rutaDestino)) {
            r.errores << "No existe " + rutaDestino + ", se omite";
            continue;
        }

        QString familia = filasFichero.first().familia;

        if (familia == "B") {
            const FilaManifest &fila = filasFichero.first();
            QString pngPath = carpetaImagenes + "/" + fila.rutaPng;
            if (!QFile::exists(pngPath)) continue;

            bool okLeer;
            QByteArray dataOriginal = leerFichero(rutaDestino, &okLeer);
            if (!okLeer) continue;
            QImage original = decodeFamiliaB(dataOriginal, fila.ancho, fila.alto).convertToFormat(QImage::Format_RGB32);
            QImage nuevo = QImage(pngPath).convertToFormat(QImage::Format_RGB32)
                               .scaled(fila.ancho, fila.alto, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            if (original == nuevo) continue; // sin cambios

            QByteArray contenido = encodeFamiliaB(QImage(pngPath), fila.ancho, fila.alto);
            if (escribirFichero(rutaDestino, contenido)) {
                r.ficherosReescritos++;
                r.imagenesReinyectadas++;
            }
            continue;
        }

        if (familia == "C") {
            const FilaManifest &fila = filasFichero.first();
            QString pngPath = carpetaImagenes + "/" + fila.rutaPng;
            if (!QFile::exists(pngPath)) continue;

            bool okLeer;
            QByteArray dataOriginal = leerFichero(rutaDestino, &okLeer);
            if (!okLeer) continue;
            QImage original = decodeEscudosSim(dataOriginal).convertToFormat(QImage::Format_RGB32);
            QImage nuevo = QImage(pngPath).convertToFormat(QImage::Format_RGB32)
                               .scaled(ESCUDOS_ANCHO, ESCUDOS_ALTO, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            if (original == nuevo) continue;

            QByteArray cabeceraOriginal;
            if (fila.paleta.startsWith("cabecera:")) {
                cabeceraOriginal = QByteArray::fromHex(fila.paleta.mid(QString("cabecera:").size()).toUtf8());
            } else {
                cabeceraOriginal = dataOriginal.left(ESCUDOS_CABECERA);
            }
            QByteArray contenido = encodeEscudosSim(QImage(pngPath), cabeceraOriginal);
            if (escribirFichero(rutaDestino, contenido)) {
                r.ficherosReescritos++;
                r.imagenesReinyectadas++;
            }
            continue;
        }

        // Familia A: parchear cada imagen en su offset, solo si cambio
        bool okLeer;
        QByteArray data = leerFichero(rutaDestino, &okLeer);
        if (!okLeer) continue;
        bool cambiado = false;

        for (const FilaManifest &fila : filasFichero) {
            QString pngPath = carpetaImagenes + "/" + fila.rutaPng;
            if (!QFile::exists(pngPath)) continue;

            QVector<QRgb> pal = paletaPorEtiqueta(fila.paleta, data, carpetaOriginal);
            QImage original = decodeFamiliaA(data, fila.offset, fila.ancho, fila.alto, pal)
                                   .convertToFormat(QImage::Format_RGB32);
            QImage nuevo = QImage(pngPath).convertToFormat(QImage::Format_RGB32)
                               .scaled(fila.ancho, fila.alto, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            if (original == nuevo) continue; // sin cambios, no tocar esta imagen

            QByteArray nuevoCuerpo = encodeFamiliaA(QImage(pngPath), fila.ancho, fila.alto, pal);
            int rowbytes = (fila.ancho + 7) / 8;
            int esperado = rowbytes * fila.alto * 4;
            if (nuevoCuerpo.size() != esperado) {
                r.errores << "Tamano inesperado en " + fila.rutaPng + ", se omite";
                continue;
            }
            data.replace(fila.offset + 8, esperado, nuevoCuerpo);
            cambiado = true;
            r.imagenesReinyectadas++;
        }

        if (cambiado) {
            if (escribirFichero(rutaDestino, data))
                r.ficherosReescritos++;
        }
    }

    return r;
}
