#ifndef GRAFOSEDES_H
#define GRAFOSEDES_H

#include "Sede.h"
#include "AVLFunciones.h"
#include "GraphvizUtil.h"
#include <string>
#include <sstream>

// ============================================================
// GRAFO NO DIRIGIDO PONDERADO DE SEDES  -  LISTA DE ADYACENCIA
//
//  * Cada SEDE es un vértice.
//  * Hay una arista entre dos sedes si comparten AL MENOS UNA película
//    programada (en funciones de ambas sedes).
//  * El PESO de la arista = cantidad de películas DISTINTAS que ambas comparten.
//  * Como el grafo es NO dirigido, cada arista se guarda dos veces:
//    en la lista de A (hacia B) y en la lista de B (hacia A), con el mismo peso.
//
// Representación (lista de adyacencia):
//
//   cabeza -> [S001] -> [S002] -> [S003] -> null          (lista de vértices)
//               |         |
//               v         v
//            (S002,2)  (S001,2)                            (lista de aristas de cada vértice)
//               |         |
//               v         v
//            (S003,1)  (S004,4)
//
// Todo es manual: listas enlazadas simples, cola para BFS y un ordenamiento
// propio (inserción) para el ranking. No se usan contenedores de la STL.
// ============================================================

// Lista simple de códigos (se usa para el catálogo de películas de cada sede)
struct NodoCodigoLista {
    std::string codigo;
    NodoCodigoLista* siguiente;
    NodoCodigoLista(const std::string& c) : codigo(c), siguiente(nullptr) {}
};

// Nodo de la lista de adyacencia: una conexión hacia otra sede
struct NodoArista {
    std::string destino;     // código de la sede vecina
    int peso;                // películas en común
    NodoArista* siguiente;
    NodoArista(const std::string& d, int p) : destino(d), peso(p), siguiente(nullptr) {}
};

// Vértice del grafo
struct VerticeSede {
    Sede datos;                      // información básica de la sede
    NodoArista* aristas;             // lista de adyacencia de este vértice
    NodoCodigoLista* catalogo;       // películas distintas que se proyectan en la sede
    VerticeSede* siguiente;          // siguiente vértice de la lista de vértices

    // Campos temporales usados por BFS
    bool visitado;
    VerticeSede* padre;

    VerticeSede(const Sede& s)
        : datos(s), aristas(nullptr), catalogo(nullptr), siguiente(nullptr),
          visitado(false), padre(nullptr) {}
};

// Un par de sedes con el peso de su relación (para ranking y sedes similares)
struct ParSedes {
    std::string a;
    std::string b;
    int peso;
};

// Arreglo dinámico de ParSedes con su cantidad (se libera solo)
struct ListaParesSedes {
    ParSedes* datos;
    int cantidad;
    ListaParesSedes() : datos(nullptr), cantidad(0) {}
    ~ListaParesSedes() { delete[] datos; }
private:
    ListaParesSedes(const ListaParesSedes&);
    ListaParesSedes& operator=(const ListaParesSedes&);
};

class GrafoSedes {
private:
    VerticeSede* cabeza;      // primer vértice
    int cantidad;             // cantidad de vértices

    // ---------- helpers de listas ----------
    VerticeSede* buscarVertice(const std::string& codigo) const {
        VerticeSede* v = cabeza;
        while (v != nullptr) {
            if (v->datos.codigo == codigo) return v;
            v = v->siguiente;
        }
        return nullptr;
    }

    static void liberarAristas(VerticeSede* v) {
        NodoArista* a = v->aristas;
        while (a != nullptr) { NodoArista* t = a; a = a->siguiente; delete t; }
        v->aristas = nullptr;
    }

    static void liberarCatalogo(VerticeSede* v) {
        NodoCodigoLista* c = v->catalogo;
        while (c != nullptr) { NodoCodigoLista* t = c; c = c->siguiente; delete t; }
        v->catalogo = nullptr;
    }

    // Agrega una película al catálogo solo si todavía no estaba (distintas)
    static void agregarCodigoUnico(VerticeSede* v, const std::string& codPel) {
        NodoCodigoLista* c = v->catalogo;
        while (c != nullptr) {
            if (c->codigo == codPel) return;
            c = c->siguiente;
        }
        NodoCodigoLista* nuevo = new NodoCodigoLista(codPel);
        nuevo->siguiente = v->catalogo;
        v->catalogo = nuevo;
    }

