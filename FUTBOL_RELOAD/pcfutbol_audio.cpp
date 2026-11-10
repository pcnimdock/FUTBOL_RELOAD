#include "pcfutbol_audio.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QtMath>
#include <cmath>
#include <cstring>

#include "thirdparty/dr_wav.h"
#include "thirdparty/dr_mp3.h"
#include "thirdparty/dr_flac.h"

extern "C" int stb_vorbis_decode_memory(const unsigned char *mem, int len, int *channels,
                                        int *sample_rate, short **output);

namespace PCFutbolAudio
{

QList<SonidoJuego> sonidosDelJuego()
{
    // En el orden en que los carga PCMANAG.DAT
    return {
        {"ALIENTO",  "Ánimos del público"},
        {"BALONAZO", "Balonazo (golpe fuerte al balón)"},
        {"CANTICO2", "Cántico del público (el juego usa CANTICO2/3 o CANTICO4/5 según el partido)"},
        {"CANTICO3", "Cántico del público (pareja de CANTICO2)"},
        {"CANTICO4", "Cántico del público (alternativa a CANTICO2)"},
        {"CANTICO5", "Cántico del público (pareja de CANTICO4)"},
        {"CHUT",     "Chut"},
        {"ENTRADA1", "Entrada a un jugador"},
        {"EXCLAMA0", "Exclamación del público"},
        {"EXCLAMA1", "Exclamación del público"},
        {"EXCLAMA2", "Exclamación del público"},
        {"EXCLAMA3", "Exclamación del público"},
        {"EXCLAMA4", "Exclamación del público"},
        {"EXCLAMA5", "Exclamación del público"},
        {"EXCLAMA6", "Exclamación del público"},
        {"EXCLAMA7", "Exclamación del público"},
        {"FONDO",    "Ambiente de fondo del estadio"},
        {"GOL",      "Celebración de gol"},
        {"POSTE",    "Balón al poste"},
        {"VALLA",    "Balón contra la valla publicitaria"},
        {"SILBATO",  "Silbato del árbitro"},
        {"SILBATOF", "Silbato del final"},
        {"BOTE",     "Bote del balón"},
        {"SILBIDOS", "Pitada del público"},
        {"UII",      "«¡Uyyy!» del público (ocasión fallada)"},
    };
}

QString descripcion(const QString &nombre)
{
    for (const SonidoJuego &s : sonidosDelJuego())
        if (s.nombre.compare(nombre, Qt::CaseInsensitive) == 0) return s.descripcion;
    return QString();
}

QString carpetaSonidos(const QString &carpetaJuego)
{
    QDir d(carpetaJuego);
    if (d.dirName().compare("SONIDOS", Qt::CaseInsensitive) == 0) return d.absolutePath();
    for (const QString &s : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (s.compare("SONIDOS", Qt::CaseInsensitive) == 0) return d.absoluteFilePath(s);
    return QString();
}

QString rutaRaw(const QString &carpeta, const QString &nombre)
{
    QDir d(carpeta);
    for (const QString &f : d.entryList(QDir::Files))
        if (f.compare(nombre + ".RAW", Qt::CaseInsensitive) == 0) return d.absoluteFilePath(f);
    return d.absoluteFilePath(nombre.toUpper() + ".RAW");
}

// ---------------------------------------------------------------------------
// Decodificar
// ---------------------------------------------------------------------------
QString filtroAbrir()
{
    return "Audio (*.wav *.wave *.mp3 *.ogg *.oga *.flac *.aif *.aiff *.aifc *.raw *.m4a *.aac *.wma *.opus);;"
           "Todos los ficheros (*)";
}

static quint32 u32le(const QByteArray &d, int o)
{
    return quint8(d[o]) | (quint8(d[o + 1]) << 8) | (quint8(d[o + 2]) << 16) | (quint32(quint8(d[o + 3])) << 24);
}
static quint16 u16le(const QByteArray &d, int o) { return quint8(d[o]) | (quint8(d[o + 1]) << 8); }

// WAV que ya es exactamente lo que guarda el juego (8 bits, mono, 11.025 Hz)
static QByteArray wavDelJuego(const QByteArray &w)
{
    if (w.size() < 44 || w.left(4) != "RIFF" || w.mid(8, 4) != "WAVE") return QByteArray();
    int formato = 0, canales = 0, bits = 0;
    quint32 frecuencia = 0;
    QByteArray datos;
    bool hayDatos = false;
    int pos = 12;
    while (pos + 8 <= w.size())
    {
        const QByteArray id = w.mid(pos, 4);
        const quint32 tam = u32le(w, pos + 4);
        if (id == "fmt " && pos + 8 + 16 <= w.size())
        {
            formato = u16le(w, pos + 8);
            canales = u16le(w, pos + 10);
            frecuencia = u32le(w, pos + 12);
            bits = u16le(w, pos + 22);
        }
        else if (id == "data")
        {
            datos = w.mid(pos + 8, int(qMin<qint64>(tam, w.size() - pos - 8)));
            hayDatos = true;
        }
        pos += 8 + int(tam) + int(tam % 2);
        if (tam > quint32(w.size())) break;
    }
    if (formato == 1 && canales == 1 && bits == 8 && frecuencia == FRECUENCIA && hayDatos)
        return datos;
    return QByteArray();
}

// Más que esto no tiene sentido para el juego (10 min = 6,6 MB de .RAW) y un
// MP3 de una hora en memoria ocuparía cientos de MB.
static const int SEGUNDOS_MAXIMOS = 600;

static void aMono(const float *p, quint64 frames, int canales, Audio *a)
{
    a->muestras.resize(int(frames));
    for (quint64 i = 0; i < frames; i++)
    {
        float s = 0;
        for (int c = 0; c < canales; c++) s += p[i * canales + c];
        a->muestras[int(i)] = s / canales;
    }
}

bool decodificarDatos(const QByteArray &datos, Audio *a, QString *error)
{
    *a = Audio();
    if (datos.size() < 4)
    {
        if (error) *error = "el fichero está vacío";
        return false;
    }
    const QByteArray cab = datos.left(4);
    const unsigned char b0 = quint8(datos[0]), b1 = quint8(datos[1]);
    const size_t n = size_t(datos.size());
    unsigned int canales = 0, frecuencia = 0;

    if (cab == "RIFF" || cab == "RIFX" || cab == "RF64" || cab == "FORM" || cab == "riff")
    {
        a->rawExacto = wavDelJuego(datos);
        drwav w;
        if (!drwav_init_memory(&w, datos.constData(), n, nullptr))
        {
            if (error) *error = "no se reconoce el WAV/AIFF (¿está dañado o es un formato raro?)";
            return false;
        }
        canales = w.channels;
        frecuencia = w.sampleRate;
        const quint64 frames = qMin<quint64>(w.totalPCMFrameCount, quint64(w.sampleRate) * SEGUNDOS_MAXIMOS);
        const bool wavRecortado = frames < w.totalPCMFrameCount;
        a->formato = QString("%1 %2 bits").arg(cab == "FORM" ? "AIFF" : "WAV").arg(w.bitsPerSample);
        if (w.translatedFormatTag == DR_WAVE_FORMAT_IEEE_FLOAT) a->formato += " flotante";
        else if (w.translatedFormatTag == DR_WAVE_FORMAT_ADPCM || w.translatedFormatTag == DR_WAVE_FORMAT_DVI_ADPCM) a->formato = "WAV ADPCM";
        else if (w.translatedFormatTag == DR_WAVE_FORMAT_ALAW) a->formato = "WAV A-law";
        else if (w.translatedFormatTag == DR_WAVE_FORMAT_MULAW) a->formato = "WAV µ-law";
        if (canales < 1 || frames == 0 || frecuencia == 0)
        {
            drwav_uninit(&w);
            if (error) *error = "el WAV no tiene muestras";
            return false;
        }
        QVector<float> tmp(int(frames * canales));
        const quint64 leidos = drwav_read_pcm_frames_f32(&w, frames, tmp.data());
        drwav_uninit(&w);
        if (leidos == 0)
        {
            if (error) *error = "no se pudieron leer las muestras (formato de WAV no admitido)";
            return false;
        }
        aMono(tmp.constData(), leidos, int(canales), a);
        if (wavRecortado) a->formato += " (sólo los primeros 10 min)";
    }
    else if (cab == "fLaC")
    {
        drflac *fl = drflac_open_memory(datos.constData(), n, nullptr);
        if (!fl)
        {
            if (error) *error = "FLAC dañado o no admitido";
            return false;
        }
        canales = fl->channels;
        frecuencia = fl->sampleRate;
        const quint64 tope = qMin<quint64>(fl->totalPCMFrameCount ? fl->totalPCMFrameCount : quint64(-1),
                                           quint64(frecuencia) * SEGUNDOS_MAXIMOS);
        QVector<float> tmp;
        QVector<float> bloque(4096 * qMax(1u, canales));
        quint64 leidos = 0;
        while (leidos < tope)
        {
            const quint64 r = drflac_read_pcm_frames_f32(fl, qMin<quint64>(4096, tope - leidos), bloque.data());
            if (r == 0) break;
            { const int o = tmp.size(); tmp.resize(o + int(r * canales)); memcpy(tmp.data() + o, bloque.constData(), size_t(r * canales) * sizeof(float)); }
            leidos += r;
        }
        if (fl->totalPCMFrameCount > leidos) a->formato = "recortado";
        drflac_close(fl);
        if (leidos == 0 || canales < 1)
        {
            if (error) *error = "FLAC sin muestras";
            return false;
        }
        aMono(tmp.constData(), leidos, int(canales), a);
        a->formato = a->formato == "recortado" ? "FLAC (sólo los primeros 10 min)" : "FLAC";
    }
    else if (cab == "OggS")
    {
        if (datos.indexOf("OpusHead") >= 0 && datos.indexOf("OpusHead") < 200)
        {
            if (error) *error = "es Opus, no Vorbis (pásalo por ffmpeg o guárdalo como WAV)";
            return false;
        }
        int ch = 0, fr = 0;
        short *p = nullptr;
        const int frames = stb_vorbis_decode_memory(reinterpret_cast<const unsigned char *>(datos.constData()),
                                                    datos.size(), &ch, &fr, &p);
        if (frames <= 0 || !p || ch < 1)
        {
            free(p);
            if (error) *error = "OGG dañado o no admitido";
            return false;
        }
        a->muestras.resize(frames);
        for (int i = 0; i < frames; i++)
        {
            int s = 0;
            for (int c = 0; c < ch; c++) s += p[i * ch + c];
            a->muestras[i] = float(s) / (32768.0f * ch);
        }
        free(p);
        canales = unsigned(ch);
        frecuencia = unsigned(fr);
        a->formato = "OGG Vorbis";
    }
    else if (cab.startsWith("ID3") || (b0 == 0xFF && (b1 & 0xE0) == 0xE0))
    {
        drmp3 mp;
        if (!drmp3_init_memory(&mp, datos.constData(), n, nullptr))
        {
            if (error) *error = "MP3 dañado o no admitido";
            return false;
        }
        canales = mp.channels;
        frecuencia = mp.sampleRate;
        const quint64 tope = quint64(frecuencia) * SEGUNDOS_MAXIMOS;
        QVector<float> tmp;
        QVector<float> bloque(4096 * qMax(1u, canales));
        quint64 leidos = 0;
        bool recortado = false;
        while (true)
        {
            if (leidos >= tope) { recortado = true; break; }
            const quint64 r = drmp3_read_pcm_frames_f32(&mp, qMin<quint64>(4096, tope - leidos), bloque.data());
            if (r == 0) break;
            { const int o = tmp.size(); tmp.resize(o + int(r * canales)); memcpy(tmp.data() + o, bloque.constData(), size_t(r * canales) * sizeof(float)); }
            leidos += r;
        }
        drmp3_uninit(&mp);
        if (leidos == 0 || canales < 1)
        {
            if (error) *error = "MP3 sin sonido";
            return false;
        }
        aMono(tmp.constData(), leidos, int(canales), a);
        a->formato = recortado ? "MP3 (sólo los primeros 10 min)" : "MP3";
    }
    else
    {
        if (error) *error = "formato no reconocido";
        return false;
    }

    a->canales = int(canales);
    a->frecuencia = int(frecuencia);
    if (a->frecuencia < 1000 || a->frecuencia > 384000)
    {
        if (error) *error = QString("frecuencia extraña (%1 Hz)").arg(a->frecuencia);
        return false;
    }
    return true;
}

static QString buscarFfmpeg()
{
    // en el PATH o junto al editor (Windows: ffmpeg.exe al lado del .exe)
    QString f = QStandardPaths::findExecutable("ffmpeg");
    if (f.isEmpty())
        f = QStandardPaths::findExecutable("ffmpeg", {QCoreApplication::applicationDirPath()});
    return f;
}

bool decodificar(const QString &ruta, Audio *a, QString *error)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error) *error = "no se puede abrir el fichero";
        return false;
    }
    const QByteArray datos = f.readAll();
    f.close();

    // Un .RAW: se toma como un sonido del juego (8 bits sin signo, 11.025 Hz)
    if (QFileInfo(ruta).suffix().compare("raw", Qt::CaseInsensitive) == 0)
    {
        *a = Audio();
        a->rawExacto = datos;
        a->frecuencia = FRECUENCIA;
        a->canales = 1;
        a->formato = "RAW del juego";
        a->muestras.resize(datos.size());
        for (int i = 0; i < datos.size(); i++) a->muestras[i] = (int(quint8(datos[i])) - 128) / 128.0f;
        return true;
    }

    QString err;
    if (decodificarDatos(datos, a, &err)) return true;

    // Otros formatos (M4A, WMA, Opus…): con ffmpeg si lo hay
    const QString ff = buscarFfmpeg();
    if (!ff.isEmpty())
    {
        // Se pide a 44.100 Hz para no tener que averiguar la frecuencia original
        QProcess q;
        q.start(ff, {"-v", "error", "-i", ruta, "-vn", "-ac", "1", "-ar", "44100", "-f", "f32le", "-acodec", "pcm_f32le", "-"});
        if (q.waitForStarted(5000) && q.waitForFinished(120000) && q.exitCode() == 0)
        {
            const QByteArray s = q.readAllStandardOutput();
            const int frames = s.size() / 4;
            if (frames > 0)
            {
                *a = Audio();
                a->muestras.resize(frames);
                memcpy(a->muestras.data(), s.constData(), size_t(frames) * 4);
                a->frecuencia = 44100;
                a->canales = 1;
                a->formato = QFileInfo(ruta).suffix().toUpper() + " (vía ffmpeg)";
                return true;
            }
        }
        if (error) *error = err + "; ffmpeg tampoco ha podido leerlo";
        return false;
    }
    if (error)
        *error = err + ". Guárdalo como WAV, MP3, OGG o FLAC (o instala ffmpeg para leer M4A, WMA, Opus…)";
    return false;
}

