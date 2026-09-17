# Manual Técnico – CinemaUSAC Fase 2

**Curso:** Estructuras de Datos  
**Proyecto:** CinemaUSAC – Sistema de Gestión de Cine  
**Fase:** 2  
**Lenguaje:** C++  
**Framework GUI:** Qt 6  
**Herramienta de reportes:** Graphviz  

---

## 1. Descripción general del sistema

CinemaUSAC es un sistema de gestión de cine que permite administrar películas, funciones, clientes, reservas de asientos, promociones y solicitudes especiales.  

En la **Fase 1** se utilizaron:

- Árbol BST (películas)
- Lista circular doblemente enlazada (solicitudes)
- Lista de listas (promociones y beneficios)
- Matriz dispersa ortogonal (asientos)

En la **Fase 2** se incorporan:

- Árbol AVL (funciones)
- Árbol B de orden 4 (clientes)
- Tabla Hash con encadenamiento (reservas)
- Persistencia en JSON (carga masiva y asientos por función)
- Matriz dispersa dinámica (una matriz activa por función)

La interfaz se desarrolla con **Qt Widgets** y los reportes visuales se generan con **Graphviz** mediante archivos `.dot` convertidos a `.png`.

---

## 2. Arquitectura del proyecto

### 2.1 Estructura de carpetas

EDD_Fase2/
├── src/CinemaUSAC/
│   ├── main.cpp
│   ├── mainwindow.h / mainwindow.cpp
│   ├── Pelicula.h
│   ├── BSTCartelera.h
│   ├── Funcion.h
│   ├── AVLFunciones.h
│   ├── Cliente.h
│   ├── ArbolBClientes.h
│   ├── Reserva.h
│   ├── TablaHashReservas.h
│   ├── NodoMatriz.h
│   ├── MatrizDispersa.h
│   ├── Solicitud.h / ListaSolicitudes.h
│   ├── Beneficio.h / Promocion.h / ListaPromociones.h
│   ├── CargadorCSV.h
│   ├── CargadorJSON.h
│   └── CinemaUSAC.pro
├── datos_prueba/
│   ├── peliculas_funciones.json
│   └── clientes_reservas.json
├── documentacion/
│   ├── Manual_Tecnico.md
│   └── Manual_Usuario.md
└── reportes/          (generados en tiempo de ejecución)
└── asientos/          (JSON por función, generados en tiempo de ejecución)


### 2.2 Capas lógicas

| Capa | Responsabilidad | Componentes |
|------|-----------------|-------------|
| Presentación | Interfaz gráfica, eventos de usuario | `MainWindow`, paneles Login/Admin/Cliente |
| Lógica de aplicación | Coordinar estructuras, flujos de negocio | Métodos de `MainWindow`, `CargadorJSON` |
| Estructuras de datos | Almacenamiento y operaciones | BST, AVL, Árbol B, Hash, Matriz, Listas |
| Persistencia | Lectura/escritura de archivos | JSON de carga masiva, JSON de asientos |

---

## 3. Estructuras de datos implementadas

### 3.1 Árbol Binario de Búsqueda (BST) – Películas

**Archivo:** `BSTCartelera.h`  
**Clave de ordenamiento:** `codigo` de la película (string).

#### Estructura del nodo (`Pelicula`)

| Campo | Tipo | Descripción |
|-------|------|-------------|
| codigo | string | Identificador único (ej. P001) |
| titulo | string | Nombre de la película |
| genero | string | Género |
| duracion | int | Minutos |
| clasificacion | string | AA, B15, etc. |
| idioma | string | Español / Subtitulada |
| fechaEstreno | string | yyyy-MM-dd |
| fechaFin | string | yyyy-MM-dd |
| izquierda / derecha | Pelicula* | Hijos del árbol |

#### Operaciones

