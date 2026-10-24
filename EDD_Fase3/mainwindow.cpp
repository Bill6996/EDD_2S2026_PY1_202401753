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
#include <QDialog>
#include <QTimer>
#include <QDate>
#include <QStatusBar>

// Precio fijo por boleto (el enunciado no define precios por función)
static const double PRECIO_BOLETO = 45.0;

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    matrizActual(nullptr),
    contadorReservas(1),
    clienteActual(nullptr),
    funcionActivaCodigo(""),
    tablaCarteleraAdmin(nullptr),
    tablaFuncionesAdmin(nullptr),
    tablaClientesAdmin(nullptr),
    mapaVisualAdmin(nullptr),
    tablaSedesAdmin(nullptr),
    tablaBloquesAdmin(nullptr),
    lblEstadoBlockchain(nullptr),
    lblDetalleSolicitudAdmin(nullptr),
    lblTotalSolicitudesAdmin(nullptr),
    lblFuncionActiva(nullptr)
{
    this->setWindowTitle("CinemaUSAC - Fase 3");
    this->resize(1200, 800);

    QDir().mkpath(QCoreApplication::applicationDirPath() + "/asientos");
    QDir().mkpath(QCoreApplication::applicationDirPath() + "/reportes");

    // ===== FASE 3: restaurar el estado guardado en la ejecución anterior =====
    // 1) Sedes, películas y funciones se descomprimen (LZW) desde sedes.edd / peliculas.edd / funciones.edd
    //    y se vuelven a insertar en el grafo, BST y AVL. Las aristas se recalculan a partir de las funciones.
    int nSedes = 0, nPeliculas = 0, nFunciones = 0;
    Persistencia::cargarTodo(grafo, cartelera, funciones, nSedes, nPeliculas, nFunciones);

    // 2) Blockchain: se leen los bloques de blocks/, se verifica la integridad de cada uno
    //    y la Tabla Hash de reservas se reconstruye usando SOLO el DATA de los bloques válidos.
    blockchain.cargarDesdeDisco();
    blockchain.reconstruirTablaHash(reservas);
    if (blockchain.cantidadBloques() > 0)
        blockchain.generarReporteBlockchain();      // reporte automático (verde = íntegro, rojo = manipulado)

    panelPrincipal = new QStackedWidget(this);
    panelPrincipal->addWidget(crearPanelLogin());
    panelPrincipal->addWidget(crearPanelAdmin());
    panelPrincipal->addWidget(crearPanelUsuario());

    setCentralWidget(panelPrincipal);

    // Mostrar de inmediato lo restaurado desde los .edd y los bloques
    if (tablaCarteleraAdmin) cartelera.cargarEnTabla(tablaCarteleraAdmin);
    refrescarTablaFunciones();
    refrescarTablaClientes();
    refrescarTablaSedes();
    refrescarTablaBloques();

    // El aviso de bloques manipulados se muestra cuando la ventana ya está lista
    QTimer::singleShot(0, this, [this]() { verificarBlockchainAlIniciar(); });
}

