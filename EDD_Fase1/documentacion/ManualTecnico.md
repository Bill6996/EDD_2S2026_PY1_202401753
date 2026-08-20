# MANUAL TÉCNICO - SISTEMA DE GESTIÓN DE CINE (CinemaUSAC)

**Universidad de San Carlos de Guatemala**  
**Facultad de Ingeniería**  
**Escuela de Ciencias y Sistemas**  
**Estructuras de Datos**  
**Semestre:** Segundo Semestre 2026  
**Fase:** Fase 1  

---

## 1. INTRODUCCIÓN

El presente documento constituye el Manual Técnico para la aplicación **CinemaUSAC**, un sistema de escritorio enfocado en la administración de taquilla, cartelera, promociones y solicitudes especiales para una cadena de cines. 

La solución está desarrollada íntegramente en **C++** sobre el marco de trabajo **Qt 6**, prescindiendo del uso de contenedores de la biblioteca estándar (STL como `std::vector` o `std::list`) para implementar estructuras de datos dinámicas mediante el manejo explícito de punteros en la memoria *heap*. Asimismo, integra el motor gráfico de **Graphviz** mediante la ejecución externa asíncrona de procesos para el renderizado visual de las estructuras en tiempo real.

---

## 2. REQUISITOS DEL SISTEMA Y ENTORNO DE DESARROLLO

### 2.1. Entorno de Desarrollo
* **Lenguaje de Programación:** C++ (Estándar C++17 o superior).
* **Framework de GUI:** Qt Framework 6.x (Qt Creator 12.0+).
* **Compilador:** MinGW-w64 (GCC 11.0+) o Clang.
* **Sistema de Construcción:** CMake / qmake.
* **Generación de Diagramas:** Graphviz Versión 2.38 o superior (debe estar registrado en la variable de entorno PATH del sistema operativo).

### 2.2. Requisitos del Sistema Operativo
* **SO:** Windows 10/11 (64-bit) o distribuciones Linux (Ubuntu/Debian) con soporte Qt.
* **Dependencias de Consola:** Herramienta ejecutable `dot` accesible desde consola/terminal.

---

## 3. ARQUITECTURA DEL SISTEMA

El diseño del software sigue una arquitectura modular separada en tres capas principales:

+-------------------------------------------------------------+
|                 CAPA DE INTERFAZ DE USUARIO                 |
|             (MainWindow, QTabWidget, QTableWidget)          |
+-------------------------------------------------------------+
|
v
+-------------------------------------------------------------+
|                CAPA DE LÓGICA Y PARSEO DE DATOS             |
|                   (CargadorCSV, QProcess)                   |
+-------------------------------------------------------------+
|
v
+-------------------------------------------------------------+
|              CAPA DE ESTRUCTURAS DE DATOS (CORE)            |
|  [BSTCartelera] [ListaSolicitudes] [ListaPromos] [Matriz]   |
+-------------------------------------------------------------+

1. **Capa Core (Estructuras de Datos):** Implementa de forma aislada las clases que almacenan la información en memoria dinámica. No depende de componentes gráficos.
2. **Capa de Negocio y Carga:** Contiene clases auxiliares como `CargadorCSV` para la ingesta masiva de archivos planos y `QProcess` para orquestar la invocación de Graphviz.
3. **Capa de Presentación (UI):** Gestiona los páneles del Administrador y Cliente mediante `QMainWindow`, conectando las señales de interfaz gráfica con los punteros de las estructuras de datos.

---

## 4. DESCRIPCIÓN DETALLADA DE ESTRUCTURAS DE DATOS

### 4.1. Árbol Binario de Búsqueda (BST) - Cartelera de Películas
Estructura jerárquica no lineal utilizada para mantener ordenada la oferta cinematográfica. Los nodos se ordenan en función de su clave principal: el `codigo` alfanumérico de la película.

