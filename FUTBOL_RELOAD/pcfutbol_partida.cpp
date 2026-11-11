#include "pcfutbol_partida.h"

#include <QFile>
#include <QSet>
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

void leerJugador(Lector &r, PCFutbolPartida::Jugador *j, int jornadaActual)
{
    j->inicio = r.o;
    j->puntero = quint16(r.u16());
    j->nombreCorto = r.texto();
    j->nombreLargo = r.texto();
    j->offEquipo = r.o;             // equipo de procedencia (el juego lo pone al fichar)
    const int equipo = r.u16();
    j->procedencia = equipo;
    if (equipo == ID_ESTRELLAS) j->clubProcedencia = r.texto();
    j->offEquipo2 = r.o;            // equipo actual
    r.u16();
    const int b0 = r.o;
    j->offDatos = b0;
    r.saltar(7);                    // posición, demarcación, extranjero, piel, pelo, dorsal, alineación
    if (!r.mal)
    {
        j->posicion = quint8(r.d[b0]);
        j->demarcacion = quint8(r.d[b0 + 1]);
        j->extranjero = quint8(r.d[b0 + 2]) != 0;
        j->piel = quint8(r.d[b0 + 3]);
        j->pelo = quint8(r.d[b0 + 4]);
        j->dorsal = quint8(r.d[b0 + 5]);
        j->alineacion = quint8(r.d[b0 + 6]);
    }
    j->offDorsal = b0 + 5;
    j->offAlineacion = b0 + 6;
    j->offIncidencias = r.o;        // 5 × [Incidencias, JornIncid, NJornIncid, NAmarillas]
    for (int c = 0; c < 5; c++) j->offGrupo[c] = r.o + 4 * c;
    if (r.hay(20))
        for (int c = 0; c < 5; c++)
        {
            const int o = r.o + 4 * c;
            const int tipo = quint8(r.d[o]);
            if (tipo == 0) continue;
            (tipo == 1 ? j->lesionado : j->sancionado) = true;
            const int faltan = quint8(r.d[o + 2]) + 1 - (jornadaActual - quint8(r.d[o + 1]));
            j->jornadasBaja = qMax(j->jornadasBaja, faltan);
        }
    r.saltar(20);
    r.saltar(2);
    j->offValor = r.o;
    j->valor = r.f64();
    j->offMedias = r.o;
    for (int i = 0; i < 4; i++) j->media[i] = r.u8();
    j->offAptitudes = r.o;
    for (int i = 0; i < 4; i++) j->aptitud[i] = r.u8();
    j->offEnergia = r.o;
    j->energia = r.u8();
    j->fin = r.o;
}