    // Cuenta cuántas películas hay en AMBOS catálogos (intersección)
    static int contarComunes(NodoCodigoLista* a, NodoCodigoLista* b) {
        int comunes = 0;
        for (NodoCodigoLista* x = a; x != nullptr; x = x->siguiente) {
            for (NodoCodigoLista* y = b; y != nullptr; y = y->siguiente) {
                if (x->codigo == y->codigo) { comunes++; break; }
            }
        }
        return comunes;
    }

    // Agrega una arista al final de la lista de adyacencia de v
    static void agregarArista(VerticeSede* v, const std::string& destino, int peso) {
        NodoArista* nueva = new NodoArista(destino, peso);
        if (v->aristas == nullptr) { v->aristas = nueva; return; }
        NodoArista* a = v->aristas;
        while (a->siguiente != nullptr) a = a->siguiente;
        a->siguiente = nueva;
    }

    // Quita de v la arista que apunta a "destino" (si existe)
    static void quitarArista(VerticeSede* v, const std::string& destino) {
        NodoArista* actual = v->aristas;
        NodoArista* anterior = nullptr;
        while (actual != nullptr) {
            if (actual->destino == destino) {
                if (anterior == nullptr) v->aristas = actual->siguiente;
                else anterior->siguiente = actual->siguiente;
                delete actual;
                return;
            }
            anterior = actual;
            actual = actual->siguiente;
        }
    }

    // Ordena un arreglo de pares de MAYOR a MENOR peso (inserción; desempate por códigos)
    static void ordenarPorPesoDesc(ParSedes* arr, int n) {
        for (int i = 1; i < n; i++) {
            ParSedes clave = arr[i];
            int j = i - 1;
            while (j >= 0 && antes(clave, arr[j])) {
                arr[j + 1] = arr[j];
                j--;
            }
            arr[j + 1] = clave;
        }
    }

    // ¿x debe ir antes que y en el ranking?
    static bool antes(const ParSedes& x, const ParSedes& y) {
        if (x.peso != y.peso) return x.peso > y.peso;
        if (x.a != y.a) return x.a < y.a;
        return x.b < y.b;
    }

    void destruir() {
        VerticeSede* v = cabeza;
        while (v != nullptr) {
            VerticeSede* t = v;
            v = v->siguiente;
            liberarAristas(t);
            liberarCatalogo(t);
            delete t;
        }
        cabeza = nullptr;
        cantidad = 0;
    }

public:
    GrafoSedes() : cabeza(nullptr), cantidad(0) {}
    ~GrafoSedes() { destruir(); }

    // El grafo no se copia (maneja memoria dinámica)
    GrafoSedes(const GrafoSedes&) = delete;
    GrafoSedes& operator=(const GrafoSedes&) = delete;

    // =========================================================
    // VÉRTICES (sedes)
    // =========================================================

    // Registrar nueva sede: agrega un vértice SIN aristas. false si el código ya existe.
    bool agregarSede(const Sede& s) {
        if (s.codigo.empty() || buscarVertice(s.codigo) != nullptr) return false;
        VerticeSede* nuevo = new VerticeSede(s);
        if (cabeza == nullptr) {
            cabeza = nuevo;
        } else {
            VerticeSede* v = cabeza;
            while (v->siguiente != nullptr) v = v->siguiente;
            v->siguiente = nuevo;
        }
        cantidad++;
        return true;
    }

    bool existe(const std::string& codigo) const { return buscarVertice(codigo) != nullptr; }

    Sede* obtenerSede(const std::string& codigo) const {
        VerticeSede* v = buscarVertice(codigo);
        return v ? &v->datos : nullptr;
    }

    // Editar los datos de una sede (el código no cambia porque es la identidad del vértice)
    bool editarSede(const std::string& codigo, const std::string& nombre, const std::string& direccion,
                    int salas, const std::string& telefono) {
        VerticeSede* v = buscarVertice(codigo);
        if (v == nullptr) return false;
        v->datos.nombre = nombre;
        v->datos.direccion = direccion;
        v->datos.cantidad_salas = salas;
        v->datos.telefono = telefono;
        return true;
    }

