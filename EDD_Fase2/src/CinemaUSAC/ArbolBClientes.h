#ifndef ARBOLBCLIENTES_H
#define ARBOLBCLIENTES_H
#include "Cliente.h"
#include <iostream>
#include <fstream>
#include <string>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>

// Orden 4 → máximo 3 claves y 4 hijos por nodo
const int MAX_CLAVES = 3;
const int MAX_HIJOS  = 4;

struct NodoB {
    Cliente* claves[MAX_CLAVES];   // Hasta 3 clientes
    NodoB*   hijos[MAX_HIJOS];     // Hasta 4 hijos
    int      numClaves;
    bool     esHoja;

    NodoB(bool hoja = true) : numClaves(0), esHoja(hoja) {
        for (int i = 0; i < MAX_CLAVES; i++) claves[i] = nullptr;
        for (int i = 0; i < MAX_HIJOS;  i++) hijos[i]  = nullptr;
    }
};

class ArbolBClientes {
private:
    NodoB* raiz;

    // ---------- Búsqueda ----------
    Cliente* buscarRec(NodoB* nodo, const std::string& id) const {
        if (nodo == nullptr) return nullptr;
        int i = 0;
        while (i < nodo->numClaves && id > nodo->claves[i]->id)
            i++;
        if (i < nodo->numClaves && id == nodo->claves[i]->id)
            return nodo->claves[i];
        if (nodo->esHoja) return nullptr;
        return buscarRec(nodo->hijos[i], id);
    }

    // Buscar por correo (recorrido completo)
    Cliente* buscarPorCorreoRec(NodoB* nodo, const std::string& correo) const {
        if (nodo == nullptr) return nullptr;
        for (int i = 0; i < nodo->numClaves; i++) {
            if (nodo->claves[i]->correo == correo)
                return nodo->claves[i];
            if (!nodo->esHoja) {
                Cliente* encontrado = buscarPorCorreoRec(nodo->hijos[i], correo);
                if (encontrado) return encontrado;
            }
        }
        if (!nodo->esHoja)
            return buscarPorCorreoRec(nodo->hijos[nodo->numClaves], correo);
        return nullptr;
    }

    // ---------- División de nodo lleno ----------
    void dividirHijo(NodoB* padre, int indice) {
        NodoB* lleno = padre->hijos[indice];
        NodoB* nuevo = new NodoB(lleno->esHoja);

        nuevo->numClaves = 1;
        nuevo->claves[0] = lleno->claves[2];
        if (!lleno->esHoja) {
            nuevo->hijos[0] = lleno->hijos[2];
            nuevo->hijos[1] = lleno->hijos[3];
            lleno->hijos[2] = lleno->hijos[3] = nullptr;
        }
        lleno->numClaves = 1;

        for (int j = padre->numClaves; j > indice; j--) {
            padre->hijos[j + 1] = padre->hijos[j];
        }
        padre->hijos[indice + 1] = nuevo;

        for (int j = padre->numClaves - 1; j >= indice; j--) {
            padre->claves[j + 1] = padre->claves[j];
        }
        padre->claves[indice] = lleno->claves[1];
        lleno->claves[1] = lleno->claves[2] = nullptr;
        padre->numClaves++;
    }

    // ---------- Inserción en nodo no lleno ----------
    void insertarNoLleno(NodoB* nodo, Cliente* cliente) {
        int i = nodo->numClaves - 1;
        if (nodo->esHoja) {
            while (i >= 0 && cliente->id < nodo->claves[i]->id) {
                nodo->claves[i + 1] = nodo->claves[i];
                i--;
            }
            nodo->claves[i + 1] = cliente;
            nodo->numClaves++;
        } else {
            while (i >= 0 && cliente->id < nodo->claves[i]->id)
                i--;
            i++;
            if (nodo->hijos[i]->numClaves == MAX_CLAVES) {
                dividirHijo(nodo, i);
                if (cliente->id > nodo->claves[i]->id)
                    i++;
            }
            insertarNoLleno(nodo->hijos[i], cliente);
        }
    }

    // ---------- Recorrido inorden para la tabla (con N° Reservas) ----------
    void inordenTabla(NodoB* nodo, QTableWidget* tabla, int& fila) {
        if (nodo == nullptr) return;
        for (int i = 0; i < nodo->numClaves; i++) {
            if (!nodo->esHoja)
                inordenTabla(nodo->hijos[i], tabla, fila);

            // Contar reservas del cliente
            int cantReservas = 0;
            NodoCodigoReserva* n = nodo->claves[i]->listaReservas;
            while (n != nullptr) {
                cantReservas++;
                n = n->siguiente;
            }

            tabla->insertRow(fila);
            tabla->setItem(fila, 0, new QTableWidgetItem(QString::fromStdString(nodo->claves[i]->id)));
            tabla->setItem(fila, 1, new QTableWidgetItem(QString::fromStdString(nodo->claves[i]->nombre)));
            tabla->setItem(fila, 2, new QTableWidgetItem(QString::fromStdString(nodo->claves[i]->correo)));
            tabla->setItem(fila, 3, new QTableWidgetItem(QString::fromStdString(nodo->claves[i]->telefono)));
            tabla->setItem(fila, 4, new QTableWidgetItem(QString::fromStdString(nodo->claves[i]->tipo)));
            tabla->setItem(fila, 5, new QTableWidgetItem(QString::number(cantReservas)));
            tabla->setItem(fila, 6, new QTableWidgetItem(QString::fromStdString(nodo->claves[i]->password)));
            fila++;
        }
        if (!nodo->esHoja)
            inordenTabla(nodo->hijos[nodo->numClaves], tabla, fila);
    }

