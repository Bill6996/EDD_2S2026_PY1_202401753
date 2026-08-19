#include "mainwindow.h"
#include "CargadorCSV.h"
#include <QInputDialog>
#include <QHeaderView>
#include <QScrollArea>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    matrizActual(nullptr),
    mapaVisualAdmin(nullptr),
    lblDetalleSolicitudAdmin(nullptr),
    lblTotalSolicitudesAdmin(nullptr) {

    this->setWindowTitle("CinemaUSAC - Sistema de Gestión");
    this->resize(1100, 750);

    panelPrincipal = new QStackedWidget(this);
    panelPrincipal->addWidget(crearPanelLogin());
    panelPrincipal->addWidget(crearPanelAdmin());
    panelPrincipal->addWidget(crearPanelUsuario());

    setCentralWidget(panelPrincipal);
}

MainWindow::~MainWindow() {
    if (matrizActual) delete matrizActual;
}

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
    tabsAdmin->addTab(pestanaReportesVisuales(), "Reportes Visuales (Graphviz)"); // NAVEGADOR DE REPORTES

    layout->addLayout(header);
    layout->addWidget(tabsAdmin);

    connect(btnLogout, &QPushButton::clicked, [this]() { panelPrincipal->setCurrentIndex(0); });
    connect(btnReportes, &QPushButton::clicked, [this]() {
        cartelera.generarReporteGraphviz();
        solicitudes.generarReporteGraphviz();
        promociones.generarReporteGraphviz();
        if (matrizActual) matrizActual->generarReporteGraphviz();
        QMessageBox::information(this, "Reportes", "Reportes Graphviz generados exitosamente en /reportes. Puedes verlos en la pestaña 'Reportes Visuales'.");
    });

    return vista;
}

