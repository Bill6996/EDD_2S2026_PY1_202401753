#include "mainwindow.h"
#include "CargadorCSV.h"
#include "CargadorJSON.h"
#include <QInputDialog>
#include <QHeaderView>
#include <QScrollArea>
#include <QPixmap>
#include <QCoreApplication>
#include <QDir>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QTreeWidget>
#include <QFrame>
#include <QFile>

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    matrizActual(nullptr),
    clienteActual(nullptr),
    funcionActivaCodigo(""),
    tablaCarteleraAdmin(nullptr),
    tablaFuncionesAdmin(nullptr),
    tablaClientesAdmin(nullptr),
    mapaVisualAdmin(nullptr),
    lblDetalleSolicitudAdmin(nullptr),
    lblTotalSolicitudesAdmin(nullptr),
    lblFuncionActiva(nullptr)
{
    this->setWindowTitle("CinemaUSAC - Fase 2");
    this->resize(1200, 800);

    QDir().mkpath(QCoreApplication::applicationDirPath() + "/asientos");
    QDir().mkpath(QCoreApplication::applicationDirPath() + "/reportes");

    panelPrincipal = new QStackedWidget(this);
    panelPrincipal->addWidget(crearPanelLogin());
    panelPrincipal->addWidget(crearPanelAdmin());
    panelPrincipal->addWidget(crearPanelUsuario());

    setCentralWidget(panelPrincipal);
}

MainWindow::~MainWindow() {
    if (matrizActual && !funcionActivaCodigo.empty()) {
        guardarMatrizActual();
    }
    if (matrizActual) {
        delete matrizActual;
        matrizActual = nullptr;
    }
}

// ============================================================
// LOGIN
// ============================================================
QWidget* MainWindow::crearPanelLogin() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);
    layout->setAlignment(Qt::AlignCenter);

    QLabel* titulo = new QLabel("PANEL GENERAL - LOGIN");
    QFont font = titulo->font(); font.setPointSize(16); font.setBold(true);
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

    QPushButton* btnRegistro = new QPushButton("Registrarse como Cliente");
    btnRegistro->setFixedWidth(300);
    btnRegistro->setStyleSheet("background-color: #1565c0; color: white; font-weight: bold; padding: 8px;");

    layout->addWidget(titulo, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addWidget(txtCorreo, 0, Qt::AlignCenter);
    layout->addWidget(txtPassword, 0, Qt::AlignCenter);
    layout->addSpacing(15);
    layout->addWidget(btnIngresar, 0, Qt::AlignCenter);
    layout->addWidget(btnRegistro, 0, Qt::AlignCenter);

    // ----- Login -----
    connect(btnIngresar, &QPushButton::clicked, [this, txtCorreo, txtPassword]() {
        QString correo = txtCorreo->text().trimmed();
        QString pass   = txtPassword->text().trimmed();

        if (correo == "AdminCine@gmail.com" && pass == "admin123Pass") {
            clienteActual = nullptr;
            txtCorreo->clear();
            txtPassword->clear();
            panelPrincipal->setCurrentIndex(1);
            return;
        }

        Cliente* cli = clientes.buscarPorCorreo(correo.toStdString());
        if (cli != nullptr && cli->password == pass.toStdString()) {
            clienteActual = cli;
            txtCorreo->clear();
            txtPassword->clear();
            panelPrincipal->setCurrentIndex(2);
            return;
        }

        QMessageBox::warning(this, "Error",
                             "Credenciales incorrectas.\n\n"
                             "Admin: AdminCine@gmail.com / admin123Pass\n"
                             "Cliente: debe estar registrado.");
    });

    // ----- Registro -----
    connect(btnRegistro, &QPushButton::clicked, [this]() {
        bool ok;
        QString id = QInputDialog::getText(this, "Registro", "ID (ej. U001):", QLineEdit::Normal, "", &ok);
        if (!ok || id.isEmpty()) return;

        QString nombre = QInputDialog::getText(this, "Registro", "Nombre completo:", QLineEdit::Normal, "", &ok);
        if (!ok || nombre.isEmpty()) return;

        QString correo = QInputDialog::getText(this, "Registro", "Correo:", QLineEdit::Normal, "", &ok);
        if (!ok || correo.isEmpty()) return;

        QString telefono = QInputDialog::getText(this, "Registro", "Teléfono:", QLineEdit::Normal, "", &ok);

        QString password = QInputDialog::getText(this, "Registro", "Contraseña:", QLineEdit::Normal, "", &ok);
        if (!ok || password.isEmpty()) return;

        // Validar que el correo no exista
        if (clientes.buscarPorCorreo(correo.toStdString()) != nullptr) {
            QMessageBox::warning(this, "Error", "Ese correo ya está registrado.");
            return;
        }

        Cliente* cli = new Cliente(id.toStdString(), nombre.toStdString(),
                                   correo.toStdString(), telefono.toStdString(),
                                   password.toStdString());
        if (clientes.insertar(cli)) {
            QMessageBox::information(this, "Registro exitoso",
                                     "Cliente registrado.\nYa puedes iniciar sesión con tu correo y contraseña.");
        } else {
            delete cli;
            QMessageBox::warning(this, "Error", "El ID ya existe. Usa otro.");
        }
    });

    return vista;
}

// ============================================================
// PANEL ADMIN
// ============================================================
QWidget* MainWindow::crearPanelAdmin() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);

    QHBoxLayout* header = new QHBoxLayout();
    QLabel* titulo = new QLabel("PANEL ADMINISTRATIVO");
    QFont font = titulo->font(); font.setBold(true); font.setPointSize(14);
    titulo->setFont(font);

    QPushButton* btnReportes = new QPushButton("Generar Reportes Graphviz");
    QPushButton* btnLogout = new QPushButton("Cerrar Sesión");

    header->addWidget(titulo);
    header->addStretch();
    header->addWidget(btnReportes);
    header->addWidget(btnLogout);

    QTabWidget* tabsAdmin = new QTabWidget();
    tabsAdmin->addTab(pestanaGestionPeliculas(), "Cartelera");
    tabsAdmin->addTab(pestanaGestionFunciones(), "Funciones y Asientos");
    tabsAdmin->addTab(pestanaGestionClientes(), "Clientes");           // NUEVA
    tabsAdmin->addTab(pestanaCargaMasiva(), "Carga Masiva JSON");       // NUEVA
    tabsAdmin->addTab(pestanaGestionPromociones(), "Promociones");
    tabsAdmin->addTab(pestanaGestionSolicitudes(), "Solicitudes Especiales");
    tabsAdmin->addTab(pestanaReportesVisuales(), "Reportes Visuales");

    layout->addLayout(header);
    layout->addWidget(tabsAdmin);

    connect(btnLogout, &QPushButton::clicked, [this]() {
        panelPrincipal->setCurrentIndex(0);
    });

    connect(btnReportes, &QPushButton::clicked, [this]() {
        cartelera.generarReporteGraphviz();
        funciones.generarReporteGraphviz();
        clientes.generarReporteGraphviz();
        reservas.generarReporteGraphviz();
        solicitudes.generarReporteGraphviz();
        promociones.generarReporteGraphviz();
        if (matrizActual) matrizActual->generarReporteGraphviz();

        QString ruta = QCoreApplication::applicationDirPath() + "/reportes";
        QMessageBox::information(this, "Reportes Generados",
                                 "Reportes generados con éxito.\n\nRuta:\n" + ruta);
    });

    return vista;
}

