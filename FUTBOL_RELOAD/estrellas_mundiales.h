#ifndef ESTRELLAS_MUNDIALES_H
#define ESTRELLAS_MUNDIALES_H
#include <QString>
#include <jugador.h>
#include <equipo.h>
#include <QList>
#include <transform_minifoto.h>


class ESTRELLAS_MUNDIALES
{
public:
    ESTRELLAS_MUNDIALES();
    QList <JUGADOR> list_jugador;
    QByteArray cabecera;
    EQUIPO estrellas_mundiales;
    JUGADOR jugador;
    TRANSFORM_MINIFOTO transform;
    void clear();
    void guardar_datos_de_archivo(QByteArray datos);
    QString ruta_pcf;


};

#endif // ESTRELLAS_MUNDIALES_H
