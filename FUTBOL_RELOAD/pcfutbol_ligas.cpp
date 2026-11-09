#include "pcfutbol_ligas.h"
#include "bbdd_futbol.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QtEndian>

namespace {

quint32 u32(const QByteArray &d, qint64 o)
{
    if (o < 0 || o + 4 > d.size()) return 0;
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(d.constData()) + o);
}

QString rutaSinMayusculas(const QDir &dir, const QString &nombre)
{
    // La carpeta del juego viene de DOS: los nombres pueden estar en
    // mayúsculas o minúsculas según cómo se copió.
    const QStringList hay = dir.entryList(QDir::Files);
    for (const QString &f : hay)
        if (f.compare(nombre, Qt::CaseInsensitive) == 0) return dir.filePath(f);
    return dir.filePath(nombre);
}

} // namespace

// ---------------------------------------------------------------------------
// Formato LE (Linear Executable de DOS4GW)
// ---------------------------------------------------------------------------
bool PCFutbolLigas::leerLE(const QString &ruta, ImagenLE *img, QString *error)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error) *error = "No se puede abrir " + ruta;
        return false;
    }
    img->ruta = ruta;
    img->nombre = QFileInfo(ruta).fileName();
    img->fichero = f.readAll();
    f.close();
    const QByteArray &d = img->fichero;

    const quint32 le = u32(d, 0x3C);
    if (d.size() < 0x40 || le + 0xB0 > quint32(d.size()) || d.mid(le, 2) != "LE")
    {
        if (error) *error = img->nombre + " no es un ejecutable LE (DOS4GW)";
        return false;
    }
    img->nPaginas = u32(d, le + 0x14);
    img->tamPagina = u32(d, le + 0x28);
    img->bytesUltimaPagina = u32(d, le + 0x2C);
    const quint32 tablaObj = u32(d, le + 0x40);
    const quint32 nObj = u32(d, le + 0x44);
    const quint32 mapaPag = u32(d, le + 0x48);
    const quint32 datos = u32(d, le + 0x80);
    if (img->tamPagina == 0 || nObj == 0 || nObj > 16)
    {
        if (error) *error = img->nombre + ": cabecera LE no válida";
        return false;
    }
    for (quint32 i = 0; i < nObj; i++)
    {
        const qint64 o = le + tablaObj + 24 * i;
        ImagenLE::Objeto ob;
        ob.tam = u32(d, o);
        ob.base = u32(d, o + 4);
        const quint32 pIdx = u32(d, o + 12);
        const quint32 pCnt = u32(d, o + 16);
        for (quint32 k = 0; k < pCnt; k++)
        {
            const qint64 e = le + mapaPag + 4 * qint64(pIdx - 1 + k);
            if (e + 4 > d.size()) break;
            const quint32 num = (quint8(d.at(e)) << 16) | (quint8(d.at(e + 1)) << 8) | quint8(d.at(e + 2));
            ob.paginas.append(qint64(datos) + qint64(num - 1) * img->tamPagina);
        }
        img->objetos.append(ob);
    }
    return true;
}

qint64 PCFutbolLigas::offsetFichero(const ImagenLE &img, quint32 va)
{
    for (const ImagenLE::Objeto &ob : img.objetos)
    {
        if (va < ob.base || va >= ob.base + ob.tam) continue;
        const quint32 rel = va - ob.base;
        const int pag = int(rel / img.tamPagina);
        if (pag >= ob.paginas.size()) return -1;
        const qint64 off = ob.paginas.at(pag) + rel % img.tamPagina;
        return (off < img.fichero.size()) ? off : -1;
    }
    return -1;
}

bool PCFutbolLigas::leerU32(const ImagenLE &img, quint32 va, quint32 *v)
{
    quint32 r = 0;
    for (int b = 0; b < 4; b++) // byte a byte: una tabla puede cruzar páginas
    {
        const qint64 o = offsetFichero(img, va + b);
        if (o < 0) return false;
        r |= quint32(quint8(img.fichero.at(o))) << (8 * b);
    }
    *v = r;
    return true;
}

