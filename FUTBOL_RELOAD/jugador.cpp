#include "jugador.h"

JUGADOR::JUGADOR()
{

}

void JUGADOR::clear()
{
    EquipoIdDBC=0;
    puntero=0;
    NombreCorto.clear();
    NombreLargo.clear();
    NumeroCamiseta=0;
    EstadoFichaje=0;
    DemarcacionPref=0;
    DemarcacionesSecundarias.clear(); //5bytes
    Extranjero=0;
    Piel=0;
    Pelo=0;
    PosicionEnCampo=0;
    LugarDeNacimiento.clear();
    FechaDeNacimiento.clear(); //3bytes
    EquipoProcedencia.clear();
    Internacional.clear();
    Altura=0;
    Peso=0;
    Comentario.clear();
    Precio=0;
    Velocidad=0;
    Regate=0;
    Agilidad=0;
    Agresividad=0;
    ActTecnica=0;
    ActRemate=0;
    ActPase=0;
    ActTiro=0;
    minifoto.clear();
    bigfoto.clear();

}
