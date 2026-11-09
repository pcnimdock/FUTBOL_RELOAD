#include "pcfutbol_equipacion.h"
#include "paleta_kits.h"
#include <QFile>
#include <QIODevice>
#include <QColor>
#include <algorithm>
#include <climits>

// ---------------------------------------------------------------------------
// MAPA REAL DE PÍXELES DEL SPRITE DE CAMISETA (20 x 20)
//
// Calibrado empíricamente contra el juego real: se generaron EQUIPOS.DAT de
// prueba donde cada una de las 168 posiciones de la textura llevaba un valor
// conocido (test A: valor = i; test B: valor = 120 + (i % 128)), se capturó el
// icono en el juego y se cruzaron ambas capturas contra PALETA.BBM. El cruce
// resuelve los 281 píxeles opacos del sprite sin ambigüedad.
//
// Cada celda = posición (0-167) de la textura que el juego pinta en ese píxel.
// -1 = píxel de fondo (transparente).
// ---------------------------------------------------------------------------
namespace {

const qint16 mapaSprite[PCFutbolEquipacion::SPRITE_W * PCFutbolEquipacion::SPRITE_H] = {
      -1,   -1,   -1,   28,   28,   16,   -1,   -1,   -1,   -1,   -1,   -1,   16,   30,   30,   -1,   -1,   -1,   -1,   -1,   // fila 0
      -1,   -1,   28,    4,    4,   16,    7,   -1,   -1,   -1,   -1,   -1,   16,   25,   25,   30,    7,   -1,   -1,   -1,   // fila 1
      -1,   28,    4,    4,    4,   16,   16,    7,   -1,   -1,   -1,   16,   16,   25,   25,   25,   30,    7,   -1,   -1,   // fila 2
      28,    4,    4,    4,    4,   72,   16,   16,    7,    7,   16,   16,   77,   25,   25,   25,   25,   30,    7,   -1,   // fila 3
      28,    4,    4,    4,    4,   72,   72,   16,   16,   16,   16,   77,   77,   25,   25,   25,   25,   30,    7,    7,   // fila 4
      31,    4,    4,  109,  109,   97,   97,  112,  112,  117,  117,  102,  102,  107,  107,   25,   25,   32,    7,    7,   // fila 5
      31,   27,    4,  109,  109,   97,   97,  112,  112,  117,  117,  102,  102,  107,  107,   25,   29,   32,    7,    7,   // fila 6
      -1,   -1,   27,  109,  109,   97,   97,  112,  112,  117,  117,  102,  102,  107,  107,   29,    7,    7,    7,    7,   // fila 7
      -1,   -1,    7,  134,  134,  120,  120,  137,  137,  141,  141,  126,  126,  131,  131,    7,    7,    7,    7,    7,   // fila 8
      -1,   -1,   -1,  134,  134,  120,  120,  137,  137,  141,  141,  126,  126,  131,  131,    7,    7,    7,   -1,   -1,   // fila 9
      -1,   -1,   -1,  158,  158,  145,  145,  160,  160,  165,  165,  150,  150,  155,  155,    7,    7,   -1,   -1,   -1,   // fila 10
      -1,   -1,   -1,  158,  158,  145,  145,  160,  160,  165,  165,  150,  150,  155,  155,    7,    7,   -1,   -1,   -1,   // fila 11
      -1,   -1,   -1,  158,  158,  145,  145,  160,  160,  165,  165,  150,  150,  155,  155,    7,    7,   -1,   -1,   -1,   // fila 12
      -1,   -1,   -1,   -1,   43,    1,    1,   47,   47,   50,   50,   34,   34,   52,    7,    7,    7,   -1,   -1,   -1,   // fila 13
      -1,   -1,   -1,   -1,   43,    1,    1,   47,   47,   50,   50,   34,   34,   52,    7,    7,    7,   -1,   -1,   -1,   // fila 14
      -1,   -1,   -1,   -1,   -1,    1,    1,   47,   47,   50,   50,   34,   34,    7,    7,    7,    7,   -1,   -1,   -1,   // fila 15
      -1,   -1,   -1,   -1,   -1,   15,   15,   15,   15,   15,   15,   15,   15,    7,    7,    7,    7,   -1,   -1,   -1,   // fila 16
      -1,   -1,   -1,   -1,   -1,   15,   15,   15,   15,   15,   15,   15,   15,    7,    7,    7,   -1,   -1,   -1,   -1,   // fila 17
      -1,   -1,   -1,   -1,   -1,   -1,    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,   -1,   -1,   -1,   -1,   // fila 18
      -1,   -1,   -1,   -1,   -1,   -1,   -1,    7,    7,    7,    7,    7,    7,    7,    7,   -1,   -1,   -1,   -1,   -1,   // fila 19
};

// Zonas anatómicas (posiciones de textura), deducidas del mapa + los 470 kits reales
const int ZONA_CUELLO[]  = { 16 };
const int ZONA_RIBETE[]  = { 15 };
const int ZONA_MANGA_I[] = { 4, 27, 28, 31 };
const int ZONA_MANGA_D[] = { 25, 29, 30, 32 };
const int ZONA_HOMBROS[] = { 72, 77 };
const int ZONA_TORSO[]   = { 97, 102, 107, 109, 112, 117,
                             120, 126, 131, 134, 137, 141,
                             145, 150, 155, 158, 160, 165 };
const int ZONA_FALDON[]  = { 1, 34, 43, 47, 50, 52 };

// Zonas del JUGADOR en el partido (ver pcfutbol_equipacion.h)
const int ZONA_BOTAS[]    = { 7, 22, 23 };
const int ZONA_PELO[]     = { 14 };
const int ZONA_MEDIAS[]   = { 18, 19, 20 };
const int ZONA_PANTALON[] = { 9, 10, 11, 12, 13, 15,
                              63, 64, 65, 66, 67, 68, 69, 70, 71 };
const int ZONA_FIJOS[]    = { 8, 17, 21 };

template <int N>
bool contiene(const int (&arr)[N], int v)
{
    for (int i = 0; i < N; i++)
    {
        if (arr[i] == v) return true;
    }
    return false;
}

int luminancia(quint8 idx)
{
    return paleta_kits[idx][0] * 299 + paleta_kits[idx][1] * 587 + paleta_kits[idx][2] * 114;
}

template <int N>
void anadir(QVector<int> &v, const int (&arr)[N])
{
    for (int i = 0; i < N; i++)
    {
        v.append(arr[i]);
    }
}

} // namespace

