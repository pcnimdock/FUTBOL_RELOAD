#include "pcfutbol_extras.h"

#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QTextStream>
#include <QtEndian>
#include <QHash>

// ===========================================================================
// Utilidades
// ===========================================================================
static inline quint16 leerU16(const QByteArray &d, int off)
{
    return static_cast<quint8>(d.at(off)) |
           (static_cast<quint8>(d.at(off + 1)) << 8);
}

static inline void escribirU16(QByteArray &d, int off, quint16 v)
{
    d[off]     = static_cast<char>(v & 0xFF);
    d[off + 1] = static_cast<char>((v >> 8) & 0xFF);
}

int PCFutbolExtras::indiceMasCercano(QRgb c, const QVector<QRgb> &paleta)
{
    int mejor = 1;
    int mejorDist = INT_MAX;
    const int r = qRed(c), g = qGreen(c), b = qBlue(c);
    // el índice 0 es transparente: nunca se elige por color
    for (int i = 1; i < paleta.size(); i++)
    {
        const int dr = r - qRed(paleta[i]);
        const int dg = g - qGreen(paleta[i]);
        const int db = b - qBlue(paleta[i]);
        const int d = dr * dr + dg * dg + db * db;
        if (d < mejorDist)
        {
            mejorDist = d;
            mejor = i;
            if (d == 0) break;
        }
    }
    return mejor;
}

QVector<QRgb> PCFutbolExtras::leerPaletaBBM(const QString &ruta, bool *ok)
{
    if (ok) *ok = false;
    QVector<QRgb> pal;

    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly))
    {
        return pal;
    }
    const QByteArray d = f.readAll();
    f.close();

    // IFF: FORM ... , buscamos el chunk CMAP (tamaños en big-endian)
    int pos = 12;
    while (pos + 8 <= d.size())
    {
        const QByteArray id = d.mid(pos, 4);
        const quint32 tam = qFromBigEndian<quint32>(
            reinterpret_cast<const uchar *>(d.constData() + pos + 4));
        if (id == "CMAP")
        {
            const int n = qMin<int>(256, tam / 3);
            for (int i = 0; i < n; i++)
            {
                const int o = pos + 8 + i * 3;
                pal.append(qRgb(static_cast<quint8>(d.at(o)),
                                static_cast<quint8>(d.at(o + 1)),
                                static_cast<quint8>(d.at(o + 2))));
            }
            while (pal.size() < 256) pal.append(qRgb(0, 0, 0));
            if (ok) *ok = true;
            return pal;
        }
        pos += 8 + tam + (tam % 2);
    }
    return pal;
}

// ===========================================================================
// SPRITES .EMP
// ===========================================================================
static bool leerDescriptorEMP(const QByteArray &d, int off, int tamDesc,
                              PCFutbolExtras::FrameEMP *f)
{
    if (off + tamDesc > d.size()) return false;

    const int X = leerU16(d, off);
    const int h = leerU16(d, off + 2);
    const int ps = leerU16(d, off + 4);
    if (tamDesc == 8 && leerU16(d, off + 6) != 0) return false;
    if (h < 1 || h > 256 || X < 2 || ps < 1) return false;

    const int bpr = X - 1;
    if (bpr * h != ps) return false;

    const int ancho = bpr * 4;
    if (ancho > 340) return false;
    if (off + tamDesc + 4 * ps > d.size()) return false;

    f->offset = off;
    f->datos = off + tamDesc;
    f->ancho = ancho;
    f->alto = h;
    f->bytesPorFila = bpr;
    f->tamPlano = ps;
    return true;
}

static QVector<PCFutbolExtras::FrameEMP> recorrerEMP(const QByteArray &d, int inicio,
                                                     int tamDesc, int *fin)
{
    QVector<PCFutbolExtras::FrameEMP> frames;
    int off = inicio;
    while (off + 8 <= d.size())
    {
        PCFutbolExtras::FrameEMP f;
        if (!leerDescriptorEMP(d, off, tamDesc, &f))
        {
            // entre frames puede haber relleno de alineación: resincronizamos
            bool encontrado = false;
            for (int salto = 2; salto <= 16 && !encontrado; salto += 2)
            {
                if (leerDescriptorEMP(d, off + salto, tamDesc, &f)) encontrado = true;
            }
            if (!encontrado) break;
        }
        frames.append(f);
        off = f.datos + 4 * f.tamPlano;
    }
    *fin = off;
    return frames;
}

