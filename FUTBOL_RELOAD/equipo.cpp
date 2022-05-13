#include "equipo.h"


EQUIPO::EQUIPO()
{

}

void EQUIPO::clear()
{
    EquipoIdDBC=0;
    byte1_know=0;
    isBBDD_neg=0;
    jugable=0;
    NombreLargo.clear();
    NombreCorto.clear();
    NombreMayusculas.clear();
    Bytes_unknow3_5bytes.clear();
    anyo_fundacion=0;
    estadio.clear();
    aforo=0;
    num_socios=0;
    presidente.clear();
    presupuesto_bbdd=0;
    presupuesto_juego=0;
    patrocinador.clear();
    proveedor.clear();
    nombre_corto_mayusculas.clear();
    cosas_bbdd.clear(); //306bytes
    bytes2_array_de_10.clear();
    miniesc.clear();
    bigesc.clear();
    esc_sim.clear();
    cabecera.clear();
}
//nación en posicion en archivo 0x2B
QString EQUIPO::get_nacion(uint8_t dato)
{
    switch(dato)
    {
    case 0x01: return("Albania");break;
    case 0x02: return("Alemania");break;
    case 0x03: return("Austria");break;
    case 0x04: return("Bélgica");break;
    case 0x05: return("Bielorusia");break;
    case 0x06: return("Bulgaria");break;
    case 0x07: return("Croacia");break;
    case 0x09: return("Chipre");break;
    case 0x0a: return("Dinamarca");break;
    case 0x0b: return("Escocia");break;
    case 0x0c: return("Eslovaquia");break;
    case 0x0d: return("Eslovenia");break;
    case 0x0e: return("España");break;
    case 0x0f: return("Finlandia");break;
    case 0x10: return("Francia");break;
    case 0x11: return("Grecia");break;
    case 0x12: return("Países Bajos");break;
    case 0x13: return("Hungría");break;
    case 0x14: return("Inglaterra");break;
    case 0x16: return("Islas Feroe");break;
    case 0x18: return("Israel");break;
    case 0x19: return("Italia");break;
    case 0x20: return("República Checa");break;
    case 0x22: return("Rumanía");break;
    case 0x23: return("Rusia");break;
    case 0x24: return("Suecia");break;
    case 0x25: return("Suiza");break;
    case 0x26: return("Turquía");break;
    case 0x27: return("Ucrania");break;
    case 0x28: return("Serbia");break;
    case 0x29: return("Argentina");break;
    case 0x1a: return("Lituania");break;
    case 0x1b: return("Luxemburgo");break;
    case 0x1c: return("Malta");break;
    case 0x1d: return("Noruega");break;
    case 0x1f: return("Polonia");break;
    default: return("No def");break;
    }
    return("No def");
}