QList<PCFutbolEquipacion::ColorEquipacion> PCFutbolEquipacion::coloresDisponibles()
{
    return {
        { "Rojo",        {0x80,0x81,0x82,0x83,0x84} },
        { "Azul",        {0x8A,0x8B,0x8C,0x8D,0x8E} },
        { "Blanco",      {0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F} },
        { "Negro",       {0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67} },
        { "Gris",        {0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F} },
        { "Amarillo",    {0x85,0x86,0x87,0x88,0x89} },
        { "Verde",       {0x8F,0x90,0x91,0x92,0x93} },
        { "Celeste",     {0x94,0x95,0x96,0x97,0x98} },
        { "Azul marino", {0x99,0x9A,0x9B,0x9C,0x9D} },
        { "Granate",     {0x9E,0x9F,0xA0,0xA1,0xA2} },
        { "Naranja",     {0xAD,0xAE,0xAF,0xB0,0xB1} },
        { "Turquesa",    {0xB2,0xB3,0xB4,0xB5,0xB6} },
        { "Morado",      {0xC1,0xC2,0xC3,0xC4,0xC5} },
    };
}

QByteArray PCFutbolEquipacion::generarTextura(Patron patron,
                                               const ColorEquipacion &colorA,
                                               const ColorEquipacion &colorB,
                                               int anchoRaya,
                                               bool preservarSombra)
{
    QByteArray textura;
    textura.resize(TAM_TEXTURA);
    textura.fill(char(0));
    if (colorA.rampa.isEmpty())
    {
        return textura;
    }
    const QVector<quint8> &rampaB = colorB.rampa.isEmpty() ? colorA.rampa : colorB.rampa;
    if (anchoRaya < 1) anchoRaya = 1;

    for (int y = 0; y < ALTO; y++)
    {
        for (int x = 0; x < ANCHO; x++)
        {
            bool usarB = false;
            int posEnFranja = 0;
            switch (patron)
            {
            case Patron::Liso:
                usarB = false;
                posEnFranja = x % anchoRaya;
                break;
            case Patron::RayasVerticales:
                usarB = ((x / anchoRaya) % 2) == 1;
                posEnFranja = x % anchoRaya;
                break;
            case Patron::FranjasHorizontales:
                usarB = ((y / anchoRaya) % 2) == 1;
                posEnFranja = y % anchoRaya;
                break;
            }
            const QVector<quint8> &rampa = usarB ? rampaB : colorA.rampa;

            // Se usan los "anchoRaya" tonos más oscuros de la rampa, pintados de
            // claro a oscuro. Con anchoRaya=4 esto reproduce EXACTAMENTE los bytes
            // de las texturas reales (p.ej. Barça: 83 82 81 80 / 8D 8C 8B 8A).
            const int tonos = qMin(anchoRaya, rampa.size());
            int idxRampa = 0;
            if (tonos > 1)
            {
                idxRampa = (tonos - 1) - (posEnFranja * tonos) / anchoRaya;
            }
            idxRampa = qBound(0, idxRampa, rampa.size() - 1);

            textura[y * ANCHO + x] = static_cast<char>(rampa.at(idxRampa));
        }
    }

    if (preservarSombra)
    {
        restaurarSombra(textura);
    }
    return textura;
}

