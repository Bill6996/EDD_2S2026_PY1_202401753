#ifndef MERKLETREE_H
#define MERKLETREE_H

#include "Transaccion.h"
#include "SHA256.h"
#include "GraphvizUtil.h"
#include <string>
#include <sstream>

// ============================================================
// LISTA ENLAZADA DE TRANSACCIONES
// Se usa para: la cola de transacciones pendientes y el campo DATA de cada bloque.
// ============================================================
struct NodoTransaccion {
    Transaccion datos;
    NodoTransaccion* siguiente;
    NodoTransaccion(const Transaccion& t) : datos(t), siguiente(nullptr) {}
};

class ListaTransacciones {
private:
    NodoTransaccion* primero;
    NodoTransaccion* ultimo;
    int cantidad;

public:
    ListaTransacciones() : primero(nullptr), ultimo(nullptr), cantidad(0) {}
    ~ListaTransacciones() { vaciar(); }
    ListaTransacciones(const ListaTransacciones&) = delete;
    ListaTransacciones& operator=(const ListaTransacciones&) = delete;

    // Encolar al final (FIFO: las transacciones se guardan en orden de llegada)
    void agregar(const Transaccion& t) {
        NodoTransaccion* nuevo = new NodoTransaccion(t);
        if (primero == nullptr) { primero = ultimo = nuevo; }
        else { ultimo->siguiente = nuevo; ultimo = nuevo; }
        cantidad++;
    }

    int getCantidad() const { return cantidad; }
    bool estaVacia() const { return cantidad == 0; }
    NodoTransaccion* getPrimero() const { return primero; }

    // Busca por código de transacción
    Transaccion* buscar(const std::string& codigo) const {
        for (NodoTransaccion* n = primero; n != nullptr; n = n->siguiente)
            if (n->datos.codigo_transaccion == codigo) return &n->datos;
        return nullptr;
    }

    // Elimina una transacción por código de reserva (se usa al cancelar una compra pendiente)
    bool eliminarPorReserva(const std::string& codigoReserva) {
        NodoTransaccion* actual = primero;
        NodoTransaccion* anterior = nullptr;
        while (actual != nullptr) {
            if (actual->datos.codigo_reserva == codigoReserva) {
                if (anterior == nullptr) primero = actual->siguiente;
                else anterior->siguiente = actual->siguiente;
                if (actual == ultimo) ultimo = anterior;
                delete actual;
                cantidad--;
                return true;
            }
            anterior = actual;
            actual = actual->siguiente;
        }
        return false;
    }

    // Mueve TODOS los nodos de "otra" hacia esta lista (otra queda vacía). No copia datos.
    void trasladarDesde(ListaTransacciones& otra) {
        vaciar();
        primero = otra.primero;
        ultimo = otra.ultimo;
        cantidad = otra.cantidad;
        otra.primero = otra.ultimo = nullptr;
        otra.cantidad = 0;
    }

    void vaciar() {
        NodoTransaccion* n = primero;
        while (n != nullptr) { NodoTransaccion* t = n; n = n->siguiente; delete t; }
        primero = ultimo = nullptr;
        cantidad = 0;
    }

    template <typename F>
    void recorrer(F accion) const {
        for (NodoTransaccion* n = primero; n != nullptr; n = n->siguiente) accion(n->datos);
    }
};

// ============================================================
// ÁRBOL DE MERKLE
//
//  * Hojas:     hash = SHA256( transacción serializada )
//  * Internos:  hash = SHA256( hashIzquierdo + hashDerecho )
//  * Raíz:      resume TODAS las transacciones; si se altera una sola,
//               la raíz cambia por completo.
//  * Si un nivel tiene cantidad IMPAR de nodos, el último se duplica
//    (convención de Bitcoin) para poder formar la última pareja.
//  * Con una sola transacción el árbol tiene un único nodo y la raíz es su hash.
// ============================================================
struct NodoMerkle {
    std::string hash;
    std::string etiqueta;       // para el reporte: código de transacción o "" en nodos internos
    bool duplicado;             // true si es una copia generada para completar una pareja
    NodoMerkle* izquierdo;
    NodoMerkle* derecho;
    NodoMerkle(const std::string& h, const std::string& e = "")
        : hash(h), etiqueta(e), duplicado(false), izquierdo(nullptr), derecho(nullptr) {}
};

class ArbolMerkle {
private:
    NodoMerkle* raiz;

