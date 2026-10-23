#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include "LZW.h"
#include "GrafoSedes.h"
#include "BSTCartelera.h"
#include "AVLFunciones.h"
#include "MatrizDispersa.h"

#include <string>
#include <sstream>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ============================================================
// PERSISTENCIA COMPRIMIDA (LZW)  ->  sedes.edd, peliculas.edd, funciones.edd
//
// GUARDAR (al cerrar el programa):
//   estructura en memoria --recorrido--> texto estructurado --LZW--> archivo .edd
//   * El texto se arma recorriendo DIRECTAMENTE las estructuras (grafo, BST, AVL).
//   * NO se escribe ningún archivo JSON/XML intermedio y NO se lee el archivo de carga masiva.
//
// CARGAR (al iniciar el programa):
//   archivo .edd --descompresión LZW--> texto en memoria --> se reconstruyen las estructuras
//   (el texto se interpreta en memoria; tampoco hay archivo intermedio).
//
// El texto tiene la forma { "sedes": [ ... ] }, { "peliculas": [ ... ] } y { "funciones": [ ... ] }.
// ============================================================
class Persistencia {
private:
    static QString rutaArchivo(const QString& nombre) {
        return QCoreApplication::applicationDirPath() + "/" + nombre;
    }

    // Escapa un texto para meterlo entre comillas
    static std::string esc(const std::string& t) {
        std::string r;
        for (char c : t) {
            if (c == '"') r += "\\\"";
            else if (c == '\\') r += "\\\\";
            else if (c == '\n') r += "\\n";
            else if (c == '\r') r += "\\r";
            else if (c == '\t') r += "\\t";
            else r += c;
        }
        return r;
    }

    // ---------- SERIALIZACIÓN (estructura -> texto) ----------

    static std::string serializarSedes(const GrafoSedes& grafo) {
        std::ostringstream out;
        out << "{\n  \"sedes\": [\n";
        int total = grafo.cantidadSedes();
        int i = 0;
        grafo.recorrerSedes([&](const Sede& s) {
            out << "    {\"codigo\": \"" << esc(s.codigo) << "\", \"nombre\": \"" << esc(s.nombre)
                << "\", \"direccion\": \"" << esc(s.direccion) << "\", \"cantidad_salas\": " << s.cantidad_salas
                << ", \"telefono\": \"" << esc(s.telefono) << "\"}" << ((++i < total) ? "," : "") << "\n";
        });
        out << "  ]\n}\n";
        return out.str();
    }

    static std::string serializarPeliculas(const BSTCartelera& cartelera) {
        std::ostringstream out;
        out << "{\n  \"peliculas\": [\n";
        bool primera = true;
        // Preorden: al volver a insertar en este orden el BST queda con la misma forma
        cartelera.recorrerPreorden([&](Pelicula* p) {
            if (!primera) out << ",\n";
            primera = false;
            out << "    {\"codigo\": \"" << esc(p->codigo) << "\", \"titulo\": \"" << esc(p->titulo)
                << "\", \"genero\": \"" << esc(p->genero) << "\", \"duracion\": " << p->duracion
                << ", \"clasificacion\": \"" << esc(p->clasificacion) << "\", \"idioma\": \"" << esc(p->idioma)
                << "\", \"fecha_estreno\": \"" << esc(p->fechaEstreno) << "\", \"fecha_fin\": \""
                << esc(p->fechaFin) << "\"}";
        });
        out << "\n  ]\n}\n";
        return out.str();
    }

    static std::string serializarFunciones(const AVLFunciones& funciones) {
        std::ostringstream out;
        out << "{\n  \"funciones\": [\n";
        bool primera = true;
        funciones.recorrerPreorden([&](Funcion* f) {
            if (!primera) out << ",\n";
            primera = false;
            out << "    {\"codigo_funcion\": \"" << esc(f->codigo_funcion) << "\", \"codigo_pelicula\": \""
                << esc(f->codigo_pelicula) << "\", \"sede\": \"" << esc(f->codigo_sede) << "\", \"fecha\": \""
                << esc(f->fecha) << "\", \"horario\": \"" << esc(f->horario) << "\", \"sala\": \"" << esc(f->sala)
                << "\", \"filas\": " << f->filas << ", \"columnas\": " << f->columnas
                << ", \"archivo_asientos\": \"" << esc(f->archivo_asientos) << "\"}";
        });
        out << "\n  ]\n}\n";
        return out.str();
    }

    // ---------- Archivos ----------

    // texto --LZW--> bytes --> archivo
    static bool guardarComprimido(const QString& nombre, const std::string& texto) {
        std::string comprimido = LZW::comprimir(texto);
        QFile f(rutaArchivo(nombre));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        f.write(comprimido.data(), (qint64)comprimido.size());
        f.close();
        return true;
    }

