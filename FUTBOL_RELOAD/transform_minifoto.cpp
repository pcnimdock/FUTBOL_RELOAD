#include "transform_minifoto.h"
#include "paleta_escudos_sim.h"
#include "QColor"

qint32 paleta_minifoto[16]=
{
    0x0000,
    0x186d14,
    0x86aa1c,
    0xa6b6cb,
    0xf7db18,
    0x0010e3,
    0xf70000,
    0x618eb6,
    0x592400,
    0x9a6124,
    0xdb9224,
    0xffd786,
    0x696969,
    0x9a9a9a,
    0xc7c7c7,
    0xf7f7f7,
};
qint32 paleta_miniesc[16]=
{
    0x0000,
    0x186d14,
    0x86aa1c,
    0xaebed3,
    0xffe320,
    0x0000ff,
    0xef2000,
    0x6996be,
    0x6da6aa,
    0x004975,
    0xa2d324,
    0x005961,
    0x717171,
    0xa2a2a2,
    0xcfcfcf,
    0xffffff,

};

const unsigned char cab_minifoto_predef[58] =
{
    0x00, 0x10, 0x00, 0x00,
    0x00, 0x18, 0x6c, 0x14,
    0x84, 0xa8, 0x1c, 0xa4,
    0xb4, 0xc8, 0xf4, 0xd8,
    0x18, 0x00, 0x10, 0xe0,
    0xf4, 0x00, 0x00, 0x60,
    0x8c, 0xb4, 0x58, 0x24,
    0x00, 0x98, 0x60, 0x24,
    0xd8, 0x90, 0x24, 0xfc,
    0xd4, 0x84, 0x68, 0x68,
    0x68, 0x98, 0x98, 0x98,
    0xc4, 0xc4, 0xc4, 0xf4,
    0xf4, 0xf4, 0x01, 0x18,
    0x00, 0x18, 0x00, 0x04,
    0x00, 0x00
};

TRANSFORM_MINIFOTO::TRANSFORM_MINIFOTO()
{
    cabecera_minifoto_standar.clear();
    for(int i=0;i<58;i++)
    {
        cabecera_minifoto_standar.append(cab_minifoto_predef[i]);
    }
}