// ---------------------------------------------------------------------------
// Convertir
// ---------------------------------------------------------------------------

// Remuestreo con filtro paso bajo (sinc con ventana de Blackman): antes de
// bajar a 11.025 Hz hay que quitar lo que pase de ~5 kHz o se oye como
// pitidos metálicos (aliasing), que es lo que pasa con una interpolación
// lineal desde 44.100 Hz.
static QVector<float> remuestrear(const QVector<float> &x, int fe, int fs)
{
    if (fe == fs) return x;
    const int n = x.size();
    const double razon = double(fe) / fs;                      // muestras de entrada por cada de salida
    const double fc = 0.45 * qMin(fe, fs);                     // Hz
    const double w = 2.0 * fc / fe;                            // frecuencia de corte normalizada (entrada)
    const double mitad = 8.0 * fe / fc;                        // 16 pasos por cero a cada lado
    const int M = int(std::ceil(mitad));
    const int FASES = 512;
    // Tabla del núcleo h(d), d en [0, M] con FASES subdivisiones por muestra
    QVector<float> tabla((M + 1) * FASES + 1);
    for (int k = 0; k < tabla.size(); k++)
    {
        const double d = double(k) / FASES;
        double s = (d == 0) ? 1.0 : std::sin(M_PI * w * d) / (M_PI * w * d);
        const double v = d / mitad;
        const double ven = v >= 1.0 ? 0.0 : 0.42 + 0.5 * std::cos(M_PI * v) + 0.08 * std::cos(2 * M_PI * v);
        tabla[k] = float(s * ven);
    }
    const qint64 nOut = qint64(std::floor(n / razon));
    QVector<float> y(int(qMax<qint64>(nOut, 0)));
    for (int i = 0; i < y.size(); i++)
    {
        const double t = i * razon;
        const int c = int(std::floor(t));
        double suma = 0, peso = 0;
        for (int j = c - M; j <= c + M + 1; j++)
        {
            const double d = std::fabs(t - j);
            if (d > mitad) continue;
            const double pos = d * FASES;
            const int k = int(pos);
            const float h = tabla[k] + float(pos - k) * (tabla[k + 1] - tabla[k]);
            peso += h;
            if (j >= 0 && j < n) suma += h * x[j];
        }
        y[i] = peso != 0 ? float(suma / peso) : 0.0f;
    }
    return y;
}

