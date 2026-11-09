#include "pcfutbol_csv.h"
#include "pcfutbol_plantillas.h"
#include "pcfutbol_temporada_real.h"

#include <QFile>
#include <QHash>
#include <QSet>
#include <QStringDecoder>

using namespace PCFutbolPlantillas;

namespace {

// ---------------------------------------------------------------------------
// Columnas
// ---------------------------------------------------------------------------
const QStringList COLS_JUGADORES = {
    "equipo_id", "equipo", "jugador_id", "nombre_corto", "nombre_largo", "dorsal",
    "posicion", "demarcacion", "dem_sec_1", "dem_sec_2", "dem_sec_3", "dem_sec_4", "dem_sec_5",
    "extranjero", "piel", "pelo",
    "velocidad", "regate", "agilidad", "agresividad", "tecnica", "remate", "pase", "tiro",
    "estado_fichaje", "dia_nac", "mes_nac", "anyo_nac", "lugar_nacimiento", "procedencia",
    "internacional", "altura_cm", "peso_kg", "precio", "comentario"};

const QStringList COLS_EQUIPOS = {
    "equipo_id", "pais", "nombre_corto", "nombre_largo", "nombre_mayusculas",
    "nombre_corto_mayusculas", "fundacion", "estadio", "aforo", "socios", "presidente",
    "presupuesto_bbdd", "presupuesto_juego", "patrocinador", "proveedor",
    "entrenador", "entrenador_nombre_completo"};

// ---------------------------------------------------------------------------
// Escritura
// ---------------------------------------------------------------------------
QString celdaTexto(const QString &s)
{
    QString t = s;
    t.replace('"', "\"\"");
    return '"' + t + '"';
}

QString celdaNum(qint64 v) { return QString::number(v); }

bool escribir(const QString &ruta, const QStringList &lineas, QString *error)
{
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly))
    {
        if (error) *error = "No se pudo escribir " + ruta;
        return false;
    }
    QByteArray d("\xEF\xBB\xBF");               // BOM: Excel detecta UTF-8
    d += lineas.join("\r\n").toUtf8();
    d += "\r\n";
    if (f.write(d) != d.size())
    {
        if (error) *error = "No se pudo escribir " + ruta;
        return false;
    }
    return true;
}

QString filaJugador(const EQUIPO &eq, const JUGADOR &j)
{
    QStringList c;
    c << celdaNum(eq.EquipoIdDBC) << celdaTexto(eq.NombreCorto) << celdaNum(j.puntero)
      << celdaTexto(j.NombreCorto) << celdaTexto(j.NombreLargo) << celdaNum(j.NumeroCamiseta)
      << celdaNum(j.PosicionEnCampo) << celdaNum(j.DemarcacionPref);
    for (int k = 0; k < 5; k++)
        c << celdaNum(k < j.DemarcacionesSecundarias.size()
                          ? static_cast<quint8>(j.DemarcacionesSecundarias.at(k)) : 0);
    c << celdaNum(j.Extranjero) << celdaNum(j.Piel) << celdaNum(j.Pelo)
      << celdaNum(j.Velocidad) << celdaNum(j.Regate) << celdaNum(j.Agilidad)
      << celdaNum(j.Agresividad) << celdaNum(j.ActTecnica ? 1 : 0) << celdaNum(j.ActRemate ? 1 : 0)
      << celdaNum(j.ActPase ? 1 : 0) << celdaNum(j.ActTiro ? 1 : 0) << celdaNum(j.EstadoFichaje);
    // Datos que sólo existen en los equipos con ficha completa
    if (eq.isBBDD_neg == 0 && j.FechaDeNacimiento.size() >= 3)
    {
        const int anyo = static_cast<quint8>(j.FechaDeNacimiento.at(2));
        c << celdaNum(static_cast<quint8>(j.FechaDeNacimiento.at(0)))
          << celdaNum(static_cast<quint8>(j.FechaDeNacimiento.at(1)))
          << (anyo ? celdaNum(1900 + anyo) : QString())
          << celdaTexto(j.LugarDeNacimiento) << celdaTexto(j.EquipoProcedencia)
          << celdaTexto(j.Internacional) << celdaNum(j.Altura) << celdaNum(j.Peso)
          << celdaNum(j.Precio) << celdaTexto(j.Comentario);
    }
    else
    {
        for (int k = 0; k < 10; k++) c << QString();
    }
    return c.join(';');
}