// ============================================================
// PESTAÑA CLIENTES (NUEVA - básica para que compile)
// ============================================================
QWidget* MainWindow::pestanaGestionClientes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* btns = new QHBoxLayout();
    QPushButton* btnRefrescar = new QPushButton("Actualizar Lista");
    QPushButton* btnAgregar   = new QPushButton("Agregar Cliente");
    QPushButton* btnEliminar  = new QPushButton("Eliminar Cliente");
    QPushButton* btnVerReservas = new QPushButton("Ver reservas del cliente");
    btns->addWidget(btnVerReservas);
    btns->addWidget(btnRefrescar);
    btns->addWidget(btnAgregar);
    btns->addWidget(btnEliminar);
    btns->addStretch();

    tablaClientesAdmin = new QTableWidget(0, 5);
    tablaClientesAdmin->setHorizontalHeaderLabels({"ID", "Nombre", "Correo", "Teléfono", "Tipo"});
    tablaClientesAdmin->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addLayout(btns);
    layout->addWidget(tablaClientesAdmin);

    connect(btnRefrescar, &QPushButton::clicked, [this]() {
        refrescarTablaClientes();
    });

    connect(btnAgregar, &QPushButton::clicked, [this]() {
        bool ok;
        QString id = QInputDialog::getText(this, "Cliente", "ID (ej. U001):", QLineEdit::Normal, "", &ok);
        if (!ok || id.isEmpty()) return;
        QString nom = QInputDialog::getText(this, "Cliente", "Nombre:", QLineEdit::Normal, "", &ok);
        if (!ok || nom.isEmpty()) return;
        QString corr = QInputDialog::getText(this, "Cliente", "Correo:", QLineEdit::Normal, "", &ok);
        if (!ok || corr.isEmpty()) return;
        QString tel = QInputDialog::getText(this, "Cliente", "Teléfono:", QLineEdit::Normal, "", &ok);
        QString pass = QInputDialog::getText(this, "Cliente", "Password:", QLineEdit::Normal, "1234", &ok);

        Cliente* cli = new Cliente(id.toStdString(), nom.toStdString(), corr.toStdString(),
                                   tel.toStdString(), pass.toStdString());
        if (clientes.insertar(cli)) {
            refrescarTablaClientes();
            QMessageBox::information(this, "Éxito", "Cliente agregado al Árbol B.");
        } else {
            delete cli;
            QMessageBox::warning(this, "Error", "ID o correo ya existen.");
        }
    });

    connect(btnEliminar, &QPushButton::clicked, [this]() {
        bool ok;
        QString id = QInputDialog::getText(this, "Eliminar", "ID del cliente:", QLineEdit::Normal, "", &ok);
        if (!ok || id.isEmpty()) return;

        Cliente* cli = clientes.buscar(id.toStdString());
        if (!cli) {
            QMessageBox::warning(this, "Error", "Cliente no encontrado.");
            return;
        }

        // 1. Eliminar en cascada sus reservas de la Tabla Hash
        NodoCodigoReserva* nodo = cli->listaReservas;
        while (nodo != nullptr) {
            reservas.eliminar(nodo->codigo_reserva);
            nodo = nodo->siguiente;
        }

        // 2. Eliminar del Árbol B
        clientes.eliminar(id.toStdString());

        refrescarTablaClientes();
        QMessageBox::information(this, "OK",
                                 "Reservas eliminadas de la Tabla Hash.\nCliente procesado en el Árbol B.");
    });

    connect(btnVerReservas, &QPushButton::clicked, [this]() {
        bool ok;
        QString id = QInputDialog::getText(this, "Reservas", "ID del cliente:", QLineEdit::Normal, "", &ok);
        if (!ok || id.isEmpty()) return;

        Cliente* cli = clientes.buscar(id.toStdString());
        if (!cli) {
            QMessageBox::warning(this, "Error", "Cliente no encontrado.");
            return;
        }

        QString lista;
        NodoCodigoReserva* nodo = cli->listaReservas;
        int cont = 0;
        while (nodo != nullptr) {
            Reserva* res = reservas.buscar(nodo->codigo_reserva);
            if (res) {
                cont++;
                lista += QString("%1) %2 | Función: %3 | F%4-C%5 | Fecha: %6\n")
                             .arg(cont)
                             .arg(QString::fromStdString(res->codigo_reserva))
                             .arg(QString::fromStdString(res->codigo_funcion))
                             .arg(res->fila)
                             .arg(res->columna)
                             .arg(QString::fromStdString(res->fecha_reserva));
            }
            nodo = nodo->siguiente;
        }

        if (cont == 0)
            QMessageBox::information(this, "Reservas", "El cliente no tiene reservas.");
        else
            QMessageBox::information(this, "Reservas de " + QString::fromStdString(cli->nombre), lista);
    });

    return tab;
}

// ============================================================
// PESTAÑA CARGA MASIVA JSON (NUEVA)
// ============================================================
QWidget* MainWindow::pestanaCargaMasiva() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLabel* lbl = new QLabel("Carga Masiva desde archivos JSON");
    lbl->setFont(QFont("Arial", 12, QFont::Bold));

    QPushButton* btnPeliculas = new QPushButton("Cargar Películas + Funciones (JSON)");
    QPushButton* btnClientes  = new QPushButton("Cargar Clientes + Reservas (JSON)");

    btnPeliculas->setStyleSheet("padding: 12px; font-weight: bold;");
    btnClientes->setStyleSheet("padding: 12px; font-weight: bold;");

    layout->addWidget(lbl);
    layout->addSpacing(20);
    layout->addWidget(btnPeliculas);
    layout->addWidget(btnClientes);
    layout->addStretch();

    connect(btnPeliculas, &QPushButton::clicked, [this]() {
        QString ruta = QFileDialog::getOpenFileName(this, "Seleccionar JSON de Películas", "", "JSON (*.json)");
        if (ruta.isEmpty()) return;

        int n = CargadorJSON::cargarPeliculasYFunciones(ruta.toStdString(), cartelera, funciones);
        if (tablaCarteleraAdmin) cartelera.cargarEnTabla(tablaCarteleraAdmin);
        refrescarTablaFunciones();
        QMessageBox::information(this, "Carga completada",
                                 QString("Se cargaron %1 películas (y sus funciones).").arg(n));
    });

    connect(btnClientes, &QPushButton::clicked, [this]() {
        QString ruta = QFileDialog::getOpenFileName(this, "Seleccionar JSON de Clientes", "", "JSON (*.json)");
        if (ruta.isEmpty()) return;

        int n = CargadorJSON::cargarClientesYReservas(ruta.toStdString(), clientes, reservas);
        refrescarTablaClientes();
        QMessageBox::information(this, "Carga completada",
                                 QString("Se cargaron %1 clientes (y sus reservas).").arg(n));
    });

    return tab;
}

