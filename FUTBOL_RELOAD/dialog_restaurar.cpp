#include "dialog_restaurar.h"
#include "util_ventanas.h"

#include <QTreeWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QDateTime>

namespace {

QString parte(const QString &relativa)
{
    const QString r = relativa.toUpper();
    if (r.endsWith("EQUIPOS.DAT")) return "Equipaciones del partido";
    if (r.contains("CAMISAS/") || r.contains("BIGCAMP/") || r.contains("BIGESC/"))
        return "Dibujo / colores de la ficha";
    if (r.endsWith("PCMANAG.DAT") || r.endsWith("PROMANAG.DAT")) return "Composición de ligas / Europa";
    if (r.endsWith("PCFUTBOL.DAT")) return "Versión antigua de «Composición de ligas»";
    if (r.endsWith(".RAW")) return "Sonidos";
    return "Otro";
}

bool iguales(const QString &a, const QString &b)
{
    QFile fa(a), fb(b);
    if (fa.size() != fb.size()) return false;
    if (!fa.open(QIODevice::ReadOnly) || !fb.open(QIODevice::ReadOnly)) return false;
    while (!fa.atEnd())
        if (fa.read(1 << 16) != fb.read(1 << 16)) return false;
    return true;
}

} // namespace

QStringList DialogRestaurar::buscarCopias(const QString &carpetaJuego)
{
    QStringList r;
    QDirIterator it(carpetaJuego, QStringList() << "*.bak" << "*.BAK", QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) r << it.next();
    r.removeDuplicates();
    r.sort(Qt::CaseInsensitive);
    return r;
}

DialogRestaurar::DialogRestaurar(const QString &carpetaJuego, QWidget *parent)
    : QDialog(parent), m_carpeta(carpetaJuego)
{
    setWindowTitle("Restaurar originales");
    setModal(true);
    resize(820, 480);

    QVBoxLayout *root = new QVBoxLayout(this);
    QLabel *ayuda = new QLabel(
        "Estas son las copias de seguridad (.bak) que el editor guardó la primera vez que "
        "cambió cada fichero del juego. Marca los que quieras devolver a como estaban y pulsa "
        "«Restaurar». La copia .bak se conserva.\n"
        "Las plantillas y fichas (botón «Guardar» de la ventana principal) no aparecen aquí "
        "porque nunca se escriben sobre el juego, sino en una carpeta aparte.", this);
    ayuda->setWordWrap(true);
    root->addWidget(ayuda);

    m_arbol = new QTreeWidget(this);
    m_arbol->setHeaderLabels({"Fichero", "Parte del editor", "Estado", "Copia de"});
    m_arbol->setRootIsDecorated(false);
    m_arbol->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    root->addWidget(m_arbol, 1);

    m_resumen = new QLabel(this);
    root->addWidget(m_resumen);

    QHBoxLayout *fb = new QHBoxLayout();
    QPushButton *todos = new QPushButton("Marcar los modificados", this);
    QPushButton *ninguno = new QPushButton("Desmarcar todo", this);
    QPushButton *rest = new QPushButton("Restaurar los marcados", this);
    QPushButton *cerrar = new QPushButton("Cerrar", this);
    fb->addWidget(todos);
    fb->addWidget(ninguno);
    fb->addStretch();
    fb->addWidget(rest);
    fb->addWidget(cerrar);
    root->addLayout(fb);

    connect(todos, &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < m_arbol->topLevelItemCount(); i++)
        {
            QTreeWidgetItem *it = m_arbol->topLevelItem(i);
            it->setCheckState(0, it->data(0, Qt::UserRole + 1).toBool() ? Qt::Checked : Qt::Unchecked);
        }
    });
    connect(ninguno, &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < m_arbol->topLevelItemCount(); i++)
            m_arbol->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
    });
    connect(rest, &QPushButton::clicked, this, &DialogRestaurar::restaurar);
    connect(cerrar, &QPushButton::clicked, this, &QDialog::accept);

    rellenar();
    UtilVentanas::hacerDesplazable(this);
}

void DialogRestaurar::rellenar()
{
    m_arbol->clear();
    const QDir base(m_carpeta);
    int modificados = 0;
    const QStringList copias = buscarCopias(m_carpeta);
    for (const QString &bak : copias)
    {
        const QString original = bak.left(bak.size() - 4);
        const QString rel = base.relativeFilePath(original);
        QTreeWidgetItem *it = new QTreeWidgetItem(m_arbol);
        it->setText(0, rel);
        it->setText(1, parte(rel));
        it->setText(3, QFileInfo(bak).lastModified().toString("dd/MM/yyyy HH:mm"));
        it->setData(0, Qt::UserRole, bak);
        bool modificado = false;
        if (!QFile::exists(original))
        {
            it->setText(2, "falta el fichero (se puede recuperar)");
            modificado = true;
        }
        else if (iguales(original, bak))
            it->setText(2, "igual que el original");
        else
        {
            it->setText(2, "MODIFICADO");
            modificado = true;
        }
        if (modificado) modificados++;
        it->setData(0, Qt::UserRole + 1, modificado);
        it->setCheckState(0, Qt::Unchecked);
    }
    m_arbol->resizeColumnToContents(1);
    m_arbol->resizeColumnToContents(2);
    m_resumen->setText(copias.isEmpty()
        ? "No hay copias .bak en " + m_carpeta + ": el editor no ha cambiado ningún fichero del juego."
        : QString("%1 copias, %2 ficheros modificados respecto a su copia.").arg(copias.size()).arg(modificados));
}

void DialogRestaurar::restaurar()
{
    QStringList marcados;
    for (int i = 0; i < m_arbol->topLevelItemCount(); i++)
        if (m_arbol->topLevelItem(i)->checkState(0) == Qt::Checked)
            marcados << m_arbol->topLevelItem(i)->data(0, Qt::UserRole).toString();
    if (marcados.isEmpty())
    {
        QMessageBox::information(this, "Restaurar", "No hay ningún fichero marcado.");
        return;
    }
    if (QMessageBox::question(this, "Restaurar",
            QString("Se van a devolver %1 ficheros a su copia original. Los cambios hechos en "
                    "ellos con el editor se perderán.\n\n¿Continuar?").arg(marcados.size()),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QStringList fallos;
    for (const QString &bak : marcados)
    {
        const QString original = bak.left(bak.size() - 4);
        QFile fb(bak);
        if (!fb.open(QIODevice::ReadOnly)) { fallos << bak; continue; }
        const QByteArray datos = fb.readAll();
        QFile fo(original);
        if (!fo.open(QIODevice::WriteOnly) || fo.write(datos) != datos.size()) fallos << original;
    }
    rellenar();
    if (fallos.isEmpty())
        QMessageBox::information(this, "Restaurar", QString("%1 ficheros restaurados.").arg(marcados.size()));
    else
        QMessageBox::critical(this, "Restaurar", "No se pudieron restaurar:\n" + fallos.join("\n"));
}
