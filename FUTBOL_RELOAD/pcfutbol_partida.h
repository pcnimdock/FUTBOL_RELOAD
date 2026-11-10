#ifndef PCFUTBOL_PARTIDA_H
#define PCFUTBOL_PARTIDA_H

/*
 * pcfutbol_partida.h / .cpp
 *
 * Partidas guardadas de Liga Manager (TACTICS/LIGA.0 … LIGA.9), PC Fútbol 4.5.
 * Formato sacado de PCMANAG.DAT (cabecera en 0x13600; estado en 0x101e0;
 * equipo en 0x24c07; jugador en 0x322e1). Ver docs/partidas-y-tacticas.md.
 *
 * La partida lleva los 42 equipos completos, con sus jugadores. El juego
 * los toma de aquí y no de los DBC, así que lo que se cambie en la base de
 * datos no llega a una partida ya empezada.
 *
 * El editor solo cambia valores de tamaño fijo EN SU SITIO: el fichero no
 * cambia de tamaño y todo lo demás queda byte a byte igual. Comprobado en el
 * juego (10/11/2026): caja del equipo, entrenamiento, táctica, y VE/RE/AG/CA,
 * energía y precio del jugador.
 */

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include "pcfutbol_tactica.h"

class PCFutbolPartida
{
public:
    struct Jugador
    {
        quint16 puntero = 0;
        QString nombreCorto, nombreLargo;
        int posicion = 0;          // 0 POR, 1 DEF, 2 MED, 3 DEL
        int dorsal = 0;
        int alineacion = 0;        // puesto en la alineación (1–11 titulares)
        int media[4] = {0, 0, 0, 0};   // VE, RE, AG, CA
        int energia = 0;
        double valor = 0;
        // posiciones en el fichero
        int offMedias = -1, offEnergia = -1, offValor = -1;
    };
    struct Equipo
    {
        int id = 0;                // EQ9500nn
        QString nombreLargo, nombreCorto, estadio, entrenador;
        QString nombreTactica;
        double caja = 0;
        int entrenamiento[3] = {0, 0, 0};   // defensa, mediocampo, delantera
        PCFutbolTactica::Tactica tactica;
        bool tieneTactica = false;
        QList<Jugador> jugadores;
        int offCaja = -1, offEntrenamiento = -1, offTactica = -1;
    };

    bool cargar(const QString &ruta, QString *error);
    bool cargarDatos(const QByteArray &datos, QString *error);
    // Guarda (la primera vez deja copia .bak del original)
    bool guardar(const QString &ruta, QString *error);

    const QByteArray &datos() const { return m_datos; }
    QString jugadorHumano() const { return m_jugador; }
    int jornada() const { return m_jornada; }
    QString fecha() const { return m_fecha; }
    int tipo() const { return m_tipo; }

    QList<Equipo> &equipos() { return m_equipos; }
    const QList<Equipo> &equipos() const { return m_equipos; }

    // Vuelca a los bytes lo que se haya cambiado en equipos()
    void aplicar();
    bool cambiada() const;

    // Límites de los campos que se pueden editar
    static const int MAX_MEDIA = 99;
    static const int MAX_ENERGIA = 99;
    // Entrenamiento (barras de la pantalla de tácticas). Reglas del juego
    // (PCMANAG.DAT, 0x4c6b7): cada barra de 12 a 43 y entre las tres como
    // mucho 75 (75 × 3,2 = 240). Los equipos empiezan en 25/25/25. Con
    // valores fuera de esto el juego se cuelga al abrir Tácticas (probado).
    static const int MIN_ENTRENAMIENTO = 12;
    static const int MAX_ENTRENAMIENTO = 43;
    static const int SUMA_MAX_ENTRENAMIENTO = 75;
    static bool entrenamientoValido(const int e[3]);

private:
    QByteArray m_datos, m_original;
    QString m_jugador, m_fecha;
    int m_jornada = 0, m_tipo = 0;
    QList<Equipo> m_equipos;
};

#endif // PCFUTBOL_PARTIDA_H
