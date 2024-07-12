#include <Arduino.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <DHT.h>

const int trigPin = 9;    // Pin del sensor ultrasónico TRIG
const int echoPin = 10;   // Pin del sensor ultrasónico ECHO
const int buzzerPin = 12;  // Pin del buzzer
const int servoPin1 = 6;  // Pin del primer servomotor
const int servoPin2 = 5;  // Pin del segundo servomotor
const int dhtPin = 4;     // Pin del sensor de temperatura DHT11
const int ledPin = 8;     // Pin del LED que emula la bomba de agua
const int sensorHumedadPin = A0;  // Pin del sensor de humedad del suelo

Servo servoMotor1;  // Objeto del primer servomotor
Servo servoMotor2;  // Objeto del segundo servomotor
SoftwareSerial sim800l(2, 3);  // RX, TX para el SIM800L

DHT dht(dhtPin, DHT11);  // Objeto del sensor DHT11

bool alertaEnviada = false;

void setup() {
  // Inicializar el sensor ultrasónico
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Inicializar el buzzer
  pinMode(buzzerPin, OUTPUT);
  
  // Inicializar los servomotores
  servoMotor1.attach(servoPin1);
  servoMotor2.attach(servoPin2);

  // Inicializar el LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Apagar el LED al inicio

  // Inicializar el sensor de humedad del suelo
  pinMode(sensorHumedadPin, INPUT);

  // Inicializar comunicación con SIM800L
  sim800l.begin(9600);
  
  // Configurar baud rate del SIM800L
  sim800l.println("AT+IPR=9600");
  delay(1000);
  while (sim800l.available()) {
    Serial.write(sim800l.read());
  }
  
  // Inicializar sensor de temperatura DHT11
  dht.begin();
  
  Serial.begin(9600);  // Iniciar comunicación serial
  Serial.println("Iniciando sistema...");
}

void loop() {
  // Leer la distancia medida por el sensor ultrasónico
  float distancia = leerDistancia();
  
  // Calcular el porcentaje de agua en el embalse
  int porcentaje = calcularPorcentaje(distancia);
  
  // Mostrar porcentaje de agua por Serial
  Serial.print("Porcentaje: ");
  Serial.print(porcentaje);
  Serial.println("%");
  
  // Leer temperatura y humedad del sensor DHT11
  float temperatura = leerTemperatura();
  float humedad = leerHumedad();
  
  // Mostrar temperatura y humedad por Serial
  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println(" %");
  
  // Leer humedad del suelo
  int humedadSuelo = leerHumedadSuelo();
  Serial.print("Humedad del suelo: ");
  Serial.println(humedadSuelo);
  
  // Controlar los servomotores según el porcentaje de agua
  controlarServomotor(porcentaje);

  // Controlar la bomba de agua (LED) según la humedad del suelo
  controlarBomba(humedadSuelo);
  
  // Enviar mensaje y realizar llamada si es necesario
  gestionarAlarma(porcentaje, distancia, temperatura, humedad);
  
  // Esperar un tiempo antes de la próxima lectura
  delay(1000);
}

float leerDistancia() {
  // Enviar un pulso al pin TRIG
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Leer la duración del pulso desde el pin ECHO
  float duracion = pulseIn(echoPin, HIGH);
  
  // Convertir la duración en distancia (cm)
  float distancia = duracion * 0.034 / 2;
  
  // Mostrar distancia por Serial
  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.println(" cm");
  
  return distancia;
}

int calcularPorcentaje(float distancia) {
  // Calcular el porcentaje de agua en el embalse
  // Ajusta estos valores según las dimensiones de tu embalse
  int alturaTotal = 30;            // Altura total del embalse en cm
  int alturaAgua = alturaTotal - distancia; // Altura del agua en el embalse
  
  // Asegurarse de que la altura del agua nunca sea mayor que la altura total
  if (alturaAgua < 0) {
    alturaAgua = 0;
  }
  
  int porcentaje = (alturaAgua * 100) / alturaTotal;
  
  return porcentaje;
}

float leerTemperatura() {
  // Leer temperatura del sensor DHT11
  float temperatura = dht.readTemperature();
  return temperatura;
}

float leerHumedad() {
  // Leer humedad del sensor DHT11
  float humedad = dht.readHumidity();
  return humedad;
}

int leerHumedadSuelo() {
  // Leer valor analógico del sensor de humedad del suelo
  int valor = analogRead(sensorHumedadPin);
  return valor;
}