    // Eliminar una sede: quita el vértice y todas las aristas que llegaban a él.
    // (Las funciones de esa sede las elimina quien llama; luego se recalculan las aristas.)
    bool eliminarSede(const std::string& codigo) {
        VerticeSede* v = cabeza;
        VerticeSede* anterior = nullptr;
        while (v != nullptr && v->datos.codigo != codigo) {
            anterior = v;
            v = v->siguiente;
        }
        if (v == nullptr) return false;

        if (anterior == nullptr) cabeza = v->siguiente;
        else anterior->siguiente = v->siguiente;

        // Quitar las aristas de los demás vértices que apuntaban a esta sede
        for (VerticeSede* otro = cabeza; otro != nullptr; otro = otro->siguiente)
            quitarArista(otro, codigo);

        liberarAristas(v);
        liberarCatalogo(v);
        delete v;
        cantidad--;
        return true;
    }

    int cantidadSedes() const { return cantidad; }

    // Aplica una lambda(const Sede&) a cada sede en orden de registro
    template <typename F>
    void recorrerSedes(F accion) const {
        for (VerticeSede* v = cabeza; v != nullptr; v = v->siguiente) accion(v->datos);
    }

    // =========================================================
    // ARISTAS (se recalculan a partir de las funciones del AVL)
    // =========================================================

    // Reconstruye TODAS las aristas desde cero:
    //   1) cada sede junta las películas distintas de sus funciones (catálogo)
    //   2) para cada par de sedes se cuenta la intersección de catálogos
    //   3) si la intersección > 0 se crea la arista con ese peso
    // Se llama cada vez que se crea, edita o elimina una función (o al cargar datos).
    // Así una arista aparece, cambia de peso o desaparece sola cuando corresponde.
    void recalcularAristas(const AVLFunciones& funciones) {
        for (VerticeSede* v = cabeza; v != nullptr; v = v->siguiente) {
            liberarAristas(v);
            liberarCatalogo(v);
        }

        funciones.recorrerInorden([this](Funcion* f) {
            VerticeSede* v = buscarVertice(f->codigo_sede);
            if (v != nullptr) agregarCodigoUnico(v, f->codigo_pelicula);
        });

        for (VerticeSede* a = cabeza; a != nullptr; a = a->siguiente) {
            for (VerticeSede* b = a->siguiente; b != nullptr; b = b->siguiente) {
                int peso = contarComunes(a->catalogo, b->catalogo);
                if (peso > 0) {
                    agregarArista(a, b->datos.codigo, peso);   // A -> B
                    agregarArista(b, a->datos.codigo, peso);   // B -> A (no dirigido)
                }
            }
        }
    }

    // Peso de la arista entre dos sedes (0 si no hay arista)
    int pesoEntre(const std::string& a, const std::string& b) const {
        VerticeSede* v = buscarVertice(a);
        if (v == nullptr) return 0;
        for (NodoArista* x = v->aristas; x != nullptr; x = x->siguiente)
            if (x->destino == b) return x->peso;
        return 0;
    }

    // ¿La sede proyecta esta película? (usa el catálogo calculado en recalcularAristas)
    bool sedeTienePelicula(const std::string& codSede, const std::string& codPel) const {
        VerticeSede* v = buscarVertice(codSede);
        if (v == nullptr) return false;
        for (NodoCodigoLista* c = v->catalogo; c != nullptr; c = c->siguiente)
            if (c->codigo == codPel) return true;
        return false;
    }