    // Copia un nodo hoja/interno (solo se usa para el nodo duplicado de niveles impares)
    static NodoMerkle* clonar(NodoMerkle* n) {
        if (n == nullptr) return nullptr;
        NodoMerkle* c = new NodoMerkle(n->hash, n->etiqueta);
        c->duplicado = true;
        c->izquierdo = clonar(n->izquierdo);
        c->derecho = clonar(n->derecho);
        return c;
    }

    static void destruir(NodoMerkle* n) {
        if (n == nullptr) return;
        destruir(n->izquierdo);
        destruir(n->derecho);
        delete n;
    }

    static std::string abreviar(const std::string& h) {
        return h.size() > 16 ? h.substr(0, 16) + "..." : h;
    }

    // Escribe el árbol en formato dot (recorrido en preorden)
    static void escribirDot(NodoMerkle* n, std::ostringstream& dot, int& contador, int idPadre) {
        int id = contador++;
        std::string color = "lightyellow";
        std::string etiqueta;
        if (n->izquierdo == nullptr && n->derecho == nullptr) {         // hoja
            color = n->duplicado ? "lightgray" : "lightblue";
            etiqueta = (n->etiqueta.empty() ? "hoja" : n->etiqueta) + (n->duplicado ? " (duplicado)" : "");
            etiqueta += "\\n" + abreviar(n->hash);
        } else {
            etiqueta = "hash\\n" + abreviar(n->hash);
        }
        if (idPadre == -1) { color = "lightgreen"; etiqueta = "ROOT MERKLE\\n" + abreviar(n->hash); }

        dot << "  n" << id << " [label=\"" << etiqueta << "\", style=filled, fillcolor=" << color
            << ", tooltip=\"" << n->hash << "\"];\n";
        if (idPadre != -1) dot << "  n" << idPadre << " -> n" << id << ";\n";

        if (n->izquierdo) escribirDot(n->izquierdo, dot, contador, id);
        if (n->derecho) escribirDot(n->derecho, dot, contador, id);
    }

public:
    ArbolMerkle() : raiz(nullptr) {}
    ~ArbolMerkle() { destruir(raiz); }
    ArbolMerkle(const ArbolMerkle&) = delete;
    ArbolMerkle& operator=(const ArbolMerkle&) = delete;

    // Construye el árbol de abajo hacia arriba, nivel por nivel
    void construir(const ListaTransacciones& transacciones) {
        destruir(raiz);
        raiz = nullptr;
        int n = transacciones.getCantidad();
        if (n == 0) return;

        // Nivel 0: una hoja por transacción
        NodoMerkle** nivel = new NodoMerkle*[n];
        int i = 0;
        for (NodoTransaccion* t = transacciones.getPrimero(); t != nullptr; t = t->siguiente)
            nivel[i++] = new NodoMerkle(SHA256::hash(t->datos.serializar()), t->datos.codigo_transaccion);
        int tam = n;

        // Subir de nivel hasta que quede un solo nodo (la raíz)
        while (tam > 1) {
            int nuevoTam = (tam + 1) / 2;
            NodoMerkle** siguienteNivel = new NodoMerkle*[nuevoTam];
            for (int k = 0; k < nuevoTam; k++) {
                NodoMerkle* izq = nivel[2 * k];
                NodoMerkle* der = (2 * k + 1 < tam) ? nivel[2 * k + 1] : clonar(izq);   // impar: duplicar
                NodoMerkle* padre = new NodoMerkle(SHA256::hash(izq->hash + der->hash));
                padre->izquierdo = izq;
                padre->derecho = der;
                siguienteNivel[k] = padre;
            }
            delete[] nivel;
            nivel = siguienteNivel;
            tam = nuevoTam;
        }
        raiz = nivel[0];
        delete[] nivel;
    }

    // Hash de la raíz ("" si no hay transacciones)
    std::string hashRaiz() const { return raiz ? raiz->hash : ""; }

    // Contenido .dot del árbol (para el reporte)
    std::string generarDot(const std::string& titulo) const {
        std::ostringstream dot;
        dot << "digraph ArbolMerkle {\n";
        dot << "  graph [label=\"" << GraphvizUtil::esc(titulo) << "\", labelloc=t, fontname=\"Arial\", fontsize=16];\n";
        dot << "  node [shape=box, fontname=\"Arial\", fontsize=11];\n";
        if (raiz == nullptr) {
            dot << "  vacio [label=\"Sin transacciones\"];\n";
        } else {
            int contador = 0;
            escribirDot(raiz, dot, contador, -1);
        }
        dot << "}\n";
        return dot.str();
    }
};

#endif // MERKLETREE_H