QImage TRANSFORM_MINIFOTO::foto2QImage(QByteArray minifoto_dfg,quint8 MINI_ESC_FOTO)
{
    unsigned int i;
    qint32 color_r,color_g,color_b;
    qint16 ancho=24;
    qint16 alto=24;

    if(MINI_ESC_FOTO==MINI_FOTO)
    {
        ancho=24;
        alto=24;
    }
    else
    {
        ancho=48; //pixels ancho MINIESC
        alto=64; //pixels alto de una MINIESC
    }
    colors.clear();
    cabecera.clear();

    if(MINI_ESC_FOTO==MINI_FOTO)
    {
        for (int i=0;i<16;i++)
        {
            color_r=(paleta_minifoto[i]>>16)&0xFF;
            color_g=(paleta_minifoto[i]>>8)&0xFF;
            color_b=(paleta_minifoto[i])&0xFF;
            colors.append(qRgb(color_r,color_g,color_b));
        }
    }
    else
    {
        for (i=0;i<16;i++)
        {
            color_r=(paleta_miniesc[i]>>16)&0xFF;
            color_g=(paleta_miniesc[i]>>8)&0xFF;
            color_b=(paleta_miniesc[i])&0xFF;
            colors.append(qRgb(color_r,color_g,color_b));
        }
    }

    //qint16 ancho=48; //pixels ancho MINIESC
    //qint16 alto=64; //pixels alto de una MINIESC
    //crear imagen vacia en formato de 8bits
    QImage  *imagen= new QImage(ancho,alto,QImage::Format_Indexed8);

    //añadir la paleta a la imagen
    imagen->setColorTable(colors);

    quint8 a[17]; //vector de los 16 primeros bytes de linea
    quint8 b[17]; //""     ""  ""  "" segundos ""
    quint8 c[17];//""      ""  ""  "" terceros
    quint8 d[17];//""      ""  "" ""  ultimos

    if(MINI_ESC_FOTO==MINI_FOTO)
    {   cabecera_minifoto.clear();


        int pos;
        int size_cabecera;
        if(minifoto_dfg.size()<300)
        {
            size_cabecera=8;
        }
        else
        {
            size_cabecera=58;
        }
        for(pos=0;pos<size_cabecera;pos++)
        {
            cabecera_minifoto.append(minifoto_dfg.at(pos));
            //quitar cabecera
        }

        a[0]=minifoto_dfg.at(pos);
        for(int z=0;z<alto;z++)
        {
            //El dfg se divide en 4 bloques
            //la combinacion de los 4 bloques contiene el indice de colores
            //Leemos la fila completa
            //en 4 partes
            int j=0;
            while(j<ancho/8)
            {
                a[j]=minifoto_dfg.at(pos++);
                j++;
            }
            j=0;
            while(j<ancho/8)
            {    b[j]=minifoto_dfg.at(pos++);j++;
            }
            j=0;
            while(j<ancho/8)
            {    c[j]=minifoto_dfg.at(pos++);j++;
            }
            j=0;
            while(j<ancho/8)
            {    d[j]=minifoto_dfg.at(pos++);j++;
            }
            j=0;
            while(j<ancho/8)//el número de bytes capturados, 6 para miniesc
            {
                //hay que imprimir 48 pixels por fila

                //obtenemos el color de 8 píxeles

                quint16 ui_dato[8];
                i=0;
                while(i<8)
                {
                    //tratamiento de los 8 bits
                    ui_dato[7-i]=(((a[j]>>i)&0x01)<<0 )+ (((b[j]>>i)&0x01)<<1) + (((c[j]>>i)&0x01)<<2) +
                            (((d[j]>>i)&0x01)<<3);
                    i++;
                }
                //ya tenemos los colores de 8 píxeles
                //aplicamos en qimage
                i=0;
                while(i<8)
                {
                    imagen->setPixel(i+8*j,z,(ui_dato[i]));
                    i++;
                }

                j++;
            }


        }
    }
    else
    {
        cabecera_miniesc.clear();
        for(i=0;i<8;i++)
        {
            cabecera_miniesc.append(minifoto_dfg.at(i));//quitar cabecera
        }
        quint32 pos=8;
        for(int z=0;z<alto;z++)
        {
            //El dfg se divide en 4 bloques
            //la combinacion de los 4 bloques contiene el indice de colores
            //Leemos la fila completa
            //en 4 partes
            int j=0;
            while(j<ancho/8)
            {
                //in>>a[j];
                a[j]=minifoto_dfg[pos++];
                j++;
            }
            j=0;
            while(j<ancho/8)
            {    //in>>b[j];
                b[j]=minifoto_dfg[pos++];
                j++;
            }
            j=0;
            while(j<ancho/8)
            {   // in>>c[j];
                c[j]=minifoto_dfg[pos++];
                j++;
            }
            j=0;
            while(j<ancho/8)
            {    //in>>d[j];
                d[j]=minifoto_dfg[pos++];
                j++;
            }
            j=0;
            while(j<ancho/8)//el número de bytes capturados, 6 para miniesc
            {
                //hay que imprimir 48 pixels por fila

                //obtenemos el color de 8 píxeles

                quint16 ui_dato[8];
                i=0;
                while(i<8)
                {
                    //tratamiento de los 8 bits
                    ui_dato[7-i]=(((a[j]>>i)&0x01)<<0 )+ (((b[j]>>i)&0x01)<<1) + (((c[j]>>i)&0x01)<<2) +
                            (((d[j]>>i)&0x01)<<3);
                    i++;
                }
                //ya tenemos los colores de 8 píxeles
                //aplicamos en qimage
                i=0;
                while(i<8)
                {
                    imagen->setPixel(i+8*j,z,(ui_dato[i]));
                    i++;
                }

                j++;
            }


        }
    }
    QImage Image_return = *imagen;
    delete imagen;
    return Image_return;
}

