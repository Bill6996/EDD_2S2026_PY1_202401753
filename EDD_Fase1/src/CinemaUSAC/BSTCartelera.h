#ifndef BSTCARTELERA_H
#define BSTCARTELERA_H

#include "Pelicula.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <QTableWidget>
#include <QDateTime>

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
            // Nodo con un solo hijo o sin hijos
            if (nodo->izquierda == nullptr) {
                Pelicula* temp = nodo->derecha;
                delete nodo;
                return temp;
            } else if (nodo->derecha == nullptr) {
                Pelicula* temp = nodo->izquierda;
                delete nodo;
                return temp;
            }
            // Nodo con dos hijos
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
            file << "  node_" << nodo->codigo << " [label=\"Codigo: " << nodo->codigo
                 << "\\nTitulo: " << nodo->titulo
                 << "\\nDuracion: " << nodo->duracion << " min"
                 << "\\nClasificacion: " << nodo->clasificacion << "\", shape=box, style=filled, fillcolor=lightgreen];\n";

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

        // Calcular Estado de Cartelera
        QDate fEstreno = QDate::fromString(QString::fromStdString(nodo->fechaEstreno), "yyyy-MM-dd");
        QDate fFin = QDate::fromString(QString::fromStdString(nodo->fechaFin), "yyyy-MM-dd");
        QDate hoy = QDate::currentDate();

        QString estado = "En cartelera";
        if (fEstreno.isValid() && hoy < fEstreno) {
            estado = "Próximo estreno";
        } else if (fFin.isValid() && hoy.daysTo(fFin) <= 7 && hoy.daysTo(fFin) >= 0) {
            estado = "Próximo a retirar";
        }

        QTableWidgetItem* itemEstado = new QTableWidgetItem(estado);
        tabla->setItem(fila, 8, itemEstado);

        // Resaltado de Alerta
        if (estado == "Próximo a retirar") {
            for (int col = 0; col < 9; col++) {
                tabla->item(fila, col)->setBackground(QBrush(QColor(255, 235, 59))); // Amarillo
                tabla->item(fila, col)->setForeground(QBrush(Qt::black));
            }
        }

        fila++;
        inordenTabla(nodo->derecha, tabla, fila);
    }

public:
    BSTCartelera() : raiz(nullptr) {}

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

    void generarReporteGraphviz() {
        std::ofstream file("../reportes/BST_Cartelera.dot");
        file << "digraph BST_Cartelera {\n";
        file << "  node [fontname=\"Arial\"];\n";
        if (raiz == nullptr) {
            file << "  empty [label=\"Cartelera Vacia\"];\n";
        } else {
            generarGraphvizRecursivo(raiz, file);
        }
        file << "}\n";
        file.close();

        system("dot -Tpng ../reportes/BST_Cartelera.dot -o ../reportes/BST_Cartelera.png");
    }

    void cargarEnTabla(QTableWidget* tabla) {
        tabla->setRowCount(0);
        int fila = 0;
        inordenTabla(raiz, tabla, fila);
    }
};

#endif // BSTCARTELERA_H