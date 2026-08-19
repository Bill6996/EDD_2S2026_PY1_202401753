#include "mainwindow.h"
#include "CargadorCSV.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), matrizActual(nullptr) {
    this->setWindowTitle("CinemaUSAC - Sistema de Gestión");
    this->resize(1000, 700);

    panelPrincipal = new QStackedWidget(this);
    panelPrincipal->addWidget(crearPanelLogin());   // Índice 0
    panelPrincipal->addWidget(crearPanelAdmin());   // Índice 1
    panelPrincipal->addWidget(crearPanelUsuario()); // Índice 2

    setCentralWidget(panelPrincipal);
}

MainWindow::~MainWindow() {
    if (matrizActual) delete matrizActual;
}

// ----------------------------------------------------
// LOGIN UNIFICADO POR CREDENCIALES
// ----------------------------------------------------
QWidget* MainWindow::crearPanelLogin() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);
    layout->setAlignment(Qt::AlignCenter);

    QLabel* titulo = new QLabel("PANEL GENERAL - LOGIN");
    QFont font = titulo->font();
    font.setPointSize(16);
    font.setBold(true);
    titulo->setFont(font);

    QLineEdit* txtCorreo = new QLineEdit();
    txtCorreo->setPlaceholderText("Correo electrónico");
    txtCorreo->setFixedWidth(300);

    QLineEdit* txtPassword = new QLineEdit();
    txtPassword->setPlaceholderText("Contraseña");
    txtPassword->setEchoMode(QLineEdit::Password);
    txtPassword->setFixedWidth(300);

    QPushButton* btnIngresar = new QPushButton("INICIAR SESION");
    btnIngresar->setFixedWidth(300);
    btnIngresar->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold; padding: 8px;");

    layout->addWidget(titulo, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addWidget(txtCorreo, 0, Qt::AlignCenter);
    layout->addWidget(txtPassword, 0, Qt::AlignCenter);
    layout->addSpacing(15);
    layout->addWidget(btnIngresar, 0, Qt::AlignCenter);

    connect(btnIngresar, &QPushButton::clicked, [this, txtCorreo, txtPassword]() {
        QString correo = txtCorreo->text().trimmed();
        QString pass = txtPassword->text().trimmed();

        if (correo == "admin@cinema.com" && pass == "admin123") {
            txtCorreo->clear(); txtPassword->clear();
            panelPrincipal->setCurrentIndex(1);
        } else if (correo == "cliente@cinema.com" && pass == "1234") {
            txtCorreo->clear(); txtPassword->clear();
            panelPrincipal->setCurrentIndex(2);
        } else {
            QMessageBox::warning(this, "Error", "Credenciales incorrectas.\nUse:\nAdmin: admin@cinema.com / admin123\nCliente: cliente@cinema.com / 1234");
        }
    });

    return vista;
}

// ----------------------------------------------------
// PANEL ADMINISTRATIVO CON PESTAÑAS (QTabWidget)
// ----------------------------------------------------
QWidget* MainWindow::crearPanelAdmin() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);

    QHBoxLayout* header = new QHBoxLayout();
    QLabel* titulo = new QLabel("PANEL ADMINISTRATIVO");
    QFont font = titulo->font();
    font.setBold(true);
    font.setPointSize(14);
    titulo->setFont(font);

    QPushButton* btnReportes = new QPushButton("Generar Reportes Graphviz");
    QPushButton* btnLogout = new QPushButton("Cerrar Sesión");

    header->addWidget(titulo);
    header->addStretch();
    header->addWidget(btnReportes);
    header->addWidget(btnLogout);

    QTabWidget* tabsAdmin = new QTabWidget();
    tabsAdmin->addTab(pestanaGestionPeliculas(), "Cartelera");
    tabsAdmin->addTab(pestanaGestionPromociones(), "Promociones");
    tabsAdmin->addTab(pestanaGestionSolicitudes(), "Solicitudes Especiales");
    tabsAdmin->addTab(pestanaGestionFunciones(), "Funciones y Asientos");

    layout->addLayout(header);
    layout->addWidget(tabsAdmin);

    connect(btnLogout, &QPushButton::clicked, [this]() { panelPrincipal->setCurrentIndex(0); });
    connect(btnReportes, &QPushButton::clicked, [this]() {
        cartelera.generarReporteGraphviz();
        solicitudes.generarReporteGraphviz();
        promociones.generarReporteGraphviz();
        if (matrizActual) matrizActual->generarReporteGraphviz();
        QMessageBox::information(this, "Reportes", "Reportes Graphviz generados exitosamente en la carpeta /reportes.");
    });

    return vista;
}

