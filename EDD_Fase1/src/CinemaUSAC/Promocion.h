#ifndef PROMOCION_H
#define PROMOCION_H
#include "Beneficio.h"
#include <string>

struct Promocion {
    std::string codigo;
    std::string nombre;
    std::string vigencia;
    std::string diasAplicables;

    Beneficio* listaBeneficios; // Puntero a la lista doble de beneficios
    Promocion* siguiente;       // Puntero circular

    Promocion(std::string c, std::string n, std::string v, std::string d)
        : codigo(c), nombre(n), vigencia(v), diasAplicables(d),
        listaBeneficios(nullptr), siguiente(nullptr) {}

    void agregarBeneficio(std::string t, std::string desc, std::string val) {
        Beneficio* nuevo = new Beneficio(t, desc, val);
        if (listaBeneficios == nullptr) {
            listaBeneficios = nuevo;
        } else {
            Beneficio* aux = listaBeneficios;
            while (aux->siguiente != nullptr) {
                aux = aux->siguiente;
            }
            aux->siguiente = nuevo;
            nuevo->anterior = aux;
        }
    }
};

#endif // PROMOCION_H