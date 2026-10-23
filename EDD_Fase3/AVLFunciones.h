#ifndef AVLFUNCIONES_H
#define AVLFUNCIONES_H

#include "Funcion.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QDate>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDate>

class AVLFunciones {
private:
    Funcion* raiz;

    // ----- Helpers de altura y balance -----
    int altura(Funcion* nodo) {
        return (nodo == nullptr) ? 0 : nodo->altura;
    }

    int factorBalance(Funcion* nodo) {
        return (nodo == nullptr) ? 0 : altura(nodo->izquierda) - altura(nodo->derecha);
    }

    void actualizarAltura(Funcion* nodo) {
        if (nodo != nullptr) {
            nodo->altura = 1 + std::max(altura(nodo->izquierda), altura(nodo->derecha));
        }
    }

    // ----- Rotaciones -----
    Funcion* rotacionDerecha(Funcion* y) {
        Funcion* x = y->izquierda;
        Funcion* T2 = x->derecha;

        x->derecha = y;
        y->izquierda = T2;

        actualizarAltura(y);
        actualizarAltura(x);
        return x;
    }

    Funcion* rotacionIzquierda(Funcion* x) {
        Funcion* y = x->derecha;
        Funcion* T2 = y->izquierda;

        y->izquierda = x;
        x->derecha = T2;

        actualizarAltura(x);
        actualizarAltura(y);
        return y;
    }

    // ----- Inserción recursiva con balanceo -----
    Funcion* insertarRec(Funcion* nodo, Funcion* nuevo) {
        if (nodo == nullptr) return nuevo;

        if (nuevo->codigo_funcion < nodo->codigo_funcion) {
            nodo->izquierda = insertarRec(nodo->izquierda, nuevo);
        } else if (nuevo->codigo_funcion > nodo->codigo_funcion) {
            nodo->derecha = insertarRec(nodo->derecha, nuevo);
        } else {
            // Código duplicado: no se inserta
            delete nuevo;
            return nodo;
        }

        actualizarAltura(nodo);
        int balance = factorBalance(nodo);

        // Caso LL
        if (balance > 1 && nuevo->codigo_funcion < nodo->izquierda->codigo_funcion)
            return rotacionDerecha(nodo);

        // Caso RR
        if (balance < -1 && nuevo->codigo_funcion > nodo->derecha->codigo_funcion)
            return rotacionIzquierda(nodo);

        // Caso LR
        if (balance > 1 && nuevo->codigo_funcion > nodo->izquierda->codigo_funcion) {
            nodo->izquierda = rotacionIzquierda(nodo->izquierda);
            return rotacionDerecha(nodo);
        }

        // Caso RL
        if (balance < -1 && nuevo->codigo_funcion < nodo->derecha->codigo_funcion) {
            nodo->derecha = rotacionDerecha(nodo->derecha);
            return rotacionIzquierda(nodo);
        }

        return nodo;
    }

    // ----- Obtener el mínimo (para eliminación) -----
    Funcion* minimo(Funcion* nodo) {
        while (nodo && nodo->izquierda != nullptr)
            nodo = nodo->izquierda;
        return nodo;
    }