    // =========================================================
    // BFS: camino más corto (en número de aristas) entre dos sedes
    // =========================================================
    // Se usa BFS porque lo que importa es la cantidad de saltos, no el peso:
    // en un grafo sin pesos BFS garantiza el camino mínimo en aristas.
    // Devuelve false si alguna sede no existe o no hay camino.
    bool caminoMasCorto(const std::string& origen, const std::string& destino,
                        std::string& camino, int& saltos) {
        camino.clear();
        saltos = 0;
        VerticeSede* ini = buscarVertice(origen);
        VerticeSede* fin = buscarVertice(destino);
        if (ini == nullptr || fin == nullptr) return false;

        // Preparar marcas
        for (VerticeSede* v = cabeza; v != nullptr; v = v->siguiente) {
            v->visitado = false;
            v->padre = nullptr;
        }

        // Cola manual (lista enlazada con frente y final)
        struct NodoCola {
            VerticeSede* vertice;
            NodoCola* siguiente;
        };
        NodoCola* frente = new NodoCola{ini, nullptr};
        NodoCola* ultimo = frente;
        ini->visitado = true;

        while (frente != nullptr) {
            // Desencolar
            NodoCola* actualNodo = frente;
            VerticeSede* actual = actualNodo->vertice;
            frente = frente->siguiente;
            if (frente == nullptr) ultimo = nullptr;
            delete actualNodo;

            if (actual == fin) break;      // destino alcanzado

            // Encolar vecinos no visitados
            for (NodoArista* a = actual->aristas; a != nullptr; a = a->siguiente) {
                VerticeSede* vecino = buscarVertice(a->destino);
                if (vecino != nullptr && !vecino->visitado) {
                    vecino->visitado = true;
                    vecino->padre = actual;
                    NodoCola* nuevo = new NodoCola{vecino, nullptr};
                    if (ultimo == nullptr) { frente = nuevo; ultimo = nuevo; }
                    else { ultimo->siguiente = nuevo; ultimo = nuevo; }
                }
            }
        }

        // Liberar lo que haya quedado en la cola
        while (frente != nullptr) {
            NodoCola* t = frente;
            frente = frente->siguiente;
            delete t;
        }

        if (!fin->visitado) return false;   // no hay camino

        // Reconstruir el camino yendo de destino hacia origen por los "padres"
        for (VerticeSede* v = fin; v != nullptr; v = v->padre) {
            std::string tramo = v->datos.codigo + " (" + v->datos.nombre + ")";
            camino = camino.empty() ? tramo : tramo + "  ->  " + camino;
            if (v != ini) saltos++;
        }
        return true;
    }

    // =========================================================
    // Ranking de aristas y sedes similares
    // =========================================================

    // Todas las aristas del grafo (una vez cada una) de mayor a menor peso
    void rankingAristas(ListaParesSedes& salida) const {
        delete[] salida.datos;
        salida.datos = nullptr;
        salida.cantidad = 0;

        int total = 0;
        for (VerticeSede* v = cabeza; v != nullptr; v = v->siguiente)
            for (NodoArista* a = v->aristas; a != nullptr; a = a->siguiente)
                if (v->datos.codigo < a->destino) total++;      // cada arista se cuenta una sola vez
        if (total == 0) return;

        salida.datos = new ParSedes[total];
        int i = 0;
        for (VerticeSede* v = cabeza; v != nullptr; v = v->siguiente)
            for (NodoArista* a = v->aristas; a != nullptr; a = a->siguiente)
                if (v->datos.codigo < a->destino)
                    salida.datos[i++] = ParSedes{v->datos.codigo, a->destino, a->peso};
        salida.cantidad = total;
        ordenarPorPesoDesc(salida.datos, total);
    }

    // Vecinos de una sede ordenados por peso descendente (sedes similares)
    void sedesSimilares(const std::string& codigo, ListaParesSedes& salida) const {
        delete[] salida.datos;
        salida.datos = nullptr;
        salida.cantidad = 0;

        VerticeSede* v = buscarVertice(codigo);
        if (v == nullptr) return;
        int total = 0;
        for (NodoArista* a = v->aristas; a != nullptr; a = a->siguiente) total++;
        if (total == 0) return;

        salida.datos = new ParSedes[total];
        int i = 0;
        for (NodoArista* a = v->aristas; a != nullptr; a = a->siguiente)
            salida.datos[i++] = ParSedes{codigo, a->destino, a->peso};
        salida.cantidad = total;
        ordenarPorPesoDesc(salida.datos, total);
    }

    // =========================================================
    // REPORTES GRAPHVIZ
    // =========================================================

