#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "BluetoothSerial.h"

// ==========================================
// CONFIGURACIÓN INICIAL
// ==========================================

BluetoothSerial SerialBT;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Configuración de Servos (Ajustar según tus servos MG90S)
#define SERVOMIN 150
#define SERVOMAX 600
#define SERVO_FREQ 50

// Pines Ultrasónico
const int trigPin = 5;
const int echoPin = 18;

// --- ASIGNACIÓN DE CANALES PCA9685 ---
// Mapeo de patas a canales del controlador
#define CH_Leg1F 0
#define CH_Leg1B 1
#define CH_Leg2F 2
#define CH_Leg2B 3
#define CH_Leg3F 4
#define CH_Leg3B 5
#define CH_Leg4F 6
#define CH_Leg4B 7

// ==========================================
// --- CALIBRACIÓN / OFFSETS ---
// ==========================================
// Ajuste fino para centrar las patas sin desarmar el robot.
// Orden: {Ch0, Ch1, Ch2, Ch3, Ch4, Ch5, Ch6, Ch7}
int servoOffsets[8] = {
    0,   // Canal 0
    0,   // Canal 1
    0,   // Canal 2
    0,   // Canal 3
    -30, // Canal 4 (Correccion debido a montaje)
    0,   // Canal 5
    0,   // Canal 6
    0    // Canal 7
};

// Variables Globales de Control
boolean stringComplete = false;
boolean stopcmd = false;
String inputString;
String reccmd;
String othercmd;

// Variables de Movimiento
int selservo, rotate1, rotate2, rotate3, rotate4;
int leftdistance, rightdistance;
long duration, distance;

// Posiciones actuales de los servos
float LALeg1F, LALeg1B, LALeg2F, LALeg2B;
float LALeg3F, LALeg3B, LALeg4F, LALeg4B;
float LAHeadservo;

// Objetivos de posición
int TOLeg1F, TOLeg1B, TOLeg2F, TOLeg2B;
int TOLeg3F, TOLeg3B, TOLeg4F, TOLeg4B;
int TOHeadservo;

// Variables de Estado
boolean autorun = false;
int autostep;
boolean fromauto = false;
boolean smoothrun = true;
int smoothdelay = 2;
int maxstep = 0;

// Variables de pasos
int totalforwardsteps = 50;
int totalsidesteps = 25;
int moveforwardsteps = 0;
int movesidesteps = 0;
int forwardsteps = 0;
int walkstep = 1;
int walkstep2;

float stepLeg1F, stepLeg1B, stepLeg2F, stepLeg2B;
float stepLeg3F, stepLeg3B, stepLeg4F, stepLeg4B;

// Altura y secuencia de caminata
int Fheight = 5;
int Bheight = 5;
int heightchange;

// Matrices de Caminata (Cinemática simplificada)
int walkF[][7] = {
    {124, 146, 177, 150, 132, 115, 115},
    {94, 132, 178, 139, 112, 84, 84},
    {37, 112, 179, 139, 95, 42, 42},
    {22, 95, 150, 115, 78, 30, 30},
    {11, 78, 124, 92, 59, 13, 13},
    {13, 59, 92, 58, 36, 2, 2}};
int walkB[][7] = {
    {3, 34, 56, 65, 48, 30, 30},
    {2, 48, 86, 96, 68, 41, 41},
    {1, 68, 143, 138, 85, 41, 41},
    {30, 85, 158, 150, 102, 65, 65},
    {56, 102, 169, 167, 121, 88, 88},
    {88, 121, 167, 178, 144, 122, 122}};

// ==========================================
// FUNCIONES PRINCIPALES
// ==========================================

