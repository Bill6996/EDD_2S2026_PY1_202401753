#ifndef BSTCARTELERA_H
#define BSTCARTELERA_H
#include "Pelicula.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

class BSTCartelera {
private:
    Pelicula* raiz;

    Pelicula* insertarRecursivo(Pelicula* nodo, Pelicula* nuevo) {
        if (nodo == nullptr) return nuevo;
        if (nuevo->codigo < nodo->codigo) {
            nodo->izquierda = insertarRecursivo(nodo->izquierda, nuevo);
        } else if (nuevo->codigo > nodo->codigo) {
            nodo->derecha = insertarRecursivo(nodo->derecha, nuevo);
        }
        return nodo;
    }

    void generarGraphvizRecursivo(Pelicula* nodo, std::ofstream& file) {
        if (nodo != nullptr) {
            file << "  node_" << nodo->codigo << " [label=\"Codigo: " << nodo->codigo
                 << "\\nTitulo: " << nodo->titulo
                 << "\\nDuracion: " << nodo->duracion << " min"
                 << "\\nClasificacion: " << nodo->clasificacion << "\", shape=box, style=filled, fillcolor=lightgreen];\n";

            if (nodo->izquierda != nullptr) {
                file << "  node_" << nodo->codigo << " -> node_" << nodo->izquierda->codigo << ";\n";
                generarGraphvizRecursivo(nodo->izquierda, file);
            }
            if (nodo->derecha != nullptr) {
                file << "  node_" << nodo->codigo << " -> node_" << nodo->derecha->codigo << ";\n";
                generarGraphvizRecursivo(nodo->derecha, file);
            }
        }
    }

public:
    BSTCartelera() : raiz(nullptr) {}

    void insertar(std::string cod, std::string tit, std::string gen, int dur,
                  std::string clas, std::string idio, std::string fEstreno, std::string fFin) {
        Pelicula* nuevo = new Pelicula(cod, tit, gen, dur, clas, idio, fEstreno, fFin);
        raiz = insertarRecursivo(raiz, nuevo);
    }

    void generarReporteGraphviz() {
        std::ofstream file("../reportes/BST_Cartelera.dot");
        file << "digraph BST_Cartelera {\n";
        file << "  node [fontname=\"Arial\"];\n";
        if (raiz == nullptr) {
            file << "  empty [label=\"Cartelera Vacia\"];\n";
        } else {
            generarGraphvizRecursivo(raiz, file);
        }
        file << "}\n";
        file.close();

        system("dot -Tpng ../reportes/BST_Cartelera.dot -o ../reportes/BST_Cartelera.png");
    }
};

#endif // BSTCARTELERA_H