// ---------------------------------------------------------------------------
// Nombres y datos de cada equipo (DBDAT/EQ95xxxx.DBC)
// ---------------------------------------------------------------------------
void PCFutbolLigas::leerEquipos(const QString &carpetaJuego)
{
    m_equipos.clear();
    BBDD_FUTBOL db;
    QDir dbdat(carpetaJuego + "/DBDAT");
    QDir camisas(carpetaJuego + "/DBDAT/CAMISAS");
    const QStringList dbcs = dbdat.entryList(QStringList() << "EQ95????.DBC" << "eq95????.dbc", QDir::Files);
    QSet<QString> dfg;
    for (const QString &c : camisas.entryList(QDir::Files)) dfg.insert(c.toUpper());

    for (const QString &nombre : dbcs)
    {
        bool ok = false;
        const quint32 id = nombre.mid(4, 4).toUInt(&ok);
        if (!ok) continue;
        QFile f(dbdat.filePath(nombre));
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QByteArray d = f.read(0x100);
        f.close();
        if (d.size() < 0x30) continue;

        Equipo e;
        e.id = id;
        e.fichaCompleta = (d.at(0x2A) == 0);
        e.tieneCamisa = dfg.contains(QString("EQ95%1.DFG").arg(id, 4, 10, QChar('0')));
        // 0x2C: nombre largo (u8 longitud + texto); a continuación el corto
        int p = 0x2C;
        QString largo, corto;
        int n = quint8(d.at(p++));
        for (int i = 0; i < n && p < d.size(); i++) largo.append(db.descodificar_caracter(quint8(d.at(p++))));
        if (p < d.size())
        {
            n = quint8(d.at(p++));
            for (int i = 0; i < n && p < d.size(); i++) corto.append(db.descodificar_caracter(quint8(d.at(p++))));
        }
        e.nombre = corto.trimmed().isEmpty() ? largo.trimmed() : corto.trimmed();
        m_equipos.insert(id, e);
    }
}

// ---------------------------------------------------------------------------
// Localización de las tablas
// ---------------------------------------------------------------------------
bool PCFutbolLigas::buscarListaCanonica(const ImagenLE &img)
{
    const int N = N_PRIMERA + N_SEGUNDA; // 42
    // Recorre todas las posiciones alineadas de los objetos y apunta las
    // secuencias de 42 IDs distintos y existentes.
    QMap<QVector<quint32>, QVector<quint32>> apariciones; // secuencia -> VAs
    for (const ImagenLE::Objeto &ob : img.objetos)
    {
        for (quint32 va = ob.base; va + 4 * N <= ob.base + ob.tam; va += 4)
        {
            QVector<quint32> sec;
            QSet<quint32> vistos;
            for (int i = 0; i < N; i++)
            {
                quint32 v = 0;
                if (!leerU32(img, va + 4 * i, &v) || !m_equipos.contains(v) || vistos.contains(v)) break;
                vistos.insert(v);
                sec.append(v);
            }
            if (sec.size() == N) apariciones[sec].append(va);
        }
    }
    for (auto it = apariciones.constBegin(); it != apariciones.constEnd(); ++it)
    {
        if (it.value().size() < 2) continue;
        // La lista larga es la aparición que sigue con más IDs hasta 0xFFFF
        for (quint32 va : it.value())
        {
            QVector<quint32> larga = it.key();
            quint32 p = va + 4 * N, v = 0;
            while (leerU32(img, p, &v) && m_equipos.contains(v) && larga.size() < 400)
            {
                larga.append(v);
                p += 4;
            }
            if (v == 0xFFFF && larga.size() > N)
            {
                m_lista = larga;
                return true;
            }
        }
    }
    return false;
}

