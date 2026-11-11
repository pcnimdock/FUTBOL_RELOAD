#ifndef PCFUTBOL_PARTIDA_H
#define PCFUTBOL_PARTIDA_H

/*
 * pcfutbol_partida.h / .cpp
 *
 * Partidas guardadas de Liga Manager (TACTICS/LIGA.0 … LIGA.9) y de
 * Promanager (TACTICS/PROLIGA.nnn), PC Fútbol 4.5.
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
        int aptitud[4] = {0, 0, 0, 0}; // técnica, remate, pase, tiro (0/1)
        // Bajas: 5 competiciones × [Incidencias, JornIncid, NJornIncid,
        // NAmarillas] (nombres de PCMANAG.DAT, 0x31bac…0x31d20).
        // Incidencias 0 = ninguna (lo demás no cuenta), 1 = lesión, otro =
        // sanción (4 = 5 amarillas en liga, 7 = 2 en copa). Jornadas que
        // faltan, como las enseña el juego (0x33263):
        //     NJornIncid + 1 - (jornada actual - JornIncid)
        bool lesionado = false;
        bool sancionado = false;
        int jornadasBaja = 0;          // las que enseña el juego (la mayor)
        int offIncidencias = -1;
        // Dónde empieza cada uno de los 5 grupos de 4 bytes de incidencias
        // (en Liga Manager van seguidos; en Promanager hay 2 bytes tras el primero)
        int offGrupo[5] = {-1, -1, -1, -1, -1};
        bool extranjero = false;
        int demarcacion = 0, piel = 0, pelo = 0;
        int procedencia = 0;           // equipo de procedencia (9900 = Estrellas, con su club en texto)
        QString clubProcedencia;
        // posiciones en el fichero
        int offMedias = -1, offEnergia = -1, offValor = -1;
        int inicio = -1, fin = -1;     // registro completo (sin la marca 2)
        // u16 equipo de procedencia, u16 equipo actual, 7 bytes de datos
        int offEquipo = -1, offEquipo2 = -1, offDatos = -1;
        int offDorsal = -1, offAlineacion = -1;
        int offOrden = -1;             // Promanager: nº de orden en la plantilla (1..N)
        int offAptitudes = -1;
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
        int offNumJugadores = -1;      // u16 con el número de jugadores
        int inicio = -1, fin = -1;     // registro del equipo (sin la marca 1)
    };

    bool cargar(const QString &ruta, QString *error);
    bool cargarDatos(const QByteArray &datos, QString *error);
    // Guarda (la primera vez deja copia .bak del original)
    bool guardar(const QString &ruta, QString *error);

    const QByteArray &datos() const { return m_datos; }
    QString jugadorHumano() const { return m_jugador; }
    int jornada() const { return m_jornada; }
    int jornadaActual() const { return m_jornadaActual; }   // la que usa el juego para las bajas
    QString fecha() const { return m_fecha; }
    int tipo() const { return m_tipo; }
    // Promanager (TACTICS/PROLIGA.nnn) o Liga Manager (TACTICS/LIGA.n)
    bool esPromanager() const { return m_pro; }
    QStringList managers() const { return m_managers; }

    QList<Equipo> &equipos() { return m_equipos; }
    const QList<Equipo> &equipos() const { return m_equipos; }

    // Vuelca a los bytes lo que se haya cambiado en equipos()
    void aplicar();
    bool cambiada() const;

    // --- Cambios que cambian el tamaño del fichero (se reescribe entero) ---
    // Nombre corto y largo de un jugador. Solo letras de Latin-1, sin
    // caracteres de control, hasta 255. Devuelve false y *error si no.
    bool renombrarJugador(int equipo, int jugador, const QString &corto, const QString &largo, QString *error);
    // Mueve un jugador a otro equipo (como un fichaje del juego: procedencia =
    // origen, equipo actual = destino), lo pone el último de
    // la alineación del destino (renumera la del origen), le da un dorsal
    // libre si el suyo está ocupado y actualiza el número de jugadores de
    // los dos equipos y su copia en la lista de traspasos si la tiene.
    // *info describe lo hecho.
    bool traspasar(int origen, int jugador, int destino, QString *info, QString *error);
    // Añade un jugador nuevo al final de la plantilla. De 'datos' se usan
    // nombres, posición, demarcación, extranjero, piel, pelo, medias,
    // aptitudes, energía y precio; el puntero (identidad) se elige libre si
    // datos.puntero es 0 o ya está usado, el dorsal y el puesto también.
    bool anadirJugador(int equipo, const Jugador &datos, QString *info, QString *error);
    // Primer puntero que no usa ningún jugador de la partida
    quint16 punteroLibre() const;

    // Deja al jugador sin lesión, sin sanción y sin amarillas acumuladas en
    // las 5 competiciones (también su copia en la lista de traspasos).
    void quitarBajas(int equipo, int jugador);
    static const int MIN_PLANTILLA = 16;   // mínimo de los DBC originales
    static const int MAX_PLANTILLA = 33;   // máximo en las partidas y los DBC

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
    // Rehace el fichero con un registro de jugador cambiado de sitio o de tamaño
    bool reconstruir(const QList<QList<QByteArray>> &jugadores, const QList<QByteArray> &cabeceras,
                     QString *error);
    struct CopiaTraspaso { quint16 puntero; int equipo; int offEquipo, offEquipo2, offIncidencias; };
    QList<CopiaTraspaso> m_copias;
    int m_inicioEquipos = -1, m_inicioCola = -1;
    QByteArray m_datos, m_original;
    QString m_jugador, m_fecha;
    int m_jornada = 0, m_tipo = 0, m_jornadaActual = 0;
    bool m_pro = false;
    QStringList m_managers;
    bool cargarCuerpo(int inicio, QString *error);
    QList<Equipo> m_equipos;
};

#endif // PCFUTBOL_PARTIDA_H
