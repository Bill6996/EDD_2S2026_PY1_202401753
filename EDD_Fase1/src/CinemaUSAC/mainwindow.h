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

    // Métodos para construir pantallas y pestañas
    QWidget* crearPanelLogin();
    QWidget* crearPanelAdmin();
    QWidget* crearPanelUsuario();

    // Pestañas de Administrador
    QWidget* pestanaGestionPeliculas();
    QWidget* pestanaGestionPromociones();
    QWidget* pestanaGestionSolicitudes();
    QWidget* pestanaGestionFunciones();

    // Pestañas de Cliente
    QWidget* pestanaClienteCartelera();
    QWidget* pestanaClienteReservas();
    QWidget* pestanaClienteSolicitudes();

    // Tablas de refresco
    QTableWidget* tablaCarteleraAdmin;
    void actualizarTablaPeliculas();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};

#endif // MAINWINDOW_H