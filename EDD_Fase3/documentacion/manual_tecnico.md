# Manual Técnico — CinemaUSAC (Fase 3)

**Curso:** Estructuras de Datos — Universidad de San Carlos de Guatemala
**Lenguaje:** C++ (estándar C++17) con Qt Widgets para la interfaz.
**Restricción del proyecto:** todas las estructuras de datos, el SHA-256 y la compresión LZW están implementados manualmente. No se usan contenedores de la STL (`vector`, `map`, etc.) ni librerías externas de estructuras, criptografía o compresión. Qt se usa para la interfaz gráfica, para ejecutar Graphviz (`QProcess`) y para **interpretar JSON en memoria** (`QJsonDocument`) al cargar archivos.

---

## Índice
1. [Visión general y arquitectura](#1-visión-general-y-arquitectura)
2. [Estructura de archivos](#2-estructura-de-archivos)
3. [Grafo de sedes](#3-grafo-de-sedes)
4. [SHA-256](#4-sha-256)
5. [Árbol de Merkle](#5-árbol-de-merkle)
6. [Blockchain](#6-blockchain)
7. [Compresión LZW y persistencia](#7-compresión-lzw-y-persistencia)
8. [Carga masiva](#8-carga-masiva)
9. [Flujos principales](#9-flujos-principales)
10. [Reportes con Graphviz](#10-reportes-con-graphviz)
11. [Estructuras de la fase 2 que se mantienen](#11-estructuras-de-la-fase-2-que-se-mantienen)
12. [Complejidades](#12-complejidades)
13. [Decisiones de diseño](#13-decisiones-de-diseño)
14. [Pruebas realizadas](#14-pruebas-realizadas)
15. [Limitaciones conocidas](#15-limitaciones-conocidas)
16. [Glosario](#16-glosario)

---

## 1. Visión general y arquitectura

La aplicación es una ventana principal (`MainWindow`) que contiene un `QStackedWidget` con tres paneles: login, administrador y cliente. `MainWindow` es dueña de todas las estructuras de datos.

```
                        ┌──────────────────────────────┐
                        │          MainWindow          │
                        └──────────────┬───────────────┘
   ┌───────────────┬───────────────┬───┴───────────┬───────────────┬────────────────┐
   │ BSTCartelera  │ AVLFunciones  │ ArbolBClientes│ TablaHash     │  GrafoSedes    │
   │ (películas)   │ (funciones)   │ (clientes)    │ Reservas      │  (sedes)       │
   └───────────────┴───────────────┴───────────────┴───────┬───────┴────────────────┘
                                                           │ se reconstruye desde
                                                    ┌──────┴───────┐
                                                    │  Blockchain  │── ArbolMerkle
                                                    └──────┬───────┘── SHA256
                                                           │
                                                     blocks/blockN.json

   Persistencia (LZW) ◄── recorre grafo, BST y AVL ──► sedes.edd / peliculas.edd / funciones.edd
```

**Ciclo de vida de los datos**

| Momento | Qué ocurre |
|---|---|
| Al iniciar | `Persistencia::cargarTodo` descomprime los `.edd` y reinserta sedes, películas y funciones. `Blockchain::cargarDesdeDisco` lee y verifica los bloques. `reconstruirTablaHash` regenera las reservas. |
| Durante el uso | Las compras entran a una cola de transacciones pendientes. Cada 4 se genera un bloque. |
| Al cerrar | Si hay pendientes se genera un bloque. `Persistencia::guardarTodo` recorre las estructuras y escribe los `.edd` comprimidos. |

## 2. Estructura de archivos

| Archivo | Contenido |
|---|---|
| `Sede.h` | Estructura `Sede` (código, nombre, dirección, salas, teléfono) |
| `GrafoSedes.h` | Grafo no dirigido ponderado, BFS, ranking, sedes similares, reportes |
| `SHA256.h` | SHA-256 manual (FIPS 180-4) |
| `Transaccion.h` | Estructura `Transaccion` y su serialización canónica |
| `MerkleTree.h` | Lista enlazada de transacciones y `ArbolMerkle` |
| `Blockchain.h` | `Bloque`, `Blockchain`, prueba de trabajo, verificación, ventas |
| `LZW.h` | Compresión y descompresión LZW |
| `Persistencia.h` | Guardado/carga de `sedes.edd`, `peliculas.edd`, `funciones.edd` |
| `GraphvizUtil.h` | Escritura de `.dot` y generación de PNG con `dot`/`neato` |
| `CargadorJSON.h` | Carga masiva de sedes, películas, funciones y clientes |
| `Funcion.h`, `AVLFunciones.h`, `BSTCartelera.h` | Modificados: la función ahora tiene `codigo_sede` |
| `mainwindow.h/.cpp` | Interfaz y lógica de administrador y cliente |
| `ejemplos/` | JSON de prueba |
| `documentacion/` | Estos manuales |

## 3. Grafo de sedes

### 3.1 Modelo
- **Vértice:** una sede.
- **Arista:** une dos sedes que proyectan al menos una película en común.
- **Peso:** cantidad de películas **distintas** proyectadas en ambas sedes.
- El grafo es **no dirigido**: la arista A–B se guarda en la lista de A y en la lista de B con el mismo peso.

### 3.2 Representación: lista de adyacencia
```
cabeza → [S001] → [S002] → [S003] → ...         (lista enlazada de vértices)
            │         │
            ▼         ▼
         [→S003 w=2] [→S001 w=1]                (lista enlazada de aristas de cada vértice)
         [→S005 w=8]
```
Cada vértice guarda además la lista de códigos de películas que se proyectan en la sede (sin repetir). Las estructuras son `VerticeSede`, `NodoArista` y `NodoCodigoLista`.

### 3.3 Cálculo de aristas (`recalcularAristas`)
Las aristas **no se guardan** ni se editan a mano: se derivan de las funciones. Cada vez que cambia una función o una sede, y después de cargar los `.edd`, se recalculan desde cero:

```
1. Borrar todas las aristas y los catálogos de películas de todos los vértices.
2. Recorrer el AVL de funciones: por cada función, agregar su película al catálogo de su sede (sin repetir).
3. Para cada pareja de vértices (A, B), con A antes que B:
       peso = |catálogo(A) ∩ catálogo(B)|
       si peso > 0: crear la arista A–B y B–A con ese peso.
```
Esto asegura que el grafo siempre sea consistente con las funciones (por ejemplo, al eliminar una función desaparece la conexión si ya no hay películas en común).

### 3.4 Camino más corto: BFS (`caminoMasCorto`)
Como las aristas se consideran todas de costo 1 (cantidad de saltos), se usa **búsqueda en anchura**:

```
cola ← [origen]; visitado[origen] ← verdadero; padre[origen] ← ninguno
mientras la cola no esté vacía:
    v ← desencolar
    si v = destino: terminar
    para cada arista (v → u):
        si no visitado[u]: visitado[u] ← verdadero; padre[u] ← v; encolar(u)
si destino no fue visitado: no hay camino
si no: reconstruir el camino siguiendo padre[] desde destino hasta origen y revertirlo
```
La cola, el arreglo de visitados y el de padres son estructuras propias (arreglos dinámicos). Devuelve el camino y la cantidad de saltos.

### 3.5 Ranking y sedes similares
- **Ranking de sedes con mayor relación:** se recopilan las aristas (una sola vez por pareja) en un arreglo y se ordenan por peso descendente con **ordenamiento por inserción**.
- **Sedes similares:** se toman las aristas del vértice elegido y se ordenan igual.

### 3.6 Operaciones de administración
`agregarSede` (rechaza códigos repetidos), `editarSede`, `eliminarSede` (quita el vértice, sus aristas y, desde la interfaz, también las funciones de esa sede) y consulta de películas por sede.

## 4. SHA-256

Implementación propia de `SHA256::hash(const std::string&)`, que devuelve el hash en hexadecimal (64 caracteres). Pasos:

1. **Relleno (padding):** se agrega un bit `1`, ceros y la longitud original en bits (64 bits) hasta que el mensaje sea múltiplo de 512 bits.
2. **Bloques de 512 bits:** cada bloque se divide en 16 palabras de 32 bits y se expande a 64 palabras con las funciones σ0 y σ1.
3. **64 rondas** por bloque con las constantes `K` y las funciones `Ch`, `Maj`, `Σ0`, `Σ1`.
4. Se suman los resultados al estado (`H0..H7`) y se concatenan en hexadecimal.

Se verificó comparando contra `sha256sum` con varias entradas.

## 5. Árbol de Merkle

### 5.1 Hoja y nodos internos
- **Hoja:** `SHA256(transacción serializada)`. La serialización canónica (`Transaccion::serializar`) une los campos con `|`:
  `codigo_transaccion|codigo_reserva|id_cliente|codigo_funcion|fila|columna|monto|metodo_pago|fecha_pago`
  El monto se escribe con `'g'` y 17 dígitos para que el texto sea idéntico siempre. Si el estado es `CANCELADA`, se agrega `|CANCELADA` al final (el estado `PAGADA` es el valor por defecto y no cambia el texto, para mantener la compatibilidad con bloques ya creados).
- **Nodo interno:** `SHA256(hashIzquierdo + hashDerecho)`.
- **Raíz:** resume todas las transacciones. Si se altera una sola, la raíz cambia por completo.

### 5.2 Construcción
Se construye de abajo hacia arriba, nivel por nivel:

```
nivel ← hojas (una por transacción)
mientras |nivel| > 1:
    para cada pareja (izq, der) del nivel:
        si no hay der (nivel impar): der ← copia de izq    ← el último nodo se duplica
        padre ← SHA256(izq.hash + der.hash)
    nivel ← padres
raíz ← nivel[0]
```
Con una sola transacción el árbol tiene un único nodo y la raíz es su hash. En el reporte los nodos duplicados se dibujan en gris con la etiqueta "(duplicado)".

```
Ejemplo con 4 transacciones                 Ejemplo con 3 (impar)
          ROOT                                      ROOT
        /      \                                  /      \
     H(1,2)   H(3,4)                           H(1,2)   H(3,3')
     /  \     /  \                             /  \      /  \
   T1   T2  T3   T4                          T1   T2   T3   T3'(dup)
```

## 6. Blockchain

### 6.1 Estructura del bloque
| Campo | Descripción |
|---|---|
| `INDEX` | Número secuencial; el bloque génesis es 0 |
| `TIMESTAMP` | Fecha y hora con formato `dd-MM-yyyy::HH:mm:ss` |
| `NONCE` | Número que se incrementa hasta cumplir la prueba de trabajo |
| `DATA` | Lista de transacciones del bloque |
| `ROOTMERKLE` | Raíz del árbol de Merkle de `DATA` |
| `PREVIOUSHASH` | `HASH` del bloque anterior (`"0000"` en el génesis) |
| `HASH` | `SHA256(INDEX + TIMESTAMP + NONCE + ROOTMERKLE + PREVIOUSHASH)`, concatenados sin espacios |

**Transacción** (`DATA`): `codigo_transaccion`, `codigo_reserva`, `id_cliente`, `codigo_funcion`, `fila`, `columna`, `monto`, `metodo_pago`, `fecha_pago` y `estado` (`PAGADA` o `CANCELADA`). Contiene toda la información necesaria para reconstruir una reserva sin otra fuente.

### 6.2 Estructura en memoria
Lista enlazada simple donde cada bloque apunta al **anterior** y los nuevos se insertan **por delante**:

```
cabeza → [Bloque 3] → [Bloque 2] → [Bloque 1] → [Bloque 0] → null
```
Además se mantiene una **cola de transacciones pendientes** (lista enlazada con inserción al final).

### 6.3 Generación de un bloque y prueba de trabajo
```
1. DATA ← todas las transacciones pendientes (la cola queda vacía)
2. ROOTMERKLE ← raíz del árbol de Merkle de DATA
3. PREVIOUSHASH ← hash del último bloque íntegro ("0000" si no hay)
4. NONCE ← 0
   repetir:
       HASH ← SHA256(INDEX + TIMESTAMP + NONCE + ROOTMERKLE + PREVIOUSHASH)
       si HASH empieza con "0000": terminar
       NONCE ← NONCE + 1
5. Guardar blocks/blockN.json e insertar el bloque al frente de la lista
```
Como cada dígito hexadecimal tiene probabilidad 1/16, se necesitan en promedio 16⁴ = 65 536 intentos.

### 6.4 Archivo `blockN.json`
```json
{
  "INDEX": 0,
  "TIMESTAMP": "22-10-2026::23:09:41",
  "NONCE": 46581,
  "DATA": [
    { "codigo_transaccion": "T001", "codigo_reserva": "R1001", "id_cliente": "U001",
      "codigo_funcion": "F001", "fila": 2, "columna": 3, "monto": 45.00,
      "metodo_pago": "Tarjeta", "fecha_pago": "22-10-2026", "estado": "PAGADA" }
  ],
  "ROOTMERKLE": "…",
  "PREVIOUSHASH": "0000",
  "HASH": "0000…"
}
```

### 6.5 Verificación de integridad al iniciar
Se leen `block0.json`, `block1.json`, … en orden. Para cada bloque se comprueba:

1. Que el archivo exista, sea JSON válido y tenga todos los campos.
2. Que la raíz de Merkle recalculada desde `DATA` sea igual a `ROOTMERKLE`.
3. Que el hash recalculado sea igual a `HASH`.
4. Que `HASH` empiece con `0000`.
5. Que `PREVIOUSHASH` sea igual al `HASH` del bloque anterior.

Si algo falla, el bloque se marca como **manipulado** (rojo) con el motivo. **Todo lo que está después del primer bloque manipulado también se marca como descartado**, porque ya no se puede confiar en la cadena a partir de ahí. Al generar un nuevo bloque, la cadena continúa desde el último bloque íntegro y los archivos descartados se eliminan.

### 6.6 Tabla Hash reconstruida desde la cadena
`reconstruirTablaHash` recorre las transacciones de los bloques **íntegros**, del más antiguo al más nuevo:
- `PAGADA` → crea la reserva y la inserta en la Tabla Hash.
- `CANCELADA` → elimina esa reserva de la Tabla Hash.

### 6.7 Cancelaciones
Un bloque nunca se modifica.
- Si la compra aún está en la cola de pendientes, se saca de la cola.
- Si la compra ya está en un bloque, se crea una **nueva transacción con el mismo `codigo_transaccion` y estado `CANCELADA`** que se encola y se escribe en un bloque posterior. La escritura más reciente actualiza a la anterior: el historial muestra ambas, la Tabla Hash elimina la reserva y las ventas totales restan el boleto y el monto (la transacción cancelada conserva el monto y la fecha originales, así se anula exactamente la venta).

### 6.8 Consultas
- **Historial del cliente y comprobantes:** se obtienen del `DATA` de los bloques íntegros y de las pendientes.
- **Ventas totales:** se acumulan boletos y monto por función y por fecha en listas enlazadas propias (`ResumenVentas`).
- **Reconexión de reservas al iniciar sesión:** como los clientes no se guardan, al entrar un cliente sus reservas vigentes se vuelven a enlazar desde la blockchain.

## 7. Compresión LZW y persistencia

### 7.1 Algoritmo
LZW (Lempel-Ziv-Welch) reemplaza secuencias repetidas de bytes por códigos de un diccionario que se construye mientras se comprime.

**Compresión**
```
diccionario ← 256 entradas (un byte cada una, código = valor del byte)
actual ← primer byte
para cada byte siguiente b:
    si (actual + b) está en el diccionario: actual ← actual + b
    si no: emitir código(actual); agregar (actual + b) al diccionario; actual ← b
emitir código(actual)
```
**Descompresión:** reconstruye el **mismo diccionario** sin que se guarde en el archivo. Incluye el caso especial en que un código aún no existe (`KwKwK`): su cadena es la anterior más su primer byte.

### 7.2 Implementación
- El diccionario del compresor es una **tabla hash propia** con direccionamiento abierto y sondeo lineal, con clave (código del prefijo, byte). Tamaño primo 131071.
- El diccionario del descompresor es un arreglo de pares (prefijo, último byte); la cadena de un código se arma siguiendo los prefijos hacia atrás.
- Máximo de **65 536 entradas** (códigos de 16 bits). Cuando se llena, deja de crecer; compresor y descompresor aplican la misma regla y se mantienen sincronizados.
- **Códigos de ancho variable:** el k-ésimo código usa entre 9 y 16 bits según el tamaño del diccionario. Así, los archivos pequeños también se reducen.

### 7.3 Formato del archivo `.edd`
| Bytes | Contenido |
|---|---|
| 0–3 | Tamaño original del texto (little-endian), sirve para validar al descomprimir |
| 4–7 | Cantidad de códigos que siguen |
| 8… | Códigos empaquetados en bits de menos a más significativo |

Si los datos están corruptos (código imposible, datos insuficientes o tamaño distinto al esperado) la descompresión falla y devuelve error.

### 7.4 Qué se guarda y cómo
Se generan tres archivos: `sedes.edd`, `peliculas.edd` y `funciones.edd`.

- **Al cerrar:** se recorren **directamente las estructuras en memoria** (sedes del grafo, películas del BST en preorden, funciones del AVL en preorden), se arma el texto y se comprime. **No se crea ningún JSON intermedio** y no se vuelve a leer el archivo de carga masiva.
- El **preorden** hace que, al reinsertar en ese orden, el BST y el AVL se reconstruyan con la misma forma.
- **Al abrir:** se descomprime en memoria y se reinsertan los elementos. Las aristas del grafo no se guardan; se recalculan.
- Los clientes **no** se guardan (el enunciado pide solo estos tres archivos).

## 8. Carga masiva

`CargadorJSON::cargarPeliculasYFunciones` procesa un JSON con `sedes` y `peliculas`, y cada película trae su arreglo `funciones`:

1. Primero se insertan las **sedes** como vértices (se omiten las repetidas o inválidas).
2. Después se insertan las **películas** en el BST (se validan campos y fechas).
3. Por cada **función** se valida: sede existente, código no repetido y fecha válida. Si algo falla, la función se **rechaza** y se agrega el motivo a la lista de errores.
4. Al terminar se **recalculan las aristas** del grafo.
5. Devuelve un resumen: sedes, películas y funciones cargadas, funciones rechazadas y el detalle de errores, que la interfaz muestra en un mensaje.

`CargadorJSON::cargarClientesYReservas` carga clientes en el Árbol B (se omiten IDs o correos repetidos) y sus reservas en la Tabla Hash (se omiten los códigos de reserva repetidos, con fecha válida).

## 9. Flujos principales

### 9.1 Compra de un boleto
```
Cliente elige película → sede → función → asiento
   → se valida que el asiento esté libre → se crea la Reserva (Tabla Hash + matriz de asientos)
   → se pide el método de pago → se crea la Transacción (monto fijo Q45, estado PAGADA)
   → se encola en la blockchain
   → si hay 4 pendientes: se genera un bloque (prueba de trabajo)
```

### 9.2 Cancelación
```
Cliente selecciona una reserva
   ├─ ¿la transacción sigue pendiente?  → se saca de la cola
   └─ ¿ya está en un bloque?            → se encola la misma transacción con estado CANCELADA
Se libera el asiento, se elimina la reserva de la Tabla Hash y de la lista del cliente
```

### 9.3 Arranque con un bloque manipulado
```
Se leen los bloques → el bloque N falla la verificación → N y todos los posteriores quedan en rojo
→ aviso al usuario → la Tabla Hash se reconstruye solo con los bloques 0..N-1
→ si se genera un bloque nuevo: INDEX = N, PREVIOUSHASH = hash del bloque N-1 (los archivos descartados se eliminan)
```

## 10. Reportes con Graphviz

`GraphvizUtil::generarPng(nombre, dot, motor)` escribe el archivo `.dot` y ejecuta `dot` (o `neato` para el grafo de sedes) mediante `QProcess`, generando el PNG en la carpeta `reportes/` junto al ejecutable. La interfaz muestra la imagen dentro de la aplicación.

| Reporte | Archivo | Descripción |
|---|---|---|
| Grafo de sedes | `Grafo_Sedes.png` | Vértices con datos de la sede, aristas con su peso (motor `neato`) |
| Lista de adyacencia | `Lista_Adyacencia.png` | Cada vértice con su lista de vecinos y pesos |
| Blockchain | `Blockchain.png` | Cadena de bloques; verde = íntegro, rojo = manipulado |
| Árbol de Merkle | `Arbol_Merkle.png` | Árbol de un bloque; hoja azul, duplicado gris, raíz verde |

## 11. Estructuras de la fase 2 que se mantienen

| Estructura | Uso |
|---|---|
| BST (`BSTCartelera`) | Películas, con recorridos inorden/preorden/postorden y resaltado de "Próximo a retirar" |
| AVL (`AVLFunciones`) | Funciones; ahora cada nodo incluye `codigo_sede` |
| Árbol B (`ArbolBClientes`) | Clientes |
| Tabla Hash (`TablaHashReservas`) | Reservas; en esta fase se reconstruye desde la blockchain |
| Matriz dispersa (`MatrizDispersa`) | Mapa de asientos de cada función |
| Listas | Promociones y solicitudes |

## 12. Complejidades

Sea S = sedes, A = aristas, P = películas, F = funciones, n = transacciones de un bloque, B = bloques, L = tamaño del texto a comprimir.

| Operación | Costo |
|---|---|
| Registrar sede | O(S) |
| Recalcular aristas | O(F + S² · P) |
| BFS | O(S + A) |
| Ranking de aristas | O(A²) (inserción) |
| SHA-256 | O(longitud del mensaje) |
| Construir árbol de Merkle | O(n) hashes |
| Prueba de trabajo | Esperado ≈ 65 536 hashes |
| Verificar la cadena | O(B · n) |
| Reconstruir Tabla Hash | O(B · n) |
| LZW comprimir / descomprimir | O(L) promedio |

## 13. Decisiones de diseño

Confirmadas con el auxiliar del curso:
- Se puede generar un bloque por tres vías: automática cada 4 transacciones, manual por el administrador y al cerrar el programa.
- `DATA` guarda solo las transacciones nuevas de cada bloque.
- Una compra ya escrita en un bloque se cancela escribiendo la misma transacción con estado `CANCELADA` en un bloque nuevo.
- Todo lo posterior a un bloque manipulado se descarta; la cadena continúa desde el último bloque válido.
- Precio fijo por boleto: Q45.00.
- Los archivos generados se guardan junto al ejecutable.

Decisiones propias:
- Las aristas del grafo se derivan de las funciones (no se editan a mano) para que el grafo nunca quede inconsistente.
- La cadena se implementa como lista enlazada con inserción por delante, que permite recorrer del bloque más reciente al génesis.
- El estado `PAGADA` no cambia el texto serializado de la transacción (compatibilidad con bloques previos); `CANCELADA` sí lo cambia, por lo que alterar el estado en un archivo también se detecta como manipulación.

## 14. Pruebas realizadas

- **SHA-256:** comparación con `sha256sum`.
- **LZW:** compresión y descompresión de archivos pequeños y grandes, verificando que el texto resultante sea idéntico y que archivos corruptos se rechacen.
- **Blockchain:** generación automática, manual y al cerrar; reinicio y reconstrucción de la Tabla Hash; cancelación de compras pendientes y de compras ya en bloque; ventas netas tras cancelar.
- **Manipulación:** al cambiar un monto en `block0.json`, el programa detecta "ROOTMERKLE no coincide", marca el bloque y los siguientes en rojo y los excluye de las reservas.
- **Carga masiva:** archivos de ejemplo propios (con una función de sede inexistente que debe rechazarse) y los archivos de prueba del auxiliar (15 sedes, 25 películas, 58 funciones y 23 clientes).
- **Persistencia:** cerrar y abrir el programa verificando que sedes, películas, funciones, bloques y reservas siguen presentes.
- **Interfaz:** recorrido de todas las pestañas con datos reales.

## 15. Limitaciones conocidas

- Los clientes no se guardan entre ejecuciones; deben cargarse de nuevo.
- Las reservas que vienen dentro del JSON de clientes no generan transacciones en la blockchain; se recuperan al volver a cargar ese archivo.
- La prueba de trabajo bloquea la interfaz durante unos instantes mientras se busca el `NONCE`.
- El precio del boleto es una constante y no se puede editar desde la interfaz.

## 16. Glosario

| Término | Significado |
|---|---|
| Vértice / Arista | Nodo y conexión de un grafo |
| Lista de adyacencia | Representación de un grafo donde cada vértice guarda la lista de sus vecinos |
| BFS | Búsqueda en anchura; encuentra el camino con menos saltos |
| Hash | Resumen de longitud fija que cambia por completo si cambia la entrada |
| Árbol de Merkle | Árbol de hashes cuya raíz resume un conjunto de datos |
| Nonce | Número que se ajusta para cumplir la prueba de trabajo |
| Prueba de trabajo | Buscar un nonce que haga que el hash empiece con un prefijo (aquí `0000`) |
| Blockchain | Cadena de bloques enlazados por el hash del bloque anterior |
| LZW | Algoritmo de compresión sin pérdida basado en un diccionario dinámico |
| `.edd` | Archivo de datos comprimido con LZW de este proyecto |