// ============================================================
// PESTAÑA PERFIL CLIENTE (stub para que compile)
// ============================================================
QWidget* MainWindow::pestanaClientePerfil() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLabel* titulo = new QLabel("Mi Perfil");
    titulo->setFont(QFont("Arial", 14, QFont::Bold));

    QLabel* lblInfo = new QLabel("Inicia sesión para ver tu perfil.");
    lblInfo->setStyleSheet("padding: 10px; border: 1px solid gray;");

    QPushButton* btnActualizar = new QPushButton("Actualizar mis datos");
    QPushButton* btnRefrescar  = new QPushButton("Refrescar perfil");

    layout->addWidget(titulo);
    layout->addWidget(lblInfo);
    layout->addWidget(btnRefrescar);
    layout->addWidget(btnActualizar);
    layout->addStretch();

    auto mostrarPerfil = [this, lblInfo]() {
        if (!clienteActual) {
            lblInfo->setText("No hay cliente autenticado.");
            return;
        }
        lblInfo->setText(
            QString("ID: %1\nNombre: %2\nCorreo: %3\nTeléfono: %4")
                .arg(QString::fromStdString(clienteActual->id))
                .arg(QString::fromStdString(clienteActual->nombre))
                .arg(QString::fromStdString(clienteActual->correo))
                .arg(QString::fromStdString(clienteActual->telefono))
            );
    };

    connect(btnRefrescar, &QPushButton::clicked, mostrarPerfil);

    connect(btnActualizar, &QPushButton::clicked, [this, mostrarPerfil]() {
        if (!clienteActual) {
            QMessageBox::warning(this, "Error", "No hay cliente autenticado.");
            return;
        }
        bool ok;
        QString nombre = QInputDialog::getText(this, "Editar", "Nombre:", QLineEdit::Normal,
                                               QString::fromStdString(clienteActual->nombre), &ok);
        if (!ok) return;
        QString telefono = QInputDialog::getText(this, "Editar", "Teléfono:", QLineEdit::Normal,
                                                 QString::fromStdString(clienteActual->telefono), &ok);
        if (!ok) return;
        QString password = QInputDialog::getText(this, "Editar", "Nueva contraseña (dejar vacío = no cambiar):",
                                                 QLineEdit::Normal, "", &ok);

        clienteActual->nombre = nombre.toStdString();
        clienteActual->telefono = telefono.toStdString();
        if (!password.isEmpty())
            clienteActual->password = password.toStdString();

        mostrarPerfil();
        QMessageBox::information(this, "OK", "Perfil actualizado.");
    });

    // Mostrar al entrar
    mostrarPerfil();

    return tab;
}

// ============================================================
// RESTO DE PESTAÑAS (las que ya tenías)
// ============================================================
QWidget* MainWindow::pestanaGestionPeliculas() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnAgregar = new QPushButton("Agregar Pelicula");
    QPushButton* btnEliminar = new QPushButton("Eliminar Película");
    QPushButton* btnCargaCSV = new QPushButton("Carga Masiva (CSV)");

    QComboBox* cmbRecorrido = new QComboBox();
    cmbRecorrido->addItems({"inorden", "preorden", "postorden"});
    QPushButton* btnRecorrer = new QPushButton("Listar por recorrido");

    QPushButton* btnEditar = new QPushButton("Editar Película");
    btnLayout->addWidget(btnEditar);

    btnLayout->addWidget(btnAgregar);
    btnLayout->addWidget(btnEliminar);
    btnLayout->addWidget(btnCargaCSV);
    btnLayout->addWidget(cmbRecorrido);
    btnLayout->addWidget(btnRecorrer);
    btnLayout->addStretch();

    tablaCarteleraAdmin = new QTableWidget(0, 9);
    QStringList headers = {"CO-PELICULA", "TITULO", "GENERO", "DURACION", "CLASIFICACION", "IDIOMA", "FECHA ESTRENO", "FECHA FIN", "ESTADO CARTELERA"};
    tablaCarteleraAdmin->setHorizontalHeaderLabels(headers);
    tablaCarteleraAdmin->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addLayout(btnLayout);
    layout->addWidget(tablaCarteleraAdmin);

    connect(btnCargaCSV, &QPushButton::clicked, [this]() {
        QString ruta = QFileDialog::getOpenFileName(this, "Seleccionar CSV", "", "Archivos CSV (*.csv)");
        if (!ruta.isEmpty()) {
            CargadorCSV::cargarPeliculas(ruta.toStdString(), cartelera);
            cartelera.cargarEnTabla(tablaCarteleraAdmin);
            QMessageBox::information(this, "Éxito", "Películas cargadas.");
        }
    });

    connect(btnAgregar, &QPushButton::clicked, [this]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Agregar", "Código (ej. P005):", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;
        QString tit = QInputDialog::getText(this, "Agregar", "Título:", QLineEdit::Normal, "", &ok);
        QString gen = QInputDialog::getText(this, "Agregar", "Género:", QLineEdit::Normal, "", &ok);
        int dur = QInputDialog::getInt(this, "Agregar", "Duración (min):", 120, 1, 500, 1, &ok);
        QString fFin = QInputDialog::getText(this, "Agregar", "Fecha Fin (YYYY-MM-DD):", QLineEdit::Normal, "2026-08-25", &ok);
        cartelera.insertar(cod.toStdString(), tit.toStdString(), gen.toStdString(), dur, "B15", "Español", "2026-08-01", fFin.toStdString());
        cartelera.cargarEnTabla(tablaCarteleraAdmin);
    });

    connect(btnEliminar, &QPushButton::clicked, [this]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Eliminar", "Código:", QLineEdit::Normal, "", &ok);
        if (ok && !cod.isEmpty()) {
            cartelera.eliminar(cod.toStdString());
            cartelera.cargarEnTabla(tablaCarteleraAdmin);
            QMessageBox::information(this, "Éxito", "Película eliminada.");
        }
    });

    connect(btnRecorrer, &QPushButton::clicked, [this, cmbRecorrido]() {
        QString rec = cmbRecorrido->currentText();
        cartelera.cargarEnTabla(tablaCarteleraAdmin, rec.toStdString());
    });

    connect(btnEditar, &QPushButton::clicked, [this]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Editar", "Código de la película:", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;

        Pelicula* p = cartelera.buscar(cod.toStdString());
        if (!p) {
            QMessageBox::warning(this, "Error", "Película no encontrada.");
            return;
        }

        QString tit = QInputDialog::getText(this, "Editar", "Título:", QLineEdit::Normal,
                                            QString::fromStdString(p->titulo), &ok);
        if (!ok) return;
        QString gen = QInputDialog::getText(this, "Editar", "Género:", QLineEdit::Normal,
                                            QString::fromStdString(p->genero), &ok);
        if (!ok) return;
        int dur = QInputDialog::getInt(this, "Editar", "Duración:", p->duracion, 1, 500, 1, &ok);
        if (!ok) return;
        QString clas = QInputDialog::getText(this, "Editar", "Clasificación:", QLineEdit::Normal,
                                             QString::fromStdString(p->clasificacion), &ok);
        if (!ok) return;
        QString idio = QInputDialog::getText(this, "Editar", "Idioma:", QLineEdit::Normal,
                                             QString::fromStdString(p->idioma), &ok);
        if (!ok) return;
        QString fEst = QInputDialog::getText(this, "Editar", "Fecha estreno:", QLineEdit::Normal,
                                             QString::fromStdString(p->fechaEstreno), &ok);
        if (!ok) return;
        QString fFin = QInputDialog::getText(this, "Editar", "Fecha fin:", QLineEdit::Normal,
                                             QString::fromStdString(p->fechaFin), &ok);
        if (!ok) return;

        // Actualizar los campos del nodo (el código no se cambia)
        p->titulo = tit.toStdString();
        p->genero = gen.toStdString();
        p->duracion = dur;
        p->clasificacion = clas.toStdString();
        p->idioma = idio.toStdString();
        p->fechaEstreno = fEst.toStdString();
        p->fechaFin = fFin.toStdString();

        cartelera.cargarEnTabla(tablaCarteleraAdmin);
        QMessageBox::information(this, "OK", "Película actualizada.");
    });

    return tab;
}

