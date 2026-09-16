#ifndef BSTCARTELERA_H
#define BSTCARTELERA_H

#include "Pelicula.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <QTableWidget>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QDate>
#include <QProcess>

//aqui implemente el manejo del arbol binario de busqueda
//se implemento insercion, eliminacion y bisqueda
//y reporte de graphyz
class BSTCartelera {
private:
    Pelicula* raiz;

    Pelicula* insertarRecursivo(Pelicula* nodo, Pelicula* nuevo) {
        if (nodo == nullptr) return nuevo;
        if (nuevo->codigo < nodo->codigo) {
            nodo->izquierda = insertarRecursivo(nodo->izquierda, nuevo);
        } else if (nuevo->codigo > nodo->codigo) {
            nodo->derecha = insertarRecursivo(nodo->derecha, nuevo);
        }
        return nodo;
    }

    Pelicula* obtenerMinimo(Pelicula* nodo) {
        Pelicula* actual = nodo;
        while (actual && actual->izquierda != nullptr) {
            actual = actual->izquierda;
        }
        return actual;
    }

    Pelicula* eliminarRecursivo(Pelicula* nodo, std::string cod) {
        if (nodo == nullptr) return nodo;

        if (cod < nodo->codigo) {
            nodo->izquierda = eliminarRecursivo(nodo->izquierda, cod);
        } else if (cod > nodo->codigo) {
            nodo->derecha = eliminarRecursivo(nodo->derecha, cod);
        } else {
            // Caso 1 y 2: Un solo hijo o sin hijos
            if (nodo->izquierda == nullptr) {
                Pelicula* temp = nodo->derecha;
                delete nodo;
                return temp;
            } else if (nodo->derecha == nullptr) {
                Pelicula* temp = nodo->izquierda;
                delete nodo;
                return temp;
            }
            // Caso 3: Dos hijos
            Pelicula* temp = obtenerMinimo(nodo->derecha);
            nodo->codigo = temp->codigo;
            nodo->titulo = temp->titulo;
            nodo->genero = temp->genero;
            nodo->duracion = temp->duracion;
            nodo->clasificacion = temp->clasificacion;
            nodo->idioma = temp->idioma;
            nodo->fechaEstreno = temp->fechaEstreno;
            nodo->fechaFin = temp->fechaFin;
            nodo->derecha = eliminarRecursivo(nodo->derecha, temp->codigo);
        }
        return nodo;
    }

    Pelicula* buscarRecursivo(Pelicula* nodo, std::string cod) {
        if (nodo == nullptr || nodo->codigo == cod) return nodo;
        if (cod < nodo->codigo) return buscarRecursivo(nodo->izquierda, cod);
        return buscarRecursivo(nodo->derecha, cod);
    }

    void generarGraphvizRecursivo(Pelicula* nodo, std::ofstream& file) {
        if (nodo != nullptr) {
            QDate fFin = QDate::fromString(QString::fromStdString(nodo->fechaFin), "yyyy-MM-dd");
            QDate hoy = QDate::currentDate();
            std::string color = "lightgreen"; // Verde: en cartelera
            if (fFin.isValid() && hoy.daysTo(fFin) <= 7 && hoy.daysTo(fFin) >= 0) {
                color = "yellow"; // Amarillo: próximo a retirar
            }

            file << "  node_" << nodo->codigo << " [label=\"Código: " << nodo->codigo
                 << "\\nTítulo: " << nodo->titulo
                 << "\\nDuración: " << nodo->duracion << " min"
                 << "\\nClasificación: " << nodo->clasificacion << "\", shape=box, style=filled, fillcolor=" << color << "];\n";

            if (nodo->izquierda != nullptr) {
                file << "  node_" << nodo->codigo << " -> node_" << nodo->izquierda->codigo << ";\n";
                generarGraphvizRecursivo(nodo->izquierda, file);
            }
            if (nodo->derecha != nullptr) {
                file << "  node_" << nodo->codigo << " -> node_" << nodo->derecha->codigo << ";\n";
                generarGraphvizRecursivo(nodo->derecha, file);
            }
        }
    }

