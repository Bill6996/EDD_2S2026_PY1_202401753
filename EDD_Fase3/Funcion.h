#ifndef FUNCION_H
#define FUNCION_H
#include <string>

struct Funcion {
    std::string codigo_funcion;
    std::string codigo_pelicula;
    std::string horario;
    std::string sala;
    std::string fecha;          // Fase 2: "yyyy-MM-dd"
    std::string codigo_sede;    // NUEVO Fase 3: código de la sede (vértice del grafo) donde se proyecta
    int filas;
    int columnas;
    std::string archivo_asientos;

    int altura;
    Funcion* izquierda;
    Funcion* derecha;

    Funcion(std::string codFun, std::string codPel, std::string hor,
            std::string sal, int f, int c, std::string arch = "",
            std::string fec = "", std::string sed = "")
        : codigo_funcion(codFun), codigo_pelicula(codPel), horario(hor),
        sala(sal), fecha(fec), codigo_sede(sed), filas(f), columnas(c), archivo_asientos(arch),
        altura(1), izquierda(nullptr), derecha(nullptr) {}
};

#endif // FUNCION_H