- `insertar` – inserción ordenada por código
- `buscar` – búsqueda por código
- `eliminar` – eliminación de nodo (0, 1 o 2 hijos)
- `cargarEnTabla(tabla, recorrido)` – preorden, inorden o postorden
- `generarReporteGraphviz` – grafo del BST

#### Decisiones de diseño

- Se reutilizó el BST de Fase 1 sin cambios estructurales.
- Los recorridos se implementaron de forma explícita para cumplir el requisito de listar en preorden, inorden y postorden.
- El estado en cartelera (En cartelera / Próximo estreno / Próximo a retirar) se calcula en tiempo de visualización con `QDate`, no se almacena en el nodo.

---

### 3.2 Árbol AVL – Funciones

**Archivos:** `Funcion.h`, `AVLFunciones.h`  
**Clave:** `codigo_funcion` (string).

#### Estructura del nodo (`Funcion`)

| Campo | Tipo | Descripción |
|-------|------|-------------|
| codigo_funcion | string | Clave (F001, F002, …) |
| codigo_pelicula | string | Referencia a película del BST |
| horario | string | Ej. 17:00 |
| sala | string | Ej. Sala 2 |
| fecha | string | yyyy-MM-dd (para colorear el reporte) |
| filas / columnas | int | Dimensiones de la sala |
| archivo_asientos | string | Nombre del JSON de asientos |
| altura | int | Altura del subárbol |
| izquierda / derecha | Funcion* | Hijos |

#### Balanceo

Se calculan factor de balance y rotaciones:

- LL → rotación simple derecha  
- RR → rotación simple izquierda  
- LR → rotación doble (izq + der)  
- RL → rotación doble (der + izq)  

La altura se actualiza en cada inserción y eliminación.

#### Operaciones

- `insertar` – con balanceo automático
- `eliminar` – con rebalanceo
- `buscar` – por código de función
- `cargarEnTabla` – preorden / inorden / postorden
- `generarReporteGraphviz` – nodos circulares; **verde** si fecha ≥ hoy, **rojo (tomato)** si fecha < hoy

#### Decisiones de diseño

- Se eligió AVL porque las funciones se consultan con frecuencia y se requiere un árbol siempre balanceado (O(log n) garantizado).
- El campo `fecha` se agregó para cumplir el requisito de colorear nodos según si la función ya se realizó.
- Cada función tiene su propio archivo `FXXX_funcion.json`; la matriz en memoria solo carga una función a la vez.

---

### 3.3 Árbol B de orden 4 – Clientes

**Archivos:** `Cliente.h`, `ArbolBClientes.h`  
**Orden:** 4 → máximo 3 claves y 4 hijos por nodo.  
**Clave:** `id` del cliente (string).

#### Estructura del cliente

| Campo | Tipo | Descripción |
|-------|------|-------------|
| id | string | U001, U002, … |
| nombre, correo, telefono, password, tipo | string | Datos del cliente |
| listaReservas | NodoCodigoReserva* | Lista simple de códigos de reserva (referencias) |

#### Nodo del Árbol B (`NodoB`)

- `claves[3]` – punteros a `Cliente*`
- `hijos[4]` – punteros a hijos
- `numClaves` – cantidad actual de claves
- `esHoja` – indica si es hoja

#### Operaciones

- `insertar` – con división de nodos llenos (`dividirHijo`)
- `buscar` – por ID
- `buscarPorCorreo` – recorrido para login y validación de registro
- `eliminar` – eliminación desde hoja (versión práctica para la demo)
- `cargarEnTabla` – recorrido inorden
- `generarReporteGraphviz` – nodos tipo `record` con compartimentos

#### Decisiones de diseño

- Orden 4 cumple el enunciado (máximo 3 claves por nodo).
- Las reservas no se duplican en el cliente: solo se guardan **códigos** en una lista enlazada.
- La eliminación completa con fusión/redistribución de nodos internos es compleja; se implementó eliminación en hoja, suficiente para demostración y para que el cliente desaparezca del listado y del reporte.
- Unicidad de ID y correo se valida en la inserción.

