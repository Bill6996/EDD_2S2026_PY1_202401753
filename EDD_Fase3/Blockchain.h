#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "Transaccion.h"
#include "MerkleTree.h"
#include "SHA256.h"
#include "GraphvizUtil.h"
#include "TablaHashReservas.h"
#include "Reserva.h"

#include <string>
#include <sstream>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDate>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ============================================================
// BLOQUE
//
//   INDEX         número secuencial (el génesis es 0)
//   TIMESTAMP     "dd-MM-yyyy::HH:mm:ss"
//   NONCE         número que se itera hasta que el HASH empiece con "0000" (prueba de trabajo)
//   DATA          transacciones de compra incluidas en el bloque
//   ROOTMERKLE    raíz del árbol de Merkle construido sobre DATA
//   PREVIOUSHASH  HASH del bloque anterior ("0000" en el génesis)
//   HASH          SHA256(INDEX + TIMESTAMP + NONCE + ROOTMERKLE + PREVIOUSHASH)  (concatenado sin espacios)
// ============================================================
struct Bloque {
    int index;
    std::string timestamp;
    long long nonce;
    ListaTransacciones data;
    std::string rootMerkle;
    std::string previousHash;
    std::string hash;

    bool valido;               // resultado de la verificación de integridad (verde / rojo)
    std::string motivo;        // por qué se marcó como manipulado ("" si es válido)
    Bloque* anterior;          // lista enlazada: apunta al bloque MÁS ANTIGUO (se inserta por delante)

    Bloque() : index(0), nonce(0), valido(true), anterior(nullptr) {}

    // HASH = SHA256(INDEX + TIMESTAMP + NONCE + ROOTMERKLE + PREVIOUSHASH)
    std::string calcularHash() const {
        return SHA256::hash(std::to_string(index) + timestamp + std::to_string(nonce) +
                            rootMerkle + previousHash);
    }

    // Raíz de Merkle calculada a partir del DATA actual
    std::string calcularRaizMerkle() const {
        ArbolMerkle arbol;
        arbol.construir(data);
        return arbol.hashRaiz();
    }
};

// ----- Resumen de ventas (listas manuales clave -> boletos / monto) -----
struct NodoVenta {
    std::string clave;
    int boletos;
    double monto;
    NodoVenta* siguiente;
    NodoVenta(const std::string& c) : clave(c), boletos(0), monto(0.0), siguiente(nullptr) {}
};

struct ResumenVentas {
    int totalBoletos;
    double totalMonto;
    NodoVenta* porFuncion;
    NodoVenta* porFecha;

    ResumenVentas() : totalBoletos(0), totalMonto(0.0), porFuncion(nullptr), porFecha(nullptr) {}
    ~ResumenVentas() { liberar(porFuncion); liberar(porFecha); }
    ResumenVentas(const ResumenVentas&) = delete;
    ResumenVentas& operator=(const ResumenVentas&) = delete;

    // Una transacción CANCELADA anula a la PAGADA que escribió antes (resta boleto y monto)
    void acumular(const Transaccion& t) {
        int signo = (t.estado == "CANCELADA") ? -1 : 1;
        totalBoletos += signo;
        totalMonto += signo * t.monto;
        sumar(porFuncion, t.codigo_funcion, signo, signo * t.monto);
        sumar(porFecha, t.fecha_pago, signo, signo * t.monto);
    }

private:
    static void sumar(NodoVenta*& lista, const std::string& clave, int boletos, double monto) {
        NodoVenta* n = lista;
        NodoVenta* ultimo = nullptr;
        while (n != nullptr && n->clave != clave) { ultimo = n; n = n->siguiente; }
        if (n == nullptr) {
            n = new NodoVenta(clave);
            if (ultimo == nullptr) lista = n; else ultimo->siguiente = n;
        }
        n->boletos += boletos;
        n->monto += monto;
    }
    static void liberar(NodoVenta* n) {
        while (n != nullptr) { NodoVenta* t = n; n = n->siguiente; delete t; }
    }
};

