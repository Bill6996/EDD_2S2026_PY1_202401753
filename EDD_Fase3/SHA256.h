#ifndef SHA256_H
#define SHA256_H

#include <string>
#include <cstdint>
#include <cstring>

// ============================================================
// SHA-256 implementado manualmente (FIPS 180-4), sin librerías externas.
//
// Pasos del algoritmo:
//   1. Padding: se agrega el bit 1, ceros, y la longitud original (64 bits).
//   2. El mensaje se procesa en bloques de 512 bits (64 bytes).
//   3. Cada bloque se expande a 64 palabras de 32 bits (mensaje "schedule").
//   4. 64 rondas de compresión actualizan los 8 registros de estado (a..h).
//   5. El resultado son los 8 registros concatenados (256 bits = 64 hex).
// ============================================================
class SHA256 {
private:
    // Constantes K: primeros 32 bits de la parte fraccionaria de las
    // raíces cúbicas de los primeros 64 números primos.
    static const uint32_t* K() {
        static const uint32_t k[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };
        return k;
    }

    // Rotación circular a la derecha de 32 bits
    static uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

    // Procesa un bloque de 64 bytes actualizando el estado h[8]
    static void procesarBloque(const uint8_t* bloque, uint32_t h[8]) {
        uint32_t w[64];

        // Primeras 16 palabras: el bloque en big-endian
        for (int i = 0; i < 16; i++) {
            w[i] = (uint32_t(bloque[i * 4]) << 24) | (uint32_t(bloque[i * 4 + 1]) << 16) |
                   (uint32_t(bloque[i * 4 + 2]) << 8) | uint32_t(bloque[i * 4 + 3]);
        }
        // Palabras 16..63: se derivan de las anteriores
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

        // 64 rondas de compresión
        for (int i = 0; i < 64; i++) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t t1 = hh + S1 + ch + K()[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + maj;

            hh = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

public:
    // Calcula SHA-256 de un texto y lo devuelve en hexadecimal (64 caracteres)
    static std::string hash(const std::string& mensaje) {
        // Valores iniciales: parte fraccionaria de las raíces cuadradas de los 8 primeros primos
        uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                         0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

        const uint8_t* datos = reinterpret_cast<const uint8_t*>(mensaje.data());
        uint64_t longitud = mensaje.size();

        // Se procesan los bloques completos de 64 bytes
        uint64_t completos = longitud / 64;
        for (uint64_t i = 0; i < completos; i++)
            procesarBloque(datos + i * 64, h);

        // Padding del último bloque (o de los dos últimos si no cabe la longitud)
        uint8_t resto[128];
        std::memset(resto, 0, sizeof(resto));
        uint64_t sobrante = longitud % 64;
        if (sobrante > 0) std::memcpy(resto, datos + completos * 64, sobrante);
        resto[sobrante] = 0x80;

        uint64_t totalResto = (sobrante < 56) ? 64 : 128;
        uint64_t bits = longitud * 8;
        for (int i = 0; i < 8; i++)
            resto[totalResto - 1 - i] = static_cast<uint8_t>((bits >> (8 * i)) & 0xff);

        procesarBloque(resto, h);
        if (totalResto == 128) procesarBloque(resto + 64, h);

        // Convertir los 8 registros a hexadecimal
        static const char* hex = "0123456789abcdef";
        std::string salida;
        salida.reserve(64);
        for (int i = 0; i < 8; i++) {
            for (int j = 7; j >= 0; j--) {
                salida.push_back(hex[(h[i] >> (j * 4)) & 0xf]);
            }
        }
        return salida;
    }
};

#endif // SHA256_H
