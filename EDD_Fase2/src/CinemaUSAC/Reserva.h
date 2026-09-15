#ifndef RESERVA_H
#define RESERVA_H
#include <string>

struct Reserva {
    std::string codigo_reserva;   // clave de la Tabla Hash (ej: "R001")
    std::string id_cliente;       // referencia al Árbol B
    std::string codigo_funcion;   // referencia al AVL
    int fila;
    int columna;
    std::string fecha_reserva;    // formato: "yyyy-MM-dd"

    Reserva(std::string codRes, std::string idCli, std::string codFun,
            int f, int c, std::string fecha)
        : codigo_reserva(codRes), id_cliente(idCli), codigo_funcion(codFun),
        fila(f), columna(c), fecha_reserva(fecha) {}
};

#endif // RESERVA_H