QWidget* MainWindow::pestanaGestionPeliculas() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnAgregar = new QPushButton("Agregar Pelicula");
    QPushButton* btnEliminar = new QPushButton("Eliminar Película");
    QPushButton* btnCargaCSV = new QPushButton("Carga Masiva (CSV)");

    btnLayout->addWidget(btnAgregar);
    btnLayout->addWidget(btnEliminar);
    btnLayout->addWidget(btnCargaCSV);
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
        QString fFin = QInputDialog::getText(this, "Agregar", "Fecha Fin (YYYY-MM-DD):", QLineEdit::Normal, "2026-08-25", &ok);

        cartelera.insertar(cod.toStdString(), tit.toStdString(), gen.toStdString(), dur, "B15", "Español", "2026-08-01", fFin.toStdString());
        cartelera.cargarEnTabla(tablaCarteleraAdmin);
    });

    connect(btnEliminar, &QPushButton::clicked, [this]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Eliminar", "Código de la película a eliminar:", QLineEdit::Normal, "", &ok);
        if (ok && !cod.isEmpty()) {
            cartelera.eliminar(cod.toStdString());
            cartelera.cargarEnTabla(tablaCarteleraAdmin);
            QMessageBox::information(this, "Éxito", "Película eliminada del Árbol BST.");
        }
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
    QStringList headers = {"CÓDIGO / ESTRUCTURA", "NOMBRE / TIPO BENEFICIO", "VIGENCIA / VALOR", "DÍAS APLICABLES / DESCRIPCIÓN"};
    treePromos->setHeaderLabels(headers);
    treePromos->header()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addLayout(topBtns);
    layout->addWidget(treePromos);

    connect(btnAddPromo, &QPushButton::clicked, [this, treePromos]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Nueva Promoción", "Código de Promoción (ej. Promo_01):", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;

        QString nom = QInputDialog::getText(this, "Nueva Promoción", "Nombre (ej. Martes de Locura):", QLineEdit::Normal, "", &ok);
        if (!ok || nom.isEmpty()) return;

        QString fIni = QInputDialog::getText(this, "Nueva Promoción", "Fecha Inicio (YYYY-MM-DD):", QLineEdit::Normal, "2026-08-01", &ok);
        QString fFin = QInputDialog::getText(this, "Nueva Promoción", "Fecha Fin (YYYY-MM-DD):", QLineEdit::Normal, "2026-12-31", &ok);
        QString dias = QInputDialog::getText(this, "Nueva Promoción", "Días aplicables (ej. Martes, Jueves):", QLineEdit::Normal, "Martes", &ok);

        promociones.insertar(cod.toStdString(), nom.toStdString(), fIni.toStdString(), fFin.toStdString(), dias.toStdString());
        promociones.cargarEnTreeWidget(treePromos);
        QMessageBox::information(this, "Éxito", "Promoción registrada en la Lista Circular Simple.");
    });

    connect(btnDelPromo, &QPushButton::clicked, [this, treePromos]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Eliminar Promoción", "Código de la promoción a eliminar:", QLineEdit::Normal, "", &ok);
        if (ok && !cod.isEmpty()) {
            if (promociones.eliminar(cod.toStdString())) {
                promociones.cargarEnTreeWidget(treePromos);
                QMessageBox::information(this, "Éxito", "Promoción eliminada de la lista circular.");
            } else {
                QMessageBox::warning(this, "Error", "No se encontró ninguna promoción con ese código.");
            }
        }
    });

    connect(btnAddBeneficio, &QPushButton::clicked, [this, treePromos]() {
        bool ok;
        QString cod = QInputDialog::getText(this, "Añadir Beneficio", "Código de la Promoción padre (ej. Promo_01):", QLineEdit::Normal, "", &ok);
        if (!ok || cod.isEmpty()) return;

        Promocion* p = promociones.buscar(cod.toStdString());
        if (!p) {
            QMessageBox::warning(this, "Error", "La promoción especificada no existe.");
            return;
        }

        QString tipo = QInputDialog::getText(this, "Nuevo Beneficio", "Tipo de beneficio (descuento, combo, 2x1):", QLineEdit::Normal, "2x1", &ok);
        QString desc = QInputDialog::getText(this, "Nuevo Beneficio", "Descripción del beneficio:", QLineEdit::Normal, "Llevate el segundo boleto gratis", &ok);
        QString val = QInputDialog::getText(this, "Nuevo Beneficio", "Valor (ej. 5%, 10%, NA):", QLineEdit::Normal, "NA", &ok);

        p->agregarBeneficio(tipo.toStdString(), desc.toStdString(), val.toStdString());
        promociones.cargarEnTreeWidget(treePromos);
        QMessageBox::information(this, "Éxito", "Beneficio agregado a la lista doblemente enlazada de la promoción.");
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
    QPushButton* btnAnterior = new QPushButton("◄ Anterior (Circular)");
    QPushButton* btnSiguiente = new QPushButton("Siguiente (Circular) ►");
    navLayout->addWidget(btnAnterior);
    navLayout->addWidget(btnSiguiente);

    QHBoxLayout* acciones = new QHBoxLayout();
    QPushButton* btnAceptar = new QPushButton("Aprobar Solicitud");
    QPushButton* btnRechazar = new QPushButton("Rechazar Solicitud");
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
                    .arg(QString::fromStdString(solicitudActualVista->estado))
                );
        } else {
            lblDetalleSolicitudAdmin->setText("No hay solicitudes pendientes en la lista.");
        }
        if (lblTotalSolicitudesAdmin) {
            lblTotalSolicitudesAdmin->setText(QString("Total Pendientes: %1").arg(solicitudes.getTotalPendientes()));
        }
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
        Solicitud* s = solicitudes.getPrimera();
        if (!s) return;
        s->estado = "En proceso / Atendida";
        QMessageBox::information(this, "Aprobada", QString("Solicitud #%1 aprobada.").arg(s->numero));
        solicitudActualVista = nullptr;
        refrescarSolicitudesAdmin();
    });

    connect(btnRechazar, &QPushButton::clicked, [this, actualizarVistaNavegacion]() {
        if (solicitudes.rechazarPrimera()) {
            QMessageBox::information(this, "Rechazada", "Solicitud rechazada y removida.");
            solicitudActualVista = nullptr;
            refrescarSolicitudesAdmin();
        }
    });

    return tab;
}

