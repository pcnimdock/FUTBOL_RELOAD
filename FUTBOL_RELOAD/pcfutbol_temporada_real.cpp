#include "pcfutbol_temporada_real.h"

#include <QDir>
#include <QFile>
#include <QHash>

namespace {

struct Ref { int equipo; int otro; quint16 jugador; bool suceso; };

class Lector
{
public:
    explicit Lector(const QByteArray &d, int p) : m_d(d), m_p(p) {}
    int pos() const { return m_p; }
    bool ok() const { return m_ok; }
    quint16 u16()
    {
        if (m_p + 2 > m_d.size()) { m_ok = false; return 0; }
        const quint16 v = quint8(m_d.at(m_p)) | (quint8(m_d.at(m_p + 1)) << 8);
        m_p += 2;
        return v;
    }
    void saltar(int n) { m_p += n; if (m_p > m_d.size()) m_ok = false; }
    void texto()
    {
        if (m_p >= m_d.size()) { m_ok = false; return; }
        saltar(1 + quint8(m_d.at(m_p)));
    }
    void textoLargo()
    {
        if (m_p >= m_d.size()) { m_ok = false; return; }
        const int n = quint8(m_d.at(m_p));
        saltar(1);
        for (int i = 0; i <= n && m_ok; i++) texto();
    }
private:
    const QByteArray &m_d;
    int m_p;
    bool m_ok = true;
};

// Lee un partido en 'p'. Devuelve la posición siguiente o -1 si no cuadra.
int leerPartido(const QByteArray &d, int p, QList<Ref> *refs)
{
    Lector l(d, p);
    const int h = l.u16(), a = l.u16();
    if (!l.ok() || h < 1 || a < 1 || h > 255 || a > 255 || h == a) return -1;
    l.saltar(6);            // goles y 4 bytes de fecha
    l.texto();              // árbitro
    l.saltar(4);            // público
    l.texto(); l.texto(); l.texto();
    l.textoLargo();         // crónica
    QList<Ref> r;
    for (int lado = 0; lado < 2 && l.ok(); lado++)
    {
        const int t = lado == 0 ? h : a, o = lado == 0 ? a : h;
        if (l.u16() != t || !l.ok()) return -1;
        for (int k = 0; k < 12; k++)
        {
            const quint16 j = l.u16(), sub = l.u16();
            l.u16();
            if (j != 0xFFFF) r.append({t, o, j, false});
            if (sub != 0xFFFF) r.append({t, o, sub, false});
        }
        for (int guarda = 0; l.ok(); guarda++)
        {
            if (guarda > 500) return -1;
            const quint16 tipo = l.u16();
            if (tipo == 0x63) break;
            const quint16 j = l.u16();
            l.u16();
            if (j != 0xFFFF) r.append({t, o, j, true});
        }
        l.textoLargo();     // declaraciones
    }
    if (!l.ok()) return -1;
    if (refs) refs->append(r);
    return l.pos();
}

QString buscar(const QString &carpeta, const QString &nombre)
{
    for (const QString &f : QDir(carpeta).entryList(QDir::Files))
        if (f.compare(nombre, Qt::CaseInsensitive) == 0) return QDir(carpeta).absoluteFilePath(f);
    return QString();
}

} // namespace

bool PCFutbolTemporadaReal::leerFichero(const QString &ruta, const QList<EQUIPO> &equipos)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly))
    {
        m_error += "No se puede leer " + ruta + ". ";
        return false;
    }
    const QByteArray d = f.readAll();
    QList<Ref> refs;
    bool bien = true;
    int p = 0;
    while (bien && p < d.size() - 2)
    {
        int sig = leerPartido(d, p, &refs);
        if (sig < 0)
        {
            sig = leerPartido(d, p + 2, &refs);   // delante va el número de jornada
            if (sig < 0)
            {
                m_error += QString("%1: formato no reconocido en el byte %2. ").arg(QDir(ruta).dirName()).arg(p);
                bien = false;   // se usa lo leído hasta aquí
                break;
            }
        }
        p = sig;
        m_partidos++;
    }

    // Plantillas por índice de equipo (EQ95000n -> n)
    QHash<int, QSet<quint16>> plantilla;
    for (const EQUIPO &e : equipos)
    {
        const int n = int(e.EquipoIdDBC) - 950000;
        for (const JUGADOR &j : e.lista_jugadores) plantilla[n].insert(j.puntero);
    }
    for (const Ref &r : refs)
    {
        int t = r.equipo;
        // un suceso puede ser de un jugador del otro equipo
        if (r.suceso && !plantilla.value(t).contains(r.jugador) && plantilla.value(r.otro).contains(r.jugador))
            t = r.otro;
        m_claves.insert(qMakePair(quint32(950000 + t), r.jugador));
    }
    return bien;
}

bool PCFutbolTemporadaReal::cargar(const QString &carpetaJuego, const QList<EQUIPO> &equipos)
{
    m_claves.clear();
    m_error.clear();
    m_partidos = 0;
    bool alguno = false;
    for (const QString &nombre : {QString("JORNAD1.DBC"), QString("JORNAD2.DBC")})
    {
        const QString ruta = buscar(carpetaJuego, nombre);
        if (ruta.isEmpty()) continue;
        alguno |= leerFichero(ruta, equipos);
    }
    if (!alguno && m_error.isEmpty()) m_error = "No hay JORNAD1.DBC ni JORNAD2.DBC en la carpeta del juego.";
    return alguno;
}
