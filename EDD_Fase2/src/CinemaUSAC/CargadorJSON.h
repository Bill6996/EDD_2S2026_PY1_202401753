#ifndef CARGADORJSON_H
#define CARGADORJSON_H

#include "BSTCartelera.h"
#include "AVLFunciones.h"
#include "ArbolBClientes.h"
#include "TablaHashReservas.h"
#include "MatrizDispersa.h"
#include "Cliente.h"
#include "Reserva.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDate>
#include <string>
#include <iostream>

class CargadorJSON {
public:
    // ============================================================
    // 1. Cargar PELÍCULAS + FUNCIONES
    // ============================================================
    // Retorna la cantidad de películas cargadas correctamente
    static int cargarPeliculasYFunciones(const std::string& ruta,
                                         BSTCartelera& cartelera,
                                         AVLFunciones& funciones) {
        QFile file(QString::fromStdString(ruta));
        if (!file.open(QIODevice::ReadOnly)) {
            std::cerr << "No se pudo abrir el archivo: " << ruta << std::endl;
            return 0;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            std::cerr << "JSON inválido" << std::endl;
            return 0;
        }

        QJsonObject root = doc.object();
        if (!root.contains("peliculas") || !root["peliculas"].isArray()) {
            std::cerr << "No se encontró el array 'peliculas'" << std::endl;
            return 0;
        }

        QJsonArray arrPeliculas = root["peliculas"].toArray();
        int contador = 0;

        for (const QJsonValue& val : arrPeliculas) {
            if (!val.isObject()) continue;
            QJsonObject obj = val.toObject();

            // ----- Validaciones básicas -----
            QString codigo = obj["codigo"].toString().trimmed();
            QString titulo = obj["titulo"].toString().trimmed();
            QString genero = obj["genero"].toString().trimmed();
            int duracion   = obj["duracion"].toInt(-1);
            QString clasif = obj["clasificacion"].toString().trimmed();
            QString idioma = obj["idioma"].toString().trimmed();
            QString fEst   = obj["fecha_estreno"].toString().trimmed();
            QString fFin   = obj["fecha_fin"].toString().trimmed();

            if (codigo.isEmpty() || titulo.isEmpty() || duracion <= 0) {
                std::cerr << "Película inválida (campos obligatorios faltantes), se omite." << std::endl;
                continue;
            }

            // Validar fechas
            QDate fechaEst = QDate::fromString(fEst, "yyyy-MM-dd");
            QDate fechaFin = QDate::fromString(fFin, "yyyy-MM-dd");
            if (!fechaEst.isValid() || !fechaFin.isValid()) {
                std::cerr << "Película " << codigo.toStdString()
                          << " tiene fechas inválidas, se omite." << std::endl;
                continue;
            }

            // Insertar en el BST
            cartelera.insertar(codigo.toStdString(), titulo.toStdString(),
                               genero.toStdString(), duracion,
                               clasif.toStdString(), idioma.toStdString(),
                               fEst.toStdString(), fFin.toStdString());
            contador++;

            // ----- Cargar funciones de esta película (si existen) -----
            if (obj.contains("funciones") && obj["funciones"].isArray()) {
                QJsonArray arrFun = obj["funciones"].toArray();
                for (const QJsonValue& valFun : arrFun) {
                    if (!valFun.isObject()) continue;
                    QJsonObject fObj = valFun.toObject();

                    QString codFun  = fObj["codigo_funcion"].toString().trimmed();
                    QString horario = fObj["horario"].toString().trimmed();
                    QString sala    = fObj["sala"].toString().trimmed();
                    int filas       = fObj["filas"].toInt(-1);
                    int columnas    = fObj["columnas"].toInt(-1);

                    if (codFun.isEmpty() || horario.isEmpty() || filas <= 0 || columnas <= 0) {
                        std::cerr << "Función inválida dentro de " << codigo.toStdString()
                            << ", se omite." << std::endl;
                        continue;
                    }

                    // archivo_asientos se genera al crear la función
                    // archivo_asientos se genera al crear la función
                    std::string archivo = codFun.toStdString() + "_funcion.json";

                    // Fecha de la función (si no viene en el JSON, usar una futura)
                    QString fechaFun = fObj["fecha"].toString().trimmed();
                    if (fechaFun.isEmpty())
                        fechaFun = "2026-12-31";

                    funciones.insertar(codFun.toStdString(),
                                       codigo.toStdString(),
                                       horario.toStdString(),
                                       sala.toStdString(),
                                       filas, columnas,
                                       archivo,
                                       fechaFun.toStdString());

                    // Crear el archivo JSON vacío de asientos
                    QString rutaAsientos = QCoreApplication::applicationDirPath()
                                           + "/asientos/" + QString::fromStdString(archivo);
                    QDir().mkpath(QCoreApplication::applicationDirPath() + "/asientos");
                    MatrizDispersa::crearArchivoVacio(codFun.toStdString(),
                                                      rutaAsientos.toStdString());
                }
            }
        }
        return contador;
    }