QWidget* MainWindow::pestanaGestionFunciones() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGridLayout* gridConfig = new QGridLayout();
    QLineEdit* txtPeli = new QLineEdit(); txtPeli->setPlaceholderText("Película (ej. P001)");
    QLineEdit* txtHorario = new QLineEdit(); txtHorario->setPlaceholderText("Horario (ej. 17:00)");
    QLineEdit* txtSala = new QLineEdit(); txtSala->setPlaceholderText("Sala (ej. Sala 1)");
    QLineEdit* txtFilas = new QLineEdit("5"); txtFilas->setPlaceholderText("Filas");
    QLineEdit* txtCols = new QLineEdit("8"); txtCols->setPlaceholderText("Columnas");

    QPushButton* btnCrear = new QPushButton("Crear Función");
    QPushButton* btnEliminarAsiento = new QPushButton("Eliminar Asiento Específico");
    QPushButton* btnEliminarFuncion = new QPushButton("Eliminar Función Completa");

    gridConfig->addWidget(new QLabel("Película:"), 0, 0); gridConfig->addWidget(txtPeli, 0, 1);
    gridConfig->addWidget(new QLabel("Horario:"), 0, 2); gridConfig->addWidget(txtHorario, 0, 3);
    gridConfig->addWidget(new QLabel("Sala:"), 0, 4); gridConfig->addWidget(txtSala, 0, 5);

    gridConfig->addWidget(new QLabel("Filas:"), 1, 0); gridConfig->addWidget(txtFilas, 1, 1);
    gridConfig->addWidget(new QLabel("Columnas:"), 1, 2); gridConfig->addWidget(txtCols, 1, 3);
    gridConfig->addWidget(btnCrear, 1, 4, 1, 2);

    QHBoxLayout* actionBtns = new QHBoxLayout();
    actionBtns->addWidget(btnEliminarAsiento);
    actionBtns->addWidget(btnEliminarFuncion);

    QLabel* lblHeaderFuncion = new QLabel("Función Activa: Ninguna");
    lblHeaderFuncion->setFont(QFont("Arial", 11, QFont::Bold));
    lblHeaderFuncion->setStyleSheet("color: #2196f3; padding: 5px;");

    mapaVisualAdmin = new QTableWidget();
    mapaVisualAdmin->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addLayout(gridConfig);
    layout->addWidget(lblHeaderFuncion);
    layout->addLayout(actionBtns);
    layout->addWidget(new QLabel("Mapa de Asientos (Matriz Dispersa):"));
    layout->addWidget(mapaVisualAdmin);

    connect(btnCrear, &QPushButton::clicked, [this, txtPeli, txtHorario, txtSala, txtFilas, txtCols, lblHeaderFuncion]() {
        int f = txtFilas->text().toInt();
        int c = txtCols->text().toInt();
        if (f <= 0 || c <= 0) { f = 5; c = 8; }

        if (matrizActual) delete matrizActual;
        matrizActual = new MatrizDispersa(f, c);

        QString p = txtPeli->text().trimmed().isEmpty() ? "P001" : txtPeli->text().trimmed();
        QString h = txtHorario->text().trimmed().isEmpty() ? "17:00" : txtHorario->text().trimmed();
        QString s = txtSala->text().trimmed().isEmpty() ? "Sala 1" : txtSala->text().trimmed();

        matrizActual->setDatosFuncion(p.toStdString(), h.toStdString(), s.toStdString());
        lblHeaderFuncion->setText(QString::fromStdString(matrizActual->getInfoFuncion()));

        refrescarMapaAsientosAdmin();
        QMessageBox::information(this, "Éxito", "Nueva función creada. La matriz anterior fue sobreescrita.");
    });

    connect(btnEliminarAsiento, &QPushButton::clicked, [this]() {
        if (!matrizActual) {
            QMessageBox::warning(this, "Error", "No existe función activa.");
            return;
        }
        bool ok;
        int f = QInputDialog::getInt(this, "Eliminar Asiento", "Número de Fila:", 1, 1, matrizActual->getFilas(), 1, &ok);
        if (!ok) return;
        int c = QInputDialog::getInt(this, "Eliminar Asiento", "Número de Columna:", 1, 1, matrizActual->getColumnas(), 1, &ok);
        if (!ok) return;

        if (matrizActual->eliminarAsiento(f, c)) {
            refrescarMapaAsientosAdmin();
            QMessageBox::information(this, "Éxito", QString("El asiento F%1-C%2 ha sido eliminado de la matriz dispersa.").arg(f).arg(c));
        } else {
            QMessageBox::warning(this, "Error", "No se encontró el nodo del asiento especificado.");
        }
    });

    connect(btnEliminarFuncion, &QPushButton::clicked, [this, lblHeaderFuncion]() {
        if (matrizActual) {
            delete matrizActual;
            matrizActual = nullptr;
            lblHeaderFuncion->setText("Función Activa: Ninguna");
            if (mapaVisualAdmin) {
                mapaVisualAdmin->setRowCount(0);
                mapaVisualAdmin->setColumnCount(0);
            }
            QMessageBox::information(this, "Eliminada", "La función y su matriz dispersa han sido eliminadas.");
        }
    });

    return tab;
}

