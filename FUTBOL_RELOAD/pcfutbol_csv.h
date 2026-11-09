#ifndef PCFUTBOL_CSV_H
#define PCFUTBOL_CSV_H

/*
 * pcfutbol_csv.h / .cpp
 *
 * Exportar e importar la base de datos (jugadores y clubes) en CSV para
 * editarla en Excel, LibreOffice o cualquier hoja de cálculo.
 *
 *  - Se escribe con ';' y UTF-8 con BOM (Excel en español lo abre bien).
 *  - Se lee con ';', ',' o tabulador, en UTF-8 o Windows-1252 (lo que
 *    guarda Excel como «CSV (delimitado por comas)»).
 *  - Celda vacía = no cambiar ese dato. Se pueden borrar columnas enteras.
 *  - Jugadores: la columna jugador_id identifica al jugador (es el número
 *    de su foto, J95xxxxx.DFG). Si cambia equipo_id, es un traspaso. Fila
 *    con jugador_id vacío = jugador nuevo.
 *  - Nada se escribe en el juego al importar: los cambios quedan en el
 *    editor hasta pulsar «Guardar».
 */

#include <QList>
#include <QString>
#include <QStringList>
#include "equipo.h"

class PCFutbolTemporadaReal;

namespace PCFutbolCsv
{
    enum Tipo { Desconocido, Jugadores, Equipos };

    bool exportarJugadores(const QList<EQUIPO> &equipos, const QString &ruta, QString *error);
    bool exportarEquipos(const QList<EQUIPO> &equipos, const QString &ruta, QString *error);
    // Explicación de columnas y códigos (LEEME_CSV.txt)
    bool escribirLeeme(const QString &ruta, QString *error);

    struct Tabla
    {
        Tipo tipo = Desconocido;
        QStringList cabecera;          // en minúsculas, sin espacios
        QList<QStringList> filas;
        QString codificacion;          // "UTF-8" o "Windows-1252"
        QChar separador;
    };
    bool leer(const QString &ruta, Tabla *t, QString *error);

    struct Resultado
    {
        int modificados = 0, nuevos = 0, traspasos = 0, borrados = 0;
        QStringList detalles;          // qué ha cambiado (traspasos, altas…)
        QStringList avisos;            // datos ajustados o filas ignoradas
    };
    // Jugadores que están en el editor pero no en el CSV (para preguntar)
    QStringList jugadoresAusentes(const QList<EQUIPO> &equipos, const Tabla &t);
    // Con 'real', los jugadores de los partidos de la temporada real no se dan
    // de baja y, si se traspasan, se deja una copia en su equipo (si no, el
    // juego no arranca; ver pcfutbol_temporada_real.h).
    Resultado importarJugadores(QList<EQUIPO> &equipos, const QList<quint16> &otrosPunteros,
                                const Tabla &t, bool borrarAusentes,
                                const PCFutbolTemporadaReal *real = nullptr);
    Resultado importarEquipos(QList<EQUIPO> &equipos, const Tabla &t);
}

#endif // PCFUTBOL_CSV_H