    // ============================================================
    // 2. Cargar CLIENTES + RESERVAS
    // ============================================================
    // Retorna la cantidad de clientes cargados correctamente
    static int cargarClientesYReservas(const std::string& ruta,
                                       ArbolBClientes& clientes,
                                       TablaHashReservas& reservas) {
        QFile file(QString::fromStdString(ruta));
        if (!file.open(QIODevice::ReadOnly)) {
            std::cerr << "No se pudo abrir el archivo: " << ruta << std::endl;
            return 0;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            std::cerr << "JSON inválido" << std::endl;
            return 0;
        }

        QJsonObject root = doc.object();
        if (!root.contains("clientes") || !root["clientes"].isArray()) {
            std::cerr << "No se encontró el array 'clientes'" << std::endl;
            return 0;
        }

        QJsonArray arrClientes = root["clientes"].toArray();
        int contador = 0;

        for (const QJsonValue& val : arrClientes) {
            if (!val.isObject()) continue;
            QJsonObject obj = val.toObject();

            QString id       = obj["id"].toString().trimmed();
            QString nombre   = obj["nombre"].toString().trimmed();
            QString correo   = obj["correo"].toString().trimmed();
            QString telefono = obj["telefono"].toString().trimmed();
            QString password = obj["password"].toString().trimmed();
            QString tipo     = obj["tipo"].toString().trimmed();
            if (tipo.isEmpty()) tipo = "cliente";

            if (id.isEmpty() || nombre.isEmpty() || correo.isEmpty() || password.isEmpty()) {
                std::cerr << "Cliente inválido (campos obligatorios), se omite." << std::endl;
                continue;
            }

            // Crear cliente
            Cliente* cli = new Cliente(id.toStdString(), nombre.toStdString(),
                                       correo.toStdString(), telefono.toStdString(),
                                       password.toStdString(), tipo.toStdString());

            // Intentar insertar (falla si ID o correo ya existen)
            if (!clientes.insertar(cli)) {
                std::cerr << "Cliente " << id.toStdString()
                << " duplicado o correo ya usado, se omite." << std::endl;
                delete cli;
                continue;
            }
            contador++;

            // ----- Cargar reservas de este cliente -----
            if (obj.contains("reservas") && obj["reservas"].isArray()) {
                QJsonArray arrRes = obj["reservas"].toArray();
                for (const QJsonValue& valRes : arrRes) {
                    if (!valRes.isObject()) continue;
                    QJsonObject rObj = valRes.toObject();

                    QString codRes  = rObj["codigo_reserva"].toString().trimmed();
                    QString codFun  = rObj["codigo_funcion"].toString().trimmed();
                    int fila        = rObj["fila"].toInt(-1);
                    int columna     = rObj["columna"].toInt(-1);
                    QString fecha   = rObj["fecha_reserva"].toString().trimmed();

                    if (codRes.isEmpty() || codFun.isEmpty() || fila <= 0 || columna <= 0) {
                        std::cerr << "Reserva inválida del cliente " << id.toStdString()
                                  << ", se omite." << std::endl;
                        continue;
                    }

                    // Validar fecha
                    QDate fRes = QDate::fromString(fecha, "yyyy-MM-dd");
                    if (!fRes.isValid()) {
                        std::cerr << "Fecha inválida en reserva " << codRes.toStdString()
                                  << ", se omite." << std::endl;
                        continue;
                    }

                    Reserva* res = new Reserva(codRes.toStdString(),
                                               id.toStdString(),
                                               codFun.toStdString(),
                                               fila, columna,
                                               fecha.toStdString());

                    if (!reservas.insertar(res)) {
                        std::cerr << "Reserva " << codRes.toStdString()
                        << " duplicada, se omite." << std::endl;
                        delete res;
                        continue;
                    }

                    // Agregar la referencia al cliente
                    cli->agregarCodigoReserva(codRes.toStdString());
                }
            }
        }
        return contador;
    }
};

#endif // CARGADORJSON_H