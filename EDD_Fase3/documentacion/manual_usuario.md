# Manual de Usuario — CinemaUSAC Fase 3

> Agrega aquí capturas de pantalla de cada sección.

## 1. Requisitos
- Compilar `CinemaUSAC.pro` con Qt Creator.
- Tener Graphviz instalado y en el PATH (para los reportes).

## 2. Inicio
Al abrir, el programa carga `sedes.edd`, `peliculas.edd` y `funciones.edd` (si existen) y verifica la blockchain. Si hay bloques manipulados aparece una advertencia y se marcan en rojo en el panel Blockchain.

## 3. Administrador
**Sedes (Grafo):** registrar, editar, eliminar sedes; ver películas por sede; camino más corto (BFS) entre dos sedes; sedes con mayor relación. Las conexiones se crean solas según las películas compartidas.

**Funciones y Asientos:** crear/editar/eliminar funciones. Ahora se elige la **sede**.

**Carga Masiva JSON:** elegir un archivo con `sedes`, `peliculas` y `funciones`. Se muestra cuántos registros se cargaron y cuáles se rechazaron. Archivo de ejemplo: `ejemplos/peliculas_prueba_fase3.json`.

**Blockchain:** tabla de bloques (verde = íntegro, rojo = manipulado), generar bloque manualmente, ver reporte de la cadena, árbol de Merkle de un bloque y ventas totales.

**Reportes Visuales:** botones 1–9 (BST, AVL, Árbol B, Tabla Hash, Matriz, **Grafo de Sedes, Lista de Adyacencia, Blockchain, Árbol de Merkle**). Se muestran dentro de la aplicación.

## 4. Cliente
(Credenciales de prueba: `ana@correo.com` / `1234`.)

- **Sedes (Grafo):** elegir una película y ver en qué sedes está; ver sus funciones por sede; ver sedes similares.
- **Reservar / Cancelar:** película → sede → función → asiento. Al comprar se elige método de pago (Tarjeta/Efectivo); se genera una transacción que entra a la blockchain. Se puede cancelar en cualquier momento: si la compra aún no está en un bloque se retira de la cola; si ya está en un bloque, el bloque no se modifica y la transacción se vuelve a registrar con estado CANCELADA en un bloque nuevo.
- **Mis Compras (Blockchain):** historial de transacciones y comprobante.

## 5. Cierre
Al cerrar la aplicación se genera un bloque con las transacciones pendientes y se guardan los `.edd`.