QString filaEquipo(const EQUIPO &eq)
{
    const bool completa = eq.isBBDD_neg == 0;
    const ENTRENADOR *en = eq.lista_entrenadores.isEmpty() ? nullptr : &eq.lista_entrenadores.first();
    QStringList c;
    c << celdaNum(eq.EquipoIdDBC) << celdaNum(eq.pais) << celdaTexto(eq.NombreCorto)
      << celdaTexto(eq.NombreLargo) << celdaTexto(eq.NombreMayusculas)
      << celdaTexto(eq.nombre_corto_mayusculas)
      << (completa ? celdaNum(eq.anyo_fundacion) : QString())
      << celdaTexto(eq.estadio) << celdaNum(eq.aforo)
      << (completa ? celdaNum(eq.num_socios) : QString())
      << (completa ? celdaTexto(eq.presidente) : QString())
      << (completa ? celdaNum(eq.presupuesto_bbdd) : QString())
      << (completa ? celdaNum(eq.presupuesto_juego) : QString())
      << (completa ? celdaTexto(eq.patrocinador) : QString())
      << (completa ? celdaTexto(eq.proveedor) : QString())
      << (en ? celdaTexto(en->NombreCorto) : QString())
      << (en && completa ? celdaTexto(en->NombreLargo) : QString());
    return c.join(';');
}

// ---------------------------------------------------------------------------
// Lectura
// ---------------------------------------------------------------------------
QString decodificar(const QByteArray &bruto, QString *codificacion)
{
    QByteArray d = bruto;
    if (d.startsWith("\xEF\xBB\xBF")) d.remove(0, 3);
    QStringDecoder utf8(QStringDecoder::Utf8);
    const QString s = utf8.decode(d);
    if (!utf8.hasError())
    {
        *codificacion = "UTF-8";
        return s;
    }
    // Windows-1252: Latin-1 salvo 0x80-0x9F
    static const ushort cp1252[32] = {
        0x20AC, 0x81, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
        0x2039, 0x0152, 0x8D, 0x017D, 0x8F, 0x90, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
        0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x9D, 0x017E, 0x0178};
    QString r;
    r.reserve(d.size());
    for (char ch : d)
    {
        const quint8 b = static_cast<quint8>(ch);
        r += (b >= 0x80 && b < 0xA0) ? QChar(cp1252[b - 0x80]) : QChar(b);
    }
    *codificacion = "Windows-1252";
    return r;
}

QChar detectarSeparador(const QString &texto)
{
    int pc = 0, coma = 0, tab = 0;
    bool comillas = false;
    for (QChar c : texto)
    {
        if (c == '"') comillas = !comillas;
        else if (!comillas && (c == '\n' || c == '\r')) break;
        else if (!comillas && c == ';') pc++;
        else if (!comillas && c == ',') coma++;
        else if (!comillas && c == '\t') tab++;
    }
    if (tab > pc && tab > coma) return '\t';
    return coma > pc ? QChar(',') : QChar(';');
}

QList<QStringList> partir(const QString &texto, QChar sep)
{
    QList<QStringList> filas;
    QStringList fila;
    QString celda;
    bool comillas = false;
    for (int i = 0; i < texto.size(); i++)
    {
        const QChar c = texto.at(i);
        if (comillas)
        {
            if (c == '"')
            {
                if (i + 1 < texto.size() && texto.at(i + 1) == '"') { celda += '"'; i++; }
                else comillas = false;
            }
            else celda += c;
        }
        else if (c == '"') comillas = true;
        else if (c == sep) { fila << celda; celda.clear(); }
        else if (c == '\r') { }
        else if (c == '\n')
        {
            fila << celda;
            celda.clear();
            filas << fila;
            fila.clear();
        }
        else celda += c;
    }
    if (!celda.isEmpty() || !fila.isEmpty())
    {
        fila << celda;
        filas << fila;
    }
    // quitar filas totalmente vacías
    QList<QStringList> r;
    for (const QStringList &f : filas)
    {
        bool vacia = true;
        for (const QString &x : f) if (!x.trimmed().isEmpty()) { vacia = false; break; }
        if (!vacia) r << f;
    }
    return r;
}

// Acceso a una fila por nombre de columna
class Fila
{
public:
    Fila(const QStringList &cab, const QStringList &f, int numero) : m_cab(cab), m_f(f), m_num(numero) {}
    bool tiene(const QString &col) const
    {
        const int i = m_cab.indexOf(col);
        return i >= 0 && i < m_f.size() && !m_f.at(i).trimmed().isEmpty();
    }
    QString texto(const QString &col) const
    {
        const int i = m_cab.indexOf(col);
        return (i >= 0 && i < m_f.size()) ? m_f.at(i) : QString();
    }
    int numero() const { return m_num; }
private:
    QStringList m_cab, m_f;
    int m_num;
};

