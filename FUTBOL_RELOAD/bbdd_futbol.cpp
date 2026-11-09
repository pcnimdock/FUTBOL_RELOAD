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
        else if(val==0x79)
        {return QChar(0x00B6);}   // 0xF9: comienzo de párrafo en los textos largos (¶)
        else
        {
            // Fuera de la tabla: es la letra Latin-1 tal cual. Lo usa el
            // último nombre del club («Atlético de Madrid», «LOGROÑES»…).
            // Antes se convertía en espacio.
            return QChar::fromLatin1(char(val|0x80));
        }
    }

    QChar letra=QChar::fromLatin1(val);
    return(letra);
}

quint8 BBDD_FUTBOL::codificar_caracter_directo(QChar val)
{
    const ushort u = val.unicode();
    if(u >= 0x80 && u <= 0xFF && u != 0xB6)
        return quint8(u ^ 0x61);
    return codificar_caracter(val);
}

quint8 BBDD_FUTBOL::codificar_caracter(QChar val)
{
    //comprobar si el carácter está en la tabla
    quint8 i;
    quint8 val2;
    if(val==QChar(0x00B6))
    {return quint8(0xF9^0x61);}   // ¶ = código de comienzo de párrafo
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

void BBDD_FUTBOL::configurarVersion(int version, const QString &rutaJuego)
{
    version_pcf = (version == 40) ? 40 : 45;
    carpeta_salida = (version_pcf == 40) ? "FUTBOL4_NEW" : "FUTBOL45_NEW";
    // Las copias reales de 4.0 y 4.5 llaman EQ959900.DBC a las Estrellas
    // Mundiales; se acepta también EQ959901.DBC por si alguna instalación
    // lo usa. Se elige el que exista.
    fichero_estrellas = "EQ959900.DBC";
    const QStringList dbc = QDir(rutaJuego + "/DBDAT").entryList(QDir::Files);
    for (const QString &f : dbc)
    {
        if (f.compare("EQ959901.DBC", Qt::CaseInsensitive) == 0) { fichero_estrellas = f; break; }
        if (f.compare("EQ959900.DBC", Qt::CaseInsensitive) == 0) fichero_estrellas = f;
    }
}

quint8 BBDD_FUTBOL::add_equipo(EQUIPO eq, QList<ENTRENADOR> ent_list, QList<JUGADOR> jug_list)
{
    ent_list[0].EquipoIdDBC=eq.EquipoIdDBC;
    return 1;
}

// ---------------------------------------------------------------------------
// Textos largos (biografía y declaraciones del entrenador, comentario del
// jugador). Formato del DBC: 1 byte N, y N+1 líneas, cada una con 1 byte de
// longitud y sus letras. Son LÍNEAS DE PANTALLA ya partidas por palabras
// (68-74 letras en los originales, terminadas en espacio); '\\' es salto de
// línea y el código 0xF9 (que el editor muestra como ¶) empieza párrafo.
//
// El editor antiguo las volvía a partir en trozos de 16-25 letras cortando
// palabras. Ahora: si el texto no se ha editado se escribe el bloque
// original byte a byte; si se ha editado, se parte por palabras con el
// ancho de los originales.
// ---------------------------------------------------------------------------
QByteArray BBDD_FUTBOL::bloqueTextoLargo(const QString &texto, const QString &leido,
                                         const QByteArray &bruto)
{
    if (!bruto.isEmpty() && texto == leido) return bruto;
    return devolver_DBC_Info_Declaraciones(texto);
}

QByteArray BBDD_FUTBOL::devolver_DBC_Info_Declaraciones(QString info)
{
    // Partir por palabras en líneas de hasta 'ancho' letras (cada línea
    // conserva el espacio final, como en los originales). Como mucho 256
    // líneas (el número va en un byte): si no caben se ensancha la línea.
    info.replace('\r', ' ');
    info.replace('\n', ' ');
    QStringList lineas;
    for (int ancho = 70; ; ancho += 10)
    {
        lineas.clear();
        QString actual;
        int i = 0;
        while (i < info.size())
        {
            // siguiente palabra con sus espacios detrás
            int j = i;
            while (j < info.size() && info.at(j) != ' ') j++;
            while (j < info.size() && info.at(j) == ' ') j++;
            QString palabra = info.mid(i, j - i);
            i = j;
            while (palabra.size() > ancho)          // palabra más larga que la línea
            {
                if (!actual.isEmpty()) { lineas << actual; actual.clear(); }
                lineas << palabra.left(ancho);
                palabra = palabra.mid(ancho);
            }
            if (actual.size() + palabra.trimmed().size() > ancho && !actual.isEmpty())
            {
                lineas << actual;
                actual.clear();
            }
            actual += palabra;
        }
        if (!actual.isEmpty() || lineas.isEmpty()) lineas << actual;
        if (lineas.size() <= 256 || ancho >= 250) break;
    }
    while (lineas.size() > 256) lineas.removeLast();

    QByteArray ret;
    ret.append(static_cast<char>(lineas.size() - 1));
    for (const QString &l : lineas)
    {
        const QString t = l.left(255);
        ret.append(static_cast<char>(t.size()));
        for (QChar c : t) ret.append(codificar_caracter(c));
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
    //comprobar que existe el directorio de salida (FUTBOL4_NEW / FUTBOL45_NEW)
    //comprobar que existe            DBDAT
    //comprobar que existe            ESCUDOS
    //comprobar que existe            DBDAT/MINIESC
    //comprobar que existe            DBDAT/MINIFOTO
    //comprobar que existe            DBDAT/MINIENTR



    QDir dir;

    if(!dir.exists(main_path+"/"+carpeta_salida+""))
    {dir.mkdir(main_path+"/"+carpeta_salida+"");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/ESCUDOS"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/ESCUDOS");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT/MINIESC"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT/MINIESC");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT/MINIFOTO"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT/MINIFOTO");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT/MINIENTR"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT/MINIENTR");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT/BIGFOTO"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT/BIGFOTO");}

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

    // Este nombre guarda las letras acentuadas en Latin-1 directo (é = 0xE9),
    // no con la tabla del juego como los demás (visto en 5 equipos).
    DBC.append(eq.nombre_corto_mayusculas.size());
    for(int i=0;i<eq.nombre_corto_mayusculas.size();i++)
        DBC.append(codificar_caracter_directo(eq.nombre_corto_mayusculas.at(i)));

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

            DBC.append(bloqueTextoLargo(eq.lista_entrenadores.at(j).Info, eq.lista_entrenadores.at(j).InfoLeido, eq.lista_entrenadores.at(j).InfoBruto));
            //hacer lo mismo con las declaraciones
            DBC.append(bloqueTextoLargo(eq.lista_entrenadores.at(j).Declaraciones, eq.lista_entrenadores.at(j).DeclaracionesLeido, eq.lista_entrenadores.at(j).DeclaracionesBruto));
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
        // Posición en la pantalla ALINEACIÓN (antes se grababa por error el
        // dorsal duplicado; ambos suelen coincidir en las plantillas
        // originales, pero divergen al reordenar o añadir jugadores).
        DBC.append(jug.PosicionEnAlineacion);
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
            DBC.append(bloqueTextoLargo(jug.Comentario, jug.ComentarioLeido, jug.ComentarioBruto));
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
    QFile arch_dbc(QString(main_path+"/"+carpeta_salida+"/DBDAT/EQ%1.DBC").arg(QString::number(eq.EquipoIdDBC),6,QChar('0')));
    if(!arch_dbc.open(QIODevice::WriteOnly))
    {
        return 0; //no se ha podido abrir para escribir
    }
    arch_dbc.write(DBC);
    arch_dbc.close();
    //grabar miniescudo
    arch_dbc.setFileName(QString(main_path+"/"+carpeta_salida+"/DBDAT/MINIESC/EQ%1.DFG").arg(QString::number(eq.EquipoIdDBC),6,QChar('0')));
    if(!arch_dbc.open(QIODevice::WriteOnly))
    {
        return 0; //no se ha podido abrir para escribir
    }
    arch_dbc.write(eq.miniesc);
    arch_dbc.close();


    //grabar escudo de simulador
    arch_dbc.setFileName(QString(main_path+"/"+carpeta_salida+"/ESCUDOS/EQ%1.EMP").arg(QString::number(eq.EquipoIdDBC),6,QChar('0')));
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
            arch_dbc.setFileName(QString(main_path+"/"+carpeta_salida+"/DBDAT/MINIENTR/E95%1.DFG").arg(QString::number(eq.lista_entrenadores.at(i).puntero),5,QChar('0')));
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
            arch_dbc.setFileName(QString(main_path+"/"+carpeta_salida+"/DBDAT/MINIFOTO/J95%1.DFG").arg(QString::number(eq.lista_jugadores.at(i).puntero),5,QChar('0')));
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
        QString path_bigfoto = QString(main_path+"/"+carpeta_salida+"/DBDAT/BIGFOTO/EQ%1").arg(QString::number(eq.EquipoIdDBC),4,QChar('0'));
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

