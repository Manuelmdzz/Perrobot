Perrobot - Reporte de Proyecto Final

Materia: Robótica

Proyecto: Diseño y Construcción de un Robot Cuadrúpedo

Estado: Finalizado

1. Descripción General

Este proyecto consiste en la construcción de un robot cuadrúpedo de 8 grados de libertad (DOF). El objetivo fue replicar y mejorar un diseño open-source, migrando la arquitectura de control original (basada en Arduino Uno) a un microcontrolador ESP32. Esto nos permitió eliminar módulos externos de Bluetooth, reducir el peso y mejorar la capacidad de procesamiento.

El robot es controlado inalámbricamente mediante una aplicación móvil vía Bluetooth Serial, permitiendo movimientos omnidireccionales, evasión de obstáculos y secuencias preprogramadas.

2. Ingeniería y Hardware

2.1 Diseño Mecánico y Ensamblaje (Modificaciones)

El chasis y las extremidades fueron fabricados mediante impresión 3D (archivos STL disponibles en la carpeta Hardware/).

Mejoras implementadas en el ensamblaje:
A diferencia del diseño original que sugería pegamento o tornillería simple, nosotros realizamos una modificación estructural para garantizar la durabilidad ante la vibración de los motores:

Tornillería M3 con Tuercas de Seguridad: Se sustituyeron los tornillos estándar por tornillos métricos M3 acoplados con tuercas de seguridad (con inserto de nylon). Esto evita que las articulaciones se aflojen con el movimiento constante de las patas.

Rondanas M5 como Espaciadores: Utilizamos rondanas de M5 en los ejes de rotación. Estas actúan como cojinetes simples para reducir la fricción plástica y rellenar holguras, dando mayor rigidez y estabilidad al caminar del robot.

2.2 Componentes Electrónicos

La electrónica se encuentra detallada en la carpeta Electronics/, donde se incluye el diagrama esquemático completo. A continuación, se describen los módulos clave:

Componente

Función en el Sistema

ESP32 DevKit V1

Microcontrolador principal. Gestiona la cinemática inversa y la comunicación Bluetooth.

PCA9685

Driver I2C de 16 canales. Permite controlar los 8 servos usando solo 2 pines del ESP32.

XL4016 (Step-Down)

Regulador de voltaje de alta eficiencia. Reduce el voltaje de las baterías (7.4V) a 5V estables (capacidad 8A) para alimentar los servos sin caídas de tensión.

Servos MG90S

8 servomotores de engranaje metálico.

Sensor HC-SR04

Sensor ultrasónico para la detección de obstáculos en modo automático.

2.3 Diagrama de Conexión (Bloques)

graph TD
Power[Baterías 18650 (2S)] -->|7.4V - 8.4V| XL4016
XL4016 -->|5V Regulado| PCA9685_V+
XL4016 -->|5V Regulado| ESP32_Vin

    ESP32 -->|I2C (21, 22)| PCA9685
    ESP32 -->|Pines 5, 18| Ultrasónico
    PCA9685 -->|PWM 0-7| Servos[8x MG90S]

3. Software y Control

El código fuente se encuentra en la carpeta Software/Final_Robotica/.

Lógica de Control

El sistema no utiliza cinemática inversa compleja en tiempo real, sino un sistema de tablas de marcha (Gait Tables) precalculadas que coordinan el movimiento de los servos para avanzar, retroceder y girar.

Ajustes de Calibración:
Debido a que es difícil alinear mecánicamente los dientes de los servos a 90° exactos, implementamos un arreglo de offsets en el código. Esto permite corregir la posición cero de cada motor por software sin desarmar el robot:

// Ejemplo de corrección en el código
int servoOffsets[8] = {0, 0, 0, -30, 0, 0, 0, 0};

Comandos Bluetooth

El robot se controla enviando los siguientes caracteres vía Bluetooth:

Dirección: F (Adelante), B (Atrás), L (Izquierda), R (Derecha).

Modos: A (Automático con evasión), M (Manual).

Acciones: V (Dar la pata), 9 (Posición de calibración).

4. Resultados

El robot logra caminar de manera estable gracias a la rigidez aportada por las tuercas de seguridad. El regulador XL4016 solucionó los problemas de reinicio del ESP32 que ocurrían con reguladores más pequeños, permitiendo que los 8 servos operen simultáneamente.

Las evidencias fotográficas del ensamblaje final y pruebas de funcionamiento se encuentran en la carpeta Results/.

5. Referencias

Proyecto base: Simple 3D Dog Robot (Instructables).

Librerías: Adafruit PWM Servo Driver, BluetoothSerial.

Autores:
Manuel Mendoza Guajardo
Samantha Chew Arenas
Alonso Perez Medrano
Eduardo Gabriel Sarmiento Govea

Fecha: 29/11/2025