// Número entero (acepta "1.234", "1 234" y "12,0" de las hojas de cálculo)
bool aEntero(QString s, qint64 *v)
{
    s = s.trimmed();
    s.remove(' ');
    s.remove(QChar(0x00A0));
    if (s.endsWith(",0") || s.endsWith(".0")) s.chop(2);
    if (s.count('.') >= 1 && !s.contains(',')) s.remove('.');   // separador de miles
    bool ok = false;
    *v = s.toLongLong(&ok);
    return ok;
}

// Código: número o nombre (sin distinguir mayúsculas ni acentos)
QString sinAcentos(const QString &s)
{
    QString r;
    for (QChar c : s.normalized(QString::NormalizationForm_D))
        if (c.category() != QChar::Mark_NonSpacing) r += c.toLower();
    return r.simplified();
}

bool aCodigo(const QString &s, const QStringList &nombres, qint64 *v)
{
    if (aEntero(s, v)) return true;
    const QString t = sinAcentos(s);
    for (int i = 0; i < nombres.size(); i++)
        if (sinAcentos(nombres.at(i)) == t) { *v = i; return true; }
    return false;
}

bool aBooleano(const QString &s, qint64 *v)
{
    const QString t = sinAcentos(s);
    if (t == "1" || t == "si" || t == "s" || t == "x" || t == "true" || t == "verdadero") { *v = 1; return true; }
    if (t == "0" || t == "no" || t == "n" || t == "false" || t == "falso") { *v = 0; return true; }
    return false;
}

// Aplica columnas de una fila a variables, acumulando avisos
class Aplicador
{
public:
    Aplicador(const Fila &f, const QString &quien, QStringList *avisos)
        : m_f(f), m_quien(quien), m_avisos(avisos) {}

    template <typename T>
    void numero(const QString &col, T &dest, qint64 min, qint64 max)
    {
        if (!m_f.tiene(col)) return;
        qint64 v;
        if (!aEntero(m_f.texto(col), &v) || v < min || v > max)
        {
            aviso(QString("%1 «%2» no válido (de %3 a %4); se deja %5")
                      .arg(col, m_f.texto(col)).arg(min).arg(max).arg(qint64(dest)));
            return;
        }
        dest = static_cast<T>(v);
    }
    template <typename T>
    void codigo(const QString &col, T &dest, const QStringList &nombres)
    {
        if (!m_f.tiene(col)) return;
        qint64 v;
        if (!aCodigo(m_f.texto(col), nombres, &v) || v < 0 || v >= nombres.size())
        {
            aviso(QString("%1 «%2» no válido (0 a %3 o el nombre); se deja %4")
                      .arg(col, m_f.texto(col)).arg(nombres.size() - 1).arg(qint64(dest)));
            return;
        }
        dest = static_cast<T>(v);
    }
    template <typename T>
    void booleano(const QString &col, T &dest)
    {
        if (!m_f.tiene(col)) return;
        qint64 v;
        if (!aBooleano(m_f.texto(col), &v))
        {
            aviso(QString("%1 «%2» no válido (0/1 o sí/no)").arg(col, m_f.texto(col)));
            return;
        }
        dest = static_cast<T>(v);
    }
    void texto(const QString &col, QString &dest, bool admiteSaltos = false)
    {
        if (!m_f.tiene(col)) return;
        QString t = m_f.texto(col);
        if (admiteSaltos)
        {
            // El comentario no usa la tabla de nombres: sólo se ajustan las
            // letras que no existen, conservando saltos de línea
            QString r;
            for (QChar c : t)
                r += (c == '\n' || c == '\r') ? QString(c) : adaptarTexto(QString(c));
            if (r != t) aviso(QString("%1: letras que el juego no tiene cambiadas por las más parecidas").arg(col));
            dest = r;
            return;
        }
        bool cambiado = false;
        QString a = adaptarTexto(t, &cambiado);
        if (a.size() > MAX_TEXTO) { a.truncate(MAX_TEXTO); cambiado = true; }
        if (cambiado) aviso(QString("%1 «%2» guardado como «%3»").arg(col, t, a));
        dest = a;
    }
private:
    void aviso(const QString &t)
    {
        if (m_avisos) *m_avisos << QString("Fila %1 (%2): %3").arg(m_f.numero()).arg(m_quien, t);
    }
    const Fila &m_f;
    QString m_quien;
    QStringList *m_avisos;
};

