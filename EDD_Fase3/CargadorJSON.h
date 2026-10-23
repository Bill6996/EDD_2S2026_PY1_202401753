#ifndef CARGADORJSON_H
#define CARGADORJSON_H

#include "BSTCartelera.h"
#include "AVLFunciones.h"
#include "ArbolBClientes.h"
#include "TablaHashReservas.h"
#include "MatrizDispersa.h"
#include "GrafoSedes.h"
#include "Cliente.h"
#include "Reserva.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QDir>
#include <QDate>
#include <string>
#include <iostream>

class CargadorJSON {
public:
    // ============================================================
    // 1. Cargar SEDES + PELÍCULAS + FUNCIONES  (Fase 3)
    // ============================================================
    // El JSON ahora trae primero el arreglo "sedes" (cada sede = un vértice del grafo)
    // y cada función incluye el campo "sede" con el código de una sede EXISTENTE.
    //  - Si el código de sede de una función no existe, la función completa se rechaza
    //    y se reporta el error (no se puede crear una relación con un vértice inexistente).
    //  - Al terminar se recalculan las aristas del grafo a partir de las funciones cargadas.
    struct ResultadoCarga {
        int sedes = 0;
        int peliculas = 0;
        int funciones = 0;
        int rechazadas = 0;          // funciones rechazadas
        QStringList errores;         // detalle de cada problema encontrado
    };

    static ResultadoCarga cargarPeliculasYFunciones(const std::string& ruta,
                                                    BSTCartelera& cartelera,
                                                    AVLFunciones& funciones,
                                                    GrafoSedes& grafo) {
        ResultadoCarga res;

        QFile file(QString::fromStdString(ruta));
        if (!file.open(QIODevice::ReadOnly)) {
            res.errores << "No se pudo abrir el archivo: " + QString::fromStdString(ruta);
            return res;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            res.errores << "JSON inválido";
            return res;
        }

        QJsonObject root = doc.object();

        // ----- 1) SEDES: cada una se inserta como un vértice nuevo (sin aristas todavía) -----
        if (root.contains("sedes") && root["sedes"].isArray()) {
            for (const QJsonValue& val : root["sedes"].toArray()) {
                if (!val.isObject()) continue;
                QJsonObject obj = val.toObject();

                QString codigo    = obj["codigo"].toString().trimmed();
                QString nombre    = obj["nombre"].toString().trimmed();
                QString direccion = obj["direccion"].toString().trimmed();
                int salas         = obj["cantidad_salas"].toInt(-1);
                QString telefono  = obj["telefono"].toString().trimmed();

                if (codigo.isEmpty() || nombre.isEmpty() || salas <= 0) {
                    res.errores << QString("Sede inválida (código, nombre o cantidad_salas faltante/incorrecto): '%1'").arg(codigo);
                    continue;
                }
                Sede s(codigo.toStdString(), nombre.toStdString(), direccion.toStdString(),
                       salas, telefono.toStdString());
                if (!grafo.agregarSede(s)) {
                    res.errores << QString("Sede %1 ya existe, se omite.").arg(codigo);
                    continue;
                }
                res.sedes++;
            }
        } else if (grafo.cantidadSedes() == 0) {
            res.errores << "El archivo no tiene el arreglo 'sedes' y no hay sedes registradas.";
        }

        // ----- 2) PELÍCULAS + FUNCIONES -----
        if (!root.contains("peliculas") || !root["peliculas"].isArray()) {
            res.errores << "No se encontró el array 'peliculas'";
            grafo.recalcularAristas(funciones);
            return res;
        }

        QJsonArray arrPeliculas = root["peliculas"].toArray();

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
                res.errores << QString("Película inválida (campos obligatorios faltantes): '%1'").arg(codigo);
                continue;
            }

