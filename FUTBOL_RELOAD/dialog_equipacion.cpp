#include "dialog_equipacion.h"
#include "util_ventanas.h"
#include "paleta_kits.h"

#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QPixmap>
#include <QIcon>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMap>
#include <QTableWidget>
#include <QHeaderView>
#include <QRegularExpression>
#include <algorithm>
#include "bbdd_futbol.h"
#include "dialog_dibujokit.h"

static inline QColor colorPaleta(int idx)
{
    if (idx < 0 || idx > 255)
    {
        return QColor(0, 0, 0);
    }
    return QColor(paleta_kits[idx][0], paleta_kits[idx][1], paleta_kits[idx][2]);
}

// ===========================================================================
// LienzoCamiseta
// ===========================================================================
LienzoCamiseta::LienzoCamiseta(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    m_textura = QByteArray(PCFutbolEquipacion::TAM_TEXTURA, char(0x7B));
    PCFutbolEquipacion::restaurarSombra(m_textura);
    setFixedSize(sizeHint());
}

QSize LienzoCamiseta::sizeHint() const
{
    return QSize(PCFutbolEquipacion::SPRITE_W * m_zoom,
                 PCFutbolEquipacion::SPRITE_H * m_zoom);
}

void LienzoCamiseta::setZoom(int z)
{
    m_zoom = qBound(6, z, 32);
    setFixedSize(sizeHint());
    updateGeometry();
    update();
}

void LienzoCamiseta::setTextura(const QByteArray &textura168)
{
    m_textura = textura168;
    m_textura.resize(PCFutbolEquipacion::TAM_TEXTURA);
    update();
}

bool LienzoCamiseta::pixelDesdeRaton(const QPoint &p, int *sx, int *sy) const
{
    const int x = p.x() / m_zoom;
    const int y = p.y() / m_zoom;
    if (x < 0 || y < 0 || x >= PCFutbolEquipacion::SPRITE_W || y >= PCFutbolEquipacion::SPRITE_H)
    {
        return false;
    }
    *sx = x;
    *sy = y;
    return true;
}

void LienzoCamiseta::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    const int Z = m_zoom;

    // Fondo tipo tablero (zona transparente del sprite)
    for (int y = 0; y < PCFutbolEquipacion::SPRITE_H; y++)
    {
        for (int x = 0; x < PCFutbolEquipacion::SPRITE_W; x++)
        {
            const bool par = ((x + y) % 2) == 0;
            p.fillRect(x * Z, y * Z, Z, Z, par ? QColor(0x35, 0x38, 0x3a) : QColor(0x2b, 0x2e, 0x30));
        }
    }

    const int posHover = (m_hoverX >= 0)
            ? PCFutbolEquipacion::posicionTextura(m_hoverX, m_hoverY) : -1;

    for (int y = 0; y < PCFutbolEquipacion::SPRITE_H; y++)
    {
        for (int x = 0; x < PCFutbolEquipacion::SPRITE_W; x++)
        {
            const int pos = PCFutbolEquipacion::posicionTextura(x, y);
            if (pos < 0)
            {
                continue;
            }
            const quint8 idx = static_cast<quint8>(m_textura.at(pos));
            p.fillRect(x * Z, y * Z, Z, Z, colorPaleta(idx));

            // Resalta los píxeles que comparten el mismo byte de textura
            if (posHover >= 0 && pos == posHover)
            {
                p.fillRect(x * Z, y * Z, Z, Z, QColor(0x86, 0xaa, 0x1c, 70));
            }
        }
    }

    if (m_rejilla && Z >= 8)
    {
        p.setPen(QColor(0, 0, 0, 70));
        for (int x = 0; x <= PCFutbolEquipacion::SPRITE_W; x++)
        {
            p.drawLine(x * Z, 0, x * Z, PCFutbolEquipacion::SPRITE_H * Z);
        }
        for (int y = 0; y <= PCFutbolEquipacion::SPRITE_H; y++)
        {
            p.drawLine(0, y * Z, PCFutbolEquipacion::SPRITE_W * Z, y * Z);
        }
    }

    if (m_hoverX >= 0 && posHover >= 0)
    {
        p.setPen(QPen(QColor(0x86, 0xaa, 0x1c), 2));
        p.drawRect(m_hoverX * Z + 1, m_hoverY * Z + 1, Z - 2, Z - 2);
    }
}

void LienzoCamiseta::mousePressEvent(QMouseEvent *ev)
{
    int sx = 0, sy = 0;
    if (!pixelDesdeRaton(ev->pos(), &sx, &sy))
    {
        return;
    }

    if (ev->button() == Qt::RightButton)
    {
        const int c = PCFutbolEquipacion::colorEnPixel(m_textura, sx, sy);
        if (c >= 0)
        {
            m_colorActual = c;
            emit colorCogido(c);
        }
        return;
    }

    if (ev->button() == Qt::LeftButton)
    {
        emit trazoIniciado();
        if (PCFutbolEquipacion::pintarPixel(m_textura, sx, sy,
                                            static_cast<quint8>(m_colorActual),
                                            m_protegerSombra))
        {
            update();
            emit texturaCambiada();
        }
    }
}

void LienzoCamiseta::mouseMoveEvent(QMouseEvent *ev)
{
    int sx = 0, sy = 0;
    if (!pixelDesdeRaton(ev->pos(), &sx, &sy))
    {
        return;
    }

    if (sx != m_hoverX || sy != m_hoverY)
    {
        m_hoverX = sx;
        m_hoverY = sy;
        emit pixelSobrevolado(sx, sy, PCFutbolEquipacion::posicionTextura(sx, sy));
        update();
    }

    if (ev->buttons() & Qt::LeftButton)
    {
        if (PCFutbolEquipacion::pintarPixel(m_textura, sx, sy,
                                            static_cast<quint8>(m_colorActual),
                                            m_protegerSombra))
        {
            update();
            emit texturaCambiada();
        }
    }
}

void LienzoCamiseta::leaveEvent(QEvent *)
{
    m_hoverX = -1;
    m_hoverY = -1;
    update();
}

// ===========================================================================
// PaletaCamiseta
// ===========================================================================
static const int CELDA_PALETA = 18;

PaletaCamiseta::PaletaCamiseta(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    const QList<PCFutbolEquipacion::ColorEquipacion> cols =
            PCFutbolEquipacion::coloresDisponibles();
    for (const PCFutbolEquipacion::ColorEquipacion &c : cols)
    {
        QVector<int> fila;
        for (quint8 v : c.rampa)
        {
            fila.append(v);
        }
        m_filas.append(fila);
        m_nombres.append(c.nombre);
    }
    setFixedSize(sizeHint());
}