void PCFutbolLigas::buscarTablas(const ImagenLE &img)
{
    // Conjuntos de cada división según la lista canónica
    const QSet<quint32> primera(m_lista.begin(), m_lista.begin() + N_PRIMERA);
    const QSet<quint32> segunda(m_lista.begin() + N_PRIMERA, m_lista.begin() + N_PRIMERA + N_SEGUNDA);
    const QSet<quint32> segundaB(m_lista.begin() + N_PRIMERA + N_SEGUNDA, m_lista.end());
    auto division = [&](int i) -> const QSet<quint32> & {
        return i < N_PRIMERA ? primera : (i < N_PRIMERA + N_SEGUNDA ? segunda : segundaB);
    };

    for (const ImagenLE::Objeto &ob : img.objetos)
    {
        for (quint32 va = ob.base; va + 4 * N_PRIMERA <= ob.base + ob.tam; va += 4)
        {
            QVector<quint32> vals;
            QSet<quint32> vistosPrimera;
            quint32 v = 0;
            while (vals.size() < m_lista.size() && leerU32(img, va + 4 * vals.size(), &v)
                   && division(vals.size()).contains(v))
            {
                if (vals.size() < N_PRIMERA)
                {
                    if (vistosPrimera.contains(v)) break;
                    vistosPrimera.insert(v);
                }
                vals.append(v);
            }
            // Tramos válidos: los 22 de Primera completos y, si sigue, la
            // Segunda completa (no se aceptan divisiones a medias)
            int n = vals.size();
            if (n < N_PRIMERA) continue;
            if (n < N_PRIMERA + N_SEGUNDA) n = N_PRIMERA;
            vals.resize(n);
            Tabla t;
            t.fichero = img.nombre;
            t.va = va;
            t.longitud = n;
            t.valores = vals;
            for (int b = 0; b < 4 * n; b++) t.offsets.append(offsetFichero(img, va + b));
            m_tablas.append(t);

            // PCFUTBOL.DAT: "22 de Primera, 0xFFFF, 20 de Segunda, 0xFFFF".
            // El tramo de Segunda va aparte, tras el separador.
            if (n == N_PRIMERA && leerU32(img, va + 4 * N_PRIMERA, &v) && v == 0xFFFF)
            {
                const quint32 va2 = va + 4 * (N_PRIMERA + 1);
                QVector<quint32> seg;
                while (seg.size() < N_SEGUNDA && leerU32(img, va2 + 4 * seg.size(), &v) && segunda.contains(v))
                    seg.append(v);
                if (seg.size() == N_SEGUNDA)
                {
                    Tabla t2;
                    t2.fichero = img.nombre;
                    t2.va = va2;
                    t2.longitud = N_SEGUNDA;
                    t2.valores = seg;
                    for (int b = 0; b < 4 * N_SEGUNDA; b++) t2.offsets.append(offsetFichero(img, va2 + b));
                    m_tablas.append(t2);
                    n += 1 + N_SEGUNDA;
                }
            }
            va += 4 * (n - 1);
        }
    }
}

void PCFutbolLigas::buscarTablaEuropa(const ImagenLE &img)
{
    // Busca "4, ids, FFFF, 3, ids, FFFF, 2, ids, FFFF, FFFF" con IDs de
    // equipos existentes y como mucho 15 u32 (lo que reserva el juego).
    static const quint32 codigos[3] = {4, 3, 2};
    for (const ImagenLE::Objeto &ob : img.objetos)
    {
        for (quint32 va = ob.base; va + 4 * 15 <= ob.base + ob.tam; va += 4)
        {
            QVector<quint32> todo;
            QVector<QVector<quint32>> grupos;
            quint32 p = va, v = 0;
            bool ok = true;
            for (int c = 0; c < 3 && ok; c++)
            {
                if (!leerU32(img, p, &v) || v != codigos[c]) { ok = false; break; }
                todo.append(v); p += 4;
                QVector<quint32> ids;
                while (leerU32(img, p, &v) && v != 0xFFFF && m_equipos.contains(v) && ids.size() < 8)
                {
                    ids.append(v); todo.append(v); p += 4;
                }
                if (v != 0xFFFF || ids.isEmpty()) { ok = false; break; }
                todo.append(v); p += 4;
                grupos.append(ids);
            }
            if (!ok || !leerU32(img, p, &v) || v != 0xFFFF) continue;
            todo.append(v);
            if (todo.size() > 15) continue;

            Tabla t;
            t.fichero = img.nombre;
            t.va = va;
            t.longitud = todo.size();
            t.valores = todo;
            for (int b = 0; b < 4 * t.longitud; b++) t.offsets.append(offsetFichero(img, va + b));
            m_tablasEuropa.append(t);
            if (m_europa.isEmpty()) m_europa = grupos;
            va += 4 * (todo.size() - 1);
        }
    }
}

