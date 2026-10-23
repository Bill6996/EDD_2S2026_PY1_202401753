#ifndef LZW_H
#define LZW_H

#include <string>
#include <cstdint>

// ============================================================
// Compresión LZW (Lempel-Ziv-Welch) implementada manualmente.
//
// IDEA: el diccionario empieza con las 256 secuencias de un byte (código = byte).
// Al comprimir se busca la secuencia más larga que ya esté en el diccionario,
// se emite su código y se agrega al diccionario "secuencia + siguiente byte".
// Al descomprimir se reconstruye el MISMO diccionario sin necesidad de guardarlo.
//
// Formato del archivo .edd:
//   [4 bytes]  tamaño original del texto (little-endian)  -> sirve para validar al descomprimir
//   [4 bytes]  cantidad de códigos que siguen (little-endian)
//   [bits]     los códigos LZW empaquetados con ANCHO VARIABLE: el k-ésimo código usa
//              max(9, bits de (255 + k)) bits (9 bits al inicio, hasta 16 bits cuando el
//              diccionario crece). Así los archivos pequeños también se reducen de tamaño.
//
// El diccionario se limita a 65536 entradas (códigos de 16 bits). Cuando se llena
// deja de crecer; compresor y descompresor aplican la misma regla, así que siguen
// sincronizados.
//
// Las estructuras auxiliares (diccionario con hash y arreglo de entradas) son
// propias; no se usa std::map ni std::unordered_map.
// ============================================================
class LZW {
private:
    static const int MAX_CODIGOS = 65536;     // 2^16
    static const int TAM_HASH = 131071;       // primo > 2 * MAX_CODIGOS

    // ----- Diccionario del COMPRESOR: (código_prefijo, byte) -> código nuevo -----
    // Tabla hash con direccionamiento abierto (sondeo lineal).
    struct DiccionarioCompresion {
        int32_t* clavePrefijo;   // código del prefijo (-1 = casilla vacía)
        int32_t* claveByte;      // byte que se agrega al prefijo
        int32_t* valor;          // código asignado a (prefijo, byte)
        int siguienteCodigo;

        DiccionarioCompresion() : siguienteCodigo(256) {
            clavePrefijo = new int32_t[TAM_HASH];
            claveByte = new int32_t[TAM_HASH];
            valor = new int32_t[TAM_HASH];
            for (int i = 0; i < TAM_HASH; i++) { clavePrefijo[i] = -1; claveByte[i] = 0; valor[i] = 0; }
        }
        ~DiccionarioCompresion() { delete[] clavePrefijo; delete[] claveByte; delete[] valor; }

        static int indice(int32_t prefijo, int32_t b) {
            uint64_t h = (uint64_t)prefijo * 257u + (uint64_t)b * 31u + 7u;
            return (int)(h % TAM_HASH);
        }

        // Devuelve el código de (prefijo, byte) o -1 si no existe
        int buscar(int32_t prefijo, int32_t b) const {
            int i = indice(prefijo, b);
            while (clavePrefijo[i] != -1) {
                if (clavePrefijo[i] == prefijo && claveByte[i] == b) return valor[i];
                i = (i + 1) % TAM_HASH;
            }
            return -1;
        }

        // Agrega (prefijo, byte) con el siguiente código libre (si aún hay espacio)
        void agregar(int32_t prefijo, int32_t b) {
            if (siguienteCodigo >= MAX_CODIGOS) return;
            int i = indice(prefijo, b);
            while (clavePrefijo[i] != -1) i = (i + 1) % TAM_HASH;
            clavePrefijo[i] = prefijo;
            claveByte[i] = b;
            valor[i] = siguienteCodigo++;
        }
    };

    // Ancho (en bits) del k-ésimo código. Cuando se emite el código k el diccionario del
    // compresor tiene 256 + k entradas, o sea que el mayor código posible es 255 + k.
    static int anchoCodigo(uint64_t k) {
        uint64_t mayor = 255 + k;
        if (mayor > 65535) mayor = 65535;
        int bits = 9;
        while (((uint64_t)1 << bits) <= mayor) bits++;
        return bits;
    }

    // Escritor de bits (empaqueta de menos a más significativo)
    struct EscritorBits {
        std::string* salida;
        uint64_t acumulado;
        int nbits;
        EscritorBits(std::string* s) : salida(s), acumulado(0), nbits(0) {}
        void escribir(uint32_t valor, int ancho) {
            acumulado |= ((uint64_t)valor << nbits);
            nbits += ancho;
            while (nbits >= 8) {
                salida->push_back((char)(acumulado & 0xff));
                acumulado >>= 8;
                nbits -= 8;
            }
        }
        void terminar() {
            if (nbits > 0) { salida->push_back((char)(acumulado & 0xff)); acumulado = 0; nbits = 0; }
        }
    };

