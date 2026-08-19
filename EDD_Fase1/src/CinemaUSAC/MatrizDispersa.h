#ifndef MATRIZDISPERSA_H
#define MATRIZDISPERSA_H
#include "NodoMatriz.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

class MatrizDispersa {
private:
    NodoMatriz* raiz;
    int maxFilas;
    int maxColumnas;

    NodoMatriz* buscarFila(int f) {
        NodoMatriz* aux = raiz;
        while (aux != nullptr) {
            if (aux->fila == f) return aux;
            aux = aux->abajo;
        }
        return nullptr;
    }

    NodoMatriz* buscarColumna(int c) {
        NodoMatriz* aux = raiz;
        while (aux != nullptr) {
            if (aux->columna == c) return aux;
            aux = aux->derecha;
        }
        return nullptr;
    }

public:
    MatrizDispersa(int filas = 0, int columnas = 0) : maxFilas(filas), maxColumnas(columnas) {
        raiz = new NodoMatriz(-1, -1);

        NodoMatriz* auxFila = raiz;
        for (int i = 1; i <= filas; i++) {
            NodoMatriz* nuevaFila = new NodoMatriz(i, -1);
            auxFila->abajo = nuevaFila;
            nuevaFila->arriba = auxFila;
            auxFila = nuevaFila;
        }

        NodoMatriz* auxCol = raiz;
        for (int j = 1; j <= columnas; j++) {
            NodoMatriz* nuevaCol = new NodoMatriz(-1, j);
            auxCol->derecha = nuevaCol;
            nuevaCol->izquierda = auxCol;
            auxCol = nuevaCol;
        }
    }

    int getFilas() const { return maxFilas; }
    int getColumnas() const { return maxColumnas; }

    NodoMatriz* buscarAsiento(int f, int c) {
        NodoMatriz* cabFila = buscarFila(f);
        if (!cabFila) return nullptr;
        NodoMatriz* actual = cabFila->derecha;
        while (actual != nullptr) {
            if (actual->columna == c) return actual;
            actual = actual->derecha;
        }
        return nullptr;
    }

    bool reservarAsiento(int f, int c, std::string cliente) {
        NodoMatriz* cabFila = buscarFila(f);
        NodoMatriz* cabCol = buscarColumna(c);

        if (!cabFila || !cabCol) return false;

        NodoMatriz* actual = cabFila->derecha;
        while (actual != nullptr) {
            if (actual->columna == c) {
                if (actual->ocupado) return false;
                actual->cliente = cliente;
                actual->ocupado = true;
                return true;
            }
            actual = actual->derecha;
        }

        NodoMatriz* nuevo = new NodoMatriz(f, c, cliente, true);

        NodoMatriz* auxF = cabFila;
        while (auxF->derecha != nullptr && auxF->derecha->columna < c) {
            auxF = auxF->derecha;
        }
        nuevo->derecha = auxF->derecha;
        if (auxF->derecha) auxF->derecha->izquierda = nuevo;
        auxF->derecha = nuevo;
        nuevo->izquierda = auxF;

        NodoMatriz* auxC = cabCol;
        while (auxC->abajo != nullptr && auxC->abajo->fila < f) {
            auxC = auxC->abajo;
        }
        nuevo->abajo = auxC->abajo;
        if (auxC->abajo) auxC->abajo->arriba = nuevo;
        auxC->abajo = nuevo;
        nuevo->arriba = auxC;

        return true;
    }

    bool cancelarReserva(int f, int c, std::string cliente) {
        NodoMatriz* asiento = buscarAsiento(f, c);
        if (asiento && asiento->ocupado && (cliente.empty() || asiento->cliente == cliente)) {
            asiento->ocupado = false;
            asiento->cliente = "";
            return true;
        }
        return false;
    }

