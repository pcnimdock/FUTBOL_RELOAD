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
    quint8 byte1_know;
    quint8 isBBDD_neg;
    quint8 jugable;
    QString NombreLargo;
    QString NombreCorto;
    QString NombreMayusculas;
    QByteArray Bytes_unknow3_5bytes;
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
    QByteArray cosas_bbdd; //306bytes o menos
    QByteArray bytes2_array_de_10;
    QByteArray miniesc;
    QByteArray bigesc;
    QByteArray esc_sim;
    QList <JUGADOR> lista_jugadores;
    QList <ENTRENADOR> lista_entrenadores;

    void clear();


};

#endif // EQUIPO_H
