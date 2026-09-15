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
#include <QComboBox>
#include <QSpinBox>

// Estructuras de Fase 1
#include "BSTCartelera.h"
#include "ListaSolicitudes.h"
#include "ListaPromociones.h"
#include "MatrizDispersa.h"

// Estructuras nuevas de Fase 2
#include "AVLFunciones.h"
#include "ArbolBClientes.h"
#include "TablaHashReservas.h"
#include "CargadorJSON.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    // ===== Estructuras de datos =====
    BSTCartelera       cartelera;          // Fase 1 (se mantiene)
    ListaSolicitudes   solicitudes;        // Fase 1 (se mantiene)
    ListaPromociones   promociones;        // Fase 1 (se mantiene)
    MatrizDispersa*    matrizActual;       // Fase 1 (ahora se carga por función)

    AVLFunciones       funciones;          // NUEVO - Fase 2
    ArbolBClientes     clientes;           // NUEVO - Fase 2
    TablaHashReservas  reservas;           // NUEVO - Fase 2

    // Cliente autenticado actualmente (nullptr si es admin o nadie)
    Cliente* clienteActual;

    // Función actualmente cargada en la matriz
    std::string funcionActivaCodigo;

    // ===== UI =====
    QStackedWidget* panelPrincipal;

    // Punteros para refrescar UI
    QTableWidget* tablaCarteleraAdmin;
    QTableWidget* tablaFuncionesAdmin;
    QTableWidget* tablaClientesAdmin;
    QTableWidget* mapaVisualAdmin;
    QLabel* lblDetalleSolicitudAdmin;
    QLabel* lblTotalSolicitudesAdmin;
    QLabel* lblFuncionActiva;

    // ===== Métodos de construcción de pantallas =====
    QWidget* crearPanelLogin();
    QWidget* crearPanelAdmin();
    QWidget* crearPanelUsuario();

    // Pestañas Admin
    QWidget* pestanaGestionPeliculas();
    QWidget* pestanaGestionFunciones();      // Ampliada en Fase 2
    QWidget* pestanaGestionClientes();       // NUEVA
    QWidget* pestanaGestionPromociones();
    QWidget* pestanaGestionSolicitudes();
    QWidget* pestanaCargaMasiva();           // NUEVA
    QWidget* pestanaReportesVisuales();

    // Pestañas Cliente
    QWidget* pestanaClienteCartelera();
    QWidget* pestanaClienteReservas();       // Ampliar: reservar + historial + cancelar
    QWidget* pestanaClienteSolicitudes();
    QWidget* pestanaClientePromociones();
    QWidget* pestanaClientePerfil();         // NUEVA (editar perfil)

    // ===== Helpers =====
    void refrescarSolicitudesAdmin();
    void refrescarMapaAsientosAdmin();
    void refrescarTablaFunciones();
    void refrescarTablaClientes();
    void cargarFuncionEnMatriz(const std::string& codigoFuncion);
    void guardarMatrizActual();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};

#endif // MAINWINDOW_H