QByteArray PCFutbolEquipacion::cabeceraFija()
{
    QByteArray h;
    h.append(char(0x90));
    h.append("QRSTUV>"); // 7 caracteres ASCII, offsets 1-7
    return h;
}

QByteArray PCFutbolEquipacion::cierreFijo()
{
    // Cierre más frecuente en los 470 kits reales (177 de ellos). Los
    // bytes 185 y 187-189 varían por equipo: al guardar en un EQUIPOS.DAT
    // existente se usa escribirTexturaEquipacion(), que no los toca.
    static const quint8 bytes[14] = {
        0x00, 0xA6, 0x68, 0x6C, 0x70, 0x72, 0x77, 0x7B,
        0x29, 0x29, 0x29, 0xFF, 0xFF, 0xFF
    };
    QByteArray c;
    for (int i = 0; i < 14; i++)
    {
        c.append(char(bytes[i]));
    }
    return c;
}

QRgb PCFutbolEquipacion::colorRepresentativo(const QByteArray &bloque190)
{
    if (bloque190.size() != TAM_BLOQUE) return qRgb(0, 0, 0);
    return qRgb(static_cast<quint8>(bloque190.at(POS_COLOR_REP_RGB)),
                static_cast<quint8>(bloque190.at(POS_COLOR_REP_RGB + 1)),
                static_cast<quint8>(bloque190.at(POS_COLOR_REP_RGB + 2)));
}

void PCFutbolEquipacion::setColorRepresentativo(QByteArray &bloque190, quint8 idxPaleta)
{
    if (bloque190.size() != TAM_BLOQUE) return;
    bloque190[POS_COLOR_REP_IDX]     = static_cast<char>(idxPaleta);
    bloque190[POS_COLOR_REP_RGB]     = static_cast<char>(paleta_kits[idxPaleta][0]);
    bloque190[POS_COLOR_REP_RGB + 1] = static_cast<char>(paleta_kits[idxPaleta][1]);
    bloque190[POS_COLOR_REP_RGB + 2] = static_cast<char>(paleta_kits[idxPaleta][2]);
}

bool PCFutbolEquipacion::escribirColorRepresentativo(const QString &equiposDatPath, int n,
                                                     bool primera, quint8 idxPaleta)
{
    bool ok = false;
    QByteArray b = leerBloqueEquipacion(equiposDatPath, n, primera, &ok);
    if (!ok) return false;
    setColorRepresentativo(b, idxPaleta);
    return escribirBloqueEquipacion(equiposDatPath, n, primera, b);
}

quint8 PCFutbolEquipacion::colorDominanteCamiseta(const QByteArray &textura168)
{
    int cuenta[256] = {0};
    for (int p = 3 * ANCHO; p < qMin(int(textura168.size()), int(TAM_TEXTURA)); p++)
    {
        if (esPosicionCamiseta(p)) cuenta[static_cast<quint8>(textura168.at(p))]++;
    }
    int mejor = 0;
    for (int i = 1; i < 256; i++)
    {
        if (cuenta[i] > cuenta[mejor]) mejor = i;
    }
    return static_cast<quint8>(mejor);
}

