#include "mainwindow.h"
#include "CargadorCSV.h"
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), matrizActual(nullptr), mapaVisualAdmin(nullptr), lblDetalleSolicitudAdmin(nullptr) {
    this->setWindowTitle("CinemaUSAC - Sistema de Gestión");
    this->resize(1100, 750);

    panelPrincipal = new QStackedWidget(this);
    panelPrincipal->addWidget(crearPanelLogin());   // 0
    panelPrincipal->addWidget(crearPanelAdmin());   // 1
    panelPrincipal->addWidget(crearPanelUsuario()); // 2

    setCentralWidget(panelPrincipal);
}

MainWindow::~MainWindow() {
    if (matrizActual) delete matrizActual;
}

// ----------------------------------------------------
// LOGIN UNIFICADO
// ----------------------------------------------------
QWidget* MainWindow::crearPanelLogin() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);
    layout->setAlignment(Qt::AlignCenter);

    QLabel* titulo = new QLabel("PANEL GENERAL - LOGIN");
    QFont font = titulo->font(); font.setPointSize(16); font.setBold(true);
    titulo->setFont(font);

    QLineEdit* txtCorreo = new QLineEdit(); txtCorreo->setPlaceholderText("Correo electrónico"); txtCorreo->setFixedWidth(300);
    QLineEdit* txtPassword = new QLineEdit(); txtPassword->setPlaceholderText("Contraseña"); txtPassword->setEchoMode(QLineEdit::Password); txtPassword->setFixedWidth(300);
    QPushButton* btnIngresar = new QPushButton("INICIAR SESION"); btnIngresar->setFixedWidth(300);
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
            refrescarSolicitudesAdmin();
            refrescarMapaAsientosAdmin();
            panelPrincipal->setCurrentIndex(1);
        } else if (correo == "cliente@cinema.com" && pass == "1234") {
            txtCorreo->clear(); txtPassword->clear();
            panelPrincipal->setCurrentIndex(2);
        } else {
            QMessageBox::warning(this, "Error", "Credenciales incorrectas.\nAdmin: admin@cinema.com / admin123\nCliente: cliente@cinema.com / 1234");
        }
    });

    return vista;
}

// ----------------------------------------------------
// PANEL ADMINISTRATIVO
// ----------------------------------------------------
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
        QMessageBox::information(this, "Reportes", "Reportes Graphviz generados exitosamente en /reportes.");
    });

    return vista;
}

QWidget* MainWindow::pestanaGestionPeliculas() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnAgregar = new QPushButton("Agregar Pelicula");
    QPushButton* btnCargaCSV = new QPushButton("Carga Masiva (CSV)");

    btnLayout->addWidget(btnAgregar);
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
            cartelera.cargarEnTabla(tablaCarteleraAdmin);
            QMessageBox::information(this, "Éxito", "Películas cargadas y reflejadas en la tabla.");
        }
    });

    connect(btnAgregar, &QPushButton::clicked, [this]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Agregar", "Código (ej. P005):", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;
        QString tit = QInputDialog::getText(this, "Agregar", "Título:", QLineEdit::Normal, "", &ok);
        QString gen = QInputDialog::getText(this, "Agregar", "Género:", QLineEdit::Normal, "", &ok);
        int dur = QInputDialog::getInt(this, "Agregar", "Duración (min):", 120, 1, 500, 1, &ok);

        cartelera.insertar(cod.toStdString(), tit.toStdString(), gen.toStdString(), dur, "B15", "Español", "2026-08-01", "2026-09-01");
        cartelera.cargarEnTabla(tablaCarteleraAdmin);
    });

    return tab;
}

QWidget* MainWindow::pestanaGestionPromociones() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* topBtns = new QHBoxLayout();
    QPushButton* btnAddPromo = new QPushButton("Agregar Una Promoción");
    QPushButton* btnAddBeneficio = new QPushButton("Añadir Beneficio");
    topBtns->addWidget(btnAddPromo);
    topBtns->addWidget(btnAddBeneficio);
    topBtns->addStretch();

    QTableWidget* tablaPromos = new QTableWidget(0, 3);
    tablaPromos->setHorizontalHeaderLabels({"CÓDIGO", "NOMBRE", "VIGENCIA"});
    tablaPromos->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addLayout(topBtns);
    layout->addWidget(tablaPromos);

    connect(btnAddPromo, &QPushButton::clicked, [this, tablaPromos]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Promoción", "Código (ej. Promo_01):", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;
        QString nom = QInputDialog::getText(this, "Promoción", "Nombre de la Promoción:", QLineEdit::Normal, "", &ok);

        promociones.insertar(cod.toStdString(), nom.toStdString(), "Vigente", "Todos los dias");

        int row = tablaPromos->rowCount();
        tablaPromos->insertRow(row);
        tablaPromos->setItem(row, 0, new QTableWidgetItem(cod));
        tablaPromos->setItem(row, 1, new QTableWidgetItem(nom));
        tablaPromos->setItem(row, 2, new QTableWidgetItem("Vigente"));
    });

    connect(btnAddBeneficio, &QPushButton::clicked, [this]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Beneficio", "Código de Promoción a la que pertenece:", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;
        Promocion* p = promociones.buscar(cod.toStdString());
        if (!p) {
            QMessageBox::warning(this, "Error", "La promoción especificada no existe.");
            return;
        }
        QString desc = QInputDialog::getText(this, "Beneficio", "Descripción del beneficio:", QLineEdit::Normal, "", &ok);
        p->agregarBeneficio("Descuento", desc.toStdString(), "10%");
        QMessageBox::information(this, "Éxito", "Beneficio agregado a la promoción.");
    });

    return tab;
}

