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

        // Crear cabeceras de filas
        NodoMatriz* auxFila = raiz;
        for (int i = 1; i <= filas; i++) {
            NodoMatriz* nuevaFila = new NodoMatriz(i, -1);
            auxFila->abajo = nuevaFila;
            nuevaFila->arriba = auxFila;
            auxFila = nuevaFila;
        }

        // Crear cabeceras de columnas
        NodoMatriz* auxCol = raiz;
        for (int j = 1; j <= columnas; j++) {
            NodoMatriz* nuevaCol = new NodoMatriz(-1, j);
            auxCol->derecha = nuevaCol;
            nuevaCol->izquierda = auxCol;
            auxCol = nuevaCol;
        }
    }

    bool reservarAsiento(int f, int c, std::string cliente) {
        NodoMatriz* cabFila = buscarFila(f);
        NodoMatriz* cabCol = buscarColumna(c);

        if (!cabFila || !cabCol) return false;

        // Buscar si ya existe el asiento
        NodoMatriz* actual = cabFila->derecha;
        while (actual != nullptr) {
            if (actual->columna == c) {
                if (actual->ocupado) return false; // Asiento ya ocupado
                actual->cliente = cliente;
                actual->ocupado = true;
                return true;
            }
            actual = actual->derecha;
        }

        // Crear nuevo nodo de reserva
        NodoMatriz* nuevo = new NodoMatriz(f, c, cliente, true);

        // Inserción ordenada en Fila
        NodoMatriz* auxF = cabFila;
        while (auxF->derecha != nullptr && auxF->derecha->columna < c) {
            auxF = auxF->derecha;
        }
        nuevo->derecha = auxF->derecha;
        if (auxF->derecha) auxF->derecha->izquierda = nuevo;
        auxF->derecha = nuevo;
        nuevo->izquierda = auxF;

        // Inserción ordenada en Columna
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

    void generarReporteGraphviz() {
        std::ofstream file("../reportes/Matriz_Asientos.dot");
        file << "digraph MatrizDispersa {\n";
        file << "  node [shape=box, fontname=\"Arial\"];\n";
        file << "  graph [splines=ortho];\n";

        // Cabecera raíz
        file << "  root [label=\"Funcion\", group=0, style=filled, fillcolor=orange];\n";

        // Cabeceras de columna
        for (int j = 1; j <= maxColumnas; j++) {
            file << "  col_" << j << " [label=\"C" << j << "\", group=" << j << ", style=filled, fillcolor=yellow];\n";
        }

        // Cabeceras de fila y nodos ocupados
        int ocupados = 0;
        for (int i = 1; i <= maxFilas; i++) {
            file << "  row_" << i << " [label=\"F" << i << "\", group=0, style=filled, fillcolor=yellow];\n";
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

        // Alineación horizontal
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

        // Enlaces raíz a cabeceras
        if (maxColumnas > 0) file << "  root -> col_1;\n";
        if (maxFilas > 0) file << "  root -> row_1;\n";

        for (int j = 1; j < maxColumnas; j++) file << "  col_" << j << " -> col_" << (j + 1) << ";\n";
        for (int i = 1; i < maxFilas; i++) file << "  row_" << i << " -> row_" << (i + 1) << ";\n";

        // Estadísticas al pie
        int total = maxFilas * maxColumnas;
        int libres = total - ocupados;
        file << "  info [label=\"Total: " << total << "\\nLibres: " << libres << "\\nOcupados: " << ocupados << "\", shape=note];\n";

        file << "}\n";
        file.close();

        system("dot -Tpng ../reportes/Matriz_Asientos.dot -o ../reportes/Matriz_Asientos.png");
    }
};

#endif // MATRIZDISPERSA_H