QString PCFutbolLigas::nombreCompeticion(int c)
{
    switch (c)
    {
    case CopaEuropa: return "Copa de Europa";
    case Recopa:     return "Recopa";
    default:         return "UEFA";
    }
}

void PCFutbolLigas::setEuropa(int competicion, int puesto, quint32 id)
{
    if (competicion < 0 || competicion >= m_europa.size()) return;
    if (puesto < 0 || puesto >= m_europa[competicion].size()) return;
    m_europa[competicion][puesto] = id;
}

bool PCFutbolLigas::cargar(const QString &carpetaJuego, QString *error)
{
    m_carpeta = carpetaJuego;
    m_lista.clear();
    m_tablas.clear();
    m_tablasEuropa.clear();
    m_europa.clear();
    leerEquipos(carpetaJuego);
    if (m_equipos.size() < N_PRIMERA + N_SEGUNDA)
    {
        if (error) *error = "No se han encontrado los DBC de los equipos en " + carpetaJuego + "/DBDAT";
        return false;
    }

    const QDir dir(carpetaJuego);
    ImagenLE manager;
    if (!leerLE(rutaSinMayusculas(dir, "PCMANAG.DAT"), &manager, error)) return false;
    if (!buscarListaCanonica(manager))
    {
        if (error) *error = "No se ha encontrado la tabla de composición de ligas en PCMANAG.DAT.\n"
                            "¿Es una versión de PC Fútbol distinta de la 4.0/4.5, o un ejecutable ya modificado?";
        return false;
    }
    buscarTablas(manager);
    buscarTablaEuropa(manager);

    // PCFUTBOL.DAT NO se toca: su tabla debe seguir coincidiendo con el
    // calendario real 95/96 (JORNAD*.DBC) o el juego falla. Ver el .h.
    for (const QString &otro : {QString("PROMANAG.DAT")})
    {
        const QString ruta = rutaSinMayusculas(dir, otro);
        if (!QFile::exists(ruta)) continue;
        ImagenLE img;
        if (leerLE(ruta, &img, nullptr)) { buscarTablas(img); buscarTablaEuropa(img); }
    }
    m_listaOriginal = m_lista;
    m_europaOriginal = m_europa;
    m_permutacion.clear();
    for (quint32 id : m_lista) m_permutacion.insert(id, id);
    return true;
}

// ---------------------------------------------------------------------------
// Edición y guardado
// ---------------------------------------------------------------------------
void PCFutbolLigas::intercambiar(quint32 a, quint32 b)
{
    if (a == b) return;
    for (quint32 &v : m_lista)
    {
        if (v == a) v = b;
        else if (v == b) v = a;
    }
    for (auto it = m_permutacion.begin(); it != m_permutacion.end(); ++it)
    {
        if (it.value() == a) it.value() = b;
        else if (it.value() == b) it.value() = a;
    }
}

void PCFutbolLigas::deshacerTodo()
{
    m_lista = m_listaOriginal;
    m_europa = m_europaOriginal;
    for (auto it = m_permutacion.begin(); it != m_permutacion.end(); ++it) it.value() = it.key();
}