void aplicarJugador(const Fila &f, JUGADOR &j, bool fichaCompleta, QStringList *avisos)
{
    Aplicador a(f, f.tiene("nombre_corto") ? f.texto("nombre_corto") : j.NombreCorto, avisos);
    a.texto("nombre_corto", j.NombreCorto);
    a.texto("nombre_largo", j.NombreLargo);
    a.numero("dorsal", j.NumeroCamiseta, 1, 99);
    a.codigo("posicion", j.PosicionEnCampo, nombresPosicion());
    a.codigo("demarcacion", j.DemarcacionPref, nombresDemarcacion());
    if (j.DemarcacionesSecundarias.size() != 5) j.DemarcacionesSecundarias = QByteArray(5, 0);
    for (int k = 0; k < 5; k++)
    {
        quint8 d = static_cast<quint8>(j.DemarcacionesSecundarias.at(k));
        a.codigo(QString("dem_sec_%1").arg(k + 1), d, nombresDemarcacion());
        j.DemarcacionesSecundarias[k] = static_cast<char>(d);
    }
    a.booleano("extranjero", j.Extranjero);
    a.codigo("piel", j.Piel, nombresPiel());
    a.codigo("pelo", j.Pelo, nombresPelo());
    a.numero("velocidad", j.Velocidad, 0, 99);
    a.numero("regate", j.Regate, 0, 99);
    a.numero("agilidad", j.Agilidad, 0, 99);
    a.numero("agresividad", j.Agresividad, 0, 99);
    a.booleano("tecnica", j.ActTecnica);
    a.booleano("remate", j.ActRemate);
    a.booleano("pase", j.ActPase);
    a.booleano("tiro", j.ActTiro);
    a.codigo("estado_fichaje", j.EstadoFichaje, nombresEstadoFichaje());
    if (!fichaCompleta) return;   // el resto no se guarda en equipos de ficha reducida

    if (j.FechaDeNacimiento.size() < 3)
    {
        j.FechaDeNacimiento = QByteArray();
        j.FechaDeNacimiento.append(char(1));
        j.FechaDeNacimiento.append(char(1));
        j.FechaDeNacimiento.append(char(75));
    }
    quint8 dia = static_cast<quint8>(j.FechaDeNacimiento.at(0));
    quint8 mes = static_cast<quint8>(j.FechaDeNacimiento.at(1));
    int anyo = 1900 + static_cast<quint8>(j.FechaDeNacimiento.at(2));
    a.numero("dia_nac", dia, 0, 31);   // 0 = sin dato (hay uno así en el original)
    a.numero("mes_nac", mes, 0, 12);
    a.numero("anyo_nac", anyo, 1900, 2155);
    j.FechaDeNacimiento[0] = static_cast<char>(dia);
    j.FechaDeNacimiento[1] = static_cast<char>(mes);
    j.FechaDeNacimiento[2] = static_cast<char>(anyo - 1900);
    a.texto("lugar_nacimiento", j.LugarDeNacimiento);
    a.texto("procedencia", j.EquipoProcedencia);
    a.texto("internacional", j.Internacional);
    if (f.tiene("altura_cm"))
    {
        // Se acepta también en metros («1,85» o «1.85»)
        QString h = f.texto("altura_cm").trimmed();
        h.replace(',', '.');
        bool ok = false;
        double v = h.toDouble(&ok);
        if (ok && v > 0 && v < 3) v *= 100;
        const int cm = qRound(v);
        if (ok && cm >= 100 && cm <= 255) j.Altura = static_cast<quint8>(cm);
        else if (avisos)
            *avisos << QString("Fila %1 (%2): altura_cm «%3» no válida (en cm, de 100 a 255); se deja %4")
                           .arg(f.numero()).arg(j.NombreCorto, f.texto("altura_cm")).arg(j.Altura);
    }
    a.numero("peso_kg", j.Peso, 30, 255);
    a.numero("precio", j.Precio, 0, 65535);
    a.texto("comentario", j.Comentario, true);
}

} // namespace