quint8 BBDD_FUTBOL::guardar_estrellas(ESTRELLAS_MUNDIALES eq, QString main_path)
{
    //comprobar que existe el directorio de salida (FUTBOL4_NEW / FUTBOL45_NEW)
    //comprobar que existe            DBDAT
    //comprobar que existe            ESCUDOS
    //comprobar que existe            DBDAT/MINIESC
    //comprobar que existe            DBDAT/MINIFOTO
    //comprobar que existe            DBDAT/MINIENTR



    QDir dir;

    if(!dir.exists(main_path+"/"+carpeta_salida+""))
    {dir.mkdir(main_path+"/"+carpeta_salida+"");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/ESCUDOS"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/ESCUDOS");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT/MINIESC"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT/MINIESC");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT/MINIFOTO"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT/MINIFOTO");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT/MINIENTR"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT/MINIENTR");}
    if(!dir.exists(main_path+"/"+carpeta_salida+"/DBDAT/BIGFOTO"))
    {dir.mkdir(main_path+"/"+carpeta_salida+"/DBDAT/BIGFOTO");}

    QByteArray DBC;
    DBC.clear();

    /*
    quint32 pos=0;
    int size_cad;
    for(pos=0;pos<0x2C;pos++){
        cabecera.append(datos.at(pos));}
    estrellas_mundiales.EquipoIdDBC=datos.at(pos++)&0xFF;
    estrellas_mundiales.EquipoIdDBC+=datos.at(pos++)*256;
    */
    DBC.append(eq.cabecera);
    DBC.append(eq.estrellas_mundiales.EquipoIdDBC&0xFF);
    DBC.append(eq.estrellas_mundiales.EquipoIdDBC/256);

    /*

    size_cad=datos.at(pos++);
    for(int i=0;i<size_cad;i++)
    {
        estrellas_mundiales.NombreCorto.append(transform.descodificar_caracter(datos.at(pos++)));
    }
    */
    uint8_t size_temp,temp;
    size_temp=eq.estrellas_mundiales.NombreCorto.size();
    DBC.append(size_temp);
    for(temp=0;temp<size_temp;temp++)
    {
        DBC.append(codificar_caracter(eq.estrellas_mundiales.NombreCorto.at(temp)));
    }

    /*

    for(int i=0;i<43;i++)
    {estrellas_mundiales.bytes2_array_de_10.append(pos++);} //se guarda aquí por guardalo en algún lado
*/
    for(temp=0;temp<43;temp++)
    {
        DBC.append(eq.estrellas_mundiales.bytes2_array_de_10.at(temp));
    }


    /*
    size_cad=datos.at(pos++);
    for(int i=0;i<size_cad;i++)
    {
        estrellas_mundiales.nombre_corto_mayusculas.append(transform.descodificar_caracter(datos.at(pos++))); //en el archivo pone "SEGUNDA B", debe de tener alguna relación
    }
    */
    size_temp=eq.estrellas_mundiales.nombre_corto_mayusculas.size();
    DBC.append(size_temp);
    for(temp=0;temp<size_temp;temp++)
    {
        DBC.append(codificar_caracter(eq.estrellas_mundiales.nombre_corto_mayusculas.at(temp)));
    }

    /*

    for(int i=0;i<275;i++)
    { //antes de llegar a bautista hay una zona que pone "Comentario" ¿alguna relación con algo?
        estrellas_mundiales.cosas_bbdd.append(datos.at(pos++));
    }
*/
    for(uint16_t i=0;i<275;i++)
    {
        DBC.append(eq.estrellas_mundiales.cosas_bbdd.at(i));
    }

    size_temp=eq.estrellas_mundiales.lista_jugadores.size(); //numero de jugadores
    temp=0;
    JUGADOR j;
    uint16_t temp_j=0;
    while(temp_j<eq.list_jugador.size())
    {
        j=eq.list_jugador.at(temp_j);
        DBC.append(j.puntero&0xFF);
        DBC.append(j.puntero/256);
        //NombreCorto
        DBC.append(j.NombreCorto.size());
        temp=0;
        while(temp<j.NombreCorto.size())
        {DBC.append(codificar_caracter(j.NombreCorto.at(temp++)));}
        //NombreLargo
        temp=0;
        DBC.append(j.NombreLargo.size());
        while(temp<j.NombreLargo.size())
        {DBC.append(codificar_caracter(j.NombreLargo.at(temp++)));}
        //NumeroCamiseta
        DBC.append(j.NumeroCamiseta);
        //EstadoFichaje
        DBC.append(j.EstadoFichaje);
        //DemarcacionPref
        DBC.append(j.DemarcacionPref);
        temp=0;
        while(temp<5)
        {DBC.append(j.DemarcacionesSecundarias.at(temp++));}
        DBC.append(j.NumeroCamiseta);
        DBC.append('\0');
        DBC.append(j.Extranjero);
        DBC.append(j.Piel);
        DBC.append(j.Pelo);
        DBC.append(j.PosicionEnCampo);
        DBC.append(QString2QBytePlusSize(j.LugarDeNacimiento));
        DBC.append('\0'); //Fecha de nacimiento
        DBC.append('\0');
        DBC.append('\0');
        DBC.append(j.EquipoProcedencia.size());
        temp=0;
        while(temp<j.EquipoProcedencia.size())
        {
            DBC.append(codificar_caracter(j.EquipoProcedencia.at(temp++)));
        }
        DBC.append(j.Internacional.size());
        temp=0;
        while(temp<j.Internacional.size())
        {
            DBC.append(codificar_caracter(j.Internacional.at(temp++)));
        }
        DBC.append(j.Altura);
        DBC.append(j.Peso);
        DBC.append('\0'); //Numero de cadenas
        temp=0;
        QString com="Comentario";
        DBC.append(com.size());
        while(temp<com.size())
        {
            DBC.append(codificar_caracter(com.at(temp++)));
        }
        DBC.append(j.Precio&0xFF);
        DBC.append(j.Precio/256);
        DBC.append(j.Velocidad);
        DBC.append(j.Regate);
        DBC.append(j.Agilidad);
        DBC.append(j.Agresividad);
        DBC.append(j.ActTecnica);
        DBC.append(j.ActRemate);
        DBC.append(j.ActPase);
        DBC.append(j.ActTiro);
        DBC.append('\0');
        temp_j++;
        if(temp_j<eq.list_jugador.size())
        {
        DBC.append('\1');
        }
        else
        {
            DBC.append('\0');
        }

        //guardar minifoto si existe

    }

    QFile arch_dbc(QString(main_path+"/"+carpeta_salida+"/DBDAT/"+fichero_estrellas));
    if(!arch_dbc.open(QIODevice::WriteOnly))
    {
        return 0; //no se ha podido abrir para escribir
    }
    arch_dbc.write(DBC);
    arch_dbc.close();

    //grabar minifoto jugadores
    for(int i=0;i<eq.list_jugador.size();i++)
    {
        if(!eq.list_jugador.at(i).minifoto.isEmpty())
        {
            arch_dbc.setFileName(QString(main_path+"/"+carpeta_salida+"/DBDAT/MINIFOTO/J95%1.DFG").arg(QString::number(eq.list_jugador.at(i).puntero),5,QChar('0')));
            if(!arch_dbc.open(QIODevice::WriteOnly))
            {
                arch_dbc.close();
                return 0; //no se ha podido abrir para escribir
            }
            arch_dbc.write(eq.list_jugador.at(i).minifoto);
            arch_dbc.close();
        }
    }

    return 1;
}
