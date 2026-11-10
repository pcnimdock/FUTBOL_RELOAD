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
    j->inicio = r.o;
    j->puntero = quint16(r.u16());
    j->nombreCorto = r.texto();
    j->nombreLargo = r.texto();
    j->offEquipo = r.o;             // equipo de procedencia (el juego lo pone al fichar)
    const int equipo = r.u16();
    if (equipo == ID_ESTRELLAS) r.texto();
    j->offEquipo2 = r.o;            // equipo actual
    r.u16();
    const int b0 = r.o;
    j->offDatos = b0;
    r.saltar(7);                    // posición, demarcación, extranjero, piel, pelo, dorsal, alineación
    if (!r.mal)
    {
        j->posicion = quint8(r.d[b0]);
        j->extranjero = quint8(r.d[b0 + 2]) != 0;
        j->dorsal = quint8(r.d[b0 + 5]);
        j->alineacion = quint8(r.d[b0 + 6]);
    }
    j->offIncidencias = r.o;        // 5 × [Incidencias, JornIncid, NJornIncid, NAmarillas]
    if (r.hay(20))
        for (int c = 0; c < 5; c++)
        {
            const int o = r.o + 4 * c;
            if (quint8(r.d[o]) == 1 && quint8(r.d[o + 2]) > 0)
            {
                j->lesionado = true;
                j->jornadasBaja = qMax(j->jornadasBaja, int(quint8(r.d[o + 2])));
            }
        }
    r.saltar(20);
    r.saltar(2);
    j->offValor = r.o;
    j->valor = r.f64();
    j->offMedias = r.o;
    for (int i = 0; i < 4; i++) j->media[i] = r.u8();
    for (int i = 0; i < 4; i++) j->aptitud[i] = r.u8();
    j->offEnergia = r.o;
    j->energia = r.u8();
    j->fin = r.o;
}

void leerEquipo(Lector &r, PCFutbolPartida::Equipo *e)
{
    e->inicio = r.o;
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
    e->fin = r.o;
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
    m_copias.clear();
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
        if (!r.mal)
        {
            Lector q(m_datos);
            q.o = j.offEquipo;
            m_copias << CopiaTraspaso{j.puntero, q.u16(), j.offEquipo, j.offEquipo2, j.offIncidencias};
        }
    }
    r.saltar(42 * 2);               // orden de los equipos en cada división
    // Equipos: 1 + equipo, y 2 + jugador por cada uno; termina en 0
    m_inicioEquipos = r.o;
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
        if (m == 0) { m_inicioCola = r.o; break; }
        if (m != 1) return fallo("tras el equipo " + e.nombreCorto);
    }
    if (m_equipos.size() < 42 || m_inicioCola < 0) return fallo("número de equipos");
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
    if (!prueba.cargarDatos(m_datos, &e))
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

// ---------------------------------------------------------------------------
// Cambios de tamaño: renombrar y traspasar
// ---------------------------------------------------------------------------
static void ponerU16(QByteArray &d, int o, int v)
{
    d[o] = char(v & 0xFF);
    d[o + 1] = char((v >> 8) & 0xFF);
}

static QByteArray textoCodificado(const QString &t)
{
    const QByteArray l = t.toLatin1();
    QByteArray b;
    b.append(char(l.size()));
    for (char c : l) b.append(char(quint8(c) ^ 0x61));
    return b;
}

static bool textoValido(const QString &t, int maximo, const QString &que, QString *error)
{
    if (t.trimmed().isEmpty())
    {
        if (error) *error = que + " vacío";
        return false;
    }
    if (t.size() > maximo)
    {
        if (error) *error = QString("%1: como mucho %2 letras (es lo más largo que hay en el juego)").arg(que).arg(maximo);
        return false;
    }
    for (QChar c : t)
        if (c.unicode() < 0x20 || c.unicode() > 0xFF)
        {
            if (error) *error = QString("%1: la letra «%2» no existe en el juego").arg(que, QString(c));
            return false;
        }
    return true;
}

