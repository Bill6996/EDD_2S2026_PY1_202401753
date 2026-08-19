#ifndef BENEFICIO_H
#define BENEFICIO_H
#include <string>

struct Beneficio {
    std::string tipo;        // Descuento, Combo, 2x1
    std::string descripcion;
    std::string valor;       // 5%, 10%, NA

    Beneficio* siguiente;
    Beneficio* anterior;

    Beneficio(std::string t, std::string d, std::string v)
        : tipo(t), descripcion(d), valor(v), siguiente(nullptr), anterior(nullptr) {}
};

#endif // BENEFICIO_H