    // Lector de bits (inverso del escritor)
    struct LectorBits {
        const std::string* datos;
        size_t pos;
        uint64_t acumulado;
        int nbits;
        LectorBits(const std::string* d, size_t inicio) : datos(d), pos(inicio), acumulado(0), nbits(0) {}
        bool leer(int ancho, uint32_t& valor) {
            while (nbits < ancho) {
                if (pos >= datos->size()) return false;
                acumulado |= ((uint64_t)(unsigned char)(*datos)[pos++]) << nbits;
                nbits += 8;
            }
            valor = (uint32_t)(acumulado & (((uint64_t)1 << ancho) - 1));
            acumulado >>= ancho;
            nbits -= ancho;
            return true;
        }
    };

public:
    // ---------------- COMPRESIÓN ----------------
    // Recibe el texto serializado y devuelve los bytes comprimidos.
    static std::string comprimir(const std::string& entrada) {
        std::string salida;

        // Cabecera: tamaño original y (más abajo) cantidad de códigos
        uint32_t n = (uint32_t)entrada.size();
        for (int i = 0; i < 4; i++) salida.push_back((char)((n >> (8 * i)) & 0xff));
        for (int i = 0; i < 4; i++) salida.push_back(0);          // se rellena al final
        if (entrada.empty()) return salida;

        DiccionarioCompresion dic;
        EscritorBits bits(&salida);
        uint32_t totalCodigos = 0;
        int32_t actual = (unsigned char)entrada[0];   // secuencia actual representada por su código

        for (size_t i = 1; i < entrada.size(); i++) {
            int32_t b = (unsigned char)entrada[i];
            int encontrado = dic.buscar(actual, b);
            if (encontrado != -1) {
                actual = encontrado;                  // la secuencia + b ya existe: seguir creciendo
            } else {
                bits.escribir((uint32_t)actual, anchoCodigo(totalCodigos));   // emitir el código de la secuencia actual
                totalCodigos++;
                dic.agregar(actual, b);               // nueva secuencia = actual + b
                actual = b;
            }
        }
        // Emitir la última secuencia pendiente
        bits.escribir((uint32_t)actual, anchoCodigo(totalCodigos));
        totalCodigos++;
        bits.terminar();

        for (int i = 0; i < 4; i++) salida[4 + i] = (char)((totalCodigos >> (8 * i)) & 0xff);
        return salida;
    }

    // ---------------- DESCOMPRESIÓN ----------------
    // Recibe los bytes comprimidos y reconstruye el texto original.
    // Devuelve false si los datos están corruptos.
    static bool descomprimir(const std::string& comprimido, std::string& resultado) {
        resultado.clear();
        if (comprimido.size() < 8) return false;

        uint32_t esperado = 0, totalCodigos = 0;
        for (int i = 0; i < 4; i++) esperado |= ((uint32_t)(unsigned char)comprimido[i]) << (8 * i);
        for (int i = 0; i < 4; i++) totalCodigos |= ((uint32_t)(unsigned char)comprimido[4 + i]) << (8 * i);
        if (esperado == 0) return totalCodigos == 0;
        if (totalCodigos == 0) return false;

        // Diccionario del descompresor: cada entrada = (código_prefijo, último_byte)
        // La cadena completa de un código se reconstruye siguiendo los prefijos hacia atrás.
        int32_t* prefijo = new int32_t[MAX_CODIGOS];
        unsigned char* ultimo = new unsigned char[MAX_CODIGOS];
        int siguiente = 256;
        for (int i = 0; i < 256; i++) { prefijo[i] = -1; ultimo[i] = (unsigned char)i; }

        LectorBits lector(&comprimido, 8);
        bool ok = true;
        int32_t anterior = -1;
        std::string tmp;

        // Obtiene la cadena de un código (se arma al revés y luego se invierte)
        auto cadenaDe = [&](int32_t codigo, std::string& out) {
            out.clear();
            while (codigo != -1) {
                out.push_back((char)ultimo[codigo]);
                codigo = prefijo[codigo];
            }
            for (size_t a = 0, b = out.size(); a + 1 < b; a++, b--) {
                char t = out[a]; out[a] = out[b - 1]; out[b - 1] = t;
            }
        };

        for (uint32_t k = 0; k < totalCodigos && ok; k++) {
            uint32_t leido = 0;
            if (!lector.leer(anchoCodigo(k), leido)) { ok = false; break; }   // se acabaron los datos
            int32_t codigo = (int32_t)leido;

            if (anterior == -1) {
                if (codigo >= 256) { ok = false; break; }   // el primer código siempre es un byte
                cadenaDe(codigo, tmp);
                resultado += tmp;
            } else {
                if (codigo < siguiente) {
                    // El código ya existe en el diccionario
                    cadenaDe(codigo, tmp);
                    resultado += tmp;
                    if (siguiente < MAX_CODIGOS) {
                        prefijo[siguiente] = anterior;
                        ultimo[siguiente] = (unsigned char)tmp[0];
                        siguiente++;
                    }
                } else if (codigo == siguiente && siguiente < MAX_CODIGOS) {
                    // Caso especial KwKwK: el código aún no está, es anterior + primer byte de anterior
                    std::string previa;
                    cadenaDe(anterior, previa);
                    tmp = previa + previa[0];
                    resultado += tmp;
                    prefijo[siguiente] = anterior;
                    ultimo[siguiente] = (unsigned char)previa[0];
                    siguiente++;
                } else {
                    ok = false;                              // código imposible: archivo corrupto
                    break;
                }
            }
            anterior = codigo;
        }

        delete[] prefijo;
        delete[] ultimo;

        if (!ok || resultado.size() != esperado) { resultado.clear(); return false; }
        return true;
    }
};

#endif // LZW_H