void PCFutbolEquipacion::rgbAHslJuego(int r, int g, int b, int *h, int *l, int *s)
{
    // Réplica de la función 0x74F3C de PCMANAG.DAT
    const int mn = qMin(r, qMin(g, b));
    const int mx = qMax(r, qMax(g, b));
    const int suma = mx + mn;
    const int delta = mx - mn;
    *l = (suma * 100) >> 9;
    if (delta == 0) { *s = 0; *h = 0; return; }
    *s = (*l < 50) ? (delta * 100) / suma : (delta * 100) / (510 - suma);
    int hh;
    if (mx == r)      hh = ((g - b) * 60) / delta;
    else if (mx == g) hh = ((b - r) * 60) / delta + 120;
    else              hh = ((r - g) * 60) / delta + 240;
    if (hh < 0) hh += 360;
    *h = hh;
}

int PCFutbolEquipacion::distanciaChoque(QRgb a, QRgb b)
{
    int h1, l1, s1, h2, l2, s2;
    rgbAHslJuego(qRed(a), qGreen(a), qBlue(a), &h1, &l1, &s1);
    rgbAHslJuego(qRed(b), qGreen(b), qBlue(b), &h2, &l2, &s2);
    int d = qAbs(h1 - h2);
    if (d > 180) d = 360 - d;
    const bool gris1 = (h1 == 0 && s1 == 0);
    const bool gris2 = (h2 == 0 && s2 == 0);
    if (gris1 != gris2) d = 200;      // uno sin tono y otro con tono: no chocan
    if (gris1 && gris2) d = 0;        // dos sin tono: siempre chocan
    return d;
}

QByteArray PCFutbolEquipacion::bloqueDesdeTextura(const QByteArray &textura168)
{
    QByteArray t = textura168;
    t.resize(TAM_TEXTURA);
    QByteArray bloque;
    bloque.append(cabeceraFija());
    bloque.append(t);
    bloque.append(cierreFijo());
    return bloque;
}

QByteArray PCFutbolEquipacion::generarBloqueEquipacion(Patron patron,
                                                        const ColorEquipacion &colorA,
                                                        const ColorEquipacion &colorB,
                                                        int anchoRaya)
{
    return bloqueDesdeTextura(generarTextura(patron, colorA, colorB, anchoRaya, true));
}

int PCFutbolEquipacion::posicionTextura(int sx, int sy)
{
    if (sx < 0 || sy < 0 || sx >= SPRITE_W || sy >= SPRITE_H)
    {
        return -1;
    }
    return mapaSprite[sy * SPRITE_W + sx];
}

int PCFutbolEquipacion::colorEnPixel(const QByteArray &textura168, int sx, int sy)
{
    const int pos = posicionTextura(sx, sy);
    if (pos < 0 || textura168.size() < TAM_TEXTURA)
    {
        return -1;
    }
    return static_cast<quint8>(textura168.at(pos));
}

bool PCFutbolEquipacion::pintarPixel(QByteArray &textura168, int sx, int sy,
                                     quint8 idxPaleta, bool protegerSombra)
{
    const int pos = posicionTextura(sx, sy);
    if (pos < 0 || textura168.size() < TAM_TEXTURA)
    {
        return false;
    }
    if (protegerSombra && pos == POS_SOMBRA)
    {
        return false; // el contorno negro del juego no se toca
    }
    if (static_cast<quint8>(textura168.at(pos)) == idxPaleta)
    {
        return false;
    }
    textura168[pos] = static_cast<char>(idxPaleta);
    return true;
}