---

### 3.4 Tabla Hash – Reservas

**Archivos:** `Reserva.h`, `TablaHashReservas.h`  
**Clave:** `codigo_reserva` (string).  
**Resolución de colisiones:** encadenamiento (listas enlazadas por cubeta).

#### Estructura de la reserva

| Campo | Tipo | Descripción |
|-------|------|-------------|
| codigo_reserva | string | Clave (R1001, …) |
| id_cliente | string | Referencia al cliente |
| codigo_funcion | string | Referencia a la función |
| fila / columna | int | Asiento |
| fecha_reserva | string | yyyy-MM-dd |

#### Función hash

```text
hash = 0
para cada carácter c en la clave:
    hash = hash * 31 + c
índice = hash % tamaño_tabla

Operaciones

insertar – O(1) promedio
buscar – O(1) promedio
eliminar – O(1) promedio
generarReporteGraphviz – cubetas y cadenas de colisión

Decisiones de diseño

Tabla Hash se eligió por acceso rápido por código de reserva (cancelación, historial).
Encadenamiento evita rehashing complejo y es simple de visualizar en Graphviz.
La relación con el cliente y la función es por código (no se almacenan objetos completos).

3.5 Matriz dispersa ortogonal – Asientos
Archivos: NodoMatriz.h, MatrizDispersa.h
Comportamiento en Fase 2

Existe una sola matriz en memoria (matrizActual).
Al cambiar de función:
Se guarda la matriz actual en su JSON.
Se libera la matriz.
Se crea una nueva con las dimensiones de la función.
Se cargan los asientos ocupados desde el JSON de esa función.


Contenido del nodo ocupado
En Fase 2 el campo cliente del nodo almacena el codigo_reserva (no el nombre del cliente). El nombre se obtiene consultando la Tabla Hash.
Persistencia por función
Archivo: asientos/F001_funcion.json

{
  "codigo_funcion": "F001",
  "asientos_ocupados": [
    { "fila": 2, "columna": 5, "codigo_reserva": "R1001" }
  ]
}

Operaciones relevantes

reservarAsiento / cancelarReserva
vaciar
guardarAJson / cargarDesdeJson
crearArchivoVacio (al crear una función nueva)
generarReporteGraphviz

Decisiones de diseño

Una matriz global evita cargar todas las salas a la vez.
JSON por función permite persistencia independiente y cumple el enunciado.
La matriz solo almacena nodos de asientos ocupados (verdadera matriz dispersa).

3.6 Estructuras mantenidas de Fase 1

Lista circular doblemente enlazadaSolicitudes especiales
Lista de listas (circular simple + doblemente enlazada de beneficios)Promociones

No se modificó su lógica interna; se integraron en los mismos paneles de la interfaz.

4. Persistencia y carga masiva
4.1 Módulo CargadorJSON
Archivo: CargadorJSON.h
Carga de películas + funciones

Lee un JSON con arreglo peliculas.
Cada película puede traer un arreglo funciones.
Valida campos obligatorios y fechas (yyyy-MM-dd).
Inserta en BST y AVL.
Crea el archivo vacío de asientos por cada función.

Carga de clientes + reservas

Lee arreglo clientes.
Valida ID, correo, password.
Inserta en Árbol B.
Por cada reserva del cliente: inserta en Tabla Hash y agrega el código a la lista del cliente.

Registros inválidos se omiten y se reportan por consola.
4.2 Archivos de prueba
Ubicación sugerida: datos_prueba/

peliculas_funciones.json
clientes_reservas.json


5. Flujos principales del sistema
5.1 Login

Admin fijo: AdminCine@gmail.com / admin123Pass
Cliente: búsqueda por correo en Árbol B + verificación de password

5.2 Reserva de asiento

Cliente carga una función → se activa la matriz de esa función.
Elige fila y columna.
Se genera codigo_reserva.
Se marca el asiento en la matriz (con el código).
Se inserta la reserva en la Tabla Hash.
Se agrega el código a la lista del cliente.
Se persiste el JSON de la función.

5.3 Cancelación de reserva

Se obtiene la reserva de la Tabla Hash.
Si la función no es la activa, se carga.
Se libera el asiento en la matriz.
Se guarda el JSON.
Se elimina de la Tabla Hash.
Se quita la referencia del cliente.

5.4 Eliminación de cliente

Se eliminan todas sus reservas de la Tabla Hash.
Se elimina el cliente del Árbol B (hoja).

5.5 Eliminación de función

Si es la función activa, se libera la matriz.
Se borra el archivo JSON de asientos.
Se elimina el nodo del AVL.

6. Interfaz gráfica (Qt)
6.1 Paneles

Índice Panel
0      Login
1      Administrador
2      Cliente

6.2 Pestañas Administrador

Cartelera (BST)
Funciones y Asientos (AVL + Matriz)
Clientes (Árbol B)
Carga Masiva JSON
Promociones
Solicitudes Especiales
Reportes Visuales

6.3 Pestañas Cliente

Ver Cartelera
Reservar / Cancelar (+ historial)
Promociones
Solicitudes
Mi Perfil

7. Reportes Graphviz
Se generan en la carpeta reportes/ junto al ejecutable:

Archivo               Estructura
Cartelera_BST.png     BST de películas
AVL_Funciones.png     AVL (verde/rojo por fecha)
ArbolB_Clientes.png   Árbol B orden 4
TablaHash_Reservas.png Cubetas y colisiones
Matriz_Asientos.png   Matriz dispersa de la función activa
Lista_Solicitudes.png Lista circular
Lista_Promociones.png Lista de listas

Proceso:

Escritura del archivo .dot
Ejecución de dot -Tpng archivo.dot -o archivo.png mediante QProcess

Requisito del sistema: tener Graphviz instalado y el comando dot en el PATH.

8. Decisiones de diseño globales

Referencias por código entre estructuras (no objetos anidados) para evitar duplicación y facilitar actualización.
Una matriz activa en memoria para no cargar todas las salas simultáneamente.
JSON como formato de persistencia (requisito de Fase 2), en lugar de CSV para datos nuevos.
Qt para la GUI por portabilidad y widgets nativos (tablas, diálogos, layouts).
Graphviz exclusivo para reportes, como indica el enunciado.
Sin contenedores STL para las estructuras principales (listas, árboles y hash propios), cumpliendo el enfoque del curso.

9. Compilación y ejecución
Requisitos

Qt 6 (Core, Gui, Widgets)
Compilador C++ (MinGW / MSVC)
Graphviz (dot en PATH)

Archivo de proyecto (.pro)

QT += core gui widgets
CONFIG += c++17
SOURCES += main.cpp mainwindow.cpp
HEADERS += ... (todos los .h del proyecto)

Pasos

Abrir CinemaUSAC.pro en Qt Creator
Configurar kit
Build → Rebuild
Ejecutar

Al iniciar se crean las carpetas asientos/ y reportes/ si no existen.

10. Pruebas recomendadas

Login admin y cliente
Carga masiva de ambos JSON
Crear función con fecha pasada y futura → verificar colores en Graphviz
Reservar asiento → historial → cancelar
Eliminar cliente y función
Cerrar y reabrir la aplicación → verificar persistencia de asientos


11. Conclusión
La Fase 2 integra cinco estructuras de datos principales (BST, AVL, Árbol B, Tabla Hash y Matriz dispersa) con persistencia JSON, interfaz Qt y reportes Graphviz. El diseño privilegia la separación de responsabilidades, el acceso eficiente por clave y el cumplimiento de los requisitos del enunciado del curso de Estructuras de Datos.
text


---

Copia ese contenido a `documentacion/Manual_Tecnico.md`.

Cuando quieras, te armo el **Manual de Usuario** (con secciones listas para que insertes tus screenshots).