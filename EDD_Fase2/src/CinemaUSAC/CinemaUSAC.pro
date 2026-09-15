QT += widgets
QT += core gui widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    AVLFunciones.h \
    ArbolBClientes.h \
    BSTCartelera.h \
    Beneficio.h \
    CargadorCSV.h \
    CargadorJSON.h \
    Cliente.h \
    Funcion.h \
    ListaPromociones.h \
    ListaSolicitudes.h \
    MatrizDispersa.h \
    NodoMatriz.h \
    Pelicula.h \
    Promocion.h \
    Reserva.h \
    Solicitud.h \
    TablaHashReservas.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