QWidget* MainWindow::pestanaGestionPromociones() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* topBtns = new QHBoxLayout();
    QPushButton* btnAddPromo = new QPushButton("Agregar Promoción");
    QPushButton* btnDelPromo = new QPushButton("Eliminar Promoción");
    QPushButton* btnAddBeneficio = new QPushButton("Añadir Beneficio");
    topBtns->addWidget(btnAddPromo);
    topBtns->addWidget(btnDelPromo);
    topBtns->addWidget(btnAddBeneficio);
    topBtns->addStretch();

    QTreeWidget* treePromos = new QTreeWidget();
    treePromos->setColumnCount(4);
    treePromos->setHeaderLabels({"CÓDIGO / ESTRUCTURA", "NOMBRE / TIPO BENEFICIO", "VIGENCIA / VALOR", "DÍAS APLICABLES / DESCRIPCIÓN"});
    treePromos->header()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addLayout(topBtns);
    layout->addWidget(treePromos);

    connect(btnAddPromo, &QPushButton::clicked, [this, treePromos]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Nueva Promoción", "Código:", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;
        QString nom = QInputDialog::getText(this, "Nueva Promoción", "Nombre:", QLineEdit::Normal, "", &ok);
        if (!ok || nom.isEmpty()) return;
        QString fIni = QInputDialog::getText(this, "Nueva Promoción", "Fecha Inicio:", QLineEdit::Normal, "2026-08-01", &ok);
        QString fFin = QInputDialog::getText(this, "Nueva Promoción", "Fecha Fin:", QLineEdit::Normal, "2026-12-31", &ok);
        QString dias = QInputDialog::getText(this, "Nueva Promoción", "Días:", QLineEdit::Normal, "Martes", &ok);
        promociones.insertar(cod.toStdString(), nom.toStdString(), fIni.toStdString(), fFin.toStdString(), dias.toStdString());
        promociones.cargarEnTreeWidget(treePromos);
    });

    connect(btnDelPromo, &QPushButton::clicked, [this, treePromos]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Eliminar", "Código:", QLineEdit::Normal, "", &ok);
        if (ok && !cod.isEmpty()) {
            if (promociones.eliminar(cod.toStdString())) {
                promociones.cargarEnTreeWidget(treePromos);
            } else {
                QMessageBox::warning(this, "Error", "No encontrada.");
            }
        }
    });

    connect(btnAddBeneficio, &QPushButton::clicked, [this, treePromos]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Beneficio", "Código promoción:", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;
        Promocion* p = promociones.buscar(cod.toStdString());
        if (!p) { QMessageBox::warning(this, "Error", "No existe."); return; }
        QString tipo = QInputDialog::getText(this, "Beneficio", "Tipo:", QLineEdit::Normal, "2x1", &ok);
        QString desc = QInputDialog::getText(this, "Beneficio", "Descripción:", QLineEdit::Normal, "", &ok);
        QString val = QInputDialog::getText(this, "Beneficio", "Valor:", QLineEdit::Normal, "NA", &ok);
        p->agregarBeneficio(tipo.toStdString(), desc.toStdString(), val.toStdString());
        promociones.cargarEnTreeWidget(treePromos);
    });

    return tab;
}