QByteArray TRANSFORM_MINIFOTO::devolver_foto_reemplazo(QImage foto,quint8 MINI_ESC_FOTO)
{
    QByteArray data;
    qint16 color_pixel;
    qint32 color_r,color_g,color_b;

    //QImage *imagen2=new QImage();
    //*imagen2=foto;
    qint16 ancho=24;
    qint16 alto=24;
    colors.clear();

    if(MINI_ESC_FOTO==MINI_FOTO)
    {
        for (int i=0;i<16;i++)
        {
            color_r=(paleta_minifoto[i]>>16)&0xFF;
            color_g=(paleta_minifoto[i]>>8)&0xFF;
            color_b=(paleta_minifoto[i])&0xFF;
            colors.append(qRgb(color_r,color_g,color_b));
        }
    }
    else
    {
        for (int i=0;i<16;i++)
        {
            color_r=(paleta_miniesc[i]>>16)&0xFF;
            color_g=(paleta_miniesc[i]>>8)&0xFF;
            color_b=(paleta_miniesc[i])&0xFF;
            colors.append(qRgb(color_r,color_g,color_b));
        }
    }
    if(MINI_ESC_FOTO==MINI_FOTO)
    {
        ancho=24;
        alto=24;
        foto = foto.scaled(24,24,Qt::IgnoreAspectRatio);
        foto = foto.convertToFormat(QImage::Format_Indexed8,colors);

    }
    else
    {
        ancho=48;
        alto=64;
        foto = foto.scaled(48,64,Qt::IgnoreAspectRatio);
        foto = foto.convertToFormat(QImage::Format_Indexed8,colors);

    }

    QImage imagen3=foto;//imagen2->convertToFormat(QImage::Format_Indexed8,colors);


    quint8 a[ancho/8]; //vector de los 16 primeros bytes de linea
    quint8 b[ancho/8]; //""     ""  ""  "" segundos ""
    quint8 c[ancho/8];//""      ""  ""  "" terceros
    quint8 d[ancho/8];//""      ""  "" ""  ultimos
    data.clear();


    for(int i=0;i<alto;i++)
    {
        //recorrer todas la filas
        for(int z=0; z<ancho/8;z++) //inicializar todo a 0
        {a[z]=b[z]=c[z]=d[z]=0;}
        int j;
        for(j=0;j<ancho;j++)
        {
            //recorrer todos los píxeles de la fila
            color_pixel=imagen3.pixelIndex(j,i);
            a[j/8]|=(color_pixel&0x01)<<(7-j%8);
            b[j/8]|=((color_pixel>>1)&0x01)<<(7-j%8);
            c[j/8]|=((color_pixel>>2)&0x01)<<(7-j%8);
            d[j/8]|=((color_pixel>>3)&0x01)<<(7-j%8);


        }
        //se ha recorrido toda la fila, guardar en qbytearray para posterior tratamiento
        for(j=0;j<ancho/8;j++)
        {
            data.append(a[j]);
        }
        for(j=0;j<ancho/8;j++)
        {
            data.append(b[j]);
        }
        for(j=0;j<ancho/8;j++)
        {
            data.append(c[j]);
        }
        for(j=0;j<ancho/8;j++)
        {
            data.append(d[j]);
        }
    }
    //se ha recorrido todos los píxeles de la imagen y ya se tiene la imagen codificada
    QByteArray dfg;
    if(MINI_ESC_FOTO==MINI_FOTO)
    {
        if(cabecera_minifoto.isEmpty())
        {
            dfg.append(cabecera_minifoto_standar);
        }
        else
        {
            dfg.append(cabecera_minifoto);
        }
    }
    else
    {
        dfg.append(cabecera_miniesc);

    }
    dfg.append(data);
    dfg.append(0xFF);
    return dfg;

}

