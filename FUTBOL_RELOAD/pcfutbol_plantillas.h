#ifndef PCFUTBOL_PLANTILLAS_H
#define PCFUTBOL_PLANTILLAS_H

/*
 * pcfutbol_plantillas.h / .cpp
 *
 * Operaciones sobre las plantillas que mantienen las reglas del juego:
 * altas, bajas, traspasos y una comprobación antes de guardar.
 *
 * Reglas sacadas de los DBC originales de PC Fútbol 4.0 y 4.5:
 *   - El «puesto en la alineación» de cada plantilla va de 1 a N sin huecos
 *     ni repetidos (los 235 equipos). Si no, el juego puede colgarse.
 *   - Ningún equipo repite dorsal.
 *   - Plantillas de 16 jugadores (extranjeros) a 33 (los españoles más largos).
 *   - El «puntero» del jugador da nombre a su foto (MINIFOTO/J95xxxxx.DFG):
 *     un jugador traspasado conserva el suyo; uno nuevo recibe uno libre.
 *   - 3.084 de los 4.151 jugadores no tienen foto: no es obligatoria.
 *   - Los textos se guardan con un byte de longitud (máximo 255) y sólo
 *     admiten ASCII y las letras de la tabla del juego (á, ñ, ü, ç…).
 */

#include <QList>
#include <QString>
#include <QStringList>
#include "equipo.h"

class PCFutbolTemporadaReal;

namespace PCFutbolPlantillas
{
    // Límites observados en los datos originales (para avisos, no errores)
    const int MIN_JUGADORES_VISTO = 16;
    const int MAX_JUGADORES_VISTO = 33;
    const int MAX_NOMBRE_CORTO_VISTO = 16;
    const int MAX_NOMBRE_LARGO_VISTO = 40;
    const int MAX_TEXTO = 255;          // límite del formato (1 byte de longitud)

    // Nombres de los códigos (para la ayuda y los informes)
    QStringList nombresPosicion();      // 0..3
    QStringList nombresDemarcacion();   // 0..18
    QStringList nombresPiel();          // 0..2
    QStringList nombresPelo();          // 0..5
    QStringList nombresEstadoFichaje(); // 0..3

    // ¿Se puede escribir este carácter en un DBC sin que cambie?
    bool caracterValido(QChar c);
    // Devuelve el texto con los caracteres no admitidos cambiados por el más
    // parecido (ć -> c, ’ -> ', « -> ", …). Si cambia algo, *cambiado = true.
    QString adaptarTexto(const QString &texto, bool *cambiado = nullptr);

    // Pone el puesto en la alineación de 1 a N respetando el orden actual.
    void renumerarAlineacion(EQUIPO &eq);
    // Primer dorsal libre (empezando por el preferido si está libre).
    int dorsalLibre(const EQUIPO &eq, int preferido = 0);
    // Primer puntero que no usa ningún jugador de los equipos ni de 'otros'.
    quint16 punteroLibre(const QList<EQUIPO> &equipos, const QList<quint16> &otros);

    // Jugador nuevo con valores razonables, ya con dorsal y alineación libres.
    JUGADOR jugadorNuevo(const EQUIPO &eq, quint16 puntero);

    // Mueve el jugador 'idxJugador' del equipo 'origen' al 'destino'.
    // Conserva puntero y fotos; cambia el dorsal sólo si está ocupado; lo
    // pone el último en la alineación y renumera la plantilla de origen.
    // marcarFichaje: pone el estado «Nuevo fichaje» (sale marcado en el juego).
    // Devuelve una descripción para el informe (vacía si no se pudo).
    // dejarCopia: el jugador sigue también en el equipo de origen (necesario si
    // juega partidos de la temporada real; ver pcfutbol_temporada_real.h).
    QString traspasar(QList<EQUIPO> &equipos, int origen, int idxJugador, int destino,
                      bool marcarFichaje, bool dejarCopia = false);

    // Quita un jugador y renumera la alineación.
    void quitarJugador(EQUIPO &eq, int idxJugador);

    struct Problema
    {
        bool grave;          // grave = puede colgar el juego o romper el fichero
        bool corregible;     // corregirAutomaticamente() lo arregla
        QString texto;
    };
    // Con 'real', también que sigan en su equipo los jugadores de los
    // partidos de la temporada real (si no, el juego no arranca).
    QList<Problema> comprobar(const QList<EQUIPO> &equipos, const PCFutbolTemporadaReal *real = nullptr);
    // Arregla alineaciones, dorsales repetidos, textos largos o con
    // caracteres no admitidos y códigos fuera de rango. Devuelve qué hizo.
    QStringList corregirAutomaticamente(QList<EQUIPO> &equipos);
}

#endif // PCFUTBOL_PLANTILLAS_H