QSize PaletaCamiseta::sizeHint() const
{
    if (m_completo)
    {
        return QSize(16 * CELDA_PALETA + 1, 16 * CELDA_PALETA + 1);
    }
    return QSize(8 * CELDA_PALETA + 1, m_filas.size() * CELDA_PALETA + 1);
}

void PaletaCamiseta::setModoCompleto(bool v)
{
    m_completo = v;
    setFixedSize(sizeHint());
    updateGeometry();
    update();
}

void PaletaCamiseta::seleccionar(int idxPaleta)
{
    m_seleccionado = idxPaleta;
    update();
}

int PaletaCamiseta::celdaEn(const QPoint &p) const
{
    const int cx = p.x() / CELDA_PALETA;
    const int cy = p.y() / CELDA_PALETA;
    if (cx < 0 || cy < 0)
    {
        return -1;
    }
    if (m_completo)
    {
        if (cx >= 16 || cy >= 16)
        {
            return -1;
        }
        return cy * 16 + cx;
    }
    if (cy >= m_filas.size() || cx >= m_filas.at(cy).size())
    {
        return -1;
    }
    return m_filas.at(cy).at(cx);
}

void PaletaCamiseta::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(0x20, 0x23, 0x25));

    const int filas = m_completo ? 16 : m_filas.size();
    for (int f = 0; f < filas; f++)
    {
        const int cols = m_completo ? 16 : m_filas.at(f).size();
        for (int c = 0; c < cols; c++)
        {
            const int idx = m_completo ? (f * 16 + c) : m_filas.at(f).at(c);
            const QRect r(c * CELDA_PALETA, f * CELDA_PALETA, CELDA_PALETA, CELDA_PALETA);
            p.fillRect(r.adjusted(1, 1, -1, -1), colorPaleta(idx));
            if (idx == m_seleccionado)
            {
                p.setPen(QPen(QColor(0x86, 0xaa, 0x1c), 2));
                p.drawRect(r.adjusted(1, 1, -2, -2));
            }
        }
    }
}

void PaletaCamiseta::mousePressEvent(QMouseEvent *ev)
{
    const int idx = celdaEn(ev->pos());
    if (idx >= 0)
    {
        m_seleccionado = idx;
        update();
        emit colorElegido(idx);
    }
}

void PaletaCamiseta::mouseMoveEvent(QMouseEvent *ev)
{
    const int idx = celdaEn(ev->pos());
    if (idx >= 0)
    {
        setToolTip(QString("Índice 0x%1 (%2)")
                   .arg(idx, 2, 16, QChar('0')).arg(idx).toUpper());
    }
    else
    {
        setToolTip(QString());
    }
}

