#ifndef CARGADORCSV_H
#define CARGADORCSV_H
#include "BSTCartelera.h"
#include <fstream>
#include <sstream>

class CargadorCSV {
public:
    static void cargarPeliculas(const std::string& ruta, BSTCartelera& arbol) {
        std::ifstream archivo(ruta);
        std::string linea;

        if (!archivo.is_open()) return;

        // Omitir encabezado
        std::getline(archivo, linea);

        while (std::getline(archivo, linea)) {
            if (linea.empty()) continue;
            std::stringstream ss(linea);
            std::string cod, tit, gen, durStr, clas, idio, fEst, fFin;

            std::getline(ss, cod, ',');
            std::getline(ss, tit, ',');
            std::getline(ss, gen, ',');
            std::getline(ss, durStr, ',');
            std::getline(ss, clas, ',');
            std::getline(ss, idio, ',');
            std::getline(ss, fEst, ',');
            std::getline(ss, fFin, ',');

            int duracion = 0;
            try { duracion = std::stoi(durStr); } catch (...) { duracion = 0; }

            arbol.insertar(cod, tit, gen, duracion, clas, idio, fEst, fFin);
        }
        archivo.close();
    }
};

#endif // CARGADORCSV_H