QWidget* MainWindow::pestanaGestionSolicitudes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* topHeader = new QHBoxLayout();
    QLabel* lblInfo = new QLabel("Solicitudes Especiales Pendientes");
    lblInfo->setFont(QFont("Arial", 11, QFont::Bold));
    lblTotalSolicitudesAdmin = new QLabel("Total Pendientes: 0");
    lblTotalSolicitudesAdmin->setStyleSheet("font-weight: bold; color: #4caf50;");
    topHeader->addWidget(lblInfo);
    topHeader->addStretch();
    topHeader->addWidget(lblTotalSolicitudesAdmin);

    lblDetalleSolicitudAdmin = new QLabel("No hay solicitudes pendientes.");
    lblDetalleSolicitudAdmin->setStyleSheet("border: 1px solid gray; padding: 15px; background-color: #222; font-size: 13px;");

    QHBoxLayout* navLayout = new QHBoxLayout();
    QPushButton* btnAnterior = new QPushButton("◄ Anterior");
    QPushButton* btnSiguiente = new QPushButton("Siguiente ►");
    navLayout->addWidget(btnAnterior);
    navLayout->addWidget(btnSiguiente);

    QHBoxLayout* acciones = new QHBoxLayout();
    QPushButton* btnAceptar = new QPushButton("Aprobar");
    QPushButton* btnRechazar = new QPushButton("Rechazar");
    acciones->addWidget(btnAceptar);
    acciones->addWidget(btnRechazar);

    layout->addLayout(topHeader);
    layout->addWidget(lblDetalleSolicitudAdmin);
    layout->addLayout(navLayout);
    layout->addLayout(acciones);
    layout->addStretch();

    static Solicitud* solicitudActualVista = nullptr;

    auto actualizarVistaNavegacion = [this]() {
        if (!solicitudActualVista) solicitudActualVista = solicitudes.getPrimera();
        if (solicitudActualVista) {
            lblDetalleSolicitudAdmin->setText(
                QString("Solicitud #%1 | Cliente: %2\nTeléfono: %3\nTipo: %4\nDescripción: %5\nEstado: %6")
                    .arg(solicitudActualVista->numero)
                    .arg(QString::fromStdString(solicitudActualVista->cliente))
                    .arg(QString::fromStdString(solicitudActualVista->telefono))
                    .arg(QString::fromStdString(solicitudActualVista->tipo))
                    .arg(QString::fromStdString(solicitudActualVista->descripcion))
                    .arg(QString::fromStdString(solicitudActualVista->estado)));
        } else {
            lblDetalleSolicitudAdmin->setText("No hay solicitudes pendientes.");
        }
        if (lblTotalSolicitudesAdmin)
            lblTotalSolicitudesAdmin->setText(QString("Total Pendientes: %1").arg(solicitudes.getTotalPendientes()));
    };

    connect(btnSiguiente, &QPushButton::clicked, [this, actualizarVistaNavegacion]() {
        if (solicitudActualVista) solicitudActualVista = solicitudActualVista->siguiente;
        actualizarVistaNavegacion();
    });
    connect(btnAnterior, &QPushButton::clicked, [this, actualizarVistaNavegacion]() {
        if (solicitudActualVista) solicitudActualVista = solicitudActualVista->anterior;
        actualizarVistaNavegacion();
    });
    connect(btnAceptar, &QPushButton::clicked, [this, actualizarVistaNavegacion]() {
        if (!solicitudActualVista) return;
        solicitudActualVista->estado = "Atendida / Aprobada";
        refrescarSolicitudesAdmin();
        actualizarVistaNavegacion();
    });
    connect(btnRechazar, &QPushButton::clicked, [this, actualizarVistaNavegacion]() {
        if (!solicitudActualVista) return;
        solicitudActualVista->estado = "Rechazada";
        refrescarSolicitudesAdmin();
        actualizarVistaNavegacion();
    });

    return tab;
}

QWidget* MainWindow::pestanaGestionFunciones() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // Tabla de funciones del AVL
    tablaFuncionesAdmin = new QTableWidget(0, 7);
    tablaFuncionesAdmin->setHorizontalHeaderLabels({"Código", "Película", "Horario", "Sala", "Filas", "Columnas", "Archivo"});
    tablaFuncionesAdmin->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QHBoxLayout* btnsFun = new QHBoxLayout();
    QPushButton* btnCrearFun = new QPushButton("Crear Función (AVL)");
    QPushButton* btnCargarFun = new QPushButton("Cargar Función en Matriz");
    QPushButton* btnRefrescarFun = new QPushButton("Actualizar Lista");
    QPushButton* btnEliminarFun = new QPushButton("Eliminar Función");
    QPushButton* btnEditarFun = new QPushButton("Editar Función");
    btnsFun->addWidget(btnEditarFun);
    btnsFun->addWidget(btnEliminarFun);
    btnsFun->addWidget(btnCrearFun);
    btnsFun->addWidget(btnCargarFun);
    btnsFun->addWidget(btnRefrescarFun);
    btnsFun->addStretch();

    lblFuncionActiva = new QLabel("Función activa: Ninguna");
    lblFuncionActiva->setStyleSheet("color: #2196f3; font-weight: bold; padding: 5px;");

    mapaVisualAdmin = new QTableWidget();
    mapaVisualAdmin->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addLayout(btnsFun);
    layout->addWidget(tablaFuncionesAdmin);
    layout->addWidget(lblFuncionActiva);
    layout->addWidget(new QLabel("Mapa de Asientos:"));
    layout->addWidget(mapaVisualAdmin);

    connect(btnRefrescarFun, &QPushButton::clicked, [this]() {
        refrescarTablaFunciones();
    });

    connect(btnCrearFun, &QPushButton::clicked, [this]() {
        bool ok;
        QString codFun = QInputDialog::getText(this, "Función", "Código función (ej. F001):", QLineEdit::Normal, "", &ok);
        if (!ok || codFun.isEmpty()) return;
        QString codPel = QInputDialog::getText(this, "Función", "Código película:", QLineEdit::Normal, "", &ok);
        if (!ok || codPel.isEmpty()) return;
        QString horario = QInputDialog::getText(this, "Función", "Horario:", QLineEdit::Normal, "17:00", &ok);
        if (!ok) return;
        QString sala = QInputDialog::getText(this, "Función", "Sala:", QLineEdit::Normal, "Sala 1", &ok);
        if (!ok) return;
        int filas = QInputDialog::getInt(this, "Función", "Filas:", 5, 1, 50, 1, &ok);
        if (!ok) return;
        int cols = QInputDialog::getInt(this, "Función", "Columnas:", 8, 1, 50, 1, &ok);
        if (!ok) return;

        // NUEVO: pedir fecha de la función
        QString fecha = QInputDialog::getText(this, "Función", "Fecha (yyyy-MM-dd):",
                                              QLineEdit::Normal,
                                              QDate::currentDate().toString("yyyy-MM-dd"), &ok);
        if (!ok) return;

        std::string archivo = codFun.toStdString() + "_funcion.json";
        funciones.insertar(codFun.toStdString(), codPel.toStdString(), horario.toStdString(),
                           sala.toStdString(), filas, cols, archivo, fecha.toStdString());

        QString ruta = QCoreApplication::applicationDirPath() + "/asientos/" + QString::fromStdString(archivo);
        MatrizDispersa::crearArchivoVacio(codFun.toStdString(), ruta.toStdString());

        refrescarTablaFunciones();
        QMessageBox::information(this, "Éxito", "Función creada en el AVL y archivo JSON generado.");
    });

    connect(btnCargarFun, &QPushButton::clicked, [this]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Cargar", "Código de función:", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;
        cargarFuncionEnMatriz(cod.toStdString());
        refrescarMapaAsientosAdmin();
    });

    connect(btnEliminarFun, &QPushButton::clicked, [this]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Eliminar Función",
                                            "Código de la función a eliminar:", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;

        Funcion* fun = funciones.buscar(cod.toStdString());
        if (!fun) {
            QMessageBox::warning(this, "Error", "Función no encontrada.");
            return;
        }

        // Si es la función activa, liberar la matriz
        if (funcionActivaCodigo == cod.toStdString()) {
            if (matrizActual) {
                delete matrizActual;
                matrizActual = nullptr;
            }
            funcionActivaCodigo = "";
            if (lblFuncionActiva) lblFuncionActiva->setText("Función activa: Ninguna");
            if (mapaVisualAdmin) {
                mapaVisualAdmin->setRowCount(0);
                mapaVisualAdmin->setColumnCount(0);
            }
        }

        // Borrar el archivo JSON de asientos
        QString ruta = QCoreApplication::applicationDirPath()
                       + "/asientos/" + QString::fromStdString(fun->archivo_asientos);
        QFile::remove(ruta);

        // Eliminar del AVL
        funciones.eliminar(cod.toStdString());
        refrescarTablaFunciones();

        QMessageBox::information(this, "Eliminada",
                                 "Función eliminada del AVL y archivo JSON borrado.");
    });

    connect(btnEditarFun, &QPushButton::clicked, [this]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Editar Función", "Código de función:", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;

        Funcion* fun = funciones.buscar(cod.toStdString());
        if (!fun) {
            QMessageBox::warning(this, "Error", "Función no encontrada.");
            return;
        }

        QString horario = QInputDialog::getText(this, "Editar", "Horario:", QLineEdit::Normal,
                                                QString::fromStdString(fun->horario), &ok);
        if (!ok) return;
        QString sala = QInputDialog::getText(this, "Editar", "Sala:", QLineEdit::Normal,
                                             QString::fromStdString(fun->sala), &ok);
        if (!ok) return;

        // Solo editamos horario y sala (filas/columnas no se cambian para no romper asientos)
        fun->horario = horario.toStdString();
        fun->sala = sala.toStdString();

        refrescarTablaFunciones();
        QMessageBox::information(this, "OK", "Función actualizada.");
    });

    return tab;
}