void moveServo(int channel, float degrees)
{
  // 1. Aplicar Offset
  if (channel >= 0 && channel < 8)
  {
    degrees = degrees + servoOffsets[channel];
  }
  // 2. Limites de seguridad
  if (degrees > 150)
    degrees = 150;
  if (degrees < 0)
    degrees = 0;

  // 3. Mapeo PWM
  int pulse = map(degrees, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(channel, 0, pulse);
}

void setup()
{
  Serial.begin(115200);
  SerialBT.begin("SpiderBot_ESP32");
  Serial.println("Bluetooth iniciado. Conectate a 'SpiderBot_ESP32'");

  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);

  // Posición inicial
  LALeg1F = 80;
  LALeg1B = 100;
  LALeg2F = 100;
  LALeg2B = 80;
  LALeg3F = 80;
  LALeg3B = 100;
  LALeg4F = 100;
  LALeg4B = 80;
  LAHeadservo = 90;

  TOLeg1F = LALeg1F;
  TOLeg1B = LALeg1B;
  TOLeg2F = LALeg2F;
  TOLeg2B = LALeg2B;
  TOLeg3F = LALeg3F;
  TOLeg3B = LALeg3B;
  TOLeg4F = LALeg4F;
  TOLeg4B = LALeg4B;
  TOHeadservo = LAHeadservo;

  delay(1000);
  Servomovement();
  delay(2000);
}

void readInput()
{
  char inChar = 0;
  bool dataReceived = false;

  if (Serial.available())
  {
    inChar = (char)Serial.read();
    dataReceived = true;
  }
  else if (SerialBT.available())
  {
    inChar = (char)SerialBT.read();
    dataReceived = true;
  }

  if (dataReceived)
  {
    // Modo Calibración (90 grados)
    if (inChar == '9')
    {
      TOLeg1F = 90;
      TOLeg1B = 90;
      TOLeg2F = 90;
      TOLeg2B = 90;
      TOLeg3F = 90;
      TOLeg3B = 90;
      TOLeg4F = 90;
      TOLeg4B = 90;
      TOHeadservo = 90;
      Servomovement();
      reccmd = "S";
      return;
    }

    if (inChar == 'A')
    {
      autorun = true;
      autostep = 0;
    }
    else if (inChar == 'M')
    {
      autorun = false;
    }

    if (autorun == false)
    {
      if (inChar == 'F' || inChar == 'B' || inChar == 'L' || inChar == 'R' || inChar == 'G')
      {
        reccmd = String(inChar);
      }
      else if (inChar == 'S')
      {
        stopcmd = true;
      }
      else if (inChar == 'C' || inChar == 'V')
      {
        reccmd = String(inChar); // C=Check, V=Saludo
      }
      else if (String("UO").indexOf(inChar) != -1)
      {
        othercmd = String(inChar);
        heightchange = 1;
      }
      else if (String("DP").indexOf(inChar) != -1)
      {
        othercmd = String(inChar);
        heightchange = -1;
      }
    }
  }
}

void loop()
{
  readInput();

  // Lógica Modo Automático
  if (autorun)
  {
    if (!fromauto)
      fromauto = true;

    if (autostep == 0)
    {
      Fheight = 5;
      Bheight = 5;
      autostep = 1;
      forwardsteps = 0;
      changeheight();
    }
    else if (autostep == 1)
    { // Caminar adelante
      Distancecal();
      if (distance < 15)
      { // Distancia de seguridad aumentada a 15cm
        reccmd = "S";
        autostep = 2;
      }
      else
      {
        reccmd = "F";
      }
    }
    else if (autostep == 2)
    { // Detener y decidir
      // Lógica de bajada de altura simplificada para brevedad
      autostep = 3;
    }
    else if (autostep == 3)
    { // Escanear entorno
      scanSurroundings();
    }
    else if (autostep == 4)
    { // Girar
      movesidesteps++;
      if (movesidesteps > totalsidesteps)
        autostep = 1;
    }
  }
  else if (fromauto)
  {
    reccmd = "S";
    fromauto = false;
  }

  // Ejecución de Comandos de Movimiento
  if (String("FBLRGIHJ").indexOf(reccmd) != -1)
  {
    executeWalkSequence();

    if (stopcmd && walkstep == 4)
    {
      reccmd = "S";
      stopcmd = false;
    }
  }
  else if (heightchange != 0)
  {
    changeheight();
  }
  else if (reccmd == "C")
  {
    selfcheck();
    reccmd = "S";
  }
  else if (reccmd == "V")
  {
    sayhai();
    reccmd = "S";
  }
}

// --- Rutinas de Movimiento ---

