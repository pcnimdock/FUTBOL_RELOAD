#include "mybutton.h"



mybutton::mybutton(QWidget *parent): QPushButton(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    Id=0;
}

void mybutton::setId(int Id)
{
    this->Id=Id;
}

int mybutton::getId()
{
    return Id;
}
QImage setOpacity(QImage& image, qreal opacity)
{
    QImage newImg(image.size(), QImage::Format_ARGB32);
    newImg.fill(Qt::transparent);

    QPainter painter(&newImg);
    painter.setOpacity(opacity);
    painter.drawImage(QRect(0, 0, image.width(), image.height()), image);

    return newImg;
}

void mybutton::setIconEQ(QImage im)
{   Image_raw=im;
    im = im.convertToFormat(QImage::Format_ARGB32);
    im= im.scaled(48*2,64*2);
    EQimage = im;
    EQimage_MouseOver = setOpacity(im, 0.5);
    setIcon(QIcon(QPixmap::fromImage(im)));
    setIconSize(QSize(48*2,64*2));
}

void mybutton::setIconEQ_italiano(QImage im)
{

      im = im.convertToFormat(QImage::Format_ARGB32);
      im= im.scaled(32*2,45*2);
      Image_raw=im;
      EQimage = im;
      EQimage_MouseOver = setOpacity(im, 0.5);
      setIcon(QIcon(QPixmap::fromImage(im)));
      setIconSize(QSize(32*2,45*2));

}

void mybutton::setIconMinifoto(QImage im)
{
    Image_raw = im;
    im = im.convertToFormat(QImage::Format_ARGB32);
    im = im.scaled(48*2,64*2);
    EQimage = im;
    EQimage_MouseOver = setOpacity(im, 0.5);
    setIcon(QIcon(QPixmap::fromImage(im)));
    setIconSize(QSize(48*2,64*2));
    isIconSetted=1;
}

void mybutton::setIconMiniEsc(QImage im)
{
    Image_raw = im;
    im = im.convertToFormat(QImage::Format_ARGB32);
    im = im.scaled(48*2,64*2);
    EQimage = im;
    EQimage_MouseOver = setOpacity(im, 0.5);
    setIcon(QIcon(QPixmap::fromImage(im)));
    setIconSize(QSize(48*2,64*2));
    isIconSetted=1;

}

void mybutton::setIconBigfoto(QImage im)
{
    Image_raw = im;
    im = im.convertToFormat(QImage::Format_ARGB32);
    im = im.scaled(48*2,64*2);
    EQimage = im;
    EQimage_MouseOver = setOpacity(im, 0.5);
    setIcon(QIcon(QPixmap::fromImage(im)));
    setIconSize(QSize(48*2,64*2));
    isIconSetted=1;
}

QImage mybutton::getIconEQ()
{
    return Image_raw;
}

bool mybutton::isIcon()
{
    return isIconSetted;
}

void mybutton::setEQname(QString name)
{
    EQname=name;
    EQname_array=EQname.toUtf8();
}

QString mybutton::getEQname()
{
    return EQname;
}


bool mybutton::event(QEvent *event)
{
    switch(event->type())
    {
    case QEvent::HoverEnter:
        //emit encima(Id);
        setIcon(QIcon(QPixmap::fromImage(EQimage_MouseOver)));
        emit encima(Id,EQname_array.data());
        return true;
        break;
    case QEvent::HoverLeave:
        setIcon(QIcon(QPixmap::fromImage(EQimage)));
        EQ_empty.clear();
        emit encima(Id,EQ_empty.data());
        //hoverLeave(static_cast<QHoverEvent*>(event));
        return true;
        break;
    case QEvent::HoverMove:
        //hoverMove(static_cast<QHoverEvent*>(event));
        return true;
        break;
    default:
        break;
    }
    //emit(hovered());
    return QWidget::event(event);
}


