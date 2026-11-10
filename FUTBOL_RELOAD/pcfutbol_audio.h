#ifndef PCFUTBOL_AUDIO_H
#define PCFUTBOL_AUDIO_H

/*
 * pcfutbol_audio.h / .cpp
 *
 * Conversor de sonidos para los partidos (SONIDOS/*.RAW de PC Fútbol 4.0 y 4.5).
 *
 * Lo que el juego exige (sacado de PCMANAG.DAT, que es quien los usa):
 *   - Carga los 25 .RAW al empezar un partido con efectos activados, cada uno
 *     entero en memoria. Si falta uno, se para con «No se encontro el fichero
 *     sonidos\xxx.raw»; si no hay memoria, con «No se pudo alocar la memoria
 *     para el sonido». No hay otro límite de tamaño: la mezcla es por
 *     software y lee las muestras directamente de ese búfer.
 *   - Del tamaño del fichero sólo usa múltiplos de 4 bytes (redondea hacia
 *     abajo), así que un fichero de menos de 4 bytes no suena.
 *   - Formato: PCM de 8 bits sin signo, mono, 11.025 Hz, sin cabecera. Lo que
 *     se meta tal cual en otro formato (un WAV renombrado, un MP3, 16 bits…)
 *     suena a ruido y a velocidad equivocada.
 *   - Los ambientes (FONDO, CANTICO2…) están grabados muy bajos (RMS 6-14 de
 *     127) y los golpes muy altos: se mezclan varios a la vez. Un ambiente
 *     nuevo al volumen normal de una canción tapa todo lo demás. Por eso, por
 *     defecto, se iguala el volumen al del sonido original.
 *
 * Lee WAV (8/16/24/32 bits, flotante, ADPCM, A-law, µ-law, WAVE_EXTENSIBLE),
 * AIFF, MP3, OGG Vorbis y FLAC con decodificadores de dominio público
 * incluidos (thirdparty/). Para cualquier otro formato (M4A, WMA…) usa
 * ffmpeg si está instalado.
 */

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

namespace PCFutbolAudio
{
    const int FRECUENCIA = 11025;

    struct SonidoJuego
    {
        QString nombre;        // sin extensión, en mayúsculas (GOL)
        QString descripcion;
    };
    // Los 25 que carga el juego, en su orden
    QList<SonidoJuego> sonidosDelJuego();
    QString descripcion(const QString &nombre);

    // Carpeta SONIDOS dentro de la del juego (sin distinguir mayúsculas). Vacía si no hay.
    QString carpetaSonidos(const QString &carpetaJuego);
    // Ruta del .RAW de un sonido dentro de 'carpetaSonidos' (exista o no)
    QString rutaRaw(const QString &carpetaSonidos, const QString &nombre);

    struct Audio
    {
        QVector<float> muestras;   // mono, -1..1
        int frecuencia = 0;
        int canales = 0;
        QString formato;           // para el informe («MP3», «WAV 16 bits»…)
        // Si ya viene en el formato del juego (WAV 8 bits mono 11.025 Hz),
        // las muestras exactas: se copian sin tocar.
        QByteArray rawExacto;
    };
    // Extensiones que se pueden abrir (para el diálogo de abrir)
    QString filtroAbrir();
    bool decodificar(const QString &ruta, Audio *a, QString *error);
    bool decodificarDatos(const QByteArray &datos, Audio *a, QString *error);

    enum Volumen
    {
        VolumenComoOriginal,   // mismo volumen medio que el sonido al que sustituye
        VolumenNormalizar,     // lo más alto posible sin saturar
        VolumenSinCambio       // tal cual venga
    };
    struct Opciones
    {
        Volumen volumen = VolumenComoOriginal;
        double rmsReferencia = 0;      // RMS del original (en escala de 8 bits); 0 = normalizar
        double ajusteDb = 0;           // ajuste fino sobre lo anterior
        bool recortarSilencio = true;  // quitar silencio al principio y al final
        double duracionMaxima = 0;     // segundos; 0 = sin límite
        bool fundidos = true;          // entrada/salida suaves (evita chasquidos)
        bool copiarSiYaEsDelJuego = true;  // WAV ya en formato del juego: sin tocar
    };
    struct Informe
    {
        double duracionEntrada = 0, duracionSalida = 0;
        double gananciaDb = 0;
        int saturadas = 0;             // muestras que han tenido que limitarse
        bool copiaExacta = false;
        QStringList avisos;
    };
    // Devuelve el .RAW listo (nunca vacío: como mínimo 4 bytes de silencio)
    QByteArray convertir(const Audio &a, const Opciones &op, Informe *inf = nullptr);

    // Volumen medio (RMS) de un .RAW, en escala de 8 bits (0..128)
    double rms(const QByteArray &raw);
    double duracion(const QByteArray &raw);

    // El original de un sonido: el .bak si existe, si no el .RAW actual
    QByteArray leerOriginal(const QString &rutaRaw);

    // Escribe el .RAW (con copia .bak la primera vez). Rellena a múltiplo de 4.
    bool escribirRaw(const QString &rutaRaw, const QByteArray &raw, QString *error);

    // Comprobación de la carpeta SONIDOS: faltan ficheros, demasiado cortos…
    struct Estado
    {
        QStringList faltan;
        QStringList vacios;            // < 4 bytes: no suenan
        qint64 total = 0, totalOriginal = 0;
        QStringList avisos;
    };
    Estado comprobar(const QString &carpetaSonidos);
    // Total de los 25 originales (844.259 bytes)
    const qint64 TOTAL_ORIGINAL = 844259;
    // Probado en un partido amistoso con DOSBox a 16 MB (lo que trae el juego):
    //   7,3 MB  -> el partido carga y suenan bien;
    //  13,3 MB  -> se cargan; 14,0 MB -> «No se pudo alocar la memoria para
    //              el sonido sonidos\gol.raw» y el juego se para.
    // Otros modos (Liga Manager…) pueden necesitar más memoria para lo demás.
    const qint64 TOTAL_AVISO = 8 * 1024 * 1024;     // más de lo probado
    const qint64 TOTAL_LIMITE = 12 * 1024 * 1024;   // casi seguro que no cabe
}

#endif // PCFUTBOL_AUDIO_H
