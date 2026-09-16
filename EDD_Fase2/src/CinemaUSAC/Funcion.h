#ifndef FUNCION_H
#define FUNCION_H
#include <string>

struct Funcion {
    std::string codigo_funcion;
    std::string codigo_pelicula;
    std::string horario;
    std::string sala;
    std::string fecha;          // NUEVO: "yyyy-MM-dd"
    int filas;
    int columnas;
    std::string archivo_asientos;

    int altura;
    Funcion* izquierda;
    Funcion* derecha;

    Funcion(std::string codFun, std::string codPel, std::string hor,
            std::string sal, int f, int c, std::string arch = "",
            std::string fec = "")
        : codigo_funcion(codFun), codigo_pelicula(codPel), horario(hor),
        sala(sal), fecha(fec), filas(f), columnas(c), archivo_asientos(arch),
        altura(1), izquierda(nullptr), derecha(nullptr) {}
};

#endif // FUNCION_H