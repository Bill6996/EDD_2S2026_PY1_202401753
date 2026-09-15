#ifndef FUNCION_H
#define FUNCION_H
#include <string>

struct Funcion {
    std::string codigo_funcion;      // clave del AVL (ej: "F001")
    std::string codigo_pelicula;     // referencia al BST
    std::string horario;             // ej: "17:00"
    std::string sala;                // ej: "Sala 2"
    int filas;
    int columnas;
    std::string archivo_asientos;    // ej: "F001_funcion.json"

    // Para el AVL
    int altura;
    Funcion* izquierda;
    Funcion* derecha;

    Funcion(std::string codFun, std::string codPel, std::string hor,
            std::string sal, int f, int c, std::string arch = "")
        : codigo_funcion(codFun), codigo_pelicula(codPel), horario(hor),
        sala(sal), filas(f), columnas(c), archivo_asientos(arch),
        altura(1), izquierda(nullptr), derecha(nullptr) {}
};

#endif // FUNCION_H