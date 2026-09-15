#ifndef LISTASOLICITUDES_H
#define LISTASOLICITUDES_H

#include "Solicitud.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>


//aqui manejo la lista circular doblemente enlazada +
//aqui es donde agrego solicitudes, navegar en el anillo o actuakizar estados
//y de ultimo con la info genera el graphiz o da la informacion



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

    Solicitud* buscarPorTelefono(std::string tel) {
        if (cabeza == nullptr) return nullptr;
        Solicitud* aux = cabeza;
        do {
            if (aux->telefono == tel) return aux;
            aux = aux->siguiente;
        } while (aux != cabeza);
        return nullptr;
    }

    void generarReporteGraphviz() {
        QString appPath = QCoreApplication::applicationDirPath();
        QDir().mkpath(appPath + "/reportes");

        QString dotFile = appPath + "/reportes/Lista_Solicitudes.dot";
        QString pngFile = appPath + "/reportes/Lista_Solicitudes.png";

        std::ofstream file(dotFile.toStdString());
        file << "digraph ListaSolicitudes {\n";
        file << "  rankdir=LR;\n";
        file << "  node [shape=circle, fontname=\"Arial\", style=filled, fillcolor=lightyellow];\n";

        if (cabeza == nullptr) {
            file << "  empty [label=\"Sin Solicitudes Pendientes\", shape=box];\n";
        } else {
            Solicitud* aux = cabeza;
            do {
                file << "  node_" << aux->numero << " [label=\""
                     << "No: " << aux->numero << "\\n"
                     << "Cliente: " << aux->cliente << "\\n"
                     << "Tipo: " << aux->tipo << "\\n"
                     << "Estado: " << aux->estado << "\"];\n";

                file << "  node_" << aux->numero << " -> node_" << aux->siguiente->numero << " [dir=both];\n";
                aux = aux->siguiente;
            } while (aux != cabeza);
        }

        file << "  counter [label=\"Total Pendientes: " << getTotalPendientes() << "\", shape=note, style=filled, fillcolor=lightblue];\n";
        file << "}\n";
        file.close();

        QProcess process;
        process.start("dot", QStringList() << "-Tpng" << dotFile << "-o" << pngFile);
        process.waitForFinished(3000);
    }

    Solicitud* getPrimera() { return cabeza; }

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