* **Estructura del Nodo (`Pelicula`):**
  ```cpp
  struct Pelicula {
      std::string codigo;
      std::string titulo;
      std::string genero;
      int duracion;
      std::string clasificacion;
      std::string idioma;
      std::string fechaEstreno;
      std::string fechaFin;
      Pelicula* izquierda;
      Pelicula* derecha;
  };


4.2. Lista Circular Doblemente Enlazada - Solicitudes Especiales
Almacena requerimientos especiales de los clientes (cumpleaños, eventos, solicitudes personalizadas). La naturaleza circular y doblemente enlazada permite la navegación continua e indefinida en ambas direcciones (siguiente/anterior) dentro de la interfaz.

    struct Solicitud {
        int numero;
        std::string cliente;
        std::string telefono;
        std::string tipo;
        std::string descripcion;
        std::string estado;
        std::string fecha;
        Solicitud* siguiente;
        Solicitud* anterior;
    };

Comportamiento Anular:

Para el primer nodo insertado (cabeza), sus punteros siguiente y anterior apuntan hacia sí mismo.

En inserciones posteriores, el puntero siguiente del último nodo reencadena hacia cabeza, mientras que cabeza->anterior apunta al último nodo creado.

4.3. Lista Circular Simple de Listas Dobles - Promociones y Beneficios
Estructura compuesta o mulitlista que modela una relación de uno a muchos (1:N) entre las promociones comerciales y los beneficios enlazados a cada una de ellas.

Lista Principal (Circular Simple): Encadena las promociones activas. El puntero siguiente del último nodo apunta nuevamente al nodo cabeza.

Sublista Secundaría (Doblemente Enlazada): Cada nodo de promoción posee un puntero listaBeneficios que encadena verticalmente la lista de beneficios asociados.

Estructuras del Nodo:

struct Beneficio {
    std::string tipo;
    std::string descripcion;
    std::string valor;
    Beneficio* siguiente;
    Beneficio* anterior;
};

struct Promocion {
    std::string codigo;
    std::string nombre;
    std::string fechaInicio;
    std::string fechaFin;
    std::string diasAplica;
    Promocion* siguiente;
    Beneficio* listaBeneficios; // Apuntador a la sublista de beneficios
};

4.4. Matriz Dispersa Ortogonal - Mapa de Asientos por Función
Representación bidimensional eficiente que modela la distribución de asientos (filas y columnas) para una función activa. Optimiza el uso de memoria al no crear instancias físicas para celdas vacías innecesarias, manteniendo únicamente los asientos interactuados mediante una red ortogonal de cabeceras.

Estructura del Nodo de Celda (NodoMatriz):

struct NodoMatriz {
    int fila;
    int columna;
    bool ocupado;
    std::string cliente;
    NodoMatriz* arriba;
    NodoMatriz* abajo;
    NodoMatriz* izquierda;
    NodoMatriz* derecha;
};

5. GENERACIÓN Y RENDERIZADO DE REPORTES (GRAPHVIZ)
El sistema genera representaciones gráficas de las estructuras de datos utilizando el lenguaje de descripción de grafos DOT de Graphviz.

5.1. Flujo de Procesamiento
Generación del Archivo .dot: La estructura de datos correspondiente recorre sus nodos e imprime el formato de nodos, relaciones, atributos de forma (shape) y colores (fillcolor) en un archivo dentro del directorio de ejecución reportes/.

Invocación del Proceso: Se utiliza la clase QProcess del framework Qt para invocar el comando externo dot de forma no bloqueante:

        QProcess process;
        process.start("dot", QStringList() << "-Tpng" << dotFilePath << "-o" << pngFilePath);
        process.waitForFinished(3000);

Despliegue Visual: La clase MainWindow carga la imagen .png resultante dentro de un QPixmap y la renderiza sobre un QLabel contenido en un marco deslizable QScrollArea.

5.2. Personalización Visual según Requerimiento
Reporte 1 (BST Cartelera): Nodos rectangulares (shape=box). Aplica algoritmo de evaluación de fechas con QDate para colorear nodos en verde (lightgreen) si están en cartelera activa y en amarillo (yellow) si su fecha de retiro es menor o igual a 7 días.

Reporte 2 (Solicitudes Especiales): Nodos circulares (shape=circle) con relaciones bidireccionales (dir=both). Incluye un nodo independiente tipo nota (shape=note) que actúa como contador dinámico de solicitudes pendientes.

Reporte 3 (Promociones y Beneficios): Fuerza la alineación horizontal de la lista circular principal usando la directiva { rank=same; ... } de Graphviz y despliega las sublistas de beneficios verticalmente hacia abajo.

Reporte 4 (Matriz Dispersa): Utiliza enlaces ortogonales (splines=ortho), agrupamiento vertical de columnas mediante la propiedad group=columna y añade un recuadro informativo con estadísticas de asientos ocupados, libres y totales.


6. GESTIÓN DE MEMORIA Y BUENAS PRÁCTICAS
Para garantizar la estabilidad del software y prevenir fugas de memoria (memory leaks):

Destrucción Recursiva/Iterativa: Cada estructura de datos incluye un destructor explícito (~BSTCartelera(), ~MatrizDispersa(), etc.) encargado de recorrer todos los nodos dinámicos creados con la instrucción new y liberar la memoria asignada mediante delete.

Reemplazo de Funciones: Cuando la administración crea una nueva función en la matriz dispersa, se invoca previamente la eliminación del objeto matrizActual, liberando todas las celdas y cabeceras de la función anterior.

Invocaciones Seguras: Se valida la existencia de punteros nulos (nullptr) antes de acceder a miembros de datos en cada navegación o consulta en la interfaz gráfica.

## 7. INTERACCIÓN Y FLUJO DE INFORMACIÓN ENTRE CLASES

La arquitectura del sistema está diseñada bajo un esquema centralizado donde la clase `MainWindow` actúa como **controlador principal**, orquestando la comunicación entre la interfaz gráfica y las estructuras de datos dinámicas.

+-----------------------+
              |      CargadorCSV      |
              +-----------------------+
                          |
                  v (Carga Masiva)
                     ---------------+

|                            MainWindow                             |
|                    (Coordinador de Interfaz)                      |
+-------------------------------------------------------------------+
|                    |                    |                    |
v                    v                    v                    v
+--------------+ +------------------+ +------------------+ +-----------------+
| BSTCARTELERA | | LISTASOLICITUDES | | LISTAPROMOCIONES | | MATRIZDISPERSA  |
|  (Películas) | |   (Solicitudes)  | |   (Promociones)  | | (Mapa Asientos) |
+--------------+ +------------------+ +------------------+ +-----------------+

### Relaciones y Datos Compartidos

* **MainWindow ↔ BSTCARTELERA:**  
  * **Relación:** `MainWindow` mantiene una instancia de `BSTCartelera`.  
  * **Flujo de datos:** Recibe la ruta de archivos planos mediante `CargadorCSV` para insertar nodos `Pelicula`. Al consultar cartelera o crear funciones, `MainWindow` busca en el árbol el código de la película para extraer título, duración e idioma.

* **MainWindow ↔ MatrizDispersa:**  
  * **Relación:** `MainWindow` gestiona el puntero `matrizActual` de forma dinámica (se destruye y reconstruye con cada función).  
  * **Flujo de datos:** Asocia la película seleccionada del BST con un horario y sala. Cuando un cliente reserva o cancela, `MainWindow` envía las coordenadas (fila, columna) y el nombre del cliente a la `MatrizDispersa`, actualizando el mapa visual en ambos paneles (Admin y Cliente).

* **MainWindow ↔ ListaSolicitudes:**  
  * **Relación:** Instancia única persistente de la lista circular doblemente enlazada.  
  * **Flujo de datos:** El Panel Cliente registra datos de contacto y descripción, generando un objeto `Solicitud`. El Panel Administrador navega mediante los punteros `siguiente` y `anterior` sobre los mismos nodos para modificar el atributo `estado` ("Atendida" / "Rechazada") en tiempo real.

* **MainWindow ↔ ListaPromociones:**  
  * **Relación:** Instancia de la lista circular de listas enlazadas.  
  * **Flujo de datos:** El Administrador inserta nodos `Promocion` y subnodos `Beneficio`. Esta estructura es leída de forma simultánea por la pestaña del Cliente para proyectar las ofertas vigentes en un control jerárquico `QTreeWidget`.

* **CargadorCSV ➔ BSTCARTELERA:**  
  * **Relación:** Clase utilitaria estática de paso.  
  * **Flujo de datos:** Parsea el archivo `.csv`, extrae los atributos de cada película y llama secuencialmente al método `insertar()` de `BSTCartelera`.