namespace PCFutbolCsv {

bool exportarJugadores(const QList<EQUIPO> &equipos, const QString &ruta, QString *error)
{
    QStringList l;
    l << COLS_JUGADORES.join(';');
    for (const EQUIPO &eq : equipos)
    {
        // en el orden de la pantalla de alineación
        QList<int> orden;
        for (int i = 0; i < eq.lista_jugadores.size(); i++) orden << i;
        std::stable_sort(orden.begin(), orden.end(), [&eq](int a, int b) {
            return eq.lista_jugadores.at(a).PosicionEnAlineacion < eq.lista_jugadores.at(b).PosicionEnAlineacion;
        });
        for (int i : orden) l << filaJugador(eq, eq.lista_jugadores.at(i));
    }
    return escribir(ruta, l, error);
}

bool exportarEquipos(const QList<EQUIPO> &equipos, const QString &ruta, QString *error)
{
    QStringList l;
    l << COLS_EQUIPOS.join(';');
    for (const EQUIPO &eq : equipos) l << filaEquipo(eq);
    return escribir(ruta, l, error);
}

bool escribirLeeme(const QString &ruta, QString *error)
{
    auto lista = [](const QStringList &n) {
        QStringList r;
        for (int i = 0; i < n.size(); i++) r << QString("  %1 = %2").arg(i).arg(n.at(i));
        return r.join("\n");
    };
    QString t;
    t += "FICHEROS CSV DEL EDITOR FUTBOL RELOAD\n"
         "=====================================\n\n"
         "jugadores.csv  -> un jugador por fila\n"
         "equipos.csv    -> un club por fila (estadio, presupuesto, presidente, entrenador...)\n\n"
         "Se abren con Excel o LibreOffice (separador ';', UTF-8). Al importar se aceptan\n"
         "también ',' y tabulador, y ficheros guardados por Excel en Windows-1252.\n\n"
         "REGLAS\n"
         "------\n"
         "- Celda vacía = no se cambia ese dato. Se pueden borrar columnas enteras\n"
         "  (salvo equipo_id, y jugador_id en jugadores).\n"
         "- Los nombres de las columnas (primera fila) no se pueden cambiar.\n"
         "- jugador_id identifica al jugador (es el número de su foto J95xxxxx.DFG).\n"
         "  * Si cambias su equipo_id, es un TRASPASO (conserva la foto).\n"
         "  * Una fila con jugador_id vacío es un JUGADOR NUEVO.\n"
         "  * Si borras la fila de un jugador, el editor pregunta si quitarlo.\n"
         "- La columna 'equipo' (nombre) es sólo informativa: manda equipo_id.\n"
         "- Los equipos de Segunda B y los extranjeros pequeños tienen ficha reducida:\n"
         "  no guardan fecha, lugar, procedencia, internacional, altura, peso, precio\n"
         "  ni comentario (salen vacíos). En equipos.csv tampoco fundación, socios,\n"
         "  presidente, presupuestos, patrocinador ni proveedor.\n"
         "- Letras: el juego tiene ASCII y á é í ó ú ñ ü ç à è... Las demás (ć, ł, ø...)\n"
         "  se cambian por la más parecida al importar.\n"
         "- Al importar NO se escribe nada en el juego: los cambios quedan en el editor\n"
         "  hasta que pulses Guardar (que antes comprueba las plantillas).\n"
         "- 'pais' en equipos.csv es sólo informativo (14 = España).\n\n"
         "CÓDIGOS (se puede poner el número o el nombre)\n"
         "----------------------------------------------\n";
    t += "posicion:\n" + lista(nombresPosicion()) + "\n";
    t += "demarcacion, dem_sec_1..5:\n" + lista(nombresDemarcacion()) + "\n";
    t += "piel:\n" + lista(nombresPiel()) + "\n";
    t += "pelo:\n" + lista(nombresPelo()) + "\n";
    t += "estado_fichaje (la etiqueta que sale junto al jugador):\n" + lista(nombresEstadoFichaje()) + "\n";
    t += "extranjero, tecnica, remate, pase, tiro: 0/1 (o sí/no)\n"
         "velocidad, regate, agilidad, agresividad: 0 a 99\n"
         "anyo_nac: año completo (1975). altura_cm: 181. precio: en las unidades del juego.\n";
    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly) || f.write(t.toUtf8()) < 0)
    {
        if (error) *error = "No se pudo escribir " + ruta;
        return false;
    }
    return true;
}

bool leer(const QString &ruta, Tabla *t, QString *error)
{
    QFile f(ruta);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error) *error = "No se pudo leer " + ruta;
        return false;
    }
    const QString texto = decodificar(f.readAll(), &t->codificacion);
    t->separador = detectarSeparador(texto);
    QList<QStringList> filas = partir(texto, t->separador);
    if (filas.isEmpty())
    {
        if (error) *error = "El fichero está vacío.";
        return false;
    }
    t->cabecera.clear();
    for (const QString &c : filas.takeFirst()) t->cabecera << c.trimmed().toLower();
    t->filas = filas;
    if (t->cabecera.contains("jugador_id") && t->cabecera.contains("equipo_id")) t->tipo = Jugadores;
    else if (t->cabecera.contains("equipo_id")) t->tipo = Equipos;
    else
    {
        t->tipo = Desconocido;
        if (error) *error = "No es un CSV del editor: falta la columna equipo_id en la primera fila.";
        return false;
    }
    return true;
}