void executeWalkSequence()
{
  // Actualizar paso
  if (String("FLGI").indexOf(reccmd) != -1)
  {
    walkstep++;
    if (walkstep > 7)
      walkstep = 1;
  }
  else
  {
    walkstep--;
    if (walkstep < 1)
      walkstep = 7;
  }

  walkstep2 = walkstep + 3;
  if (walkstep2 > 7)
    walkstep2 -= 7;
  if (walkstep2 < 1)
    walkstep2 += 7;

  // Asignar angulos según matriz
  if (reccmd == "F" || reccmd == "B")
  {
    rotate1 = walkF[Fheight][walkstep - 1];
    rotate2 = walkB[Fheight][walkstep - 1];
    rotate3 = walkF[Bheight][walkstep - 1];
    rotate4 = walkB[Bheight][walkstep - 1];

    TOLeg1F = rotate1;
    TOLeg1B = rotate2;
    TOLeg4F = 180 - rotate3;
    TOLeg4B = 180 - rotate4;
    Servomovement();

    rotate1 = walkF[Fheight][walkstep2 - 1];
    rotate2 = walkB[Fheight][walkstep2 - 1];
    rotate3 = walkF[Bheight][walkstep2 - 1];
    rotate4 = walkB[Bheight][walkstep2 - 1];

    TOLeg2F = 180 - rotate1;
    TOLeg2B = 180 - rotate2;
    TOLeg3F = rotate3;
    TOLeg3B = rotate4;
    Servomovement();
  }
  else if (reccmd == "L" || reccmd == "R")
  {
    // Lógica de giro
    rotate1 = walkF[Fheight][walkstep - 1];
    rotate2 = walkB[Fheight][walkstep - 1];
    rotate3 = walkF[Bheight][walkstep - 1];
    rotate4 = walkB[Bheight][walkstep - 1];

    TOLeg1F = rotate1;
    TOLeg1B = rotate2;
    TOLeg4F = rotate4;
    TOLeg4B = rotate3; // Invertido para giro
    Servomovement();

    rotate1 = walkF[Fheight][(8 - walkstep) - 1];
    rotate2 = walkB[Fheight][(8 - walkstep) - 1];
    rotate3 = walkF[Bheight][(8 - walkstep) - 1];
    rotate4 = walkB[Bheight][(8 - walkstep) - 1];

    TOLeg2F = 180 - rotate1;
    TOLeg2B = 180 - rotate2;
    TOLeg3F = 180 - rotate4;
    TOLeg3B = 180 - rotate3;
    Servomovement();
  }
}

void scanSurroundings()
{
  smoothdelay = 8;
  TOHeadservo = 0;
  Servomovement();
  Distancecal();
  delay(100);
  rightdistance = distance;
  TOHeadservo = 180;
  Servomovement();
  Distancecal();
  delay(100);
  leftdistance = distance;
  TOHeadservo = 90;
  Servomovement();
  smoothdelay = 2;

  if (leftdistance > rightdistance)
    reccmd = "L";
  else
    reccmd = "R";
  movesidesteps = 0;
  autostep = 4;
}

void sayhai()
{
  // Posición base
  TOLeg1F = 0;
  TOLeg1B = 180;
  TOLeg2F = 180;
  TOLeg2B = 0;
  TOLeg3F = 180;
  TOLeg3B = 0;
  TOLeg4F = 0;
  TOLeg4B = 180;
  TOHeadservo = 90;
  Servomovement();

  // Saludo
  for (int i = 1; i <= 3; i++)
  {
    delay(400);
    TOLeg1F = 60;
    TOHeadservo = 135;
    Servomovement();
    delay(400);
    TOLeg1F = 100;
    TOHeadservo = 45;
    Servomovement();
  }
  // Retorno
  TOLeg1F = 0;
  TOHeadservo = 90;
  Servomovement();
}

void selfcheck()
{
  // Secuencia de prueba rápida
  TOHeadservo = 90;
  TOLeg1F = 0;
  TOLeg1B = 180;
  Servomovement();
  delay(300);
  TOLeg1F = 180;
  TOLeg1B = 0;
  Servomovement();
  delay(300);
  // ... (Simplificado para el ejemplo)
}