// Jugador de Promanager (PROMANAG.DAT 0x36173). Campos cruzados con el DBC
// en 1.027 jugadores: 7 u8 (posición, …), 30 u8 (…, extranjero, piel, pelo,
// dorsal, alineación, …), u16 equipo actual, u16, u16 nº de orden (1..N),
// double precio, double, u32, 11 u8 (VE RE AG CA, 4 aptitudes, 3 más; el
// último es la energía).
void leerJugadorPro(Lector &r, PCFutbolPartida::Jugador *j, int jornadaActual)
{
    j->inicio = r.o;
    j->puntero = quint16(r.u16());
    j->nombreCorto = r.texto();
    j->nombreLargo = r.texto();
    j->offEquipo = r.o;
    const int equipo = r.u16();
    j->procedencia = equipo;
    if (equipo == ID_ESTRELLAS) j->clubProcedencia = r.texto();
    const int b7 = r.o;
    r.saltar(7);
    const int b30 = r.o;
    r.saltar(30);
    if (!r.mal)
        for (int g : {8, 14, 18, 22, 26})
        {
            const int o = b30 + g;
            const int tipo = quint8(r.d[o]);
            if (tipo == 0) continue;
            (tipo == 1 ? j->lesionado : j->sancionado) = true;
            const int faltan = quint8(r.d[o + 2]) + 1 - (jornadaActual - quint8(r.d[o + 1]));
            j->jornadasBaja = qMax(j->jornadasBaja, faltan);
        }
    if (!r.mal)
    {
        j->posicion = quint8(r.d[b7]);
        j->extranjero = quint8(r.d[b30 + 1]) != 0;
        j->piel = quint8(r.d[b30 + 2]);
        j->pelo = quint8(r.d[b30 + 3]);
        j->dorsal = quint8(r.d[b30 + 4]);
        j->alineacion = quint8(r.d[b30 + 5]);
    }
    // Incidencias por competición: 5 grupos de [Incidencias, JornIncid,
    // NJornIncid, NAmarillas] en b30+8, +14, +18, +22 y +26 (entre el
    // primero y el segundo, 2 bytes: el 13 es el minuto de la última tarjeta)
    const int grupos[5] = {8, 14, 18, 22, 26};
    for (int c = 0; c < 5; c++) j->offGrupo[c] = b30 + grupos[c];
    j->offIncidencias = b30 + 8;
    j->offDatos = b7;
    j->offDorsal = b30 + 4;
    j->offAlineacion = b30 + 5;
    j->offEquipo2 = r.o;            // equipo actual
    r.u16();
    r.u16();
    j->offOrden = r.o;
    r.u16();
    j->offValor = r.o;
    j->valor = r.f64();
    r.f64();
    r.u32();
    j->offMedias = r.o;
    for (int i = 0; i < 4; i++) j->media[i] = r.u8();
    j->offAptitudes = r.o;
    for (int i = 0; i < 4; i++) j->aptitud[i] = r.u8();
    r.saltar(2);
    j->offEnergia = r.o;
    j->energia = r.u8();
    j->fin = r.o;
}