QImage TRANSFORM_MINIFOTO::miniesc2QImage(QByteArray miniesc_dfg)
{

    return foto2QImage(miniesc_dfg,MINI_ESC);

}

QByteArray TRANSFORM_MINIFOTO::devolver_miniesc_reemplazo(QImage foto)
{
    return devolver_foto_reemplazo(foto,MINI_ESC);
}

QImage TRANSFORM_MINIFOTO::minifoto2QImage(QByteArray miniesc_dfg)
{
    return foto2QImage(miniesc_dfg,MINI_FOTO);
}

QByteArray TRANSFORM_MINIFOTO::devolver_minifoto_reemplazo(QImage foto)
{
    return devolver_foto_reemplazo(foto,MINI_FOTO);
}

QImage TRANSFORM_MINIFOTO::fotosim2QImage(QByteArray escudo_sim)
{
    unsigned int i;
    quint8 data;
    quint32 pos=0;
    cabecera_escudo_sim.clear();

    for(i=0;i<0x18;i++)
    {
        cabecera_escudo_sim.append(escudo_sim.at(i)); //in>>data;//mandar a la basura los bytes
    }
    pos=0x18;
    colors.clear();
    qint32 color_r,color_g,color_b;
    for (i=0;i<256;i++)
    {
        color_r=paleta_escudos_sim[i][0];
        color_g=paleta_escudos_sim[i][1];
        color_b=paleta_escudos_sim[i][2];
        colors.append(qRgb(color_r,color_g,color_b));
    }
    qint16 ancho=52; //pixels ancho MINIESC
    qint16 alto=65; //pixels alto de una MINIESC

    if(escudo_sim.size()>1500)
    {
        ancho=52; //pixels ancho MINIESC
        alto=65; //pixels alto de una MINIESC

    }
    else
    {
        ancho=32; //pixels ancho MINIESC
        alto=45; //pixels alto de una MINIESC

    }
    //crear imagen vacia en formato de 8bits
    QImage  *imagen= new QImage(ancho,alto,QImage::Format_Indexed8);

    //añadir la paleta a la imagen
    imagen->setColorTable(colors);


    qint8 fila_imagen,columna_imagen;
    qint8 recorrido;
    if(escudo_sim.size()>(1500))
    {
        for (recorrido=0;recorrido<4;recorrido++)
        {
            for(fila_imagen=0;fila_imagen<65;fila_imagen++)
            {
                columna_imagen=recorrido;
                while(columna_imagen<52)
                {
                    data=escudo_sim.at(pos++);
                    imagen->setPixel(columna_imagen,fila_imagen,((uint)data));
                    columna_imagen=columna_imagen+4;
                }

            }
        }
    }
    else
    {
        for (recorrido=0;recorrido<4;recorrido++)
        {
            for(fila_imagen=0;fila_imagen<45;fila_imagen++)
            {
                columna_imagen=recorrido;
                while(columna_imagen<32)
                {
                    data=escudo_sim.at(pos++);
                    imagen->setPixel(columna_imagen,fila_imagen,((uint)data));
                    columna_imagen=columna_imagen+4;
                }

            }
        }

    }
    QImage Imagen_return = *imagen;
    delete imagen;

    return Imagen_return;
}

