#include "bbdd_futbol.h"
//#include <QSqlQuery>
#include <QFileDialog>


BBDD_FUTBOL::BBDD_FUTBOL()
{
   // sql=QSqlDatabase::addDatabase("QSQLITE");
   // sql.setDatabaseName("bbdd.db");

}

BBDD_FUTBOL::~BBDD_FUTBOL()
{

}

//quint8 BBDD_FUTBOL::opendb()
//{
//    if(!sql.open())
//    {
//        return 1;
//    }

//    QString query_string = "select * from EQUIPOS";
//    QSqlQuery query;
//    if(!query.exec(query_string))
//    {
//        //error al buscar tablas
//        //significa que la base de datos está vacía

//        //crea la estructura de base de datos
//        query_string="CREATE TABLE \"ENTRENADOR\" ( \"EquipoIdDBC\" INTEGER,"
//                     " \"Puntero\" INTEGER UNIQUE, \"NombreCorto\" TEXT,"
//                     " \"NombreLargo\" TEXT, \"Info\" TEXT, \"Declaraciones\" "
//                     "TEXT, \"minifoto\" BLOB, \"bigfoto\" BLOB )";
//        if(!query.exec(query_string))
//        {return 2;}
//        query_string="CREATE TABLE \"EQUIPO\" ( \"EquipoIdDBC\" INTEGER UNIQUE, "
//                     "\"byte1_know\" INTEGER, \"isBBDD_neg\" "
//                     "INTEGER, \"jugable\" INTEGER, \"NombreLargo\" TEXT, "
//                     "\"NombreCorto\" TEXT, \"NombreMayuculas\" TEXT,"
//                     " \"Bytes_unknow3_5bytes\" BLOB, \"anyo_fundacion\" INTEGER,"
//                     " \"estadio\" TEXT, \"aforo\" INTEGER, "
//                     "\"num_socios\" INTEGER, \"presidente\" TEXT, "
//                     "\"presupuesto_bbdd\" INTEGER, \"presupuesto_juego\" "
//                     "INTEGER, \"patrocinador\" TEXT, \"proveedor\" TEXT, "
//                     "\"nombre_corto_mayusculas\" TEXT, \"cosas_bbdd\" BLOB,"
//                     " \"bytes_2_array_de_10\" BLOB, \"miniesc\" BLOB,"
//                     " \"bigesc\" BLOB, \"esc_sim\" BLOB )";
//        if(!query.exec(query_string))
//        {return 2;}
//        query_string="CREATE TABLE \"JUGADOR\" ( \"EquipoIdDBC\" INTEGER,"
//                     " \"puntero\" INTEGER UNIQUE, \"NombreCorto\" TEXT, "
//                     "\"NombreLargo\" TEXT, \"NumeroCamiseta\" INTEGER, "
//                     "\"EstadoFichaje\" INTEGER, \"DemarcacionPref\" INTEGER,"
//                     " \"DemarcacionesSecundarias\" BLOB, \"Extranjero\" INTEGER, "
//                     "\"Piel\" INTEGER, \"Pelo\" INTEGER, \"PosicionEnCampo\" "
//                     "INTEGER, \"LugarDeNacimiento\" TEXT, \"FechaDeNacimiento\" "
//                     "INTEGER, \"EquipoProcedencia\" INTEGER, \"Internacional\" TEXT, "
//                     "\"Altura\" INTEGER, \"Peso\" INTEGER, \"Comentario\" TEXT, "
//                     "\"Precio\" INTEGER, \"Velocidad\" INTEGER, \"Regate\" INTEGER,"
//                     " \"Agilidad\" INTEGER, \"Agresividad\" INTEGER, \"ActTecnica\" INTEGER, "
//                     "\"ActRemate\" INTEGER, \"ActPase\" INTEGER, \"ActTiro\" INTEGER, "
//                     "\"minifoto\" BLOB, \"bigfoto\" BLOB )";
//        if(!query.exec(query_string))
//        {return 2;}
//        query_string="CREATE TABLE sqlite_sequence(name,seq)";
//        if(!query.exec(query_string))
//        {return 2;}
//    }
//return 0;
//}

