#ifndef TABLAHASHRESERVAS_H
#define TABLAHASHRESERVAS_H

#include "Reserva.h"
#include <iostream>
#include <fstream>
#include <string>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QString>

// Nodo de la lista enlazada (para resolver colisiones por encadenamiento)
struct NodoHash {
    Reserva* reserva;
    NodoHash* siguiente;

    NodoHash(Reserva* r) : reserva(r), siguiente(nullptr) {}
};

class TablaHashReservas {
private:
    NodoHash** tabla;          // Array de punteros a listas
    int capacidad;             // Tamaño de la tabla (número primo recomendado)
    int cantidad;              // Cuántas reservas hay actualmente

    // Función hash simple: suma de caracteres del código % capacidad
    int hash(const std::string& clave) const {
        unsigned long hashVal = 0;
        for (char c : clave) {
            hashVal = hashVal * 31 + static_cast<unsigned char>(c);
        }
        return static_cast<int>(hashVal % capacidad);
    }

public:
    // Constructor: tamaño por defecto 101 (primo)
    TablaHashReservas(int cap = 101) : capacidad(cap), cantidad(0) {
        tabla = new NodoHash*[capacidad];
        for (int i = 0; i < capacidad; i++) {
            tabla[i] = nullptr;
        }
    }

    // Destructor: libera toda la memoria
    ~TablaHashReservas() {
        for (int i = 0; i < capacidad; i++) {
            NodoHash* actual = tabla[i];
            while (actual != nullptr) {
                NodoHash* temp = actual;
                actual = actual->siguiente;
                delete temp->reserva;
                delete temp;
            }
        }
        delete[] tabla;
    }

    // Insertar una reserva
    bool insertar(Reserva* nueva) {
        if (nueva == nullptr) return false;

        // Verificar que no exista ya ese código
        if (buscar(nueva->codigo_reserva) != nullptr) {
            return false; // Ya existe
        }

        int indice = hash(nueva->codigo_reserva);
        NodoHash* nodo = new NodoHash(nueva);
        nodo->siguiente = tabla[indice];  // Insertar al inicio de la lista
        tabla[indice] = nodo;
        cantidad++;
        return true;
    }

    // Buscar por codigo_reserva
    Reserva* buscar(const std::string& codigo) const {
        int indice = hash(codigo);
        NodoHash* actual = tabla[indice];
        while (actual != nullptr) {
            if (actual->reserva->codigo_reserva == codigo) {
                return actual->reserva;
            }
            actual = actual->siguiente;
        }
        return nullptr;
    }

    // Eliminar por codigo_reserva
    bool eliminar(const std::string& codigo) {
        int indice = hash(codigo);
        NodoHash* actual = tabla[indice];
        NodoHash* anterior = nullptr;

        while (actual != nullptr) {
            if (actual->reserva->codigo_reserva == codigo) {
                if (anterior == nullptr) {
                    tabla[indice] = actual->siguiente;
                } else {
                    anterior->siguiente = actual->siguiente;
                }
                delete actual->reserva;
                delete actual;
                cantidad--;
                return true;
            }
            anterior = actual;
            actual = actual->siguiente;
        }
        return false;
    }

    // Getters útiles
    int getCantidad() const { return cantidad; }
    int getCapacidad() const { return capacidad; }

    // Contar buckets ocupados y colisiones (para el reporte)
    void obtenerEstadisticas(int& bucketsOcupados, int& colisiones) const {
        bucketsOcupados = 0;
        colisiones = 0;
        for (int i = 0; i < capacidad; i++) {
            if (tabla[i] != nullptr) {
                bucketsOcupados++;
                NodoHash* actual = tabla[i]->siguiente;
                while (actual != nullptr) {
                    colisiones++;
                    actual = actual->siguiente;
                }
            }
        }
    }

    // Generar reporte Graphviz
    void generarReporteGraphviz() {
        QString appPath = QCoreApplication::applicationDirPath();
        QDir().mkpath(appPath + "/reportes");

        QString dotFile = appPath + "/reportes/TablaHash_Reservas.dot";
        QString pngFile = appPath + "/reportes/TablaHash_Reservas.png";

        std::ofstream file(dotFile.toStdString());
        file << "digraph TablaHash {\n";
        file << "  rankdir=LR;\n";
        file << "  node [fontname=\"Arial\"];\n";
        file << "  graph [label=\"Tabla Hash de Reservas\\n";

        int ocupados = 0, colisiones = 0;
        obtenerEstadisticas(ocupados, colisiones);
        file << "Tamaño: " << capacidad
             << " | Reservas: " << cantidad
             << " | Buckets ocupados: " << ocupados
             << " | Colisiones: " << colisiones << "\", labelloc=t, fontsize=14];\n\n";

        for (int i = 0; i < capacidad; i++) {
            std::string color = (tabla[i] == nullptr) ? "lightgray" : "lightgreen";
            file << "  bucket_" << i << " [label=\"Bucket " << i
                 << "\", shape=box, style=filled, fillcolor=" << color << "];\n";

            if (tabla[i] != nullptr) {
                NodoHash* actual = tabla[i];
                int contador = 0;
                std::string anterior = "bucket_" + std::to_string(i);

                while (actual != nullptr) {
                    std::string nodoId = "nodo_" + std::to_string(i) + "_" + std::to_string(contador);
                    file << "  " << nodoId << " [label=\""
                         << actual->reserva->codigo_reserva << "\\n"
                         << actual->reserva->id_cliente << " → "
                         << actual->reserva->codigo_funcion
                         << "\", shape=box, style=filled, fillcolor=lightyellow];\n";
                    file << "  " << anterior << " -> " << nodoId << ";\n";
                    anterior = nodoId;
                    actual = actual->siguiente;
                    contador++;
                }
            }
        }

        file << "}\n";
        file.close();

        QProcess process;
        process.start("dot", QStringList() << "-Tpng" << dotFile << "-o" << pngFile);
        process.waitForFinished(5000);
    }
};

#endif // TABLAHASHRESERVAS_H