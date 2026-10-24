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
    ArbolBClientes.h \
    AVLFunciones.h \
    Beneficio.h \
    Blockchain.h \
    BSTCartelera.h \
    CargadorCSV.h \
    CargadorJSON.h \
    Cliente.h \
    Funcion.h \
    GrafoSedes.h \
    GraphvizUtil.h \
    ListaPromociones.h \
    ListaSolicitudes.h \
    LZW.h \
    mainwindow.h \
    MatrizDispersa.h \
    MerkleTree.h \
    NodoMatriz.h \
    Pelicula.h \
    Persistencia.h \
    Promocion.h \
    Reserva.h \
    Sede.h \
    SHA256.h \
    Solicitud.h \
    TablaHashReservas.h \
    Transaccion.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