QVector<int> PCFutbolEquipacion::posicionesZona(Zona zona)
{
    QVector<int> v;
    switch (zona)
    {
    case Zona::Sombra:         v.append(POS_SOMBRA); break;
    case Zona::Cuello:         anadir(v, ZONA_CUELLO); break;
    case Zona::Ribete:         anadir(v, ZONA_RIBETE); break;
    case Zona::MangaIzquierda: anadir(v, ZONA_MANGA_I); break;
    case Zona::MangaDerecha:   anadir(v, ZONA_MANGA_D); break;
    case Zona::Mangas:         anadir(v, ZONA_MANGA_I); anadir(v, ZONA_MANGA_D); break;
    case Zona::Hombros:        anadir(v, ZONA_HOMBROS); break;
    case Zona::Torso:          anadir(v, ZONA_TORSO); break;
    case Zona::Faldon:         anadir(v, ZONA_FALDON); break;
    case Zona::TodoLoVisible:
        for (int i = 0; i < SPRITE_W * SPRITE_H; i++)
        {
            const int p = mapaSprite[i];
            if (p >= 0 && !v.contains(p))
            {
                v.append(p);
            }
        }
        break;
    case Zona::Camiseta:
        for (int p = 0; p < TAM_TEXTURA; p++)
        {
            if (esPosicionCamiseta(p)) v.append(p);
        }
        break;
    case Zona::Pantalon: anadir(v, ZONA_PANTALON); break;
    case Zona::Medias:   anadir(v, ZONA_MEDIAS); break;
    case Zona::Botas:    anadir(v, ZONA_BOTAS); break;
    case Zona::Pelo:     anadir(v, ZONA_PELO); break;
    }
    return v;
}

bool PCFutbolEquipacion::esPosicionCamiseta(int pos)
{
    if (pos < 0 || pos >= TAM_TEXTURA) return false;
    return !contiene(ZONA_BOTAS, pos) && !contiene(ZONA_PELO, pos) &&
           !contiene(ZONA_MEDIAS, pos) && !contiene(ZONA_PANTALON, pos) &&
           !contiene(ZONA_FIJOS, pos);
}

void PCFutbolEquipacion::copiarSoloCamiseta(QByteArray &destino, const QByteArray &origen)
{
    if (destino.size() < TAM_TEXTURA || origen.size() < TAM_TEXTURA) return;
    for (int p = 0; p < TAM_TEXTURA; p++)
    {
        if (esPosicionCamiseta(p)) destino[p] = origen[p];
    }
}

QVector<quint8> PCFutbolEquipacion::rampaDeIndice(quint8 idx)
{
    if (idx == 0x29) // blanco puro: lo usan los kits como tono más claro del blanco
    {
        for (const ColorEquipacion &c : coloresDisponibles())
            if (c.nombre == "Blanco") return c.rampa;
    }
    for (const ColorEquipacion &c : coloresDisponibles())
    {
        if (c.rampa.contains(idx)) return c.rampa;
    }
    return {};
}

void PCFutbolEquipacion::pintarZonaRampa(QByteArray &textura168, Zona zona,
                                         const QVector<quint8> &rampa)
{
    if (textura168.size() < TAM_TEXTURA || rampa.isEmpty()) return;
    const QVector<int> pos = posicionesZona(zona);
    if (pos.isEmpty()) return;

    // La rampa se ordena de oscuro a claro por luminancia real
    QVector<quint8> r = rampa;
    std::sort(r.begin(), r.end(), [](quint8 a, quint8 b) { return luminancia(a) < luminancia(b); });

    int lmin = INT_MAX, lmax = INT_MIN;
    for (int p : pos)
    {
        const int l = luminancia(static_cast<quint8>(textura168.at(p)));
        lmin = qMin(lmin, l);
        lmax = qMax(lmax, l);
    }
    for (int p : pos)
    {
        int k = r.size() / 2; // zona de un solo tono: tono medio
        if (lmax > lmin)
        {
            const int l = luminancia(static_cast<quint8>(textura168.at(p)));
            k = qRound(double(l - lmin) * (r.size() - 1) / double(lmax - lmin));
        }
        textura168[p] = static_cast<char>(r.at(qBound(0, k, int(r.size()) - 1)));
    }
}

QByteArray PCFutbolEquipacion::tablaRecoloreado(const QByteArray &textura168,
                                                const QByteArray &bloqueOriginal)
{
    QByteArray t = textura168;
    t.resize(TAM_TEXTURA);
    QByteArray lut;
    lut.append(cabeceraFija());
    lut.append(t);
    if (bloqueOriginal.size() == TAM_BLOQUE)
        lut.append(bloqueOriginal.mid(8 + TAM_TEXTURA));
    else
        lut.append(cierreFijo());
    return lut;
}