// ============================================================
// BLOCKCHAIN
//
// Lista enlazada simple de bloques que se insertan por delante:
//
//   cabeza -> [Bloque 3] -> [Bloque 2] -> [Bloque 1] -> [Bloque 0] -> null
//
// Además mantiene:
//   * una COLA de transacciones pendientes (compras que aún no están en un bloque);
//   * los archivos blocks/blockN.json (persistencia);
//   * la verificación de integridad de cada bloque al iniciar.
// ============================================================
class Blockchain {
private:
    Bloque* cabeza;               // bloque más reciente leído (puede estar manipulado)
    Bloque* ultimoValido;         // último bloque NO manipulado (a partir de él se continúa la cadena)
    int cantidad;                 // bloques en memoria
    ListaTransacciones pendientes;    // cola de transacciones aún sin bloque
    int tamanoAutomatico;         // cada cuántas transacciones se genera un bloque solo

    // ---------- Archivos ----------
    QString carpetaBloques() const {
        QString ruta = QCoreApplication::applicationDirPath() + "/blocks";
        QDir().mkpath(ruta);
        return ruta;
    }
    QString rutaBloque(int index) const {
        return carpetaBloques() + "/block" + QString::number(index) + ".json";
    }

    static std::string jsonEsc(const std::string& t) {
        std::string r;
        for (char c : t) {
            if (c == '"') r += "\\\"";
            else if (c == '\\') r += "\\\\";
            else if (c == '\n') r += "\\n";
            else r += c;
        }
        return r;
    }

    // Escribe un bloque con el mismo formato del enunciado (INDEX, TIMESTAMP, NONCE, DATA, ...)
    bool guardarBloque(const Bloque* b) const {
        std::ostringstream js;
        js << "{\n";
        js << "  \"INDEX\": " << b->index << ",\n";
        js << "  \"TIMESTAMP\": \"" << jsonEsc(b->timestamp) << "\",\n";
        js << "  \"NONCE\": " << b->nonce << ",\n";
        js << "  \"DATA\": [\n";
        int i = 0;
        int total = b->data.getCantidad();
        b->data.recorrer([&](const Transaccion& t) {
            js << "    {\n";
            js << "      \"codigo_transaccion\": \"" << jsonEsc(t.codigo_transaccion) << "\",\n";
            js << "      \"codigo_reserva\": \"" << jsonEsc(t.codigo_reserva) << "\",\n";
            js << "      \"id_cliente\": \"" << jsonEsc(t.id_cliente) << "\",\n";
            js << "      \"codigo_funcion\": \"" << jsonEsc(t.codigo_funcion) << "\",\n";
            js << "      \"fila\": " << t.fila << ",\n";
            js << "      \"columna\": " << t.columna << ",\n";
            js << "      \"monto\": " << QString::number(t.monto, 'f', 2).toStdString() << ",\n";
            js << "      \"metodo_pago\": \"" << jsonEsc(t.metodo_pago) << "\",\n";
            js << "      \"fecha_pago\": \"" << jsonEsc(t.fecha_pago) << "\",\n";
            js << "      \"estado\": \"" << jsonEsc(t.estado) << "\"\n";
            js << "    }" << ((++i < total) ? "," : "") << "\n";
        });
        js << "  ],\n";
        js << "  \"ROOTMERKLE\": \"" << b->rootMerkle << "\",\n";
        js << "  \"PREVIOUSHASH\": \"" << b->previousHash << "\",\n";
        js << "  \"HASH\": \"" << b->hash << "\"\n";
        js << "}\n";

        QFile f(rutaBloque(b->index));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        std::string texto = js.str();
        f.write(texto.data(), (qint64)texto.size());
        f.close();
        return true;
    }

    // Lee un bloque desde su archivo. Si el JSON está dañado devuelve un bloque marcado como manipulado.
    Bloque* leerBloque(int index) const {
        Bloque* b = new Bloque();
        b->index = index;

        QFile f(rutaBloque(index));
        if (!f.open(QIODevice::ReadOnly)) {
            b->valido = false;
            b->motivo = "No se pudo abrir el archivo";
            return b;
        }
        QByteArray bytes = f.readAll();
        f.close();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            b->valido = false;
            b->motivo = "JSON ilegible o dañado";
            return b;
        }
        QJsonObject o = doc.object();
        if (!o.contains("INDEX") || !o.contains("TIMESTAMP") || !o.contains("NONCE") || !o.contains("DATA") ||
            !o.contains("ROOTMERKLE") || !o.contains("PREVIOUSHASH") || !o.contains("HASH") ||
            !o["DATA"].isArray()) {
            b->valido = false;
            b->motivo = "Faltan campos obligatorios";
            return b;
        }