// ===========================================================================
// DialogEquipacion
// ===========================================================================
DialogEquipacion::DialogEquipacion(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Editar equipación (camiseta, pantalón, medias y botas)");
    // Botones de maximizar/minimizar: en Linux los QDialog no los traen
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
    setModal(true);
    colores = PCFutbolEquipacion::coloresDisponibles();

    for (int i = 0; i < 2; i++)
    {
        m_tex[i] = QByteArray(PCFutbolEquipacion::TAM_TEXTURA, char(0x7B));
        PCFutbolEquipacion::restaurarSombra(m_tex[i]);
        m_texOrig[i] = m_tex[i];
    }

    // --- Ruta y número de equipo -------------------------------------------
    QGroupBox *gbRuta = new QGroupBox("Ubicación en el juego", this);
    QFormLayout *formRuta = new QFormLayout();

    QHBoxLayout *filaRuta = new QHBoxLayout();
    le_ruta = new QLineEdit(this);
    le_ruta->setPlaceholderText("Ruta a PALETAS/EQUIPOS.DAT...");
    QPushButton *btnBuscarRuta = new QPushButton("Buscar...", this);
    filaRuta->addWidget(le_ruta);
    filaRuta->addWidget(btnBuscarRuta);
    formRuta->addRow("EQUIPOS.DAT:", filaRuta);

    QHBoxLayout *filaNum = new QHBoxLayout();
    sp_numero = new QSpinBox(this);
    sp_numero->setRange(1, 300);
    QPushButton *btnLeer = new QPushButton("Leer equipación del fichero", this);
    filaNum->addWidget(sp_numero);
    filaNum->addWidget(btnLeer);
    filaNum->addStretch();
    formRuta->addRow("Número de equipo (n):", filaNum);
    gbRuta->setLayout(formRuta);

    QLabel *lblAviso = new QLabel(
        "⚠️ El número de equipo (n) es la posición del equipo entre los ficheros "
        "EQ*.DBC ordenados, sin contar Estrellas Mundiales. Si no es exacto, se "
        "puede sobrescribir la equipación de OTRO equipo.", this);
    lblAviso->setWordWrap(true);
    lblAviso->setStyleSheet("color:#e0b34a;");

    // --- Lienzo -------------------------------------------------------------
    m_cbEquipacion = new QComboBox(this);
    m_cbEquipacion->addItem("1ª equipación");
    m_cbEquipacion->addItem("2ª equipación");

    m_lienzo = new LienzoCamiseta(this);
    m_lienzo->setZoom(16);

    m_lblInfo = new QLabel("Clic izq.: pintar · Clic der.: cuentagotas · "
                           "Arrastrar: pintar seguido · Ctrl+Z: deshacer", this);
    m_lblInfo->setWordWrap(true);
    m_lblInfo->setMinimumWidth(340);
    m_lblInfo->setStyleSheet("color:#9fb0b8;");

    m_spZoom = new QSpinBox(this);
    m_spZoom->setRange(6, 32);
    m_spZoom->setValue(16);
    m_spZoom->setSuffix(" px");

    m_chkRejilla = new QCheckBox("Rejilla", this);
    m_chkRejilla->setChecked(true);

    m_chkSombra = new QCheckBox("Proteger botas/contorno al pintar el icono", this);
    m_chkSombra->setChecked(true);
    m_chkSombra->setToolTip("El byte 7 de la textura es el color principal de las BOTAS "
                            "en el partido, y el icono lo usa además como contorno "
                            "(0x60 = negro en los 470 equipos originales). Para "
                            "cambiar las botas usa la zona \"Botas\".");

    QHBoxLayout *filaOpts = new QHBoxLayout();
    filaOpts->addWidget(new QLabel("Zoom:", this));
    filaOpts->addWidget(m_spZoom);
    filaOpts->addWidget(m_chkRejilla);
    filaOpts->addStretch();

    QVBoxLayout *colIzq = new QVBoxLayout();
    colIzq->addWidget(m_cbEquipacion);
    colIzq->addWidget(m_lienzo, 0, Qt::AlignCenter);
    colIzq->addLayout(filaOpts);
    colIzq->addWidget(m_chkSombra);
    colIzq->addWidget(m_lblInfo);

    QGroupBox *gbPartido = new QGroupBox("Vista en el partido", this);
    QVBoxLayout *lpart = new QVBoxLayout(gbPartido);
    m_lblJugador = new QLabel(this);
    m_lblJugador->setAlignment(Qt::AlignCenter);
    m_lblJugador->setMinimumHeight(130);
    m_lblJugador->setStyleSheet("background:#2f6b34;");
    lpart->addWidget(m_lblJugador);
    colIzq->addWidget(gbPartido);
    colIzq->addStretch();

    // --- Paleta -------------------------------------------------------------
    m_paleta = new PaletaCamiseta(this);
    m_chkPaletaCompleta = new QCheckBox("Paleta completa (256 colores)", this);
    m_lblColor = new QLabel(this);
    m_lblColor->setFixedHeight(26);
    m_lblColor->setAutoFillBackground(true);

    QGroupBox *gbPaleta = new QGroupBox("Color", this);
    QVBoxLayout *lp = new QVBoxLayout(gbPaleta);
    lp->addWidget(m_paleta, 0, Qt::AlignLeft);
    lp->addWidget(m_chkPaletaCompleta);
    lp->addWidget(m_lblColor);

    // --- Patrones -----------------------------------------------------------
    m_cbPatron = new QComboBox(this);
    m_cbPatron->addItem("Liso");
    m_cbPatron->addItem("Rayas verticales");
    m_cbPatron->addItem("Franjas horizontales");

    m_cbColorA = new QComboBox(this);
    m_cbColorB = new QComboBox(this);
    for (const PCFutbolEquipacion::ColorEquipacion &c : colores)
    {
        QPixmap pm(14, 14);
        pm.fill(colorPaleta(c.rampa.at(c.rampa.size() / 2)));
        m_cbColorA->addItem(QIcon(pm), c.nombre);
        m_cbColorB->addItem(QIcon(pm), c.nombre);
    }
    if (m_cbColorB->count() > 2)
    {
        m_cbColorB->setCurrentIndex(2); // Blanco
    }

    m_spAncho = new QSpinBox(this);
    m_spAncho->setRange(1, 12);
    m_spAncho->setValue(4);
    m_spAncho->setToolTip("Ancho de cada raya en la textura. 4 = igual que los equipos originales.");

    QPushButton *btnPatron = new QPushButton("Aplicar patrón", this);

    QGroupBox *gbPatron = new QGroupBox("Patrón predefinido", this);
    QFormLayout *fp = new QFormLayout(gbPatron);
    fp->addRow("Patrón:", m_cbPatron);
    fp->addRow("Color A:", m_cbColorA);
    fp->addRow("Color B:", m_cbColorB);
    fp->addRow("Ancho de raya:", m_spAncho);
    fp->addRow(btnPatron);

    // --- Zonas --------------------------------------------------------------
    m_cbZona = new QComboBox(this);
    m_cbZona->addItem("Cuello");
    m_cbZona->addItem("Ribete inferior");
    m_cbZona->addItem("Manga izquierda");
    m_cbZona->addItem("Manga derecha");
    m_cbZona->addItem("Ambas mangas");
    m_cbZona->addItem("Hombros");
    m_cbZona->addItem("Torso");
    m_cbZona->addItem("Faldón (parte baja)");
    m_cbZona->addItem("Toda la camiseta (icono)");
    m_cbZona->addItem("Camiseta completa (partido)");
    m_cbZona->addItem("Pantalón");
    m_cbZona->addItem("Medias");
    m_cbZona->addItem("Botas");
    m_cbZona->setCurrentIndex(10);
    m_chkDegradado = new QCheckBox("Con degradado (familia del color elegido)", this);
    m_chkDegradado->setChecked(true);
    m_chkDegradado->setToolTip("Conserva las luces y sombras: cada píxel recibe un tono "
                               "más claro u oscuro de la familia del color elegido.");
    QPushButton *btnZona = new QPushButton("Rellenar zona con el color elegido", this);

    QGroupBox *gbZona = new QGroupBox("Zonas", this);
    QVBoxLayout *lz = new QVBoxLayout(gbZona);
    lz->addWidget(m_cbZona);
    lz->addWidget(m_chkDegradado);
    lz->addWidget(btnZona);

    // --- Color para detectar choques ----------------------------------------
    m_lblColorRep = new QLabel(this);
    m_lblColorRep->setWordWrap(true);
    QPushButton *btnRepCalc = new QPushButton("Calcular desde la camiseta", this);
    QPushButton *btnRepPal = new QPushButton("Usar color de la paleta", this);
    QPushButton *btnChoques = new QPushButton("Comprobar choques con otros equipos…", this);
    btnRepCalc->setToolTip("Toma el color más frecuente del torso de la camiseta.");
    QGroupBox *gbChoque = new QGroupBox("Color para detectar choques (automático)", this);
    gbChoque->setToolTip("El juego compara este color de la 1ª equipación de los dos equipos:\n"
                         "si sus tonos distan menos de 60°, el visitante juega con la 2ª.\n"
                         "Blanco, gris y negro sólo chocan entre sí. No hay 3ª equipación.");
    QVBoxLayout *lc = new QVBoxLayout(gbChoque);
    lc->addWidget(m_lblColorRep);
    QHBoxLayout *filaRep = new QHBoxLayout();
    filaRep->addWidget(btnRepCalc);
    filaRep->addWidget(btnRepPal);
    lc->addLayout(filaRep);
    lc->addWidget(btnChoques);
    connect(btnRepCalc, &QPushButton::clicked, this, &DialogEquipacion::calcularColorRepresentativo);
    connect(btnRepPal, &QPushButton::clicked, this, &DialogEquipacion::usarColorPaletaRepresentativo);
    connect(btnChoques, &QPushButton::clicked, this, &DialogEquipacion::comprobarChoques);

    QVBoxLayout *colDer = new QVBoxLayout();
    colDer->addWidget(gbPaleta);
    colDer->addWidget(gbPatron);
    colDer->addWidget(gbZona);
    colDer->addWidget(gbChoque);
    colDer->addStretch();

    QHBoxLayout *cols = new QHBoxLayout();
    cols->addLayout(colIzq);
    cols->addLayout(colDer);

    // --- Botones ------------------------------------------------------------
    QPushButton *btnUndo    = new QPushButton("Deshacer", this);
    QPushButton *btnRestore = new QPushButton("Restaurar original", this);
    QPushButton *btnDibujo  = new QPushButton("Dibujo en la ficha del equipo…", this);
    btnDibujo->setToolTip("Dibujo de camiseta y pantalón de la Base de datos (DBDAT/CAMISAS)");
    connect(btnDibujo, &QPushButton::clicked, this, &DialogEquipacion::abrirDibujoKit);
    QPushButton *btnGuardar = new QPushButton("Guardar en EQUIPOS.DAT", this);
    QPushButton *btnCerrar  = new QPushButton("Cerrar", this);
    btnGuardar->setDefault(true);

    QHBoxLayout *filaBotones = new QHBoxLayout();
    filaBotones->addWidget(btnUndo);
    filaBotones->addWidget(btnRestore);
    filaBotones->addWidget(btnDibujo);
    filaBotones->addStretch();
    filaBotones->addWidget(btnGuardar);
    filaBotones->addWidget(btnCerrar);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->addWidget(gbRuta);
    root->addWidget(lblAviso);
    root->addLayout(cols);
    root->addLayout(filaBotones);

    // --- Conexiones ---------------------------------------------------------
    connect(btnBuscarRuta, &QPushButton::clicked, this, &DialogEquipacion::buscarRutaEquiposDat);
    connect(btnLeer, &QPushButton::clicked, this, &DialogEquipacion::leerDelFichero);
    connect(m_cbEquipacion, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DialogEquipacion::cambiarEquipacion);
    connect(m_paleta, &PaletaCamiseta::colorElegido, this, &DialogEquipacion::onColorElegido);
    connect(m_lienzo, &LienzoCamiseta::colorCogido, this, [this](int idx) {
        m_paleta->seleccionar(idx);
        onColorElegido(idx);
    });
    connect(m_lienzo, &LienzoCamiseta::trazoIniciado, this, &DialogEquipacion::onTrazoIniciado);
    connect(m_lienzo, &LienzoCamiseta::texturaCambiada, this, &DialogEquipacion::onTexturaCambiada);
    connect(m_lienzo, &LienzoCamiseta::pixelSobrevolado, this, &DialogEquipacion::onPixelSobrevolado);
    connect(m_chkPaletaCompleta, &QCheckBox::toggled, m_paleta, &PaletaCamiseta::setModoCompleto);
    connect(m_chkRejilla, &QCheckBox::toggled, m_lienzo, &LienzoCamiseta::setMostrarRejilla);
    connect(m_chkSombra, &QCheckBox::toggled, m_lienzo, &LienzoCamiseta::setProtegerSombra);
    connect(m_spZoom, QOverload<int>::of(&QSpinBox::valueChanged), m_lienzo, &LienzoCamiseta::setZoom);
    connect(btnPatron, &QPushButton::clicked, this, &DialogEquipacion::aplicarPatron);
    connect(btnZona, &QPushButton::clicked, this, &DialogEquipacion::rellenarZona);
    connect(btnUndo, &QPushButton::clicked, this, &DialogEquipacion::deshacer);
    connect(btnRestore, &QPushButton::clicked, this, &DialogEquipacion::restaurarOriginal);
    connect(btnGuardar, &QPushButton::clicked, this, &DialogEquipacion::guardarEnDisco);
    connect(btnCerrar, &QPushButton::clicked, this, &QDialog::close);
    new QShortcut(QKeySequence::Undo, this, SLOT(deshacer()));

    m_paleta->seleccionar(0x80);
    onColorElegido(0x80);
    refrescar();

    // Que la ventana quepa en pantallas pequeñas (contenido con barras de desplazamiento)
    UtilVentanas::hacerDesplazable(this);
}

