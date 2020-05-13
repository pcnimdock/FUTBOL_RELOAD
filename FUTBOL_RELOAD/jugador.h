#ifndef JUGADOR_H
#define JUGADOR_H
#include <QByteArray>
#include <QString>

class JUGADOR
{
public:
    JUGADOR();
    quint32 EquipoIdDBC;
    quint16 puntero;
    QString NombreCorto;
    QString NombreLargo;
    quint8 NumeroCamiseta;
    quint8 EstadoFichaje;
    quint8 DemarcacionPref;
    QByteArray DemarcacionesSecundarias; //5bytes
    quint8 Extranjero;
    quint8 Piel;
    quint8 Pelo;
    quint8 PosicionEnCampo;
    QString LugarDeNacimiento;
    QByteArray FechaDeNacimiento; //3bytes
    QString EquipoProcedencia;
    QString Internacional;
    quint8 Altura;
    quint8 Peso;
    QString Comentario;
    quint16 Precio;
    quint8 Velocidad;
    quint8 Regate;
    quint8 Agilidad;
    quint8 Agresividad;
    quint8 ActTecnica;
    quint8 ActRemate;
    quint8 ActPase;
    quint8 ActTiro;
    QByteArray minifoto;
    QByteArray bigfoto;
    void clear();


};

#endif // JUGADOR_H