QWidget* MainWindow::pestanaReportesVisuales() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* topBtns = new QHBoxLayout();
    QPushButton* btnBST = new QPushButton("1. BST Películas");
    QPushButton* btnAVL = new QPushButton("2. AVL Funciones");
    QPushButton* btnB = new QPushButton("3. Árbol B Clientes");
    QPushButton* btnHash = new QPushButton("4. Tabla Hash");
    QPushButton* btnMatriz = new QPushButton("5. Matriz Asientos");
    topBtns->addWidget(btnBST);
    topBtns->addWidget(btnAVL);
    topBtns->addWidget(btnB);
    topBtns->addWidget(btnHash);
    topBtns->addWidget(btnMatriz);

    QLabel* lblViewer = new QLabel("Selecciona un reporte.");
    lblViewer->setAlignment(Qt::AlignCenter);
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidget(lblViewer);
    scroll->setWidgetResizable(true);

    layout->addLayout(topBtns);
    layout->addWidget(scroll);

    QString appPath = QCoreApplication::applicationDirPath();
    auto cargar = [lblViewer, appPath](const QString& nombre) {
        QPixmap pix(appPath + "/reportes/" + nombre);
        if (pix.isNull())
            lblViewer->setText("No se encontró:\n" + appPath + "/reportes/" + nombre);
        else {
            lblViewer->setPixmap(pix);
            lblViewer->adjustSize();
        }
    };

    connect(btnBST, &QPushButton::clicked, [cargar]() { cargar("Cartelera_BST.png"); });
    connect(btnAVL, &QPushButton::clicked, [cargar]() { cargar("AVL_Funciones.png"); });
    connect(btnB, &QPushButton::clicked, [cargar]() { cargar("ArbolB_Clientes.png"); });
    connect(btnHash, &QPushButton::clicked, [cargar]() { cargar("TablaHash_Reservas.png"); });
    connect(btnMatriz, &QPushButton::clicked, [cargar]() { cargar("Matriz_Asientos.png"); });

    return tab;
}

void MainWindow::refrescarSolicitudesAdmin() {
    if (!lblDetalleSolicitudAdmin) return;
    Solicitud* s = solicitudes.getPrimera();
    if (s) {
        lblDetalleSolicitudAdmin->setText(
            QString("Solicitud #%1 | Cliente: %2\nTeléfono: %3\nTipo: %4\nDescripción: %5\nEstado: %6")
                .arg(s->numero)
                .arg(QString::fromStdString(s->cliente))
                .arg(QString::fromStdString(s->telefono))
                .arg(QString::fromStdString(s->tipo))
                .arg(QString::fromStdString(s->descripcion))
                .arg(QString::fromStdString(s->estado)));
    } else {
        lblDetalleSolicitudAdmin->setText("No hay solicitudes pendientes.");
    }
    if (lblTotalSolicitudesAdmin)
        lblTotalSolicitudesAdmin->setText(QString("Total Pendientes: %1").arg(solicitudes.getTotalPendientes()));
}

void MainWindow::refrescarMapaAsientosAdmin() {
    if (!mapaVisualAdmin || !matrizActual) return;
    int f = matrizActual->getFilas();
    int c = matrizActual->getColumnas();
    mapaVisualAdmin->setRowCount(f);
    mapaVisualAdmin->setColumnCount(c);
    for (int r = 1; r <= f; r++) {
        for (int col = 1; col <= c; col++) {
            NodoMatriz* nodo = matrizActual->buscarAsiento(r, col);
            QTableWidgetItem* item = new QTableWidgetItem();
            if (nodo && nodo->ocupado) {
                item->setText(QString("F%1-C%2\nOCUPADO\n(%3)").arg(r).arg(col).arg(QString::fromStdString(nodo->cliente)));
                item->setBackground(QBrush(QColor(198, 40, 40)));
                item->setForeground(QBrush(Qt::white));
            } else {
                item->setText(QString("F%1-C%2\nLIBRE").arg(r).arg(col));
                item->setBackground(QBrush(QColor(46, 125, 50)));
                item->setForeground(QBrush(Qt::white));
            }
            item->setTextAlignment(Qt::AlignCenter);
            mapaVisualAdmin->setItem(r - 1, col - 1, item);
        }
    }
}

// ============================================================
// PANEL CLIENTE
// ============================================================
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

    QTabWidget* tabs = new QTabWidget();
    tabs->addTab(pestanaClienteCartelera(), "Ver Cartelera");
    tabs->addTab(pestanaClienteReservas(), "Reservar / Cancelar");
    tabs->addTab(pestanaClientePromociones(), "Promociones");
    tabs->addTab(pestanaClienteSolicitudes(), "Solicitudes");
    tabs->addTab(pestanaClientePerfil(), "Mi Perfil");

    layout->addLayout(header);
    layout->addWidget(tabs);

    connect(btnLogout, &QPushButton::clicked, [this]() {
        clienteActual = nullptr;
        panelPrincipal->setCurrentIndex(0);
    });

    return vista;
}

QWidget* MainWindow::pestanaClienteCartelera() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QPushButton* btnVer = new QPushButton("Actualizar Cartelera");
    QTableWidget* tabla = new QTableWidget(0, 9);
    tabla->setHorizontalHeaderLabels({"CÓDIGO", "TÍTULO", "GÉNERO", "DURACIÓN", "CLASIFICACIÓN", "IDIOMA", "ESTRENO", "FIN", "ESTADO"});
    tabla->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addWidget(btnVer);
    layout->addWidget(tabla);

    connect(btnVer, &QPushButton::clicked, [this, tabla]() {
        cartelera.cargarEnTabla(tabla);
    });

    return tab;
}