    // Reporte 1: el grafo no dirigido (vértices con info básica y aristas con su peso)
    QString generarReporteGrafo() const {
        std::ostringstream dot;
        dot << "graph GrafoSedes {\n";
        dot << "  layout=neato;\n  overlap=false;\n  sep=\"+12\";\n";
        dot << "  graph [label=\"Grafo no dirigido ponderado de sedes\\nSedes: " << cantidad
            << "\", labelloc=t, fontname=\"Arial\", fontsize=16];\n";
        dot << "  node [shape=circle, style=filled, fillcolor=lightblue, fontname=\"Arial\", fontsize=10];\n";
        dot << "  edge [fontname=\"Arial\", fontsize=12, color=\"#444444\"];\n";

        if (cabeza == nullptr) {
            dot << "  vacio [label=\"Sin sedes registradas\", shape=box];\n";
        }

        for (VerticeSede* v = cabeza; v != nullptr; v = v->siguiente) {
            dot << "  \"" << GraphvizUtil::esc(v->datos.codigo) << "\" [label=\""
                << GraphvizUtil::esc(v->datos.codigo) << "\\n"
                << GraphvizUtil::esc(v->datos.nombre) << "\\n"
                << GraphvizUtil::esc(v->datos.direccion) << "\\n"
                << "Salas: " << v->datos.cantidad_salas << "\\n"
                << "Tel: " << GraphvizUtil::esc(v->datos.telefono) << "\"];\n";
        }
        for (VerticeSede* v = cabeza; v != nullptr; v = v->siguiente) {
            for (NodoArista* a = v->aristas; a != nullptr; a = a->siguiente) {
                if (v->datos.codigo < a->destino) {   // cada arista se dibuja una sola vez
                    dot << "  \"" << GraphvizUtil::esc(v->datos.codigo) << "\" -- \""
                        << GraphvizUtil::esc(a->destino) << "\" [label=\"" << a->peso
                        << "\", penwidth=" << (1 + (a->peso > 6 ? 6 : a->peso) / 2.0) << "];\n";
                }
            }
        }
        dot << "}\n";
        return GraphvizUtil::generarPng("Grafo_Sedes", dot.str());
    }

    // Reporte 2: la lista de adyacencia (cada vértice seguido de su lista de conexiones)
    QString generarReporteListaAdyacencia() const {
        std::ostringstream dot;
        dot << "digraph ListaAdyacencia {\n";
        dot << "  rankdir=LR;\n  nodesep=0.35;\n";
        dot << "  graph [label=\"Lista de adyacencia del grafo de sedes\", labelloc=t, fontname=\"Arial\", fontsize=16];\n";
        dot << "  node [fontname=\"Arial\", shape=record];\n";

        if (cabeza == nullptr) {
            dot << "  vacio [label=\"Sin sedes registradas\", shape=box];\n";
        }

        // Columna de vértices (todos en el mismo rank, unidos por aristas invisibles para apilarlos)
        dot << "  { rank=same;\n";
        for (VerticeSede* v = cabeza; v != nullptr; v = v->siguiente) {
            dot << "    \"v_" << GraphvizUtil::esc(v->datos.codigo) << "\" [style=filled, fillcolor=lightblue, label=\"{"
                << GraphvizUtil::esc(v->datos.codigo) << "|" << GraphvizUtil::esc(v->datos.nombre)
                << "|Salas: " << v->datos.cantidad_salas << "|Tel: " << GraphvizUtil::esc(v->datos.telefono) << "}\"];\n";
        }
        for (VerticeSede* v = cabeza; v != nullptr && v->siguiente != nullptr; v = v->siguiente) {
            dot << "    \"v_" << GraphvizUtil::esc(v->datos.codigo) << "\" -> \"v_"
                << GraphvizUtil::esc(v->siguiente->datos.codigo) << "\" [style=invis];\n";
        }
        dot << "  }\n";

        // Lista de conexiones de cada vértice
        for (VerticeSede* v = cabeza; v != nullptr; v = v->siguiente) {
            std::string anterior = "v_" + v->datos.codigo;
            for (NodoArista* a = v->aristas; a != nullptr; a = a->siguiente) {
                std::string id = "a_" + v->datos.codigo + "_" + a->destino;
                Sede* destino = obtenerSede(a->destino);
                dot << "  \"" << GraphvizUtil::esc(id) << "\" [style=filled, fillcolor=lightyellow, label=\"{"
                    << GraphvizUtil::esc(a->destino) << "|" << GraphvizUtil::esc(destino ? destino->nombre : "")
                    << "|peso: " << a->peso << "}\"];\n";
                dot << "  \"" << GraphvizUtil::esc(anterior) << "\" -> \"" << GraphvizUtil::esc(id) << "\";\n";
                anterior = id;
            }
        }
        dot << "}\n";
        return GraphvizUtil::generarPng("Lista_Adyacencia", dot.str());
    }
};

#endif // GRAFOSEDES_H
