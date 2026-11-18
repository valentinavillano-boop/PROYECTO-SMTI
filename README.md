# Proyecto SMTI — Servidor TCP de Telemetría Genérico

**Universidad del Cauca · Programación Orientada a Objetos · 2026.2**
**Evaluación 3 — Unidad 9: Programación Genérica, Excepciones y Redes (Sockets)**

Autor: _[escribe aquí tu nombre y código]_

Sistema de Monitoreo y Telemetría Industrial (SMTI): un servidor TCP en C++ (POSIX sockets, GCC/Linux, ZinjaI) que recibe tramas de nodos sensores remotos, las valida, crea el objeto polimórfico correspondiente en el **Heap** y lo guarda en una **lista enlazada genérica** (`ListaGen<T>`). Los fallos de red se manejan con `throw std::runtime_error` y `try/catch`.

## Estructura

```
Proyecto_SMTI/
├── include/
│   ├── Sensor.h               Clase base abstracta (virtual puro, destructor virtual, operator<<)
│   ├── SensorTemperatura.h    Derivada polimórfica (override)
│   ├── SensorPresion.h        Derivada polimórfica (override)
│   ├── SensorCaudal.h         Derivada polimórfica (override)
│   ├── ListaGen.h             TAD lista genérica con templates (destructor sin fugas)
│   ├── MuestraSensor.h        struct MuestraSensor { int idSensor; double valor; char tipo[12]; }
│   ├── ServidorSockets.h      Clase ServidorSMTI (socket/bind/listen/accept/recv)
│   └── ExcepcionesSMTI.h      Excepciones basadas en std::runtime_error
├── src/
│   ├── Sensor.cpp · SensorTemperatura.cpp · SensorPresion.cpp · SensorCaudal.cpp
│   ├── ServidorSockets.cpp
│   └── main.cpp               Punto de entrada integrador
├── cliente/ClienteSMTI.cpp    Nodo sensor de prueba (no forma parte del proyecto ZinjaI)
├── SMTI.zpr                   Proyecto ZinjaI
├── Makefile
└── README.md
```

## Compilar y ejecutar

**Con Makefile (terminal Linux):**

```bash
make            # genera ./SMTI (servidor) y ./ClienteSMTI (cliente de prueba)
./SMTI          # servidor en el puerto 8080  (opcional: ./SMTI 9000)
```

En **otra terminal**:

```bash
./ClienteSMTI 127.0.0.1 8080 --salir     # envía tramas de ejemplo y cierra el servidor
# o a mano:
printf '101;TEMPERATURA;92.5;Caldera-01\nSALIR\n' | nc 127.0.0.1 8080
```

**Con ZinjaI:** abrir `SMTI.zpr` y compilar con F9. Si no abre el proyecto, crear uno nuevo y agregar los `.cpp` de `src/` y los `.h` de `include/`. Los `.cpp` incluyen las cabeceras con ruta relativa (`../include/...`), por lo que no hay que configurar directorios de inclusión. El cliente se compila aparte.

**Verificar fugas de memoria:** `make leaks` (requiere valgrind) o compilar con `-fsanitize=address`.

## Protocolo de tramas (texto, una por línea)

```
ID;TIPO;VALOR[;UBICACION]
101;TEMPERATURA;92.5;Caldera-01
```

`TIPO` ∈ `TEMPERATURA`, `PRESION`, `CAUDAL`. El comando `SALIR` cierra el servidor ordenadamente (Ctrl+C también). Una trama inválida recibe `ERR SMTI: ...` y el servidor continúa.

## Cumplimiento de la rúbrica

| Criterio | Dónde está |
|---|---|
| **1. Sockets POSIX** | `ServidorSMTI::iniciar()` (`socket`, `setsockopt`, `bind` al 8080, `listen`) y `ejecutar()`/`atenderCliente()` (`accept`, `recv`). `recv` usa timeout (`SO_RCVTIMEO`) para no bloquearse indefinidamente. |
| **2. Jerarquía polimórfica** | `Sensor` abstracta (`virtual void procesarLectura() = 0`, `virtual ~Sensor()`); `SensorTemperatura/Presion/Caudal` con `override`. |
| **3. TAD genérico y Heap** | `ListaGen<T>` con templates; `~ListaGen()` hace `delete` del dato (si `T` es puntero) y del nodo. Copia deshabilitada para evitar doble `delete`. Usada como `ListaGen<Sensor*>` y `ListaGen<MuestraSensor>`. |
| **4. Excepciones** | Fallos de socket/bind (puerto ocupado)/listen/accept → `throw std::runtime_error(...)`, capturados en `main()`. Tramas inválidas → `ExcepcionTrama` (deriva de `std::runtime_error`), capturada en el servidor. |

## Preguntas típicas de sustentación

- **¿Por qué `virtual ~Sensor()`?** Al hacer `delete` sobre un `Sensor*` que apunta a un `SensorTemperatura`, la vtable asegura que se llame primero el destructor de la derivada y luego el de la base. Sin `virtual` el comportamiento es indefinido y hay fuga.
- **¿Qué es la vtable?** Tabla de punteros a funciones virtuales por clase; cada objeto con métodos virtuales guarda un puntero oculto (vptr) a la de su clase. `procesarLectura()` se resuelve en ejecución por ese puntero (enlace dinámico).
- **¿Por qué `Sensor` es abstracta?** Tiene un método virtual puro; no se puede instanciar, solo sirve como interfaz común.
- **¿Qué hace `htons`?** Convierte el puerto al orden de bytes de red (big-endian).
- **¿Qué hace `SO_REUSEADDR`?** Permite volver a enlazar el puerto de inmediato tras cerrar el servidor.
- **¿Por qué `accept()` devuelve otro descriptor?** El de `listen` sigue escuchando; el nuevo es la sesión dedicada con ese cliente.
- **¿Qué pasa si el puerto está ocupado?** `bind()` devuelve -1 con `errno = EADDRINUSE`; se lanza `std::runtime_error` y `main()` lo captura con un mensaje amigable.
- **¿Cómo se evitan los memory leaks?** Cada `new SensorXxx` se inserta en `ListaGen`, que es dueña de los objetos; al salir de `main()` su destructor libera todo. Si la inserción fallara, el servidor hace `delete` antes de propagar la excepción.

## Referencias

- Comer, D. E. (2014). *Computer Networks and Internets* (6th ed.). Pearson Education.
- Deitel, P., & Deitel, H. (2017). *C++ How to Program* (10th ed.). Pearson Education.
- Kurose, J. F., & Ross, K. W. (2021). *Computer Networking: A Top-Down Approach* (8th ed.). Pearson.
- Lippman, S. B., Lajoie, J., & Moo, B. E. (2013). *C++ Primer* (5th ed.). Addison-Wesley.
- Stevens, W. R., Fenner, B., & Rudoff, A. M. (2004). *UNIX Network Programming, Volume 1: The Sockets Networking API* (3rd ed.). Addison-Wesley.
- Stroustrup, B. (2013). *The C++ Programming Language* (4th ed.). Addison-Wesley.