    // ----- Eliminación recursiva con balanceo -----
    Funcion* eliminarRec(Funcion* nodo, const std::string& codigo) {
        if (nodo == nullptr) return nullptr;

        if (codigo < nodo->codigo_funcion) {
            nodo->izquierda = eliminarRec(nodo->izquierda, codigo);
        } else if (codigo > nodo->codigo_funcion) {
            nodo->derecha = eliminarRec(nodo->derecha, codigo);
        } else {
            // Nodo encontrado
            if (nodo->izquierda == nullptr || nodo->derecha == nullptr) {
                Funcion* temp = nodo->izquierda ? nodo->izquierda : nodo->derecha;
                if (temp == nullptr) {
                    temp = nodo;
                    nodo = nullptr;
                } else {
                    // Copiar datos
                    *nodo = *temp;
                    nodo->izquierda = temp->izquierda;
                    nodo->derecha = temp->derecha;
                }
                delete temp;
            } else {
                Funcion* temp = minimo(nodo->derecha);
                nodo->codigo_funcion = temp->codigo_funcion;
                nodo->codigo_pelicula = temp->codigo_pelicula;
                nodo->horario = temp->horario;
                nodo->sala = temp->sala;
                nodo->filas = temp->filas;
                nodo->columnas = temp->columnas;
                nodo->archivo_asientos = temp->archivo_asientos;
                nodo->fecha = temp->fecha;
                nodo->codigo_sede = temp->codigo_sede;      // Fase 3: no perder la sede al copiar el sucesor
                nodo->derecha = eliminarRec(nodo->derecha, temp->codigo_funcion);
            }
        }

        if (nodo == nullptr) return nullptr;

        actualizarAltura(nodo);
        int balance = factorBalance(nodo);

        // Rebalancear
        if (balance > 1 && factorBalance(nodo->izquierda) >= 0)
            return rotacionDerecha(nodo);
        if (balance > 1 && factorBalance(nodo->izquierda) < 0) {
            nodo->izquierda = rotacionIzquierda(nodo->izquierda);
            return rotacionDerecha(nodo);
        }
        if (balance < -1 && factorBalance(nodo->derecha) <= 0)
            return rotacionIzquierda(nodo);
        if (balance < -1 && factorBalance(nodo->derecha) > 0) {
            nodo->derecha = rotacionDerecha(nodo->derecha);
            return rotacionIzquierda(nodo);
        }

        return nodo;
    }

    // ----- Búsqueda -----
    Funcion* buscarRec(Funcion* nodo, const std::string& codigo) const {
        if (nodo == nullptr || nodo->codigo_funcion == codigo)
            return nodo;
        if (codigo < nodo->codigo_funcion)
            return buscarRec(nodo->izquierda, codigo);
        return buscarRec(nodo->derecha, codigo);
    }

    // ----- Agrega una fila (8 columnas) a la tabla con los datos de la función -----
    void agregarFilaTabla(Funcion* nodo, QTableWidget* tabla, int& fila) {
        tabla->insertRow(fila);
        tabla->setItem(fila, 0, new QTableWidgetItem(QString::fromStdString(nodo->codigo_funcion)));
        tabla->setItem(fila, 1, new QTableWidgetItem(QString::fromStdString(nodo->codigo_pelicula)));
        tabla->setItem(fila, 2, new QTableWidgetItem(QString::fromStdString(nodo->horario)));
        tabla->setItem(fila, 3, new QTableWidgetItem(QString::fromStdString(nodo->sala)));
        tabla->setItem(fila, 4, new QTableWidgetItem(QString::number(nodo->filas)));
        tabla->setItem(fila, 5, new QTableWidgetItem(QString::number(nodo->columnas)));
        tabla->setItem(fila, 6, new QTableWidgetItem(QString::fromStdString(nodo->archivo_asientos)));
        tabla->setItem(fila, 7, new QTableWidgetItem(QString::fromStdString(nodo->codigo_sede)));   // Fase 3
        fila++;
    }

    // ----- Recorridos para la tabla -----
    void inordenTabla(Funcion* nodo, QTableWidget* tabla, int& fila) {
        if (nodo == nullptr) return;
        inordenTabla(nodo->izquierda, tabla, fila);

        agregarFilaTabla(nodo, tabla, fila);

        inordenTabla(nodo->derecha, tabla, fila);
    }

    void preordenTabla(Funcion* nodo, QTableWidget* tabla, int& fila) {
        if (nodo == nullptr) return;

        agregarFilaTabla(nodo, tabla, fila);

        preordenTabla(nodo->izquierda, tabla, fila);
        preordenTabla(nodo->derecha, tabla, fila);
    }

    void postordenTabla(Funcion* nodo, QTableWidget* tabla, int& fila) {
        if (nodo == nullptr) return;
        postordenTabla(nodo->izquierda, tabla, fila);
        postordenTabla(nodo->derecha, tabla, fila);

        agregarFilaTabla(nodo, tabla, fila);
    }

