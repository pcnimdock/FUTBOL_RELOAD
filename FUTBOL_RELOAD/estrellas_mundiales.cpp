#include "estrellas_mundiales.h"

ESTRELLAS_MUNDIALES::ESTRELLAS_MUNDIALES()
{
    clear();
}

void ESTRELLAS_MUNDIALES::clear()
{
    jugador.clear();
    list_jugador.clear();
    cabecera.clear();
}

void ESTRELLAS_MUNDIALES::guardar_datos_de_archivo(QByteArray datos)
{
    quint32 pos=0;
    int size_cad;
    for(pos=0;pos<0x2C;pos++){
        cabecera.append(datos.at(pos));}
    estrellas_mundiales.EquipoIdDBC=datos.at(pos++)&0xFF;
    estrellas_mundiales.EquipoIdDBC+=datos.at(pos++)*256;
    size_cad=datos.at(pos++);
    for(int i=0;i<size_cad;i++)
    {
        estrellas_mundiales.NombreCorto.append(transform.descodificar_caracter(datos.at(pos++)));
    }

    for(int i=0;i<43;i++)
    {estrellas_mundiales.bytes2_array_de_10.append(pos++);} //se guarda aquí por guardalo en algún lado

    size_cad=datos.at(pos++);
    for(int i=0;i<size_cad;i++)
    {
        estrellas_mundiales.nombre_corto_mayusculas.append(transform.descodificar_caracter(datos.at(pos++))); //en el archivo pone "SEGUNDA B", debe de tener alguna relación
    }

    for(int i=0;i<275;i++)
    { //antes de llegar a bautista hay una zona que pone "Comentario" ¿alguna relación con algo?
        estrellas_mundiales.cosas_bbdd.append(datos.at(pos++));
    }

    //aquí se comienza a mirar los jugadores

    int isJugador=1;

    while(isJugador)
    {
        jugador.puntero=datos.at(pos++)&0xFF;
        jugador.puntero+=(datos.at(pos++)&0xFF)*256;

        size_cad=datos.at(pos++);
        for(int i=0;i<size_cad;i++)
        {
            jugador.NombreCorto.append(transform.descodificar_caracter(datos.at(pos++)));
        }
        size_cad=datos.at(pos++);
        for(int i=0;i<size_cad;i++)
        {
            jugador.NombreLargo.append(transform.descodificar_caracter(datos.at(pos++)));
        }
        //22 bytes de cosas
        jugador.NumeroCamiseta=datos.at(pos++);
        jugador.EstadoFichaje=datos.at(pos++); //de la base de datos (de 0 a 3 //por comprobar)
        jugador.DemarcacionPref=datos.at(pos++);
        for(int temp=0;temp<5;temp++){
            jugador.DemarcacionesSecundarias.append(datos.at(pos++));}
        pos++; //lo mismo que el número de camiseta
        pos++; // hay un byte 0x00

        jugador.Extranjero=datos.at(pos++);
        jugador.Piel=datos.at(pos++);
        jugador.Pelo=datos.at(pos++);
        jugador.PosicionEnCampo=datos.at(pos++); //0 portero, 1 defensor, 2 volante, 3 delantero
        size_cad=datos.at(pos++);
        for(int temp=0;temp<5;temp++){jugador.LugarDeNacimiento.append(transform.descodificar_caracter(datos.at(pos++)));} //aquí pone "Nac"

        jugador.FechaDeNacimiento.append(datos.at(pos++)); //DIA (0)
  //      jugador.FechaDeNacimiento.append(datos.at(pos++)); //MES (0)
  //      jugador.FechaDeNacimiento.append(datos.at(pos++)); //AÑO (0)

        size_cad=datos.at(pos++);
        for(int i=0;i<size_cad;i++)
        {
            jugador.EquipoProcedencia.append(transform.descodificar_caracter(datos.at(pos++)));
        }

        size_cad=datos.at(pos++);
        for(int temp=0;temp<size_cad;temp++){jugador.Internacional.append(transform.descodificar_caracter(datos.at(pos++)));}
        jugador.Altura=datos.at(pos++);
        jugador.Peso=datos.at(pos++);
        quint8 num_cadenas=datos.at(pos++);
        quint8 temp2=0;
        size_cad=datos.at(pos++);
        while(temp2<=num_cadenas)
        {
            for(int temp=0;temp<size_cad;temp++){jugador.Comentario.append(transform.descodificar_caracter(datos.at(pos++)));}
            size_cad=datos.at(pos++);
            temp2++;
        }
        jugador.Precio=size_cad&0xFF;
        jugador.Precio+=(datos.at(pos++)&0xFF)*256;

        jugador.Velocidad=datos.at(pos++);
        jugador.Regate=datos.at(pos++);
        jugador.Agilidad=datos.at(pos++);
        jugador.Agresividad=datos.at(pos++);

        jugador.ActTecnica=datos.at(pos++);
        jugador.ActRemate=datos.at(pos++);
        jugador.ActPase=datos.at(pos++);
        jugador.ActTiro=datos.at(pos++);
        pos++; //aquí debe haber un 0x00
        if(datos.at(pos++)==0)
        {isJugador=0;}
        list_jugador.append(jugador);

    //FIN repaso jugador
    }

}