// Pestaña 1 Admin: Películas
QWidget* MainWindow::pestanaGestionPeliculas() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnAgregar = new QPushButton("Agregar Pelicula");
    QPushButton* btnEliminar = new QPushButton("Eliminar");
    QPushButton* btnCargaCSV = new QPushButton("Carga Masiva (CSV)");

    btnLayout->addWidget(btnAgregar);
    btnLayout->addWidget(btnEliminar);
    btnLayout->addWidget(btnCargaCSV);
    btnLayout->addStretch();

    tablaCarteleraAdmin = new QTableWidget(0, 8);
    QStringList headers = {"CO-PELICULA", "TITULO", "GENERO", "DURACION", "CLASIFICACION", "IDIOMA", "FECHA ESTRENO", "FECHA FIN"};
    tablaCarteleraAdmin->setHorizontalHeaderLabels(headers);
    tablaCarteleraAdmin->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addLayout(btnLayout);
    layout->addWidget(tablaCarteleraAdmin);

    connect(btnCargaCSV, &QPushButton::clicked, [this]() {
        QString ruta = QFileDialog::getOpenFileName(this, "Seleccionar CSV", "", "Archivos CSV (*.csv)");
        if (!ruta.isEmpty()) {
            CargadorCSV::cargarPeliculas(ruta.toStdString(), cartelera);
            QMessageBox::information(this, "Éxito", "Películas cargadas desde el archivo CSV.");
        }
    });

    return tab;
}

// Pestaña 2 Admin: Promociones
QWidget* MainWindow::pestanaGestionPromociones() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* topBtns = new QHBoxLayout();
    QPushButton* btnAddPromo = new QPushButton("Agregar Una Promoción");
    QPushButton* btnAddBeneficio = new QPushButton("Añadir Beneficio");
    topBtns->addWidget(btnAddPromo);
    topBtns->addWidget(btnAddBeneficio);
    topBtns->addStretch();

    QLabel* info = new QLabel("Estructura de Promociones (Lista Circular de Listas)\nUtilice la opción de Reportes Graphviz para visualizar el diagrama completo.");
    info->setAlignment(Qt::AlignCenter);

    layout->addLayout(topBtns);
    layout->addWidget(info);

    connect(btnAddPromo, &QPushButton::clicked, [this]() {
        promociones.insertar("Promo_01", "Martes de Locura", "2026-01-01 / 2026-12-31", "Martes");
        QMessageBox::information(this, "Promoción", "Promoción 'Promo_01' registrada.");
    });

    return tab;
}

// Pestaña 3 Admin: Solicitudes
QWidget* MainWindow::pestanaGestionSolicitudes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLabel* lblSolicitudes = new QLabel("Gestión de Solicitudes Especiales (Lista Circular Doble)");
    QHBoxLayout* acciones = new QHBoxLayout();

    QPushButton* btnAceptar = new QPushButton("Aceptar Solicitud");
    QPushButton* btnRechazar = new QPushButton("Rechazar Solicitud");

    acciones->addWidget(btnAceptar);
    acciones->addWidget(btnRechazar);

    layout->addWidget(lblSolicitudes);
    layout->addLayout(acciones);
    layout->addStretch();

    return tab;
}

// Pestaña 4 Admin: Funciones y Mapa de Asientos
QWidget* MainWindow::pestanaGestionFunciones() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGridLayout* grid = new QGridLayout();
    QLineEdit* txtPelicula = new QLineEdit(); txtPelicula->setPlaceholderText("Código Película (ej. P001)");
    QLineEdit* txtSala = new QLineEdit(); txtSala->setPlaceholderText("Sala (ej. Sala 1)");
    QLineEdit* txtFilas = new QLineEdit(); txtFilas->setPlaceholderText("Filas (ej. 10)");
    QLineEdit* txtCols = new QLineEdit(); txtCols->setPlaceholderText("Columnas (ej. 10)");
    QPushButton* btnCrear = new QPushButton("Crear Función");

    grid->addWidget(new QLabel("Película:"), 0, 0); grid->addWidget(txtPelicula, 0, 1);
    grid->addWidget(new QLabel("Sala:"), 0, 2); grid->addWidget(txtSala, 0, 3);
    grid->addWidget(new QLabel("Filas:"), 1, 0); grid->addWidget(txtFilas, 1, 1);
    grid->addWidget(new QLabel("Columnas:"), 1, 2); grid->addWidget(txtCols, 1, 3);
    grid->addWidget(btnCrear, 2, 0, 1, 4);

    layout->addLayout(grid);
    layout->addStretch();

    connect(btnCrear, &QPushButton::clicked, [this, txtFilas, txtCols]() {
        int f = txtFilas->text().toInt();
        int c = txtCols->text().toInt();
        if (f <= 0 || c <= 0) { f = 10; c = 10; }

        if (matrizActual) delete matrizActual;
        matrizActual = new MatrizDispersa(f, c);
        QMessageBox::information(this, "Matriz Generada", QString("Función creada con matriz dispersa de %1x%2 asientos.").arg(f).arg(c));
    });

    return tab;
}