static double rmsF(const QVector<float> &x)
{
    if (x.isEmpty()) return 0;
    double s = 0;
    for (float v : x) s += double(v) * v;
    return std::sqrt(s / x.size());
}

static float picoF(const QVector<float> &x)
{
    float p = 0;
    for (float v : x) p = qMax(p, std::fabs(v));
    return p;
}

// Limitador suave: no toca hasta 0,7; de ahí a 1 se curva sin pasarse nunca.
static inline float limitar(float v, bool *toco)
{
    const float a = std::fabs(v);
    if (a <= 0.7f) return v;
    *toco = a > 1.0f;
    const float r = 0.7f + 0.3f * std::tanh((a - 0.7f) / 0.3f);
    return v < 0 ? -r : r;
}

QByteArray convertir(const Audio &a, const Opciones &op, Informe *inf)
{
    Informe local;
    Informe &r = inf ? *inf : local;
    r = Informe();
    r.duracionEntrada = a.frecuencia > 0 ? double(a.muestras.size()) / a.frecuencia : 0;

    // Ya está en el formato del juego y no se pide nada: se copia tal cual
    const bool sinCambios = op.volumen == VolumenSinCambio && op.ajusteDb == 0 &&
                            !op.recortarSilencio && op.duracionMaxima <= 0 && !op.fundidos;
    if (!a.rawExacto.isEmpty() && (op.copiarSiYaEsDelJuego || sinCambios))
    {
        QByteArray raw = a.rawExacto;
        if (raw.size() < 4) raw.append(QByteArray(4 - raw.size(), char(0x80)));
        r.copiaExacta = true;
        r.duracionSalida = duracion(raw);
        return raw;
    }

    // 1) a 11.025 Hz (si hay duración máxima, no se procesa lo que va a sobrar;
    //    se dejan 30 s de más por si hay que recortar silencio al principio)
    QVector<float> x;
    const qint64 util = op.duracionMaxima > 0 ? qint64((op.duracionMaxima + 30) * a.frecuencia) : -1;
    if (util > 0 && util < a.muestras.size())
        x = remuestrear(a.muestras.mid(0, int(util)), a.frecuencia, FRECUENCIA);
    else
        x = remuestrear(a.muestras, a.frecuencia, FRECUENCIA);

    // 2) quitar continua (algunas grabaciones vienen descentradas)
    if (!x.isEmpty())
    {
        double m = 0;
        for (float v : x) m += v;
        m /= x.size();
        if (std::fabs(m) > 0.002)
            for (float &v : x) v -= float(m);
    }

    // 3) recortar silencio (umbral -45 dB, dejando 15 ms de margen)
    if (op.recortarSilencio && !x.isEmpty())
    {
        const float umbral = qMax(0.0056f, picoF(x) * 0.01f);
        int ini = 0, fin = x.size() - 1;
        while (ini < x.size() && std::fabs(x[ini]) < umbral) ini++;
        if (ini >= x.size()) x.clear();                      // todo es silencio
        while (fin > ini && std::fabs(x[fin]) < umbral) fin--;
        const int margen = FRECUENCIA * 15 / 1000;
        ini = qMax(0, ini - margen);
        fin = qMin(x.size() - 1, fin + margen);
        if (x.isEmpty() || ini > fin) x.clear();
        else if (ini > 0 || fin < x.size() - 1) x = x.mid(ini, fin - ini + 1);
    }

    // 4) duración máxima
    bool cortado = false;
    if (op.duracionMaxima > 0 && x.size() > op.duracionMaxima * FRECUENCIA)
    {
        x.resize(int(op.duracionMaxima * FRECUENCIA));
        cortado = true;
    }

    if (x.isEmpty())
    {
        r.avisos << "El sonido está en silencio: se guarda un silencio mínimo.";
        return QByteArray(4, char(0x80));
    }

    // 5) volumen
    const double rmsX = rmsF(x);
    const float pico = picoF(x);
    double g = 1.0;
    if (op.volumen == VolumenComoOriginal && op.rmsReferencia > 0 && rmsX > 1e-6)
    {
        g = (op.rmsReferencia / 128.0) / rmsX;
        // sin pasar de +6 dB por encima del máximo limpio (el limitador lo recoge)
        const double tope = 2.0 * 0.98 / qMax(pico, 1e-6f);
        if (g > tope)
        {
            g = tope;
            r.avisos << "Es más flojo que el original: se ha subido todo lo posible sin que distorsione.";
        }
    }
    else if (op.volumen == VolumenNormalizar || (op.volumen == VolumenComoOriginal && op.rmsReferencia <= 0))
    {
        g = pico > 1e-6f ? 0.98 / pico : 1.0;
    }
    g *= std::pow(10.0, op.ajusteDb / 20.0);
    r.gananciaDb = 20.0 * std::log10(qMax(g, 1e-9));

    // 6) fundidos: 5 ms de entrada, 10 ms de salida (50 ms si se ha cortado)
    const int fIn = op.fundidos ? qMin(x.size() / 4, FRECUENCIA * 5 / 1000) : 0;
    const int fOut = (op.fundidos || cortado) ? qMin(x.size() / 4, FRECUENCIA * (cortado ? 50 : 10) / 1000) : 0;

    // 7) a 8 bits con un poco de ruido de redondeo (dither) para que las
    //    colas suaves no suenen a «rasposo». Semilla fija: misma entrada,
    //    mismo resultado.
    quint32 semilla = 0x12345678u;
    auto aleatorio = [&semilla]() {
        semilla = semilla * 1664525u + 1013904223u;
        return float(semilla >> 8) / float(1 << 24);          // 0..1
    };
    QByteArray raw(x.size(), char(0x80));
    for (int i = 0; i < x.size(); i++)
    {
        float v = float(x[i] * g);
        if (i < fIn) v *= float(i) / fIn;
        if (fOut > 0 && i >= x.size() - fOut) v *= float(x.size() - 1 - i) / fOut;
        bool toco = false;
        v = limitar(v, &toco);
        if (toco) r.saturadas++;
        const float d = aleatorio() - aleatorio();             // triangular, ±1 escalón
        const int q = qBound(0, int(std::lround(v * 127.0f + d)) + 128, 255);
        raw[i] = char(q);
    }
    // El juego sólo usa múltiplos de 4 bytes: se rellena con silencio
    while (raw.size() % 4) raw.append(char(0x80));

    if (cortado)
        r.avisos << QString("Se ha cortado a %1 s.").arg(op.duracionMaxima, 0, 'f', 1);
    if (r.saturadas > raw.size() / 200)
        r.avisos << "Partes muy fuertes: se han suavizado los picos para que no distorsione.";
    r.duracionSalida = duracion(raw);
    return raw;
}

