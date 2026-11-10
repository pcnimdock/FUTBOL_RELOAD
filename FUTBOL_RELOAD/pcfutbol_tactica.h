#ifndef PCFUTBOL_TACTICA_H
#define PCFUTBOL_TACTICA_H

/*
 * pcfutbol_tactica.h / .cpp
 *
 * Táctica de un equipo: dónde se coloca cada uno de los 11 titulares.
 * Ver docs/partidas-y-tacticas.md.
 *
 *  - En el DBC son los primeros 176 bytes de EQUIPO::cosas_bbdd
 *    (11 jugadores × 8 u16 LE). Es la táctica con la que empieza el equipo
 *    en una liga nueva. Está en los 236 DBC de la 4.5 y los 235 de la 4.0.
 *  - El jugador k es el que ocupa el puesto k de la alineación.
 *  - Campo de 15 columnas × 9 filas: columna 0 = portería propia,
 *    14 = la contraria; fila 0 = arriba.
 *  - Por jugador:
 *      zona (x, y, ancho, alto): rectángulo por el que se mueve;
 *      defensa (x, y): dónde se coloca al defender (círculo en el juego);
 *      ataque (x, y): dónde se coloca al atacar (flecha).
 *    En los 5.181 jugadores originales, la defensa y el ataque caen siempre
 *    dentro de la zona, y la zona dentro del campo. Se mantiene esa regla.
 *  - TACTICS/TACTIC.nnn (244 bytes) es lo que escribe «Grabar táctica» en el
 *    juego: nombre (24, XOR 0x6A), mánager (16, XOR 0x6A), fecha (3 u32),
 *    hora (4 u32: centésimas, segundos, minutos, hora) y los 176 bytes.
 */

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace PCFutbolTactica
{
    const int COLUMNAS = 15;
    const int FILAS = 9;
    const int JUGADORES = 11;
    const int TAM = JUGADORES * 8 * 2;          // 176 bytes
    const int TAM_FICHERO = 0x44 + TAM;         // 244 bytes

    struct Jugador
    {
        int zx = 0, zy = 0, zancho = 1, zalto = 1;   // zona
        int dx = 0, dy = 0;                          // defensa
        int ax = 0, ay = 0;                          // ataque
    };
    struct Tactica
    {
        Jugador j[JUGADORES];
        bool operator==(const Tactica &o) const;
        bool operator!=(const Tactica &o) const { return !(*this == o); }
    };

    // Del bloque cosas_bbdd del DBC. false si no tiene táctica reconocible.
    bool leer(const QByteArray &cosas, Tactica *t);
    // Escribe la táctica al principio de cosas_bbdd (no cambia su tamaño)
    bool escribir(QByteArray &cosas, const Tactica &t);

    QByteArray aBytes(const Tactica &t);
    bool deBytes(const QByteArray &b, Tactica *t);

    // Las 6 formaciones del juego (las de los botones de la pantalla de
    // tácticas, sacadas de PCMANAG.DAT): 3-4-3, 3-5-2, 4-3-3, 4-4-2, 5-3-2, 5-4-1
    QStringList nombresFormaciones();
    Tactica formacion(int i);

    // Problemas que rompen la regla de arriba (vacío = todo bien)
    QStringList comprobar(const Tactica &t);
    // Mete cada posición dentro de su zona y cada zona dentro del campo
    void corregir(Tactica &t);

    // Ficheros TACTICS/TACTIC.nnn del juego
    bool leerFichero(const QString &ruta, Tactica *t, QString *nombre, QString *manager, QString *error);
    bool grabarFichero(const QString &ruta, const Tactica &t, const QString &nombre,
                       const QString &manager, QString *error);
    // Primer TACTIC.000 … TACTIC.199 libre en esa carpeta (vacío si no hay)
    QString ficheroLibre(const QString &carpetaTactics);
    // Carpeta TACTICS del juego (sin distinguir mayúsculas); vacía si no hay
    QString carpetaTactics(const QString &carpetaJuego);
}

#endif // PCFUTBOL_TACTICA_H
