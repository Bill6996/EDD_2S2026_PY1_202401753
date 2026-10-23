# Manual Técnico — CinemaUSAC Fase 3

Proyecto de Estructuras de Datos (USAC). C++ con Qt Widgets. Todas las estructuras son propias (sin STL de contenedores ni librerías externas de estructuras/cripto/compresión). Qt/QJson se usa solo para la GUI y para interpretar JSON en memoria.

## 1. Estructura de archivos
| Archivo | Contenido |
|---|---|
| `Sede.h`, `GrafoSedes.h` | Grafo no dirigido ponderado (lista de adyacencia), BFS, ranking, sedes similares, reportes |
| `SHA256.h` | SHA-256 manual (FIPS 180-4) |
| `Transaccion.h`, `MerkleTree.h` | Transacción, lista enlazada de transacciones, Árbol de Merkle |
| `Blockchain.h` | Bloques, prueba de trabajo, verificación, reconstrucción de la Tabla Hash |
| `LZW.h`, `Persistencia.h` | Compresión LZW y guardado/carga de `sedes.edd`, `peliculas.edd`, `funciones.edd` |
| `GraphvizUtil.h` | Generación de PNG con `dot`/`neato` |
| `CargadorJSON.h` | Carga masiva (sedes, películas, funciones con `sede`) |
| Fase 2 | `BSTCartelera.h`, `AVLFunciones.h`, `ArbolBClientes.h`, `MatrizDispersa.h`, `TablaHashReservas.h`, listas |

## 2. Grafo de sedes
- **Vértice** = sede. **Arista** entre dos sedes = existen películas distintas que se proyectan en ambas. **Peso** = cantidad de películas distintas en común.
- Estructura: lista enlazada de vértices; cada vértice tiene su lista enlazada de aristas (lista de adyacencia) y una lista de códigos de películas.
- Las aristas **no se guardan**: se recalculan (`recalcularAristas`) cada vez que cambia una función o sede, y al cargar los `.edd`. Para cada par de sedes se intersectan sus catálogos de películas: O(S² · P).
- **BFS** (`caminoMasCorto`): cola manual, arreglo de visitados y de padres; devuelve el camino con menos saltos.
- **Ranking**: se arma una lista de pares (sedeA, sedeB, peso) y se ordena por inserción descendente por peso.
- **Sedes similares**: aristas del vértice ordenadas por peso.
- Reportes: `Grafo_Sedes.png` (neato) y `Lista_Adyacencia.png` (dot).

## 3. Blockchain
- Lista enlazada simple insertando al frente (cabeza = último bloque).
- Campos: `INDEX`, `TIMESTAMP` (`dd-MM-yyyy::HH:mm:ss`), `NONCE`, `DATA` (transacciones), `ROOTMERKLE`, `PREVIOUSHASH`, `HASH`.
- `HASH = SHA256(INDEX + TIMESTAMP + NONCE + ROOTMERKLE + PREVIOUSHASH)` concatenado sin espacios.
- **Prueba de trabajo**: se incrementa `NONCE` desde 0 hasta que el hash empiece con `0000`.
- El bloque génesis usa `PREVIOUSHASH = "0000"`.
- Cada bloque se guarda en `blocks/blockN.json` junto al ejecutable.
- Cada transacción tiene un `estado`: `PAGADA` o `CANCELADA`.
- Un bloque se genera automáticamente al juntar 4 transacciones pendientes, manualmente desde el panel admin, o al cerrar la aplicación si quedan pendientes.

### Árbol de Merkle
- Hoja = `SHA256(transacción serializada)`; serialización: campos separados por `|`, monto con `'g'` y 17 dígitos.
- Nodo interno = `SHA256(hashIzq + hashDer)`.
- Nivel impar: se duplica el último nodo. Con una transacción, la raíz es el hash de la hoja.

### Cancelaciones
Los bloques nunca se modifican. Si se cancela una compra que todavía está en la cola de pendientes, simplemente se saca de la cola. Si la compra ya está escrita en un bloque, **la misma transacción se escribe de nuevo con estado `CANCELADA`** en un bloque posterior; la escritura más reciente actualiza a la anterior (la Tabla Hash elimina la reserva, y las ventas totales restan el boleto y el monto).

### Verificación al iniciar
Para cada bloque se comprueba: (1) raíz de Merkle recalculada = `ROOTMERKLE`, (2) hash recalculado = `HASH`, (3) `HASH` empieza con `0000`, (4) `PREVIOUSHASH` = hash del bloque anterior. Si falla algo el bloque se marca **rojo** (manipulado) y se avisa al abrir; los íntegros se marcan **verde**. Todo lo que está **después** del primer bloque manipulado también se descarta (rojo); si se genera un bloque nuevo, la cadena continúa desde el último bloque íntegro anterior.

### Tabla Hash
Al iniciar se reconstruye la Tabla Hash de reservas recorriendo **solo los bloques íntegros**: cada transacción contiene los datos necesarios para recrear la reserva.

## 4. Persistencia con LZW
- Al cerrar: se recorre cada estructura en memoria (grafo, BST en preorden, AVL en preorden), se construye el texto en memoria y se comprime con LZW → `sedes.edd`, `peliculas.edd`, `funciones.edd`. No hay JSON intermedio.
- Al abrir: se leen los `.edd`, se descomprimen en memoria y se reinsertan los elementos.
- **LZW**: el diccionario inicia con los 256 bytes. Se busca la secuencia más larga conocida, se emite su código y se agrega secuencia+siguiente byte. El descompresor reconstruye el mismo diccionario sin guardarlo (incluye el caso especial KwKwK).
- Formato: 4 bytes tamaño original + 4 bytes cantidad de códigos + códigos con ancho variable (9 a 16 bits). Diccionario máx. 65536 entradas. El diccionario del compresor es una tabla hash propia con sondeo lineal.

## 5. Carga masiva
JSON con `sedes`, `peliculas` y `funciones`. Primero se cargan las sedes; cada función debe traer `sede` existente, si no se **rechaza** (también si el código está duplicado o la fecha es inválida). Al final se recalcula el grafo y se muestra el resumen con los errores.

## 6. Decisiones de diseño (confirmadas con el auxiliar)
- `DATA` de un bloque contiene solo las transacciones nuevas (el auxiliar aceptó este enfoque o el historial completo).
- Se genera un bloque cada 4 transacciones, manualmente por el administrador y al cerrar el programa si quedan pendientes.
- Cancelar una compra ya escrita en un bloque = escribir la misma transacción con estado `CANCELADA` en un bloque nuevo.
- Después de un bloque manipulado se descarta todo lo posterior; la cadena nueva continúa desde el último válido.
- Precio fijo por boleto: Q45.
- Bloques y `.edd` se guardan junto al ejecutable.

## 7. Complejidades
| Operación | Costo |
|---|---|
| Insertar sede | O(S) (verifica duplicado) |
| Recalcular aristas | O(S² · P) |
| BFS | O(S + A) |
| Merkle | O(n) hashes |
| Minado | Esperado ~65 000 intentos de hash (4 ceros hex) |
| LZW | O(n) promedio |
| Verificar cadena | O(B · T) |

## 8. Requisitos
Qt 5/6 Widgets, compilador C++11 o superior, Graphviz (`dot` y `neato` en el PATH).