double rms(const QByteArray &raw)
{
    if (raw.isEmpty()) return 0;
    double s = 0;
    for (char c : raw)
    {
        const int v = int(quint8(c)) - 128;
        s += v * v;
    }
    return std::sqrt(s / raw.size());
}

double duracion(const QByteArray &raw) { return double(raw.size()) / FRECUENCIA; }

QByteArray leerOriginal(const QString &ruta)
{
    for (const QString &p : {ruta + ".bak", ruta})
    {
        QFile f(p);
        if (f.open(QIODevice::ReadOnly)) return f.readAll();
    }
    return QByteArray();
}

bool escribirRaw(const QString &ruta, const QByteArray &rawIn, QString *error)
{
    QByteArray raw = rawIn;
    // Menos de 4 bytes no suena. No se rellena a múltiplo de 4: el juego ya
    // ignora lo que sobre, y así un sonido sin tocar queda idéntico.
    while (raw.size() < 4) raw.append(char(0x80));
    if (QFile::exists(ruta))
    {
        QFile f(ruta);
        if (f.open(QIODevice::ReadOnly) && f.readAll() == raw) return true;     // ya es igual
        f.close();
        if (!QFile::exists(ruta + ".bak") && !QFile::copy(ruta, ruta + ".bak"))
        {
            if (error) *error = "no se pudo crear la copia " + QFileInfo(ruta).fileName() + ".bak";
            return false;
        }
    }
    QFile o(ruta);
    if (!o.open(QIODevice::WriteOnly) || o.write(raw) != raw.size())
    {
        if (error) *error = "no se pudo escribir " + QFileInfo(ruta).fileName();
        return false;
    }
    return true;
}