// 3. Pestaña de Reportes Visuales Graphviz en Panel Admin
QWidget* MainWindow::pestanaReportesVisuales() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* topBtns = new QHBoxLayout();
    QPushButton* btnVerBST = new QPushButton("1. Cartelera (BST)");
    QPushButton* btnVerSolicitudes = new QPushButton("2. Solicitudes (Circular Doble)");
    QPushButton* btnVerPromos = new QPushButton("3. Promociones (Lista de Listas)");
    QPushButton* btnVerMatriz = new QPushButton("4. Asientos (Matriz Dispersa)");

    topBtns->addWidget(btnVerBST);
    topBtns->addWidget(btnVerSolicitudes);
    topBtns->addWidget(btnVerPromos);
    topBtns->addWidget(btnVerMatriz);

    QLabel* lblViewer = new QLabel("Selecciona un reporte arriba para visualizar la imagen Graphviz.");
    lblViewer->setAlignment(Qt::AlignCenter);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidget(lblViewer);
    scrollArea->setWidgetResizable(true);

    layout->addLayout(topBtns);
    layout->addWidget(scrollArea);

    auto cargarImagen = [lblViewer](const QString& ruta) {
        QPixmap pix(ruta);
        if (pix.isNull()) {
            lblViewer->setText("No se encontró el archivo de imagen:\n" + ruta + "\n\nAsegúrate de hacer clic en 'Generar Reportes Graphviz' primero.");
        } else {
            lblViewer->setPixmap(pix);
            lblViewer->adjustSize();
        }
    };

    connect(btnVerBST, &QPushButton::clicked, [cargarImagen]() { cargarImagen("../reportes/Cartelera_BST.png"); });
    connect(btnVerSolicitudes, &QPushButton::clicked, [cargarImagen]() { cargarImagen("../reportes/Lista_Solicitudes.png"); });
    connect(btnVerPromos, &QPushButton::clicked, [cargarImagen]() { cargarImagen("../reportes/Lista_Promociones.png"); });
    connect(btnVerMatriz, &QPushButton::clicked, [cargarImagen]() { cargarImagen("../reportes/Matriz_Asientos.png"); });

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
                .arg(QString::fromStdString(s->estado))
            );
    } else {
        lblDetalleSolicitudAdmin->setText("No hay solicitudes pendientes en la lista.");
    }
    if (lblTotalSolicitudesAdmin) {
        lblTotalSolicitudesAdmin->setText(QString("Total Pendientes: %1").arg(solicitudes.getTotalPendientes()));
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
    tabsUsuario->addTab(pestanaClienteReservas(), "Reservar / Cancelar Asientos");
    tabsUsuario->addTab(pestanaClientePromociones(), "Promociones Vigentes");
    tabsUsuario->addTab(pestanaClienteSolicitudes(), "Solicitudes Especiales");

    layout->addLayout(header);
    layout->addWidget(tabsUsuario);

    connect(btnLogout, &QPushButton::clicked, [this]() { panelPrincipal->setCurrentIndex(0); });

    return vista;
}