// --- API pública ------------------------------------------------------------
void DialogEquipacion::setRutaEquiposDat(const QString &ruta)
{
    le_ruta->setText(ruta);
    cargarSpritesPartido();
    refrescar();
}

void DialogEquipacion::setNumeroEquipo(int n)
{
    if (n > 0)
    {
        sp_numero->setValue(n);
    }
}

void DialogEquipacion::mostrarTexturaActual(bool primera, const QByteArray &textura168)
{
    if (textura168.size() < PCFutbolEquipacion::TAM_TEXTURA)
    {
        return;
    }
    const int i = primera ? 0 : 1;
    m_tex[i] = textura168;
    m_texOrig[i] = textura168;
    m_undo[i].clear();
    refrescar();
}

QByteArray DialogEquipacion::texturaEditada(bool primera) const
{
    return m_tex[primera ? 0 : 1];
}

// --- Slots ------------------------------------------------------------------
void DialogEquipacion::onColorElegido(int idxPaleta)
{
    m_lienzo->setColorActual(idxPaleta);
    QPalette pal = m_lblColor->palette();
    pal.setColor(QPalette::Window, colorPaleta(idxPaleta));
    m_lblColor->setPalette(pal);
    m_lblColor->setText(QString("  Color 0x%1")
                        .arg(idxPaleta, 2, 16, QChar('0')).toUpper());
}

void DialogEquipacion::refrescar()
{
    m_lienzo->setTextura(m_tex[m_actual]);
    if (m_lblJugador)
    {
        const QImage im = vistaPartido(3);
        if (im.isNull())
            m_lblJugador->setText("Indica la ruta de EQUIPOS.DAT\n(se usan los sprites de GRAFICOS)");
        else
            m_lblJugador->setPixmap(QPixmap::fromImage(im));
    }
    actualizarColorRepresentativo();
}

// --- Vista del jugador en el partido --------------------------------------
// Carpeta del juego = carpeta que contiene PALETAS/EQUIPOS.DAT
void DialogEquipacion::cargarSpritesPartido()
{
    m_palGraficos.clear();
    m_empDatos.clear();
    m_empFrames.clear();
    m_empFichero.clear();
    if (le_ruta->text().isEmpty()) return;

    QDir juego = QFileInfo(le_ruta->text()).absoluteDir();
    juego.cdUp();
    bool ok = false;
    m_palGraficos = PCFutbolExtras::leerPaletaBBM(juego.filePath("GRAFICOS/PALETA.BBM"), &ok);
    if (!ok || m_palGraficos.size() < 256)
    {
        m_palGraficos.clear();
        return;
    }
    struct Pose { const char *fichero; int frame; };
    static const Pose poses[] = { {"PASE.EMP", 2}, {"CORRER.EMP", 2}, {"CORRER.EMP", 18},
                                  {"CHUT.EMP", 4}, {"ENTRADA.EMP", 3} };
    QMap<QString, int> cargados;
    for (const Pose &po : poses)
    {
        const QString nombre = QString::fromLatin1(po.fichero);
        if (!cargados.contains(nombre))
        {
            QFile f(juego.filePath("GRAFICOS/" + nombre));
            if (!f.open(QIODevice::ReadOnly)) continue;
            m_empDatos.append(f.readAll());
            cargados[nombre] = m_empDatos.size() - 1;
        }
        const int iDat = cargados[nombre];
        const QVector<PCFutbolExtras::FrameEMP> fr = PCFutbolExtras::parsearEMP(m_empDatos.at(iDat));
        if (fr.isEmpty()) continue;
        m_empFrames.append(fr.at(qMin(po.frame, int(fr.size()) - 1)));
        m_empFichero.append(iDat);
    }
}