const quint8 caracteres[48]={
    0xC7 ,0xFC ,0xE9 ,0xE2 ,0xE4 ,0xE0 ,0xE5 ,0xE7 ,0xEA ,0xEB ,
    0xE8 ,0xEF ,0xEE ,0xEC ,0xC4 ,0xC5 ,0xC9 ,0xE6 ,0xC6 ,0xF4 ,
    0xF6 ,0xF2 ,0xFB ,0xF9 ,0xFF ,0xD6 ,0xDC ,0xA2 ,0xA3 ,0xA5 ,
    0x00 ,0x00 ,0xE1 ,0xED ,0xF3 ,0xFA ,0xF1 ,0xD1 ,0xAA ,0xBA ,
    0xBF ,0x81 ,0xAC ,0xBD ,0xBC ,0xA1 ,0xAB ,0xBB};

QChar BBDD_FUTBOL::descodificar_caracter(quint8 val)
{
    val^=0x61;
    if(val&0x80)
    {
        val&=(~0x80);
        if(val<48)
        {val=caracteres[val];}
        else
        {val=0x20;}
    }

    QChar letra=QChar::fromLatin1(val);
    return(letra);
}

quint8 BBDD_FUTBOL::codificar_caracter(QChar val)
{
    //comprobar si el carácter está en la tabla
    quint8 i;
    quint8 val2;
    val2=val.toLatin1();
    for(i=0;i<48;i++)
    {
        if(caracteres[i]==val2)
        {break;}
    }
    if(i<48)
    {
        //el caracter existe en la tabla
        i+=0x80;
        i^=0x61;
    }
    else
    { //no existe en la tabla
        i=val.toLatin1();
        i^=0x61;
    }
    return(i);
}

quint8 BBDD_FUTBOL::add_equipo(EQUIPO eq, QList<ENTRENADOR> ent_list, QList<JUGADOR> jug_list)
{
    ent_list[0].EquipoIdDBC=eq.EquipoIdDBC;
    return 1;
}

QByteArray BBDD_FUTBOL::devolver_DBC_Info_Declaraciones(QString info)
{
    QByteArray ret;
    ret.clear();

    quint32 info_size=info.size();
    quint8 tamanyo_del_chunk=16;

    while((info_size/tamanyo_del_chunk)>254) //buscar tamaño para que no sobrepase los 8 bytes
    {
        tamanyo_del_chunk++;
    }

    //  if((info_size/tamanyo_del_chunk)%tamanyo_del_chunk==0)
    //  {num_cad=info_size/tamanyo_del_chunk;}
    //  else
    //  {
    //      num_cad=info_size/tamanyo_del_chunk+1;
    //  }

    quint8 num_cad;
    num_cad=info_size/tamanyo_del_chunk;
    ret.append(num_cad);
    int k=0;
    int b=0;
    while(k<=num_cad)
    {
        QString sub;
        sub = info.mid(k*tamanyo_del_chunk,tamanyo_del_chunk);
        ret.append(sub.size());
        int subsize= sub.size();
        for(int l=0;l<subsize;l++)
        {
            ret.append(codificar_caracter(sub.at(l)));
        }

        k++;
    }
    return ret;

}