QWidget* MainWindow::pestanaClienteCartelera() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLineEdit* txtBuscarCod = new QLineEdit(); txtBuscarCod->setPlaceholderText("Código de película (ej. P001)");
    QPushButton* btnBuscar = new QPushButton("Buscar Película");
    QPushButton* btnVer = new QPushButton("Actualizar Cartelera Completa");

    searchLayout->addWidget(txtBuscarCod);
    searchLayout->addWidget(btnBuscar);
    searchLayout->addWidget(btnVer);

    QTableWidget* tablaUser = new QTableWidget(0, 9);
    tablaUser->setHorizontalHeaderLabels({"CÓDIGO", "TÍTULO", "GÉNERO", "DURACIÓN", "CLASIFICACIÓN", "IDIOMA", "ESTRENO", "FIN", "ESTADO CARTELERA"});
    tablaUser->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addLayout(searchLayout);
    layout->addWidget(tablaUser);

    connect(btnVer, &QPushButton::clicked, [this, tablaUser]() {
        cartelera.cargarEnTabla(tablaUser);
    });

    connect(btnBuscar, &QPushButton::clicked, [this, txtBuscarCod]() {
        QString cod = txtBuscarCod->text().trimmed();
        Pelicula* p = cartelera.buscar(cod.toStdString());
        if (p) {
            QMessageBox::information(this, "Detalle de Película",
                                     QString("Título: %1\nGénero: %2\nDuración: %3 min\nClasificación: %4\nIdioma: %5")
                                         .arg(QString::fromStdString(p->titulo))
                                         .arg(QString::fromStdString(p->genero))
                                         .arg(p->duracion)
                                         .arg(QString::fromStdString(p->clasificacion))
                                         .arg(QString::fromStdString(p->idioma)));
        } else {
            QMessageBox::warning(this, "No encontrada", "No existe película registrada con ese código.");
        }
    });

    return tab;
}