namespace {
struct Origen { int equipo = -1, jugador = -1; };

// Para cada fila, de dónde sale el jugador (o -1 si es nuevo)
QList<Origen> localizar(const QList<EQUIPO> &equipos, const Tabla &t, const QHash<quint32, int> &porId)
{
    QHash<int, QList<Origen>> porPuntero;
    for (int e = 0; e < equipos.size(); e++)
        for (int k = 0; k < equipos.at(e).lista_jugadores.size(); k++)
            porPuntero[equipos.at(e).lista_jugadores.at(k).puntero].append({e, k});
    QSet<QPair<int, int>> usados;
    QList<Origen> r;
    for (int i = 0; i < t.filas.size(); i++)
    {
        Fila f(t.cabecera, t.filas.at(i), i + 2);
        Origen o;
        qint64 pid = 0, eid = 0;
        const bool tienePid = f.tiene("jugador_id") && aEntero(f.texto("jugador_id"), &pid);
        const int destino = (f.tiene("equipo_id") && aEntero(f.texto("equipo_id"), &eid))
                                ? porId.value(static_cast<quint32>(eid), -1) : -1;
        if (tienePid && porPuntero.contains(int(pid)))
        {
            const QList<Origen> &cand = porPuntero.value(int(pid));
            // primero, en su mismo equipo; si no, el primero libre (traspaso)
            for (const Origen &c : cand)
                if (c.equipo == destino && !usados.contains({c.equipo, c.jugador})) { o = c; break; }
            if (o.equipo < 0)
                for (const Origen &c : cand)
                    if (!usados.contains({c.equipo, c.jugador})) { o = c; break; }
            if (o.equipo >= 0) usados.insert({o.equipo, o.jugador});
        }
        r << o;
    }
    return r;
}
} // namespace

QStringList jugadoresAusentes(const QList<EQUIPO> &equipos, const Tabla &t)
{
    QHash<quint32, int> porId;
    for (int e = 0; e < equipos.size(); e++) porId.insert(equipos.at(e).EquipoIdDBC, e);
    const QList<Origen> org = localizar(equipos, t, porId);
    QSet<QPair<int, int>> enCsv;
    for (const Origen &o : org) if (o.equipo >= 0) enCsv.insert({o.equipo, o.jugador});
    QStringList r;
    for (int e = 0; e < equipos.size(); e++)
        for (int k = 0; k < equipos.at(e).lista_jugadores.size(); k++)
            if (!enCsv.contains({e, k}))
                r << equipos.at(e).NombreCorto + " / " + equipos.at(e).lista_jugadores.at(k).NombreCorto;
    return r;
}