QImage DialogEquipacion::vistaPartido(int escala) const
{
    if (m_palGraficos.size() < 256 || m_empFrames.isEmpty()) return QImage();

    const QByteArray lut = PCFutbolEquipacion::tablaRecoloreado(m_tex[m_actual], m_bloqueOrig[m_actual]);
    int W = 0, H = 0;
    for (const PCFutbolExtras::FrameEMP &f : m_empFrames)
    {
        W += f.ancho + 4;
        H = qMax(H, f.alto);
    }
    QImage im(W * escala, H * escala, QImage::Format_ARGB32);
    im.fill(QColor(0x2f, 0x6b, 0x34));
    int xo = 0;
    for (int i = 0; i < m_empFrames.size(); i++)
    {
        const PCFutbolExtras::FrameEMP &f = m_empFrames.at(i);
        const QByteArray &d = m_empDatos.at(m_empFichero.at(i));
        const int yo = H - f.alto;
        for (int plano = 0; plano < 4; plano++)
        {
            const int base = f.datos + plano * f.tamPlano;
            for (int y = 0; y < f.alto; y++)
            {
                for (int xb = 0; xb < f.bytesPorFila; xb++)
                {
                    const quint8 idx = static_cast<quint8>(d.at(base + y * f.bytesPorFila + xb));
                    if (idx == 0) continue;
                    const int x = xb * 4 + plano;
                    if (x >= f.ancho) continue;
                    const quint8 c = (idx < lut.size()) ? static_cast<quint8>(lut.at(idx)) : idx;
                    const QRgb rgb = m_palGraficos.at(c) | 0xFF000000u;
                    for (int dy = 0; dy < escala; dy++)
                        for (int dx = 0; dx < escala; dx++)
                            im.setPixel((xo + x) * escala + dx, (yo + y) * escala + dy, rgb);
                }
            }
        }
        xo += f.ancho + 4;
    }
    return im;
}

void DialogEquipacion::empujarUndo()
{
    m_undo[m_actual].append(m_tex[m_actual]);
    if (m_undo[m_actual].size() > 100)
    {
        m_undo[m_actual].removeFirst();
    }
}

void DialogEquipacion::cambiarEquipacion(int idx)
{
    m_tex[m_actual] = m_lienzo->textura();
    m_actual = qBound(0, idx, 1);
    refrescar();
}

void DialogEquipacion::onTrazoIniciado()
{
    m_tex[m_actual] = m_lienzo->textura();
    empujarUndo();
}

void DialogEquipacion::onTexturaCambiada()
{
    m_tex[m_actual] = m_lienzo->textura();
}

void DialogEquipacion::onPixelSobrevolado(int sx, int sy, int pos)
{
    if (pos < 0)
    {
        m_lblInfo->setText("Fondo (fuera de la camiseta)");
        return;
    }
    const int color = PCFutbolEquipacion::colorEnPixel(m_tex[m_actual], sx, sy);
    m_lblInfo->setText(QString("Píxel (%1,%2) → byte %3 de la textura (fila %4, col %5) · "
                               "zona: %6 · color 0x%7")
                       .arg(sx).arg(sy).arg(pos)
                       .arg(pos / PCFutbolEquipacion::ANCHO)
                       .arg(pos % PCFutbolEquipacion::ANCHO)
                       .arg(nombreZona(pos))
                       .arg(color, 2, 16, QChar('0')));
}

QString DialogEquipacion::nombreZona(int pos) const
{
    struct Entrada { PCFutbolEquipacion::Zona z; const char *n; };
    static const Entrada tabla[] = {
        { PCFutbolEquipacion::Zona::Botas,          "BOTAS (y contorno del icono)" },
        { PCFutbolEquipacion::Zona::Pantalon,       "pantalón" },
        { PCFutbolEquipacion::Zona::Medias,         "medias" },
        { PCFutbolEquipacion::Zona::Pelo,           "pelo (fijo)" },
        { PCFutbolEquipacion::Zona::Cuello,         "cuello" },
        { PCFutbolEquipacion::Zona::Ribete,         "ribete inferior" },
        { PCFutbolEquipacion::Zona::MangaIzquierda, "manga izquierda" },
        { PCFutbolEquipacion::Zona::MangaDerecha,   "manga derecha" },
        { PCFutbolEquipacion::Zona::Hombros,        "hombros" },
        { PCFutbolEquipacion::Zona::Torso,          "torso" },
        { PCFutbolEquipacion::Zona::Faldon,         "faldón" }
    };
    for (const Entrada &e : tabla)
    {
        if (PCFutbolEquipacion::posicionesZona(e.z).contains(pos))
        {
            return QString::fromUtf8(e.n);
        }
    }
    return "otra";
}

void DialogEquipacion::aplicarPatron()
{
    const int ia = qBound(0, m_cbColorA->currentIndex(), colores.size() - 1);
    const int ib = qBound(0, m_cbColorB->currentIndex(), colores.size() - 1);

    PCFutbolEquipacion::Patron patron = PCFutbolEquipacion::Patron::Liso;
    if (m_cbPatron->currentIndex() == 1)
    {
        patron = PCFutbolEquipacion::Patron::RayasVerticales;
    }
    else if (m_cbPatron->currentIndex() == 2)
    {
        patron = PCFutbolEquipacion::Patron::FranjasHorizontales;
    }

    m_tex[m_actual] = m_lienzo->textura();
    empujarUndo();
    // El patrón sólo se aplica a la camiseta: pantalón, medias, botas y pelo
    // conservan sus colores.
    const QByteArray generada = PCFutbolEquipacion::generarTextura(patron,
                                                                   colores.at(ia), colores.at(ib),
                                                                   m_spAncho->value(), true);
    PCFutbolEquipacion::copiarSoloCamiseta(m_tex[m_actual], generada);
    // Camiseta nueva: el color que usa el juego para los choques debe seguirla
    m_repManual[m_actual] = false;
    m_rep[m_actual] = PCFutbolEquipacion::colorDominanteCamiseta(m_tex[m_actual]);
    refrescar();
}

