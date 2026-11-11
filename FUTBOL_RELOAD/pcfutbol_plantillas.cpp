#include "pcfutbol_plantillas.h"
#include "pcfutbol_temporada_real.h"

#include <QSet>
#include <QHash>
#include <algorithm>

namespace {

// Letras no ASCII que admite el juego (misma tabla que BBDD_FUTBOL)
const quint8 TABLA_JUEGO[48] = {
    0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB,
    0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5, 0xC9, 0xE6, 0xC6, 0xF4,
    0xF6, 0xF2, 0xFB, 0xF9, 0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5,
    0x00, 0x00, 0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA,
    0xBF, 0x81, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB};

QString nombreEquipo(const EQUIPO &eq)
{
    return eq.NombreCorto.isEmpty() ? QString::number(eq.EquipoIdDBC) : eq.NombreCorto;
}

QString nombreJugador(const JUGADOR &j)
{
    return j.NombreCorto.isEmpty() ? QString("(puntero %1)").arg(j.puntero) : j.NombreCorto;
}

bool textoValido(const QString &s)
{
    for (QChar c : s)
        if (!PCFutbolPlantillas::caracterValido(c)) return false;
    return true;
}

} // namespace

namespace PCFutbolPlantillas {

QStringList nombresPosicion()
{
    return {"Portero", "Defensa", "Centrocampista", "Delantero"};
}

QStringList nombresDemarcacion()
{
    return {"Sin rol", "Portero", "Lateral derecho", "Lateral izquierdo", "Líbero",
            "Central izquierdo", "Central derecho", "Centrocampista derecho",
            "Interior derecho", "Delantero centro", "Medio centro",
            "Centrocampista izquierdo", "Extremo derecho", "Media punta centro",
            "Extremo izquierdo", "Organizador defensivo", "Media punta derecho",
            "Media punta izquierdo", "Interior izquierdo"};
}

QStringList nombresPiel()
{
    return {"Blanca", "Negra", "Mulata"};
}

QStringList nombresPelo()
{
    return {"Rubio", "Castaño claro", "Negro", "Gris", "Pelirrojo", "Castaño oscuro"};
}

QStringList nombresEstadoFichaje()
{
    return {"Sin novedad", "Nuevo fichaje", "Ascendido del filial", "Baja del equipo"};
}

bool caracterValido(QChar c)
{
    const ushort u = c.unicode();
    if (u >= 0x20 && u < 0x7F) return true;
    if (u == 0x00B6) return true;   // ¶: código de párrafo de los textos largos
    if (u == 0 || u > 0xFF) return false;
    for (quint8 t : TABLA_JUEGO)
        if (t != 0 && t == u) return true;
    return false;
}

QString adaptarTexto(const QString &texto, bool *cambiado)
{
    static const QHash<QChar, QString> equivalencias = {
        {QChar(0x2018), "'"}, {QChar(0x2019), "'"}, {QChar(0x00B4), "'"}, {QChar(0x0060), "'"},
        {QChar(0x201C), "\""}, {QChar(0x201D), "\""},
        {QChar(0x2013), "-"}, {QChar(0x2014), "-"}, {QChar(0x00A0), " "},
        {QChar(0x2026), "..."}, {QChar(0x00DF), "ss"}, {QChar(0x0141), "L"},
        {QChar(0x0142), "l"}, {QChar(0x00D8), "O"}, {QChar(0x00F8), "o"},
        {QChar(0x0110), "D"}, {QChar(0x0111), "d"}, {QChar(0x00C1), "A"},
        {QChar(0x00CD), "I"}, {QChar(0x00D3), "O"}, {QChar(0x00DA), "U"},
        {QChar(0x00C0), "A"}, {QChar(0x00C8), "E"}, {QChar(0x00D2), "O"},
        {QChar(0x00C3), "A"}, {QChar(0x00E3), "a"}, {QChar(0x00D5), "O"},
        {QChar(0x00F5), "o"}, {QChar(0x00FD), "y"}, {QChar(0x00DD), "Y"},
        {QChar(0x00F0), "d"}, {QChar(0x00DE), "Th"}, {QChar(0x00FE), "th"},
        {QChar('\t'), " "}, {QChar('\r'), " "}, {QChar('\n'), " "}};
    QString r;
    bool hubo = false;
    for (QChar c : texto)
    {
        if (caracterValido(c)) { r += c; continue; }
        hubo = true;
        if (equivalencias.contains(c)) { r += equivalencias.value(c); continue; }
        // Quitar acentos: descomponer y quedarse con la letra base
        const QString d = QString(c).normalized(QString::NormalizationForm_D);
        QString base;
        for (QChar b : d)
            if (caracterValido(b)) base += b;
        r += base.isEmpty() ? QString("?") : base;
    }
    if (cambiado) *cambiado = hubo;
    return r;
}

void renumerarAlineacion(EQUIPO &eq)
{
    QList<int> orden;
    for (int i = 0; i < eq.lista_jugadores.size(); i++) orden << i;
    // Orden estable por el puesto actual; los 0 (sin puesto) van al final
    std::stable_sort(orden.begin(), orden.end(), [&eq](int a, int b) {
        const int pa = eq.lista_jugadores.at(a).PosicionEnAlineacion;
        const int pb = eq.lista_jugadores.at(b).PosicionEnAlineacion;
        if ((pa == 0) != (pb == 0)) return pb == 0;
        return pa < pb;
    });
    for (int k = 0; k < orden.size(); k++)
        eq.lista_jugadores[orden.at(k)].PosicionEnAlineacion = static_cast<quint8>(k + 1);
}

int dorsalLibre(const EQUIPO &eq, int preferido)
{
    QSet<int> usados;
    for (const JUGADOR &j : eq.lista_jugadores) usados.insert(j.NumeroCamiseta);
    if (preferido > 0 && preferido < 100 && !usados.contains(preferido)) return preferido;
    int d = 1;
    while (usados.contains(d)) d++;
    return d;
}

quint16 punteroLibre(const QList<EQUIPO> &equipos, const QList<quint16> &otros)
{
    QSet<int> usados(otros.begin(), otros.end());
    for (const EQUIPO &e : equipos)
        for (const JUGADOR &j : e.lista_jugadores) usados.insert(j.puntero);
    int p = 1;
    while (usados.contains(p) && p < 65535) p++;
    return static_cast<quint16>(p);
}

JUGADOR jugadorNuevo(const EQUIPO &eq, quint16 puntero)
{
    JUGADOR j;
    j.clear();
    j.EquipoIdDBC = eq.EquipoIdDBC;
    j.puntero = puntero;
    j.NombreCorto = "Nuevo";
    j.NombreLargo = "Jugador NUEVO";
    j.NumeroCamiseta = static_cast<quint8>(dorsalLibre(eq));
    j.EstadoFichaje = 1;                    // «Nuevo fichaje»
    j.DemarcacionPref = 10;                 // medio centro
    j.DemarcacionesSecundarias = QByteArray(5, 0);
    j.PosicionEnAlineacion = static_cast<quint8>(eq.lista_jugadores.size() + 1);
    j.PosicionEnCampo = 2;                  // centrocampista
    j.Piel = 0;
    j.Pelo = 2;
    j.Extranjero = 0;
    j.LugarDeNacimiento = "-";
    j.FechaDeNacimiento = QByteArray();
    j.FechaDeNacimiento.append(char(1));
    j.FechaDeNacimiento.append(char(1));
    j.FechaDeNacimiento.append(char(75));   // 1975
    j.EquipoProcedencia = "-";
    j.Internacional = "0";
    j.Altura = 178;
    j.Peso = 74;
    j.Comentario = " ";
    j.Precio = 20;
    j.Velocidad = 50;
    j.Regate = 50;
    j.Agilidad = 50;
    j.Agresividad = 50;
    return j;
}

QString traspasar(QList<EQUIPO> &equipos, int origen, int idxJugador, int destino,
                  bool marcarFichaje, bool dejarCopia)
{
    if (origen < 0 || origen >= equipos.size() || destino < 0 || destino >= equipos.size()
        || origen == destino)
        return QString();
    EQUIPO &eo = equipos[origen];
    EQUIPO &ed = equipos[destino];
    if (idxJugador < 0 || idxJugador >= eo.lista_jugadores.size()) return QString();

    JUGADOR j = eo.lista_jugadores.at(idxJugador);
    const int dorsalAntes = j.NumeroCamiseta;
    j.EquipoIdDBC = ed.EquipoIdDBC;
    j.NumeroCamiseta = static_cast<quint8>(dorsalLibre(ed, dorsalAntes));
    j.PosicionEnAlineacion = static_cast<quint8>(ed.lista_jugadores.size() + 1);
    if (marcarFichaje) j.EstadoFichaje = 1;
    // Los equipos de Segunda B (ficha reducida) no guardan estos datos;
    // si el jugador va a un equipo con ficha completa se le dan valores.
    if (ed.isBBDD_neg == 0 && j.FechaDeNacimiento.size() < 3)
    {
        j.FechaDeNacimiento = QByteArray();
        j.FechaDeNacimiento.append(char(1));
        j.FechaDeNacimiento.append(char(1));
        j.FechaDeNacimiento.append(char(75));
        if (j.LugarDeNacimiento.isEmpty()) j.LugarDeNacimiento = "-";
        if (j.EquipoProcedencia.isEmpty()) j.EquipoProcedencia = "-";
        if (j.Internacional.isEmpty()) j.Internacional = "0";
        if (j.Altura == 0) j.Altura = 178;
        if (j.Peso == 0) j.Peso = 74;
        if (j.Comentario.isEmpty()) j.Comentario = " ";
        if (j.Precio == 0) j.Precio = 20;
    }
    // Si ya está en el destino (mismo puntero, p. ej. filial), no se duplica
    for (const JUGADOR &x : ed.lista_jugadores)
        if (x.puntero == j.puntero) return QString();
    ed.lista_jugadores.append(j);
    if (!dejarCopia)
    {
        eo.lista_jugadores.removeAt(idxJugador);
        renumerarAlineacion(eo);
    }

    QString txt = QString("%1: %2 → %3").arg(nombreJugador(j), nombreEquipo(eo), nombreEquipo(ed));
    if (dejarCopia) txt += " (se queda también en " + nombreEquipo(eo) + " por la temporada real)";
    if (j.NumeroCamiseta != dorsalAntes)
        txt += QString(" (dorsal %1 ocupado, pasa al %2)").arg(dorsalAntes).arg(j.NumeroCamiseta);
    return txt;
}

void quitarJugador(EQUIPO &eq, int idxJugador)
{
    if (idxJugador < 0 || idxJugador >= eq.lista_jugadores.size()) return;
    eq.lista_jugadores.removeAt(idxJugador);
    renumerarAlineacion(eq);
}

QList<Problema> comprobar(const QList<EQUIPO> &equipos, const PCFutbolTemporadaReal *real,
                          const QList<JUGADOR> *estrellas)
{
    QList<Problema> r;
    auto grave = [&r](const QString &t, bool corr) { r.append({true, corr, t}); };
    auto aviso = [&r](const QString &t, bool corr) { r.append({false, corr, t}); };

    if (estrellas)
    {
        const int n = estrellas->size();
        if (n > MAX_ESTRELLAS)
            grave(QString("Estrellas Mundiales: %1 jugadores. Con más de %2 la Liga Manager se cuelga "
                          "al cargar (probado en el juego)").arg(n).arg(MAX_ESTRELLAS), false);
        else if (n > MAX_ESTRELLAS_PROBADO)
            aviso(QString("Estrellas Mundiales: %1 jugadores. Se ha probado una liga entera con %2; "
                          "cerca de %3 la Liga Manager puede colgarse al cargar, antes cuanto más "
                          "largos sean los nombres").arg(n).arg(MAX_ESTRELLAS_PROBADO).arg(MAX_ESTRELLAS), false);
    }

    if (real)
    {
        QHash<quint32, int> porId;
        for (int e = 0; e < equipos.size(); e++) porId.insert(equipos.at(e).EquipoIdDBC, e);
        for (const auto &c : real->claves())
        {
            if (!porId.contains(c.first)) continue;
            const EQUIPO &eq = equipos.at(porId.value(c.first));
            bool esta = false;
            for (const JUGADOR &j : eq.lista_jugadores) esta |= j.puntero == c.second;
            if (!esta)
                grave(QString("%1: falta el jugador con jugador_id %2, que juega partidos de la temporada "
                              "real 95/96. Sin él el juego NO ARRANCA («Jugador no encontrado»). "
                              "Vuelve a ponerlo en el equipo (en un traspaso, deja una copia).")
                          .arg(nombreEquipo(eq)).arg(c.second), false);
        }
    }

    for (const EQUIPO &eq : equipos)
    {
        const QString ne = nombreEquipo(eq);
        const int n = eq.lista_jugadores.size();

        if (n == 0)
            grave(ne + ": no tiene jugadores", false);
        else if (n < MIN_JUGADORES_VISTO)
            aviso(QString("%1: sólo %2 jugadores (en el juego original ninguno tiene menos de %3)")
                      .arg(ne).arg(n).arg(MIN_JUGADORES_VISTO), false);
        if (n > MAX_JUGADORES_JUEGO)
            grave(QString("%1: %2 jugadores. Con más de %3 el juego NO ARRANCA («DFG: id desconocido»; "
                          "probado en el juego)").arg(ne).arg(n).arg(MAX_JUGADORES_JUEGO), false);
        if (n > 0)
        {
            bool hayPortero = false;
            for (const JUGADOR &j : eq.lista_jugadores)
                if (j.PosicionEnCampo == 0) hayPortero = true;
            if (!hayPortero) aviso(ne + ": no tiene ningún portero", false);
        }

        // Alineación 1..N sin huecos
        QList<int> al;
        for (const JUGADOR &j : eq.lista_jugadores) al << j.PosicionEnAlineacion;
        std::sort(al.begin(), al.end());
        for (int k = 0; k < al.size(); k++)
        {
            if (al.at(k) != k + 1)
            {
                grave(ne + ": los puestos en la alineación no van de 1 a " + QString::number(n) +
                      " (puede colgar el juego)", true);
                break;
            }
        }

        // Dorsales y punteros repetidos
        QHash<int, QString> dorsales;
        QSet<int> punteros;
        for (const JUGADOR &j : eq.lista_jugadores)
        {
            if (dorsales.contains(j.NumeroCamiseta))
                grave(QString("%1: dorsal %2 repetido (%3 y %4)").arg(ne).arg(j.NumeroCamiseta)
                          .arg(dorsales.value(j.NumeroCamiseta), nombreJugador(j)), true);
            dorsales.insert(j.NumeroCamiseta, nombreJugador(j));
            if (punteros.contains(j.puntero))
                grave(QString("%1: %2 está dos veces en la plantilla (puntero %3)")
                          .arg(ne, nombreJugador(j)).arg(j.puntero), false);
            punteros.insert(j.puntero);
        }

        // Jugadores: textos y códigos
        for (const JUGADOR &j : eq.lista_jugadores)
        {
            const QString nj = ne + " / " + nombreJugador(j);
            if (j.NombreCorto.trimmed().isEmpty()) grave(nj + ": sin nombre corto", false);
            const QList<QPair<QString, QString>> textos = {
                {"nombre corto", j.NombreCorto}, {"nombre largo", j.NombreLargo},
                {"lugar de nacimiento", j.LugarDeNacimiento},
                {"equipo de procedencia", j.EquipoProcedencia},
                {"internacional", j.Internacional}};
            for (const auto &t : textos)
            {
                if (t.second.size() > MAX_TEXTO)
                    grave(QString("%1: %2 de más de %3 letras").arg(nj, t.first).arg(MAX_TEXTO), true);
                if (!textoValido(t.second))
                    aviso(QString("%1: el %2 «%3» tiene letras que el juego no tiene")
                              .arg(nj, t.first, t.second), true);
            }
            if (j.NombreCorto.size() > MAX_NOMBRE_CORTO_VISTO)
                aviso(QString("%1: nombre corto de %2 letras (en el juego original como mucho %3; "
                              "puede salir cortado)").arg(nj).arg(j.NombreCorto.size())
                          .arg(MAX_NOMBRE_CORTO_VISTO), false);
            if (j.NombreLargo.size() > MAX_NOMBRE_LARGO_VISTO)
                aviso(QString("%1: nombre largo de %2 letras (en el original como mucho %3)")
                          .arg(nj).arg(j.NombreLargo.size()).arg(MAX_NOMBRE_LARGO_VISTO), false);
            bool fueraRango = j.PosicionEnCampo > 3 || j.DemarcacionPref > 18 || j.Piel > 2 ||
                              j.Pelo > 5 || j.EstadoFichaje > 3 || j.Extranjero > 1 ||
                              j.DemarcacionesSecundarias.size() != 5;
            for (char d : j.DemarcacionesSecundarias)
                if (static_cast<quint8>(d) > 18) fueraRango = true;
            if (fueraRango) grave(nj + ": algún código (posición, demarcación, piel, pelo…) "
                                       "fuera de rango", true);
            if (j.NumeroCamiseta == 0 || j.NumeroCamiseta > 99)
                aviso(QString("%1: dorsal %2").arg(nj).arg(j.NumeroCamiseta), true);
        }

        // Textos del club
        const QList<QPair<QString, QString>> club = {
            {"nombre corto", eq.NombreCorto}, {"nombre largo", eq.NombreLargo},
            {"nombre en mayúsculas", eq.NombreMayusculas},
            {"nombre corto en mayúsculas", eq.nombre_corto_mayusculas},
            {"estadio", eq.estadio}, {"presidente", eq.presidente},
            {"patrocinador", eq.patrocinador}, {"proveedor", eq.proveedor}};
        for (const auto &t : club)
        {
            if (t.second.size() > MAX_TEXTO)
                grave(QString("%1: %2 de más de %3 letras").arg(ne, t.first).arg(MAX_TEXTO), true);
            if (!textoValido(t.second))
                aviso(QString("%1: el %2 «%3» tiene letras que el juego no tiene")
                          .arg(ne, t.first, t.second), true);
        }
        for (const ENTRENADOR &en : eq.lista_entrenadores)
        {
            for (const QString &t : {en.NombreCorto, en.NombreLargo})
            {
                if (t.size() > MAX_TEXTO)
                    grave(QString("%1: entrenador con nombre de más de %2 letras").arg(ne).arg(MAX_TEXTO), true);
                if (!textoValido(t))
                    aviso(QString("%1: el entrenador «%2» tiene letras que el juego no tiene").arg(ne, t), true);
            }
        }
    }
    return r;
}

QStringList corregirAutomaticamente(QList<EQUIPO> &equipos)
{
    QStringList hecho;
    auto arreglarTexto = [&hecho](QString &s, const QString &donde) {
        bool cambio = false;
        QString n = adaptarTexto(s, &cambio);
        if (n.size() > MAX_TEXTO) { n.truncate(MAX_TEXTO); cambio = true; }
        if (cambio)
        {
            hecho << QString("%1: «%2» → «%3»").arg(donde, s.left(60), n.left(60));
            s = n;
        }
    };
    auto limitar = [](quint8 &v, quint8 max, quint8 porDefecto) {
        if (v > max) { v = porDefecto; return true; }
        return false;
    };

    for (EQUIPO &eq : equipos)
    {
        const QString ne = nombreEquipo(eq);
        // Alineación
        QList<int> antes;
        for (const JUGADOR &j : eq.lista_jugadores) antes << j.PosicionEnAlineacion;
        renumerarAlineacion(eq);
        for (int k = 0; k < eq.lista_jugadores.size(); k++)
        {
            if (antes.at(k) != eq.lista_jugadores.at(k).PosicionEnAlineacion)
            {
                hecho << ne + ": alineación renumerada de 1 a " + QString::number(eq.lista_jugadores.size());
                break;
            }
        }
        // Dorsales repetidos o fuera de rango: se queda el primero; los
        // demás pasan al primer dorsal libre
        QSet<int> ocupados;
        QList<int> repetidos;
        for (int k = 0; k < eq.lista_jugadores.size(); k++)
        {
            const int d = eq.lista_jugadores.at(k).NumeroCamiseta;
            if (d >= 1 && d <= 99 && !ocupados.contains(d)) ocupados.insert(d);
            else repetidos << k;
        }
        for (int k : repetidos)
        {
            JUGADOR &j = eq.lista_jugadores[k];
            int d = 1;
            while (ocupados.contains(d)) d++;
            ocupados.insert(d);
            hecho << QString("%1 / %2: dorsal %3 → %4").arg(ne, nombreJugador(j))
                         .arg(j.NumeroCamiseta).arg(d);
            j.NumeroCamiseta = static_cast<quint8>(d);
        }
        // Textos y códigos de los jugadores
        for (JUGADOR &j : eq.lista_jugadores)
        {
            const QString nj = ne + " / " + nombreJugador(j);
            arreglarTexto(j.NombreCorto, nj);
            arreglarTexto(j.NombreLargo, nj);
            arreglarTexto(j.LugarDeNacimiento, nj);
            arreglarTexto(j.EquipoProcedencia, nj);
            arreglarTexto(j.Internacional, nj);
            bool c = false;
            c |= limitar(j.PosicionEnCampo, 3, 2);
            c |= limitar(j.DemarcacionPref, 18, 0);
            c |= limitar(j.Piel, 2, 0);
            c |= limitar(j.Pelo, 5, 2);
            c |= limitar(j.EstadoFichaje, 3, 0);
            c |= limitar(j.Extranjero, 1, 1);
            if (j.DemarcacionesSecundarias.size() != 5)
            {
                j.DemarcacionesSecundarias = j.DemarcacionesSecundarias.left(5);
                while (j.DemarcacionesSecundarias.size() < 5) j.DemarcacionesSecundarias.append(char(0));
                c = true;
            }
            for (int k = 0; k < 5; k++)
                if (static_cast<quint8>(j.DemarcacionesSecundarias.at(k)) > 18) { j.DemarcacionesSecundarias[k] = 0; c = true; }
            if (c) hecho << nj + ": códigos fuera de rango puestos a un valor válido";
        }
        // Textos del club
        arreglarTexto(eq.NombreCorto, ne);
        arreglarTexto(eq.NombreLargo, ne);
        arreglarTexto(eq.NombreMayusculas, ne);
        arreglarTexto(eq.nombre_corto_mayusculas, ne);
        arreglarTexto(eq.estadio, ne);
        arreglarTexto(eq.presidente, ne);
        arreglarTexto(eq.patrocinador, ne);
        arreglarTexto(eq.proveedor, ne);
        for (ENTRENADOR &en : eq.lista_entrenadores)
        {
            arreglarTexto(en.NombreCorto, ne + " (entrenador)");
            arreglarTexto(en.NombreLargo, ne + " (entrenador)");
        }
    }
    return hecho;
}

} // namespace PCFutbolPlantillas
