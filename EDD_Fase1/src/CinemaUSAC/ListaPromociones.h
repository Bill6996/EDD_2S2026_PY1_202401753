#ifndef LISTAPROMOCIONES_H
#define LISTAPROMOCIONES_H
#include "Promocion.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

class ListaPromociones {
private:
    Promocion* cabeza;

public:
    ListaPromociones() : cabeza(nullptr) {}

    void insertar(std::string cod, std::string nom, std::string vig, std::string dias) {
        Promocion* nuevo = new Promocion(cod, nom, vig, dias);
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

    Promocion* buscar(std::string cod) {
        if (cabeza == nullptr) return nullptr;
        Promocion* aux = cabeza;
        do {
            if (aux->codigo == cod) return aux;
            aux = aux->siguiente;
        } while (aux != cabeza);
        return nullptr;
    }

    void generarReporteGraphviz() {
        std::ofstream file("../reportes/Lista_Promociones.dot");
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
                     << aux->nombre << "\\n(" << aux->codigo << ")\", style=filled, fillcolor=lightblue];\n";

                file << "  promo_" << aux->codigo << " -> promo_" << aux->siguiente->codigo << ";\n";

                Beneficio* ben = aux->listaBeneficios;
                int i = 0;
                std::string ultimoNodo = "promo_" + aux->codigo;

                while (ben != nullptr) {
                    std::string nodeBen = "ben_" + aux->codigo + "_" + std::to_string(i);
                    file << "  " << nodeBen << " [label=\"" << ben->tipo << "\\n" << ben->descripcion
                         << "\\nValor: " << ben->valor << "\", style=filled, fillcolor=lightyellow];\n";

                    file << "  " << ultimoNodo << " -> " << nodeBen << ";\n";
                    if (ultimoNodo != "promo_" + aux->codigo) {
                        file << "  " << nodeBen << " -> " << ultimoNodo << ";\n";
                    }

                    ultimoNodo = nodeBen;
                    ben = ben->siguiente;
                    i++;
                }

                aux = aux->siguiente;
            } while (aux != cabeza);
        }

        file << "}\n";
        file.close();

        system("dot -Tpng ../reportes/Lista_Promociones.dot -o ../reportes/Lista_Promociones.png");
    }
};

#endif // LISTAPROMOCIONES_H