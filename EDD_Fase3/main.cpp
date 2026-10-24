#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return QApplication::exec();
}

//aqui es donde llamamos todo e inicializamos las instancias
//desoliega la ventana princiál