Resultado importarJugadores(QList<EQUIPO> &equipos, const QList<quint16> &otrosPunteros,
                            const Tabla &t, bool borrarAusentes, const PCFutbolTemporadaReal *real)
{
    Resultado res;
    QHash<quint32, int> porId;
    for (int e = 0; e < equipos.size(); e++) porId.insert(equipos.at(e).EquipoIdDBC, e);
    const QList<Origen> org = localizar(equipos, t, porId);
    const QList<EQUIPO> antes = equipos;

    // Plantillas nuevas: se parte de las actuales y se marca lo que se va
    QList<QList<JUGADOR>> quedan(equipos.size());
    QList<QList<bool>> seVa(equipos.size());
    for (int e = 0; e < equipos.size(); e++)
        seVa[e] = QList<bool>(equipos.at(e).lista_jugadores.size(), borrarAusentes);
    for (const Origen &o : org)
        if (o.equipo >= 0) seVa[o.equipo][o.jugador] = false;
    QHash<QPair<int, int>, int> filaDe;          // (equipo, jugador) -> fila
    for (int i = 0; i < org.size(); i++)
        if (org.at(i).equipo >= 0) filaDe.insert({org.at(i).equipo, org.at(i).jugador}, i);

    QList<quint16> punterosUsados = otrosPunteros;
    for (const EQUIPO &e : equipos)
        for (const JUGADOR &j : e.lista_jugadores) punterosUsados << j.puntero;

    // Destino de cada fila
    QList<int> destinoDe(t.filas.size(), -1);
    for (int i = 0; i < t.filas.size(); i++)
    {
        Fila f(t.cabecera, t.filas.at(i), i + 2);
        qint64 eid = 0;
        if (!f.tiene("equipo_id") || !aEntero(f.texto("equipo_id"), &eid) || !porId.contains(quint32(eid)))
        {
            res.avisos << QString("Fila %1: equipo_id «%2» no existe; fila ignorada")
                              .arg(i + 2).arg(f.texto("equipo_id"));
            if (org.at(i).equipo >= 0) seVa[org.at(i).equipo][org.at(i).jugador] = false;
            continue;
        }
        destinoDe[i] = porId.value(quint32(eid));
        if (org.at(i).equipo >= 0 && org.at(i).equipo != destinoDe[i])
            seVa[org.at(i).equipo][org.at(i).jugador] = true;   // traspasado
    }

    // Jugadores de la temporada real: no pueden salir de su equipo
    QSet<QPair<int, int>> copias;                // se quedan sin aplicar la fila
    auto atado = [&](int e, int k) {
        return real && real->juegaPartidosReales(equipos.at(e).EquipoIdDBC,
                                                  equipos.at(e).lista_jugadores.at(k).puntero);
    };
    for (int e = 0; e < equipos.size(); e++)
    {
        for (int k = 0; k < equipos.at(e).lista_jugadores.size(); k++)
        {
            if (!seVa.at(e).at(k) || !atado(e, k)) continue;
            seVa[e][k] = false;
            const QString nj = equipos.at(e).NombreCorto + " / " + equipos.at(e).lista_jugadores.at(k).NombreCorto;
            const bool traspasado = filaDe.contains({e, k}) && destinoDe.at(filaDe.value({e, k})) >= 0;
            if (traspasado)
            {
                copias.insert({e, k});
                res.avisos << nj + ": juega partidos de la temporada real 95/96; se traspasa pero se deja "
                                   "también una copia en su equipo (si no, el juego no arranca)";
            }
            else
                res.avisos << nj + ": juega partidos de la temporada real 95/96; NO se da de baja "
                                   "(el juego no arrancaría)";
        }
    }

    // 1) Los que se quedan en su equipo, en su orden, actualizados
    for (int e = 0; e < equipos.size(); e++)
    {
        const EQUIPO &eq = equipos.at(e);
        for (int k = 0; k < eq.lista_jugadores.size(); k++)
        {
            if (seVa.at(e).at(k))
            {
                const bool traspasado = filaDe.contains({e, k}) && destinoDe.at(filaDe.value({e, k})) >= 0;
                if (!traspasado)
                {
                    res.borrados++;
                    res.detalles << "Baja: " + eq.NombreCorto + " / " + eq.lista_jugadores.at(k).NombreCorto;
                }
                continue;
            }
            JUGADOR j = eq.lista_jugadores.at(k);
            if (filaDe.contains({e, k}) && !copias.contains({e, k}))
            {
                const int i = filaDe.value({e, k});
                aplicarJugador(Fila(t.cabecera, t.filas.at(i), i + 2), j, eq.isBBDD_neg == 0, &res.avisos);
            }
            quedan[e] << j;
        }
    }
    // 2) Los que llegan (traspasos y nuevos), en el orden del CSV
    for (int i = 0; i < t.filas.size(); i++)
    {
        const int d = destinoDe.at(i);
        if (d < 0) continue;
        const Origen o = org.at(i);
        if (o.equipo == d) continue;              // ya tratado arriba
        Fila f(t.cabecera, t.filas.at(i), i + 2);
        EQUIPO tmp = equipos.at(d);
        tmp.lista_jugadores = quedan.at(d);
        if (o.equipo >= 0)
        {
            // ya está en el destino con el mismo puntero (p. ej. filial)
            const quint16 pj = equipos.at(o.equipo).lista_jugadores.at(o.jugador).puntero;
            bool ya = false;
            for (const JUGADOR &x : tmp.lista_jugadores) ya |= x.puntero == pj;
            if (ya)
            {
                res.avisos << QString("Fila %1: %2 ya está en %3; no se duplica")
                                  .arg(i + 2).arg(equipos.at(o.equipo).lista_jugadores.at(o.jugador).NombreCorto,
                                                  tmp.NombreCorto);
                continue;
            }
        }
        JUGADOR j;
        if (o.equipo >= 0)
        {
            j = equipos.at(o.equipo).lista_jugadores.at(o.jugador);
            j.EquipoIdDBC = tmp.EquipoIdDBC;
            j.PosicionEnAlineacion = static_cast<quint8>(tmp.lista_jugadores.size() + 1);
            if (!f.tiene("dorsal")) j.NumeroCamiseta = static_cast<quint8>(dorsalLibre(tmp, j.NumeroCamiseta));
            res.traspasos++;
            res.detalles << QString("Traspaso: %1: %2 → %3").arg(j.NombreCorto,
                                equipos.at(o.equipo).NombreCorto, tmp.NombreCorto);
        }
        else
        {
            qint64 pid = 0;
            quint16 p;
            if (f.tiene("jugador_id") && aEntero(f.texto("jugador_id"), &pid) && pid > 0 && pid < 65536
                && !punterosUsados.contains(quint16(pid)))
                p = quint16(pid);
            else
            {
                QList<EQUIPO> vacio;
                p = punteroLibre(vacio, punterosUsados);
            }
            punterosUsados << p;
            j = jugadorNuevo(tmp, p);
            res.nuevos++;
        }
        aplicarJugador(f, j, tmp.isBBDD_neg == 0, &res.avisos);
        // Dorsal ocupado en el nuevo equipo (lo normal al traspasar sin
        // tocar la columna): se le da el primero libre
        for (const JUGADOR &otro : tmp.lista_jugadores)
        {
            if (otro.NumeroCamiseta == j.NumeroCamiseta)
            {
                const int libre = dorsalLibre(tmp, j.NumeroCamiseta);
                res.avisos << QString("Fila %1 (%2): el dorsal %3 ya lo tiene %4 en %5; pasa al %6")
                                  .arg(i + 2).arg(j.NombreCorto).arg(j.NumeroCamiseta)
                                  .arg(otro.NombreCorto, tmp.NombreCorto).arg(libre);
                j.NumeroCamiseta = static_cast<quint8>(libre);
                break;
            }
        }
        if (o.equipo < 0)
            res.detalles << QString("Alta: %1 en %2 (jugador_id %3)").arg(j.NombreCorto, tmp.NombreCorto).arg(j.puntero);
        quedan[d] << j;
    }

    // 3) Aplicar y contar modificados
    for (int e = 0; e < equipos.size(); e++)
    {
        equipos[e].lista_jugadores = quedan.at(e);
        renumerarAlineacion(equipos[e]);
    }
    QHash<QPair<quint32, int>, QString> filasAntes;
    for (const EQUIPO &eq : antes)
        for (const JUGADOR &j : eq.lista_jugadores)
            filasAntes.insert({eq.EquipoIdDBC, j.puntero}, filaJugador(eq, j));
    for (const EQUIPO &eq : equipos)
        for (const JUGADOR &j : eq.lista_jugadores)
        {
            const auto clave = qMakePair(eq.EquipoIdDBC, int(j.puntero));
            if (filasAntes.contains(clave) && filasAntes.value(clave) != filaJugador(eq, j))
                res.modificados++;
        }
    return res;
}

