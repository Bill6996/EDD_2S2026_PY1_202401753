#ifndef GRAPHVIZUTIL_H
#define GRAPHVIZUTIL_H

#include <string>
#include <fstream>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QString>

// ============================================================
// Utilidades comunes para los reportes de la Fase 3.
// Todas las estructuras nuevas (grafo, lista de adyacencia, blockchain, Merkle)
// escriben su archivo .dot y llaman a generarPng() para obtener la imagen.
// Los reportes se guardan en <carpeta del ejecutable>/reportes, igual que en las fases anteriores.
// ============================================================
class GraphvizUtil {
public:
    // Escapa comillas, backslash y saltos de línea para usar texto dentro de un label de dot
    static std::string esc(const std::string& t) {
        std::string r;
        for (char c : t) {
            if (c == '"') r += "\\\"";
            else if (c == '\\') r += "\\\\";
            else if (c == '\n') r += "\\n";
            else r += c;
        }
        return r;
    }

    // Carpeta donde se guardan los reportes
    static QString carpetaReportes() {
        QString ruta = QCoreApplication::applicationDirPath() + "/reportes";
        QDir().mkpath(ruta);
        return ruta;
    }

    // Escribe <nombre>.dot y ejecuta Graphviz para crear <nombre>.png.
    // Devuelve la ruta del PNG (o "" si no se pudo escribir el .dot)
    static QString generarPng(const QString& nombre, const std::string& contenidoDot,
                              const QString& motor = "dot") {
        QString base = carpetaReportes() + "/" + nombre;
        QString dotFile = base + ".dot";
        QString pngFile = base + ".png";

        std::ofstream file(dotFile.toStdString());
        if (!file.is_open()) return "";
        file << contenidoDot;
        file.close();

        QProcess process;
        process.start(motor, QStringList() << "-Tpng" << dotFile << "-o" << pngFile);
        process.waitForFinished(8000);
        return pngFile;
    }
};

#endif // GRAPHVIZUTIL_H
