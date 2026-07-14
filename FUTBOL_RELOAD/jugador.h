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
    // Etiqueta de fichaje: 0 = nada, 1 = nuevo fichaje, 2 = subido del filial
    quint8 EstadoFichaje;
    quint8 DemarcacionPref;
    QByteArray DemarcacionesSecundarias; //5bytes

    // Byte que antes se saltaba como desconocido ("lo mismo que el número
    // de camiseta"). IDENTIFICADO: es la posición del jugador dentro de la
    // pantalla ALINEACIÓN del juego (1..N, único por plantilla). Suele
    // coincidir con el dorsal en las plantillas originales, pero NO es el
    // dorsal: si se añaden jugadores debe ser el siguiente valor libre o
    // el juego se cuelga (documentado por la comunidad de pcfutbolmania).
    quint8 PosicionEnAlineacion;

    quint8 Extranjero;          // 0 = nacional, 1 = extranjero
    quint8 Piel;                // 0 = blanca, 1 = negra
    quint8 Pelo;                // 0 rubio, 1 marrón claro, 2 negro, 3 gris, 4 rojo, 5 marrón oscuro
    quint8 PosicionEnCampo;     // 0 portero, 1 defensa, 2 centrocampista, 3 delantero
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
