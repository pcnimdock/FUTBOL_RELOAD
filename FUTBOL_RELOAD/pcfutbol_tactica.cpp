#include "pcfutbol_tactica.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QTime>

namespace PCFutbolTactica
{

bool Tactica::operator==(const Tactica &o) const
{
    return aBytes(*this) == aBytes(o);
}

QByteArray aBytes(const Tactica &t)
{
    QByteArray b;
    b.reserve(TAM);
    for (const Jugador &j : t.j)
        for (int v : {j.zx, j.zy, j.zancho, j.zalto, j.dx, j.dy, j.ax, j.ay})
        {
            b.append(char(v & 0xFF));
            b.append(char((v >> 8) & 0xFF));
        }
    return b;
}

bool deBytes(const QByteArray &b, Tactica *t)
{
    if (b.size() < TAM) return false;
    Tactica r;
    for (int k = 0; k < JUGADORES; k++)
    {
        int v[8];
        for (int i = 0; i < 8; i++)
        {
            const int o = (k * 8 + i) * 2;
            v[i] = quint8(b[o]) | (quint8(b[o + 1]) << 8);
            if (v[i] > 15) return false;                 // no es una táctica
        }
        r.j[k] = {v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]};
    }
    *t = r;
    return true;
}

bool leer(const QByteArray &cosas, Tactica *t)
{
    Tactica r;
    if (!deBytes(cosas, &r)) return false;
    // el portero de los originales es siempre 0,3,2,3,0,4,0,4; aquí basta con
    // que el bloque cumpla la regla (por si alguien ya lo ha cambiado)
    if (!comprobar(r).isEmpty()) return false;
    *t = r;
    return true;
}

bool escribir(QByteArray &cosas, const Tactica &t)
{
    if (cosas.size() < TAM) return false;
    const QByteArray b = aBytes(t);
    for (int i = 0; i < TAM; i++) cosas[i] = b[i];
    return true;
}

QStringList nombresFormaciones()
{
    return {"3-4-3", "3-5-2", "4-3-3", "4-4-2", "5-3-2", "5-4-1"};
}

Tactica formacion(int i)
{
    // PCMANAG.DAT (4.5) desde 0xA4FD0, 6 tablas de 0x160 bytes (11 × 8 int32)
    static const int datos[6][11][8] = {
    {{0,3,2,3,0,4,0,4}, {0,5,6,4,1,7,4,7}, {0,0,6,4,1,1,4,1}, {0,3,6,3,1,4,4,4}, {3,2,6,5,4,4,7,4}, {4,0,9,4,6,1,10,1}, {9,4,6,5,9,6,12,6}, {5,6,8,3,6,7,10,7}, {10,2,5,5,10,4,12,4}, {5,2,8,5,7,4,11,4}, {9,0,6,5,9,2,12,2}},
    {{0,3,2,3,0,4,0,4}, {0,5,8,4,2,6,5,6}, {0,0,8,4,2,2,5,2}, {0,2,5,5,1,4,4,4}, {3,1,7,5,5,2,8,2}, {6,0,9,4,7,1,12,1}, {6,5,9,4,7,7,12,7}, {5,3,9,5,5,6,10,6}, {10,2,5,5,11,4,13,4}, {4,2,8,5,5,4,10,4}, {9,0,6,5,10,3,12,3}},
    {{0,3,2,3,0,4,0,4}, {0,6,8,3,3,7,6,7}, {0,0,10,4,3,1,7,1}, {0,2,5,5,2,5,4,5}, {0,2,5,5,2,3,4,3}, {6,0,9,4,6,1,12,1}, {8,4,7,5,10,7,12,6}, {6,5,8,4,6,7,11,7}, {10,2,5,5,11,4,13,4}, {5,2,7,5,6,4,10,4}, {9,0,6,5,10,1,12,2}},
    {{0,3,2,3,0,4,0,4}, {0,6,8,3,2,7,5,7}, {0,0,10,4,2,1,6,1}, {0,2,6,5,2,5,5,5}, {0,2,6,5,1,3,4,3}, {5,0,10,3,6,1,11,1}, {5,3,9,4,6,5,12,5}, {6,5,9,4,7,7,11,7}, {10,2,5,5,11,5,13,5}, {4,1,8,6,6,3,10,3}, {9,1,6,5,11,3,13,3}},
    {{0,3,2,3,0,4,0,4}, {0,6,8,3,2,7,5,7}, {0,0,10,3,2,1,6,1}, {0,2,6,5,1,4,4,4}, {0,4,7,4,2,6,5,6}, {0,1,7,4,2,2,5,2}, {6,0,9,4,7,1,11,1}, {6,4,9,5,7,7,11,7}, {10,2,5,6,11,5,13,5}, {5,2,7,5,6,4,10,4}, {9,0,6,5,10,2,12,3}},
    {{0,3,2,3,0,4,0,4}, {0,6,8,3,2,7,5,7}, {0,0,10,3,2,1,5,1}, {0,2,6,5,1,4,4,4}, {0,4,6,4,2,6,5,6}, {0,1,6,4,1,2,4,2}, {6,5,9,4,7,7,11,7}, {6,0,9,4,7,1,11,1}, {9,2,6,5,10,4,12,4}, {5,2,9,5,6,5,10,5}, {6,1,7,5,6,3,10,3}},
    };
    Tactica t;
    i = qBound(0, i, 5);
    for (int k = 0; k < JUGADORES; k++)
    {
        const int *v = datos[i][k];
        t.j[k] = {v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]};
    }
    return t;
}