    void inordenTabla(Pelicula* nodo, QTableWidget* tabla, int& fila) {
        if (nodo == nullptr) return;
        inordenTabla(nodo->izquierda, tabla, fila);

        tabla->insertRow(fila);
        tabla->setItem(fila, 0, new QTableWidgetItem(QString::fromStdString(nodo->codigo)));
        tabla->setItem(fila, 1, new QTableWidgetItem(QString::fromStdString(nodo->titulo)));
        tabla->setItem(fila, 2, new QTableWidgetItem(QString::fromStdString(nodo->genero)));
        tabla->setItem(fila, 3, new QTableWidgetItem(QString::number(nodo->duracion) + " min"));
        tabla->setItem(fila, 4, new QTableWidgetItem(QString::fromStdString(nodo->clasificacion)));
        tabla->setItem(fila, 5, new QTableWidgetItem(QString::fromStdString(nodo->idioma)));
        tabla->setItem(fila, 6, new QTableWidgetItem(QString::fromStdString(nodo->fechaEstreno)));
        tabla->setItem(fila, 7, new QTableWidgetItem(QString::fromStdString(nodo->fechaFin)));

        // Calcular Estado de Cartelera para la columna 9
        QDate fEstreno = QDate::fromString(QString::fromStdString(nodo->fechaEstreno), "yyyy-MM-dd");
        QDate fFin = QDate::fromString(QString::fromStdString(nodo->fechaFin), "yyyy-MM-dd");
        QDate hoy = QDate::currentDate();

        QString estado = "En cartelera";
        if (fEstreno.isValid() && hoy < fEstreno) {
            estado = "Próximo estreno";
        } else if (fFin.isValid() && hoy.daysTo(fFin) <= 7 && hoy.daysTo(fFin) >= 0) {
            estado = "Próximo a retirar";
        }

        tabla->setItem(fila, 8, new QTableWidgetItem(estado));

        // Resaltar en amarillo las 9 columnas si está próximo a retirar
        if (estado == "Próximo a retirar") {
            for (int col = 0; col < 9; col++) {
                tabla->item(fila, col)->setBackground(QBrush(QColor(255, 235, 59)));
                tabla->item(fila, col)->setForeground(QBrush(Qt::black));
            }
        }

        fila++;
        inordenTabla(nodo->derecha, tabla, fila);
    }

    void preordenTabla(Pelicula* nodo, QTableWidget* tabla, int& fila) {
        if (nodo == nullptr) return;

        tabla->insertRow(fila);
        tabla->setItem(fila, 0, new QTableWidgetItem(QString::fromStdString(nodo->codigo)));
        tabla->setItem(fila, 1, new QTableWidgetItem(QString::fromStdString(nodo->titulo)));
        tabla->setItem(fila, 2, new QTableWidgetItem(QString::fromStdString(nodo->genero)));
        tabla->setItem(fila, 3, new QTableWidgetItem(QString::number(nodo->duracion) + " min"));
        tabla->setItem(fila, 4, new QTableWidgetItem(QString::fromStdString(nodo->clasificacion)));
        tabla->setItem(fila, 5, new QTableWidgetItem(QString::fromStdString(nodo->idioma)));
        tabla->setItem(fila, 6, new QTableWidgetItem(QString::fromStdString(nodo->fechaEstreno)));
        tabla->setItem(fila, 7, new QTableWidgetItem(QString::fromStdString(nodo->fechaFin)));

        QDate fEstreno = QDate::fromString(QString::fromStdString(nodo->fechaEstreno), "yyyy-MM-dd");
        QDate fFin = QDate::fromString(QString::fromStdString(nodo->fechaFin), "yyyy-MM-dd");
        QDate hoy = QDate::currentDate();
        QString estado = "En cartelera";
        if (fEstreno.isValid() && hoy < fEstreno) estado = "Próximo estreno";
        else if (fFin.isValid() && hoy.daysTo(fFin) <= 7 && hoy.daysTo(fFin) >= 0) estado = "Próximo a retirar";
        tabla->setItem(fila, 8, new QTableWidgetItem(estado));
        fila++;

        preordenTabla(nodo->izquierda, tabla, fila);
        preordenTabla(nodo->derecha, tabla, fila);
    }

