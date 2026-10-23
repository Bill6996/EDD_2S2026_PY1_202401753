#ifndef SEDE_H
#define SEDE_H
#include <string>

// Datos de una sede (sucursal) de CinemaUSAC.
// Cada sede es un VÉRTICE del grafo no dirigido ponderado (ver GrafoSedes.h).
struct Sede {
    std::string codigo;        // "S001"
    std::string nombre;        // "CinemaUSAC Zona 10"
    std::string direccion;     // "Zona 10, Ciudad de Guatemala"
    int cantidad_salas;
    std::string telefono;

    Sede() : cantidad_salas(0) {}
    Sede(std::string cod, std::string nom, std::string dir, int salas, std::string tel)
        : codigo(cod), nombre(nom), direccion(dir), cantidad_salas(salas), telefono(tel) {}
};

#endif // SEDE_H
