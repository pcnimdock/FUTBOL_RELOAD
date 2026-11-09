#ifndef PCFUTBOL_LIGAS_H
#define PCFUTBOL_LIGAS_H

/*
 * pcfutbol_ligas.h / .cpp
 *
 * Composición de las ligas españolas (Primera / Segunda / Segunda B).
 *
 * QUÉ SE HA DESCUBIERTO (ingeniería inversa de PCMANAG.DAT, 4.5):
 *  Los DBC no dicen en qué división juega cada club. La composición está
 *  en tablas de IDs (u32 LE, el número del fichero EQ95xxxx.DBC) dentro
 *  del segmento de datos de los ejecutables LE (DOS4GW):
 *
 *   - Lista larga (99 entradas, terminada en 0xFFFF): todos los equipos
 *     españoles ordenados por la clasificación de la temporada anterior.
 *     Posiciones 1-22 = Primera, 23-42 = Segunda, 43-99 = Segunda B
 *     (Ávila y Manchego aparecen dos veces). PCMANAG 0xA22DC,
 *     PROMANAG 0x964BC.
 *   - Tabla de 42 (Primera+Segunda) con la que arranca una partida nueva:
 *     PCMANAG 0xA3B24, PROMANAG 0x981A0. El código la recorre con bucles
 *     "cmp $0xA8" (42*4) y la clasificación usa filas de 22 (imul $0x16).
 *   - PCFUTBOL.DAT (base de datos informativa): 22 de Primera, 0xFFFF,
 *     20 de Segunda, 0xFFFF, en 0x54344. ¡NO SE MODIFICA! Comprobado en
 *     DOSBox: PCFUTBOL.DAT cruza esa tabla con el calendario real de la
 *     temporada 95/96 (JORNAD1.DBC = Primera, JORNAD2.DBC = Segunda, IDs
 *     de equipo en u16) a través de RetEquipPtr (0x29430), que sólo busca
 *     en la división cargada. Si se intercambia un equipo, al entrar en el
 *     amistoso o el seguimiento sale "ERROR: Equipo no encontrado [id]".
 *     Los modos de liga (PCMANAG/PROMANAG) no usan JORNAD*.DBC: con sólo
 *     esos dos parcheados la Liga Manager arranca con la composición nueva
 *     (verificado) y el amistoso también la usa.
 *  (tools/ligas_pcf.py sólo modificaba la lista larga; esta versión
 *  modifica todas las copias.)
 *
 *  Por eso aquí se puede CAMBIAR QUÉ EQUIPOS juegan en cada división
 *  (intercambiándolos), pero NO el número de equipos: 22 y 20 están
 *  fijados en cientos de sitios del código (tamaños de arrays, número de
 *  jornadas, ascensos...).
 *
 *  Las tablas se localizan por CONTENIDO (no por dirección fija): se busca
 *  una secuencia de 42 IDs distintos, todos con su DBC en DBDAT, que
 *  aparezca al menos dos veces en PCMANAG.DAT. Después, en cada ejecutable
 *  se busca cualquier tramo cuyos 22 primeros IDs sean los de Primera (en
 *  cualquier orden: PROMANAG trae Albacete y Valladolid invertidos), los 20
 *  siguientes los de Segunda, etc.
 *
 *  Al guardar, cada tabla conserva su propio orden: sólo se sustituyen los
 *  equipos intercambiados (permutación de IDs).
 *
 *  Sólo afecta a partidas NUEVAS: las ligas grabadas (TACTICS\LIGA.n)
 *  guardan su propia composición.
 */

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>

class PCFutbolLigas
{
public:
    static const int N_PRIMERA = 22;
    static const int N_SEGUNDA = 20;

    struct Equipo {
        quint32 id = 0;
        QString nombre;
        bool fichaCompleta = false;   // byte 0x2A del DBC == 0
        bool tieneCamisa = false;     // existe DBDAT/CAMISAS/EQ95xxxx.DFG
    };

    struct Tabla {
        QString fichero;              // nombre del ejecutable (PCMANAG.DAT...)
        quint32 va = 0;               // dirección virtual (para el informe)
        int longitud = 0;             // entradas que se reescriben
        QVector<qint64> offsets;      // offset en el fichero de cada byte (4*longitud)
        QVector<quint32> valores;     // IDs tal y como están en el fichero
    };

    // Lee las tablas de la carpeta del juego. false + error si no las encuentra.
    bool cargar(const QString &carpetaJuego, QString *error);

    // Lista larga actual (orden = Primera, Segunda, Segunda B)
    QVector<quint32> lista() const { return m_lista; }
    QVector<Tabla> tablas() const { return m_tablas; }
    Equipo equipo(quint32 id) const { return m_equipos.value(id); }

    // Intercambia dos equipos en TODAS sus apariciones de la lista
    void intercambiar(quint32 a, quint32 b);

    // Restaura la lista tal y como se leyó
    void deshacerTodo();
    bool modificada() const { return m_lista != m_listaOriginal; }

    // Escribe la lista en todas las tablas encontradas. Antes de tocar cada
    // ejecutable crea una copia <fichero>.bak (sólo si no existe ya).
    bool guardar(QString *error, QStringList *informe);

    // true si PCFUTBOL.DAT tiene copia .bak y es distinto de ella: lo dejó
    // modificado una versión anterior del editor y conviene restaurarlo.
    bool pcfutbolModificadoAntes() const;
    // Restaura PCFUTBOL.DAT desde PCFUTBOL.DAT.bak
    bool restaurarPcfutbol(QString *error) const;

private:
    struct ImagenLE {
        QString ruta;
        QString nombre;
        QByteArray fichero;
        // por objeto: base, tamaño virtual y offset en fichero de cada página
        struct Objeto { quint32 base; quint32 tam; QVector<qint64> paginas; };
        QVector<Objeto> objetos;
        quint32 tamPagina = 4096;
        quint32 bytesUltimaPagina = 4096;
        quint32 nPaginas = 0;
    };

    static bool leerLE(const QString &ruta, ImagenLE *img, QString *error);
    static bool leerU32(const ImagenLE &img, quint32 va, quint32 *v);
    static qint64 offsetFichero(const ImagenLE &img, quint32 va);
    void leerEquipos(const QString &carpetaJuego);
    bool buscarListaCanonica(const ImagenLE &img);
    void buscarTablas(const ImagenLE &img);

    QString m_carpeta;
    QVector<quint32> m_lista, m_listaOriginal;
    QMap<quint32, quint32> m_permutacion;   // ID original -> ID que lo sustituye
    QVector<Tabla> m_tablas;
    QMap<quint32, Equipo> m_equipos;
};

#endif // PCFUTBOL_LIGAS_H
