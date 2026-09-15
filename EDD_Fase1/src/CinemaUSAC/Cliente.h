#ifndef CLIENTE_H
#define CLIENTE_H
#include <string>

// Lista simple de códigos de reserva que pertenece a este cliente
struct NodoCodigoReserva {
    std::string codigo_reserva;
    NodoCodigoReserva* siguiente;

    NodoCodigoReserva(std::string cod)
        : codigo_reserva(cod), siguiente(nullptr) {}
};

struct Cliente {
    std::string id;           // clave del Árbol B (ej: "U001")
    std::string nombre;
    std::string correo;
    std::string telefono;
    std::string password;
    std::string tipo;         // "cliente"

    // Solo guardamos las referencias (códigos), no la reserva completa
    NodoCodigoReserva* listaReservas;

    Cliente(std::string i, std::string nom, std::string corr,
            std::string tel, std::string pass, std::string tip = "cliente")
        : id(i), nombre(nom), correo(corr), telefono(tel),
        password(pass), tipo(tip), listaReservas(nullptr) {}

    // Agregar un código de reserva a la lista del cliente
    void agregarCodigoReserva(std::string cod) {
        NodoCodigoReserva* nuevo = new NodoCodigoReserva(cod);
        nuevo->siguiente = listaReservas;
        listaReservas = nuevo;
    }

    // Eliminar un código de reserva de la lista
    bool eliminarCodigoReserva(std::string cod) {
        if (listaReservas == nullptr) return false;

        if (listaReservas->codigo_reserva == cod) {
            NodoCodigoReserva* temp = listaReservas;
            listaReservas = listaReservas->siguiente;
            delete temp;
            return true;
        }

        NodoCodigoReserva* actual = listaReservas;
        while (actual->siguiente != nullptr) {
            if (actual->siguiente->codigo_reserva == cod) {
                NodoCodigoReserva* temp = actual->siguiente;
                actual->siguiente = temp->siguiente;
                delete temp;
                return true;
            }
            actual = actual->siguiente;
        }
        return false;
    }
};

#endif // CLIENTE_H