Resultado importarEquipos(QList<EQUIPO> &equipos, const Tabla &t)
{
    Resultado res;
    QHash<quint32, int> porId;
    for (int e = 0; e < equipos.size(); e++) porId.insert(equipos.at(e).EquipoIdDBC, e);
    for (int i = 0; i < t.filas.size(); i++)
    {
        Fila f(t.cabecera, t.filas.at(i), i + 2);
        qint64 eid = 0;
        if (!f.tiene("equipo_id") || !aEntero(f.texto("equipo_id"), &eid) || !porId.contains(quint32(eid)))
        {
            res.avisos << QString("Fila %1: equipo_id «%2» no existe; fila ignorada")
                              .arg(i + 2).arg(f.texto("equipo_id"));
            continue;
        }
        EQUIPO &eq = equipos[porId.value(quint32(eid))];
        const QString antes = filaEquipo(eq);
        Aplicador a(f, eq.NombreCorto, &res.avisos);
        a.texto("nombre_corto", eq.NombreCorto);
        a.texto("nombre_largo", eq.NombreLargo);
        a.texto("nombre_mayusculas", eq.NombreMayusculas);
        a.texto("nombre_corto_mayusculas", eq.nombre_corto_mayusculas);
        a.texto("estadio", eq.estadio);
        a.numero("aforo", eq.aforo, 0, 4000000000LL);
        if (eq.isBBDD_neg == 0)
        {
            a.numero("fundacion", eq.anyo_fundacion, 0, 65535);
            a.numero("socios", eq.num_socios, 0, 4000000000LL);
            a.texto("presidente", eq.presidente);
            a.numero("presupuesto_bbdd", eq.presupuesto_bbdd, 0, 4000000000LL);
            a.numero("presupuesto_juego", eq.presupuesto_juego, 0, 4000000000LL);
            a.texto("patrocinador", eq.patrocinador);
            a.texto("proveedor", eq.proveedor);
        }
        if (!eq.lista_entrenadores.isEmpty())
        {
            a.texto("entrenador", eq.lista_entrenadores[0].NombreCorto);
            if (eq.isBBDD_neg == 0)
                a.texto("entrenador_nombre_completo", eq.lista_entrenadores[0].NombreLargo);
        }
        if (filaEquipo(eq) != antes)
        {
            res.modificados++;
            res.detalles << "Club modificado: " + eq.NombreCorto;
        }
    }
    return res;
}

} // namespace PCFutbolCsv