// ----------------------------------------------------
// PANEL DE CLIENTE / TAQUILLA
// ----------------------------------------------------
QWidget* MainWindow::crearPanelUsuario() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);

    QHBoxLayout* header = new QHBoxLayout();
    QLabel* titulo = new QLabel("PANEL DE CLIENTE");
    QFont font = titulo->font(); font.setBold(true); font.setPointSize(14);
    titulo->setFont(font);

    QPushButton* btnLogout = new QPushButton("Cerrar Sesión");
    header->addWidget(titulo);
    header->addStretch();
    header->addWidget(btnLogout);

    QTabWidget* tabsUsuario = new QTabWidget();
    tabsUsuario->addTab(pestanaClienteCartelera(), "Ver Cartelera");
    tabsUsuario->addTab(pestanaClienteReservas(), "Reservar / Cancelar Asientos");
    tabsUsuario->addTab(pestanaClienteSolicitudes(), "Solicitudes Especiales");

    layout->addLayout(header);
    layout->addWidget(tabsUsuario);

    connect(btnLogout, &QPushButton::clicked, [this]() { panelPrincipal->setCurrentIndex(0); });

    return vista;
}

QWidget* MainWindow::pestanaClienteCartelera() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    QLabel* lbl = new QLabel("Cartelera de Películas Disponibles");
    layout->addWidget(lbl);
    layout->addStretch();
    return tab;
}

QWidget* MainWindow::pestanaClienteReservas() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* inputs = new QHBoxLayout();
    QLineEdit* txtFila = new QLineEdit(); txtFila->setPlaceholderText("Fila");
    QLineEdit* txtCol = new QLineEdit(); txtCol->setPlaceholderText("Columna/Asiento");
    QLineEdit* txtCliente = new QLineEdit(); txtCliente->setPlaceholderText("Nombre del Cliente");
    QPushButton* btnReservar = new QPushButton("Reservar Asiento");

    inputs->addWidget(txtFila);
    inputs->addWidget(txtCol);
    inputs->addWidget(txtCliente);
    inputs->addWidget(btnReservar);

    layout->addLayout(inputs);
    layout->addStretch();

    connect(btnReservar, &QPushButton::clicked, [this, txtFila, txtCol, txtCliente]() {
        if (!matrizActual) {
            QMessageBox::warning(this, "Error", "No existe ninguna función activa creada por el administrador.");
            return;
        }
        int f = txtFila->text().toInt();
        int c = txtCol->text().toInt();
        QString cli = txtCliente->text().trimmed();

        if (cli.isEmpty()) cli = "Cliente";

        bool ok = matrizActual->reservarAsiento(f, c, cli.toStdString());
        if (ok) {
            QMessageBox::information(this, "Reserva Éxito", QString("Asiento F%1-C%2 reservado a nombre de %3.").arg(f).arg(c).arg(cli));
        } else {
            QMessageBox::warning(this, "Ocupado", "El asiento seleccionado ya está ocupado o fuera de rango.");
        }
    });

    return tab;
}

QWidget* MainWindow::pestanaClienteSolicitudes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLineEdit* txtNombre = new QLineEdit(); txtNombre->setPlaceholderText("Nombre y Apellido");
    QLineEdit* txtTel = new QLineEdit(); txtTel->setPlaceholderText("Teléfono");
    QLineEdit* txtTipo = new QLineEdit(); txtTipo->setPlaceholderText("Tipo (Cumpleaños, Aniversario, etc.)");
    QLineEdit* txtDesc = new QLineEdit(); txtDesc->setPlaceholderText("Descripción de la solicitud");
    QPushButton* btnEnviar = new QPushButton("Enviar Solicitud Especial");

    layout->addWidget(txtNombre);
    layout->addWidget(txtTel);
    layout->addWidget(txtTipo);
    layout->addWidget(txtDesc);
    layout->addWidget(btnEnviar);
    layout->addStretch();

    connect(btnEnviar, &QPushButton::clicked, [this, txtNombre, txtTel, txtTipo, txtDesc]() {
        solicitudes.insertar(txtNombre->text().toStdString(), txtTel->text().toStdString(),
                             txtTipo->text().toStdString(), txtDesc->text().toStdString(), "2026-08-19");
        QMessageBox::information(this, "Solicitud", "Solicitud registrada correctamente.");
    });

    return tab;
}