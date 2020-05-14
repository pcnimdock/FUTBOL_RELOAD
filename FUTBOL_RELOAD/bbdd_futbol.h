#ifndef BBDD_FUTBOL_H
#define BBDD_FUTBOL_H

#include <QObject>
//#include <QtSql>
#include <QString>
#include <QChar>
#include <QList>
#include <QByteArray>

#include "equipo.h"
#include "jugador.h"
#include "entrenador.h"

class BBDD_FUTBOL
{
public:
    BBDD_FUTBOL();
    ~BBDD_FUTBOL();
    quint8 add_jugador(quint16 ptr_equipo,quint16 ptr_jugador);
    //quint8 opendb();
    QChar descodificar_caracter(quint8 val);
    quint8 codificar_caracter(QChar val);
    quint8 add_equipo(EQUIPO eq,QList <ENTRENADOR> ent_list, QList <JUGADOR> jug_list);
    quint8 guardar_equipo(EQUIPO eq,QString main_path);
private:
    //QSqlDatabase sql;
    QByteArray devolver_DBC_Info_Declaraciones(QString info);
    QByteArray QString2QBytePlusSize(QString cad);
};

#endif // BBDD_FUTBOL_H