QWidget* MainWindow::pestanaClienteReservas() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // ----- Cargar función -----
    QLabel* lbl = new QLabel("1. Carga una función para ver el mapa de asientos");
    QLineEdit* txtCodFun = new QLineEdit();
    txtCodFun->setPlaceholderText("Código de función (ej. F001)");
    QPushButton* btnCargar = new QPushButton("Cargar Función");

    QTableWidget* mapa = new QTableWidget();
    mapa->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // ----- Reservar -----
    QHBoxLayout* inputs = new QHBoxLayout();
    QLineEdit* txtFila = new QLineEdit(); txtFila->setPlaceholderText("Fila");
    QLineEdit* txtCol  = new QLineEdit(); txtCol->setPlaceholderText("Columna");
    QPushButton* btnReservar = new QPushButton("Reservar Asiento");
    inputs->addWidget(txtFila);
    inputs->addWidget(txtCol);
    inputs->addWidget(btnReservar);

    // ----- Historial + Cancelar -----
    QHBoxLayout* histBtns = new QHBoxLayout();
    QPushButton* btnHistorial = new QPushButton("Ver mi historial de reservas");
    QPushButton* btnCancelar  = new QPushButton("Cancelar reserva seleccionada");
    btnHistorial->setStyleSheet("background-color: #1565c0; color: white; font-weight: bold; padding: 8px;");
    btnCancelar->setStyleSheet("background-color: #c62828; color: white; font-weight: bold; padding: 8px;");
    histBtns->addWidget(btnHistorial);
    histBtns->addWidget(btnCancelar);

    QTableWidget* tablaHistorial = new QTableWidget(0, 6);
    tablaHistorial->setHorizontalHeaderLabels({"Código", "Función", "Fecha", "Fila", "Columna", "Estado"});
    tablaHistorial->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tablaHistorial->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablaHistorial->setSelectionMode(QAbstractItemView::SingleSelection);

    layout->addWidget(lbl);
    layout->addWidget(txtCodFun);
    layout->addWidget(btnCargar);
    layout->addWidget(mapa);
    layout->addLayout(inputs);
    layout->addSpacing(15);
    layout->addLayout(histBtns);
    layout->addWidget(tablaHistorial);

    // ----- Lambda refrescar mapa -----
    auto refrescarMapa = [this, mapa]() {
        if (!matrizActual) {
            mapa->setRowCount(0);
            mapa->setColumnCount(0);
            return;
        }
        int f = matrizActual->getFilas();
        int c = matrizActual->getColumnas();
        mapa->setRowCount(f);
        mapa->setColumnCount(c);
        for (int r = 1; r <= f; r++) {
            for (int col = 1; col <= c; col++) {
                NodoMatriz* n = matrizActual->buscarAsiento(r, col);
                QTableWidgetItem* item = new QTableWidgetItem();
                if (n && n->ocupado) {
                    item->setText(QString("F%1-C%2\n[X]").arg(r).arg(col));
                    item->setBackground(QBrush(QColor(198, 40, 40)));
                    item->setForeground(QBrush(Qt::white));
                } else {
                    item->setText(QString("F%1-C%2\n[LIBRE]").arg(r).arg(col));
                    item->setBackground(QBrush(QColor(46, 125, 50)));
                    item->setForeground(QBrush(Qt::white));
                }
                item->setTextAlignment(Qt::AlignCenter);
                mapa->setItem(r - 1, col - 1, item);
            }
        }
    };

    // ----- Lambda refrescar historial -----
    auto refrescarHistorial = [this, tablaHistorial]() {
        tablaHistorial->setRowCount(0);
        if (!clienteActual) return;

        int fila = 0;
        NodoCodigoReserva* nodo = clienteActual->listaReservas;
        while (nodo != nullptr) {
            Reserva* res = reservas.buscar(nodo->codigo_reserva);
            if (res != nullptr) {
                tablaHistorial->insertRow(fila);

                QDate fechaRes = QDate::fromString(QString::fromStdString(res->fecha_reserva), "yyyy-MM-dd");
                QString estado = "VALIDA";
                if (fechaRes.isValid() && fechaRes < QDate::currentDate())
                    estado = "VENCIDA";

                tablaHistorial->setItem(fila, 0, new QTableWidgetItem(QString::fromStdString(res->codigo_reserva)));
                tablaHistorial->setItem(fila, 1, new QTableWidgetItem(QString::fromStdString(res->codigo_funcion)));
                tablaHistorial->setItem(fila, 2, new QTableWidgetItem(QString::fromStdString(res->fecha_reserva)));
                tablaHistorial->setItem(fila, 3, new QTableWidgetItem(QString::number(res->fila)));
                tablaHistorial->setItem(fila, 4, new QTableWidgetItem(QString::number(res->columna)));
                tablaHistorial->setItem(fila, 5, new QTableWidgetItem(estado));

                if (estado == "VENCIDA") {
                    tablaHistorial->item(fila, 5)->setBackground(QBrush(QColor(198, 40, 40)));
                    tablaHistorial->item(fila, 5)->setForeground(QBrush(Qt::white));
                } else {
                    tablaHistorial->item(fila, 5)->setBackground(QBrush(QColor(46, 125, 50)));
                    tablaHistorial->item(fila, 5)->setForeground(QBrush(Qt::white));
                }
                fila++;
            }
            nodo = nodo->siguiente;
        }
    };

    // ----- Cargar función -----
    connect(btnCargar, &QPushButton::clicked, [this, txtCodFun, refrescarMapa]() {
        QString cod = txtCodFun->text().trimmed();
        if (cod.isEmpty()) return;
        cargarFuncionEnMatriz(cod.toStdString());
        refrescarMapa();
    });

    // ----- Reservar -----
    connect(btnReservar, &QPushButton::clicked, [this, txtFila, txtCol, refrescarMapa, refrescarHistorial]() {
        if (!matrizActual || !clienteActual) {
            QMessageBox::warning(this, "Error", "Debes estar logueado y tener una función cargada.");
            return;
        }
        int f = txtFila->text().toInt();
        int c = txtCol->text().toInt();
        if (f <= 0 || c <= 0) {
            QMessageBox::warning(this, "Error", "Fila y columna deben ser números válidos.");
            return;
        }

        static int contRes = 1;
        std::string codRes = "R" + std::to_string(1000 + contRes++);

        if (matrizActual->reservarAsiento(f, c, codRes)) {
            Reserva* res = new Reserva(
                codRes, clienteActual->id, funcionActivaCodigo, f, c,
                QDate::currentDate().toString("yyyy-MM-dd").toStdString()
                );
            reservas.insertar(res);
            clienteActual->agregarCodigoReserva(codRes);
            guardarMatrizActual();
            refrescarMapa();
            refrescarHistorial();
            QMessageBox::information(this, "Reserva OK",
                                     QString("Asiento reservado.\nCódigo: %1").arg(QString::fromStdString(codRes)));
        } else {
            QMessageBox::warning(this, "Ocupado", "Ese asiento no está disponible.");
        }
    });

    // ----- Ver historial -----
    connect(btnHistorial, &QPushButton::clicked, [this, refrescarHistorial]() {
        if (!clienteActual) {
            QMessageBox::warning(this, "Error", "No hay cliente autenticado.");
            return;
        }
        refrescarHistorial();
        if (clienteActual->listaReservas == nullptr)
            QMessageBox::information(this, "Historial", "No tienes reservas registradas.");
    });

    // ----- CANCELAR RESERVA -----
    connect(btnCancelar, &QPushButton::clicked, [this, tablaHistorial, refrescarMapa, refrescarHistorial]() {
        if (!clienteActual) {
            QMessageBox::warning(this, "Error", "No hay cliente autenticado.");
            return;
        }

        int filaSel = tablaHistorial->currentRow();
        if (filaSel < 0) {
            QMessageBox::warning(this, "Error", "Selecciona una reserva de la tabla para cancelar.");
            return;
        }

        QString codReserva = tablaHistorial->item(filaSel, 0)->text();
        Reserva* res = reservas.buscar(codReserva.toStdString());
        if (res == nullptr) {
            QMessageBox::warning(this, "Error", "Reserva no encontrada.");
            return;
        }

        // Guardar datos antes de eliminar
        std::string codFun = res->codigo_funcion;
        int filaAsiento = res->fila;
        int colAsiento  = res->columna;

        // 1. Cargar la función si no está activa
        if (funcionActivaCodigo != codFun) {
            cargarFuncionEnMatriz(codFun);
        }

        // 2. Liberar asiento + guardar JSON
        if (matrizActual) {
            matrizActual->cancelarReserva(filaAsiento, colAsiento, "");
            guardarMatrizActual();
        }

        // 3. Eliminar de Hash
        reservas.eliminar(codReserva.toStdString());

        // 4. Quitar referencia del cliente
        clienteActual->eliminarCodigoReserva(codReserva.toStdString());

        refrescarMapa();
        refrescarHistorial();

        QMessageBox::information(this, "Cancelada",
                                 QString("Reserva %1 cancelada.\nAsiento F%2-C%3 liberado.")
                                     .arg(codReserva).arg(filaAsiento).arg(colAsiento));
    });

    return tab;
}

