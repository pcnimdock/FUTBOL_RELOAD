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
    // Latin-1 directo (sólo el último nombre del club; ver guardar_equipo)
    quint8 codificar_caracter_directo(QChar val);
    quint8 add_equipo(EQUIPO eq,QList <ENTRENADOR> ent_list, QList <JUGADOR> jug_list);
    quint8 guardar_equipo(EQUIPO eq,QString main_path);
    quint8 guardar_estrellas(ESTRELLAS_MUNDIALES eq,QString main_path);

    // --- Adaptación PC Fútbol 4.0 / 4.5 -----------------------------------
    // El formato de los DBC es el mismo en las dos versiones; lo que cambia
    // es, por comodidad, el nombre de la carpeta de salida. Las Estrellas
    // Mundiales son EQ959900.DBC en las dos copias reales (se acepta también
    // EQ959901.DBC si existe). Ver configurarVersion().
    // MainWindow lo fija al detectar la versión del juego.
    void configurarVersion(int version, const QString &rutaJuego); // 40 o 45
    QString carpetaSalida() const { return carpeta_salida; }
    QString ficheroEstrellas() const { return fichero_estrellas; }

private:
    int     version_pcf      = 45;
    QString carpeta_salida   = "FUTBOL45_NEW";
    QString fichero_estrellas= "EQ959900.DBC";
public:
private:
    //QSqlDatabase sql;
    QByteArray devolver_DBC_Info_Declaraciones(QString info);
    QByteArray bloqueTextoLargo(const QString &texto, const QString &leido, const QByteArray &bruto);
    QByteArray QString2QBytePlusSize(QString cad);
};

#endif // BBDD_FUTBOL_H