Estado comprobar(const QString &carpeta)
{
    Estado e;
    for (const SonidoJuego &s : sonidosDelJuego())
    {
        const QString ruta = rutaRaw(carpeta, s.nombre);
        QFileInfo fi(ruta);
        if (!fi.exists())
        {
            e.faltan << s.nombre + ".RAW";
            if (QFile::exists(ruta + ".bak")) e.avisos << s.nombre + ".RAW falta pero hay copia .bak";
            continue;
        }
        if (fi.size() < 4) e.vacios << s.nombre + ".RAW";
        e.total += fi.size();
        const QFileInfo bak(ruta + ".bak");
        e.totalOriginal += bak.exists() ? bak.size() : fi.size();
    }
    if (!e.faltan.isEmpty())
        e.avisos.prepend("Faltan " + e.faltan.join(", ") +
                         ": el partido se para con «No se encontro el fichero».");
    if (!e.vacios.isEmpty())
        e.avisos << "Demasiado cortos para sonar (menos de 4 bytes): " + e.vacios.join(", ");
    if (e.total > TOTAL_LIMITE)
        e.avisos << QString("Los sonidos ocupan %1 KB: casi seguro que NO caben. El juego los carga todos en "
                            "memoria al empezar el partido; con 14 MB se para con «No se pudo alocar la "
                            "memoria para el sonido». Acorta los más largos.").arg(e.total / 1024);
    else if (e.total > TOTAL_AVISO)
        e.avisos << QString("Los sonidos ocupan %1 KB (los originales, %2 KB). Está probado hasta 7,3 MB; "
                            "con más puede faltar memoria en algún modo de juego.")
                        .arg(e.total / 1024).arg(TOTAL_ORIGINAL / 1024);
    return e;
}

} // namespace PCFutbolAudio