    // archivo --> bytes --LZW--> texto
    static bool leerDescomprimido(const QString& nombre, std::string& texto) {
        QFile f(rutaArchivo(nombre));
        if (!f.open(QIODevice::ReadOnly)) return false;
        QByteArray bytes = f.readAll();
        f.close();
        return LZW::descomprimir(std::string(bytes.constData(), (size_t)bytes.size()), texto);
    }

    // Interpreta el texto (en memoria) y devuelve el arreglo de objetos con ese nombre
    static QJsonArray arregloDeTexto(const std::string& texto, const char* nombre) {
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray(texto.data(), (int)texto.size()));
        if (doc.isNull() || !doc.isObject()) return QJsonArray();
        return doc.object()[nombre].toArray();
    }

public:
    // =========================================================
    // GUARDAR: se llama al cerrar el programa
    // =========================================================
    static bool guardarTodo(const GrafoSedes& grafo, const BSTCartelera& cartelera, const AVLFunciones& funciones) {
        bool a = guardarComprimido("sedes.edd", serializarSedes(grafo));
        bool b = guardarComprimido("peliculas.edd", serializarPeliculas(cartelera));
        bool c = guardarComprimido("funciones.edd", serializarFunciones(funciones));
        return a && b && c;
    }

    // =========================================================
    // CARGAR: se llama al iniciar el programa
    // =========================================================
    // Devuelve true si existía al menos un archivo .edd. Los contadores indican cuántos elementos se cargaron.
    static bool cargarTodo(GrafoSedes& grafo, BSTCartelera& cartelera, AVLFunciones& funciones,
                           int& nSedes, int& nPeliculas, int& nFunciones) {
        nSedes = nPeliculas = nFunciones = 0;
        bool huboArchivos = false;
        std::string texto;

        // ----- sedes.edd -----
        if (leerDescomprimido("sedes.edd", texto)) {
            huboArchivos = true;
            for (const QJsonValue& v : arregloDeTexto(texto, "sedes")) {
                QJsonObject o = v.toObject();
                Sede s(o["codigo"].toString().toStdString(), o["nombre"].toString().toStdString(),
                       o["direccion"].toString().toStdString(), o["cantidad_salas"].toInt(),
                       o["telefono"].toString().toStdString());
                if (grafo.agregarSede(s)) nSedes++;      // cada sede vuelve a ser un vértice
            }
        }

        // ----- peliculas.edd -----
        if (leerDescomprimido("peliculas.edd", texto)) {
            huboArchivos = true;
            for (const QJsonValue& v : arregloDeTexto(texto, "peliculas")) {
                QJsonObject o = v.toObject();
                cartelera.insertar(o["codigo"].toString().toStdString(), o["titulo"].toString().toStdString(),
                                   o["genero"].toString().toStdString(), o["duracion"].toInt(),
                                   o["clasificacion"].toString().toStdString(), o["idioma"].toString().toStdString(),
                                   o["fecha_estreno"].toString().toStdString(), o["fecha_fin"].toString().toStdString());
                nPeliculas++;
            }
        }

        // ----- funciones.edd -----
        if (leerDescomprimido("funciones.edd", texto)) {
            huboArchivos = true;
            for (const QJsonValue& v : arregloDeTexto(texto, "funciones")) {
                QJsonObject o = v.toObject();
                std::string cod = o["codigo_funcion"].toString().toStdString();
                std::string archivo = o["archivo_asientos"].toString().toStdString();
                funciones.insertar(cod, o["codigo_pelicula"].toString().toStdString(),
                                   o["horario"].toString().toStdString(), o["sala"].toString().toStdString(),
                                   o["filas"].toInt(), o["columnas"].toInt(), archivo,
                                   o["fecha"].toString().toStdString(), o["sede"].toString().toStdString());
                nFunciones++;

                // Si por algún motivo falta el archivo de asientos de la función, se crea vacío
                QString rutaAsientos = QCoreApplication::applicationDirPath() + "/asientos/" + QString::fromStdString(archivo);
                if (!archivo.empty() && !QFile::exists(rutaAsientos)) {
                    QDir().mkpath(QCoreApplication::applicationDirPath() + "/asientos");
                    MatrizDispersa::crearArchivoVacio(cod, rutaAsientos.toStdString());
                }
            }
        }

        // Las aristas NO se guardan: se derivan de las funciones y se recalculan
        grafo.recalcularAristas(funciones);
        return huboArchivos;
    }
};

#endif // PERSISTENCIA_H