bool PCFutbolLigas::guardar(QString *error, QStringList *informe)
{
    const QDir dir(m_carpeta);
    QMap<QString, QVector<Tabla>> porFichero;
    for (const Tabla &t : m_tablas) porFichero[t.fichero].append(t);
    // Tabla de Europa: misma forma, con los equipos elegidos (sin permutar)
    const bool europaCambiada = (m_europa != m_europaOriginal);
    QMap<QString, QVector<Tabla>> europaPorFichero;
    if (europaCambiada)
    {
        for (Tabla t : m_tablasEuropa)
        {
            QVector<quint32> nueva;
            for (int c = 0; c < m_europa.size(); c++)
            {
                nueva.append(c == 0 ? 4u : (c == 1 ? 3u : 2u));
                nueva += m_europa.at(c);
                nueva.append(0xFFFF);
            }
            nueva.append(0xFFFF);
            if (nueva.size() != t.longitud) continue; // nunca debería pasar
            t.valores = nueva;
            europaPorFichero[t.fichero].append(t);
            if (!porFichero.contains(t.fichero)) porFichero[t.fichero] = {};
        }
    }

    for (auto it = porFichero.constBegin(); it != porFichero.constEnd(); ++it)
    {
        const QString ruta = rutaSinMayusculas(dir, it.key());
        const QString bak = ruta + ".bak";
        if (!QFile::exists(bak) && !QFile::copy(ruta, bak))
        {
            if (error) *error = "No se ha podido crear la copia de seguridad " + bak + ". No se ha modificado nada.";
            return false;
        }
        QFile f(ruta);
        if (!f.open(QIODevice::ReadWrite))
        {
            if (error) *error = "No se puede escribir en " + ruta;
            return false;
        }
        QByteArray d = f.readAll();
        for (const Tabla &t : it.value())
        {
            for (int i = 0; i < t.longitud; i++)
            {
                const quint32 v = m_permutacion.value(t.valores.at(i), t.valores.at(i));
                for (int b = 0; b < 4; b++)
                {
                    const qint64 o = t.offsets.at(4 * i + b);
                    if (o >= 0 && o < d.size()) d[o] = char((v >> (8 * b)) & 0xFF);
                }
            }
            if (informe)
                *informe << QString("%1: tabla en 0x%2 (%3 equipos)")
                                .arg(it.key()).arg(t.va, 0, 16).arg(t.longitud);
        }
        for (const Tabla &t : europaPorFichero.value(it.key()))
        {
            for (int i = 0; i < t.longitud; i++)
                for (int b = 0; b < 4; b++)
                {
                    const qint64 o = t.offsets.at(4 * i + b);
                    if (o >= 0 && o < d.size()) d[o] = char((t.valores.at(i) >> (8 * b)) & 0xFF);
                }
            if (informe)
                *informe << QString("%1: clasificados para Europa en 0x%2").arg(it.key()).arg(t.va, 0, 16);
        }
        f.seek(0);
        const bool ok = (f.write(d) == d.size());
        f.close();
        if (!ok)
        {
            if (error) *error = "Error al escribir " + ruta + ". Tienes la copia en " + bak;
            return false;
        }
    }
    // Relee para que las tablas reflejen lo que hay ahora en disco
    return cargar(m_carpeta, error);
}

bool PCFutbolLigas::pcfutbolModificadoAntes() const
{
    const QDir dir(m_carpeta);
    const QString ruta = rutaSinMayusculas(dir, "PCFUTBOL.DAT");
    QFile a(ruta), b(ruta + ".bak");
    if (!b.exists() || !a.open(QIODevice::ReadOnly) || !b.open(QIODevice::ReadOnly)) return false;
    return a.readAll() != b.readAll();
}

bool PCFutbolLigas::restaurarPcfutbol(QString *error) const
{
    const QDir dir(m_carpeta);
    const QString ruta = rutaSinMayusculas(dir, "PCFUTBOL.DAT");
    QFile b(ruta + ".bak");
    if (!b.open(QIODevice::ReadOnly))
    {
        if (error) *error = "No se puede leer " + ruta + ".bak";
        return false;
    }
    const QByteArray datos = b.readAll();
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly) || f.write(datos) != datos.size())
    {
        if (error) *error = "No se puede escribir " + ruta;
        return false;
    }
    return true;
}
