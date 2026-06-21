# 🛠️ Prometheus MK-IV: Filosofía de Diseño y Arquitectura Hardware

Este documento detalla las decisiones técnicas de ingeniería detrás de la construcción física y electrónica del coche Prometheus MK-IV.

## 1. Arquitectura Electrónica: El Sistema de Doble Batería

Una de las decisiones más críticas en el diseño de Prometheus ha sido la implementación de un sistema de **doble alimentación**. Aislar el circuito de tracción pesada es fundamental para evitar caídas de tensión bruscas que reiniciarían el ESP32 y cortarían la conexión Bluetooth en plena misión.

**La Solución Implementada:**

- **Batería A (Lógica y Servomotores):** 
  - **Especificaciones:** Pack de celdas **18650** con una capacidad de **6600mAh**. Su voltaje base (~3.5V) atraviesa un módulo elevador (*Boost/Step-up*) que garantiza una entrega de **5V** fijos, estables y limpios.
  - **Función:** Alimenta de forma centralizada al microcontrolador principal, a la sensórica y a los servomotores.
  - **Desglose de Consumos:** 
    - *Microcontrolador:* Exige un flujo constante de entre **150mA y 240mA** debido al procesamiento y la transmisión inalámbrica Bluetooth ininterrumpida.
    - *Sensores:* Consumo residual inferior a **10mA**.
    - *Servomotores:* El consumo es altamente dinámico. En movimiento, un servo estándar demanda entre **500mA y 800mA**, pero bajo un esfuerzo máximo o atasco (Stall) puede chupar picos de **1.5A a 2.5A** de golpe. Dependiendo de la configuración del vehículo, el consumo se multiplica: la versión de carreras solo alimenta un servo (dirección), mientras que la versión pesada activa hasta 4 servomotores simultáneamente.
  - **Cálculo Realista de Autonomía:** Teniendo en cuenta todo este estrés combinado y el salto de voltaje del regulador, la reserva de 6600mAh estabiliza el sistema y proporciona unas reales y sólidas **3 horas de autonomía** de trabajo intenso ininterrumpido, evitando en todo momento que el microcontrolador sufra apagones.

- **Batería B (Tracción Bruta):** 
  - **Especificaciones:** Batería **LiPo de 7.5V y 2200mAh**.
  - **Función:** Dedicada **en exclusiva** al controlador de potencia y a los motores de tracción de las ruedas.
  - **Desglose de Consumos:** 
    - *Controlador de potencia:* El propio módulo gestor presenta una pequeña pérdida interna por caída de tensión y conmutación, rondando los **20mA a 35mA** de consumo base.
    - *Motores de tracción:* A una velocidad de crucero plana y constante, consumen entre **200mA y 400mA** por rueda. Sin embargo, la exigencia real llega en los arranques desde parado o al atascarse contra obstáculos: en esas fracciones de segundo, cada motor exige violentos latigazos de **más de 1A a 1.5A** para vencer la inercia del vehículo.
  - **Cálculo Realista de Autonomía:** Aunque los motores exigen grandes picos de corriente momentáneos, en un escenario real de conducción (intercalando aceleraciones, maniobras, derrapes y pausas), el consumo medio continuo se estabiliza. Gracias a la eficiencia y a la tasa de descarga de la LiPo de 2200mAh, el sistema está verificado en pista para proporcionar sin problemas unas **2 horas de conducción** pura e intensa.

*(Nota Técnica: Las líneas de masa / GND de ambas baterías están obligatoriamente unificadas en el circuito para establecer una referencia común. Esto es indispensable para que los pulsos de señal PWM enviados por el ESP32 sean interpretados correctamente por los drivers del motor).*

## 2. Ingeniería de Chasis 3D

El chasis del Prometheus MK-IV ha sido diseñado combinando máxima dureza con optimizaciones avanzadas de fabricación aditiva para resistir entornos de competición.

*   **Tornillería y Ensamblaje:** El montaje del coche hace uso intensivo de tornillería métrica de precisión (**M2, M2.5 y M3**) para fijar estructuralmente todas las piezas, asegurar los motores y consolidar las articulaciones mecánicas frente a fuertes vibraciones.
*   **Geometría Estructural (Relleno Giroide):** Para soportar la tensión extrema sin añadir un peso masivo, las piezas se imprimen utilizando un patrón de relleno interno **Giroide** (*Gyroid*). Este patrón matemático distribuye la fuerza de forma tridimensional, absorbiendo energía y multiplicando la resistencia mecánica del plástico en todas las direcciones (X, Y, Z).
*   **Material Ecológico Llevado al Límite (PLA):** Todo el chasis y las piezas mecánicas están fabricados exclusivamente en **PLA** (Poliácido Láctico). Esta decisión hace que el coche sea una alternativa **ecológica y biodegradable**. Dado que el PLA es un plástico rígido pero frágil por naturaleza, la ingeniería y geometría de las piezas ha sido calculada al milímetro para empujar el material **hasta el límite absoluto de las propiedades que ofrece**, logrando rendimiento y resistencia de competición utilizando un plástico de origen orgánico.
*   **Impresión con Soportes y Zonas de Sacrificio:** Debido a la complejidad de las piezas mecánicas del vehículo, el diseño requiere el uso de estructuras de soporte durante la impresión 3D. Los grosores de pared, el número de perímetros y la densidad del relleno Giroide no pretenden hacer el coche "indestructible". De hecho, si el coche sufre un impacto directo o un vuelco a la máxima velocidad, las piezas exteriores sí van a partirse y fracturarse. Esto es una brillante decisión de diseño pensada para absorber la energía cinética del choque, sacrificando el plástico impreso (barato y reemplazable) con el fin de **prevenir que los sistemas críticos (la placa ESP32, los motores y la sensórica) reciban daños irreparables**.
*   **Requisitos de Impresión (Gran Formato):** A diferencia de modelos a pequeña escala, Prometheus es un vehículo de gran envergadura. La pieza individual más grande del chasis ocupa la práctica totalidad de una plancha de impresión de **250 x 250 mm**. Por tanto, su fabricación requiere impresoras 3D con un volumen de construcción intermedio o superior, asegurando así que las piezas estructurales más críticas se impriman de una sola pieza, garantizando su integridad sin necesidad de pegamentos ni uniones frágiles.

## 3. Dinámica y Manipulación

*   **Control Independiente de Servo:** En lugar de depender exclusivamente de la dirección tipo tanque (derrape diferencial), Prometheus integra un servo central (Pin 16) que gobierna el ángulo de ataque direccional. Esto permite trazadas mucho más limpias a alta velocidad (RACE MODE).
*   **Aislamiento de Timers PWM:** A nivel de software, los servos de manipulación (Brazo y Compuerta) se han asignado a timers independientes del ESP32 para evitar solapamientos de pulsos eléctricos y asegurar máxima fuerza de sujeción (Holding Torque) cuando el coche carga peso estático.
