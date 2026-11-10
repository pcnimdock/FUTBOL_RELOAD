#include "pcfutbol_partida.h"

#include <QFile>
#include <QtEndian>
#include <cstring>

namespace {

// Lector que no se sale nunca del fichero: si se acaba, marca error y
// devuelve ceros, y la comprobación final rechaza la partida.
struct Lector
{
    const QByteArray &d;
    int o = 0;
    bool mal = false;
    explicit Lector(const QByteArray &datos) : d(datos) {}
    bool hay(int n) { if (o + n > d.size()) { mal = true; return false; } return true; }
    int u8() { if (!hay(1)) return 0; return quint8(d[o++]); }
    int u16() { if (!hay(2)) return 0; int v = quint8(d[o]) | (quint8(d[o + 1]) << 8); o += 2; return v; }
    quint32 u32() { if (!hay(4)) return 0; quint32 v = qFromLittleEndian<quint32>(d.constData() + o); o += 4; return v; }
    double f64()
    {
        if (!hay(8)) return 0;
        double v;
        quint64 b = qFromLittleEndian<quint64>(d.constData() + o);
        std::memcpy(&v, &b, 8);
        o += 8;
        return v;
    }
    void saltar(int n) { if (hay(n)) o += n; }
    QString texto()
    {
        const int n = u8();
        if (!hay(n)) return QString();
        QByteArray s(n, 0);
        for (int i = 0; i < n; i++) s[i] = char(quint8(d[o + i]) ^ 0x61);
        o += n;
        return QString::fromLatin1(s);
    }
};

const int ID_ESTRELLAS = 9900;      // 0x26AC: jugadores sueltos («SEGUNDA B»)

void leerJugador(Lector &r, PCFutbolPartida::Jugador *j)
{
    j->puntero = quint16(r.u16());
    j->nombreCorto = r.texto();
    j->nombreLargo = r.texto();
    const int equipo = r.u16();
    if (equipo == ID_ESTRELLAS) r.texto();
    r.u16();
    const int b0 = r.o;
    r.saltar(7);                    // posición, demarcación, …, dorsal
    if (!r.mal)
    {
        j->posicion = quint8(r.d[b0]);
        j->dorsal = quint8(r.d[b0 + 5]);
        j->alineacion = quint8(r.d[b0 + 6]);
    }
    r.saltar(20);                   // estadísticas por competición
    r.saltar(2);
    j->offValor = r.o;
    j->valor = r.f64();
    j->offMedias = r.o;
    for (int i = 0; i < 4; i++) j->media[i] = r.u8();
    r.saltar(4);
    j->offEnergia = r.o;
    j->energia = r.u8();
}

void leerEquipo(Lector &r, PCFutbolPartida::Equipo *e)
{
    e->nombreLargo = r.texto();
    e->nombreCorto = r.texto();
    r.texto();                      // nombre en mayúsculas
    e->estadio = r.texto();
    r.u16();                        // entrenador
    e->entrenador = r.texto();
    r.u32();
    r.u32();
    r.u16();
    r.u16();
    e->id = r.u16();
    r.saltar(4 * 2 + 4);
    e->nombreTactica = r.texto();
    e->offTactica = r.o;
    if (r.hay(PCFutbolTactica::TAM))
        e->tieneTactica = PCFutbolTactica::deBytes(r.d.mid(r.o, PCFutbolTactica::TAM), &e->tactica);
    r.saltar(PCFutbolTactica::TAM);
    e->offEntrenamiento = r.o;
    for (int i = 0; i < 3; i++) e->entrenamiento[i] = r.u16();
    r.saltar(2 * 2);
    const int offDobles = r.o;
    r.saltar(24 * 8);
    e->offCaja = offDobles + 8;     // el segundo: lo que hay en caja
    if (!r.mal)
    {
        Lector c(r.d);
        c.o = e->offCaja;
        e->caja = c.f64();
    }
    r.saltar(6 * 2 + 11 * 2 + 2 * 2);
}

} // namespace