void DialogEquipacion::rellenarZona()
{
    static const PCFutbolEquipacion::Zona zonas[] = {
        PCFutbolEquipacion::Zona::Cuello,
        PCFutbolEquipacion::Zona::Ribete,
        PCFutbolEquipacion::Zona::MangaIzquierda,
        PCFutbolEquipacion::Zona::MangaDerecha,
        PCFutbolEquipacion::Zona::Mangas,
        PCFutbolEquipacion::Zona::Hombros,
        PCFutbolEquipacion::Zona::Torso,
        PCFutbolEquipacion::Zona::Faldon,
        PCFutbolEquipacion::Zona::TodoLoVisible,
        PCFutbolEquipacion::Zona::Camiseta,
        PCFutbolEquipacion::Zona::Pantalon,
        PCFutbolEquipacion::Zona::Medias,
        PCFutbolEquipacion::Zona::Botas
    };
    const int nZonas = int(sizeof(zonas) / sizeof(zonas[0]));
    const int i = qBound(0, m_cbZona->currentIndex(), nZonas - 1);
    const quint8 color = static_cast<quint8>(m_paleta->colorSeleccionado());
    // Las botas se piden explícitamente: no se protegen
    const bool proteger = (zonas[i] != PCFutbolEquipacion::Zona::Botas) && m_chkSombra->isChecked();

    m_tex[m_actual] = m_lienzo->textura();
    empujarUndo();
    const QVector<quint8> rampa = PCFutbolEquipacion::rampaDeIndice(color);
    if (m_chkDegradado->isChecked() && !rampa.isEmpty())
    {
        QByteArray t = m_tex[m_actual];
        PCFutbolEquipacion::pintarZonaRampa(t, zonas[i], rampa);
        if (proteger) t[PCFutbolEquipacion::POS_SOMBRA] = m_tex[m_actual].at(PCFutbolEquipacion::POS_SOMBRA);
        m_tex[m_actual] = t;
    }
    else
    {
        PCFutbolEquipacion::pintarZona(m_tex[m_actual], zonas[i], color, proteger);
    }
    refrescar();
}

void DialogEquipacion::deshacer()
{
    if (m_undo[m_actual].isEmpty())
    {
        return;
    }
    m_tex[m_actual] = m_undo[m_actual].takeLast();
    refrescar();
}

void DialogEquipacion::restaurarOriginal()
{
    m_tex[m_actual] = m_lienzo->textura();
    empujarUndo();
    m_tex[m_actual] = m_texOrig[m_actual];
    refrescar();
}

void DialogEquipacion::buscarRutaEquiposDat()
{
    const QString ruta = QFileDialog::getOpenFileName(
                this, "Selecciona PALETAS/EQUIPOS.DAT",
                le_ruta->text(), "EQUIPOS.DAT (EQUIPOS.DAT);;Todos los ficheros (*)");
    if (!ruta.isEmpty())
    {
        le_ruta->setText(ruta);
    }
}

void DialogEquipacion::leerDelFichero()
{
    if (le_ruta->text().isEmpty())
    {
        QMessageBox::warning(this, "Falta la ruta",
                             "Indica primero la ruta a PALETAS/EQUIPOS.DAT.");
        return;
    }

    if (m_empFrames.isEmpty()) cargarSpritesPartido();

    bool alguna = false;
    for (int i = 0; i < 2; i++)
    {
        bool ok = false;
        const QByteArray bloque = PCFutbolEquipacion::leerBloqueEquipacion(
                    le_ruta->text(), sp_numero->value(), (i == 0), &ok);
        if (ok && bloque.size() == PCFutbolEquipacion::TAM_BLOQUE)
        {
            m_tex[i] = bloque.mid(8, PCFutbolEquipacion::TAM_TEXTURA);
            m_texOrig[i] = m_tex[i];
            m_bloqueOrig[i] = bloque;
            m_undo[i].clear();
            m_rep[i] = -1;
            m_repManual[i] = false;
            alguna = true;
        }
    }

    if (!alguna)
    {
        QMessageBox::warning(this, "No se ha podido leer",
                             "No se ha podido leer el bloque de ese equipo. "
                             "Comprueba la ruta y el número de equipo.");
        return;
    }
    refrescar();
}