// 1. Pestaña de Reservas del Cliente con Selección/Información de Película Activa
QWidget* MainWindow::pestanaClienteReservas() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLabel* lblPeliculaActiva = new QLabel("FUNCIÓN ACTIVA: Ninguna cargada por el administrador.");
    lblPeliculaActiva->setFont(QFont("Arial", 11, QFont::Bold));
    lblPeliculaActiva->setStyleSheet("color: #ff9800; background-color: #333; padding: 10px; border-radius: 5px;");

    QTableWidget* mapaCliente = new QTableWidget();
    mapaCliente->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto refrescarMapaCliente = [this, mapaCliente, lblPeliculaActiva]() {
        if (!matrizActual) {
            lblPeliculaActiva->setText("FUNCIÓN ACTIVA: Ninguna cargada por el administrador.");
            mapaCliente->setRowCount(0);
            mapaCliente->setColumnCount(0);
            return;
        }

        lblPeliculaActiva->setText(QString("FUNCIÓN SELECCIONADA ➔ %1").arg(QString::fromStdString(matrizActual->getInfoFuncion())));

        int f = matrizActual->getFilas();
        int c = matrizActual->getColumnas();
        mapaCliente->setRowCount(f);
        mapaCliente->setColumnCount(c);

        for (int r = 1; r <= f; r++) {
            for (int col = 1; col <= c; col++) {
                NodoMatriz* nodo = matrizActual->buscarAsiento(r, col);
                QTableWidgetItem* item = new QTableWidgetItem();
                if (nodo && nodo->ocupado) {
                    item->setText(QString("F%1-C%2\n[X]").arg(r).arg(col));
                    item->setBackground(QBrush(QColor(198, 40, 40))); // ROJO
                    item->setForeground(QBrush(Qt::white));
                } else {
                    item->setText(QString("F%1-C%2\n[LIBRE]").arg(r).arg(col));
                    item->setBackground(QBrush(QColor(46, 125, 50))); // VERDE
                    item->setForeground(QBrush(Qt::white));
                }
                item->setTextAlignment(Qt::AlignCenter);
                mapaCliente->setItem(r - 1, col - 1, item);
            }
        }
    };

    QHBoxLayout* inputs = new QHBoxLayout();
    QLineEdit* txtFila = new QLineEdit(); txtFila->setPlaceholderText("Fila (ej. 1)");
    QLineEdit* txtCol = new QLineEdit(); txtCol->setPlaceholderText("Columna (ej. 1)");
    QLineEdit* txtCliente = new QLineEdit(); txtCliente->setPlaceholderText("Nombre del Cliente");
    QPushButton* btnReservar = new QPushButton("Confirmar Reserva");
    QPushButton* btnCancelarReserva = new QPushButton("Cancelar Reserva");
    QPushButton* btnListarReservas = new QPushButton("Mis Reservas Activas");

    inputs->addWidget(txtFila);
    inputs->addWidget(txtCol);
    inputs->addWidget(txtCliente);
    inputs->addWidget(btnReservar);
    inputs->addWidget(btnCancelarReserva);
    inputs->addWidget(btnListarReservas);

    layout->addWidget(lblPeliculaActiva);
    layout->addWidget(mapaCliente);
    layout->addLayout(inputs);

    connect(btnReservar, &QPushButton::clicked, [this, txtFila, txtCol, txtCliente, refrescarMapaCliente]() {
        if (!matrizActual) {
            QMessageBox::warning(this, "Error", "No hay función activa creada por el administrador.");
            return;
        }
        int f = txtFila->text().toInt();
        int c = txtCol->text().toInt();
        QString cli = txtCliente->text().trimmed();
        if (cli.isEmpty()) cli = "Cliente";

        if (matrizActual->reservarAsiento(f, c, cli.toStdString())) {
            refrescarMapaCliente();
            refrescarMapaAsientosAdmin();
            QMessageBox::information(this, "Reserva Confirmada", QString("Asiento F%1-C%2 asignado exitosamente a %3.").arg(f).arg(c).arg(cli));
        } else {
            QMessageBox::warning(this, "Asiento Ocupado", "El asiento ya está reservado. Por favor seleccione otro asiento libre en verde.");
        }
    });

    connect(btnCancelarReserva, &QPushButton::clicked, [this, txtFila, txtCol, txtCliente, refrescarMapaCliente]() {
        if (!matrizActual) return;
        int f = txtFila->text().toInt();
        int c = txtCol->text().toInt();
        QString cli = txtCliente->text().trimmed();

        if (matrizActual->cancelarReserva(f, c, cli.toStdString())) {
            refrescarMapaCliente();
            refrescarMapaAsientosAdmin();
            QMessageBox::information(this, "Cancelada", QString("Reserva F%1-C%2 cancelada. Asiento disponible nuevamente.").arg(f).arg(c));
        } else {
            QMessageBox::warning(this, "Error", "No se encontró una reserva a ese nombre en dicha posición.");
        }
    });

    connect(btnListarReservas, &QPushButton::clicked, [this, txtCliente]() {
        if (!matrizActual) return;
        QString cli = txtCliente->text().trimmed();
        if (cli.isEmpty()) {
            QMessageBox::warning(this, "Atención", "Ingrese su nombre para buscar sus reservas.");
            return;
        }
        QString lista = "";
        for (int r = 1; r <= matrizActual->getFilas(); r++) {
            for (int col = 1; col <= matrizActual->getColumnas(); col++) {
                NodoMatriz* n = matrizActual->buscarAsiento(r, col);
                if (n && n->ocupado && QString::fromStdString(n->cliente) == cli) {
                    lista += QString("• Fila %1 - Columna %2\n").arg(r).arg(col);
                }
            }
        }
        if (lista.isEmpty()) QMessageBox::information(this, "Sin Reservas", "No tiene reservas activas registradas.");
        else QMessageBox::information(this, "Reservas Activas", "Asientos reservados a nombre de " + cli + ":\n\n" + lista);
    });

    refrescarMapaCliente();

    return tab;
}