bool PCFutbolPartida::cargar(const QString &ruta, QString *error)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error) *error = "no se puede abrir " + ruta;
        return false;
    }
    return cargarDatos(f.readAll(), error);
}

bool PCFutbolPartida::cargarDatos(const QByteArray &datos, QString *error)
{
    m_datos = datos;
    m_original = datos;
    m_equipos.clear();
    Lector r(m_datos);
    auto fallo = [&](const QString &donde) {
        if (error) *error = QString("No es una partida de Liga Manager de PC Fútbol 4.5 o está dañada "
                                    "(%1, posición %2).").arg(donde).arg(r.o);
        return false;
    };

    // Cabecera
    int f[7];
    for (int &v : f) v = r.u16();
    m_fecha = QString("%1/%2/%3 %4:%5").arg(f[2], 2, 10, QChar('0')).arg(f[1], 2, 10, QChar('0'))
                  .arg(f[0]).arg(f[6], 2, 10, QChar('0')).arg(f[5], 2, 10, QChar('0'));
    m_tipo = r.u8();
    m_jugador = r.texto();
    m_jornada = r.u8();
    if (r.mal || r.o + 0x46 > m_datos.size() || !m_datos.mid(r.o, 0x46).contains("DINAMIC"))
        return fallo("cabecera");
    r.saltar(0x46);
    r.saltar(8);
    // Calendarios: Primera 45 × 11 partidos, Segunda 38 × 10, de 4 bytes
    r.saltar(45 * 11 * 4 + 38 * 10 * 4);
    // Copas: cabecera, marca y eliminatorias de 10 bytes
    r.saltar(7 * 2);
    if (r.u16() != 1) return fallo("Copa del Rey");
    r.saltar(10 * (15 + 77));
    r.saltar(15 * 2);
    if (r.u16() != 2) return fallo("Copa de Europa");
    r.saltar(10 * 31);
    r.saltar(23 * 2);
    if (r.u16() != 3) return fallo("Recopa");
    r.saltar(10 * 63);
    r.saltar(6 * 2);
    if (r.u16() != 4) return fallo("UEFA");
    r.saltar(10 * (7 + 48));
    if (r.u16() != 5) return fallo("tablas");
    r.saltar(19 * 2);
    // Lista de traspasos: [0, u16, double, u16, double, jugador]… FFFF
    for (int n = 0;; n++)
    {
        const int m = r.u16();
        if (m == 0xFFFF) break;
        if (m != 0 || n > 2000 || r.mal) return fallo("lista de traspasos");
        r.u16(); r.f64(); r.u16(); r.f64();
        Jugador j;
        leerJugador(r, &j);
    }
    r.saltar(42 * 2);               // orden de los equipos en cada división
    // Equipos: 1 + equipo, y 2 + jugador por cada uno; termina en 0
    if (r.u16() != 1) return fallo("primer equipo");
    for (int k = 0; k < 60; k++)
    {
        Equipo e;
        leerEquipo(r, &e);
        int m;
        while ((m = r.u16()) == 2 && !r.mal)
        {
            Jugador j;
            leerJugador(r, &j);
            e.jugadores << j;
        }
        if (r.mal) return fallo("equipo " + e.nombreCorto);
        m_equipos << e;
        if (m == 0) break;
        if (m != 1) return fallo("tras el equipo " + e.nombreCorto);
    }
    if (m_equipos.size() < 42) return fallo("número de equipos");
    return true;
}

static void ponerF64(QByteArray &d, int o, double v)
{
    quint64 b;
    std::memcpy(&b, &v, 8);
    qToLittleEndian<quint64>(b, d.data() + o);
}