QByteArray TRANSFORM_MINIFOTO::devolver_fotosim_reemplazo(QImage escudo_sim)
{
    qint8 data_temp;
    QByteArray dfg;
    qint16 ancho=52;
    qint16 alto=65;

    if(escudo_sim.size().width()==64)
    {
        ancho=32;
        alto=45;
    }
    qint8 fila_imagen,columna_imagen;
    qint8 recorrido;

    QImage *imagen2=new QImage();
    *imagen2=escudo_sim;
    if(escudo_sim.size().width()==64)
    {
        *imagen2=imagen2->scaled(32,45,Qt::IgnoreAspectRatio);
    }
    else
    {
    *imagen2=imagen2->scaled(52,65,Qt::IgnoreAspectRatio);
    }
    colors.clear();
    qint32 color_r,color_g,color_b;
    for (int i=0;i<256;i++)
    {
        color_r=paleta_escudos_sim[i][0];
        color_g=paleta_escudos_sim[i][1];
        color_b=paleta_escudos_sim[i][2];
        colors.append(qRgb(color_r,color_g,color_b));
    }
    QImage imagen3=imagen2->convertToFormat(QImage::Format_Indexed8,colors);

    dfg.clear();
    dfg.append(cabecera_escudo_sim);

    //escribir índices en el archivo

    if(escudo_sim.size().width()!=64)
    {
    for (recorrido=0;recorrido<4;recorrido++)
    {
        for(fila_imagen=0;fila_imagen<alto;fila_imagen++)
        {
            columna_imagen=recorrido;
            while(columna_imagen<ancho)
            {
                dfg.append(((qint8)imagen3.pixelIndex(columna_imagen,fila_imagen)));
                columna_imagen=columna_imagen+4;
            }

        }
    }
    }
    else
    {
        for (recorrido=0;recorrido<4;recorrido++)
        {
            for(fila_imagen=0;fila_imagen<45;fila_imagen++) //alto en pixels
            {
                columna_imagen=recorrido;
                while(columna_imagen<32)//ancho en pixels
                {
                    dfg.append(((qint8)imagen3.pixelIndex(columna_imagen,fila_imagen)));
                    columna_imagen=columna_imagen+4;
                }

            }
        }
    }

    // data_temp=0xFF;
    //out<< data_temp;
    data_temp=0;
    dfg.append(data_temp);
    dfg.append(data_temp);
    dfg.append(data_temp);
    dfg.append(data_temp);
    delete imagen2;
    return dfg;
}

const quint8 caracteres[48]={
    0xC7 ,0xFC ,0xE9 ,0xE2 ,0xE4 ,0xE0 ,0xE5 ,0xE7 ,0xEA ,0xEB ,
    0xE8 ,0xEF ,0xEE ,0xEC ,0xC4 ,0xC5 ,0xC9 ,0xE6 ,0xC6 ,0xF4 ,
    0xF6 ,0xF2 ,0xFB ,0xF9 ,0xFF ,0xD6 ,0xDC ,0xA2 ,0xA3 ,0xA5 ,
    0x00 ,0x00 ,0xE1 ,0xED ,0xF3 ,0xFA ,0xF1 ,0xD1 ,0xAA ,0xBA ,
    0xBF ,0x81 ,0xAC ,0xBD ,0xBC ,0xA1 ,0xAB ,0xBB};

QChar TRANSFORM_MINIFOTO::descodificar_caracter(quint8 val)
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
    return(QChar::fromLatin1(val));
}

quint8 TRANSFORM_MINIFOTO::codificar_caracter(QChar val)
{
    //comprobar si el carácter está en la tabla
    quint8 i;
    for(i=0;i<48;i++)
    {
        if(caracteres[i]==val.toLatin1())
        {break;}
    }
    if(i<48)
    {
        //el caracter existe en la tabla
        i+=0x80;
        i^=61;
    }
    else
    { //no existe en la tabla
        i^=61;
    }
    return(i);
}

