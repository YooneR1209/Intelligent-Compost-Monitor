#include <DHT.h>
#include <ESP32Servo.h>

// ---------------- DHT11 ----------------
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ---------------- Servo ----------------
Servo servoMotor;
#define SERVO_PIN 10

// ---------------- LEDs ----------------
#define LED_VERDE 20
#define LED_ROJO 21

// ---------------- Queue ----------------
QueueHandle_t colaTemp;

float temperaturaActual = 0;

// =================================================
// TAREA SENSOR
// =================================================
void tareaSensor(void *pvParameters) {

  float temp;

  while (true) {

    temp = dht.readTemperature();

    if (!isnan(temp)) {

      xQueueSend(
        colaTemp,
        &temp,
        portMAX_DELAY
      );
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// =================================================
// TAREA SERVO + LEDS
// =================================================
void tareaControl(void *pvParameters) {

  float temp;

  int pos = 0;
  bool subir = true;

  while (true) {

    if (xQueueReceive(
          colaTemp,
          &temp,
          pdMS_TO_TICKS(100)
        )) {

      temperaturaActual = temp;
    }

    if (temperaturaActual > 35) {

      digitalWrite(LED_ROJO, HIGH);
      digitalWrite(LED_VERDE, LOW);

      if (subir) {

        pos += 15;

        if (pos >= 180) {
          pos = 180;
          subir = false;
        }

      } else {

        pos -= 15;

        if (pos <= 0) {
          pos = 0;
          subir = true;
        }
      }

      servoMotor.write(pos);

      vTaskDelay(pdMS_TO_TICKS(250));

    } else {

      digitalWrite(LED_ROJO, LOW);
      digitalWrite(LED_VERDE, HIGH);

      servoMotor.write(0);

      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

// =================================================
// TAREA MONITOR SERIAL
// =================================================
void tareaSerial(void *pvParameters) {

  while (true) {

    Serial.print("Temp: ");
    Serial.print(temperaturaActual, 1);
    Serial.println(" C");

    // --- CAMBIO AQUÍ: De 35 para que coincida con los LEDs ---
    if (temperaturaActual > 35) {
      Serial.println("Estado: TEMP ALTA");
    } else {
      Serial.println("Estado: TEMP NORMAL");
    }
    // ----------------------------------------------------------------

    Serial.println("-----------------------");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// =================================================
// SETUP
// =================================================
void setup() {

  Serial.begin(115200);
  
  // PRUEBA: Imprimimos algo inmediatamente
  Serial.println(" ");
  Serial.println("!!! EL ESP32 SE HA ENCENDIDO !!!");
  delay(1000); // Le damos un segundo para que llegue el mensaje

  dht.begin();

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  servoMotor.attach(SERVO_PIN);

  colaTemp = xQueueCreate(
    5,
    sizeof(float)
  );

  xTaskCreate(
    tareaSensor,
    "Sensor",
    2048,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    tareaControl,
    "Control",
    4096,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    tareaSerial,
    "Serial",
    2048,
    NULL,
    1,
    NULL
  );
}

void loop() {
}