bool PCFutbolPartida::reconstruir(const QList<QList<QByteArray>> &jugadores, const QList<QByteArray> &cabeceras,
                                  QString *error)
{
    QByteArray n = m_datos.left(m_inicioEquipos);
    for (int k = 0; k < cabeceras.size(); k++)
    {
        n.append(char(1)); n.append(char(0));
        n.append(cabeceras[k]);
        for (const QByteArray &j : jugadores[k])
        {
            n.append(char(2)); n.append(char(0));
            n.append(j);
        }
    }
    n.append(char(0)); n.append(char(0));
    n.append(m_datos.mid(m_inicioCola));
    const QByteArray original = m_original, antes = m_datos;
    QString e;
    if (!cargarDatos(n, &e))
    {
        // no debería pasar: se deja todo como estaba
        cargarDatos(antes, nullptr);
        m_original = original;
        if (error) *error = "Error interno al rehacer la partida: " + e;
        return false;
    }
    m_original = original;          // para que «cambiada()» lo detecte
    return true;
}

bool PCFutbolPartida::renombrarJugador(int equipo, int jugador, const QString &corto, const QString &largo,
                                       QString *error)
{
    if (equipo < 0 || equipo >= m_equipos.size() || jugador < 0 || jugador >= m_equipos[equipo].jugadores.size())
        return false;
    if (!textoValido(corto, 16, "Nombre corto", error) || !textoValido(largo, 40, "Nombre largo", error))
        return false;
    aplicar();
    QList<QList<QByteArray>> js;
    QList<QByteArray> cs;
    for (int k = 0; k < m_equipos.size(); k++)
    {
        const Equipo &e = m_equipos[k];
        cs << m_datos.mid(e.inicio, e.fin - e.inicio);
        QList<QByteArray> l;
        for (int i = 0; i < e.jugadores.size(); i++)
        {
            const Jugador &j = e.jugadores[i];
            QByteArray r = m_datos.mid(j.inicio, j.fin - j.inicio);
            if (k == equipo && i == jugador)
            {
                QByteArray nuevo = r.left(2);
                nuevo += textoCodificado(corto);
                nuevo += textoCodificado(largo);
                nuevo += r.mid(j.offEquipo - j.inicio);
                r = nuevo;
            }
            l << r;
        }
        js << l;
    }
    return reconstruir(js, cs, error);
}

