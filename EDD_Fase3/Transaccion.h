#ifndef TRANSACCION_H
#define TRANSACCION_H

#include <string>
#include <QString>

// ============================================================
// TRANSACCIÓN DE COMPRA
// Es el "pago de una reserva". Contiene TODA la información necesaria para
// reconstruir una Reserva en la Tabla Hash usando únicamente la blockchain.
// ============================================================
struct Transaccion {
    std::string codigo_transaccion;   // "T001"
    std::string codigo_reserva;       // "R1001"  (clave de la Tabla Hash)
    std::string id_cliente;           // "U001"   (clave del Árbol B)
    std::string codigo_funcion;       // "F001"   (clave del AVL)
    int fila;
    int columna;
    double monto;
    std::string metodo_pago;          // "Tarjeta" | "Efectivo"
    std::string fecha_pago;           // formato dd-MM-yyyy
    std::string estado;               // "PAGADA" (por defecto) | "CANCELADA"

    Transaccion() : fila(0), columna(0), monto(0.0), estado("PAGADA") {}

    // Texto canónico de la transacción: es lo que se hashea para formar la HOJA
    // del árbol de Merkle. Cualquier cambio en un campo cambia este texto
    // (y por lo tanto el hash), que es lo que permite detectar manipulaciones.
    std::string serializar() const {
        return codigo_transaccion + "|" + codigo_reserva + "|" + id_cliente + "|" +
               codigo_funcion + "|" + std::to_string(fila) + "|" + std::to_string(columna) + "|" +
               QString::number(monto, 'g', 17).toStdString() + "|" +
               metodo_pago + "|" + fecha_pago +
               // El estado PAGADA es el valor por defecto y no cambia el texto; CANCELADA lo extiende.
               // Así, alterar el estado en un bloque cambia el hash de la hoja (se detecta la manipulación).
               (estado == "PAGADA" ? std::string("") : "|" + estado);
    }
};

#endif // TRANSACCION_H