// Equipo de Promanager (PROMANAG.DAT 0x26453): 5 textos, 2 u16, 3 u8, 3 u16,
// 2 u32, táctica, u16, u16 nº de jugadores, entrenador (u16 + texto), u16
// (1 = 48 registros de 84 bytes del mánager humano), 7 double ([1] caja),
// 9 u8, 11 u16, 4 u16. No lleva entrenamiento.
void leerEquipoPro(Lector &r, PCFutbolPartida::Equipo *e)
{
    e->inicio = r.o;
    e->nombreLargo = r.texto();
    e->nombreCorto = r.texto();
    r.texto();                      // mayúsculas
    r.texto();                      // presidente
    e->estadio = r.texto();
    e->id = r.u16();
    r.u16();
    r.saltar(3);
    r.saltar(3 * 2 + 2 * 4);
    e->nombreTactica = r.texto();
    e->offTactica = r.o;
    if (r.hay(PCFutbolTactica::TAM))
        e->tieneTactica = PCFutbolTactica::deBytes(r.d.mid(r.o, PCFutbolTactica::TAM), &e->tactica);
    r.saltar(PCFutbolTactica::TAM);
    r.u16();
    e->offNumJugadores = r.o;
    r.u16();
    r.u16();
    e->entrenador = r.texto();
    if (r.u16() != 0) r.saltar(48 * 84);
    const int offDobles = r.o;
    r.saltar(7 * 8);
    e->offCaja = offDobles + 8;
    if (!r.mal)
    {
        Lector c(r.d);
        c.o = e->offCaja;
        e->caja = c.f64();
    }
    r.saltar(9 + 11 * 2 + 4 * 2);
    e->fin = r.o;
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
    r.saltar(2);
    e->offNumJugadores = r.o;
    r.saltar(2);
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
        if (error) *error = QString("No es una partida de Liga Manager o Promanager de PC Fútbol 4.5, "
                                    "o está dañada o incompleta "
                                    "(%1, posición %2).").arg(donde).arg(r.o);
        return false;
    };

    // Cabecera: fecha (7 u16) y después
    //   Liga Manager: u8 tipo, texto jugador, u8 jornada
    //   Promanager:   8 × (u16 nº de mánager, texto si no es FFFF), u8 jornada
    int f[7];
    for (int &v : f) v = r.u16();
    m_fecha = QString("%1/%2/%3 %4:%5").arg(f[2], 2, 10, QChar('0')).arg(f[1], 2, 10, QChar('0'))
                  .arg(f[0]).arg(f[6], 2, 10, QChar('0')).arg(f[5], 2, 10, QChar('0'));
    const int trasFecha = r.o;
    auto copyrightEn = [&](int o) {
        return o >= 0 && o + 0x46 <= m_datos.size() && m_datos.mid(o, 0x46).contains("DINAMIC");
    };
    m_pro = false;
    m_managers.clear();
    m_tipo = r.u8();
    m_jugador = r.texto();
    m_jornada = r.u8();
    if (r.mal || !copyrightEn(r.o))
    {
        Lector p(m_datos);
        p.o = trasFecha;
        QStringList mans;
        for (int k = 0; k < 8; k++)
        {
            const int id = p.u16();
            if (id != 0xFFFF) mans << p.texto();
        }
        const int jornada = p.u8();
        if (p.mal || !copyrightEn(p.o)) { r.o = trasFecha; return fallo("cabecera"); }
        m_pro = true;
        m_managers = mans;
        m_jugador = mans.join(", ");
        m_jornada = jornada;
        m_tipo = 0;
        r.o = p.o;
    }
    r.saltar(0x46);
    if (r.hay(8)) m_jornadaActual = quint8(m_datos[r.o + 1]);
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
    // (el Promanager no la tiene)
    for (int n = 0; !m_pro; n++)
    {
        const int m = r.u16();
        if (m == 0xFFFF) break;
        if (m != 0 || n > 2000 || r.mal) return fallo("lista de traspasos");
        r.u16(); r.f64(); r.u16(); r.f64();
        Jugador j;
        leerJugador(r, &j, m_jornadaActual);
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
        if (m_pro) leerEquipoPro(r, &e); else leerEquipo(r, &e);
        int m;
        while ((m = r.u16()) == 2 && !r.mal)
        {
            Jugador j;
            if (m_pro) leerJugadorPro(r, &j, m_jornadaActual); else leerJugador(r, &j, m_jornadaActual);
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
    const bool desdeEstrellas = eo.id == ID_ESTRELLAS;
    if (ed.id == ID_ESTRELLAS)
        return fallo("Las Estrellas Mundiales no son un equipo de la liga: el juego nunca vende "
                     "jugadores allí, así que el editor tampoco.");
    if (!desdeEstrellas && eo.jugadores.size() - 1 < MIN_PLANTILLA)
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
            // una estrella conserva su procedencia (9900 + club en texto)
            if (!desdeEstrellas) ponerU16(m_datos, c.offEquipo, ed.id);
            ponerU16(m_datos, c.offEquipo2, ed.id);
            copias++;
        }

    // El registro movido, ya con su equipo, dorsal y puesto nuevos
    QByteArray movido = m_datos.mid(jm.inicio, jm.fin - jm.inicio);
    // Como hace el juego al fichar: el primer u16 queda con el equipo de
    // procedencia y el segundo con el equipo actual
    // (una estrella ya tiene procedencia 9900 con su club: se deja igual)
    if (!desdeEstrellas) ponerU16(movido, jm.offEquipo - jm.inicio, eo.id);
    ponerU16(movido, jm.offEquipo2 - jm.inicio, ed.id);
    movido[jm.offDorsal - jm.inicio] = char(dorsal);
    movido[jm.offAlineacion - jm.inicio] = char(alineacion);
    if (jm.offOrden >= 0) ponerU16(movido, jm.offOrden - jm.inicio, alineacion);

    QList<QList<QByteArray>> js;
    QList<QByteArray> cs;
    for (int k = 0; k < m_equipos.size(); k++)
    {
        const Equipo &e = m_equipos[k];
        QByteArray cab = m_datos.mid(e.inicio, e.fin - e.inicio);
        const int offNum = e.offNumJugadores - e.inicio;          // nº de jugadores
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
                r[j.offAlineacion - j.inicio] = char(j.alineacion - 1);
            if (k == origen && j.offOrden >= 0)
            {
                // Promanager: el nº de orden también va de 1 a N sin huecos
                Lector q(m_datos);
                q.o = j.offOrden;
                const int orden = q.u16(), ordenMovido = [&]() { Lector z(m_datos); z.o = jm.offOrden; return z.u16(); }();
                if (orden > ordenMovido) ponerU16(r, j.offOrden - j.inicio, orden - 1);
            }
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
    for (int c = 0; c < 5; c++)
        if (j.offGrupo[c] >= 0)
            for (int i = 0; i < 4; i++) m_datos[j.offGrupo[c] + i] = 0;
    for (const CopiaTraspaso &c : m_copias)
        if (c.puntero == j.puntero && c.equipo == m_equipos[equipo].id && c.offIncidencias >= 0)
            for (int i = 0; i < 20; i++) m_datos[c.offIncidencias + i] = 0;
    j.lesionado = false;
    j.sancionado = false;
    j.jornadasBaja = 0;
}

quint16 PCFutbolPartida::punteroLibre() const
{
    QSet<int> usados;
    for (const Equipo &e : m_equipos)
        for (const Jugador &j : e.jugadores) usados.insert(j.puntero);
    for (const CopiaTraspaso &c : m_copias) usados.insert(c.puntero);
    // Las Estrellas usan 9900 en adelante; los demás, números bajos
    for (int p = 1; p < ID_ESTRELLAS; p++)
        if (!usados.contains(p)) return quint16(p);
    return 0;
}

bool PCFutbolPartida::anadirJugador(int equipo, const Jugador &datos, QString *info, QString *error)
{
    auto fallo = [&](const QString &t) { if (error) *error = t; return false; };
    if (equipo < 0 || equipo >= m_equipos.size()) return fallo("Equipo no válido.");
    const Equipo &e = m_equipos[equipo];
    if (e.id == ID_ESTRELLAS) return fallo("No se añaden jugadores a las Estrellas Mundiales.");
    if (e.jugadores.size() + 1 > MAX_PLANTILLA)
        return fallo(QString("%1 tendría %2 jugadores (máximo %3).").arg(e.nombreCorto).arg(e.jugadores.size() + 1).arg(MAX_PLANTILLA));
    if (!textoValido(datos.nombreCorto, 16, "Nombre corto", error) || !textoValido(datos.nombreLargo, 40, "Nombre largo", error))
        return false;

    QSet<int> usados;
    for (const Equipo &q : m_equipos) for (const Jugador &j : q.jugadores) usados.insert(j.puntero);
    for (const CopiaTraspaso &c : m_copias) usados.insert(c.puntero);
    quint16 puntero = datos.puntero;
    if (puntero == 0 || usados.contains(puntero)) puntero = punteroLibre();
    if (puntero == 0) return fallo("No quedan identidades libres.");

    QList<int> dorsales;
    for (const Jugador &j : e.jugadores) dorsales << j.dorsal;
    int dorsal = datos.dorsal > 0 ? datos.dorsal : 1;
    if (dorsales.contains(dorsal))
        for (dorsal = 1; dorsales.contains(dorsal); dorsal++) {}
    const int alineacion = e.jugadores.size() + 1;

    QByteArray r;
    if (m_pro)
    {
        // Promanager: se parte de un jugador del mismo equipo (para los campos
        // que no se conocen) y se ponen los datos del nuevo
        const Jugador *plantilla = nullptr;
        for (const Jugador &j : e.jugadores)
            if (j.procedencia != ID_ESTRELLAS && !j.lesionado && !j.sancionado) plantilla = &j;
        if (!plantilla) return fallo("No hay ningún jugador en el equipo que sirva de modelo.");
        r.append(char(puntero & 0xFF)); r.append(char(puntero >> 8));
        r += textoCodificado(datos.nombreCorto);
        r += textoCodificado(datos.nombreLargo);
        r += m_datos.mid(plantilla->offEquipo, plantilla->fin - plantilla->offEquipo);
        Lector lr(r);
        Jugador n;
        leerJugadorPro(lr, &n, m_jornadaActual);
        if (lr.mal || n.fin != r.size()) return fallo("Error interno al crear el jugador.");
        ponerU16(r, n.offEquipo, e.id);
        ponerU16(r, n.offEquipo2, e.id);
        ponerU16(r, n.offOrden, alineacion);
        r[n.offDatos] = char(qBound(0, datos.posicion, 3));
        r[n.offDorsal - 3] = char(datos.extranjero ? 1 : 0);
        r[n.offDorsal - 2] = char(qBound(0, datos.piel, 2));
        r[n.offDorsal - 1] = char(qBound(0, datos.pelo, 5));
        r[n.offDorsal] = char(dorsal);
        r[n.offAlineacion] = char(alineacion);
        QByteArray v(8, 0);
        ponerF64(v, 0, qMax(0.0, datos.valor));
        for (int i = 0; i < 8; i++) r[n.offValor + i] = v[i];
        for (int i = 0; i < 4; i++) r[n.offMedias + i] = char(qBound(0, datos.media[i], MAX_MEDIA));
        for (int i = 0; i < 4; i++) r[n.offAptitudes + i] = char(datos.aptitud[i] ? 1 : 0);
        r[n.offEnergia] = char(qBound(0, datos.energia, MAX_ENERGIA));
        for (int c = 0; c < 5; c++)                                    // sin bajas ni amarillas
            for (int i = 0; i < 4; i++) r[n.offGrupo[c] + i] = 0;
    }
    else
    {
    r.append(char(puntero & 0xFF)); r.append(char(puntero >> 8));
    r += textoCodificado(datos.nombreCorto);
    r += textoCodificado(datos.nombreLargo);
    r.append(char(e.id & 0xFF)); r.append(char(e.id >> 8));        // procedencia
    r.append(char(e.id & 0xFF)); r.append(char(e.id >> 8));        // equipo actual
    r.append(char(qBound(0, datos.posicion, 3)));
    r.append(char(qBound(0, datos.demarcacion, 18)));
    r.append(char(datos.extranjero ? 1 : 0));
    r.append(char(qBound(0, datos.piel, 2)));
    r.append(char(qBound(0, datos.pelo, 5)));
    r.append(char(dorsal));
    r.append(char(alineacion));
    r.append(QByteArray(20, 0));                                   // sin bajas ni amarillas
    r.append(QByteArray(2, 0));
    QByteArray v(8, 0);
    ponerF64(v, 0, qMax(0.0, datos.valor));
    r += v;
    for (int i = 0; i < 4; i++) r.append(char(qBound(0, datos.media[i], MAX_MEDIA)));
    for (int i = 0; i < 4; i++) r.append(char(datos.aptitud[i] ? 1 : 0));
    r.append(char(qBound(0, datos.energia, MAX_ENERGIA)));
    }

    aplicar();
    QList<QList<QByteArray>> js;
    QList<QByteArray> cs;
    for (int k = 0; k < m_equipos.size(); k++)
    {
        const Equipo &q = m_equipos[k];
        QByteArray cab = m_datos.mid(q.inicio, q.fin - q.inicio);
        if (k == equipo) ponerU16(cab, q.offNumJugadores - q.inicio, q.jugadores.size() + 1);
        cs << cab;
        QList<QByteArray> l;
        for (const Jugador &j : q.jugadores) l << m_datos.mid(j.inicio, j.fin - j.inicio);
        if (k == equipo) l << r;
        js << l;
    }
    const QString nombreEquipo = e.nombreCorto;
    if (!reconstruir(js, cs, error)) return false;
    if (info)
        *info = QString("%1 añadido a %2: puesto %3 de la alineación, dorsal %4 (identidad %5)")
                    .arg(datos.nombreCorto, nombreEquipo).arg(alineacion).arg(dorsal).arg(puntero);
    return true;
}