QImage TRANSFORM_MINIFOTO::bigfoto2QImage(QByteArray data)
{
    unsigned int i;
    colors_bigfoto.clear();
    cabecera_bigfoto.clear();



    qint8 color[3];
    int pos=0;
    for(i=0;i<2;i++)
    {
        cabecera_bigfoto.append(data.at(pos++)); //leer dos datos que se repiten en todas los BIFOTO

    }


    for(i=0;i<16;i++)
    {
        //leer la paleta de colores en el archivo
        cabecera_bigfoto.append(data.at(pos));
        color[0]=data.at(pos++);
        cabecera_bigfoto.append(data.at(pos));
        color[1]=data.at(pos++);
        cabecera_bigfoto.append(data.at(pos));
        color[2]=data.at(pos++);
        //añadir color a la paleta
        colors_bigfoto.append(qRgb((qint16)color[0],(qint16)color[1],(qint16)color[2]));
    }
    for(i=0;i<8;i++)
    {
        //tirar a la basura los siguientes 8 bytes
        cabecera_bigfoto.append(data.at(pos));
        color[0]=data.at(pos++);
    }

    qint16 ancho=128; //pixels ancho de una BIGFOTO
    qint16 alto=182; //pixels alto de una BIGFOTO
    //crear imagen vacia en formato de 8bits
    QImage  *imagen= new QImage(ancho,alto,QImage::Format_Indexed8);

    //añadir la paleta a la imagen
    imagen->setColorTable(colors_bigfoto);

    quint8 a[17]; //vector de los 16 primeros bytes de linea
    quint8 b[17]; //""     ""  ""  "" segundos ""
    quint8 c[17];//""      ""  ""  "" terceros
    quint8 d[17];//""      ""  "" ""  ultimos

    for(int z=0;z<alto;z++)
    {
        //El dfg se divide en 4 bloques
        //la combinacion de los 4 bloques contiene el indice de colores
        //Leemos la fila completa
        //en 4 partes
        for(int j=0;j<16;j++)
        {
            a[j]=data.at(pos++);
        }
        for(int j=0;j<16;j++)
        {
            b[j]=data.at(pos++);;
        }
        for(int j=0;j<16;j++)
        {
            c[j]=data.at(pos++);
        }
        for(int j=0;j<16;j++)
        {
            d[j]=data.at(pos++);
        }

        for(int j=0;j<16;j++)
        {
            //tratamos la fila completa
            quint16 ui_dato[8];
            for(i=0;i<8;i++)
            {
                //tratamiento de los 8 bits
                ui_dato[7-i]=(((a[j]>>i)&0x01)<<0 )+ (((b[j]>>i)&0x01)<<1) + (((c[j]>>i)&0x01)<<2) +
                        (((d[j]>>i)&0x01)<<3);
            }
            //aplicamos en qimage
            for(i=0;i<8;i++)
            {
                imagen->setPixel(i+8*j,z,(ui_dato[i]));

            }


        }


    }
    QImage ret;
    ret = *imagen;
    return ret;
}

/* .0 { color: rgb(22, 21, 33) }
 .1 { color: rgb(58, 38, 23) }
 .2 { color: rgb(51, 44, 54) }
 .3 { color: rgb(40, 47, 75) }
 .4 { color: rgb(77, 71, 83) }
 .5 { color: rgb(64, 75, 110) }
 .6 { color: rgb(107, 84, 68) }
 .7 { color: rgb(114, 85, 43) }
 .8 { color: rgb(103, 103, 116) }
 .9 { color: rgb(96, 104, 140) }
 .10 { color: rgb(127, 137, 161) }
 .11 { color: rgb(143, 137, 131) }
 .12 { color: rgb(167, 145, 108) }
 .13 { color: rgb(171, 172, 166) }
 .14 { color: rgb(199, 202, 197) }
 .15 { color: rgb(232, 235, 232) }
 */
quint16 color_bigfoto_predef[16][3]=
{
    {22,21,33},
    {58,38,23},
    {51,44,54},
    {40,47,75},
    {77,71,83},
    {64,75,110},
    {107,84,68},
    {114,85,43},
    {103,103,116},
    {96,104,140},
    {127,137,161},
    {143,137,131},
    {167,145,108},
    {171,172,166},
    {199,202,197},
    {232,235,232}

};

const unsigned char bytes_de_bigfoto[8] =
{
    0x01, 0x7c, 0x00, 0xb6,
    0x00, 0x04, 0x00, 0x00
};


