#ifndef LISTASOLICITUDES_H
#define LISTASOLICITUDES_H

#include "Solicitud.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

class ListaSolicitudes {
private:
    Solicitud* cabeza;
    int contadorGlobal;

public:
    ListaSolicitudes() : cabeza(nullptr), contadorGlobal(1) {}

    void insertar(std::string cliente, std::string telefono, std::string tipo,
                  std::string descripcion, std::string fecha) {
        Solicitud* nuevo = new Solicitud(contadorGlobal++, cliente, telefono, tipo, descripcion, fecha);

        if (cabeza == nullptr) {
            cabeza = nuevo;
            cabeza->siguiente = cabeza;
            cabeza->anterior = cabeza;
        } else {
            Solicitud* ultimo = cabeza->anterior;
            ultimo->siguiente = nuevo;
            nuevo->anterior = ultimo;
            nuevo->siguiente = cabeza;
            cabeza->anterior = nuevo;
        }
    }

    void generarReporteGraphviz() {
        std::ofstream file("../reportes/Lista_Solicitudes.dot");
        file << "digraph ListaSolicitudes {\n";
        file << "  rankdir=LR;\n";
        file << "  node [shape=circle, fontname=\"Arial\"];\n";

        if (cabeza == nullptr) {
            file << "  empty [label=\"Sin Solicitudes Pendientes\"];\n";
        } else {
            Solicitud* aux = cabeza;
            do {
                file << "  node_" << aux->numero << " [label=\""
                     << "No: " << aux->numero << "\\n"
                     << "Cliente: " << aux->cliente << "\\n"
                     << "Tipo: " << aux->tipo << "\\n"
                     << "Estado: " << aux->estado << "\"];\n";

                file << "  node_" << aux->numero << " -> node_" << aux->siguiente->numero << ";\n";
                file << "  node_" << aux->siguiente->numero << " -> node_" << aux->numero << ";\n";

                aux = aux->siguiente;
            } while (aux != cabeza);
        }

        file << "}\n";
        file.close();

        system("dot -Tpng ../reportes/Lista_Solicitudes.dot -o ../reportes/Lista_Solicitudes.png");
    }

    Solicitud* getPrimera() {
        return cabeza;
    }

    bool rechazarPrimera() {
        if (cabeza == nullptr) return false;
        if (cabeza->siguiente == cabeza) {
            delete cabeza;
            cabeza = nullptr;
        } else {
            Solicitud* ultimo = cabeza->anterior;
            Solicitud* temp = cabeza;
            cabeza = cabeza->siguiente;
            ultimo->siguiente = cabeza;
            cabeza->anterior = ultimo;
            delete temp;
        }
        return true;
    }

    int getTotalPendientes() {
        if (cabeza == nullptr) return 0;
        int count = 0;
        Solicitud* aux = cabeza;
        do {
            count++;
            aux = aux->siguiente;
        } while (aux != cabeza);
        return count;
    }
};

#endif // LISTASOLICITUDES_H