    // ----- Graphviz -----
    void generarGraphvizRec(Funcion* nodo, std::ofstream& file) {
        if (nodo == nullptr) return;

        // Por ahora todas verdes (después se puede colorear según si ya pasó la fecha)
        QDate fFun = QDate::fromString(QString::fromStdString(nodo->fecha), "yyyy-MM-dd");
            QDate hoy = QDate::currentDate();
            std::string color = "lightgreen"; // programada
            if (fFun.isValid() && fFun < hoy) {
                color = "tomato"; // ya se realizó
            }

        file << "  node_" << nodo->codigo_funcion
             << " [label=\"" << nodo->codigo_funcion
             << "\\nPelícula: " << nodo->codigo_pelicula
             << "\\n" << nodo->horario
             << "\\n" << nodo->sala
             << "\\n" << nodo->fecha
             << "\\nSede: " << nodo->codigo_sede
             << "\", shape=circle, style=filled, fillcolor=" << color << "];\n";

        if (nodo->izquierda) {
            file << "  node_" << nodo->codigo_funcion << " -> node_" << nodo->izquierda->codigo_funcion << ";\n";
            generarGraphvizRec(nodo->izquierda, file);
        }
        if (nodo->derecha) {
            file << "  node_" << nodo->codigo_funcion << " -> node_" << nodo->derecha->codigo_funcion << ";\n";
            generarGraphvizRec(nodo->derecha, file);
        }
    }

    template <typename F>
    void inordenGenerico(Funcion* nodo, F& accion) const {
        if (nodo == nullptr) return;
        inordenGenerico(nodo->izquierda, accion);
        accion(nodo);
        inordenGenerico(nodo->derecha, accion);
    }

    template <typename F>
    void preordenGenerico(Funcion* nodo, F& accion) const {
        if (nodo == nullptr) return;
        accion(nodo);
        preordenGenerico(nodo->izquierda, accion);
        preordenGenerico(nodo->derecha, accion);
    }

    // ----- Destructor helper -----
    void destruir(Funcion* nodo) {
        if (nodo == nullptr) return;
        destruir(nodo->izquierda);
        destruir(nodo->derecha);
        delete nodo;
    }

public:
    AVLFunciones() : raiz(nullptr) {}

    ~AVLFunciones() {
        destruir(raiz);
    }

    // Insertar nueva función
    void insertar(std::string codFun, std::string codPel, std::string horario,
                  std::string sala, int filas, int columnas, std::string archivo = "",
                  std::string fecha = "", std::string sede = "") {
        Funcion* nuevo = new Funcion(codFun, codPel, horario, sala, filas, columnas, archivo, fecha, sede);
        raiz = insertarRec(raiz, nuevo);
    }
    // Eliminar por código
    void eliminar(const std::string& codigo) {
        raiz = eliminarRec(raiz, codigo);
    }

    // Buscar por código
    Funcion* buscar(const std::string& codigo) const {
        return buscarRec(raiz, codigo);
    }

    // Cargar en tabla según recorrido
    void cargarEnTabla(QTableWidget* tabla, const std::string& recorrido = "inorden") {
        tabla->setRowCount(0);
        int fila = 0;
        if (recorrido == "preorden")
            preordenTabla(raiz, tabla, fila);
        else if (recorrido == "postorden")
            postordenTabla(raiz, tabla, fila);
        else
            inordenTabla(raiz, tabla, fila);
    }

    // Generar reporte Graphviz
    void generarReporteGraphviz() {
        QString appPath = QCoreApplication::applicationDirPath();
        QDir().mkpath(appPath + "/reportes");

        QString dotFile = appPath + "/reportes/AVL_Funciones.dot";
        QString pngFile = appPath + "/reportes/AVL_Funciones.png";

        std::ofstream file(dotFile.toStdString());
        file << "digraph AVLFunciones {\n";
        file << "  node [fontname=\"Arial\"];\n";

        if (raiz == nullptr) {
            file << "  empty [label=\"Sin funciones\", shape=box];\n";
        } else {
            generarGraphvizRec(raiz, file);
        }

        file << "}\n";
        file.close();

        QProcess process;
        process.start("dot", QStringList() << "-Tpng" << dotFile << "-o" << pngFile);
        process.waitForFinished(5000);
    }

    bool estaVacio() const { return raiz == nullptr; }

    // ----- Fase 3: recorridos genéricos (aceptan una lambda) -----
    // Inorden: devuelve las funciones ordenadas por código (para listados y consultas)
    template <typename F>
    void recorrerInorden(F accion) const { inordenGenerico(raiz, accion); }

    // Preorden: se usa al serializar (reinsertar en preorden reproduce la forma del árbol)
    template <typename F>
    void recorrerPreorden(F accion) const { preordenGenerico(raiz, accion); }

};

#endif // AVLFUNCIONES_H