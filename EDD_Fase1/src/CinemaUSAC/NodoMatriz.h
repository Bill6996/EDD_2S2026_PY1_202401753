#ifndef NODOMATRIZ_H
#define NODOMATRIZ_H
#include <string>

struct NodoMatriz {
    int fila;
    int columna;
    std::string cliente;
    bool ocupado;

    NodoMatriz* arriba;
    NodoMatriz* abajo;
    NodoMatriz* izquierda;
    NodoMatriz* derecha;

    NodoMatriz(int f, int c, std::string cli = "", bool oc = false)
        : fila(f), columna(c), cliente(cli), ocupado(oc),
        arriba(nullptr), abajo(nullptr), izquierda(nullptr), derecha(nullptr) {}
};

#endif // NODOMATRIZ_H