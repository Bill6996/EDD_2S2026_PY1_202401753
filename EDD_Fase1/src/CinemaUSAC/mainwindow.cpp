#include "mainwindow.h"
#include "CargadorCSV.h"
#include <QMessageBox>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), matrizActual(nullptr) {
    this->setWindowTitle("CinemaUSAC - Sistema de Gestion");
    this->resize(900, 600);

    panelPrincipal = new QStackedWidget(this);
    panelPrincipal->addWidget(crearPanelLogin());   // Indice 0
    panelPrincipal->addWidget(crearPanelAdmin());   // Indice 1
    panelPrincipal->addWidget(crearPanelUsuario()); // Indice 2

    setCentralWidget(panelPrincipal);
}

MainWindow::~MainWindow() {
    if (matrizActual) delete matrizActual;
}

QWidget* MainWindow::crearPanelLogin() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);

    QLabel* titulo = new QLabel("ACCESO AL SISTEMA CINEMAUSAC");
    titulo->setAlignment(Qt::AlignCenter);

    QPushButton* btnAdmin = new QPushButton("Ingresar como Administrador");
    QPushButton* btnUsuario = new QPushButton("Ingresar como Cliente");

    layout->addWidget(titulo);
    layout->addWidget(btnAdmin);
    layout->addWidget(btnUsuario);

    connect(btnAdmin, &QPushButton::clicked, [this]() { panelPrincipal->setCurrentIndex(1); });
    connect(btnUsuario, &QPushButton::clicked, [this]() { panelPrincipal->setCurrentIndex(2); });

    return vista;
}

QWidget* MainWindow::crearPanelAdmin() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);

    QLabel* titulo = new QLabel("PANEL ADMINISTRATIVO");
    QPushButton* btnCargaCSV = new QPushButton("Carga Masiva de Peliculas (CSV)");
    QPushButton* btnCrearFuncion = new QPushButton("Crear Funcion y Mapa de Asientos (Matriz)");
    QPushButton* btnReportes = new QPushButton("Generar Todos los Reportes Graphviz");
    QPushButton* btnVolver = new QPushButton("Cerrar Sesion");

    layout->addWidget(titulo);
    layout->addWidget(btnCargaCSV);
    layout->addWidget(btnCrearFuncion);
    layout->addWidget(btnReportes);
    layout->addWidget(btnVolver);

    connect(btnCargaCSV, &QPushButton::clicked, [this]() {
        QString ruta = QFileDialog::getOpenFileName(this, "Seleccionar CSV", "", "Archivos CSV (*.csv)");
        if (!ruta.isEmpty()) {
            CargadorCSV::cargarPeliculas(ruta.toStdString(), cartelera);
            QMessageBox::information(this, "Exito", "Peliculas cargadas al Arbol BST correctamente.");
        }
    });

    connect(btnCrearFuncion, &QPushButton::clicked, [this]() {
        if (matrizActual) delete matrizActual;
        matrizActual = new MatrizDispersa(10, 10); // Ejemplo sala 10x10
        QMessageBox::information(this, "Funcion Creada", "Se inicializo una sala con matriz dispersa de 10x10 asientos.");
    });

    connect(btnReportes, &QPushButton::clicked, [this]() {
        cartelera.generarReporteGraphviz();
        solicitudes.generarReporteGraphviz();
        promociones.generarReporteGraphviz();
        if (matrizActual) matrizActual->generarReporteGraphviz();
        QMessageBox::information(this, "Graphviz", "Reportes generados en la carpeta /reportes.");
    });

    connect(btnVolver, &QPushButton::clicked, [this]() { panelPrincipal->setCurrentIndex(0); });

    return vista;
}

QWidget* MainWindow::crearPanelUsuario() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);

    QLabel* titulo = new QLabel("PANEL DE CLIENTE / TAQUILLA");
    QPushButton* btnReservar = new QPushButton("Reservar Asiento");
    QPushButton* btnVolver = new QPushButton("Cerrar Sesion");

    layout->addWidget(titulo);
    layout->addWidget(btnReservar);
    layout->addWidget(btnVolver);

    connect(btnReservar, &QPushButton::clicked, [this]() {
        if (!matrizActual) {
            QMessageBox::warning(this, "Error", "No hay funciones creadas por el administrador.");
            return;
        }
        bool ok = matrizActual->reservarAsiento(4, 5, "Cliente_1");
        if (ok) QMessageBox::information(this, "Reserva", "Asiento F4-C5 reservado exitosamente.");
        else QMessageBox::warning(this, "Ocupado", "El asiento ya esta ocupado.");
    });

    connect(btnVolver, &QPushButton::clicked, [this]() { panelPrincipal->setCurrentIndex(0); });

    return vista;
}