QWidget* MainWindow::pestanaGestionSolicitudes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* topHeader = new QHBoxLayout();
    QLabel* lblInfo = new QLabel("Solicitudes Especiales Pendientes");
    lblInfo->setFont(QFont("Arial", 11, QFont::Bold));

    QPushButton* btnRefrescar = new QPushButton("Actualizar Solicitudes");
    topHeader->addWidget(lblInfo);
    topHeader->addStretch();
    topHeader->addWidget(btnRefrescar);

    lblDetalleSolicitudAdmin = new QLabel("No hay solicitudes pendientes.");
    lblDetalleSolicitudAdmin->setStyleSheet("border: 1px solid gray; padding: 15px; background-color: #222; font-size: 13px;");

    QHBoxLayout* acciones = new QHBoxLayout();
    QPushButton* btnAceptar = new QPushButton("Aceptar / Atender Solicitud");
    QPushButton* btnRechazar = new QPushButton("Rechazar Solicitud");
    acciones->addWidget(btnAceptar); acciones->addWidget(btnRechazar);

    layout->addLayout(topHeader);
    layout->addWidget(lblDetalleSolicitudAdmin);
    layout->addLayout(acciones);
    layout->addStretch();

    connect(btnRefrescar, &QPushButton::clicked, [this]() {
        refrescarSolicitudesAdmin();
    });

    connect(btnAceptar, &QPushButton::clicked, [this]() {
        Solicitud* s = solicitudes.getPrimera();
        if (!s) { QMessageBox::information(this, "Info", "No hay solicitudes pendientes."); return; }
        s->estado = "Atendida";
        QMessageBox::information(this, "Atendida", QString("Solicitud #%1 marcada como Atendida.").arg(s->numero));
        solicitudes.rechazarPrimera();
        refrescarSolicitudesAdmin();
    });

    connect(btnRechazar, &QPushButton::clicked, [this]() {
        if (solicitudes.rechazarPrimera()) {
            QMessageBox::information(this, "Rechazada", "Solicitud eliminada de la lista.");
        } else {
            QMessageBox::information(this, "Info", "No hay solicitudes pendientes.");
        }
        refrescarSolicitudesAdmin();
    });

    return tab;
}

QWidget* MainWindow::pestanaGestionFunciones() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGridLayout* gridConfig = new QGridLayout();
    QLineEdit* txtFilas = new QLineEdit("5"); txtFilas->setPlaceholderText("Filas (ej. 5)");
    QLineEdit* txtCols = new QLineEdit("8"); txtCols->setPlaceholderText("Columnas (ej. 8)");
    QPushButton* btnCrear = new QPushButton("Crear Función");
    QPushButton* btnRefrescarMapa = new QPushButton("Actualizar Mapa");

    gridConfig->addWidget(new QLabel("Filas:"), 0, 0); gridConfig->addWidget(txtFilas, 0, 1);
    gridConfig->addWidget(new QLabel("Columnas:"), 0, 2); gridConfig->addWidget(txtCols, 0, 3);
    gridConfig->addWidget(btnCrear, 0, 4);
    gridConfig->addWidget(btnRefrescarMapa, 0, 5);

    mapaVisualAdmin = new QTableWidget();
    mapaVisualAdmin->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addLayout(gridConfig);
    layout->addWidget(new QLabel("Mapa de Asientos Ocupados (Matriz Dispersa):"));
    layout->addWidget(mapaVisualAdmin);

    connect(btnCrear, &QPushButton::clicked, [this, txtFilas, txtCols]() {
        int f = txtFilas->text().toInt();
        int c = txtCols->text().toInt();
        if (f <= 0 || c <= 0) { f = 5; c = 8; }

        if (matrizActual) delete matrizActual;
        matrizActual = new MatrizDispersa(f, c);

        refrescarMapaAsientosAdmin();
        QMessageBox::information(this, "Éxito", QString("Función creada con sala de %1 filas x %2 columnas.").arg(f).arg(c));
    });

    connect(btnRefrescarMapa, &QPushButton::clicked, [this]() {
        refrescarMapaAsientosAdmin();
    });

    return tab;
}

// Métodos auxiliares de refresco
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
                .arg(QString::fromStdString(s->estado))
            );
    } else {
        lblDetalleSolicitudAdmin->setText("No hay solicitudes pendientes en la lista.");
    }
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
                item->setBackground(QBrush(QColor(198, 40, 40))); // ROJO
                item->setForeground(QBrush(Qt::white));
            } else {
                item->setText(QString("F%1-C%2\nLIBRE").arg(r).arg(col));
                item->setBackground(QBrush(QColor(46, 125, 50))); // VERDE
                item->setForeground(QBrush(Qt::white));
            }

            item->setTextAlignment(Qt::AlignCenter);
            mapaVisualAdmin->setItem(r - 1, col - 1, item);
        }
    }
}