bool PCFutbolPartida::traspasar(int origen, int jugador, int destino, QString *info, QString *error)
{
    auto fallo = [&](const QString &t) { if (error) *error = t; return false; };
    if (origen < 0 || origen >= m_equipos.size() || destino < 0 || destino >= m_equipos.size() ||
        jugador < 0 || jugador >= m_equipos[origen].jugadores.size())
        return fallo("Equipo o jugador no válido.");
    const Equipo &eo = m_equipos[origen], &ed = m_equipos[destino];
    if (origen == destino) return fallo("El jugador ya está en ese equipo.");
    if (eo.id == ID_ESTRELLAS || ed.id == ID_ESTRELLAS)
        return fallo("Los jugadores sin equipo (Estrellas / Segunda B) se guardan de otra forma; "
                     "de momento no se pueden mover.");
    if (eo.jugadores.size() - 1 < MIN_PLANTILLA)
        return fallo(QString("%1 se quedaría con %2 jugadores (mínimo %3).")
                         .arg(eo.nombreCorto).arg(eo.jugadores.size() - 1).arg(MIN_PLANTILLA));
    if (ed.jugadores.size() + 1 > MAX_PLANTILLA)
        return fallo(QString("%1 tendría %2 jugadores (máximo %3).")
                         .arg(ed.nombreCorto).arg(ed.jugadores.size() + 1).arg(MAX_PLANTILLA));
    const Jugador &jm = eo.jugadores[jugador];
    for (const Jugador &j : ed.jugadores)
        if (j.puntero == jm.puntero)
            return fallo(QString("%1 ya tiene un jugador con la misma identidad (%2).").arg(ed.nombreCorto, j.nombreCorto));

    aplicar();
    // Dorsal: el suyo si está libre en el destino; si no, el primero libre
    QList<int> usados;
    for (const Jugador &j : ed.jugadores) usados << j.dorsal;
    int dorsal = jm.dorsal;
    if (usados.contains(dorsal))
        for (dorsal = 1; usados.contains(dorsal); dorsal++) {}
    const int alineacion = ed.jugadores.size() + 1;

    // Copia en la lista de traspasos (está antes de los equipos: mismo sitio)
    int copias = 0;
    for (const CopiaTraspaso &c : m_copias)
        if (c.puntero == jm.puntero && c.equipo == eo.id)
        {
            ponerU16(m_datos, c.offEquipo, ed.id);
            ponerU16(m_datos, c.offEquipo2, ed.id);
            copias++;
        }

    // El registro movido, ya con su equipo, dorsal y puesto nuevos
    QByteArray movido = m_datos.mid(jm.inicio, jm.fin - jm.inicio);
    // Como hace el juego al fichar: el primer u16 queda con el equipo de
    // procedencia y el segundo con el equipo actual
    ponerU16(movido, jm.offEquipo - jm.inicio, eo.id);
    ponerU16(movido, jm.offEquipo2 - jm.inicio, ed.id);
    movido[jm.offDatos - jm.inicio + 5] = char(dorsal);
    movido[jm.offDatos - jm.inicio + 6] = char(alineacion);

    QList<QList<QByteArray>> js;
    QList<QByteArray> cs;
    for (int k = 0; k < m_equipos.size(); k++)
    {
        const Equipo &e = m_equipos[k];
        QByteArray cab = m_datos.mid(e.inicio, e.fin - e.inicio);
        const int offNum = e.offEntrenamiento - e.inicio + 8;     // nº de jugadores
        if (k == origen) ponerU16(cab, offNum, e.jugadores.size() - 1);
        if (k == destino) ponerU16(cab, offNum, e.jugadores.size() + 1);
        cs << cab;
        QList<QByteArray> l;
        for (int i = 0; i < e.jugadores.size(); i++)
        {
            if (k == origen && i == jugador) continue;
            const Jugador &j = e.jugadores[i];
            QByteArray r = m_datos.mid(j.inicio, j.fin - j.inicio);
            if (k == origen && j.alineacion > jm.alineacion)
                r[j.offDatos - j.inicio + 6] = char(j.alineacion - 1);
            l << r;
        }
        if (k == destino) l << movido;
        js << l;
    }

    const QString nombre = jm.nombreCorto, de = eo.nombreCorto, a = ed.nombreCorto;
    const int dorsalAntes = jm.dorsal;
    if (!reconstruir(js, cs, error)) return false;
    if (info)
    {
        *info = QString("%1: %2 → %3, puesto %4 de la alineación").arg(nombre, de, a).arg(alineacion);
        if (dorsal != dorsalAntes) *info += QString(", dorsal %1 (el %2 estaba ocupado)").arg(dorsal).arg(dorsalAntes);
        if (copias) *info += ", también en la lista de traspasos";
    }
    return true;
}

void PCFutbolPartida::quitarBajas(int equipo, int jugador)
{
    if (equipo < 0 || equipo >= m_equipos.size() || jugador < 0 || jugador >= m_equipos[equipo].jugadores.size())
        return;
    Jugador &j = m_equipos[equipo].jugadores[jugador];
    if (j.offIncidencias < 0) return;
    for (int i = 0; i < 20; i++) m_datos[j.offIncidencias + i] = 0;
    for (const CopiaTraspaso &c : m_copias)
        if (c.puntero == j.puntero && c.equipo == m_equipos[equipo].id && c.offIncidencias >= 0)
            for (int i = 0; i < 20; i++) m_datos[c.offIncidencias + i] = 0;
    j.lesionado = false;
    j.jornadasBaja = 0;
}