QByteArray TRANSFORM_MINIFOTO::devolver_foto_bigfoto(QImage im)
{
    //convertir
    QByteArray data;
    qint16 color_pixel;

    qint16 ancho=128;
    qint16 alto=182;

    QImage *imagen2=&im;
    *imagen2=imagen2->scaled(ancho,alto,Qt::IgnoreAspectRatio);
    if(colors_bigfoto.isEmpty())
    {
        //se está incluyendo una imagen nueva
        colors_bigfoto=im.colorTable();
        if(colors_bigfoto.count()!=16)
        {
            //esta foto no es compatible
            //se le colocará una tabla predefinida
            cabecera_bigfoto.clear();
            quint8 zero=0;
            cabecera_bigfoto.append(zero);
            cabecera_bigfoto.append(0x10);

            for (int i=0;i<16;i++)
            {
                colors_bigfoto.append(qRgb(color_bigfoto_predef[i][0],
                                      color_bigfoto_predef[i][1],
                        color_bigfoto_predef[i][2]));
                cabecera_bigfoto.append(color_bigfoto_predef[i][0]);
                cabecera_bigfoto.append(color_bigfoto_predef[i][1]);
                cabecera_bigfoto.append(color_bigfoto_predef[i][2]);
            }
            for(int i=0; i<8;i++)
            {
                cabecera_bigfoto.append(bytes_de_bigfoto[i]);
            }

        }
        else
        {
            //se ha metido una imagen nueva de 16 colores
            cabecera_bigfoto.clear();
            quint8 zero=0;
            cabecera_bigfoto.append(zero);
            cabecera_bigfoto.append(0x10);

            for (int i=0;i<16;i++)
            {
                QColor color=QColor(colors_bigfoto.at(i));
                cabecera_bigfoto.append(color.red());
                cabecera_bigfoto.append(color.green());
                cabecera_bigfoto.append(color.blue());
            }
            for(int i=0; i<8;i++)
            {
                cabecera_bigfoto.append(bytes_de_bigfoto[i]);
            }

        }

    }
    QImage imagen3=imagen2->convertToFormat(QImage::Format_Indexed8,colors_bigfoto);


    quint8 a[ancho/8]; //vector de los 16 primeros bytes de linea
    quint8 b[ancho/8]; //""     ""  ""  "" segundos ""
    quint8 c[ancho/8];//""      ""  ""  "" terceros
    quint8 d[ancho/8];//""      ""  "" ""  ultimos
    data.clear();
    data.append(cabecera_bigfoto);

    for(int i=0;i<alto;i++)
    {
        //recorrer todas la filas
        for(int z=0; z<ancho/8;z++) //inicializar todo a 0
        {a[z]=b[z]=c[z]=d[z]=0;}
        int j;
        for(j=0;j<ancho;j++)
        {
            //recorrer todos los píxeles de la fila
            color_pixel=imagen3.pixelIndex(j,i);
            a[j/8]|=(color_pixel&0x01)<<(7-j%8);
            b[j/8]|=((color_pixel>>1)&0x01)<<(7-j%8);
            c[j/8]|=((color_pixel>>2)&0x01)<<(7-j%8);
            d[j/8]|=((color_pixel>>3)&0x01)<<(7-j%8);


        }
        //se ha recorrido toda la fila, guardar en qbytearray para posterior tratamiento
        for(j=0;j<ancho/8;j++)
        {
            data.append(a[j]);
        }
        for(j=0;j<ancho/8;j++)
        {
            data.append(b[j]);
        }
        for(j=0;j<ancho/8;j++)
        {
            data.append(c[j]);
        }
        for(j=0;j<ancho/8;j++)
        {
            data.append(d[j]);
        }
    }

    data.append(0xFF);
    //se ha recorrido todos los píxeles de la imagen y ya se tiene la imagen codificada
    return data;
}