    // ---------- Graphviz ----------
    void generarGraphvizRec(NodoB* nodo, std::ofstream& file, int& contador,
                            std::string& idNodo, bool esRaiz) {
        if (nodo == nullptr) return;

        idNodo = "nodo" + std::to_string(contador++);

        // Colores: raíz / internos / hojas
        std::string color;
        if (esRaiz) {
            color = "orange";        // RAÍZ
        } else if (nodo->esHoja) {
            color = "lightgreen";    // HOJAS
        } else {
            color = "lightblue";     // NODOS INTERNOS
        }

        file << "  " << idNodo
             << " [shape=record, style=filled, fillcolor=" << color
             << ", label=\"";
        for (int i = 0; i < nodo->numClaves; i++) {
            if (i > 0) file << " | ";
            file << "<f" << i << "> " << nodo->claves[i]->id
                 << "\\n" << nodo->claves[i]->nombre;
        }
        file << "\"];\n";

        if (!nodo->esHoja) {
            for (int i = 0; i <= nodo->numClaves; i++) {
                std::string idHijo;
                generarGraphvizRec(nodo->hijos[i], file, contador, idHijo, false);
                file << "  " << idNodo << ":f" << std::min(i, nodo->numClaves - 1)
                     << " -> " << idHijo << ";\n";
            }
        }
    }

    // ---------- Liberar memoria ----------
    void destruir(NodoB* nodo) {
        if (nodo == nullptr) return;
        if (!nodo->esHoja) {
            for (int i = 0; i <= nodo->numClaves; i++)
                destruir(nodo->hijos[i]);
        }
        for (int i = 0; i < nodo->numClaves; i++)
            delete nodo->claves[i];
        delete nodo;
    }

    // Quitar una clave de un nodo hoja
    void quitarDeHoja(NodoB* nodo, int indice) {
        delete nodo->claves[indice];
        for (int i = indice; i < nodo->numClaves - 1; i++) {
            nodo->claves[i] = nodo->claves[i + 1];
        }
        nodo->claves[nodo->numClaves - 1] = nullptr;
        nodo->numClaves--;
    }

    // Eliminar desde hoja
    bool eliminarDeHoja(NodoB* nodo, const std::string& id) {
        if (nodo == nullptr) return false;
        int i = 0;
        while (i < nodo->numClaves && id > nodo->claves[i]->id)
            i++;
        if (nodo->esHoja) {
            if (i < nodo->numClaves && nodo->claves[i]->id == id) {
                NodoCodigoReserva* n = nodo->claves[i]->listaReservas;
                while (n != nullptr) {
                    NodoCodigoReserva* temp = n;
                    n = n->siguiente;
                    delete temp;
                }
                quitarDeHoja(nodo, i);
                return true;
            }
            return false;
        }
        return eliminarDeHoja(nodo->hijos[i], id);
    }

public:
    ArbolBClientes() : raiz(nullptr) {}
    ~ArbolBClientes() {
        destruir(raiz);
    }

    bool insertar(Cliente* cliente) {
        if (cliente == nullptr) return false;
        if (buscar(cliente->id) != nullptr) return false;
        if (buscarPorCorreo(cliente->correo) != nullptr) return false;
        if (raiz == nullptr) {
            raiz = new NodoB(true);
            raiz->claves[0] = cliente;
            raiz->numClaves = 1;
            return true;
        }
        if (raiz->numClaves == MAX_CLAVES) {
            NodoB* nuevaRaiz = new NodoB(false);
            nuevaRaiz->hijos[0] = raiz;
            dividirHijo(nuevaRaiz, 0);
            raiz = nuevaRaiz;
        }
        insertarNoLleno(raiz, cliente);
        return true;
    }

    Cliente* buscar(const std::string& id) const {
        return buscarRec(raiz, id);
    }

    Cliente* buscarPorCorreo(const std::string& correo) const {
        return buscarPorCorreoRec(raiz, correo);
    }

    void cargarEnTabla(QTableWidget* tabla) {
        tabla->setRowCount(0);
        int fila = 0;
        inordenTabla(raiz, tabla, fila);
    }

    void generarReporteGraphviz() {
        QString appPath = QCoreApplication::applicationDirPath();
        QDir().mkpath(appPath + "/reportes");
        QString dotFile = appPath + "/reportes/ArbolB_Clientes.dot";
        QString pngFile = appPath + "/reportes/ArbolB_Clientes.png";
        std::ofstream file(dotFile.toStdString());
        file << "digraph ArbolB {\n";
        file << "  node [fontname=\"Arial\"];\n";
        file << "  rankdir=TB;\n";
        if (raiz == nullptr) {
            file << "  empty [label=\"Sin clientes\", shape=box];\n";
        } else {
            int contador = 0;
                std::string idRaiz;
                generarGraphvizRec(raiz, file, contador, idRaiz, true);  // true = es la raíz

        }
        file << "}\n";
        file.close();
        QProcess process;
        process.start("dot", QStringList() << "-Tpng" << dotFile << "-o" << pngFile);
        process.waitForFinished(5000);
    }

    bool estaVacio() const { return raiz == nullptr; }

    bool eliminar(const std::string& id) {
        if (raiz == nullptr) return false;
        if (buscar(id) == nullptr) return false;
        bool ok = eliminarDeHoja(raiz, id);
        if (ok && raiz->numClaves == 0 && !raiz->esHoja) {
            NodoB* vieja = raiz;
            raiz = raiz->hijos[0];
            delete vieja;
        } else if (ok && raiz->numClaves == 0 && raiz->esHoja) {
            delete raiz;
            raiz = nullptr;
        }
        return ok;
    }
};

#endif // ARBOLBCLIENTES_H