QWidget* MainWindow::pestanaClientePromociones() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QPushButton* btnCargarPromos = new QPushButton("Ver Promociones Vigentes");
    QTreeWidget* treeUserPromos = new QTreeWidget();
    treeUserPromos->setColumnCount(4);
    treeUserPromos->setHeaderLabels({"CÓDIGO", "PROMOCIÓN / BENEFICIO", "VIGENCIA / VALOR", "DÍAS / DESCRIPCIÓN"});
    treeUserPromos->header()->setSectionResizeMode(QHeaderView::Stretch);

    layout->addWidget(btnCargarPromos);
    layout->addWidget(treeUserPromos);

    connect(btnCargarPromos, &QPushButton::clicked, [this, treeUserPromos]() {
        promociones.cargarEnTreeWidget(treeUserPromos);
    });

    return tab;
}

// 2. Pestaña de Solicitudes Especiales con Limpieza de Campos Automática (clear)
QWidget* MainWindow::pestanaClienteSolicitudes() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLabel* lblEnviar = new QLabel("Realizar Solicitud Especial:");
    QLineEdit* txtNombre = new QLineEdit(); txtNombre->setPlaceholderText("Nombre del Cliente");
    QLineEdit* txtTel = new QLineEdit(); txtTel->setPlaceholderText("Teléfono");
    QLineEdit* txtTipo = new QLineEdit(); txtTipo->setPlaceholderText("Tipo (Cumpleaños, Evento, etc.)");
    QLineEdit* txtDesc = new QLineEdit(); txtDesc->setPlaceholderText("Descripción");
    QPushButton* btnEnviar = new QPushButton("Enviar Solicitud");

    QFrame* line = new QFrame(); line->setFrameShape(QFrame::HLine);

    QLabel* lblConsultar = new QLabel("Consultar Estado de Solicitud por Teléfono:");
    QHBoxLayout* consultLayout = new QHBoxLayout();
    QLineEdit* txtConsultarTel = new QLineEdit(); txtConsultarTel->setPlaceholderText("Ingrese su Teléfono");
    QPushButton* btnConsultar = new QPushButton("Consultar Estado");
    consultLayout->addWidget(txtConsultarTel); consultLayout->addWidget(btnConsultar);

    layout->addWidget(lblEnviar);
    layout->addWidget(txtNombre); layout->addWidget(txtTel);
    layout->addWidget(txtTipo); layout->addWidget(txtDesc);
    layout->addWidget(btnEnviar);
    layout->addWidget(line);
    layout->addWidget(lblConsultar);
    layout->addLayout(consultLayout);
    layout->addStretch();

    connect(btnEnviar, &QPushButton::clicked, [this, txtNombre, txtTel, txtTipo, txtDesc]() {
        if (txtNombre->text().isEmpty() || txtTel->text().isEmpty()) {
            QMessageBox::warning(this, "Campos Vacíos", "Por favor complete al menos el Nombre y Teléfono.");
            return;
        }

        solicitudes.insertar(txtNombre->text().toStdString(), txtTel->text().toStdString(),
                             txtTipo->text().toStdString(), txtDesc->text().toStdString(), "2026-08-19");
        refrescarSolicitudesAdmin();

        // Limpiar los campos de texto
        txtNombre->clear();
        txtTel->clear();
        txtTipo->clear();
        txtDesc->clear();

        QMessageBox::information(this, "Éxito", "Solicitud registrada para revisión del administrador.");
    });

    connect(btnConsultar, &QPushButton::clicked, [this, txtConsultarTel]() {
        QString tel = txtConsultarTel->text().trimmed();
        Solicitud* s = solicitudes.buscarPorTelefono(tel.toStdString());
        if (s) {
            QMessageBox::information(this, "Estado de Solicitud",
                                     QString("Número: #%1\nCliente: %2\nTipo: %3\nDescripción: %4\nEstado: %5")
                                         .arg(s->numero)
                                         .arg(QString::fromStdString(s->cliente))
                                         .arg(QString::fromStdString(s->tipo))
                                         .arg(QString::fromStdString(s->descripcion))
                                         .arg(QString::fromStdString(s->estado)));
        } else {
            QMessageBox::warning(this, "No encontrada", "No se encontró ninguna solicitud asociada a ese número telefónico.");
        }
    });

    return tab;
}