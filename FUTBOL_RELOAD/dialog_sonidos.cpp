#include "dialog_sonidos.h"
#include "pcfutbol_extras.h"
#include "util_ventanas.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

using namespace PCFutbolAudio;

enum Columna { ColNombre, ColQue, ColDuracion, ColVolumen, ColEstado, NCol };

DialogSonidos::DialogSonidos(const QString &carpetaJuego, QWidget *parent)
    : QDialog(parent), m_carpeta(PCFutbolAudio::carpetaSonidos(carpetaJuego))
{
    setWindowTitle("Sonidos del partido");
    resize(980, 640);
    QVBoxLayout *root = new QVBoxLayout(this);

    QLabel *ayuda = new QLabel(
        "Sustituye cualquier sonido por un audio tuyo (WAV, MP3, OGG, FLAC, AIFF…). Se convierte a lo "
        "único que entiende el juego (mono, 8 bits, 11.025 Hz), con filtro para que no suene metálico, "
        "y por defecto con el <b>mismo volumen que el original</b>: los ambientes del juego están "
        "grabados muy bajos porque suenan a la vez que los golpes y el silbato.<br>"
        "El juego carga los 25 sonidos al empezar cada partido; si falta uno no arranca el partido. "
        "Nada se escribe hasta «Guardar en el juego», y la primera vez se guarda copia .bak.", this);
    ayuda->setWordWrap(true);
    root->addWidget(ayuda);

    QHBoxLayout *centro = new QHBoxLayout();
    m_tabla = new QTableWidget(0, NCol, this);
    m_tabla->setHorizontalHeaderLabels({"Sonido", "Qué es", "Duración", "Volumen", "Estado"});
    m_tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabla->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabla->verticalHeader()->hide();
    m_tabla->horizontalHeader()->setSectionResizeMode(ColQue, QHeaderView::Stretch);
    m_tabla->horizontalHeader()->setSectionResizeMode(ColEstado, QHeaderView::Stretch);
    centro->addWidget(m_tabla, 1);

    QVBoxLayout *botones = new QVBoxLayout();
    auto boton = [this, botones](const QString &t, const QString &tip, void (DialogSonidos::*f)()) {
        QPushButton *b = new QPushButton(t, this);
        b->setToolTip(tip);
        connect(b, &QPushButton::clicked, this, f);
        botones->addWidget(b);
        return b;
    };
    boton("Sustituir…", "Elegir un audio para el sonido seleccionado", &DialogSonidos::sustituir);
    boton("Escuchar", "Abre el sonido (como sonará en el juego) con el reproductor del sistema",
          &DialogSonidos::escuchar);
    boton("Exportar WAV…", "Guardar el sonido seleccionado como WAV", &DialogSonidos::exportarWav);
    boton("Volver al original", "Deshacer el cambio, o recuperar el sonido original del .bak",
          &DialogSonidos::volverAlOriginal);
    botones->addSpacing(16);
    boton("Convertir un archivo…", "Convertir cualquier audio a .RAW del juego y guardarlo donde quieras",
          &DialogSonidos::convertirSuelto);
    botones->addStretch();
    centro->addLayout(botones);
    root->addLayout(centro, 1);

    QGroupBox *grupo = new QGroupBox("Conversión (se aplica al sustituir; cambiarla reconvierte los seleccionados)", this);
    QFormLayout *form = new QFormLayout(grupo);
    m_volumen = new QComboBox(this);
    m_volumen->addItem("Igual que el sonido original (recomendado)", VolumenComoOriginal);
    m_volumen->addItem("Lo más alto posible sin distorsionar", VolumenNormalizar);
    m_volumen->addItem("Sin cambiar", VolumenSinCambio);
    m_ajuste = new QDoubleSpinBox(this);
    m_ajuste->setRange(-18, 12);
    m_ajuste->setSingleStep(1);
    m_ajuste->setSuffix(" dB");
    m_ajuste->setToolTip("Sube o baja el volumen sobre lo anterior (+6 dB = el doble de fuerte)");
    m_maximo = new QDoubleSpinBox(this);
    m_maximo->setRange(0, 600);
    m_maximo->setDecimals(1);
    m_maximo->setSuffix(" s");
    m_maximo->setSpecialValueText("sin límite");
    m_maximo->setToolTip("Cortar el audio a esta duración (con un fundido al final)");
    m_recortar = new QCheckBox("Quitar el silencio del principio y del final", this);
    m_recortar->setChecked(true);
    m_fundidos = new QCheckBox("Entrada y salida suaves (evita chasquidos)", this);
    m_fundidos->setChecked(true);
    form->addRow("Volumen:", m_volumen);
    form->addRow("Ajuste fino:", m_ajuste);
    form->addRow("Duración máxima:", m_maximo);
    form->addRow(m_recortar);
    form->addRow(m_fundidos);
    root->addWidget(grupo);

    auto alCambiar = [this]() {
        for (QTableWidgetItem *it : m_tabla->selectedItems())
            if (it->column() == 0) reconvertir(it->text());
        actualizarEstado();
    };
    connect(m_volumen, &QComboBox::currentIndexChanged, this, alCambiar);
    connect(m_ajuste, &QDoubleSpinBox::editingFinished, this, alCambiar);
    connect(m_maximo, &QDoubleSpinBox::editingFinished, this, alCambiar);
    connect(m_recortar, &QCheckBox::toggled, this, alCambiar);
    connect(m_fundidos, &QCheckBox::toggled, this, alCambiar);

    m_estado = new QLabel(this);
    m_estado->setWordWrap(true);
    root->addWidget(m_estado);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    bb->button(QDialogButtonBox::Save)->setText("Guardar en el juego");
    bb->button(QDialogButtonBox::Close)->setText("Cerrar");
    connect(bb, &QDialogButtonBox::accepted, this, &DialogSonidos::guardar);
    connect(bb, &QDialogButtonBox::rejected, this, [this]() {
        if (!m_pendientes.isEmpty() &&
            QMessageBox::question(this, "Sonidos", QString("Hay %1 cambio(s) sin guardar. ¿Cerrar sin guardarlos?")
                                                       .arg(m_pendientes.size()),
                                  QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            return;
        reject();
    });
    root->addWidget(bb);
    connect(m_tabla, &QTableWidget::cellDoubleClicked, this, [this]() { sustituir(); });

    if (!valido()) return;
    llenarTabla();
    actualizarEstado();
    UtilVentanas::hacerDesplazable(this);
}

QString DialogSonidos::nombreFila(int fila) const
{
    QTableWidgetItem *it = m_tabla->item(fila, ColNombre);
    return it ? it->text() : QString();
}

QByteArray DialogSonidos::rawActual(const QString &nombre) const
{
    if (m_pendientes.contains(nombre)) return m_pendientes.value(nombre).raw;
    QFile f(rutaRaw(m_carpeta, nombre));
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

void DialogSonidos::llenarTabla()
{
    const QList<SonidoJuego> l = sonidosDelJuego();
    m_tabla->setRowCount(l.size());
    for (int i = 0; i < l.size(); i++)
    {
        m_tabla->setItem(i, ColNombre, new QTableWidgetItem(l[i].nombre));
        m_tabla->setItem(i, ColQue, new QTableWidgetItem(l[i].descripcion));
        for (int c = ColDuracion; c < NCol; c++) m_tabla->setItem(i, c, new QTableWidgetItem());
        actualizarFila(i);
    }
    m_tabla->resizeColumnToContents(ColNombre);
    m_tabla->resizeColumnToContents(ColDuracion);
    m_tabla->resizeColumnToContents(ColVolumen);
    m_tabla->selectRow(0);
}

void DialogSonidos::actualizarFila(int fila)
{
    const QString nombre = nombreFila(fila);
    const QString ruta = rutaRaw(m_carpeta, nombre);
    const QByteArray raw = rawActual(nombre);
    const bool existe = QFile::exists(ruta);
    m_tabla->item(fila, ColDuracion)->setText(raw.isEmpty() && !existe ? "—"
                                              : QString("%1 s").arg(duracion(raw), 0, 'f', 2));
    m_tabla->item(fila, ColVolumen)->setText(raw.isEmpty() ? "—" : QString::number(rms(raw), 'f', 1));
    m_tabla->item(fila, ColVolumen)->setToolTip(
        QString("Volumen medio (RMS, de 0 a 128). El original: %1").arg(rms(leerOriginal(ruta)), 0, 'f', 1));

    QString estado;
    QColor color;
    if (m_pendientes.contains(nombre))
    {
        const Pendiente &p = m_pendientes[nombre];
        if (p.restaurar) estado = "Sin guardar: volver al original";
        else
        {
            estado = QString("Sin guardar: %1 (%2, %3 s → %4 s, %5%6 dB)")
                         .arg(QFileInfo(p.origen).fileName(), p.audio.formato)
                         .arg(p.informe.duracionEntrada, 0, 'f', 1)
                         .arg(p.informe.duracionSalida, 0, 'f', 1)
                         .arg(p.informe.gananciaDb >= 0 ? "+" : "")
                         .arg(p.informe.gananciaDb, 0, 'f', 1);
            if (p.informe.copiaExacta) estado = "Sin guardar: " + QFileInfo(p.origen).fileName() + " (copiado tal cual)";
            if (!p.informe.avisos.isEmpty()) estado += " — " + p.informe.avisos.join(" ");
        }
        color = QColor(0xe0, 0xb3, 0x4a);
    }
    else if (!existe)
    {
        estado = "¡FALTA! El partido no arranca sin él";
        color = QColor(0xe0, 0x5a, 0x4a);
    }
    else if (QFile::exists(ruta + ".bak"))
    {
        QFile a(ruta), b(ruta + ".bak");
        const bool igual = a.open(QIODevice::ReadOnly) && b.open(QIODevice::ReadOnly) && a.readAll() == b.readAll();
        estado = igual ? "Original" : "Cambiado (hay copia del original)";
    }
    else estado = "Original";
    if (!raw.isEmpty() && raw.size() < 4) estado += " — demasiado corto para sonar";
    QTableWidgetItem *it = m_tabla->item(fila, ColEstado);
    it->setText(estado);
    it->setToolTip(estado);
    it->setForeground(color.isValid() ? QBrush(color) : QBrush());
}

void DialogSonidos::actualizarEstado()
{
    qint64 total = 0;
    QStringList faltan;
    for (const SonidoJuego &s : sonidosDelJuego())
    {
        const QByteArray r = rawActual(s.nombre);
        if (r.isEmpty() && !QFile::exists(rutaRaw(m_carpeta, s.nombre))) faltan << s.nombre;
        total += r.size();
    }
    QString t = QString("Ocupan %1 KB en total (los originales, %2 KB).")
                    .arg(total / 1024).arg(TOTAL_ORIGINAL / 1024);
    if (total > TOTAL_AVISO)
        t += QString(" <span style='color:#e0b34a'>⚠ Es mucho: el juego los carga todos en memoria al empezar "
                     "el partido y, si no caben, se para («No se pudo alocar la memoria para el sonido»). "
                     "Acorta los más largos (duración máxima).</span>");
    if (!faltan.isEmpty())
        t += QString(" <span style='color:#e05a4a'>⚠ Faltan %1: el partido no arrancará. "
                     "Usa «Volver al original» o sustitúyelos.</span>").arg(faltan.join(", "));
    if (!m_pendientes.isEmpty()) t += QString(" · %1 cambio(s) sin guardar.").arg(m_pendientes.size());
    m_estado->setText(t);
}

Opciones DialogSonidos::opciones(const QString &nombre) const
{
    Opciones op;
    op.volumen = Volumen(m_volumen->currentData().toInt());
    op.rmsReferencia = rms(leerOriginal(rutaRaw(m_carpeta, nombre)));
    op.ajusteDb = m_ajuste->value();
    op.recortarSilencio = m_recortar->isChecked();
    op.duracionMaxima = m_maximo->value();
    op.fundidos = m_fundidos->isChecked();
    return op;
}

void DialogSonidos::reconvertir(const QString &nombre)
{
    if (!m_pendientes.contains(nombre) || m_pendientes[nombre].restaurar) return;
    Pendiente &p = m_pendientes[nombre];
    p.raw = convertir(p.audio, opciones(nombre), &p.informe);
    for (int i = 0; i < m_tabla->rowCount(); i++)
        if (nombreFila(i) == nombre) actualizarFila(i);
}

void DialogSonidos::sustituir()
{
    const int fila = m_tabla->currentRow();
    if (fila < 0) return;
    const QString nombre = nombreFila(fila);
    const QString f = QFileDialog::getOpenFileName(this, "Audio para " + nombre, QString(), filtroAbrir());
    if (f.isEmpty()) return;
    Pendiente p;
    QString err;
    setCursor(Qt::WaitCursor);
    const bool ok = decodificar(f, &p.audio, &err);
    unsetCursor();
    if (!ok)
    {
        QMessageBox::warning(this, "Sonidos", QFileInfo(f).fileName() + ": " + err);
        return;
    }
    p.origen = f;
    setCursor(Qt::WaitCursor);
    p.raw = convertir(p.audio, opciones(nombre), &p.informe);
    unsetCursor();
    m_pendientes.insert(nombre, p);
    actualizarFila(fila);
    actualizarEstado();
}

void DialogSonidos::escuchar()
{
    const int fila = m_tabla->currentRow();
    if (fila < 0 || !m_tmp.isValid()) return;
    const QString nombre = nombreFila(fila);
    const QByteArray raw = rawActual(nombre);
    if (raw.isEmpty()) return;
    // Nombre distinto cada vez: algunos reproductores no recargan el mismo fichero
    static int n = 0;
    const QString ruta = m_tmp.filePath(QString("%1_%2.wav").arg(nombre).arg(++n));
    QFile o(ruta);
    if (!o.open(QIODevice::WriteOnly)) return;
    o.write(PCFutbolExtras::rawAWav(raw));
    o.close();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(ruta)))
        QMessageBox::information(this, "Sonidos", "No hay reproductor de audio asociado a .wav. El fichero está en:\n" + ruta);
}

void DialogSonidos::exportarWav()
{
    const int fila = m_tabla->currentRow();
    if (fila < 0) return;
    const QString nombre = nombreFila(fila);
    const QByteArray raw = rawActual(nombre);
    if (raw.isEmpty()) return;
    const QString f = QFileDialog::getSaveFileName(this, "Exportar " + nombre, nombre + ".wav", "WAV (*.wav)");
    if (f.isEmpty()) return;
    QFile o(f);
    if (!o.open(QIODevice::WriteOnly) || o.write(PCFutbolExtras::rawAWav(raw)) < 0)
        QMessageBox::warning(this, "Sonidos", "No se pudo escribir " + f);
}

void DialogSonidos::volverAlOriginal()
{
    for (QTableWidgetItem *it : m_tabla->selectedItems())
    {
        if (it->column() != ColNombre) continue;
        const QString nombre = it->text();
        const QString ruta = rutaRaw(m_carpeta, nombre);
        if (m_pendientes.contains(nombre) && !m_pendientes[nombre].restaurar)
            m_pendientes.remove(nombre);                       // sólo deshacer lo de esta ventana
        else if (QFile::exists(ruta + ".bak"))
        {
            Pendiente p;
            p.restaurar = true;
            p.raw = leerOriginal(ruta);
            m_pendientes.insert(nombre, p);
        }
        actualizarFila(it->row());
    }
    actualizarEstado();
}

void DialogSonidos::convertirSuelto()
{
    const QString f = QFileDialog::getOpenFileName(this, "Audio a convertir", QString(), filtroAbrir());
    if (f.isEmpty()) return;
    Audio a;
    QString err;
    if (!decodificar(f, &a, &err))
    {
        QMessageBox::warning(this, "Convertir", QFileInfo(f).fileName() + ": " + err);
        return;
    }
    // Sin sonido de referencia: «igual que el original» no aplica, se normaliza
    Opciones op = opciones(QString());
    op.rmsReferencia = 0;
    if (op.volumen == VolumenComoOriginal) op.volumen = VolumenNormalizar;
    Informe inf;
    const QByteArray raw = convertir(a, op, &inf);
    const QString destino = QFileDialog::getSaveFileName(
        this, "Guardar como", QFileInfo(f).completeBaseName().toUpper() + ".RAW",
        "Sonido del juego (*.RAW);;WAV del juego (8 bits, 11.025 Hz) (*.wav)");
    if (destino.isEmpty()) return;
    QFile o(destino);
    const QByteArray datos = destino.endsWith(".wav", Qt::CaseInsensitive) ? PCFutbolExtras::rawAWav(raw) : raw;
    if (!o.open(QIODevice::WriteOnly) || o.write(datos) != datos.size())
    {
        QMessageBox::warning(this, "Convertir", "No se pudo escribir " + destino);
        return;
    }
    QMessageBox::information(this, "Convertir",
        QString("%1 (%2, %3 Hz, %4 canal(es), %5 s)\n→ %6 (%7 s, %8 bytes)%9")
            .arg(QFileInfo(f).fileName(), a.formato).arg(a.frecuencia).arg(a.canales)
            .arg(inf.duracionEntrada, 0, 'f', 2).arg(QFileInfo(destino).fileName())
            .arg(inf.duracionSalida, 0, 'f', 2).arg(raw.size())
            .arg(inf.avisos.isEmpty() ? QString() : "\n\n" + inf.avisos.join("\n")));
}

void DialogSonidos::guardar()
{
    if (m_pendientes.isEmpty())
    {
        accept();
        return;
    }
    qint64 total = 0;
    for (const SonidoJuego &s : sonidosDelJuego()) total += rawActual(s.nombre).size();
    if (total > TOTAL_AVISO &&
        QMessageBox::question(this, "Sonidos",
            QString("Los sonidos ocuparían %1 KB (los originales, %2 KB). El juego los carga todos al empezar "
                    "cada partido; con tanto puede quedarse sin memoria y pararse. No está probado.\n\n"
                    "¿Guardar igualmente?").arg(total / 1024).arg(TOTAL_ORIGINAL / 1024),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QStringList errores;
    for (auto it = m_pendientes.cbegin(); it != m_pendientes.cend(); ++it)
    {
        QString err;
        if (escribirRaw(rutaRaw(m_carpeta, it.key()), it.value().raw, &err)) m_guardados++;
        else errores << err;
    }
    m_pendientes.clear();
    for (int i = 0; i < m_tabla->rowCount(); i++) actualizarFila(i);
    actualizarEstado();

    const Estado e = comprobar(m_carpeta);
    QString msg = QString("Guardados %1 sonido(s) en\n%2").arg(m_guardados).arg(m_carpeta);
    if (!errores.isEmpty()) msg += "\n\nErrores:\n" + errores.join("\n");
    if (!e.avisos.isEmpty()) msg += "\n\nAvisos:\n" + e.avisos.join("\n");
    if (errores.isEmpty() && e.avisos.isEmpty())
    {
        QMessageBox::information(this, "Sonidos", msg + "\n\nLos 25 sonidos están y el juego puede cargarlos.");
        accept();
    }
    else QMessageBox::warning(this, "Sonidos", msg);
}