        b->index = o["INDEX"].toInt(-1);
        b->timestamp = o["TIMESTAMP"].toString().toStdString();
        b->nonce = (long long)o["NONCE"].toDouble(-1);
        b->rootMerkle = o["ROOTMERKLE"].toString().toStdString();
        b->previousHash = o["PREVIOUSHASH"].toString().toStdString();
        b->hash = o["HASH"].toString().toStdString();

        QJsonArray datos = o["DATA"].toArray();
        for (const QJsonValue& v : datos) {
            QJsonObject t = v.toObject();
            Transaccion tr;
            tr.codigo_transaccion = t["codigo_transaccion"].toString().toStdString();
            tr.codigo_reserva = t["codigo_reserva"].toString().toStdString();
            tr.id_cliente = t["id_cliente"].toString().toStdString();
            tr.codigo_funcion = t["codigo_funcion"].toString().toStdString();
            tr.fila = t["fila"].toInt();
            tr.columna = t["columna"].toInt();
            tr.monto = t["monto"].toDouble();
            tr.metodo_pago = t["metodo_pago"].toString().toStdString();
            tr.fecha_pago = t["fecha_pago"].toString().toStdString();
            tr.estado = t["estado"].toString("PAGADA").toStdString();
            if (tr.estado.empty()) tr.estado = "PAGADA";
            b->data.agregar(tr);
        }
        return b;
    }

    // ---------- Verificación de integridad ----------
    // Un bloque es VÁLIDO (verde) solo si TODO coincide; si algo fue editado en el JSON es MANIPULADO (rojo).
    // "hayPrevio": el bloque no es el génesis. Si el bloque anterior era ilegible (hashPrevio vacío)
    // no se puede comprobar el enlace, así que no se culpa a este bloque por eso.
    void verificarBloque(Bloque* b, bool hayPrevio, const std::string& hashPrevio) const {
        if (!b->motivo.empty()) { b->valido = false; return; }   // ya venía dañado de la lectura

        b->valido = true;
        b->motivo = "";

        if (b->calcularRaizMerkle() != b->rootMerkle) {
            b->valido = false;
            b->motivo = "ROOTMERKLE no coincide con las transacciones (DATA modificado)";
        } else if (b->calcularHash() != b->hash) {
            b->valido = false;
            b->motivo = "HASH no coincide con el contenido del bloque";
        } else if (b->hash.compare(0, 4, "0000") != 0) {
            b->valido = false;
            b->motivo = "El HASH no cumple la prueba de trabajo (prefijo 0000)";
        } else if (!(hayPrevio && hashPrevio.empty()) &&
                   b->previousHash != (hayPrevio ? hashPrevio : std::string("0000"))) {
            b->valido = false;
            b->motivo = "PREVIOUSHASH no coincide con el bloque anterior";
        }
    }

    void liberarTodo() {
        Bloque* b = cabeza;
        while (b != nullptr) { Bloque* t = b; b = b->anterior; delete t; }
        cabeza = nullptr;
        ultimoValido = nullptr;
        cantidad = 0;
    }

    // Después de leer/modificar la cadena: ¿cuál es el bloque más reciente no manipulado?
    void recalcularUltimoValido() {
        ultimoValido = nullptr;
        for (Bloque* b = cabeza; b != nullptr; b = b->anterior) {
            if (b->valido) { ultimoValido = b; break; }
        }
    }

    // Descarta (y borra sus archivos) los bloques manipulados que están por encima del último válido.
    // Es necesario para poder continuar la cadena desde el último bloque íntegro.
    void descartarBloquesPosterioresAlUltimoValido() {
        while (cabeza != nullptr && cabeza != ultimoValido) {
            Bloque* t = cabeza;
            cabeza = cabeza->anterior;
            QFile::remove(rutaBloque(t->index));
            delete t;
            cantidad--;
        }
    }

    static std::string abreviar(const std::string& h, size_t n = 12) {
        return h.size() > n ? h.substr(0, n) + "..." : h;
    }

