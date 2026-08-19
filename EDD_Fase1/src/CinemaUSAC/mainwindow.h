#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>

#include "BSTCartelera.h"
#include "ListaSolicitudes.h"
#include "ListaPromociones.h"
#include "MatrizDispersa.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    // Estructuras globales
    BSTCartelera cartelera;
    ListaSolicitudes solicitudes;
    ListaPromociones promociones;
    MatrizDispersa* matrizActual;

    // Componentes de la interfaz
    QStackedWidget* panelPrincipal;

    QWidget* crearPanelLogin();
    QWidget* crearPanelAdmin();
    QWidget* crearPanelUsuario();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};

#endif // MAINWINDOW_H