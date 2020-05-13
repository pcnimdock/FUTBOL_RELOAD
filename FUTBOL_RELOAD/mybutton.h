#ifndef MYBUTTON_H
#define MYBUTTON_H

#include <QtWidgets>

class mybutton : public QPushButton
{
    Q_OBJECT
public:
    explicit mybutton(QWidget *parent = 0);
    void setId(int Id);
    int getId();
    void setIconEQ(QImage im);
    void setIconMinifoto(QImage im);
    void setIconMiniEsc(QImage im);
    void setIconBigfoto(QImage im);
    QImage getIconEQ();
    bool isIcon();
    void setEQname(QString);
    QString getEQname();
private:
    quint8 Id;
    QString EQname;
    QByteArray EQname_array;
    QImage EQimage;
    QImage EQimage_MouseOver;
    QByteArray EQ_empty;
    bool isIconSetted;
    QImage Image_raw;



protected:
    bool event(QEvent *event);
signals:
    void encima(int,char *);


};

#endif // MYBUTTON_H
