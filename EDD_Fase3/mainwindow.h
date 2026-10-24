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
#include <QStringList>
#include <QList>

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

// Estructuras nuevas de Fase 3
#include "GrafoSedes.h"
#include "Blockchain.h"
#include "Persistencia.h"

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

    GrafoSedes         grafo;              // NUEVO - Fase 3 (sedes = vértices, lista de adyacencia)
    Blockchain         blockchain;         // NUEVO - Fase 3 (compras con Merkle + prueba de trabajo)
    int                contadorReservas;   // NUEVO - Fase 3 (para generar códigos de reserva únicos)

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
    QTableWidget* tablaSedesAdmin;       // Fase 3
    QTableWidget* tablaBloquesAdmin;     // Fase 3
    QLabel* lblEstadoBlockchain;         // Fase 3
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
    QWidget* pestanaGestionSedes();          // NUEVA Fase 3: grafo de sedes
    QWidget* pestanaBlockchainAdmin();       // NUEVA Fase 3: bloques, Merkle y ventas

    // Pestañas Cliente
    QWidget* pestanaClienteCartelera();
    QWidget* pestanaClienteReservas();       // Ampliar: reservar + historial + cancelar
    QWidget* pestanaClienteSolicitudes();
    QWidget* pestanaClientePromociones();
    QWidget* pestanaClientePerfil();         // NUEVA (editar perfil)
    QWidget* pestanaClienteSedes();          // NUEVA Fase 3: sedes disponibles / similares
    QWidget* pestanaClienteCompras();        // NUEVA Fase 3: historial y comprobantes desde la blockchain

    // ===== Helpers =====
    void refrescarSolicitudesAdmin();
    void refrescarMapaAsientosAdmin();
    void refrescarTablaFunciones();
    void refrescarTablaClientes();
    void cargarFuncionEnMatriz(const std::string& codigoFuncion);
    void guardarMatrizActual();

    // ===== Helpers Fase 3 =====
    void refrescarTablaSedes();
    void refrescarTablaBloques();
    void refrescarGrafo();                                   // recalcula aristas + refresca tablas
    void eliminarFuncionInterna(const std::string& codigo);  // quita función, asientos y libera la matriz si estaba activa
    QString elegirSede(const QString& titulo, const QString& etiqueta, const QString& seleccionada = "");
    std::string siguienteCodigoReserva();
    void mostrarImagen(const QString& titulo, const QString& rutaPng);
    void mostrarTabla(const QString& titulo, const QString& resumen, const QStringList& encabezados,
                      const QList<QStringList>& filas);
    void llenarComboPeliculas(QComboBox* combo);
    int elegirBloque(const QString& titulo);                 // -1 si se cancela o no hay bloques
    void generarReportesFase3();
    void verificarBlockchainAlIniciar();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
};

#endif // MAINWINDOW_H