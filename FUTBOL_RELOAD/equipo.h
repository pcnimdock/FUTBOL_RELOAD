#ifndef EQUIPO_H
#define EQUIPO_H
#include <QByteArray>
#include <QString>
#include <QList>

#include "jugador.h"
#include "entrenador.h"

class EQUIPO
{
public:
    EQUIPO();
    quint32 EquipoIdDBC;
    QByteArray cabecera;
    //QByteArray copyright;
    quint8 tipo_dbc;
    quint8 pais;
    QString pais_string;
    quint8 byte1_know;
    quint8 isBBDD_neg;
    quint8 jugable;
    QString NombreLargo;
    QString NombreCorto;
    QString NombreMayusculas;

    // Antiguo "Bytes_unknow3_5bytes". IDENTIFICADO (verificado contra los
    // DBC de PCF4.5 y la documentación de la comunidad de pcfutbolmania):
    //   +0 u16 LE  posición X en pantalla del escudo grande (BIGESC)
    //   +2 u16 LE  posición Y en pantalla del escudo grande (BIGESC)
    //   +4 u8      byte extra, valor 0-9 (posiblemente índice de paleta
    //              o grupo de color del escudo; pendiente de confirmar)
    // Se mantiene el QByteArray como fuente de verdad para grabar el DBC
    // sin pérdidas; los getters/setters de abajo lo decodifican.
    QByteArray Bytes_unknow3_5bytes;
    quint16 escudoPosX() const;
    quint16 escudoPosY() const;
    quint8  escudoByteExtra() const;
    void    setEscudoPos(quint16 x, quint16 y);

    quint16 anyo_fundacion;
    QString estadio;
    quint32 aforo;
    quint32 num_socios;
    QString presidente;
    quint32 presupuesto_bbdd;
    quint32 presupuesto_juego;
    QString patrocinador;
    QString proveedor;
    QString nombre_corto_mayusculas;
    // Bloque de estadísticas históricas del club. ESTRUCTURA EXACTA,
    // verificada byte a byte contra los 234 DBC de PC Fútbol 4.0 (y válida
    // también en 4.5, donde la heurística antigua encajaba por casualidad):
    //
    //   [202 bytes]  parte fija: pares u16 LE por competición (Liga, Copa,
    //                Copa de Europa, Recopa, UEFA…) + varios campos sueltos
    //   [1 byte   ]  L = longitud de la etiqueta de temporadas
    //   [L bytes  ]  etiqueta codificada (XOR 0x61). Los clubes con historial
    //                largo llevan " 2- 3- 4- … -18-19" (L=56); los modestos
    //                llevan "-" o "--" (L=1 ó 2)
    //   [47 bytes ]  puesto final por temporada (1 byte por temporada)
    //
    // Tamaño total: 250..306 bytes (306 = "o menos" del comentario original).
    // El editor lo conserva como blob opaco para regrabarlo sin pérdidas.
    QByteArray cosas_bbdd; //202 + 1 + L + 47 bytes

    // Posiciones en la clasificación de la última década (10 bytes),
    // equivalente al campo "lastdecade" del PKF de PCF6. 0x3F (63) parece
    // usarse como "sin dato / no participó".
    QByteArray bytes2_array_de_10;
    QByteArray miniesc;
    QByteArray bigesc;
    QByteArray esc_sim;
    QList <JUGADOR> lista_jugadores;
    QList <ENTRENADOR> lista_entrenadores;

    // Camiseta (equipación) real leída de PALETAS/EQUIPOS.DAT al procesar.
    // numero_kit = posición (1-based) del equipo entre los EQ*.DBC
    // ordenados por su ID numérico, excluyendo Estrellas Mundiales; -1 si
    // no se ha podido determinar o no se encontró EQUIPOS.DAT.
    int numero_kit = -1;
    QByteArray equipacion1_textura; // 168 bytes, vacío si no se pudo leer
    QByteArray equipacion2_textura; // 168 bytes, vacío si no se pudo leer

    void clear();
    QString get_nacion(uint8_t dato);


};

#endif // EQUIPO_H
