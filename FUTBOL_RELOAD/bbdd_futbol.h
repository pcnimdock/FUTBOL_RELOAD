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
#include "estrellas_mundiales.h"

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
    quint8 guardar_estrellas(ESTRELLAS_MUNDIALES eq,QString main_path);

    // --- Adaptación PC Fútbol 4.0 / 4.5 -----------------------------------
    // El formato de los DBC es el mismo en las dos versiones; lo que cambia
    // es el NOMBRE del fichero de Estrellas Mundiales (EQ959900.DBC en 4.0,
    // EQ959901.DBC en 4.5) y, por comodidad, el nombre de la carpeta de
    // salida. MainWindow los fija al detectar la versión del juego.
    void configurarVersion(int version);          // 40 o 45
    QString carpetaSalida() const { return carpeta_salida; }
    QString ficheroEstrellas() const { return fichero_estrellas; }

private:
    int     version_pcf      = 45;
    QString carpeta_salida   = "FUTBOL45_NEW";
    QString fichero_estrellas= "EQ959901.DBC";
public:
private:
    //QSqlDatabase sql;
    QByteArray devolver_DBC_Info_Declaraciones(QString info);
    QByteArray QString2QBytePlusSize(QString cad);
};

#endif // BBDD_FUTBOL_H