    void postordenTabla(Pelicula* nodo, QTableWidget* tabla, int& fila) {
        if (nodo == nullptr) return;
        postordenTabla(nodo->izquierda, tabla, fila);
        postordenTabla(nodo->derecha, tabla, fila);

        tabla->insertRow(fila);
        tabla->setItem(fila, 0, new QTableWidgetItem(QString::fromStdString(nodo->codigo)));
        tabla->setItem(fila, 1, new QTableWidgetItem(QString::fromStdString(nodo->titulo)));
        tabla->setItem(fila, 2, new QTableWidgetItem(QString::fromStdString(nodo->genero)));
        tabla->setItem(fila, 3, new QTableWidgetItem(QString::number(nodo->duracion) + " min"));
        tabla->setItem(fila, 4, new QTableWidgetItem(QString::fromStdString(nodo->clasificacion)));
        tabla->setItem(fila, 5, new QTableWidgetItem(QString::fromStdString(nodo->idioma)));
        tabla->setItem(fila, 6, new QTableWidgetItem(QString::fromStdString(nodo->fechaEstreno)));
        tabla->setItem(fila, 7, new QTableWidgetItem(QString::fromStdString(nodo->fechaFin)));

        QDate fEstreno = QDate::fromString(QString::fromStdString(nodo->fechaEstreno), "yyyy-MM-dd");
        QDate fFin = QDate::fromString(QString::fromStdString(nodo->fechaFin), "yyyy-MM-dd");
        QDate hoy = QDate::currentDate();
        QString estado = "En cartelera";
        if (fEstreno.isValid() && hoy < fEstreno) estado = "Próximo estreno";
        else if (fFin.isValid() && hoy.daysTo(fFin) <= 7 && hoy.daysTo(fFin) >= 0) estado = "Próximo a retirar";
        tabla->setItem(fila, 8, new QTableWidgetItem(estado));
        fila++;
    }


    void destruirArbol(Pelicula* nodo) {
        if (nodo != nullptr) {
            destruirArbol(nodo->izquierda);
            destruirArbol(nodo->derecha);
            delete nodo;
        }
    }

public:
    BSTCartelera() : raiz(nullptr) {}

    ~BSTCartelera() {
        destruirArbol(raiz);
    }

    void insertar(std::string cod, std::string tit, std::string gen, int dur,
                  std::string clas, std::string idio, std::string fEstreno, std::string fFin) {
        Pelicula* nuevo = new Pelicula(cod, tit, gen, dur, clas, idio, fEstreno, fFin);
        raiz = insertarRecursivo(raiz, nuevo);
    }

    void eliminar(std::string cod) {
        raiz = eliminarRecursivo(raiz, cod);
    }

    Pelicula* buscar(std::string cod) {
        return buscarRecursivo(raiz, cod);
    }

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

    void generarReporteGraphviz() {
        QString appPath = QCoreApplication::applicationDirPath();
        QDir().mkpath(appPath + "/reportes");

        QString dotFile = appPath + "/reportes/Cartelera_BST.dot";
        QString pngFile = appPath + "/reportes/Cartelera_BST.png";

        std::ofstream file(dotFile.toStdString());
        file << "digraph BSTCartelera {\n";
        file << "  node [fontname=\"Arial\"];\n";

        if (raiz == nullptr) {
            file << "  empty [label=\"Cartelera Vacía\", shape=box];\n";
        } else {
            generarGraphvizRecursivo(raiz, file);
        }

        file << "}\n";
        file.close();

        // Ejecución robusta con QProcess
        QProcess process;
        process.start("dot", QStringList() << "-Tpng" << dotFile << "-o" << pngFile);
        process.waitForFinished(3000);
    }
};

#endif // BSTCARTELERA_H