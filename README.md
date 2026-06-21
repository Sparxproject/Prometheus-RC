# 🏎️ Prometheus RC Rover

**Creador:** Ernesto Torregrosa Palacio (Exalumno CEU)

## 📖 Descripción del Proyecto
Este repositorio contiene el código fuente oficial para el sistema de control del rover radiocontrol **Prometheus (Versión MK-IV)**. El sistema utiliza un microcontrolador **ESP32** para conectarse de forma nativa a un mando inalámbrico de **Xbox Series X/S o Xbox One** mediante Bluetooth, ofreciendo un control de altísima precisión.

El código base está dividido en tres módulos independientes dependiendo de la misión a realizar:

*   🏁 **PROMETHEUS_RACE:** Modo de competición. Elimina cualquier filtro de suavizado para ofrecer una respuesta 1:1 ultrarrápida entre los gatillos del mando y la potencia de los motores. Diseñado para maximizar la agilidad y velocidad en pista.
*   📡 **PROMETHEUS_SENSORS:** Modo de exploración. Integra lectura del entorno mediante sensores (Magnético y sensor de color I2C TCS34725). Incluye rutinas de telemetría por Bluetooth Serial para monitorizar datos del coche en tiempo real.
*   🦾 **PROMETHEUS_ARM:** Modo de manipulación. Diseñado para controlar cargas pesadas (Brazo robótico, Garra y Compuerta). Aísla las señales PWM de tracción de las de los servos para garantizar la máxima estabilidad eléctrica durante tareas de fuerza bruta, e incluye macros automáticas (como secuencias de descarga).

## ⚙️ Hardware Principal Soportado
*   Placa base: **ESP32**
*   Tracción: **Cualquier motor DC estándar** (Control mediante puente H / PWM)
*   Dirección y Manipulación: **Servomotores** (5V o estándar)
*   Periférico de control: **Mando Inalámbrico Xbox Series X/S o Xbox One** (Versiones con Bluetooth)
*   Sensores (Versión SENSORS): Módulo magnético y sensor RGB Adafruit TCS34725

---

## ⚖️ Licencia y Condiciones de Uso (CC BY-NC-SA 4.0)

Este proyecto está registrado y protegido bajo la licencia **Creative Commons Atribución - No Comercial - Compartir Igual 4.0 Internacional**. 

Al descargar, visualizar o utilizar este repositorio, aceptas las siguientes condiciones:

### ✅ QUÉ PUEDES HACER:
*   **Inspirarte y Aprender:** Eres libre de estudiar el código para entender cómo funciona la conexión ESP32-Xbox y el mapeo de control PWM.
*   **Modificar y Adaptar:** Puedes copiar el código, usar los motores que tú quieras, cambiar los pines, añadir nuevos sensores o mejorarlo para adaptarlo a tus propios proyectos de radiocontrol.
*   **Compartir:** Puedes distribuir tus versiones mejoradas libremente.

### ❌ QUÉ NO PUEDES HACER:
*   💰 **Uso Comercial:** Está **estrictamente prohibido** utilizar este código, o cualquier parte de él, para ganar dinero, vender proyectos, o darle un uso empresarial sin permiso previo del autor.
*   🕵️‍♂️ **Plagiar (Falta de Atribución):** Si presentas este código (o una versión modificada basada en él) en un entorno académico, escolar, certamen o feria de ciencias, **debes citar obligatoriamente a "Ernesto Torregrosa Palacio"** como el creador original del software base. Apropiarse del código sin dar crédito constituye una violación de la licencia y plagio académico.
*   🔒 **Cerrar el código (Compartir Igual):** Si modificas este código y publicas tu versión, estás obligado a hacerlo bajo esta misma licencia para que la comunidad pueda seguir aprendiendo de forma abierta.