QVector<PCFutbolExtras::FrameEMP> PCFutbolExtras::parsearEMP(const QByteArray &d)
{
    QVector<FrameEMP> mejor;
    if (d.size() < 20) return mejor;

    const int tamDesc = (static_cast<quint8>(d.at(1)) == 0x41) ? 8 : 6;
    const int limite = qMin(d.size(), 0x600);

    for (int ini = 0x10; ini < limite; ini += 2)
    {
        int fin = 0;
        const QVector<FrameEMP> fr = recorrerEMP(d, ini, tamDesc, &fin);
        if (fr.isEmpty()) continue;
        // el recorrido bueno consume el fichero hasta el relleno final
        if (d.size() - fin < 24 && fr.size() > mejor.size())
        {
            mejor = fr;
        }
    }
    return mejor;
}

QImage PCFutbolExtras::decodificarFrame(const QByteArray &d, const FrameEMP &f,
                                        const QVector<QRgb> &paleta)
{
    QImage img(f.ancho, f.alto, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    for (int plano = 0; plano < 4; plano++)
    {
        const int base = f.datos + plano * f.tamPlano;
        for (int y = 0; y < f.alto; y++)
        {
            for (int xb = 0; xb < f.bytesPorFila; xb++)
            {
                const quint8 idx = static_cast<quint8>(d.at(base + y * f.bytesPorFila + xb));
                if (idx == 0) continue; // transparente
                const int x = xb * 4 + plano;
                if (x < f.ancho && idx < paleta.size())
                {
                    img.setPixel(x, y, paleta[idx] | 0xFF000000u);
                }
            }
        }
    }
    return img;
}

bool PCFutbolExtras::codificarFrame(QByteArray &d, const FrameEMP &f, const QImage &img,
                                    const QVector<QRgb> &paleta)
{
    if (paleta.size() < 256) return false;

    QImage src = img.convertToFormat(QImage::Format_ARGB32);
    if (src.width() != f.ancho || src.height() != f.alto)
    {
        src = src.scaled(f.ancho, f.alto, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    for (int plano = 0; plano < 4; plano++)
    {
        const int base = f.datos + plano * f.tamPlano;
        for (int y = 0; y < f.alto; y++)
        {
            for (int xb = 0; xb < f.bytesPorFila; xb++)
            {
                const int x = xb * 4 + plano;
                const int pos = base + y * f.bytesPorFila + xb;
                quint8 idx = 0;
                if (x < f.ancho)
                {
                    const QRgb c = src.pixel(x, y);
                    if (qAlpha(c) < 128)
                    {
                        idx = 0;
                    }
                    else
                    {
                        // La paleta tiene 8 colores repetidos (mismo RGB, distinto
                        // índice). Si el byte original ya daba exactamente este
                        // color, lo dejamos tal cual: así reexportar e reimportar
                        // sin tocar nada devuelve el fichero byte a byte idéntico.
                        const quint8 previo = static_cast<quint8>(d.at(pos));
                        if (previo != 0 && previo < paleta.size() &&
                            (paleta[previo] | 0xFF000000u) == (c | 0xFF000000u))
                        {
                            idx = previo;
                        }
                        else
                        {
                            idx = static_cast<quint8>(indiceMasCercano(c, paleta));
                        }
                    }
                }
                d[pos] = static_cast<char>(idx);
            }
        }
    }
    return true;
}

// ===========================================================================
// FONDO POR TILES (.BLO + .BIN)
// ===========================================================================
static void tileAImagen(const QByteArray &blo, int idx, bool espejo,
                        const QVector<QRgb> &paleta, QImage &destino, int px, int py)
{
    const int base = idx * 256;
    if (base + 256 > blo.size()) return;

    for (int plano = 0; plano < 4; plano++)
    {
        for (int y = 0; y < PCFutbolExtras::TILE; y++)
        {
            for (int xb = 0; xb < 4; xb++)
            {
                const quint8 v = static_cast<quint8>(blo.at(base + plano * 64 + y * 4 + xb));
                int x = xb * 4 + plano;
                if (espejo) x = PCFutbolExtras::TILE - 1 - x;
                destino.setPixel(px + x, py + y, paleta[v]);
            }
        }
    }
}

QImage PCFutbolExtras::montarFondo(const QByteArray &blo, const QByteArray &mapa,
                                   const QVector<QRgb> &paleta)
{
    QImage img(MAPA_ANCHO * TILE, MAPA_ALTO * TILE, QImage::Format_RGB32);
    img.fill(Qt::black);
    if (mapa.size() < MAPA_ANCHO * MAPA_ALTO * 2 || paleta.size() < 256) return img;

    for (int ty = 0; ty < MAPA_ALTO; ty++)
    {
        for (int tx = 0; tx < MAPA_ANCHO; tx++)
        {
            const int k = ty * MAPA_ANCHO + tx;
            const quint16 e = leerU16(mapa, k * 2);
            tileAImagen(blo, e & 0x7FFF, (e & 0x8000) != 0, paleta, img, tx * TILE, ty * TILE);
        }
    }
    return img;
}

// Convierte un tile de la imagen a sus 256 bytes en formato planar
static QByteArray tileDesdeImagen(const QImage &img, int px, int py,
                                  const QVector<QRgb> &paleta, bool espejo,
                                  int (*cercano)(QRgb, const QVector<QRgb> &))
{
    QByteArray t(256, char(0));
    for (int plano = 0; plano < 4; plano++)
    {
        for (int y = 0; y < PCFutbolExtras::TILE; y++)
        {
            for (int xb = 0; xb < 4; xb++)
            {
                int x = xb * 4 + plano;
                if (espejo) x = PCFutbolExtras::TILE - 1 - x;
                const quint8 idx = static_cast<quint8>(cercano(img.pixel(px + x, py + y), paleta));
                t[plano * 64 + y * 4 + xb] = static_cast<char>(idx);
            }
        }
    }
    return t;
}

bool PCFutbolExtras::desmontarFondo(const QImage &imagen, const QVector<QRgb> &paleta,
                                    int tilesOriginales, QByteArray *blo, QByteArray *mapa,
                                    QString *aviso)
{
    if (paleta.size() < 256) return false;

    QImage img = imagen.convertToFormat(QImage::Format_RGB32);
    if (img.width() != MAPA_ANCHO * TILE || img.height() != MAPA_ALTO * TILE)
    {
        img = img.scaled(MAPA_ANCHO * TILE, MAPA_ALTO * TILE,
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    blo->clear();
    mapa->resize(MAPA_ANCHO * MAPA_ALTO * 2);

    QHash<QByteArray, int> vistos; // tile -> índice (reaprovecha repetidos)

    for (int ty = 0; ty < MAPA_ALTO; ty++)
    {
        for (int tx = 0; tx < MAPA_ANCHO; tx++)
        {
            const QByteArray normal = tileDesdeImagen(img, tx * TILE, ty * TILE,
                                                      paleta, false, indiceMasCercano);
            const QByteArray reflejado = tileDesdeImagen(img, tx * TILE, ty * TILE,
                                                         paleta, true, indiceMasCercano);
            quint16 entrada = 0;

            if (vistos.contains(normal))
            {
                entrada = static_cast<quint16>(vistos.value(normal));
            }
            else if (vistos.contains(reflejado))
            {
                // el tile ya existe espejado: reutilizamos y marcamos el bit 15
                entrada = static_cast<quint16>(vistos.value(reflejado) | 0x8000);
            }
            else
            {
                const int idx = blo->size() / 256;
                vistos.insert(normal, idx);
                blo->append(normal);
                entrada = static_cast<quint16>(idx);
            }
            escribirU16(*mapa, (ty * MAPA_ANCHO + tx) * 2, entrada);
        }
    }

    const int nuevos = blo->size() / 256;
    if (nuevos > tilesOriginales && aviso)
    {
        *aviso = QString("el tileset pasa de %1 a %2 tiles; el juego podría no tener "
                         "sitio para tantos. Reduce los colores o las zonas con ruido.")
                     .arg(tilesOriginales).arg(nuevos);
    }
    return true;
}

// ===========================================================================
// TEXTOS .GRC
// ===========================================================================
static QByteArray cifrarGRC(const QString &texto)
{
    const QByteArray claro = texto.toLatin1();
    const int n = claro.size();
    QByteArray out(n, char(0));
    for (int i = 0; i < n; i++)
    {
        const quint8 clave = static_cast<quint8>((0xC0 + n + i) & 0xFF);
        out[i] = static_cast<char>(static_cast<quint8>(claro.at(i)) ^ clave);
    }
    return out;
}

static QString descifrarGRC(const QByteArray &cifrado)
{
    const int n = cifrado.size();
    QByteArray out(n, char(0));
    for (int i = 0; i < n; i++)
    {
        const quint8 clave = static_cast<quint8>((0xC0 + n + i) & 0xFF);
        out[i] = static_cast<char>(static_cast<quint8>(cifrado.at(i)) ^ clave);
    }
    return QString::fromLatin1(out);
}

static bool textoPlausible(const QString &s)
{
    // Los textos del juego son ASCII en mayusculas (mas la N con tilde). Ser
    // estricto aqui es lo que separa las cadenas de verdad del ruido binario
    // que, por casualidad, tambien se descifra a algo "imprimible".
    static const QString signos = QString::fromUtf8(" .,:;-/()%'?!+*<>=&@#\"");
    if (s.size() < 3) return false;

    int letras = 0;
    for (const QChar &c : s)
    {
        const ushort u = c.unicode();
        const bool mayuscula = (u >= 'A' && u <= 'Z') || u == 0x00D1 /* N con tilde */;
        const bool digito = (u >= '0' && u <= '9');
        if (mayuscula || digito) { letras++; continue; }
        if (!signos.contains(c)) return false;
    }
    return letras >= qMax(3, s.size() - 2);
}

QVector<PCFutbolExtras::TextoGRC> PCFutbolExtras::leerTextosGRC(const QByteArray &d)
{
    QVector<TextoGRC> res;
    const int cabecera = d.indexOf(char(0x1A));
    if (cabecera < 0) return res;

    int off = cabecera + 1 + 12;
    while (off < d.size() - 1)
    {
        const int len = leerU16(d, off - 2);
        if (len >= 2 && len <= 60 && off + len <= d.size())
        {
            const QString s = descifrarGRC(d.mid(off, len));
            if (textoPlausible(s))
            {
                TextoGRC t;
                t.offset = off;
                t.tipo = leerU16(d, off - 12);
                t.x = leerU16(d, off - 6);
                t.y = leerU16(d, off - 4);
                t.longitud = len;
                t.texto = s;
                res.append(t);
                off += len + 12;
                continue;
            }
        }
        off++;
    }
    return res;
}

void PCFutbolExtras::escribirTextoGRC(QByteArray &d, const TextoGRC &t, const QString &nuevo)
{
    // La longitud es fija: rellenamos con espacios o recortamos, así el
    // fichero conserva su tamaño y ningún offset se descoloca.
    QString s = nuevo;
    if (s.size() > t.longitud) s = s.left(t.longitud);
    while (s.size() < t.longitud) s.append(' ');

    const QByteArray cif = cifrarGRC(s);
    for (int i = 0; i < t.longitud; i++)
    {
        d[t.offset + i] = cif.at(i);
    }
}

// ===========================================================================
// EXPORTAR
// ===========================================================================
static QVector<QRgb> paletaDe(const QString &carpetaJuego, const QString &subcarpeta)
{
    bool ok = false;
    QVector<QRgb> p = PCFutbolExtras::leerPaletaBBM(
        carpetaJuego + "/" + subcarpeta + "/PALETA.BBM", &ok);
    if (!ok)
    {
        p = PCFutbolExtras::leerPaletaBBM(carpetaJuego + "/PALETAS/PALETA.BBM", &ok);
    }
    return p;
}

void PCFutbolExtras::exportarTodo(const QString &carpetaJuego, const QString &carpetaSalida,
                                  Resultado &r, CallbackProgreso avance)
{
    // Total aproximado de pasos: un .EMP, un fondo o un .GRC = 1 paso.
    int total = 0, hechos = 0;
    {
        QDirIterator ic(carpetaJuego + "/GRAFICOS", QStringList() << "*.EMP",
                        QDir::Files, QDirIterator::Subdirectories);
        while (ic.hasNext()) { ic.next(); total++; }
        QDirIterator ig(carpetaJuego, QStringList() << "*.GRC", QDir::Files,
                        QDirIterator::Subdirectories);
        while (ig.hasNext()) { ig.next(); total++; }
        total += 2; // FON_GRAN + FON_PEQU
    }
    bool cancelado = false;

    const QVector<QRgb> palGraficos = paletaDe(carpetaJuego, "GRAFICOS");
    if (palGraficos.size() < 256)
    {
        r.errores << "No se encontró GRAFICOS/PALETA.BBM: no se exportan sprites ni fondos.";
    }

    // --- 1. Sprites .EMP de GRAFICOS ---------------------------------------
    if (palGraficos.size() >= 256)
    {
        const QString dirEmp = carpetaSalida + "/GRAFICOS";
        QDirIterator it(carpetaJuego + "/GRAFICOS", QStringList() << "*.EMP",
                        QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            const QString ruta = it.next();
            const QString nombre = QFileInfo(ruta).completeBaseName();

            hechos++;
            if (avance && !avance(hechos, total, "GRAFICOS/" + nombre + ".EMP"))
            { cancelado = true; break; }

            QFile f(ruta);
            if (!f.open(QIODevice::ReadOnly)) continue;
            const QByteArray d = f.readAll();
            f.close();

            const QVector<FrameEMP> frames = parsearEMP(d);
            if (frames.isEmpty())
            {
                r.errores << "No se pudo interpretar " + nombre + ".EMP";
                continue;
            }

            const QString dest = dirEmp + "/" + nombre;
            QDir().mkpath(dest);
            for (int i = 0; i < frames.size(); i++)
            {
                const QImage img = decodificarFrame(d, frames.at(i), palGraficos);
                img.save(QString("%1/%2_%3.png").arg(dest, nombre,
                                                     QString::number(i).rightJustified(3, '0')));
                r.spritesExportados++;
            }
        }
    }

    // --- 2. Fondos por tiles ------------------------------------------------
    if (palGraficos.size() >= 256)
    {
        const QStringList variantes = {"GRAN", "PEQU"};
        for (const QString &v : variantes)
        {
            if (cancelado) break;
            hechos++;
            if (avance && !avance(hechos, total, "GRAFICOS/FON_" + v + ".BLO"))
            { cancelado = true; break; }
            QFile fb(QString("%1/GRAFICOS/FON_%2.BLO").arg(carpetaJuego, v));
            QFile fm(QString("%1/GRAFICOS/MAP_%2.BIN").arg(carpetaJuego, v));
            if (!fb.open(QIODevice::ReadOnly) || !fm.open(QIODevice::ReadOnly)) continue;

            const QImage img = montarFondo(fb.readAll(), fm.readAll(), palGraficos);
            fb.close();
            fm.close();

            QDir().mkpath(carpetaSalida + "/GRAFICOS");
            img.save(QString("%1/GRAFICOS/FONDO_%2.png").arg(carpetaSalida, v));
            r.fondosExportados++;
        }
    }

    // --- 3. Textos .GRC -----------------------------------------------------
    if (cancelado)
    {
        r.errores << "Exportación cancelada por el usuario.";
        return;
    }

    QDirIterator itg(carpetaJuego, QStringList() << "*.GRC", QDir::Files,
                     QDirIterator::Subdirectories);
    QDir().mkpath(carpetaSalida + "/TEXTOS");
    while (itg.hasNext())
    {
        const QString ruta = itg.next();
        const QString nombre = QFileInfo(ruta).completeBaseName();

        hechos++;
        if (avance && !avance(hechos, total, nombre + ".GRC"))
        {
            r.errores << "Exportación cancelada por el usuario.";
            return;
        }

        QFile f(ruta);
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QByteArray d = f.readAll();
        f.close();

        const QVector<TextoGRC> textos = leerTextosGRC(d);
        if (textos.isEmpty()) continue;

        QFile out(QString("%1/TEXTOS/%2.txt").arg(carpetaSalida, nombre));
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            r.errores << "No se pudo escribir TEXTOS/" + nombre + ".txt";
            continue;
        }
        QTextStream ts(&out);
        ts.setEncoding(QStringConverter::Latin1);
        ts << "# Textos de " << nombre << ".GRC - edita SOLO lo que hay tras el ultimo '|'.\n"
           << "# La longitud es fija: un texto mas corto se rellena con espacios y uno\n"
           << "# mas largo se recorta. No toques los offsets ni el orden de las lineas.\n"
           << "# Codificacion del fichero: Latin-1 (ISO-8859-1).\n"
           << "# offset|tipo|x|y|longitud|texto\n";
        for (const TextoGRC &t : textos)
        {
            ts << t.offset << "|" << t.tipo << "|" << t.x << "|" << t.y << "|"
               << t.longitud << "|" << t.texto << "\n";
            r.textosExportados++;
        }
        out.close();
    }
}

// ===========================================================================
// IMPORTAR
// ===========================================================================
void PCFutbolExtras::importarTodo(const QString &carpetaNueva, const QString &carpetaImagenes,
                                  Resultado &r, CallbackProgreso avance)
{
    int total = 0, hechos = 0;
    {
        QDirIterator ic(carpetaNueva + "/GRAFICOS", QStringList() << "*.EMP",
                        QDir::Files, QDirIterator::Subdirectories);
        while (ic.hasNext()) { ic.next(); total++; }
        QDirIterator ig(carpetaNueva, QStringList() << "*.GRC", QDir::Files,
                        QDirIterator::Subdirectories);
        while (ig.hasNext()) { ig.next(); total++; }
        total += 2;
    }

    const QVector<QRgb> palGraficos = paletaDe(carpetaNueva, "GRAFICOS");

    // --- 1. Sprites .EMP ----------------------------------------------------
    if (palGraficos.size() >= 256)
    {
        QDirIterator it(carpetaNueva + "/GRAFICOS", QStringList() << "*.EMP",
                        QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            const QString ruta = it.next();
            const QString nombre = QFileInfo(ruta).completeBaseName();

            hechos++;
            if (avance && !avance(hechos, total, "GRAFICOS/" + nombre + ".EMP"))
            {
                r.errores << "Importación cancelada por el usuario.";
                return;
            }

            const QString dirPng = QString("%1/GRAFICOS/%2").arg(carpetaImagenes, nombre);
            if (!QDir(dirPng).exists()) continue;

            QFile f(ruta);
            if (!f.open(QIODevice::ReadOnly)) continue;
            QByteArray d = f.readAll();
            f.close();

            const QVector<FrameEMP> frames = parsearEMP(d);
            if (frames.isEmpty()) continue;

            bool cambiado = false;
            for (int i = 0; i < frames.size(); i++)
            {
                const QString png = QString("%1/%2_%3.png").arg(
                    dirPng, nombre, QString::number(i).rightJustified(3, '0'));
                if (!QFile::exists(png)) continue;

                QImage img(png);
                if (img.isNull())
                {
                    r.errores << "No se pudo leer " + png;
                    continue;
                }
                if (codificarFrame(d, frames.at(i), img, palGraficos))
                {
                    r.spritesImportados++;
                    cambiado = true;
                }
            }

            if (cambiado)
            {
                QFile out(ruta);
                if (out.open(QIODevice::WriteOnly))
                {
                    out.write(d);
                    out.close();
                }
                else
                {
                    r.errores << "No se pudo reescribir " + nombre + ".EMP";
                }
            }
        }
    }

    // --- 2. Fondos por tiles ------------------------------------------------
    if (palGraficos.size() >= 256)
    {
        const QStringList variantes = {"GRAN", "PEQU"};
        for (const QString &v : variantes)
        {
            const QString png = QString("%1/GRAFICOS/FONDO_%2.png").arg(carpetaImagenes, v);
            if (!QFile::exists(png)) continue;

            const QString rutaBlo = QString("%1/GRAFICOS/FON_%2.BLO").arg(carpetaNueva, v);
            const QString rutaMap = QString("%1/GRAFICOS/MAP_%2.BIN").arg(carpetaNueva, v);

            QFile fb(rutaBlo);
            if (!fb.open(QIODevice::ReadOnly)) continue;
            const int tilesOriginales = fb.size() / 256;
            fb.close();

            const QImage img(png);
            if (img.isNull())
            {
                r.errores << "No se pudo leer " + png;
                continue;
            }

            QByteArray blo, mapa;
            QString aviso;
            if (!desmontarFondo(img, palGraficos, tilesOriginales, &blo, &mapa, &aviso))
            {
                r.errores << "No se pudo reconstruir el fondo " + v;
                continue;
            }
            if (!aviso.isEmpty())
            {
                r.errores << QString("FONDO_%1: %2").arg(v, aviso);
            }

            QFile ob(rutaBlo), om(rutaMap);
            if (ob.open(QIODevice::WriteOnly) && om.open(QIODevice::WriteOnly))
            {
                ob.write(blo);
                om.write(mapa);
                ob.close();
                om.close();
                r.fondosImportados++;
            }
            else
            {
                r.errores << "No se pudo reescribir FON_" + v + ".BLO / MAP_" + v + ".BIN";
            }
        }
    }

    // --- 3. Textos .GRC -----------------------------------------------------
    QDirIterator itg(carpetaNueva, QStringList() << "*.GRC", QDir::Files,
                     QDirIterator::Subdirectories);
    while (itg.hasNext())
    {
        const QString ruta = itg.next();
        const QString nombre = QFileInfo(ruta).completeBaseName();
        const QString txt = QString("%1/TEXTOS/%2.txt").arg(carpetaImagenes, nombre);
        if (!QFile::exists(txt)) continue;

        QFile f(ruta);
        if (!f.open(QIODevice::ReadOnly)) continue;
        QByteArray d = f.readAll();
        f.close();

        const QVector<TextoGRC> textos = leerTextosGRC(d);
        QHash<int, TextoGRC> porOffset;
        for (const TextoGRC &t : textos) porOffset.insert(t.offset, t);

        QFile in(txt);
        if (!in.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QTextStream ts(&in);
        ts.setEncoding(QStringConverter::Latin1);

        bool cambiado = false;
        while (!ts.atEnd())
        {
            const QString linea = ts.readLine();
            if (linea.trimmed().isEmpty() || linea.startsWith('#')) continue;

            const QStringList campos = linea.split('|');
            if (campos.size() < 6) continue;

            bool okOff = false;
            const int offset = campos.at(0).toInt(&okOff);
            if (!okOff || !porOffset.contains(offset))
            {
                r.errores << QString("%1.txt: offset %2 desconocido, línea ignorada")
                                 .arg(nombre, campos.at(0));
                continue;
            }

            const TextoGRC &t = porOffset.value(offset);
            const QString nuevo = campos.mid(5).join('|'); // por si el texto lleva '|'
            if (nuevo == t.texto) continue;

            if (nuevo.size() > t.longitud)
            {
                r.errores << QString("%1.txt: \"%2\" no cabe (máx. %3 caracteres), se recorta")
                                 .arg(nombre, nuevo).arg(t.longitud);
            }
            escribirTextoGRC(d, t, nuevo);
            r.textosImportados++;
            cambiado = true;
        }
        in.close();

        if (cambiado)
        {
            QFile out(ruta);
            if (out.open(QIODevice::WriteOnly))
            {
                out.write(d);
                out.close();
            }
            else
            {
                r.errores << "No se pudo reescribir " + nombre + ".GRC";
            }
        }
    }
}

// ===========================================================================
// SONIDOS .RAW  <->  .WAV
// ===========================================================================
static void anexarU32(QByteArray &d, quint32 v)
{
    d.append(char(v & 0xFF));
    d.append(char((v >> 8) & 0xFF));
    d.append(char((v >> 16) & 0xFF));
    d.append(char((v >> 24) & 0xFF));
}

static void anexarU16(QByteArray &d, quint16 v)
{
    d.append(char(v & 0xFF));
    d.append(char((v >> 8) & 0xFF));
}

QByteArray PCFutbolExtras::rawAWav(const QByteArray &raw, int frecuencia)
{
    // WAV mínimo: RIFF/WAVE + fmt (PCM) + data. Las muestras van tal cual,
    // porque el .RAW ya es exactamente lo que un WAV de 8 bits guarda:
    // enteros sin signo con el silencio en 0x80.
    QByteArray w;
    w.append("RIFF");
    anexarU32(w, 36 + raw.size());
    w.append("WAVE");
    w.append("fmt ");
    anexarU32(w, 16);
    anexarU16(w, 1);                        // PCM
    anexarU16(w, 1);                        // mono
    anexarU32(w, frecuencia);
    anexarU32(w, frecuencia);               // bytes/segundo = frec * 1 canal * 1 byte
    anexarU16(w, 1);                        // alineación de bloque
    anexarU16(w, 8);                        // bits por muestra
    w.append("data");
    anexarU32(w, raw.size());
    w.append(raw);
    return w;
}

QByteArray PCFutbolExtras::wavARaw(const QByteArray &wav, QString *error)
{
    QByteArray vacio;
    if (wav.size() < 44 || wav.left(4) != "RIFF" || wav.mid(8, 4) != "WAVE")
    {
        if (error) *error = "no es un WAV";
        return vacio;
    }

    int formato = 0, canales = 0, bits = 0, frecuencia = 0;
    int datosOff = -1, datosLen = 0;

    int pos = 12;
    while (pos + 8 <= wav.size())
    {
        const QByteArray id = wav.mid(pos, 4);
        const quint32 tam = static_cast<quint8>(wav.at(pos + 4)) |
                            (static_cast<quint8>(wav.at(pos + 5)) << 8) |
                            (static_cast<quint8>(wav.at(pos + 6)) << 16) |
                            (static_cast<quint32>(static_cast<quint8>(wav.at(pos + 7))) << 24);
        if (id == "fmt " && pos + 8 + 16 <= wav.size())
        {
            formato    = leerU16(wav, pos + 8);
            canales    = leerU16(wav, pos + 10);
            frecuencia = leerU16(wav, pos + 12) | (leerU16(wav, pos + 14) << 16);
            bits       = leerU16(wav, pos + 22);
        }
        else if (id == "data")
        {
            datosOff = pos + 8;
            datosLen = qMin<int>(tam, wav.size() - datosOff);
        }
        pos += 8 + tam + (tam % 2);
    }

    if (formato != 1 || datosOff < 0 || canales < 1 || frecuencia < 1)
    {
        if (error) *error = "solo se admite WAV PCM sin comprimir";
        return vacio;
    }
    if (bits != 8 && bits != 16)
    {
        if (error) *error = "solo se admite WAV de 8 o 16 bits";
        return vacio;
    }

    // 1) a mono, en flotante centrado en 0
    const int anchoMuestra = (bits / 8) * canales;
    const int nMuestras = datosLen / anchoMuestra;
    QVector<float> mono(nMuestras, 0.0f);

    for (int i = 0; i < nMuestras; i++)
    {
        float suma = 0.0f;
        for (int c = 0; c < canales; c++)
        {
            const int o = datosOff + i * anchoMuestra + c * (bits / 8);
            if (bits == 8)
            {
                suma += static_cast<int>(static_cast<quint8>(wav.at(o))) - 128;
            }
            else
            {
                const qint16 v = static_cast<qint16>(leerU16(wav, o));
                suma += v / 256.0f;   // 16 bits -> escala de 8 bits
            }
        }
        mono[i] = suma / canales;
    }

    if (nMuestras < 1)
    {
        if (error) *error = "el WAV no tiene muestras";
        return vacio;
    }

    // 2) remuestreo lineal a 11.025 Hz
    const int destino = (frecuencia == FRECUENCIA_RAW)
                            ? nMuestras
                            : static_cast<int>(qint64(nMuestras) * FRECUENCIA_RAW / frecuencia);
    QByteArray raw(destino, char(0x80));

    for (int i = 0; i < destino; i++)
    {
        float m;
        if (frecuencia == FRECUENCIA_RAW)
        {
            m = mono.at(i);
        }
        else
        {
            const float src = static_cast<float>(i) * frecuencia / FRECUENCIA_RAW;
            const int i0 = static_cast<int>(src);
            const int i1 = qMin(i0 + 1, nMuestras - 1);
            const float t = src - i0;
            m = mono.at(qBound(0, i0, nMuestras - 1)) * (1.0f - t) + mono.at(i1) * t;
        }
        raw[i] = static_cast<char>(static_cast<quint8>(qBound(0, qRound(m) + 128, 255)));
    }
    return raw;
}

void PCFutbolExtras::exportarSonidos(const QString &carpetaJuego, const QString &carpetaSalida,
                                     Resultado &r)
{
    QDirIterator it(carpetaJuego, QStringList() << "*.RAW", QDir::Files,
                    QDirIterator::Subdirectories);
    bool alguno = false;
    while (it.hasNext())
    {
        const QString ruta = it.next();
        const QString nombre = QFileInfo(ruta).completeBaseName();

        QFile f(ruta);
        if (!f.open(QIODevice::ReadOnly))
        {
            r.errores << "No se pudo leer " + nombre + ".RAW";
            continue;
        }
        const QByteArray raw = f.readAll();
        f.close();

        if (!alguno)
        {
            QDir().mkpath(carpetaSalida + "/SONIDOS");
            alguno = true;
        }

        QFile out(QString("%1/SONIDOS/%2.wav").arg(carpetaSalida, nombre));
        if (!out.open(QIODevice::WriteOnly))
        {
            r.errores << "No se pudo escribir SONIDOS/" + nombre + ".wav";
            continue;
        }
        out.write(rawAWav(raw));
        out.close();
        r.sonidosExportados++;
    }
}

void PCFutbolExtras::importarSonidos(const QString &carpetaNueva, const QString &carpetaSonidos,
                                     Resultado &r)
{
    QDirIterator it(carpetaNueva, QStringList() << "*.RAW", QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString ruta = it.next();
        const QString nombre = QFileInfo(ruta).completeBaseName();
        const QString wavPath = QString("%1/SONIDOS/%2.wav").arg(carpetaSonidos, nombre);
        if (!QFile::exists(wavPath)) continue;

        QFile fw(wavPath);
        if (!fw.open(QIODevice::ReadOnly)) continue;
        const QByteArray wav = fw.readAll();
        fw.close();

        QString err;
        const QByteArray raw = wavARaw(wav, &err);
        if (raw.isEmpty())
        {
            r.errores << QString("%1.wav: %2").arg(nombre, err);
            continue;
        }

        // Si el original no ha cambiado, no reescribimos nada.
        QFile fo(ruta);
        if (fo.open(QIODevice::ReadOnly))
        {
            const QByteArray original = fo.readAll();
            fo.close();
            if (original == raw) continue;
        }

        QFile out(ruta);
        if (!out.open(QIODevice::WriteOnly))
        {
            r.errores << "No se pudo reescribir " + nombre + ".RAW";
            continue;
        }
        out.write(raw);
        out.close();
        r.sonidosImportados++;
    }
}
