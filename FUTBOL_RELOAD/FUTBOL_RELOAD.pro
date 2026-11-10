QT       += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    pcfutbol_partida.cpp \
    dialog_partida.cpp \
    pcfutbol_tactica.cpp \
    dialog_tactica.cpp \
    pcfutbol_audio.cpp \
    dialog_sonidos.cpp \
    thirdparty/dr_impl.c \
    thirdparty/stb_vorbis.c \
    pcfutbol_ligas.cpp \
    dialog_ligas.cpp \
    pcfutbol_dibujokit.cpp \
    dialog_dibujokit.cpp \
    pcfutbol_plantillas.cpp \
    pcfutbol_csv.cpp \
    dialog_traspasos.cpp \
    dialog_restaurar.cpp \
    pcfutbol_temporada_real.cpp \
    bbdd_futbol.cpp \
    dialog_estrellas_mundiales.cpp \
    dialog_jugador.cpp \
    dialogequipos.cpp \
    entrenador.cpp \
    equipo.cpp \
    estrellas_mundiales.cpp \
    jugador.cpp \
    main.cpp \
    mainwindow.cpp \
    table_model.cpp \
    transform_minifoto.cpp \
    pcfutbol_imagenes.cpp \
    pcfutbol_extras.cpp \
    pcfutbol_equipacion.cpp \
    dialog_equipacion.cpp \
    mybutton.cpp

HEADERS += \
    pcfutbol_partida.h \
    dialog_partida.h \
    pcfutbol_tactica.h \
    dialog_tactica.h \
    pcfutbol_audio.h \
    dialog_sonidos.h \
    util_ventanas.h \
    pcfutbol_ligas.h \
    dialog_ligas.h \
    pcfutbol_dibujokit.h \
    dialog_dibujokit.h \
    pcfutbol_plantillas.h \
    pcfutbol_csv.h \
    dialog_traspasos.h \
    dialog_restaurar.h \
    pcfutbol_temporada_real.h \
    bbdd_futbol.h \
    dialog_estrellas_mundiales.h \
    dialog_jugador.h \
    dialogequipos.h \
    entrenador.h \
    equipo.h \
    estrellas_mundiales.h \
    jugador.h \
    mainwindow.h \
    paleta_escudos_sim.h \
    table_model.h \
    transform_minifoto.h \
    pcfutbol_imagenes.h \
    pcfutbol_extras.h \
    pcfutbol_equipacion.h \
    paleta_kits.h \
    dialog_equipacion.h \
    mybutton.h

FORMS += \
    dialog_estrellas_mundiales.ui \
    dialog_jugador.ui \
    dialogequipos.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

TRANSLATIONS = hellotr_it.ts