    void generarReporteGraphviz() {
        std::ofstream file("../reportes/Matriz_Asientos.dot");
        file << "digraph MatrizDispersa {\n";
        file << "  node [fontname=\"Arial\"];\n";
        file << "  graph [splines=ortho];\n";

        // Título dinámico
        std::string titulo = "Función: " + (pelicula.empty() ? "Sin asignar" : pelicula) +
                             " - " + (horario.empty() ? "00:00" : horario) +
                             " - " + (sala.empty() ? "Sala 1" : sala);
        file << "  labelloc=\"t\";\n";
        file << "  label=\"" << titulo << "\";\n";

        file << "  root [label=\"Funcion\", group=0, shape=box, style=filled, fillcolor=orange];\n";

        for (int j = 1; j <= maxColumnas; j++) {
            file << "  col_" << j << " [label=\"C" << j << "\", group=" << j << ", shape=box, style=filled, fillcolor=yellow];\n";
        }

        int ocupados = 0;
        for (int i = 1; i <= maxFilas; i++) {
            file << "  row_" << i << " [label=\"F" << i << "\", group=0, shape=box, style=filled, fillcolor=yellow];\n";
            NodoMatriz* fHead = buscarFila(i);
            NodoMatriz* curr = (fHead) ? fHead->derecha : nullptr;

            while (curr != nullptr) {
                if (curr->ocupado) {
                    ocupados++;
                    file << "  node_" << i << "_" << curr->columna
                         << " [label=\"" << curr->cliente << "\", group=" << curr->columna
                         << ", shape=circle, style=filled, fillcolor=lightpink];\n";
                }
                curr = curr->derecha;
            }
        }

        file << "  { rank=same; root; ";
        for (int j = 1; j <= maxColumnas; j++) file << "col_" << j << "; ";
        file << "}\n";

        for (int i = 1; i <= maxFilas; i++) {
            file << "  { rank=same; row_" << i << "; ";
            NodoMatriz* curr = buscarFila(i)->derecha;
            while (curr != nullptr) {
                file << "node_" << i << "_" << curr->columna << "; ";
                curr = curr->derecha;
            }
            file << "}\n";
        }

        if (maxColumnas > 0) file << "  root -> col_1;\n";
        if (maxFilas > 0) file << "  root -> row_1;\n";

        for (int j = 1; j < maxColumnas; j++) file << "  col_" << j << " -> col_" << (j + 1) << ";\n";
        for (int i = 1; i < maxFilas; i++) file << "  row_" << i << " -> row_" << (i + 1) << ";\n";

        int total = maxFilas * maxColumnas;
        int libres = total - ocupados;
        file << "  info [label=\"Estadísticas:\\nTotal Asientos: " << total << "\\nReservados: " << ocupados << "\\nLibres: " << libres << "\", shape=note, style=filled, fillcolor=lightgray];\n";

        file << "}\n";
        file.close();

        system("dot -Tpng ../reportes/Matriz_Asientos.dot -o ../reportes/Matriz_Asientos.png");
    }

    // Agregar en la sección private: de MatrizDispersa
    std::string pelicula;
    std::string horario;
    std::string sala;

    // Agregar en la sección public: de MatrizDispersa
    void setDatosFuncion(std::string p, std::string h, std::string s) {
        pelicula = p;
        horario = h;
        sala = s;
    }

    std::string getInfoFuncion() const {
        if (pelicula.empty()) return "Sin función asignada";
        return "Película: " + pelicula + " | Horario: " + horario + " | Sala: " + sala;
    }

    // Método para eliminar/desconectar un asiento específico de la matriz dispersa
    bool eliminarAsiento(int f, int c) {
        NodoMatriz* cabFila = buscarFila(f);
        if (!cabFila) return false;

        NodoMatriz* actual = cabFila->derecha;
        while (actual != nullptr) {
            if (actual->columna == c) {
                // Desconectar punteros horizontales
                if (actual->izquierda) actual->izquierda->derecha = actual->derecha;
                if (actual->derecha) actual->derecha->izquierda = actual->izquierda;

                // Desconectar punteros verticales
                if (actual->arriba) actual->arriba->abajo = actual->abajo;
                if (actual->abajo) actual->abajo->arriba = actual->arriba;

                delete actual;
                return true;
            }
            actual = actual->derecha;
        }
        return false;
    }
};

#endif // MATRIZDISPERSA_H