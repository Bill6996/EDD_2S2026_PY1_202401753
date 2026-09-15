#ifndef NODOMATRIZ_H
#define NODOMATRIZ_H
#include <string>


// esta es la matriz dispersa, ya que almacena las coordenada de la fila y columna
//y el estado para ver si esta libre yo ocupado
//4 punteros de enlace ortogonal
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