QWidget* MainWindow::pestanaClientePromociones() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    QPushButton* btn = new QPushButton("Ver Promociones");
    QTreeWidget* tree = new QTreeWidget();
    tree->setColumnCount(4);
    tree->setHeaderLabels({"CÓDIGO", "PROMOCIÓN", "VIGENCIA", "DÍAS"});
    tree->header()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(btn);
    layout->addWidget(tree);
    connect(btn, &QPushButton::clicked, [this, tree]() {
        promociones.cargarEnTreeWidget(tree);
    });
    return tab;
}

QWidget* MainWindow::pestanaClienteSolicitudes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLineEdit* txtNombre = new QLineEdit(); txtNombre->setPlaceholderText("Nombre");
    QLineEdit* txtTel = new QLineEdit(); txtTel->setPlaceholderText("Teléfono");
    QLineEdit* txtTipo = new QLineEdit(); txtTipo->setPlaceholderText("Tipo");
    QLineEdit* txtDesc = new QLineEdit(); txtDesc->setPlaceholderText("Descripción");
    QPushButton* btnEnviar = new QPushButton("Enviar Solicitud");

    layout->addWidget(txtNombre);
    layout->addWidget(txtTel);
    layout->addWidget(txtTipo);
    layout->addWidget(txtDesc);
    layout->addWidget(btnEnviar);
    layout->addStretch();

    connect(btnEnviar, &QPushButton::clicked, [this, txtNombre, txtTel, txtTipo, txtDesc]() {
        if (txtNombre->text().isEmpty() || txtTel->text().isEmpty()) {
            QMessageBox::warning(this, "Error", "Completa nombre y teléfono.");
            return;
        }
        solicitudes.insertar(txtNombre->text().toStdString(), txtTel->text().toStdString(),
                             txtTipo->text().toStdString(), txtDesc->text().toStdString(),
                             QDate::currentDate().toString("yyyy-MM-dd").toStdString());
        refrescarSolicitudesAdmin();
        txtNombre->clear(); txtTel->clear(); txtTipo->clear(); txtDesc->clear();
        QMessageBox::information(this, "OK", "Solicitud enviada.");
    });

    return tab;
}

// ============================================================
// HELPERS FASE 2
// ============================================================
void MainWindow::guardarMatrizActual() {
    if (matrizActual == nullptr || funcionActivaCodigo.empty()) return;
    Funcion* fun = funciones.buscar(funcionActivaCodigo);
    if (fun == nullptr) return;
    QString ruta = QCoreApplication::applicationDirPath()
                   + "/asientos/" + QString::fromStdString(fun->archivo_asientos);
    matrizActual->guardarAJson(funcionActivaCodigo, ruta.toStdString());
}

void MainWindow::cargarFuncionEnMatriz(const std::string& codigoFuncion) {
    if (matrizActual != nullptr && !funcionActivaCodigo.empty()) {
        guardarMatrizActual();
        delete matrizActual;
        matrizActual = nullptr;
    }

    Funcion* fun = funciones.buscar(codigoFuncion);
    if (fun == nullptr) {
        QMessageBox::warning(this, "Error", "Función no encontrada.");
        return;
    }

    matrizActual = new MatrizDispersa(fun->filas, fun->columnas);
    matrizActual->setDatosFuncion(fun->codigo_pelicula, fun->horario, fun->sala);
    funcionActivaCodigo = codigoFuncion;

    QString ruta = QCoreApplication::applicationDirPath()
                   + "/asientos/" + QString::fromStdString(fun->archivo_asientos);
    matrizActual->cargarDesdeJson(ruta.toStdString());

    if (lblFuncionActiva) {
        lblFuncionActiva->setText(
            QString("Función activa: %1 | %2 | %3")
                .arg(QString::fromStdString(fun->codigo_funcion))
                .arg(QString::fromStdString(fun->horario))
                .arg(QString::fromStdString(fun->sala)));
    }
}

void MainWindow::refrescarTablaFunciones() {
    if (tablaFuncionesAdmin)
        funciones.cargarEnTabla(tablaFuncionesAdmin, "inorden");
}

void MainWindow::refrescarTablaClientes() {
    if (tablaClientesAdmin)
        clientes.cargarEnTabla(tablaClientesAdmin);
}