public:
    Blockchain() : cabeza(nullptr), ultimoValido(nullptr), cantidad(0), tamanoAutomatico(4) {}
    ~Blockchain() { liberarTodo(); }
    Blockchain(const Blockchain&) = delete;
    Blockchain& operator=(const Blockchain&) = delete;

    // =========================================================
    // CARGA AL INICIAR: lee blocks/block0.json, block1.json, ... y verifica cada uno
    // =========================================================
    // Devuelve cuántos bloques se leyeron. Cada bloque queda marcado como válido/manipulado.
    int cargarDesdeDisco() {
        liberarTodo();
        std::string hashAnterior;
        bool cadenaRota = false;     // true desde el primer bloque manipulado en adelante
        for (int n = 0; QFile::exists(rutaBloque(n)); n++) {
            Bloque* b = leerBloque(n);
            verificarBloque(b, n > 0, hashAnterior);
            hashAnterior = b->hash;

            // Todo lo que está DESPUÉS de un bloque manipulado se descarta (ya no se puede confiar en la cadena)
            if (cadenaRota && b->valido) {
                b->valido = false;
                b->motivo = "Posterior a un bloque manipulado (se descarta)";
            }
            if (!b->valido) cadenaRota = true;

            b->anterior = cabeza;      // inserción por delante
            cabeza = b;
            cantidad++;
        }
        recalcularUltimoValido();
        return cantidad;
    }

    int cantidadBloques() const { return cantidad; }

    int cantidadManipulados() const {
        int m = 0;
        for (Bloque* b = cabeza; b != nullptr; b = b->anterior) if (!b->valido) m++;
        return m;
    }

    Bloque* getCabeza() const { return cabeza; }
    Bloque* getUltimoValido() const { return ultimoValido; }

    Bloque* buscarBloque(int index) const {
        for (Bloque* b = cabeza; b != nullptr; b = b->anterior)
            if (b->index == index) return b;
        return nullptr;
    }

    // =========================================================
    // TRANSACCIONES PENDIENTES Y GENERACIÓN DE BLOQUES
    // =========================================================
    int cantidadPendientes() const { return pendientes.getCantidad(); }
    int getTamanoAutomatico() const { return tamanoAutomatico; }
    const ListaTransacciones& getPendientes() const { return pendientes; }

    // Genera "T001", "T002", ... continuando desde la mayor existente (cadena + pendientes)
    std::string siguienteCodigoTransaccion() const {
        int mayor = 0;
        auto revisar = [&](const Transaccion& t) {
            if (t.codigo_transaccion.size() > 1 && t.codigo_transaccion[0] == 'T') {
                int n = atoi(t.codigo_transaccion.c_str() + 1);
                if (n > mayor) mayor = n;
            }
        };
        for (Bloque* b = cabeza; b != nullptr; b = b->anterior) b->data.recorrer(revisar);
        pendientes.recorrer(revisar);

        std::string num = std::to_string(mayor + 1);
        while (num.size() < 3) num = "0" + num;
        return "T" + num;
    }

    // Agrega una compra a la cola. Si ya hay 4 pendientes, genera un bloque automáticamente.
    // "bloqueAutomatico" apunta al bloque creado (o nullptr si solo se encoló).
    void encolarTransaccion(const Transaccion& t, Bloque*& bloqueAutomatico) {
        bloqueAutomatico = nullptr;
        pendientes.agregar(t);
        if (pendientes.getCantidad() >= tamanoAutomatico) {
            std::string error;
            bloqueAutomatico = generarBloque(error);
        }
    }

    // Genera un bloque con TODAS las transacciones pendientes:
    //   1. DATA = cola de pendientes        2. ROOTMERKLE = raíz del árbol de Merkle de DATA
    //   3. PREVIOUSHASH = hash del último bloque íntegro ("0000" si es el génesis)
    //   4. Prueba de trabajo: NONCE = 0,1,2,... hasta que el HASH empiece con "0000"
    //   5. Se guarda en blocks/blockN.json y se inserta por delante en la lista
    // Devuelve nullptr (y llena "error") si no hay nada que registrar.
    Bloque* generarBloque(std::string& error) {
        error.clear();
        if (pendientes.estaVacia()) {
            error = "No hay transacciones pendientes para incluir en un bloque.";
            return nullptr;
        }

        // Si había bloques manipulados al final de la cadena, se descartan para continuar desde el último íntegro
        descartarBloquesPosterioresAlUltimoValido();

        Bloque* nuevo = new Bloque();
        nuevo->index = ultimoValido ? ultimoValido->index + 1 : 0;
        nuevo->timestamp = QDateTime::currentDateTime().toString("dd-MM-yyyy::HH:mm:ss").toStdString();
        nuevo->data.trasladarDesde(pendientes);                 // la cola queda vacía
        nuevo->rootMerkle = nuevo->calcularRaizMerkle();
        nuevo->previousHash = ultimoValido ? ultimoValido->hash : "0000";

        // PRUEBA DE TRABAJO
        nuevo->nonce = 0;
        while (true) {
            nuevo->hash = nuevo->calcularHash();
            if (nuevo->hash.compare(0, 4, "0000") == 0) break;
            nuevo->nonce++;
        }
        nuevo->valido = true;

        if (!guardarBloque(nuevo)) error = "No se pudo escribir " + rutaBloque(nuevo->index).toStdString();

        nuevo->anterior = cabeza;
        cabeza = nuevo;
        ultimoValido = nuevo;
        cantidad++;
        return nuevo;
    }

    // ¿La reserva tiene una compra PAGADA escrita en un bloque íntegro (y no cancelada después)?
    // Si es así devuelve true y deja en "ultima" la última transacción de esa reserva.
    bool reservaPagadaEnBloque(const std::string& codigoReserva, Transaccion& ultima) const {
        bool encontrada = false;
        recorrerTransaccionesValidas([&](const Transaccion& t, const Bloque*) {
            if (t.codigo_reserva == codigoReserva) { ultima = t; encontrada = true; }   // gana la más reciente
        });
        return encontrada && ultima.estado == "PAGADA";
    }

    // Cancelar una compra que aún está en la cola (no entró a ningún bloque)
    bool cancelarPendiente(const std::string& codigoReserva) {
        return pendientes.eliminarPorReserva(codigoReserva);
    }

    // =========================================================
    // CONSULTAS (solo sobre bloques íntegros)
    // =========================================================

    // Recorre las transacciones de los bloques válidos desde el génesis hacia el más reciente.
    // La lambda recibe (const Transaccion&, const Bloque*).
    template <typename F>
    void recorrerTransaccionesValidas(F accion) const {
        // La lista va del más nuevo al más viejo; se arma un arreglo para recorrerla al revés
        Bloque** arreglo = new Bloque*[cantidad > 0 ? cantidad : 1];
        int n = 0;
        for (Bloque* b = cabeza; b != nullptr; b = b->anterior)
            if (b->valido) arreglo[n++] = b;
        for (int i = n - 1; i >= 0; i--) {
            Bloque* b = arreglo[i];
            b->data.recorrer([&](const Transaccion& t) { accion(t, b); });
        }
        delete[] arreglo;
    }

    // Busca una transacción por código en los bloques válidos. "bloque" queda en nullptr si estaba pendiente.
    bool buscarTransaccion(const std::string& codigo, Transaccion& resultado, const Bloque*& bloque) const {
        // Una transacción puede aparecer 2 veces (PAGADA y luego CANCELADA): la más reciente actualiza a la anterior
        Transaccion* p = pendientes.buscar(codigo);
        if (p != nullptr) { resultado = *p; bloque = nullptr; return true; }
        bool hallada = false;
        bloque = nullptr;
        recorrerTransaccionesValidas([&](const Transaccion& t, const Bloque* b) {
            if (t.codigo_transaccion == codigo) {
                resultado = t;
                bloque = b;
                hallada = true;
            }
        });
        return hallada;
    }

    // Totales de ventas: boletos, monto, desglose por función y por fecha
    void calcularVentas(ResumenVentas& resumen) const {
        recorrerTransaccionesValidas([&](const Transaccion& t, const Bloque*) { resumen.acumular(t); });
    }

    // Reconstruye la Tabla Hash de reservas usando ÚNICAMENTE el DATA de los bloques válidos
    int reconstruirTablaHash(TablaHashReservas& tabla) const {
        int insertadas = 0;
        recorrerTransaccionesValidas([&](const Transaccion& t, const Bloque*) {
            // Una transacción CANCELADA deshace la reserva que una PAGADA anterior creó
            if (t.estado == "CANCELADA") {
                if (tabla.buscar(t.codigo_reserva) != nullptr) { tabla.eliminar(t.codigo_reserva); insertadas--; }
                return;
            }
            // fecha_pago viene como dd-MM-yyyy; las reservas usan yyyy-MM-dd
            QDate fecha = QDate::fromString(QString::fromStdString(t.fecha_pago), "dd-MM-yyyy");
            std::string fechaReserva = fecha.isValid() ? fecha.toString("yyyy-MM-dd").toStdString() : t.fecha_pago;
            Reserva* r = new Reserva(t.codigo_reserva, t.id_cliente, t.codigo_funcion,
                                     t.fila, t.columna, fechaReserva);
            if (tabla.insertar(r)) insertadas++;
            else delete r;
        });
        return insertadas;
    }

    // =========================================================
    // REPORTES GRAPHVIZ
    // =========================================================

    // Cadena completa como lista enlazada: verde = íntegro, rojo = manipulado
    QString generarReporteBlockchain() const {
        std::ostringstream dot;
        dot << "digraph Blockchain {\n";
        dot << "  rankdir=LR;\n";
        dot << "  graph [label=\"Blockchain CinemaUSAC  |  Bloques: " << cantidad
            << "  |  Manipulados: " << cantidadManipulados()
            << "  |  Transacciones pendientes: " << pendientes.getCantidad()
            << "\", labelloc=t, fontname=\"Arial\", fontsize=16];\n";
        dot << "  node [shape=box, style=filled, fontname=\"Arial\", fontsize=11];\n";

        if (cabeza == nullptr) {
            dot << "  vacia [label=\"Blockchain vacía\\n(aún no hay bloques)\", fillcolor=lightgray];\n";
        }

        for (Bloque* b = cabeza; b != nullptr; b = b->anterior) {
            dot << "  b" << b->index << " [fillcolor=" << (b->valido ? "palegreen" : "tomato")
                << ", label=\"Bloque " << b->index
                << "\\nINDEX: " << b->index
                << "\\nNONCE: " << b->nonce
                << "\\nHASH: " << GraphvizUtil::esc(abreviar(b->hash))
                << "\\nPREV: " << GraphvizUtil::esc(abreviar(b->previousHash))
                << "\\nTransacciones: " << b->data.getCantidad()
                << (b->valido ? "\\nÍNTEGRO" : "\\nMANIPULADO")
                << "\", tooltip=\"" << GraphvizUtil::esc(b->motivo) << "\"];\n";
            if (b->anterior != nullptr)
                dot << "  b" << b->index << " -> b" << b->anterior->index << ";\n";
        }
        dot << "}\n";
        return GraphvizUtil::generarPng("Blockchain", dot.str());
    }

    // Árbol de Merkle de un bloque específico (cualquiera, no solo el último)
    QString generarReporteMerkle(int index, std::string& error) const {
        error.clear();
        Bloque* b = buscarBloque(index);
        if (b == nullptr) {
            error = "No existe el bloque " + std::to_string(index);
            return "";
        }
        ArbolMerkle arbol;
        arbol.construir(b->data);

        std::string titulo = "Árbol de Merkle - Bloque " + std::to_string(b->index) +
                             "\nROOTMERKLE calculado: " + abreviar(arbol.hashRaiz(), 24);
        if (!b->valido) titulo += "\n(BLOQUE MANIPULADO: " + b->motivo + ")";   // esc() convierte \n a salto de línea de dot
        QString png = GraphvizUtil::generarPng("Arbol_Merkle", arbol.generarDot(titulo));
        return png;
    }
};

#endif // BLOCKCHAIN_H
