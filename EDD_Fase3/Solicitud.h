#ifndef SOLICITUD_H
#define SOLICITUD_H
#include <string>

struct Solicitud {
    int numero;
    std::string cliente;
    std::string telefono;
    std::string tipo;         // cumpleaños, aniversario, etc.
    std::string descripcion;
    std::string fecha;
    std::string estado;      // Pendiente, En proceso, Atendida, Rechazada

    Solicitud* siguiente;
    Solicitud* anterior;

    Solicitud(int num, std::string cli, std::string tel, std::string tp,
              std::string desc, std::string fch, std::string est = "Pendiente")
        : numero(num), cliente(cli), telefono(tel), tipo(tp),
        descripcion(desc), fecha(fch), estado(est),
        siguiente(nullptr), anterior(nullptr) {}
};

#endif // SOLICITUD_H