QByteArray BBDD_FUTBOL::QString2QBytePlusSize(QString cad)
{
    QByteArray ret;
    ret.append(cad.size());
    for(int i=0;i<cad.size();i++)
    {
        ret.append(codificar_caracter(cad.at(i)));
    }
    return ret;

}
quint8 BBDD_FUTBOL::guardar_equipo(EQUIPO eq,QString main_path)
{
    //comprobar que existe directorio FUTBOL45_NEW
    //comprobar que existe            DBDAT
    //comprobar que existe            ESCUDOS
    //comprobar que existe            DBDAT/MINIESC
    //comprobar que existe            DBDAT/MINIFOTO
    //comprobar que existe            DBDAT/MINIENTR



    QDir dir;

    if(!dir.exists(main_path+"/FUTBOL45_NEW"))
    {dir.mkdir(main_path+"/FUTBOL45_NEW");}
    if(!dir.exists(main_path+"/FUTBOL45_NEW/DBDAT"))
    {dir.mkdir(main_path+"/FUTBOL45_NEW/DBDAT");}
    if(!dir.exists(main_path+"/FUTBOL45_NEW/ESCUDOS"))
    {dir.mkdir(main_path+"/FUTBOL45_NEW/ESCUDOS");}
    if(!dir.exists(main_path+"/FUTBOL45_NEW/DBDAT/MINIESC"))
    {dir.mkdir(main_path+"/FUTBOL45_NEW/DBDAT/MINIESC");}
    if(!dir.exists(main_path+"/FUTBOL45_NEW/DBDAT/MINIFOTO"))
    {dir.mkdir(main_path+"/FUTBOL45_NEW/DBDAT/MINIFOTO");}
    if(!dir.exists(main_path+"/FUTBOL45_NEW/DBDAT/MINIENTR"))
    {dir.mkdir(main_path+"/FUTBOL45_NEW/DBDAT/MINIENTR");}
    if(!dir.exists(main_path+"/FUTBOL45_NEW/DBDAT/BIGFOTO"))
    {dir.mkdir(main_path+"/FUTBOL45_NEW/DBDAT/BIGFOTO");}

    QByteArray DBC;
    DBC.clear();
    DBC.append(eq.cabecera);
    //DBC.append(eq.isBBDD_neg);
    //DBC.append(eq.jugable);
    DBC.append(eq.pais);
    DBC.append(eq.NombreLargo.size());
    for(int i=0;i<eq.NombreLargo.size();i++)
        DBC.append(codificar_caracter(eq.NombreLargo.at(i)));

    DBC.append(eq.NombreCorto.size());
    for(int i=0;i<eq.NombreCorto.size();i++)
        DBC.append(codificar_caracter(eq.NombreCorto.at(i)));

    DBC.append(eq.NombreMayusculas.size());
    for(int i=0;i<eq.NombreMayusculas.size();i++)
        DBC.append(codificar_caracter(eq.NombreMayusculas.at(i)));

    if(eq.isBBDD_neg==0)
    {
        DBC.append(eq.Bytes_unknow3_5bytes);
        DBC.append((quint8)(eq.anyo_fundacion&0xFF));
        DBC.append((quint8)((eq.anyo_fundacion/256)&0xFF));
    }

    DBC.append(eq.estadio.size());
    for(int i=0;i<eq.estadio.size();i++)
        DBC.append(codificar_caracter(eq.estadio.at(i)));

    DBC.append(eq.aforo&0xFF);
    DBC.append((eq.aforo/256)&0xFF);
    DBC.append((eq.aforo/256/256)&0xFF);
    DBC.append((eq.aforo/256/256/256)&0xFF);


    if(eq.isBBDD_neg==0)
    {
        DBC.append(eq.num_socios&0xFF);
        DBC.append((eq.num_socios/256)&0xFF);
        DBC.append((eq.num_socios/256/256)&0xFF);
        DBC.append((eq.num_socios/256/256/256)&0xFF);

        DBC.append(eq.presidente.size());
        for(int i=0;i<eq.presidente.size();i++)
            DBC.append(codificar_caracter(eq.presidente.at(i)));

        DBC.append(eq.presupuesto_bbdd&0xFF);
        DBC.append((eq.presupuesto_bbdd/256)&0xFF);
        DBC.append((eq.presupuesto_bbdd/256/256)&0xFF);
        DBC.append((eq.presupuesto_bbdd/256/256/256)&0xFF);

        DBC.append(eq.presupuesto_juego&0xFF);
        DBC.append((eq.presupuesto_juego/256)&0xFF);
        DBC.append((eq.presupuesto_juego/256/256)&0xFF);
        DBC.append((eq.presupuesto_juego/256/256/256)&0xFF);

        DBC.append(eq.patrocinador.size());
        for(int i=0;i<eq.patrocinador.size();i++)
            DBC.append(codificar_caracter(eq.patrocinador.at(i)));

        DBC.append(eq.proveedor.size());
        for(int i=0;i<eq.proveedor.size();i++)
            DBC.append(codificar_caracter(eq.proveedor.at(i)));

        DBC.append(eq.bytes2_array_de_10);
    }

    DBC.append(eq.nombre_corto_mayusculas.size());
    for(int i=0;i<eq.nombre_corto_mayusculas.size();i++)
        DBC.append(codificar_caracter(eq.nombre_corto_mayusculas.at(i)));

    DBC.append(eq.cosas_bbdd);

    //guardar entrenadores
    for(int j=0;j<eq.lista_entrenadores.size();j++)
    {
        DBC.append(0x02);
        DBC.append(eq.lista_entrenadores.at(j).puntero&0xFF);
        DBC.append((eq.lista_entrenadores.at(j).puntero/256)&0xFF);

        DBC.append(QString2QBytePlusSize(eq.lista_entrenadores.at(j).NombreCorto));
        if(eq.isBBDD_neg==0)
        {
            DBC.append(QString2QBytePlusSize(eq.lista_entrenadores.at(j).NombreLargo));
            //añadir info del entrenador

            DBC.append(devolver_DBC_Info_Declaraciones(eq.lista_entrenadores.at(j).Info));
            //hacer lo mismo con las declaraciones
            DBC.append(devolver_DBC_Info_Declaraciones(eq.lista_entrenadores.at(j).Declaraciones));
        }
        quint8 cero=0;
        DBC.append(cero); //añadir un cero de fin de entrenador
        //fin bucle de entrenador
    }

    //bucle de jugadores
    for(int j=0;j<eq.lista_jugadores.size();j++)
    {
        JUGADOR jug = eq.lista_jugadores.at(j);
        DBC.append(0x01); //jugador nuevo
        DBC.append(jug.puntero&0xFF);
        DBC.append((jug.puntero/256)&0xFF);
        DBC.append(QString2QBytePlusSize(jug.NombreCorto));
        DBC.append(QString2QBytePlusSize(jug.NombreLargo));
        DBC.append(jug.NumeroCamiseta); //revisar
        DBC.append(jug.EstadoFichaje);
        DBC.append(jug.DemarcacionPref);
        DBC.append(jug.DemarcacionesSecundarias);
        DBC.append(jug.NumeroCamiseta); //revisar
        quint8 zero=0;
        DBC.append(zero);
        DBC.append(jug.Extranjero);
        DBC.append(jug.Piel);
        DBC.append(jug.Pelo);
        DBC.append(jug.PosicionEnCampo); //0 portero, 1 defensor, 2 volante, 3 delantero
        if(eq.isBBDD_neg==0)
        {
            DBC.append(QString2QBytePlusSize(jug.LugarDeNacimiento));
            DBC.append(jug.FechaDeNacimiento);
            DBC.append(QString2QBytePlusSize(jug.EquipoProcedencia));
            DBC.append(QString2QBytePlusSize(jug.Internacional));
            DBC.append(jug.Altura);
            DBC.append(jug.Peso);
            DBC.append(devolver_DBC_Info_Declaraciones(jug.Comentario));
            DBC.append(jug.Precio&0xFF);
            DBC.append((jug.Precio/256)&0xFF);
        }
        DBC.append(jug.Velocidad);
        DBC.append(jug.Regate);
        DBC.append(jug.Agilidad);
        DBC.append(jug.Agresividad);

        DBC.append(jug.ActTecnica);
        DBC.append(jug.ActRemate);
        DBC.append(jug.ActPase);
        DBC.append(jug.ActTiro);
        DBC.append(zero); //fin de fichero DBC

        //-------------------
        //FIN BUCLE JUGADOR ----
    }
    int zero=0;
    DBC.append(zero);

    //FIN DEL ARCHIVO DBC

    //GRABAR LOS ARCHIVOS
    // QString("EQ95%1.DFG").arg(QString::number(equipo.EquipoIdDBC),4,QChar('0')) + "/"
    QFile arch_dbc(QString(main_path+"/FUTBOL45_NEW/DBDAT/EQ%1.DBC").arg(QString::number(eq.EquipoIdDBC),6,QChar('0')));
    if(!arch_dbc.open(QIODevice::WriteOnly))
    {
        return 0; //no se ha podido abrir para escribir
    }
    arch_dbc.write(DBC);
    arch_dbc.close();
    //grabar miniescudo
    arch_dbc.setFileName(QString(main_path+"/FUTBOL45_NEW/DBDAT/MINIESC/EQ%1.DFG").arg(QString::number(eq.EquipoIdDBC),6,QChar('0')));
    if(!arch_dbc.open(QIODevice::WriteOnly))
    {
        return 0; //no se ha podido abrir para escribir
    }
    arch_dbc.write(eq.miniesc);
    arch_dbc.close();


    //grabar escudo de simulador
    arch_dbc.setFileName(QString(main_path+"/FUTBOL45_NEW/ESCUDOS/EQ%1.EMP").arg(QString::number(eq.EquipoIdDBC),6,QChar('0')));
    if(!arch_dbc.open(QIODevice::WriteOnly))
    {
        return 0; //no se ha podido abrir para escribir
    }
    arch_dbc.write(eq.esc_sim);
    arch_dbc.close();

    //grabar minifoto entrenadores
    for(int i=0;i<eq.lista_entrenadores.size();i++)
    {
        if(!eq.lista_entrenadores.at(i).minifoto.isEmpty())
        {
            arch_dbc.setFileName(QString(main_path+"/FUTBOL45_NEW/DBDAT/MINIENTR/E95%1.DFG").arg(QString::number(eq.lista_entrenadores.at(i).puntero),5,QChar('0')));
            if(!arch_dbc.open(QIODevice::WriteOnly))
            {
                arch_dbc.close();
                return 0; //no se ha podido abrir para escribir
            }
            arch_dbc.write(eq.lista_entrenadores.at(i).minifoto);
            arch_dbc.close();
        }
    }

    //grabar minifoto jugadores
    for(int i=0;i<eq.lista_jugadores.size();i++)
    {
        if(!eq.lista_jugadores.at(i).minifoto.isEmpty())
        {
            arch_dbc.setFileName(QString(main_path+"/FUTBOL45_NEW/DBDAT/MINIFOTO/J95%1.DFG").arg(QString::number(eq.lista_jugadores.at(i).puntero),5,QChar('0')));
            if(!arch_dbc.open(QIODevice::WriteOnly))
            {
                arch_dbc.close();
                return 0; //no se ha podido abrir para escribir
            }
            arch_dbc.write(eq.lista_jugadores.at(i).minifoto);
            arch_dbc.close();
        }
    }

    //grabar bigfoto jugadores

    //cada equipo tiene una carpeta con sus jugadores
    //recorrer los jugadores por si alguno tiene un bigfoto
    bool exist_bigfoto=0;
    for(int i=0;i<eq.lista_jugadores.size();i++)
    {
        if(!eq.lista_jugadores.at(i).bigfoto.isEmpty())
        {
            exist_bigfoto=1;
            break;
        }
    }

    if(exist_bigfoto)
    {
        QString path_bigfoto = QString(main_path+"/FUTBOL45_NEW/DBDAT/BIGFOTO/EQ%1").arg(QString::number(eq.EquipoIdDBC),4,QChar('0'));
        if(!dir.exists(path_bigfoto))
        {dir.mkdir(path_bigfoto);}
        //Crear carpeta de bigfoto del equipo
        for(int i=0;i<eq.lista_jugadores.size();i++)
        {
            if(!eq.lista_jugadores.at(i).bigfoto.isEmpty())
            {
                QString path_arch = path_bigfoto + QString("/J95%1.DFG").arg(QString::number(eq.lista_jugadores.at(i).puntero),5,QChar('0'));
                arch_dbc.setFileName(path_arch);
                if(!arch_dbc.open(QIODevice::WriteOnly))
                {
                    arch_dbc.close();
                    return 0; //no se ha podido abrir para escribir
                }
                arch_dbc.write(eq.lista_jugadores.at(i).bigfoto);
                arch_dbc.close();
            }
        }
    }
    return 1;
}