void changeheight()
{
  // Ajuste de altura basado en heightchange
  if (othercmd == "O")
  {
    Fheight = 5;
    Bheight = 5;
  }
  // Recalcula posiciones estáticas
  rotate1 = walkF[Fheight][4];
  rotate2 = walkB[Fheight][4];
  TOLeg1F = rotate1;
  TOLeg1B = rotate2;
  TOLeg2F = 180 - rotate1;
  TOLeg2B = 180 - rotate2;

  rotate1 = walkF[Bheight][4];
  rotate2 = walkB[Bheight][4];
  TOLeg3F = rotate1;
  TOLeg3B = rotate2;
  TOLeg4F = 180 - rotate1;
  TOLeg4B = 180 - rotate2;
  Servomovement();
}

void Servomovement()
{
  if (smoothrun)
    smoothmove();

  LALeg1F = TOLeg1F;
  LALeg1B = TOLeg1B;
  LALeg2F = TOLeg2F;
  LALeg2B = TOLeg2B;
  LALeg3F = TOLeg3F;
  LALeg3B = TOLeg3B;
  LALeg4F = TOLeg4F;
  LALeg4B = TOLeg4B;
  LAHeadservo = TOHeadservo;

  moveServo(CH_Leg1F, TOLeg1F);
  moveServo(CH_Leg1B, TOLeg1B);
  moveServo(CH_Leg2F, TOLeg2F);
  moveServo(CH_Leg2B, TOLeg2B);
  moveServo(CH_Leg3F, TOLeg3F);
  moveServo(CH_Leg3B, TOLeg3B);
  moveServo(CH_Leg4F, TOLeg4F);
  moveServo(CH_Leg4B, TOLeg4B);
}

void smoothmove()
{
  // Interpolación lineal para movimiento suave
  maxstep = 0;
  // Calcular la diferencia máxima entre posición actual y objetivo
  float diffs[] = {
      abs(LALeg1F - TOLeg1F), abs(LALeg1B - TOLeg1B),
      abs(LALeg2F - TOLeg2F), abs(LALeg2B - TOLeg2B),
      abs(LALeg3F - TOLeg3F), abs(LALeg3B - TOLeg3B),
      abs(LALeg4F - TOLeg4F), abs(LALeg4B - TOLeg4B)};

  for (int i = 0; i < 8; i++)
    if (diffs[i] > maxstep)
      maxstep = diffs[i];

  if (maxstep > 0)
  {
    stepLeg1F = (TOLeg1F - LALeg1F) / maxstep;
    stepLeg1B = (TOLeg1B - LALeg1B) / maxstep;
    stepLeg2F = (TOLeg2F - LALeg2F) / maxstep;
    stepLeg2B = (TOLeg2B - LALeg2B) / maxstep;
    stepLeg3F = (TOLeg3F - LALeg3F) / maxstep;
    stepLeg3B = (TOLeg3B - LALeg3B) / maxstep;
    stepLeg4F = (TOLeg4F - LALeg4F) / maxstep;
    stepLeg4B = (TOLeg4B - LALeg4B) / maxstep;

    for (int i = 0; i <= maxstep; i++)
    {
      LALeg1F += stepLeg1F;
      LALeg1B += stepLeg1B;
      LALeg2F += stepLeg2F;
      LALeg2B += stepLeg2B;
      LALeg3F += stepLeg3F;
      LALeg3B += stepLeg3B;
      LALeg4F += stepLeg4F;
      LALeg4B += stepLeg4B;

      moveServo(CH_Leg1F, LALeg1F);
      moveServo(CH_Leg1B, LALeg1B);
      moveServo(CH_Leg2F, LALeg2F);
      moveServo(CH_Leg2B, LALeg2B);
      moveServo(CH_Leg3F, LALeg3F);
      moveServo(CH_Leg3B, LALeg3B);
      moveServo(CH_Leg4F, LALeg4F);
      moveServo(CH_Leg4B, LALeg4B);

      delay(smoothdelay);
    }
  }

  // Movimiento cabeza independiente
  if (LAHeadservo != TOHeadservo)
  {
    int dir = (LAHeadservo > TOHeadservo) ? -1 : 1;
    while (abs(LAHeadservo - TOHeadservo) > 0)
    {
      LAHeadservo += dir;
      // Aquí deberías mover el servo de la cabeza si tuviera un canal asignado en moveServo
      // moveServo(CH_Head, LAHeadservo);
      delay(smoothdelay);
    }
  }
}

void Distancecal()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
}