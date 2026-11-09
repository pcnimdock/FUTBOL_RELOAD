#include "pcfutbol_dibujokit.h"
#include "pcfutbol_imagenes.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

namespace {

// Busca un fichero sin distinguir mayúsculas (los juegos de DOS mezclan)
QString buscarFichero(const QString &carpeta, const QString &nombre)
{
    QDir dir(carpeta);
    if (!dir.exists()) return QString();
    for (const QString &f : dir.entryList(QDir::Files))
    {
        if (f.compare(nombre, Qt::CaseInsensitive) == 0) return dir.absoluteFilePath(f);
    }
    return QString();
}

QString subcarpeta(const QString &carpetaJuego, const QString &nombre)
{
    QDir dbdat(carpetaJuego);
    for (const QString &d : dbdat.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        if (d.compare("DBDAT", Qt::CaseInsensitive) != 0) continue;
        QDir sub(dbdat.absoluteFilePath(d));
        for (const QString &s : sub.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        {
            if (s.compare(nombre, Qt::CaseInsensitive) == 0) return sub.absoluteFilePath(s);
        }
    }
    return QString();
}

QByteArray leerTodo(const QString &ruta)
{
    QFile f(ruta);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

// Paleta de reserva si no hay BIGCAMP ni BIGESC: los colores fijos del interfaz
const QRgb PALETA_RESERVA[16] = {
    qRgb(0, 0, 0),       qRgb(183, 87, 0),    qRgb(231, 159, 0),   qRgb(175, 191, 211),
    qRgb(255, 227, 35),  qRgb(0, 0, 255),     qRgb(255, 0, 0),     qRgb(107, 151, 191),
    qRgb(147, 35, 0),    qRgb(255, 0, 255),   qRgb(255, 0, 255),   qRgb(255, 0, 255),
    qRgb(115, 115, 115), qRgb(163, 163, 163), qRgb(207, 207, 207), qRgb(255, 255, 255)
};

} // namespace

QVector<QRgb> PCFutbolDibujoKit::leerPaletaB(const QByteArray &d)
{
    QVector<QRgb> p;
    if (d.size() < 50 || static_cast<quint8>(d.at(0)) != 0x00 || static_cast<quint8>(d.at(1)) != 0x10)
        return p;
    for (int i = 0; i < 16; i++)
    {
        p.append(qRgb(static_cast<quint8>(d.at(2 + 3 * i)),
                      static_cast<quint8>(d.at(3 + 3 * i)),
                      static_cast<quint8>(d.at(4 + 3 * i))));
    }
    return p;
}

bool PCFutbolDibujoKit::cargar(const QString &carpetaJuego, const QString &id, QString *error)
{
    *this = PCFutbolDibujoKit();
    m_rutaDfg = buscarFichero(subcarpeta(carpetaJuego, "CAMISAS"), "EQ" + id + ".DFG");
    if (m_rutaDfg.isEmpty())
    {
        if (error) *error = QString("Este equipo no tiene dibujo de equipación "
                                    "(DBDAT/CAMISAS/EQ%1.DFG). Sólo lo tienen 50 equipos.").arg(id);
        return false;
    }
    m_dfg = leerTodo(m_rutaDfg);
    const unsigned char *d = reinterpret_cast<const unsigned char *>(m_dfg.constData());
    if (m_dfg.size() < 8 || d[0] != 0x01 || d[5] != 0x04)
    {
        if (error) *error = "Formato no reconocido en " + m_rutaDfg;
        m_dfg.clear();
        return false;
    }
    m_ancho = d[1] | (d[2] << 8);
    m_alto = d[3] | (d[4] << 8);
    const int rowbytes = (m_ancho + 7) / 8;
    if (m_ancho <= 0 || m_alto <= 0 || 8 + rowbytes * m_alto * 4 > m_dfg.size())
    {
        if (error) *error = "Dimensiones incoherentes en " + m_rutaDfg;
        m_dfg.clear();
        return false;
    }
    m_indices.resize(m_ancho * m_alto);
    int pos = 8;
    for (int y = 0; y < m_alto; y++)
    {
        for (int x = 0; x < m_ancho; x++)
        {
            int idx = 0;
            for (int p = 0; p < 4; p++)
                idx |= ((d[pos + p * rowbytes + x / 8] >> (7 - x % 8)) & 1) << p;
            m_indices[y * m_ancho + x] = static_cast<quint8>(idx);
        }
        pos += 4 * rowbytes;
    }
    m_indicesOrig = m_indices;

    // Paleta: foto del estadio si existe; si no, escudo grande (verificado en el juego)
    m_rutaCamp = buscarFichero(subcarpeta(carpetaJuego, "BIGCAMP"), "ES" + id + ".DFG");
    m_rutaEsc = buscarFichero(subcarpeta(carpetaJuego, "BIGESC"), "EQ" + id + ".DFG");
    if (!m_rutaCamp.isEmpty()) m_camp = leerTodo(m_rutaCamp);
    if (!m_rutaEsc.isEmpty()) m_esc = leerTodo(m_rutaEsc);
    m_paleta = leerPaletaB(m_camp);
    m_origenPaleta = "DBDAT/BIGCAMP/ES" + id + ".DFG (foto del estadio)";
    if (m_paleta.size() != 16)
    {
        m_paleta = leerPaletaB(m_esc);
        m_origenPaleta = "DBDAT/BIGESC/EQ" + id + ".DFG (escudo grande)";
    }
    if (m_paleta.size() != 16)
    {
        m_paleta.clear();
        for (QRgb c : PALETA_RESERVA) m_paleta.append(c);
        m_origenPaleta = "paleta genérica (el equipo no tiene foto de estadio ni escudo grande)";
    }
    m_paletaOrig = m_paleta;
    return true;
}

int PCFutbolDibujoKit::sustituir(int de, int a)
{
    int n = 0;
    for (quint8 &v : m_indices)
    {
        if (v == de) { v = static_cast<quint8>(a & 15); n++; }
    }
    return n;
}

QVector<int> PCFutbolDibujoKit::recuento() const
{
    QVector<int> r(16, 0);
    for (quint8 v : m_indices) r[v & 15]++;
    return r;
}

void PCFutbolDibujoKit::setColorEquipo(int idx, QColor color)
{
    if (!esColorDeEquipo(idx) || m_paleta.size() != 16) return;
    // La VGA sólo usa 6 bits por componente: se guarda ya redondeado
    m_paleta[idx] = qRgb(color.red() & 0xFC, color.green() & 0xFC, color.blue() & 0xFC);
}

QImage PCFutbolDibujoKit::imagen(int escala) const
{
    if (!cargado()) return QImage();
    QImage im(m_ancho, m_alto, QImage::Format_Indexed8);
    im.setColorTable(m_paleta);
    for (int y = 0; y < m_alto; y++)
        for (int x = 0; x < m_ancho; x++)
            im.setPixel(x, y, m_indices.at(y * m_ancho + x));
    if (escala > 1)
        im = im.scaled(m_ancho * escala, m_alto * escala, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    return im;
}

void PCFutbolDibujoKit::importar(const QImage &img)
{
    if (!cargado() || img.isNull()) return;
    const QImage q = PCFutbolImagenes::cuantizarAPaleta(img, m_ancho, m_alto, m_paleta);
    for (int y = 0; y < m_alto; y++)
        for (int x = 0; x < m_ancho; x++)
            m_indices[y * m_ancho + x] = static_cast<quint8>(q.pixelIndex(x, y) & 15);
}

bool PCFutbolDibujoKit::modificado() const
{
    return m_indices != m_indicesOrig || m_paleta != m_paletaOrig;
}

bool PCFutbolDibujoKit::escribirConBak(const QString &ruta, const QByteArray &datos, QString *error)
{
    const QString bak = ruta + ".bak";
    if (!QFile::exists(bak) && !QFile::copy(ruta, bak))
    {
        if (error) *error = "No se pudo crear la copia de seguridad " + bak;
        return false;
    }
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly) || f.write(datos) != datos.size())
    {
        if (error) *error = "No se pudo escribir " + ruta;
        return false;
    }
    return true;
}

bool PCFutbolDibujoKit::guardar(QString *error, QStringList *informe)
{
    if (!cargado()) return false;

    if (m_indices != m_indicesOrig)
    {
        QByteArray d = m_dfg;
        const int rowbytes = (m_ancho + 7) / 8;
        int pos = 8;
        for (int y = 0; y < m_alto; y++)
        {
            for (int p = 0; p < 4; p++)
                for (int b = 0; b < rowbytes; b++) d[pos + p * rowbytes + b] = 0;
            for (int x = 0; x < m_ancho; x++)
            {
                const int idx = m_indices.at(y * m_ancho + x);
                for (int p = 0; p < 4; p++)
                    if ((idx >> p) & 1) d[pos + p * rowbytes + x / 8] = char(d.at(pos + p * rowbytes + x / 8) | (1 << (7 - x % 8)));
            }
            pos += 4 * rowbytes;
        }
        if (!escribirConBak(m_rutaDfg, d, error)) return false;
        m_dfg = d;
        m_indicesOrig = m_indices;
        if (informe) *informe << "Dibujo guardado en " + QFileInfo(m_rutaDfg).fileName();
    }

    if (m_paleta != m_paletaOrig)
    {
        // Los colores de equipo se escriben en la foto del estadio y en el
        // escudo grande, para que la ficha salga igual tenga o no foto.
        struct Destino { QString ruta; QByteArray *datos; };
        for (Destino dst : { Destino{m_rutaCamp, &m_camp}, Destino{m_rutaEsc, &m_esc} })
        {
            if (dst.ruta.isEmpty() || leerPaletaB(*dst.datos).size() != 16) continue;
            QByteArray d = *dst.datos;
            for (int idx : {1, 2, 8})
            {
                d[2 + 3 * idx]     = char(qRed(m_paleta.at(idx)));
                d[2 + 3 * idx + 1] = char(qGreen(m_paleta.at(idx)));
                d[2 + 3 * idx + 2] = char(qBlue(m_paleta.at(idx)));
            }
            if (!escribirConBak(dst.ruta, d, error)) return false;
            *dst.datos = d;
            if (informe) *informe << "Colores del equipo guardados en " + QFileInfo(dst.ruta).fileName();
        }
        m_paletaOrig = m_paleta;
    }
    return true;
}
