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