// ----------------------------------------------------
// PANEL CLIENTE
// ----------------------------------------------------
QWidget* MainWindow::crearPanelUsuario() {
    QWidget* vista = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(vista);

    QHBoxLayout* header = new QHBoxLayout();
    QLabel* titulo = new QLabel("PANEL DE CLIENTE");
    QFont font = titulo->font(); font.setBold(true); font.setPointSize(14);
    titulo->setFont(font);

    QPushButton* btnLogout = new QPushButton("Cerrar Sesión");
    header->addWidget(titulo); header->addStretch(); header->addWidget(btnLogout);

    QTabWidget* tabsUsuario = new QTabWidget();
    tabsUsuario->addTab(pestanaClienteCartelera(), "Ver Cartelera");
    tabsUsuario->addTab(pestanaClienteReservas(), "Reservar Asiento");
    tabsUsuario->addTab(pestanaClienteSolicitudes(), "Solicitudes Especiales");

    layout->addLayout(header);
    layout->addWidget(tabsUsuario);

    connect(btnLogout, &QPushButton::clicked, [this]() { panelPrincipal->setCurrentIndex(0); });

    return vista;
}

QWidget* MainWindow::pestanaClienteCartelera() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    QTableWidget* tablaUser = new QTableWidget(0, 8);
    tablaUser->setHorizontalHeaderLabels({"CÓDIGO", "TÍTULO", "GÉNERO", "DURACIÓN", "CLASIFICACIÓN", "IDIOMA", "ESTRENO", "FIN"});
    tablaUser->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QPushButton* btnVer = new QPushButton("Actualizar Cartelera");
    layout->addWidget(btnVer);
    layout->addWidget(tablaUser);

    connect(btnVer, &QPushButton::clicked, [this, tablaUser]() {
        cartelera.cargarEnTabla(tablaUser);
    });

    return tab;
}

QWidget* MainWindow::pestanaClienteReservas() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* inputs = new QHBoxLayout();
    QLineEdit* txtFila = new QLineEdit(); txtFila->setPlaceholderText("Fila (ej. 1)");
    QLineEdit* txtCol = new QLineEdit(); txtCol->setPlaceholderText("Columna (ej. 1)");
    QLineEdit* txtCliente = new QLineEdit(); txtCliente->setPlaceholderText("Nombre del Cliente");
    QPushButton* btnReservar = new QPushButton("Reservar Asiento");

    inputs->addWidget(txtFila); inputs->addWidget(txtCol); inputs->addWidget(txtCliente); inputs->addWidget(btnReservar);

    layout->addLayout(inputs);
    layout->addStretch();

    connect(btnReservar, &QPushButton::clicked, [this, txtFila, txtCol, txtCliente]() {
        if (!matrizActual) {
            QMessageBox::warning(this, "Error", "No hay función activa creada.");
            return;
        }
        int f = txtFila->text().toInt();
        int c = txtCol->text().toInt();
        QString cli = txtCliente->text().trimmed();
        if (cli.isEmpty()) cli = "Cliente";

        bool ok = matrizActual->reservarAsiento(f, c, cli.toStdString());
        if (ok) {
            refrescarMapaAsientosAdmin(); // Actualiza el tablero visual del Admin
            QMessageBox::information(this, "Reserva Éxito", QString("Asiento F%1-C%2 reservado.").arg(f).arg(c));
        } else {
            QMessageBox::warning(this, "Error", "El asiento ya está ocupado o está fuera de rango.");
        }
    });

    return tab;
}

QWidget* MainWindow::pestanaClienteSolicitudes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLineEdit* txtNombre = new QLineEdit(); txtNombre->setPlaceholderText("Nombre del Cliente");
    QLineEdit* txtTel = new QLineEdit(); txtTel->setPlaceholderText("Teléfono");
    QLineEdit* txtTipo = new QLineEdit(); txtTipo->setPlaceholderText("Tipo (Cumpleaños, Evento, etc.)");
    QLineEdit* txtDesc = new QLineEdit(); txtDesc->setPlaceholderText("Descripción");
    QPushButton* btnEnviar = new QPushButton("Enviar Solicitud");

    layout->addWidget(txtNombre); layout->addWidget(txtTel);
    layout->addWidget(txtTipo); layout->addWidget(txtDesc);
    layout->addWidget(btnEnviar); layout->addStretch();

    connect(btnEnviar, &QPushButton::clicked, [this, txtNombre, txtTel, txtTipo, txtDesc]() {
        solicitudes.insertar(txtNombre->text().toStdString(), txtTel->text().toStdString(),
                             txtTipo->text().toStdString(), txtDesc->text().toStdString(), "2026-08-19");
        refrescarSolicitudesAdmin(); // Refresca la vista en el panel admin
        QMessageBox::information(this, "Éxito", "Solicitud registrada para revisión del administrador.");
    });

    return tab;
}