            // Validar fechas
            QDate fechaEst = QDate::fromString(fEst, "yyyy-MM-dd");
            QDate fechaFin = QDate::fromString(fFin, "yyyy-MM-dd");
            if (!fechaEst.isValid() || !fechaFin.isValid()) {
                res.errores << QString("Película %1 tiene fechas inválidas, se omite.").arg(codigo);
                continue;
            }

            // Insertar en el BST (si ya existía, el BST ignora el duplicado)
            bool yaExistia = (cartelera.buscar(codigo.toStdString()) != nullptr);
            if (!yaExistia) {
                cartelera.insertar(codigo.toStdString(), titulo.toStdString(),
                                   genero.toStdString(), duracion,
                                   clasif.toStdString(), idioma.toStdString(),
                                   fEst.toStdString(), fFin.toStdString());
                res.peliculas++;
            }

            // ----- Cargar funciones de esta película (si existen) -----
            if (obj.contains("funciones") && obj["funciones"].isArray()) {
                QJsonArray arrFun = obj["funciones"].toArray();
                for (const QJsonValue& valFun : arrFun) {
                    if (!valFun.isObject()) continue;
                    QJsonObject fObj = valFun.toObject();

                    QString codFun  = fObj["codigo_funcion"].toString().trimmed();
                    QString horario = fObj["horario"].toString().trimmed();
                    QString sala    = fObj["sala"].toString().trimmed();
                    QString sede    = fObj["sede"].toString().trimmed();      // NUEVO Fase 3
                    int filas       = fObj["filas"].toInt(-1);
                    int columnas    = fObj["columnas"].toInt(-1);

                    if (codFun.isEmpty() || horario.isEmpty() || filas <= 0 || columnas <= 0) {
                        res.errores << QString("Función inválida dentro de %1 (%2), se omite.").arg(codigo, codFun);
                        res.rechazadas++;
                        continue;
                    }

                    // La sede es obligatoria y debe existir como vértice del grafo
                    if (sede.isEmpty() || !grafo.existe(sede.toStdString())) {
                        res.errores << QString("Función %1 RECHAZADA: la sede '%2' no existe.").arg(codFun, sede);
                        res.rechazadas++;
                        continue;
                    }

                    // Código de función repetido: no se sobreescribe (evita perder los asientos reservados)
                    if (funciones.buscar(codFun.toStdString()) != nullptr) {
                        res.errores << QString("Función %1 ya existe, se omite.").arg(codFun);
                        res.rechazadas++;
                        continue;
                    }

                    // Fecha de la función (si no viene en el JSON, usar una futura)
                    QString fechaFun = fObj["fecha"].toString().trimmed();
                    if (fechaFun.isEmpty())
                        fechaFun = "2026-12-31";
                    else if (!QDate::fromString(fechaFun, "yyyy-MM-dd").isValid()) {
                        res.errores << QString("Función %1 RECHAZADA: fecha inválida '%2'.").arg(codFun, fechaFun);
                        res.rechazadas++;
                        continue;
                    }

                    // archivo_asientos se genera al crear la función
                    std::string archivo = codFun.toStdString() + "_funcion.json";

                    funciones.insertar(codFun.toStdString(),
                                       codigo.toStdString(),
                                       horario.toStdString(),
                                       sala.toStdString(),
                                       filas, columnas,
                                       archivo,
                                       fechaFun.toStdString(),
                                       sede.toStdString());
                    res.funciones++;

                    // Crear el archivo JSON vacío de asientos
                    QString rutaAsientos = QCoreApplication::applicationDirPath()
                                           + "/asientos/" + QString::fromStdString(archivo);
                    QDir().mkpath(QCoreApplication::applicationDirPath() + "/asientos");
                    MatrizDispersa::crearArchivoVacio(codFun.toStdString(),
                                                      rutaAsientos.toStdString());
                }
            }
        }

        // ----- 3) Con todas las funciones cargadas se calculan las aristas del grafo -----
        grafo.recalcularAristas(funciones);

        for (const QString& e : res.errores)
            std::cerr << e.toStdString() << std::endl;
        return res;
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