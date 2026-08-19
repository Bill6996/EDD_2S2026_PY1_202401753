#ifndef PELICULA_H
#define PELICULA_H
#include <string>

struct Pelicula {
    std::string codigo;
    std::string titulo;
    std::string genero;
    int duracion;
    std::string clasificacion;
    std::string idioma;
    std::string fechaEstreno;
    std::string fechaFin;

    Pelicula* izquierda;
    Pelicula* derecha;

    Pelicula(std::string cod, std::string tit, std::string gen, int dur,
             std::string clas, std::string idio, std::string fEstreno, std::string fFin)
        : codigo(cod), titulo(tit), genero(gen), duracion(dur),
        clasificacion(clas), idioma(idio), fechaEstreno(fEstreno), fechaFin(fFin),
        izquierda(nullptr), derecha(nullptr) {}
};

#endif // PELICULA_H