void DialogEquipacion::guardarEnDisco()
{
    m_tex[m_actual] = m_lienzo->textura();

    if (le_ruta->text().isEmpty())
    {
        QMessageBox::warning(this, "Falta la ruta",
                             "Indica primero la ruta a PALETAS/EQUIPOS.DAT.");
        return;
    }

    if (QMessageBox::question(this, "Confirmar guardado",
            QString("Se van a sobrescribir las DOS equipaciones del equipo n = %1.\n\n"
                    "Se hará antes una copia de seguridad (.bak) del fichero original.\n\n"
                    "¿Continuar?").arg(sp_numero->value()),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    const QString rutaBak = le_ruta->text() + ".bak";
    if (!QFile::exists(rutaBak))
    {
        if (!QFile::copy(le_ruta->text(), rutaBak))
        {
            QMessageBox::critical(this, "Error",
                                  "No se ha podido crear la copia de seguridad. "
                                  "No se ha guardado nada.");
            return;
        }
    }

    // Color para detectar choques: recalcular las dos equipaciones si hace falta
    for (int i = 0; i < 2; i++) ajustarColorRepAutomatico(i);

    bool ok = true;
    for (int i = 0; i < 2; i++)
    {
        // Sólo se reescriben los 168 bytes de textura: la cabecera y el
        // cierre (que lleva datos propios de cada equipo) se conservan.
        if (!PCFutbolEquipacion::escribirTexturaEquipacion(le_ruta->text(),
                                                           sp_numero->value(),
                                                           (i == 0), m_tex[i]))
        {
            ok = false;
        }
        // Color representativo (bytes 185 y 187-189) si se ha cambiado
        if (m_rep[i] >= 0)
        {
            if (PCFutbolEquipacion::escribirColorRepresentativo(le_ruta->text(), sp_numero->value(),
                                                                (i == 0), static_cast<quint8>(m_rep[i])))
            {
                PCFutbolEquipacion::setColorRepresentativo(m_bloqueOrig[i], static_cast<quint8>(m_rep[i]));
                m_rep[i] = -1;
            }
            else
            {
                ok = false;
            }
        }
    }

    if (ok)
    {
        m_guardado = true;
        for (int i = 0; i < 2; i++)
        {
            m_texOrig[i] = m_tex[i];
        }
        QMessageBox::information(this, "Guardado",
                                 "Equipaciones guardadas correctamente.\n"
                                 "Copia de seguridad del original en:\n" + rutaBak);
    }
    else
    {
        QMessageBox::critical(this, "Error",
                              "No se ha podido escribir alguna de las equipaciones. "
                              "Revisa la ruta y el número de equipo.\n"
                              "Tienes la copia de seguridad en:\n" + rutaBak);
    }
}

// --- Color representativo y choques de color --------------------------------
QRgb DialogEquipacion::colorRepActual(int i) const
{
    if (m_rep[i] >= 0)
    {
        return colorPaleta(m_rep[i]).rgb();
    }
    if (m_bloqueOrig[i].size() == PCFutbolEquipacion::TAM_BLOQUE)
    {
        return PCFutbolEquipacion::colorRepresentativo(m_bloqueOrig[i]);
    }
    bool ok = false;
    const QByteArray b = PCFutbolEquipacion::leerBloqueEquipacion(le_ruta->text(),
                                                                  sp_numero->value(), i == 0, &ok);
    return ok ? PCFutbolEquipacion::colorRepresentativo(b) : qRgb(0, 0, 0);
}

void DialogEquipacion::actualizarColorRepresentativo()
{
    if (!m_lblColorRep) return;
    if (le_ruta->text().isEmpty())
    {
        m_lblColorRep->setText("(indica la ruta de EQUIPOS.DAT)");
        return;
    }
    ajustarColorRepAutomatico(m_actual);
    const QRgb rep = colorRepActual(m_actual);
    int h, l, s;
    PCFutbolEquipacion::rgbAHslJuego(qRed(rep), qGreen(rep), qBlue(rep), &h, &l, &s);
    QString texto = QString("<span style='background:%1'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span>"
                            "&nbsp;RGB %2,%3,%4 · tono %5° · sat. %6%")
                        .arg(QColor(rep).name()).arg(qRed(rep)).arg(qGreen(rep)).arg(qBlue(rep))
                        .arg(h).arg(s);
    texto += m_repManual[m_actual]
        ? "<br><i>Elegido a mano: no se recalcula al cambiar la camiseta.</i>"
        : "<br><i>Automático: sigue a la camiseta (se recalcula solo si la cambias).</i>";
    if (m_rep[m_actual] >= 0) texto += " <i>Sin guardar.</i>";

    const QByteArray &t = m_tex[m_actual];
    if (m_repManual[m_actual] && !repPareceCamiseta(rep, t))
    {
        const QColor dom = colorPaleta(PCFutbolEquipacion::colorDominanteCamiseta(t));
        texto += QString("<br><span style='color:#e0b34a'>⚠ No se parece a la camiseta "
                         "(<span style='background:%1'>&nbsp;&nbsp;&nbsp;&nbsp;</span>): el juego "
                         "decidiría los choques con un color que no es el real. Pulsa "
                         "«Calcular desde la camiseta» para volver al modo automático.</span>").arg(dom.name());
    }
    m_lblColorRep->setText(texto);
}

bool DialogEquipacion::repPareceCamiseta(QRgb rep, const QByteArray &t) const
{
    // Se cuenta qué parte del torso "choca" con él (regla del juego); con un
    // 20 % basta, así una camiseta a rayas admite cualquiera de sus colores.
    int total = 0, parecidos = 0;
    for (int p = 3 * PCFutbolEquipacion::ANCHO; p < t.size(); p++)
    {
        if (!PCFutbolEquipacion::esPosicionCamiseta(p)) continue;
        total++;
        if (PCFutbolEquipacion::chocan(rep, colorPaleta(static_cast<quint8>(t.at(p))).rgb()))
            parecidos++;
    }
    return total == 0 || parecidos * 5 >= total;
}

void DialogEquipacion::ajustarColorRepAutomatico(int i)
{
    if (i == m_actual) m_tex[i] = m_lienzo->textura();
    if (m_repManual[i] || le_ruta->text().isEmpty()) return;
    // Sólo si la camiseta ha cambiado respecto a la leída: un kit sin tocar
    // conserva siempre su color (en uno de los 470 originales no se parece
    // a la camiseta, y el juego lo usa así).
    bool camisetaCambiada = false;
    for (int p = 0; p < m_tex[i].size() && p < m_texOrig[i].size(); p++)
    {
        if (PCFutbolEquipacion::esPosicionCamiseta(p) && m_tex[i].at(p) != m_texOrig[i].at(p))
        {
            camisetaCambiada = true;
            break;
        }
    }
    if (!camisetaCambiada) return;
    // Mientras el color (el del fichero o el ya calculado) siga pareciéndose a
    // la camiseta no se toca: así se respetan los colores retocados a mano de
    // los kits originales y no cambia nada si sólo se editan detalles.
    if (repPareceCamiseta(colorRepActual(i), m_tex[i])) return;
    m_rep[i] = PCFutbolEquipacion::colorDominanteCamiseta(m_tex[i]);
}

void DialogEquipacion::calcularColorRepresentativo()
{
    m_tex[m_actual] = m_lienzo->textura();
    m_repManual[m_actual] = false;
    m_rep[m_actual] = PCFutbolEquipacion::colorDominanteCamiseta(m_tex[m_actual]);
    refrescar();
}

void DialogEquipacion::usarColorPaletaRepresentativo()
{
    m_repManual[m_actual] = true;
    m_rep[m_actual] = m_paleta->colorSeleccionado();
    refrescar();
}

QStringList DialogEquipacion::ficherosEquiposOrdenados() const
{
    // Mismo orden que la tabla interna de PCMANAG/PROMANAG (verificado):
    // EQ*.DBC ordenados por ID, sin Estrellas Mundiales (EQ9599xx).
    QDir dir = QFileInfo(le_ruta->text()).absoluteDir();
    if (!dir.cdUp() || !dir.cd("DBDAT")) return QStringList();
    const QRegularExpression re("^EQ(\\d{6})\\.DBC$", QRegularExpression::CaseInsensitiveOption);
    QStringList lista;
    for (const QString &f : dir.entryList(QDir::Files))
    {
        const QRegularExpressionMatch m = re.match(f);
        if (m.hasMatch() && !m.captured(1).startsWith("9599"))
            lista << dir.absoluteFilePath(f);
    }
    std::sort(lista.begin(), lista.end(), [](const QString &a, const QString &b) {
        return QFileInfo(a).fileName().mid(2, 6).toInt() < QFileInfo(b).fileName().mid(2, 6).toInt();
    });
    return lista;
}

static QString nombreCortoEquipo(const QString &rutaDbc)
{
    QFile f(rutaDbc);
    if (!f.open(QIODevice::ReadOnly)) return QFileInfo(rutaDbc).baseName();
    const QByteArray d = f.read(0x200);
    BBDD_FUTBOL db;
    int pos = 0x2C;                                   // nombre largo, luego el corto
    if (pos >= d.size()) return QFileInfo(rutaDbc).baseName();
    pos += 1 + static_cast<quint8>(d.at(pos));
    if (pos >= d.size()) return QFileInfo(rutaDbc).baseName();
    const int len = static_cast<quint8>(d.at(pos++));
    QString n;
    for (int i = 0; i < len && pos < d.size(); i++)
        n.append(db.descodificar_caracter(static_cast<quint8>(d.at(pos++))));
    n = n.trimmed();
    return n.isEmpty() ? QFileInfo(rutaDbc).baseName() : n;
}

void DialogEquipacion::comprobarChoques()
{
    m_tex[m_actual] = m_lienzo->textura();
    QFile f(le_ruta->text());
    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Falta la ruta", "Indica primero la ruta a PALETAS/EQUIPOS.DAT.");
        return;
    }
    const QByteArray todo = f.readAll();
    f.close();
    const int nEquipos = todo.size() / (2 * PCFutbolEquipacion::TAM_BLOQUE);
    const int yo = sp_numero->value();
    const QStringList dbc = ficherosEquiposOrdenados();
    ajustarColorRepAutomatico(0);
    ajustarColorRepAutomatico(1);
    const QRgb mi1 = colorRepActual(0);
    const QRgb mi2 = colorRepActual(1);

    struct Fila { int n; QString nombre; QRgb r1, r2; bool malVisit, malLocal; };
    QList<Fila> filas;
    for (int n = 1; n <= nEquipos; n++)
    {
        if (n == yo) continue;
        const QRgb r1 = PCFutbolEquipacion::colorRepresentativo(
                    todo.mid(PCFutbolEquipacion::kitOffset(n, true), PCFutbolEquipacion::TAM_BLOQUE));
        if (!PCFutbolEquipacion::chocan(mi1, r1)) continue;
        Fila fl;
        fl.n = n;
        fl.r1 = r1;
        fl.r2 = PCFutbolEquipacion::colorRepresentativo(
                    todo.mid(PCFutbolEquipacion::kitOffset(n, false), PCFutbolEquipacion::TAM_BLOQUE));
        fl.nombre = (n - 1 < dbc.size()) ? nombreCortoEquipo(dbc.at(n - 1)) : QString("Equipo %1").arg(n);
        // Cambia el visitante: fuera usas tu 2ª; en casa el rival usa su 2ª
        fl.malVisit = PCFutbolEquipacion::chocan(mi2, r1);
        fl.malLocal = PCFutbolEquipacion::chocan(mi1, fl.r2);
        filas.append(fl);
    }
    std::stable_sort(filas.begin(), filas.end(), [](const Fila &a, const Fila &b) {
        return int(a.malVisit) + int(a.malLocal) > int(b.malVisit) + int(b.malLocal);
    });
    int malos = 0;
    for (const Fila &fl : filas) if (fl.malVisit || fl.malLocal) malos++;

    QDialog dlg(this);
    dlg.setWindowTitle("Choques de color");
    dlg.resize(760, 480);
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    QLabel *cab = new QLabel(QString(
        "Equipos cuya 1ª equipación choca con la tuya según la regla del juego "
        "(tono a menos de 60°; blanco, gris y negro chocan entre sí): <b>%1</b>.<br>"
        "En esos partidos el visitante juega con su 2ª equipación y el juego <b>no vuelve a "
        "comprobar</b> (no hay 3ª). Las filas con ✖ (%2) se jugarán con colores parecidos.")
        .arg(filas.size()).arg(malos), &dlg);
    cab->setWordWrap(true);
    lay->addWidget(cab);

    QTableWidget *t = new QTableWidget(filas.size(), 5, &dlg);
    t->setHorizontalHeaderLabels({"n", "Rival", "Rival 1ª / 2ª", "Tú de visitante", "Tú de local"});
    t->verticalHeader()->setVisible(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setIconSize(QSize(40, 16));
    for (int i = 0; i < filas.size(); i++)
    {
        const Fila &fl = filas.at(i);
        t->setItem(i, 0, new QTableWidgetItem(QString::number(fl.n)));
        t->setItem(i, 1, new QTableWidgetItem(fl.nombre));
        QPixmap pm(40, 16);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.fillRect(0, 0, 18, 16, QColor(fl.r1));
        p.fillRect(22, 0, 18, 16, QColor(fl.r2));
        p.setPen(Qt::gray);
        p.drawRect(0, 0, 17, 15);
        p.drawRect(22, 0, 17, 15);
        p.end();
        t->setItem(i, 2, new QTableWidgetItem(QIcon(pm), ""));
        QTableWidgetItem *iv = new QTableWidgetItem(fl.malVisit ? "✖ tu 2ª también choca" : "✔ usas tu 2ª");
        QTableWidgetItem *il = new QTableWidgetItem(fl.malLocal ? "✖ su 2ª también choca" : "✔ el rival usa su 2ª");
        if (fl.malVisit) iv->setForeground(QColor("#e05a4a"));
        if (fl.malLocal) il->setForeground(QColor("#e05a4a"));
        t->setItem(i, 3, iv);
        t->setItem(i, 4, il);
    }
    t->resizeColumnsToContents();
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    lay->addWidget(t);
    QPushButton *ok = new QPushButton("Cerrar", &dlg);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    lay->addWidget(ok, 0, Qt::AlignRight);
    dlg.exec();
}

// --- Dibujo de la equipación en la ficha del equipo (DBDAT/CAMISAS) -----------
void DialogEquipacion::abrirDibujoKit()
{
    const QStringList dbc = ficherosEquiposOrdenados();
    const int n = sp_numero->value();
    if (le_ruta->text().isEmpty() || n < 1 || n > dbc.size())
    {
        QMessageBox::warning(this, "Falta el equipo",
                             "Indica la ruta de EQUIPOS.DAT y el número de equipo.");
        return;
    }
    QDir juego = QFileInfo(le_ruta->text()).absoluteDir();
    juego.cdUp();
    const QString id = QFileInfo(dbc.at(n - 1)).fileName().mid(2, 6);
    DialogDibujoKit dlg(juego.absolutePath(), id, nombreCortoEquipo(dbc.at(n - 1)), this);
    dlg.exec();
}