MainWindow::~MainWindow() {
    if (matrizActual && !funcionActivaCodigo.empty()) {
        guardarMatrizActual();
    }
    if (matrizActual) {
        delete matrizActual;
        matrizActual = nullptr;
    }

    // ===== FASE 3: persistencia al cerrar =====
    // Las compras que aún esperan bloque se registran para no perderlas.
    if (blockchain.cantidadPendientes() > 0) {
        std::string error;
        blockchain.generarBloque(error);
    }
    // Estado ACTUAL de las estructuras en memoria -> texto -> LZW -> .edd (sin JSON intermedio)
    Persistencia::guardarTodo(grafo, cartelera, funciones);
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

            // FASE 3: los clientes se vuelven a cargar vacíos al abrir el programa, pero sus reservas
            // vigentes siguen en la Tabla Hash (reconstruida desde la blockchain). Se reconectan a su lista.
            blockchain.recorrerTransaccionesValidas([&](const Transaccion& t, const Bloque*) {
                if (t.id_cliente != cli->id || t.estado == "CANCELADA") return;
                if (reservas.buscar(t.codigo_reserva) == nullptr) return;    // cancelada después
                for (NodoCodigoReserva* n = cli->listaReservas; n != nullptr; n = n->siguiente)
                    if (n->codigo_reserva == t.codigo_reserva) return;        // ya estaba
                cli->agregarCodigoReserva(t.codigo_reserva);
            });
            txtCorreo->clear();
            txtPassword->clear();
            // Los combos de película/sede se llenaron al crear la ventana: se refrescan para ver
            // lo que el administrador cargó después (pulsa los botones "Actualizar lista(s)" del panel)
            for (QPushButton* b : panelPrincipal->widget(2)->findChildren<QPushButton*>())
                if (b->text() == "Actualizar lista" || b->text() == "Actualizar listas") b->click();
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
    tabsAdmin->addTab(pestanaGestionSedes(), "Sedes (Grafo)");           // NUEVA Fase 3
    tabsAdmin->addTab(pestanaGestionFunciones(), "Funciones y Asientos");
    tabsAdmin->addTab(pestanaGestionClientes(), "Clientes");           // NUEVA
    tabsAdmin->addTab(pestanaBlockchainAdmin(), "Blockchain");          // NUEVA Fase 3
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
        generarReportesFase3();      // grafo, lista de adyacencia, blockchain y Merkle

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

    tablaClientesAdmin = new QTableWidget(0, 7);
    tablaClientesAdmin->setHorizontalHeaderLabels(
        {"ID", "Nombre", "Correo", "Teléfono", "Tipo", "N° Reservas", "Password"});

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

        CargadorJSON::ResultadoCarga r =
            CargadorJSON::cargarPeliculasYFunciones(ruta.toStdString(), cartelera, funciones, grafo);
        if (tablaCarteleraAdmin) cartelera.cargarEnTabla(tablaCarteleraAdmin);
        refrescarGrafo();

        QString msg = QString("Sedes cargadas: %1\nPelículas cargadas: %2\nFunciones cargadas: %3\nFunciones rechazadas: %4")
                          .arg(r.sedes).arg(r.peliculas).arg(r.funciones).arg(r.rechazadas);
        if (!r.errores.isEmpty()) {
            msg += "\n\nErrores reportados:\n";
            int mostrados = 0;
            for (const QString& e : r.errores) {
                if (mostrados++ >= 15) { msg += QString("... y %1 más (ver consola)").arg(r.errores.size() - 15); break; }
                msg += "• " + e + "\n";
            }
        }
        QMessageBox::information(this, "Carga completada", msg);
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
    tablaFuncionesAdmin = new QTableWidget(0, 8);
    tablaFuncionesAdmin->setHorizontalHeaderLabels({"Código", "Película", "Horario", "Sala", "Filas", "Columnas", "Archivo", "Sede"});
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

    // CREAR CONEXIÓN: crear función + asignarla a una sede. Al guardarla se recalculan las aristas del grafo.
    connect(btnCrearFun, &QPushButton::clicked, [this]() {
        if (grafo.cantidadSedes() == 0) {
            QMessageBox::warning(this, "Sin sedes",
                                 "Primero registra al menos una sede (pestaña \"Sedes (Grafo)\").");
            return;
        }
        bool ok;
        QString codFun = QInputDialog::getText(this, "Función", "Código función (ej. F001):", QLineEdit::Normal, "", &ok);
        if (!ok || codFun.isEmpty()) return;
        if (funciones.buscar(codFun.toStdString()) != nullptr) {
            QMessageBox::warning(this, "Error", "Ya existe una función con ese código.");
            return;
        }
        QString codPel = QInputDialog::getText(this, "Función", "Código película:", QLineEdit::Normal, "", &ok);
        if (!ok || codPel.isEmpty()) return;
        if (cartelera.buscar(codPel.toStdString()) == nullptr) {
            QMessageBox::warning(this, "Error", "Esa película no existe en la cartelera.");
            return;
        }
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

        // FASE 3: la sede es obligatoria (la función es la "conexión" entre película y sede)
        QString sede = elegirSede("Función", "Sede donde se proyectará:");
        if (sede.isEmpty()) return;

        std::string archivo = codFun.toStdString() + "_funcion.json";
        funciones.insertar(codFun.toStdString(), codPel.toStdString(), horario.toStdString(),
                           sala.toStdString(), filas, cols, archivo, fecha.toStdString(),
                           sede.toStdString());

        QString ruta = QCoreApplication::applicationDirPath() + "/asientos/" + QString::fromStdString(archivo);
        MatrizDispersa::crearArchivoVacio(codFun.toStdString(), ruta.toStdString());

        refrescarGrafo();     // recalcula las aristas del grafo con la nueva función
        QMessageBox::information(this, "Éxito",
                                 "Función creada en el AVL y asignada a la sede " + sede +
                                 ".\nSe recalcularon las aristas del grafo.");
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

        if (funciones.buscar(cod.toStdString()) == nullptr) {
            QMessageBox::warning(this, "Error", "Función no encontrada.");
            return;
        }

        eliminarFuncionInterna(cod.toStdString());
        refrescarGrafo();     // la eliminación puede bajar el peso de una arista o quitarla

        QMessageBox::information(this, "Eliminada",
                                 "Función eliminada del AVL y archivo JSON borrado.\nSe recalcularon las aristas del grafo.");
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

        // FASE 3: también se puede cambiar la película y la sede (esto modifica las aristas del grafo)
        QString codPel = QInputDialog::getText(this, "Editar", "Código película:", QLineEdit::Normal,
                                               QString::fromStdString(fun->codigo_pelicula), &ok);
        if (!ok) return;
        if (cartelera.buscar(codPel.toStdString()) == nullptr) {
            QMessageBox::warning(this, "Error", "Esa película no existe en la cartelera.");
            return;
        }
        QString sede = elegirSede("Editar Función", "Sede:", QString::fromStdString(fun->codigo_sede));
        if (sede.isEmpty()) return;

        // Solo editamos horario, sala, película y sede (filas/columnas no se cambian para no romper asientos)
        fun->horario = horario.toStdString();
        fun->sala = sala.toStdString();
        fun->codigo_pelicula = codPel.toStdString();
        fun->codigo_sede = sede.toStdString();
        if (matrizActual && funcionActivaCodigo == fun->codigo_funcion)
            matrizActual->setDatosFuncion(fun->codigo_pelicula, fun->horario, fun->sala);

        refrescarGrafo();     // recalcula las aristas
        QMessageBox::information(this, "OK", "Función actualizada.\nSe recalcularon las aristas del grafo.");
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

    // FASE 3: reportes nuevos (se regeneran al pulsar para mostrar siempre el estado actual)
    QHBoxLayout* topBtns2 = new QHBoxLayout();
    QPushButton* btnGrafo = new QPushButton("6. Grafo de Sedes");
    QPushButton* btnAdy = new QPushButton("7. Lista de Adyacencia");
    QPushButton* btnChain = new QPushButton("8. Blockchain");
    QPushButton* btnMerkle = new QPushButton("9. Árbol de Merkle");
    topBtns2->addWidget(btnGrafo);
    topBtns2->addWidget(btnAdy);
    topBtns2->addWidget(btnChain);
    topBtns2->addWidget(btnMerkle);

    QLabel* lblViewer = new QLabel("Selecciona un reporte.");
    lblViewer->setAlignment(Qt::AlignCenter);
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidget(lblViewer);
    scroll->setWidgetResizable(true);

    layout->addLayout(topBtns);
    layout->addLayout(topBtns2);
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

    connect(btnGrafo, &QPushButton::clicked, [this, cargar]() {
        grafo.generarReporteGrafo();
        cargar("Grafo_Sedes.png");
    });
    connect(btnAdy, &QPushButton::clicked, [this, cargar]() {
        grafo.generarReporteListaAdyacencia();
        cargar("Lista_Adyacencia.png");
    });
    connect(btnChain, &QPushButton::clicked, [this, cargar]() {
        blockchain.generarReporteBlockchain();
        cargar("Blockchain.png");
    });
    connect(btnMerkle, &QPushButton::clicked, [this, cargar]() {
        int idx = elegirBloque("Árbol de Merkle");
        if (idx < 0) return;
        std::string error;
        blockchain.generarReporteMerkle(idx, error);
        cargar("Arbol_Merkle.png");
    });

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
    tabs->addTab(pestanaClienteSedes(), "Sedes (Grafo)");                // NUEVA Fase 3
    tabs->addTab(pestanaClienteReservas(), "Reservar / Cancelar");
    tabs->addTab(pestanaClienteCompras(), "Mis Compras (Blockchain)");   // NUEVA Fase 3
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

    // ========== 1. SELECCIÓN PELÍCULA + FUNCIÓN ==========
    QLabel* lblPaso1 = new QLabel("1. Selecciona película y función");
    lblPaso1->setFont(QFont("Arial", 11, QFont::Bold));

    QHBoxLayout* selLayout = new QHBoxLayout();

    QComboBox* cmbPeliculas = new QComboBox();
    cmbPeliculas->setMinimumWidth(220);
    cmbPeliculas->addItem("-- Seleccione película --", "");

    // FASE 3: película -> sede -> función
    QComboBox* cmbSedes = new QComboBox();
    cmbSedes->setMinimumWidth(220);
    cmbSedes->addItem("-- Seleccione sede --", "");

    QComboBox* cmbFunciones = new QComboBox();
    cmbFunciones->setMinimumWidth(220);
    cmbFunciones->addItem("-- Seleccione función --", "");

    QPushButton* btnCargarMapa = new QPushButton("Cargar mapa de asientos");
    btnCargarMapa->setStyleSheet("background-color: #1976d2; color: white; font-weight: bold; padding: 6px;");

    QPushButton* btnActualizarListas = new QPushButton("Actualizar listas");
    btnActualizarListas->setStyleSheet("background-color: #546e7a; color: white; padding: 6px;");

    selLayout->addWidget(new QLabel("Película:"));
    selLayout->addWidget(cmbPeliculas);
    selLayout->addWidget(new QLabel("Sede:"));
    selLayout->addWidget(cmbSedes);
    selLayout->addWidget(new QLabel("Función:"));
    selLayout->addWidget(cmbFunciones);
    selLayout->addWidget(btnCargarMapa);
    selLayout->addWidget(btnActualizarListas);

    QLabel* lblFuncionActivaCliente = new QLabel("Ninguna función cargada");
    lblFuncionActivaCliente->setStyleSheet("color: #ff9800; font-weight: bold; padding: 6px;");

    // ========== 2. MAPA DE ASIENTOS ==========
    QLabel* lblPaso2 = new QLabel("2. Mapa de asientos (verde = libre, rojo = ocupado)");
    lblPaso2->setFont(QFont("Arial", 11, QFont::Bold));

    QTableWidget* mapa = new QTableWidget();
    mapa->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // ========== 3. RESERVAR ==========
    QHBoxLayout* inputs = new QHBoxLayout();
    QLineEdit* txtFila = new QLineEdit();
    txtFila->setPlaceholderText("Fila");
    txtFila->setFixedWidth(80);
    QLineEdit* txtCol = new QLineEdit();
    txtCol->setPlaceholderText("Columna");
    txtCol->setFixedWidth(80);
    QPushButton* btnReservar = new QPushButton("Reservar Asiento");
    btnReservar->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold; padding: 6px;");
    inputs->addWidget(new QLabel("Fila:"));
    inputs->addWidget(txtFila);
    inputs->addWidget(new QLabel("Columna:"));
    inputs->addWidget(txtCol);
    inputs->addWidget(btnReservar);
    inputs->addStretch();

    // ========== 4. HISTORIAL + CANCELAR ==========
    QLabel* lblPaso3 = new QLabel("3. Historial de mis reservas");
    lblPaso3->setFont(QFont("Arial", 11, QFont::Bold));

    QHBoxLayout* histBtns = new QHBoxLayout();
    QPushButton* btnHistorial = new QPushButton("Ver mi historial");
    QPushButton* btnCancelar  = new QPushButton("Cancelar reserva seleccionada");
    btnHistorial->setStyleSheet("background-color: #1565c0; color: white; font-weight: bold; padding: 6px;");
    btnCancelar->setStyleSheet("background-color: #c62828; color: white; font-weight: bold; padding: 6px;");
    histBtns->addWidget(btnHistorial);
    histBtns->addWidget(btnCancelar);
    histBtns->addStretch();

    QTableWidget* tablaHistorial = new QTableWidget(0, 6);
    tablaHistorial->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tablaHistorial->setHorizontalHeaderLabels(
        {"Código", "Función", "Fecha", "Fila", "Columna", "Estado"});
    tablaHistorial->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tablaHistorial->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablaHistorial->setSelectionMode(QAbstractItemView::SingleSelection);

    // Layout general
    layout->addWidget(lblPaso1);
    layout->addLayout(selLayout);
    layout->addWidget(lblFuncionActivaCliente);
    layout->addWidget(lblPaso2);
    layout->addWidget(mapa);
    layout->addLayout(inputs);
    layout->addSpacing(10);
    layout->addWidget(lblPaso3);
    layout->addLayout(histBtns);
    layout->addWidget(tablaHistorial);

    // ========== LAMBDAS ==========

    // Refrescar combo de películas desde el BST
    auto refrescarPeliculas = [this, cmbPeliculas, cmbSedes, cmbFunciones]() {
        cmbPeliculas->blockSignals(true);
        llenarComboPeliculas(cmbPeliculas);
        cmbPeliculas->blockSignals(false);

        cmbSedes->clear();
        cmbSedes->addItem("-- Seleccione sede --", "");
        cmbFunciones->clear();
        cmbFunciones->addItem("-- Seleccione función --", "");
    };

    // Al cambiar la película -> llenar las SEDES (vértices del grafo) donde se proyecta
    auto refrescarSedesDePelicula = [this, cmbPeliculas, cmbSedes, cmbFunciones]() {
        cmbSedes->blockSignals(true);
        cmbSedes->clear();
        cmbSedes->addItem("-- Seleccione sede --", "");
        cmbFunciones->clear();
        cmbFunciones->addItem("-- Seleccione función --", "");

        QString codPel = cmbPeliculas->currentData().toString().trimmed();
        if (!codPel.isEmpty()) {
            grafo.recorrerSedes([&](const Sede& sd) {
                if (grafo.sedeTienePelicula(sd.codigo, codPel.toStdString()))
                    cmbSedes->addItem(QString::fromStdString(sd.codigo + " - " + sd.nombre),
                                      QString::fromStdString(sd.codigo));
            });
        }
        cmbSedes->blockSignals(false);
    };

    // Al cambiar la sede -> llenar las funciones de ESA película en ESA sede (AVL)
    auto refrescarFuncionesDePelicula = [this, cmbPeliculas, cmbSedes, cmbFunciones]() {
        cmbFunciones->clear();
        cmbFunciones->addItem("-- Seleccione función --", "");

        QString codPel = cmbPeliculas->currentData().toString().trimmed();
        QString codSede = cmbSedes->currentData().toString().trimmed();
        if (codPel.isEmpty() || codSede.isEmpty()) return;

        funciones.recorrerInorden([&](Funcion* f) {
            if (QString::compare(QString::fromStdString(f->codigo_pelicula), codPel, Qt::CaseInsensitive) == 0 &&
                f->codigo_sede == codSede.toStdString()) {
                cmbFunciones->addItem(QString::fromStdString(f->codigo_funcion + " | " + f->fecha + " " +
                                                             f->horario + " | " + f->sala),
                                      QString::fromStdString(f->codigo_funcion));
            }
        });
    };

    // Pintar mapa
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

    // Historial (solo B-Tree + Hash, sin tocar matriz)
    auto refrescarHistorial = [this, tablaHistorial]() {
        tablaHistorial->setRowCount(0);
        if (!clienteActual) return;

        int fila = 0;
        NodoCodigoReserva* nodo = clienteActual->listaReservas;
        while (nodo != nullptr) {
            Reserva* res = reservas.buscar(nodo->codigo_reserva);
            if (res != nullptr) {
                tablaHistorial->insertRow(fila);

                QDate fechaRes = QDate::fromString(
                    QString::fromStdString(res->fecha_reserva), "yyyy-MM-dd");
                QString estado = "VALIDA";
                // Estado según fecha de la FUNCIÓN si existe; si no, fecha de reserva
                Funcion* fun = funciones.buscar(res->codigo_funcion);
                if (fun && !fun->fecha.empty()) {
                    QDate fFun = QDate::fromString(QString::fromStdString(fun->fecha), "yyyy-MM-dd");
                    if (fFun.isValid() && fFun < QDate::currentDate())
                        estado = "VENCIDA";
                } else if (fechaRes.isValid() && fechaRes < QDate::currentDate()) {
                    estado = "VENCIDA";
                }

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

    // ========== CONNECTS ==========

    // Al mostrar el panel, cargar películas
    refrescarPeliculas();

    connect(btnActualizarListas, &QPushButton::clicked, [refrescarPeliculas]() {
        refrescarPeliculas();
    });

    connect(cmbPeliculas, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [refrescarSedesDePelicula](int) {
                refrescarSedesDePelicula();
            });

    connect(cmbSedes, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [refrescarFuncionesDePelicula](int) {
                refrescarFuncionesDePelicula();
            });

    // Cargar mapa al elegir función
    connect(btnCargarMapa, &QPushButton::clicked,
            [this, cmbFunciones, lblFuncionActivaCliente, refrescarMapa]() {
                QString codFun = cmbFunciones->currentData().toString();
                if (codFun.isEmpty()) {
                    QMessageBox::warning(this, "Atención", "Selecciona una función.");
                    return;
                }
                cargarFuncionEnMatriz(codFun.toStdString());
                lblFuncionActivaCliente->setText(
                    "Función cargada: " + codFun + "  |  " +
                    QString::fromStdString(matrizActual ? matrizActual->getInfoFuncion() : ""));
                refrescarMapa();
            });

    // Reservar = COMPRA DE BOLETO (Fase 3): método de pago + transacción para la blockchain
    connect(btnReservar, &QPushButton::clicked,
            [this, txtFila, txtCol, refrescarMapa, refrescarHistorial]() {
                if (!matrizActual || !clienteActual) {
                    QMessageBox::warning(this, "Error",
                                         "Debes estar logueado y tener una función cargada.");
                    return;
                }
                int f = txtFila->text().toInt();
                int c = txtCol->text().toInt();
                if (f <= 0 || c <= 0) {
                    QMessageBox::warning(this, "Error", "Fila y columna inválidas.");
                    return;
                }

                // 1) El cliente confirma el pago indicando el método (si cancela, no se reserva nada)
                bool okPago = false;
                QString metodo = QInputDialog::getItem(
                    this, "Confirmar pago",
                    QString("Asiento F%1-C%2 | Monto a pagar: Q%3\nMétodo de pago:").arg(f).arg(c).arg(PRECIO_BOLETO, 0, 'f', 2),
                    QStringList() << "Tarjeta" << "Efectivo", 0, false, &okPago);
                if (!okPago) return;

                // 2) Flujo existente: matriz dispersa + archivo de la función + Tabla Hash
                std::string codRes = siguienteCodigoReserva();
                if (!matrizActual->reservarAsiento(f, c, codRes)) {
                    QMessageBox::warning(this, "Ocupado", "Ese asiento no está disponible.");
                    return;
                }
                Reserva* res = new Reserva(
                    codRes, clienteActual->id, funcionActivaCodigo, f, c,
                    QDate::currentDate().toString("yyyy-MM-dd").toStdString());
                reservas.insertar(res);
                clienteActual->agregarCodigoReserva(codRes);
                guardarMatrizActual();

                // 3) NUEVO: se arma la transacción y se agrega a la cola de pendientes de la blockchain
                Transaccion t;
                t.codigo_transaccion = blockchain.siguienteCodigoTransaccion();
                t.codigo_reserva = codRes;
                t.id_cliente = clienteActual->id;
                t.codigo_funcion = funcionActivaCodigo;
                t.fila = f;
                t.columna = c;
                t.monto = PRECIO_BOLETO;
                t.metodo_pago = metodo.toStdString();
                t.fecha_pago = QDate::currentDate().toString("dd-MM-yyyy").toStdString();

                Bloque* bloqueAuto = nullptr;
                blockchain.encolarTransaccion(t, bloqueAuto);   // si ya hay 4 pendientes genera el bloque solo
                refrescarTablaBloques();

                refrescarMapa();
                refrescarHistorial();

                QString msg = QString("Compra realizada.\nReserva: %1\nTransacción: %2\nMonto: Q%3 (%4)")
                                  .arg(QString::fromStdString(codRes))
                                  .arg(QString::fromStdString(t.codigo_transaccion))
                                  .arg(PRECIO_BOLETO, 0, 'f', 2).arg(metodo);
                if (bloqueAuto)
                    msg += QString("\n\nSe generó automáticamente el bloque %1 (4 transacciones).").arg(bloqueAuto->index);
                else
                    msg += QString("\n\nTransacción pendiente de bloque (%1/%2).")
                               .arg(blockchain.cantidadPendientes()).arg(blockchain.getTamanoAutomatico());
                QMessageBox::information(this, "Compra OK", msg);
            });

    // Historial
    connect(btnHistorial, &QPushButton::clicked, [this, refrescarHistorial]() {
        if (!clienteActual) {
            QMessageBox::warning(this, "Error", "No hay cliente autenticado.");
            return;
        }
        refrescarHistorial();
        if (clienteActual->listaReservas == nullptr)
            QMessageBox::information(this, "Historial", "No tienes reservas registradas.");
    });

    // Cancelar
    connect(btnCancelar, &QPushButton::clicked,
            [this, tablaHistorial, refrescarMapa, refrescarHistorial]() {
                if (!clienteActual) {
                    QMessageBox::warning(this, "Error", "No hay cliente autenticado.");
                    return;
                }
                int filaSel = tablaHistorial->currentRow();
                if (filaSel < 0) {
                    QMessageBox::warning(this, "Error", "Selecciona una reserva de la tabla.");
                    return;
                }

                QString codReserva = tablaHistorial->item(filaSel, 0)->text();
                Reserva* res = reservas.buscar(codReserva.toStdString());
                if (!res) {
                    QMessageBox::warning(this, "Error", "Reserva no encontrada.");
                    return;
                }

                // FASE 3: si la compra ya está escrita en un bloque, ese bloque NO se modifica:
                // la misma transacción se vuelve a escribir con estado CANCELADA (en la cola hacia un bloque nuevo)
                // y esa segunda escritura actualiza a la primera.
                QString extraBloque;
                Transaccion original;
                if (blockchain.reservaPagadaEnBloque(codReserva.toStdString(), original)) {
                    Transaccion cancelacion = original;
                    cancelacion.estado = "CANCELADA";
                    Bloque* bloqueAuto = nullptr;
                    blockchain.encolarTransaccion(cancelacion, bloqueAuto);
                    extraBloque = QString("\n\nLa transacción %1 se registró de nuevo con estado CANCELADA. ")
                                      .arg(QString::fromStdString(original.codigo_transaccion));
                    if (bloqueAuto)
                        extraBloque += QString("Quedó escrita en el bloque %1.").arg(bloqueAuto->index);
                    else
                        extraBloque += "Quedará escrita en el siguiente bloque.";
                } else {
                    // Si todavía está pendiente en la cola, se retira de ahí
                    blockchain.cancelarPendiente(codReserva.toStdString());
                }
                refrescarTablaBloques();

                std::string codFun = res->codigo_funcion;
                int filaAsiento = res->fila;
                int colAsiento  = res->columna;

                if (funcionActivaCodigo != codFun)
                    cargarFuncionEnMatriz(codFun);

                if (matrizActual) {
                    matrizActual->cancelarReserva(filaAsiento, colAsiento, "");
                    guardarMatrizActual();
                }

                reservas.eliminar(codReserva.toStdString());
                clienteActual->eliminarCodigoReserva(codReserva.toStdString());

                refrescarMapa();
                refrescarHistorial();

                QMessageBox::information(this, "Cancelada",
                                         QString("Reserva %1 cancelada.\nAsiento F%2-C%3 liberado.%4")
                                             .arg(codReserva).arg(filaAsiento).arg(colAsiento).arg(extraBloque));
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
// ============================================================
// FASE 3 - HELPERS GENERALES
// ============================================================

// Recalcula las aristas del grafo (a partir de las funciones del AVL) y refresca las tablas que dependen de ello
void MainWindow::refrescarGrafo() {
    grafo.recalcularAristas(funciones);
    refrescarTablaSedes();
    refrescarTablaFunciones();
}

void MainWindow::refrescarTablaSedes() {
    if (!tablaSedesAdmin) return;
    tablaSedesAdmin->setRowCount(0);
    int fila = 0;
    grafo.recorrerSedes([&](const Sede& s) {
        int nFunciones = 0;
        funciones.recorrerInorden([&](Funcion* f) {
            if (f->codigo_sede == s.codigo) nFunciones++;
        });
        ListaParesSedes vecinas;
        grafo.sedesSimilares(s.codigo, vecinas);

        tablaSedesAdmin->insertRow(fila);
        tablaSedesAdmin->setItem(fila, 0, new QTableWidgetItem(QString::fromStdString(s.codigo)));
        tablaSedesAdmin->setItem(fila, 1, new QTableWidgetItem(QString::fromStdString(s.nombre)));
        tablaSedesAdmin->setItem(fila, 2, new QTableWidgetItem(QString::fromStdString(s.direccion)));
        tablaSedesAdmin->setItem(fila, 3, new QTableWidgetItem(QString::number(s.cantidad_salas)));
        tablaSedesAdmin->setItem(fila, 4, new QTableWidgetItem(QString::fromStdString(s.telefono)));
        tablaSedesAdmin->setItem(fila, 5, new QTableWidgetItem(QString::number(nFunciones)));
        tablaSedesAdmin->setItem(fila, 6, new QTableWidgetItem(QString::number(vecinas.cantidad)));
        fila++;
    });
}

void MainWindow::refrescarTablaBloques() {
    if (lblEstadoBlockchain) {
        lblEstadoBlockchain->setText(
            QString("Bloques: %1  |  Manipulados: %2  |  Transacciones pendientes: %3 (se genera un bloque automáticamente al llegar a %4)")
                .arg(blockchain.cantidadBloques())
                .arg(blockchain.cantidadManipulados())
                .arg(blockchain.cantidadPendientes())
                .arg(blockchain.getTamanoAutomatico()));
    }
    if (!tablaBloquesAdmin) return;

    tablaBloquesAdmin->setRowCount(0);
    int fila = 0;
    for (Bloque* b = blockchain.getCabeza(); b != nullptr; b = b->anterior) {   // del más reciente al génesis
        tablaBloquesAdmin->insertRow(fila);
        QStringList valores;
        valores << QString::number(b->index)
                << QString::fromStdString(b->timestamp)
                << QString::number(b->nonce)
                << QString::number(b->data.getCantidad())
                << QString::fromStdString(b->hash)
                << QString::fromStdString(b->previousHash)
                << (b->valido ? "ÍNTEGRO" : "MANIPULADO");
        for (int c = 0; c < valores.size(); c++) {
            QTableWidgetItem* item = new QTableWidgetItem(valores[c]);
            item->setBackground(QBrush(b->valido ? QColor(46, 125, 50) : QColor(198, 40, 40)));
            item->setForeground(QBrush(Qt::white));
            if (c == 6 && !b->valido) item->setToolTip(QString::fromStdString(b->motivo));
            tablaBloquesAdmin->setItem(fila, c, item);
        }
        fila++;
    }
}

// Elimina una función: libera la matriz si estaba activa, borra su archivo de asientos y la quita del AVL
void MainWindow::eliminarFuncionInterna(const std::string& codigo) {
    Funcion* fun = funciones.buscar(codigo);
    if (!fun) return;
    std::string archivo = fun->archivo_asientos;      // se copia antes de liberar el nodo

    if (funcionActivaCodigo == codigo) {
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

    QFile::remove(QCoreApplication::applicationDirPath() + "/asientos/" + QString::fromStdString(archivo));
    funciones.eliminar(codigo);
}

// Lista desplegable con las sedes del grafo. Devuelve el código elegido ("" si se cancela)
QString MainWindow::elegirSede(const QString& titulo, const QString& etiqueta, const QString& seleccionada) {
    if (grafo.cantidadSedes() == 0) {
        QMessageBox::warning(this, "Sin sedes", "No hay sedes registradas.");
        return "";
    }
    QStringList items;
    int indice = 0, i = 0;
    grafo.recorrerSedes([&](const Sede& s) {
        items << QString::fromStdString(s.codigo + " - " + s.nombre);
        if (QString::fromStdString(s.codigo) == seleccionada) indice = i;
        i++;
    });
    bool ok = false;
    QString elegido = QInputDialog::getItem(this, titulo, etiqueta, items, indice, false, &ok);
    if (!ok || elegido.isEmpty()) return "";
    return elegido.section(" - ", 0, 0).trimmed();
}

// Código de reserva único (R1001, R1002, ...). Se salta los que ya existen en la Tabla Hash
// (que se reconstruye desde la blockchain), así nunca se repite entre ejecuciones.
std::string MainWindow::siguienteCodigoReserva() {
    std::string codigo;
    do {
        codigo = "R" + std::to_string(1000 + contadorReservas++);
    } while (reservas.buscar(codigo) != nullptr);
    return codigo;
}

// Muestra una imagen (reporte Graphviz) en una ventana con scroll
void MainWindow::mostrarImagen(const QString& titulo, const QString& rutaPng) {
    QPixmap pix(rutaPng);
    if (pix.isNull()) {
        QMessageBox::warning(this, "Reporte no disponible",
                             "No se pudo generar o abrir la imagen:\n" + rutaPng +
                             "\n\nVerifica que Graphviz (dot) esté instalado y en el PATH.");
        return;
    }
    QDialog dlg(this);
    dlg.setWindowTitle(titulo);
    dlg.resize(1100, 700);
    QVBoxLayout* l = new QVBoxLayout(&dlg);
    QLabel* lbl = new QLabel();
    lbl->setPixmap(pix);
    lbl->setAlignment(Qt::AlignCenter);
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidget(lbl);
    scroll->setWidgetResizable(true);
    l->addWidget(scroll);
    QPushButton* cerrar = new QPushButton("Cerrar");
    connect(cerrar, &QPushButton::clicked, &dlg, &QDialog::accept);
    l->addWidget(cerrar);
    dlg.exec();
}

// Muestra una tabla de resultados en una ventana de diálogo
void MainWindow::mostrarTabla(const QString& titulo, const QString& resumen, const QStringList& encabezados,
                              const QList<QStringList>& filas) {
    QDialog dlg(this);
    dlg.setWindowTitle(titulo);
    dlg.resize(950, 500);
    QVBoxLayout* l = new QVBoxLayout(&dlg);
    if (!resumen.isEmpty()) {
        QLabel* lblResumen = new QLabel(resumen);
        lblResumen->setStyleSheet("font-weight: bold; padding: 4px;");
        l->addWidget(lblResumen);
    }
    QTableWidget* tabla = new QTableWidget(filas.size(), encabezados.size());
    tabla->setHorizontalHeaderLabels(encabezados);
    tabla->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
    for (int r = 0; r < filas.size(); r++)
        for (int c = 0; c < filas[r].size(); c++)
            tabla->setItem(r, c, new QTableWidgetItem(filas[r][c]));
    l->addWidget(tabla);
    QPushButton* cerrar = new QPushButton("Cerrar");
    connect(cerrar, &QPushButton::clicked, &dlg, &QDialog::accept);
    l->addWidget(cerrar);
    dlg.exec();
}

// Llena un combo con las películas del BST (en inorden)
void MainWindow::llenarComboPeliculas(QComboBox* combo) {
    combo->clear();
    combo->addItem("-- Seleccione película --", "");

    QTableWidget temp;
    temp.setColumnCount(9);
    cartelera.cargarEnTabla(&temp, "inorden");
    for (int r = 0; r < temp.rowCount(); r++) {
        if (!temp.item(r, 0)) continue;
        QString cod = temp.item(r, 0)->text();
        QString tit = temp.item(r, 1) ? temp.item(r, 1)->text() : "";
        if (!cod.isEmpty())
            combo->addItem(cod + " - " + tit, cod);
    }
}

// Elegir uno de los bloques de la cadena (cualquiera, no solo el último). Devuelve su INDEX o -1
int MainWindow::elegirBloque(const QString& titulo) {
    if (blockchain.cantidadBloques() == 0) {
        QMessageBox::information(this, titulo, "Aún no hay bloques en la blockchain.");
        return -1;
    }
    QStringList items;
    for (Bloque* b = blockchain.getCabeza(); b != nullptr; b = b->anterior)
        items << QString("Bloque %1 (%2)").arg(b->index).arg(b->valido ? "íntegro" : "MANIPULADO");
    bool ok = false;
    QString elegido = QInputDialog::getItem(this, titulo, "Selecciona el bloque:", items, 0, false, &ok);
    if (!ok || elegido.isEmpty()) return -1;
    return elegido.section(' ', 1, 1).toInt();
}

void MainWindow::generarReportesFase3() {
    grafo.generarReporteGrafo();
    grafo.generarReporteListaAdyacencia();
    blockchain.generarReporteBlockchain();
    if (blockchain.getCabeza() != nullptr) {        // árbol de Merkle del último bloque de la cadena
        std::string error;
        blockchain.generarReporteMerkle(blockchain.getCabeza()->index, error);
    }
}

// Al iniciar: avisa si algún bloque fue manipulado (el reporte con colores ya se generó en el constructor)
void MainWindow::verificarBlockchainAlIniciar() {
    refrescarTablaBloques();
    int total = blockchain.cantidadBloques();
    if (total == 0) return;

    int manipulados = blockchain.cantidadManipulados();
    if (manipulados == 0) {
        statusBar()->showMessage(
            QString("Blockchain verificada: %1 bloque(s) íntegro(s). Reconstruidas las reservas desde la cadena.").arg(total), 8000);
        return;
    }

    QString detalle;
    for (Bloque* b = blockchain.getCabeza(); b != nullptr; b = b->anterior)
        if (!b->valido)
            detalle += QString("• Bloque %1: %2\n").arg(b->index).arg(QString::fromStdString(b->motivo));

    Bloque* ultimo = blockchain.getUltimoValido();
    QMessageBox::warning(this, "Blockchain: bloques manipulados",
                         QString("Se detectaron %1 bloque(s) manipulado(s) de %2 (en rojo en el reporte):\n\n%3\n"
                                 "Último bloque íntegro: %4.\n"
                                 "Solo los bloques íntegros se usan para reconstruir las reservas, el historial y las ventas.")
                             .arg(manipulados).arg(total).arg(detalle)
                             .arg(ultimo ? QString::number(ultimo->index) : QString("ninguno")));
}

// ============================================================
// FASE 3 - ADMIN: SEDES (GRAFO NO DIRIGIDO PONDERADO)
// ============================================================
QWidget* MainWindow::pestanaGestionSedes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* btns = new QHBoxLayout();
    QPushButton* btnRegistrar = new QPushButton("Registrar sede");
    QPushButton* btnEditar    = new QPushButton("Editar sede");
    QPushButton* btnEliminar  = new QPushButton("Eliminar sede");
    QPushButton* btnPelis     = new QPushButton("Ver películas por sede");
    QPushButton* btnBFS       = new QPushButton("Camino más corto (BFS)");
    QPushButton* btnRanking   = new QPushButton("Sedes con mayor relación");
    QPushButton* btnRefrescar = new QPushButton("Actualizar");
    btns->addWidget(btnRegistrar);
    btns->addWidget(btnEditar);
    btns->addWidget(btnEliminar);
    btns->addWidget(btnPelis);
    btns->addWidget(btnBFS);
    btns->addWidget(btnRanking);
    btns->addWidget(btnRefrescar);
    btns->addStretch();

    QLabel* nota = new QLabel("Cada sede es un vértice. Las aristas aparecen solas cuando dos sedes comparten películas "
                              "(el peso = películas distintas en común); se crean desde \"Funciones y Asientos\".");
    nota->setWordWrap(true);

    tablaSedesAdmin = new QTableWidget(0, 7);
    tablaSedesAdmin->setHorizontalHeaderLabels(
        {"Código", "Nombre", "Dirección", "Salas", "Teléfono", "Funciones", "Conexiones"});
    tablaSedesAdmin->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tablaSedesAdmin->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addLayout(btns);
    layout->addWidget(nota);
    layout->addWidget(tablaSedesAdmin);

    refrescarTablaSedes();

    connect(btnRefrescar, &QPushButton::clicked, [this]() { refrescarGrafo(); });

    // ----- a. Registrar nueva sede (agrega un vértice sin aristas) -----
    connect(btnRegistrar, &QPushButton::clicked, [this]() {
        bool ok;
        QString codigo = QInputDialog::getText(this, "Registrar sede", "Código (ej. S005):", QLineEdit::Normal, "", &ok).trimmed();
        if (!ok || codigo.isEmpty()) return;
        if (grafo.existe(codigo.toStdString())) {
            QMessageBox::warning(this, "Error", "Ya existe una sede con ese código.");
            return;
        }
        QString nombre = QInputDialog::getText(this, "Registrar sede", "Nombre:", QLineEdit::Normal, "", &ok).trimmed();
        if (!ok || nombre.isEmpty()) return;
        QString direccion = QInputDialog::getText(this, "Registrar sede", "Dirección:", QLineEdit::Normal, "", &ok).trimmed();
        if (!ok) return;
        int salas = QInputDialog::getInt(this, "Registrar sede", "Cantidad de salas:", 1, 1, 100, 1, &ok);
        if (!ok) return;
        QString telefono = QInputDialog::getText(this, "Registrar sede", "Teléfono:", QLineEdit::Normal, "", &ok).trimmed();
        if (!ok) return;

        grafo.agregarSede(Sede(codigo.toStdString(), nombre.toStdString(), direccion.toStdString(),
                               salas, telefono.toStdString()));
        refrescarTablaSedes();
        QMessageBox::information(this, "Éxito",
                                 "Sede registrada como un nuevo vértice (sin aristas).\n"
                                 "Las conexiones se crean al programar funciones con películas compartidas.");
    });

    // ----- a. Editar sede -----
    connect(btnEditar, &QPushButton::clicked, [this]() {
        QString codigo = elegirSede("Editar sede", "Sede a editar:");
        if (codigo.isEmpty()) return;
        Sede* s = grafo.obtenerSede(codigo.toStdString());
        if (!s) return;

        bool ok;
        QString nombre = QInputDialog::getText(this, "Editar sede", "Nombre:", QLineEdit::Normal,
                                               QString::fromStdString(s->nombre), &ok).trimmed();
        if (!ok || nombre.isEmpty()) return;
        QString direccion = QInputDialog::getText(this, "Editar sede", "Dirección:", QLineEdit::Normal,
                                                  QString::fromStdString(s->direccion), &ok).trimmed();
        if (!ok) return;
        int salas = QInputDialog::getInt(this, "Editar sede", "Cantidad de salas:", s->cantidad_salas, 1, 100, 1, &ok);
        if (!ok) return;
        QString telefono = QInputDialog::getText(this, "Editar sede", "Teléfono:", QLineEdit::Normal,
                                                 QString::fromStdString(s->telefono), &ok).trimmed();
        if (!ok) return;

        grafo.editarSede(codigo.toStdString(), nombre.toStdString(), direccion.toStdString(),
                         salas, telefono.toStdString());
        refrescarTablaSedes();
        QMessageBox::information(this, "OK", "Sede actualizada.");
    });

    // ----- Eliminar sede (quita el vértice, sus aristas y sus funciones) -----
    connect(btnEliminar, &QPushButton::clicked, [this]() {
        QString codigo = elegirSede("Eliminar sede", "Sede a eliminar:");
        if (codigo.isEmpty()) return;

        // Funciones programadas en esa sede (una función no puede existir sin su sede)
        QStringList codigosFunciones;
        funciones.recorrerInorden([&](Funcion* f) {
            if (f->codigo_sede == codigo.toStdString())
                codigosFunciones << QString::fromStdString(f->codigo_funcion);
        });

        QString pregunta = "¿Eliminar la sede " + codigo + "?";
        if (!codigosFunciones.isEmpty())
            pregunta += QString("\n\nTambién se eliminarán sus %1 función(es): %2\n(y sus archivos de asientos). "
                                "Las compras ya registradas en la blockchain no se modifican.")
                            .arg(codigosFunciones.size()).arg(codigosFunciones.join(", "));
        if (QMessageBox::question(this, "Eliminar sede", pregunta) != QMessageBox::Yes) return;

        for (const QString& cf : codigosFunciones)
            eliminarFuncionInterna(cf.toStdString());
        grafo.eliminarSede(codigo.toStdString());
        refrescarGrafo();      // recalcula las aristas restantes
        QMessageBox::information(this, "Eliminada", "Sede eliminada del grafo. Aristas recalculadas.");
    });

    // ----- c. Ver películas por sede (consulta directa sobre el vértice) -----
    connect(btnPelis, &QPushButton::clicked, [this]() {
        QString codigo = elegirSede("Películas por sede", "Sede:");
        if (codigo.isEmpty()) return;

        QList<QStringList> filas;
        funciones.recorrerInorden([&](Funcion* f) {
            if (f->codigo_sede != codigo.toStdString()) return;
            Pelicula* p = cartelera.buscar(f->codigo_pelicula);
            filas << (QStringList() << QString::fromStdString(f->codigo_pelicula)
                                    << (p ? QString::fromStdString(p->titulo) : QString("(no está en cartelera)"))
                                    << QString::fromStdString(f->codigo_funcion)
                                    << QString::fromStdString(f->fecha)
                                    << QString::fromStdString(f->horario)
                                    << QString::fromStdString(f->sala));
        });
        if (filas.isEmpty()) {
            QMessageBox::information(this, "Películas por sede", "Esa sede no tiene funciones programadas.");
            return;
        }
        mostrarTabla("Películas en la sede " + codigo, QString("Funciones programadas: %1").arg(filas.size()),
                     QStringList() << "Película" << "Título" << "Función" << "Fecha" << "Horario" << "Sala", filas);
    });

    // ----- d. Camino más corto entre dos sedes (BFS) -----
    connect(btnBFS, &QPushButton::clicked, [this]() {
        if (grafo.cantidadSedes() < 2) {
            QMessageBox::warning(this, "BFS", "Se necesitan al menos 2 sedes.");
            return;
        }
        QString origen = elegirSede("Camino más corto (BFS)", "Sede de ORIGEN:");
        if (origen.isEmpty()) return;
        QString destino = elegirSede("Camino más corto (BFS)", "Sede de DESTINO:");
        if (destino.isEmpty()) return;

        std::string camino;
        int saltos = 0;
        if (grafo.caminoMasCorto(origen.toStdString(), destino.toStdString(), camino, saltos)) {
            QString msg = "Camino más corto (BFS):\n\n" + QString::fromStdString(camino) +
                          QString("\n\nSaltos (aristas): %1").arg(saltos);
            if (saltos > 1) msg += QString("\nSedes intermedias: %1").arg(saltos - 1);
            QMessageBox::information(this, "Camino más corto", msg);
        } else {
            QMessageBox::information(this, "Camino más corto",
                                     "No existe un camino entre " + origen + " y " + destino +
                                     " (no están conectadas por películas compartidas).");
        }
    });

    // ----- e. Ranking de aristas (de mayor a menor peso) -----
    connect(btnRanking, &QPushButton::clicked, [this]() {
        ListaParesSedes ranking;
        grafo.rankingAristas(ranking);
        if (ranking.cantidad == 0) {
            QMessageBox::information(this, "Ranking", "Todavía no hay aristas en el grafo.");
            return;
        }
        QList<QStringList> filas;
        for (int i = 0; i < ranking.cantidad; i++) {
            Sede* a = grafo.obtenerSede(ranking.datos[i].a);
            Sede* b = grafo.obtenerSede(ranking.datos[i].b);
            filas << (QStringList() << QString::number(i + 1)
                                    << QString::fromStdString(ranking.datos[i].a + " - " + (a ? a->nombre : ""))
                                    << QString::fromStdString(ranking.datos[i].b + " - " + (b ? b->nombre : ""))
                                    << QString::number(ranking.datos[i].peso));
        }
        mostrarTabla("Sedes con mayor relación", "Aristas ordenadas de mayor a menor peso",
                     QStringList() << "#" << "Sede A" << "Sede B" << "Películas en común", filas);
    });

    return tab;
}

// ============================================================
// FASE 3 - ADMIN: BLOCKCHAIN
// ============================================================
QWidget* MainWindow::pestanaBlockchainAdmin() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* btns = new QHBoxLayout();
    QPushButton* btnGenerar  = new QPushButton("Generar nuevo bloque");
    QPushButton* btnReporte  = new QPushButton("Ver reporte de la blockchain");
    QPushButton* btnMerkle   = new QPushButton("Generar árbol de Merkle de un bloque");
    QPushButton* btnVentas   = new QPushButton("Ver ventas totales");
    QPushButton* btnRefrescar = new QPushButton("Actualizar");
    btns->addWidget(btnGenerar);
    btns->addWidget(btnReporte);
    btns->addWidget(btnMerkle);
    btns->addWidget(btnVentas);
    btns->addWidget(btnRefrescar);
    btns->addStretch();

    lblEstadoBlockchain = new QLabel();
    lblEstadoBlockchain->setStyleSheet("font-weight: bold; padding: 4px;");
    lblEstadoBlockchain->setWordWrap(true);

    tablaBloquesAdmin = new QTableWidget(0, 7);
    tablaBloquesAdmin->setHorizontalHeaderLabels(
        {"INDEX", "TIMESTAMP", "NONCE", "TRANSACCIONES", "HASH", "PREVIOUSHASH", "ESTADO"});
    tablaBloquesAdmin->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tablaBloquesAdmin->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addLayout(btns);
    layout->addWidget(lblEstadoBlockchain);
    layout->addWidget(tablaBloquesAdmin);

    refrescarTablaBloques();

    connect(btnRefrescar, &QPushButton::clicked, [this]() { refrescarTablaBloques(); });

    // ----- a. Generar nuevo bloque (manual) -----
    connect(btnGenerar, &QPushButton::clicked, [this]() {
        std::string error;
        Bloque* b = blockchain.generarBloque(error);
        if (b == nullptr) {
            QMessageBox::information(this, "Generar bloque", QString::fromStdString(error));
            return;
        }
        refrescarTablaBloques();
        QMessageBox::information(this, "Bloque generado",
                                 QString("Bloque %1 generado con %2 transacción(es).\nNONCE: %3\nHASH: %4\nROOTMERKLE: %5%6")
                                     .arg(b->index).arg(b->data.getCantidad()).arg(b->nonce)
                                     .arg(QString::fromStdString(b->hash))
                                     .arg(QString::fromStdString(b->rootMerkle))
                                     .arg(error.empty() ? QString() : "\n\nAviso: " + QString::fromStdString(error)));
    });

    // ----- b. Reporte de la blockchain completa -----
    connect(btnReporte, &QPushButton::clicked, [this]() {
        QString png = blockchain.generarReporteBlockchain();
        mostrarImagen("Blockchain (verde = íntegro, rojo = manipulado)", png);
    });

    // ----- c. Árbol de Merkle de un bloque específico -----
    connect(btnMerkle, &QPushButton::clicked, [this]() {
        int idx = elegirBloque("Árbol de Merkle");
        if (idx < 0) return;
        std::string error;
        QString png = blockchain.generarReporteMerkle(idx, error);
        if (png.isEmpty()) {
            QMessageBox::warning(this, "Merkle", QString::fromStdString(error));
            return;
        }
        mostrarImagen("Árbol de Merkle - Bloque " + QString::number(idx), png);
    });

    // ----- d. Ventas totales (a partir del DATA de los bloques íntegros) -----
    connect(btnVentas, &QPushButton::clicked, [this]() {
        ResumenVentas ventas;
        blockchain.calcularVentas(ventas);
        if (ventas.totalBoletos == 0) {
            QMessageBox::information(this, "Ventas totales",
                                     QString("Aún no hay ventas registradas en bloques.\n(Transacciones pendientes de bloque: %1)")
                                         .arg(blockchain.cantidadPendientes()));
            return;
        }
        QList<QStringList> filas;
        for (NodoVenta* n = ventas.porFuncion; n != nullptr; n = n->siguiente)
            filas << (QStringList() << "Función" << QString::fromStdString(n->clave) << QString::number(n->boletos)
                                    << QString("Q%1").arg(n->monto, 0, 'f', 2));
        for (NodoVenta* n = ventas.porFecha; n != nullptr; n = n->siguiente)
            filas << (QStringList() << "Fecha" << QString::fromStdString(n->clave) << QString::number(n->boletos)
                                    << QString("Q%1").arg(n->monto, 0, 'f', 2));
        mostrarTabla("Ventas totales (blockchain)",
                     QString("Boletos vendidos: %1   |   Monto total recaudado: Q%2   |   Pendientes sin bloque: %3")
                         .arg(ventas.totalBoletos).arg(ventas.totalMonto, 0, 'f', 2).arg(blockchain.cantidadPendientes()),
                     QStringList() << "Desglose por" << "Clave" << "Boletos" << "Monto", filas);
    });

    return tab;
}

// ============================================================
// FASE 3 - CLIENTE: SEDES (GRAFO)
// ============================================================
QWidget* MainWindow::pestanaClienteSedes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* fila1 = new QHBoxLayout();
    QComboBox* cmbPelicula = new QComboBox();
    cmbPelicula->setMinimumWidth(280);
    QPushButton* btnActualizar = new QPushButton("Actualizar lista");
    QPushButton* btnSedes = new QPushButton("Ver sedes donde está disponible");
    fila1->addWidget(new QLabel("Película:"));
    fila1->addWidget(cmbPelicula);
    fila1->addWidget(btnActualizar);
    fila1->addWidget(btnSedes);
    fila1->addStretch();

    QHBoxLayout* fila2 = new QHBoxLayout();
    QComboBox* cmbSede = new QComboBox();
    cmbSede->setMinimumWidth(280);
    QPushButton* btnFunciones = new QPushButton("Ver funciones de la película en esta sede");
    QPushButton* btnSimilares = new QPushButton("Ver sedes similares");
    fila2->addWidget(new QLabel("Sede:"));
    fila2->addWidget(cmbSede);
    fila2->addWidget(btnFunciones);
    fila2->addWidget(btnSimilares);
    fila2->addStretch();

    QLabel* lblTitulo = new QLabel("1) Elige una película y pulsa \"Ver sedes donde está disponible\".");
    lblTitulo->setStyleSheet("font-weight: bold; padding: 4px;");

    QTableWidget* tabla = new QTableWidget(0, 0);
    tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addLayout(fila1);
    layout->addLayout(fila2);
    layout->addWidget(lblTitulo);
    layout->addWidget(tabla);

    llenarComboPeliculas(cmbPelicula);

    // Rellena la tabla de resultados con los encabezados y filas dados
    auto llenar = [tabla](const QStringList& encabezados, const QList<QStringList>& filas) {
        tabla->clear();
        tabla->setColumnCount(encabezados.size());
        tabla->setHorizontalHeaderLabels(encabezados);
        tabla->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tabla->setRowCount(filas.size());
        for (int r = 0; r < filas.size(); r++)
            for (int c = 0; c < filas[r].size(); c++)
                tabla->setItem(r, c, new QTableWidgetItem(filas[r][c]));
    };

    connect(btnActualizar, &QPushButton::clicked, [this, cmbPelicula]() { llenarComboPeliculas(cmbPelicula); });

    // ----- a. Sedes donde está disponible una película (recorre los vértices del grafo) -----
    connect(btnSedes, &QPushButton::clicked, [this, cmbPelicula, cmbSede, lblTitulo, llenar]() {
        QString codPel = cmbPelicula->currentData().toString();
        if (codPel.isEmpty()) {
            QMessageBox::warning(this, "Atención", "Selecciona una película.");
            return;
        }
        cmbSede->clear();
        QList<QStringList> filas;
        grafo.recorrerSedes([&](const Sede& s) {
            bool agregada = false;
            funciones.recorrerInorden([&](Funcion* f) {
                if (f->codigo_sede != s.codigo || QString::fromStdString(f->codigo_pelicula) != codPel) return;
                filas << (QStringList() << QString::fromStdString(s.codigo) << QString::fromStdString(s.nombre)
                                        << QString::fromStdString(s.direccion)
                                        << QString::fromStdString(f->codigo_funcion)
                                        << QString::fromStdString(f->fecha) << QString::fromStdString(f->horario)
                                        << QString::fromStdString(f->sala));
                agregada = true;
            });
            if (agregada)
                cmbSede->addItem(QString::fromStdString(s.codigo + " - " + s.nombre), QString::fromStdString(s.codigo));
        });
        if (filas.isEmpty()) {
            llenar(QStringList(), QList<QStringList>());
            lblTitulo->setText("Esa película no tiene funciones programadas en ninguna sede.");
            return;
        }
        lblTitulo->setText(QString("Sedes donde está disponible %1 (%2 sede(s)). Elige una sede para ver sus funciones o sedes similares.")
                               .arg(cmbPelicula->currentText()).arg(cmbSede->count()));
        llenar(QStringList() << "Sede" << "Nombre" << "Dirección" << "Función" << "Fecha" << "Horario" << "Sala", filas);
    });

    // ----- b. Funciones de esa película en la sede elegida -----
    connect(btnFunciones, &QPushButton::clicked, [this, cmbPelicula, cmbSede, lblTitulo, llenar]() {
        QString codPel = cmbPelicula->currentData().toString();
        QString codSede = cmbSede->currentData().toString();
        if (codPel.isEmpty() || codSede.isEmpty()) {
            QMessageBox::warning(this, "Atención", "Primero busca las sedes de una película y elige una sede.");
            return;
        }
        QList<QStringList> filas;
        funciones.recorrerInorden([&](Funcion* f) {
            if (f->codigo_sede == codSede.toStdString() && QString::fromStdString(f->codigo_pelicula) == codPel)
                filas << (QStringList() << QString::fromStdString(f->codigo_funcion) << QString::fromStdString(f->fecha)
                                        << QString::fromStdString(f->horario) << QString::fromStdString(f->sala)
                                        << QString("%1 x %2").arg(f->filas).arg(f->columnas));
        });
        lblTitulo->setText(QString("Funciones de %1 en la sede %2 (para reservar, ve a la pestaña \"Reservar / Cancelar\")")
                               .arg(cmbPelicula->currentText()).arg(cmbSede->currentText()));
        llenar(QStringList() << "Función" << "Fecha" << "Horario" << "Sala" << "Asientos (filas x columnas)", filas);
    });

    // ----- c. Sedes similares: vecinos de la sede ordenados por peso descendente -----
    connect(btnSimilares, &QPushButton::clicked, [this, cmbSede, lblTitulo, llenar]() {
        QString codSede = cmbSede->currentData().toString();
        if (codSede.isEmpty()) {
            QMessageBox::warning(this, "Atención", "Primero busca las sedes de una película y elige una sede.");
            return;
        }
        ListaParesSedes similares;
        grafo.sedesSimilares(codSede.toStdString(), similares);
        if (similares.cantidad == 0) {
            llenar(QStringList(), QList<QStringList>());
            lblTitulo->setText("La sede " + codSede + " no comparte películas con otras sedes.");
            return;
        }
        QList<QStringList> filas;
        for (int i = 0; i < similares.cantidad; i++) {
            Sede* s = grafo.obtenerSede(similares.datos[i].b);
            filas << (QStringList() << QString::fromStdString(similares.datos[i].b)
                                    << (s ? QString::fromStdString(s->nombre) : QString())
                                    << (s ? QString::fromStdString(s->direccion) : QString())
                                    << QString::number(similares.datos[i].peso));
        }
        lblTitulo->setText("Sedes con cartelera similar a " + codSede + " (de más a menos películas en común)");
        llenar(QStringList() << "Sede" << "Nombre" << "Dirección" << "Películas en común", filas);
    });

    return tab;
}

// ============================================================
// FASE 3 - CLIENTE: HISTORIAL Y COMPROBANTES DESDE LA BLOCKCHAIN
// ============================================================
QWidget* MainWindow::pestanaClienteCompras() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* btns = new QHBoxLayout();
    QPushButton* btnHistorial = new QPushButton("Ver historial de compras");
    QPushButton* btnComprobante = new QPushButton("Ver comprobante de una transacción");
    btns->addWidget(btnHistorial);
    btns->addWidget(btnComprobante);
    btns->addStretch();

    QLabel* nota = new QLabel("El historial se obtiene recorriendo el DATA de los bloques íntegros de la blockchain, "
                              "por eso se conserva aunque cierres y vuelvas a abrir el programa.");
    nota->setWordWrap(true);

    QTableWidget* tabla = new QTableWidget(0, 9);
    tabla->setHorizontalHeaderLabels({"Transacción", "Reserva", "Función", "Asiento", "Monto", "Método", "Fecha", "Bloque", "Estado"});
    tabla->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tabla->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabla->setSelectionBehavior(QAbstractItemView::SelectRows);
    tabla->setSelectionMode(QAbstractItemView::SingleSelection);

    layout->addLayout(btns);
    layout->addWidget(nota);
    layout->addWidget(tabla);

    connect(btnHistorial, &QPushButton::clicked, [this, tabla]() {
        tabla->setRowCount(0);
        if (!clienteActual) {
            QMessageBox::warning(this, "Error", "No hay cliente autenticado.");
            return;
        }
        int fila = 0;
        // Agrega una fila si la transacción pertenece al cliente (bloque == nullptr -> aún pendiente)
        auto agregar = [&](const Transaccion& t, const Bloque* b) {
            if (t.id_cliente != clienteActual->id) return;
            tabla->insertRow(fila);
            tabla->setItem(fila, 0, new QTableWidgetItem(QString::fromStdString(t.codigo_transaccion)));
            tabla->setItem(fila, 1, new QTableWidgetItem(QString::fromStdString(t.codigo_reserva)));
            tabla->setItem(fila, 2, new QTableWidgetItem(QString::fromStdString(t.codigo_funcion)));
            tabla->setItem(fila, 3, new QTableWidgetItem(QString("F%1-C%2").arg(t.fila).arg(t.columna)));
            tabla->setItem(fila, 4, new QTableWidgetItem(QString("Q%1").arg(t.monto, 0, 'f', 2)));
            tabla->setItem(fila, 5, new QTableWidgetItem(QString::fromStdString(t.metodo_pago)));
            tabla->setItem(fila, 6, new QTableWidgetItem(QString::fromStdString(t.fecha_pago)));
            tabla->setItem(fila, 7, new QTableWidgetItem(b ? QString::number(b->index) : QString("-")));
            bool cancelada = (t.estado == "CANCELADA");
            QTableWidgetItem* estado = new QTableWidgetItem(
                QString(b ? "EN BLOQUE" : "PENDIENTE") + " - " + QString::fromStdString(t.estado));
            estado->setBackground(QBrush(cancelada ? QColor(198, 40, 40)
                                                   : (b ? QColor(46, 125, 50) : QColor(239, 108, 0))));
            estado->setForeground(QBrush(Qt::white));
            tabla->setItem(fila, 8, estado);
            fila++;
        };
        blockchain.recorrerTransaccionesValidas(agregar);
        blockchain.getPendientes().recorrer([&](const Transaccion& t) { agregar(t, nullptr); });

        if (fila == 0)
            QMessageBox::information(this, "Historial", "No tienes compras registradas.");
    });

    connect(btnComprobante, &QPushButton::clicked, [this, tabla]() {
        if (!clienteActual) {
            QMessageBox::warning(this, "Error", "No hay cliente autenticado.");
            return;
        }
        QString sugerido;
        int filaSel = tabla->currentRow();
        if (filaSel >= 0 && tabla->item(filaSel, 0)) sugerido = tabla->item(filaSel, 0)->text();

        bool ok;
        QString codigo = QInputDialog::getText(this, "Comprobante", "Código de transacción (ej. T001):",
                                               QLineEdit::Normal, sugerido, &ok).trimmed();
        if (!ok || codigo.isEmpty()) return;

        Transaccion t;
        const Bloque* bloque = nullptr;
        if (!blockchain.buscarTransaccion(codigo.toStdString(), t, bloque)) {
            QMessageBox::warning(this, "Comprobante", "No existe una transacción con ese código.");
            return;
        }
        if (t.id_cliente != clienteActual->id) {
            QMessageBox::warning(this, "Comprobante", "Esa transacción no pertenece a tu cuenta.");
            return;
        }

        QString detalleFuncion = "(función eliminada)";
        Funcion* f = funciones.buscar(t.codigo_funcion);
        if (f) {
            Pelicula* p = cartelera.buscar(f->codigo_pelicula);
            Sede* s = grafo.obtenerSede(f->codigo_sede);
            detalleFuncion = QString("%1 | Sede: %2 | %3 %4 | %5")
                                 .arg(p ? QString::fromStdString(p->titulo) : QString::fromStdString(f->codigo_pelicula))
                                 .arg(s ? QString::fromStdString(s->nombre) : QString::fromStdString(f->codigo_sede))
                                 .arg(QString::fromStdString(f->fecha)).arg(QString::fromStdString(f->horario))
                                 .arg(QString::fromStdString(f->sala));
        }

        QString texto = QString("COMPROBANTE DE COMPRA\n\n"
                                "Transacción: %1\nReserva: %2\nCliente: %3\nFunción: %4 (%5)\nAsiento: Fila %6, Columna %7\n"
                                "Monto: Q%8\nMétodo de pago: %9\nFecha de pago: %10\nEstado: " + QString::fromStdString(t.estado) + "\n\n")
                            .arg(QString::fromStdString(t.codigo_transaccion))
                            .arg(QString::fromStdString(t.codigo_reserva))
                            .arg(QString::fromStdString(t.id_cliente))
                            .arg(QString::fromStdString(t.codigo_funcion)).arg(detalleFuncion)
                            .arg(t.fila).arg(t.columna)
                            .arg(t.monto, 0, 'f', 2)
                            .arg(QString::fromStdString(t.metodo_pago))
                            .arg(QString::fromStdString(t.fecha_pago));
        if (bloque) {
            texto += QString("Registrada en el BLOQUE (INDEX): %1\nTimestamp del bloque: %2\nHASH del bloque:\n%3")
                         .arg(bloque->index).arg(QString::fromStdString(bloque->timestamp))
                         .arg(QString::fromStdString(bloque->hash));
        } else {
            texto += "Estado: PENDIENTE de ser incluida en un bloque (el administrador la registrará pronto).";
        }
        QMessageBox::information(this, "Comprobante " + codigo, texto);
    });

    return tab;
}
