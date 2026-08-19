#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>

#include "BSTCartelera.h"
#include "ListaSolicitudes.h"
#include "ListaPromociones.h"
#include "MatrizDispersa.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    BSTCartelera cartelera;
    ListaSolicitudes solicitudes;
    ListaPromociones promociones;
    MatrizDispersa* matrizActual;

    QStackedWidget* panelPrincipal;

    // Punteros globales para refresco de UI en tiempo real
    QTableWidget* tablaCarteleraAdmin;
    QTableWidget* mapaVisualAdmin;
    QLabel* lblDetalleSolicitudAdmin;
    QLabel* lblTotalSolicitudesAdmin;

    // Métodos para construir pantallas y pestañas
    QWidget* crearPanelLogin();
    QWidget* crearPanelAdmin();
    QWidget* crearPanelUsuario();

    QWidget* pestanaGestionPeliculas();
    QWidget* pestanaGestionPromociones();
    QWidget* pestanaGestionSolicitudes();
    QWidget* pestanaGestionFunciones();

    QWidget* pestanaClienteCartelera();
    QWidget* pestanaClienteReservas();
    QWidget* pestanaClienteSolicitudes();

    // Funciones de sincronización
    void refrescarSolicitudesAdmin();
    void refrescarMapaAsientosAdmin();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};

#endif // MAINWINDOW_H