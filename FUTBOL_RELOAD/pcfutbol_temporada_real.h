#ifndef PCFUTBOL_TEMPORADA_REAL_H
#define PCFUTBOL_TEMPORADA_REAL_H

/*
 * pcfutbol_temporada_real.h / .cpp
 *
 * Jugadores que aparecen en los partidos de la temporada real (JORNAD1.DBC =
 * Primera 95/96, 42 jornadas x 11 partidos; JORNAD2.DBC = Segunda, 38 x 10).
 *
 * VERIFICADO EN EL JUEGO (9/11/2026): PCFUTBOL.DAT (el menú) comprueba al
 * arrancar que cada jugador de esos partidos sigue en su equipo. Si falta,
 * sale «NJugadorEquipo: ERROR: Jugador no encontrado. Equipo: Real Madrid,
 * Indice Jugador: 1» en bucle y el juego no arranca. Pasó al dar de baja a
 * Buyo (puntero 1) y al traspasar a Lopetegi (Barcelona, puntero 23).
 *
 * El juego original ya tiene 30 jugadores repetidos en dos equipos con el
 * mismo puntero (los del filial, p. ej. Barcelona y Barcelona B), así que
 * para traspasar a uno de estos jugadores se deja una copia en el equipo
 * de origen.
 *
 * FORMATO (los dos ficheros, leídos enteros sin sobrantes):
 *   repetido: [u16 número de jornada] y sus partidos
 *   partido:  u16 local, u16 visitante (índice n -> EQ95000n.DBC),
 *             u8 goles local, u8 goles visitante, 4 bytes (fecha y otros),
 *             texto árbitro, u32 público, texto estado del campo,
 *             texto tiempo, texto hora, texto largo crónica,
 *             y para cada equipo (local y luego visitante):
 *               u16 equipo,
 *               12 x (u16 jugador, u16 sustituto o FFFF, u16 minuto),
 *               sucesos (u16 tipo, u16 jugador, u16 valor) hasta tipo 0x63,
 *               texto largo declaraciones.
 *   texto = u8 longitud + letras; texto largo = u8 N + (N+1) textos.
 *   Los jugadores son punteros (el número de J95xxxxx.DFG). Un suceso puede
 *   ser de un jugador del otro equipo (gol en propia puerta, etc.).
 */

#include <QSet>
#include <QPair>
#include <QString>
#include <QList>
#include "equipo.h"

class PCFutbolTemporadaReal
{
public:
    // (EquipoIdDBC, puntero)
    typedef QPair<quint32, quint16> Clave;

    // Lee JORNAD1.DBC y JORNAD2.DBC de la carpeta del juego. Si no están o
    // no se entienden, queda vacío y error() lo explica.
    bool cargar(const QString &carpetaJuego, const QList<EQUIPO> &equipos);
    QString error() const { return m_error; }
    int partidos() const { return m_partidos; }

    bool vacio() const { return m_claves.isEmpty(); }
    int numJugadores() const { return m_claves.size(); }
    bool juegaPartidosReales(quint32 equipoId, quint16 puntero) const
    {
        return m_claves.contains(qMakePair(equipoId, puntero));
    }
    const QSet<Clave> &claves() const { return m_claves; }

private:
    bool leerFichero(const QString &ruta, const QList<EQUIPO> &equipos);

    QSet<Clave> m_claves;
    QString m_error;
    int m_partidos = 0;
};

#endif // PCFUTBOL_TEMPORADA_REAL_H
