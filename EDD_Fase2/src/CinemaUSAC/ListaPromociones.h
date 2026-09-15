#ifndef LISTAPROMOCIONES_H
#define LISTAPROMOCIONES_H

#include "Promocion.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>

//se implemento la lista circular simple doble
//aqui se inserta promociones, agrega beneficios o promocion especifica y carga datos y genera reporte

class ListaPromociones {
private:
    Promocion* cabeza;

public:
    ListaPromociones() : cabeza(nullptr) {}

    // Inserción en Lista Circular Simple
    void insertar(std::string cod, std::string nom, std::string fInicio, std::string fFin, std::string dias) {
        Promocion* nuevo = new Promocion(cod, nom, fInicio + " a " + fFin, dias);
        if (cabeza == nullptr) {
            cabeza = nuevo;
            cabeza->siguiente = cabeza;
        } else {
            Promocion* aux = cabeza;
            while (aux->siguiente != cabeza) {
                aux = aux->siguiente;
            }
            aux->siguiente = nuevo;
            nuevo->siguiente = cabeza;
        }
    }

    // Buscar una promoción por código
    Promocion* buscar(std::string cod) {
        if (cabeza == nullptr) return nullptr;
        Promocion* aux = cabeza;
        do {
            if (aux->codigo == cod) return aux;
            aux = aux->siguiente;
        } while (aux != cabeza);
        return nullptr;
    }

    // Eliminación manual en Lista Circular Simple
    bool eliminar(std::string cod) {
        if (cabeza == nullptr) return false;

        Promocion* actual = cabeza;
        Promocion* anterior = nullptr;

        // Buscar el nodo y su anterior
        do {
            if (actual->codigo == cod) break;
            anterior = actual;
            actual = actual->siguiente;
        } while (actual != cabeza);

        // Si no se encontró
        if (actual->codigo != cod) return false;

        // Si es el único nodo de la lista
        if (actual == cabeza && actual->siguiente == cabeza) {
            delete cabeza;
            cabeza = nullptr;
            return true;
        }

        // Si el nodo a eliminar es la cabeza
        if (actual == cabeza) {
            Promocion* ultimo = cabeza;
            while (ultimo->siguiente != cabeza) {
                ultimo = ultimo->siguiente;
            }
            cabeza = cabeza->siguiente;
            ultimo->siguiente = cabeza;
            delete actual;
        } else {
            // Si es un nodo intermedio o final
            anterior->siguiente = actual->siguiente;
            delete actual;
        }

        return true;
    }

    // Renderizar la Lista de Listas en el TreeWidget de Qt
    void cargarEnTreeWidget(QTreeWidget* tree) {
        tree->clear();
        if (cabeza == nullptr) return;

        Promocion* aux = cabeza;
        do {
            QTreeWidgetItem* promoItem = new QTreeWidgetItem(tree);
            promoItem->setText(0, QString::fromStdString(aux->codigo));
            promoItem->setText(1, QString::fromStdString(aux->nombre));
            promoItem->setText(2, QString::fromStdString(aux->vigencia));
            promoItem->setText(3, QString::fromStdString(aux->diasAplicables));
            promoItem->setBackground(0, QBrush(QColor(33, 150, 243, 40))); // Resaltado Azul

            // Recorrer lista doblemente enlazada de beneficios
            Beneficio* ben = aux->listaBeneficios;
            while (ben != nullptr) {
                QTreeWidgetItem* benItem = new QTreeWidgetItem(promoItem);
                benItem->setText(0, "  └─ [BENEFICIO]");
                benItem->setText(1, QString::fromStdString(ben->tipo));
                benItem->setText(2, QString::fromStdString(ben->valor));
                benItem->setText(3, QString::fromStdString(ben->descripcion));
                benItem->setBackground(0, QBrush(QColor(255, 235, 59, 30))); // Resaltado Amarillo
                ben = ben->siguiente;
            }

            promoItem->setExpanded(true); // Desplegar beneficios automáticamente
            aux = aux->siguiente;
        } while (aux != cabeza);
    }

    void generarReporteGraphviz() {
        QString appPath = QCoreApplication::applicationDirPath();
        QDir().mkpath(appPath + "/reportes");

        QString dotFile = appPath + "/reportes/Lista_Promociones.dot";
        QString pngFile = appPath + "/reportes/Lista_Promociones.png";

        std::ofstream file(dotFile.toStdString());
        file << "digraph ListaPromociones {\n";
        file << "  rankdir=TB;\n";
        file << "  node [shape=box, fontname=\"Arial\"];\n";

        if (cabeza == nullptr) {
            file << "  empty [label=\"Sin Promociones Activas\"];\n";
        } else {
            file << "  { rank=same; ";
            Promocion* aux = cabeza;
            do {
                file << "promo_" << aux->codigo << "; ";
                aux = aux->siguiente;
            } while (aux != cabeza);
            file << "}\n";

            aux = cabeza;
            do {
                file << "  promo_" << aux->codigo << " [label=\""
                     << aux->nombre << "\\n(" << aux->codigo << ")\\nVigencia: " << aux->vigencia
                     << "\", style=filled, fillcolor=lightblue];\n";

                file << "  promo_" << aux->codigo << " -> promo_" << aux->siguiente->codigo << ";\n";

                Beneficio* ben = aux->listaBeneficios;
                int i = 0;
                std::string ultimoNodo = "promo_" + aux->codigo;

                while (ben != nullptr) {
                    std::string nodeBen = "ben_" + aux->codigo + "_" + std::to_string(i);
                    file << "  " << nodeBen << " [label=\"Tipo: " << ben->tipo << "\\n" << ben->descripcion
                         << "\\nValor: " << ben->valor << "\", style=filled, fillcolor=lightyellow];\n";

                    file << "  " << ultimoNodo << " -> " << nodeBen << " [dir=both];\n";

                    ultimoNodo = nodeBen;
                    ben = ben->siguiente;
                    i++;
                }

                aux = aux->siguiente;
            } while (aux != cabeza);
        }

        file << "}\n";
        file.close();

        QProcess process;
        process.start("dot", QStringList() << "-Tpng" << dotFile << "-o" << pngFile);
        process.waitForFinished(3000);
    }

};

#endif // LISTAPROMOCIONES_H