void PCFutbolPartida::aplicar()
{
    for (const Equipo &e : m_equipos)
    {
        if (e.offCaja >= 0) ponerF64(m_datos, e.offCaja, e.caja);
        if (e.offEntrenamiento >= 0)
            for (int i = 0; i < 3; i++)
            {
                const int v = qBound(0, e.entrenamiento[i], 0xFFFF);
                m_datos[e.offEntrenamiento + 2 * i] = char(v & 0xFF);
                m_datos[e.offEntrenamiento + 2 * i + 1] = char(v >> 8);
            }
        if (e.offTactica >= 0 && e.tieneTactica && PCFutbolTactica::comprobar(e.tactica).isEmpty())
        {
            const QByteArray b = PCFutbolTactica::aBytes(e.tactica);
            for (int i = 0; i < b.size(); i++) m_datos[e.offTactica + i] = b[i];
        }
        for (const Jugador &j : e.jugadores)
        {
            if (j.offMedias >= 0)
                for (int i = 0; i < 4; i++) m_datos[j.offMedias + i] = char(qBound(0, j.media[i], 255));
            if (j.offEnergia >= 0) m_datos[j.offEnergia] = char(qBound(0, j.energia, 255));
            if (j.offValor >= 0) ponerF64(m_datos, j.offValor, qMax(0.0, j.valor));
        }
    }
}

bool PCFutbolPartida::entrenamientoValido(const int e[3])
{
    int suma = 0;
    for (int i = 0; i < 3; i++)
    {
        if (e[i] < MIN_ENTRENAMIENTO || e[i] > MAX_ENTRENAMIENTO) return false;
        suma += e[i];
    }
    return suma <= SUMA_MAX_ENTRENAMIENTO;
}

bool PCFutbolPartida::cambiada() const
{
    PCFutbolPartida copia = *this;
    copia.aplicar();
    return copia.m_datos != m_original;
}

bool PCFutbolPartida::guardar(const QString &ruta, QString *error)
{
    // Lo que cuelga el juego no se escribe
    QStringList malos;
    for (const Equipo &e : m_equipos)
    {
        if (e.offEntrenamiento < 0) continue;
        // si no se ha tocado, se respeta lo que hubiera
        Lector r(m_original);
        r.o = e.offEntrenamiento;
        const int antes[3] = {r.u16(), r.u16(), r.u16()};
        const bool igual = antes[0] == e.entrenamiento[0] && antes[1] == e.entrenamiento[1] &&
                           antes[2] == e.entrenamiento[2];
        if (!igual && !entrenamientoValido(e.entrenamiento))
            malos << QString("%1 (%2/%3/%4)").arg(e.nombreCorto).arg(e.entrenamiento[0])
                         .arg(e.entrenamiento[1]).arg(e.entrenamiento[2]);
    }
    if (!malos.isEmpty())
    {
        if (error)
            *error = QString("Entrenamiento no válido en: %1. Cada barra va de %2 a %3 y entre las tres "
                             "como mucho %4; si no, el juego se cuelga al abrir Tácticas.")
                         .arg(malos.join(", ")).arg(MIN_ENTRENAMIENTO).arg(MAX_ENTRENAMIENTO)
                         .arg(SUMA_MAX_ENTRENAMIENTO);
        return false;
    }
    aplicar();
    // Comprobación: lo que se va a escribir se vuelve a leer igual
    PCFutbolPartida prueba;
    QString e;
    if (m_datos.size() != m_original.size() || !prueba.cargarDatos(m_datos, &e))
    {
        if (error) *error = "La partida modificada no se puede volver a leer; no se ha guardado. " + e;
        return false;
    }
    if (QFile::exists(ruta) && !QFile::exists(ruta + ".bak") && !QFile::copy(ruta, ruta + ".bak"))
    {
        if (error) *error = "No se pudo crear la copia " + ruta + ".bak";
        return false;
    }
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly) || f.write(m_datos) != m_datos.size())
    {
        if (error) *error = "No se pudo escribir " + ruta;
        return false;
    }
    m_original = m_datos;
    return true;
}