static bool dentro(int x, int y, const Jugador &j)
{
    return x >= j.zx && x < j.zx + j.zancho && y >= j.zy && y < j.zy + j.zalto;
}

QStringList comprobar(const Tactica &t)
{
    QStringList p;
    for (int k = 0; k < JUGADORES; k++)
    {
        const Jugador &j = t.j[k];
        if (j.zancho < 1 || j.zalto < 1 || j.zx < 0 || j.zy < 0 ||
            j.zx + j.zancho > COLUMNAS || j.zy + j.zalto > FILAS)
            p << QString("Jugador %1: la zona se sale del campo").arg(k + 1);
        if (!dentro(j.dx, j.dy, j)) p << QString("Jugador %1: la posición de defensa está fuera de su zona").arg(k + 1);
        if (!dentro(j.ax, j.ay, j)) p << QString("Jugador %1: la posición de ataque está fuera de su zona").arg(k + 1);
    }
    return p;
}

void corregir(Tactica &t)
{
    for (Jugador &j : t.j)
    {
        j.zancho = qBound(1, j.zancho, COLUMNAS);
        j.zalto = qBound(1, j.zalto, FILAS);
        j.zx = qBound(0, j.zx, COLUMNAS - j.zancho);
        j.zy = qBound(0, j.zy, FILAS - j.zalto);
        j.dx = qBound(j.zx, j.dx, j.zx + j.zancho - 1);
        j.dy = qBound(j.zy, j.dy, j.zy + j.zalto - 1);
        j.ax = qBound(j.zx, j.ax, j.zx + j.zancho - 1);
        j.ay = qBound(j.zy, j.ay, j.zy + j.zalto - 1);
    }
}

// --- TACTIC.nnn ---------------------------------------------------------
static QString textoXor(const QByteArray &d, int o, int n)
{
    QByteArray s;
    for (int i = 0; i < n; i++)
    {
        const char c = char(quint8(d[o + i]) ^ 0x6A);
        if (c == 0) break;
        s.append(c);
    }
    return QString::fromLatin1(s);
}

static void ponerTextoXor(QByteArray &d, int o, int n, const QString &s)
{
    const QByteArray l = s.toLatin1().left(n - 1);
    for (int i = 0; i < n; i++)
        d[o + i] = char((i < l.size() ? quint8(l[i]) : 0) ^ 0x6A);
}

static void ponerU32(QByteArray &d, int o, quint32 v)
{
    for (int i = 0; i < 4; i++) d[o + i] = char((v >> (8 * i)) & 0xFF);
}

bool leerFichero(const QString &ruta, Tactica *t, QString *nombre, QString *manager, QString *error)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error) *error = "no se puede abrir";
        return false;
    }
    const QByteArray d = f.readAll();
    if (d.size() < TAM_FICHERO || !deBytes(d.mid(0x44), t))
    {
        if (error) *error = "no es un fichero de táctica del juego";
        return false;
    }
    if (nombre) *nombre = textoXor(d, 0x00, 24);
    if (manager) *manager = textoXor(d, 0x18, 16);
    return true;
}

bool grabarFichero(const QString &ruta, const Tactica &t, const QString &nombre,
                   const QString &manager, QString *error)
{
    QByteArray d(TAM_FICHERO, 0);
    ponerTextoXor(d, 0x00, 24, nombre);
    ponerTextoXor(d, 0x18, 16, manager);
    const QDate hoy = QDate::currentDate();
    const QTime ahora = QTime::currentTime();
    ponerU32(d, 0x28, quint32(hoy.year()));
    ponerU32(d, 0x2C, quint32(hoy.month()));
    ponerU32(d, 0x30, quint32(hoy.day()));
    ponerU32(d, 0x34, quint32(ahora.msec() / 10));
    ponerU32(d, 0x38, quint32(ahora.second()));
    ponerU32(d, 0x3C, quint32(ahora.minute()));
    ponerU32(d, 0x40, quint32(ahora.hour()));
    const QByteArray b = aBytes(t);
    for (int i = 0; i < TAM; i++) d[0x44 + i] = b[i];
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly) || f.write(d) != d.size())
    {
        if (error) *error = "no se puede escribir " + ruta;
        return false;
    }
    return true;
}

QString ficheroLibre(const QString &carpeta)
{
    QDir dir(carpeta);
    QStringList usados;
    for (const QString &f : dir.entryList(QDir::Files)) usados << f.toUpper();
    for (int i = 0; i < 200; i++)
    {
        const QString n = QString("TACTIC.%1").arg(i, 3, 10, QChar('0'));
        if (!usados.contains(n)) return dir.absoluteFilePath(n);
    }
    return QString();
}

QString carpetaTactics(const QString &carpetaJuego)
{
    QDir d(carpetaJuego);
    for (const QString &s : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (s.compare("TACTICS", Qt::CaseInsensitive) == 0) return d.absoluteFilePath(s);
    return QString();
}

} // namespace PCFutbolTactica