void controlarBomba(int humedadSuelo) {
  // Definir umbrales de humedad del suelo
  int umbralSeco = 600;   // Ajustar según el sensor específico
  int umbralHumedo = 400; // Ajustar según el sensor específico

  if (humedadSuelo >= umbralSeco) {
    // Si el suelo está seco, encender el LED
    digitalWrite(ledPin, HIGH);
    Serial.println("LED encendido (emulando bomba de agua).");
  } else if (humedadSuelo <= umbralHumedo) {
    // Si el suelo está suficientemente húmedo, apagar el LED
    digitalWrite(ledPin, LOW);
    Serial.println("LED apagado (emulando bomba de agua).");
  }
}

void controlarServomotor(int porcentaje) {
  // Calcular el ángulo del servomotor1 según el porcentaje de agua
  int anguloServo1;
  if (porcentaje <= 80) {
    anguloServo1 = 90;  // Si el porcentaje es igual o menor a 80%, el servo1 está en 90 grados
  } else {
    anguloServo1 = 0;   // Si el porcentaje es mayor a 80%, el servo1 está en 0 grados
  }
  
  // Calcular el ángulo del servomotor2 según el porcentaje de agua
  int anguloServo2;
  if (porcentaje <= 80) {
    anguloServo2 = 90;  // Si el porcentaje es igual o menor a 80%, el servo2 está en 90 grados
  } else {
    anguloServo2 = 0;   // Si el porcentaje es mayor a 80%, el servo2 está en 0 grados
    // Activar el buzzer si el porcentaje es mayor a 80%
    activarAlerta();
  }
  
  // Mover los servomotores a los ángulos calculados
  servoMotor1.write(anguloServo1);
  servoMotor2.write(anguloServo2);
  
  // Desactivar el buzzer si el porcentaje es menor o igual a 80%
  if (porcentaje <= 80) {
    desactivarAlerta();
  }
}

void activarAlerta() {
  // Activar el buzzer para sonar la alerta
  tone(buzzerPin, 1000); // Puedes ajustar la frecuencia del tono según sea necesario
  
  // Mostrar alerta por Serial
  Serial.println("¡Alerta de inundación activada!");
}
  
void desactivarAlerta() {
  // Detener el sonido del buzzer
  noTone(buzzerPin);
}

void gestionarAlarma(int porcentaje, float distancia, float temperatura, float humedad) {
  // Si el porcentaje es mayor o igual al 90% y la alerta no ha sido enviada aún
  if (porcentaje >= 90 && !alertaEnviada) {
    // Realizar llamada
    realizarLlamada();
    
    // Esperar un tiempo para asegurar que la llamada se procese correctamente
    delay(10000);  // Ajustar según la duración esperada de la llamada
    
    // Enviar mensaje SMS
    enviarSMS(distancia, temperatura, humedad);
    
    // Marcar que la alerta ha sido enviada
    alertaEnviada = true;
  } else if (porcentaje < 90) {
    // Resetear la alerta cuando el porcentaje baje de 90%
    alertaEnviada = false;
  }
}

void realizarLlamada() {
  // Realizar una llamada
  sim800l.println("ATD+51NUMERO;");  // Número de celular en formato internacional
  delay(30000);  // Esperar 30 segundos (ajustar según la duración de la llamada)
  sim800l.println("ATH");  // Cortar la llamada
  delay(1000);  // Asegurar que el comando ATH sea procesado
  
  // Mostrar llamada realizada por Serial
  Serial.println("Llamada realizada.");
  
  // Leer y mostrar la respuesta del SIM800L por Serial
  while (sim800l.available()) {
    Serial.write(sim800l.read());
  }
  Serial.println();
}

void enviarSMS(float distancia, float temperatura, float humedad) {
  // Mensaje de texto a enviar
  String mensaje = "Alerta de inundacion! Nivel de agua: " + String(distancia) + " cm. ";
  mensaje += "Temperatura actual: " + String(temperatura) + " °C. ";
  mensaje += "Humedad actual: " + String(humedad) + " %.";
  
  // Configurar SIM800L para enviar mensaje SMS
  sim800l.println("AT+CMGF=1");  // Configurar para enviar SMS en modo texto
  delay(1000);
  sim800l.println("AT+CMGS=\"+51NUMERO\"");  // Número de celular en formato internacional
  delay(1000);
  sim800l.print(mensaje);
  delay(1000);
  sim800l.write(26);  // Enviar Ctrl+Z para finalizar el mensaje
  delay(5000);  // Asegurar suficiente tiempo para enviar el mensaje
  
  // Leer y mostrar la respuesta del SIM800L por Serial
  while (sim800l.available()) {
    Serial.write(sim800l.read());
  }
  Serial.println();
  
  // Mostrar mensaje enviado por Serial
  Serial.println("Mensaje SMS enviado.");
}