bool PCFutbolEquipacion::escribirTexturaEquipacion(const QString &equiposDatPath, int n, bool primera,
                                                   const QByteArray &textura168)
{
    if (textura168.size() != TAM_TEXTURA) return false;
    QFile f(equiposDatPath);
    if (!f.open(QIODevice::ReadWrite)) return false;
    const int offset = kitOffset(n, primera);
    if (offset < 0 || offset + TAM_BLOQUE > f.size())
    {
        f.close();
        return false;
    }
    // Comprobación de seguridad: el bloque debe empezar por la cabecera fija
    f.seek(offset);
    if (f.read(8) != cabeceraFija())
    {
        f.close();
        return false;
    }
    f.seek(offset + 8);
    const qint64 escritos = f.write(textura168);
    f.close();
    return escritos == TAM_TEXTURA;
}

void PCFutbolEquipacion::pintarZona(QByteArray &textura168, Zona zona,
                                    quint8 idxPaleta, bool protegerSombra)
{
    if (textura168.size() < TAM_TEXTURA)
    {
        return;
    }
    const QVector<int> pos = posicionesZona(zona);
    for (int p : pos)
    {
        if (protegerSombra && p == POS_SOMBRA)
        {
            continue;
        }
        textura168[p] = static_cast<char>(idxPaleta);
    }
}

void PCFutbolEquipacion::restaurarSombra(QByteArray &textura168)
{
    if (textura168.size() >= TAM_TEXTURA)
    {
        textura168[POS_SOMBRA] = static_cast<char>(COLOR_SOMBRA);
    }
}

QImage PCFutbolEquipacion::previsualizar(const QByteArray &textura168, int escala)
{
    if (escala < 1) escala = 1;
    const int W = SPRITE_W * escala;
    const int H = SPRITE_H * escala;

    QImage im(W, H, QImage::Format_ARGB32);
    im.fill(Qt::transparent);

    const bool hayTextura = (textura168.size() >= TAM_TEXTURA);

    for (int sy = 0; sy < SPRITE_H; sy++)
    {
        for (int sx = 0; sx < SPRITE_W; sx++)
        {
            const int pos = mapaSprite[sy * SPRITE_W + sx];
            if (pos < 0)
            {
                continue; // fondo
            }
            QColor color(90, 90, 90); // gris si todavía no hay textura
            if (hayTextura)
            {
                const quint8 idx = static_cast<quint8>(textura168.at(pos));
                color = QColor(paleta_kits[idx][0], paleta_kits[idx][1], paleta_kits[idx][2]);
            }
            for (int dy = 0; dy < escala; dy++)
            {
                for (int dx = 0; dx < escala; dx++)
                {
                    im.setPixelColor(sx * escala + dx, sy * escala + dy, color);
                }
            }
        }
    }
    return im;
}

int PCFutbolEquipacion::kitOffset(int n, bool primera)
{
    const int kitIdx = 2 * (n - 1) + (primera ? 0 : 1);
    return kitIdx * TAM_BLOQUE;
}

QByteArray PCFutbolEquipacion::leerBloqueEquipacion(const QString &equiposDatPath, int n, bool primera, bool *ok)
{
    if (ok) *ok = false;
    QFile f(equiposDatPath);
    if (!f.open(QIODevice::ReadOnly))
    {
        return QByteArray();
    }
    const int offset = kitOffset(n, primera);
    if (offset < 0 || offset + TAM_BLOQUE > f.size())
    {
        f.close();
        return QByteArray();
    }
    f.seek(offset);
    const QByteArray bloque = f.read(TAM_BLOQUE);
    f.close();
    if (bloque.size() == TAM_BLOQUE && ok)
    {
        *ok = true;
    }
    return bloque;
}

bool PCFutbolEquipacion::escribirBloqueEquipacion(const QString &equiposDatPath, int n, bool primera, const QByteArray &bloque190)
{
    if (bloque190.size() != TAM_BLOQUE)
    {
        return false;
    }
    QFile f(equiposDatPath);
    if (!f.open(QIODevice::ReadWrite))
    {
        return false;
    }
    const int offset = kitOffset(n, primera);
    if (offset < 0 || offset + TAM_BLOQUE > f.size())
    {
        f.close();
        return false;
    }
    f.seek(offset);
    const qint64 escritos = f.write(bloque190);
    f.close();
    return escritos == TAM_BLOQUE;
}
