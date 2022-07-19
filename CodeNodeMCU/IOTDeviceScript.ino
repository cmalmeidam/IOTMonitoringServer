#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//[#INCLUIDO RETO 6]
LiquidCrystal_I2C lcd(0x27,16,2);  // 
 
//[#INCLUIDO RETO 6]
#define lighsensor A0
const float VIN = 3.3;
const int R = 10000;       //Resistencia Ω
const int LED_PIN = D0;
//------
// Ancho de la pantalla (en pixeles)
//#define SCREEN_WIDTH 128
// Alto de la pantalla (en pixeles)
//#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Pin del sensor de temperatura y humedad
#define DHTTYPE DHT11 // DHT 11
#define dht_dpin 0
DHT dht(dht_dpin, DHTTYPE);
// Intervalo en segundo de las mediciones
#define MEASURE_INTERVAL 2
// Duración aproximada en la pantalla de las alertas que se reciban
#define ALERT_DURATION 60
// Pantalla OLED
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// Cliente WiFi
WiFiClient net;
// Cliente MQTT
PubSubClient client(net);
// Variables a editar TODO

// WiFi
// Nombre de la red WiFi
const char ssid[] = "FlaAlmeida"; // TODO cambiar por el nombre de la red WiFi
// Contraseña de la red WiFi
const char pass[] = "Btatt8414+"; // TODO cambiar por la contraseña de la red WiFi
//Conexión a Mosquitto
#define USER "admin2" // TODO Reemplace UsuarioMQTT por un usuario (no administrador) que haya creado en la configuración del bróker de MQTT.
const char MQTT_HOST[] = "54.175.213.199"; // TODO Reemplace ip.maquina.mqtt por la IP del bróker MQTT que usted desplegó. Ej: 192.168.0.1
const int MQTT_PORT = 8082;
const char MQTT_USER[] = USER;
//Contraseña de MQTT
const char MQTT_PASS[] = "admin2"; // TODO Reemplace ContrasenaMQTT por la contraseña correpondiente al usuario especificado.
//Tópico al que se recibirán los datos
// El tópico de publicación debe tener estructura: <país>/<estado>/<ciudad>/<usuario>/out
const char MQTT_TOPIC_PUB[] = "colombia/cundinamarca/bogota/"USER"/out"; //TODO Reemplace el valor por el tópico de publicación que le corresponde.
// El tópico de suscripción debe tener estructura: <país>/<estado>/<ciudad>/<usuario>/in
const char MQTT_TOPIC_SUB[] = "colombia/cundinamarca/bogota/"USER"/in"; //TODO Reemplace el valor por el tópico de suscripción que le corresponde.
// Declaración de variables globales
// Timestamp de la fecha actual.
time_t now;
// Tiempo de la última medición
long long int measureTime = millis();
// Tiempo en que inició la última alerta
long long int alertTime = millis();
// Mensaje para mostrar en la pantalla
String alert = "";
// Valor de la medición de temperatura
float temp;
// Valor de la medición de la humedad
float humi;
//------[#INCLUIDO RETO 6]
// Valor de la medición de la luminosidad
float lux;

/**
 * Conecta el dispositivo con el bróker MQTT usando
 * las credenciales establecidas.
 * Si ocurre un error lo imprime en la consola.
 */
void mqtt_connect()
{
  //Intenta realizar la conexión indefinidamente hasta que lo logre
  while (!client.connected()) {    
    Serial.print("MQTT connecting ... ");    
    if (client.connect(MQTT_USER, MQTT_USER, MQTT_PASS)) {      
      Serial.println("connected.");
      Serial.println (MQTT_TOPIC_SUB);
      client.subscribe(MQTT_TOPIC_SUB);
      client.setCallback(receivedCallback);      
    } else {      
      Serial.println("Problema con la conexión, revise los valores de las constantes MQTT");
      int state = client.state();
      Serial.print("Código de error = ");
      alert = "MQTT error: " + String(state);
      Serial.println("****************************");
      Serial.println("state:");
      Serial.println(state);
      Serial.println("  alert:");
      Serial.println(alert);      
      if ( client.state() == MQTT_CONNECT_UNAUTHORIZED ) {
        ESP.deepSleep(0);
      }      
      // Espera 5 segundos antes de volver a intentar
      delay(5000);
    }
  }
}

/**
 * Publica la temperatura, humedad y luminosidad dadas al tópico configurado usando el cliente MQTT.
 */
void sendSensorData(float temperatura, float humedad, float luminosidad) {
  String data = "{";
  data += "\"temperatura\": "+ String(temperatura, 1) +", ";
  data += "\"humedad\": "+ String(humedad, 1)+", ";
  //------[#INCLUIDO RETO 6]
  data += "\"luminosidad\": "+ String(luminosidad, 1);
  data += "}";
  char payload[data.length()+1];
  data.toCharArray(payload,data.length()+1);  
  client.publish(MQTT_TOPIC_PUB, payload);
}

/**
 * Lee la luminosidad del sensor
 */
//------[#INCLUIDO RETO 6]
float readLuminosidad() {  
  int v = analogRead(lighsensor); 
  float vout = (float(v)/float(1023)) * (VIN);
  float i = vout/R;
  float rv = (VIN-vout)/i;
  int lux = (-0.01 * rv) + 110; // Se linealiza entre 10 y 100 lux mediante informacion del fabricante
  if(!isnan(lux) && lux<0){
    lux = 0;
  }
  return lux;
}

/**
 * Lee la temperatura del sensor DHT, la imprime en consola y la devuelve.
 */
float readTemperatura() {  
  // Se lee la temperatura en grados centígrados (por defecto)
  float t = dht.readTemperature();   
  return t;
}

/**
 * Lee la humedad del sensor DHT, la imprime en consola y la devuelve.
 */
float readHumedad() {
  // Se lee la humedad relativa
  float h = dht.readHumidity();   
  return h;
}

/**
 * Verifica si las variables ingresadas son números válidos.
 * Si no son números válidos, se imprime un mensaje en consola.
 */
bool checkMeasures(float t, float h, float l) {
  // Se comprueba si ha habido algún error en la lectura
    if (isnan(t) || isnan(h)) {
      Serial.println("Error obteniendo los datos del sensor DHT11");
      return false;    }
//------[#INCLUIDO RETO 6]
     if (isnan(l)) {
      Serial.println("Error obteniendo los datos del sensor Luminosidad");
      return false;
    }    
    return true;
}


void displayNoSignal() {
   Serial.println("No hay señal");   
}

/**
 * Muestra en la pantalla el mensaje de "Connecting to:" 
 * y luego el nombre de la red a la que se conecta.
 */
void displayConnecting(String ssid) {
  Serial.println("");
  Serial.println("Connecting to:\n");
  Serial.println(ssid);
}

/**
 * Verifica si ha llegdo alguna alerta al dispositivo.
 * Si no ha llegado devuelve OK, de lo contrario retorna la alerta.
 * También asigna el tiempo en el que se dispara la alerta.
 */
String checkAlert() {
  String message = "OK";  
  if (alert.length() != 0) {
    message = alert;
    if ((millis() - alertTime) >= ALERT_DURATION * 1000 ) {
      alert = "";
      alertTime = millis();
     }
  }
  return message;
}

/**
 * Función que se ejecuta cuando llega un mensaje a la suscripción MQTT.
 * Construye el mensaje que llegó y si contiene ALERT lo asgina a la variable 
 * alert que es la que se lee para mostrar los mensajes.
 */
void receivedCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("************Received [");
  Serial.print(topic);
  Serial.print("]: ");  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ALERTAS");   
  String data = "";
  for (int i = 0; i < length; i++) {
    data += String((char)payload[i]);
  }
  Serial.print(data);
  if (data.indexOf("ALERT") >= 0) {
    alert = data;
    Serial.print(alert);
    //------[#INCLUIDO RETO 6]
    if(data.indexOf("luminosidad") >= 0) {
       lcd.setCursor(0,1);
       lcd.print("Prendiendo alarma");   
       digitalWrite(LED_PIN, HIGH);
       delay(5000); 
       digitalWrite(LED_PIN, LOW);
       lcd.clear();
    }
  }
}

/**
 * Verifica si el dispositivo está conectado al WiFi.
 * Si no está conectado intenta reconectar a la red.
 * Si está conectado, intenta conectarse a MQTT si aún 
 * no se tiene conexión.
 */
void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Checking wifi");
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      WiFi.begin(ssid, pass);
      Serial.print(".");
     // displayNoSignal();
      delay(10);
    }
    Serial.println("connected");
  }
  else
  {
    if (!client.connected())
    {
      mqtt_connect();
    }
    else
    {
      client.loop();
    }
  }
}

/**
 * Imprime en consola la cantidad de redes WiFi disponibles y
 * sus nombres.
 */
void listWiFiNetworks() {
  int numberOfNetworks = WiFi.scanNetworks();
  Serial.println("\nNumber of networks: ");
  Serial.print(numberOfNetworks);
  for(int i =0; i<numberOfNetworks; i++){
      Serial.println(WiFi.SSID(i)); 
  }
}

/**
 * Inicia el servicio de WiFi e intenta conectarse a la red WiFi específicada en las constantes.
 */
void startWiFi() {  
  WiFi.hostname(USER);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);  
  Serial.println("(\n\nAttempting to connect to SSID: ");
  Serial.print(ssid);
  //Intenta conectarse con los valores de las constantes ssid y pass a la red Wifi
  //Si la conexión falla el node se dormirá hasta que lo resetee
  while (WiFi.status() != WL_CONNECTED)
  {
    if ( WiFi.status() == WL_NO_SSID_AVAIL ) {
      Serial.println("\nNo se encuentra la red WiFi ");
      Serial.print(ssid);
      WiFi.begin(ssid, pass);
    } else if ( WiFi.status() == WL_WRONG_PASSWORD ) {
      Serial.println("\nLa contraseña de la red WiFi no es válida.");
    } else if ( WiFi.status() == WL_CONNECT_FAILED ) {
      Serial.println("\nNo se ha logrado conectar con la red, resetee el node y vuelva a intentar");
      WiFi.begin(ssid, pass);
    }
    Serial.print(".");
    delay(1000);
  }
  Serial.println("connected!");
}

/**
 * Consulta y guarda el tiempo actual con servidores SNTP.
 */
void setTime() {
  //Sincroniza la hora del dispositivo con el servidor SNTP (Simple Network Time Protocol)
  Serial.print("Setting time using SNTP");
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < 1510592825) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  //Una vez obtiene la hora, imprime en el monitor el tiempo actual
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

/**
 * Configura el servidor MQTT y asigna la función callback para mensajes recibidos por suscripción.
 * Intenta conectarse al servidor.
 */
void configureMQTT() {
  //Llama a funciones de la librería PubSubClient para configurar la conexión con Mosquitto
  Serial.println("MQTT_TOPIC_PUB = ");
  Serial.println(MQTT_TOPIC_PUB);
  client.setServer(MQTT_HOST, MQTT_PORT);  
  // Se configura la función que se ejecutará cuando lleguen mensajes a la suscripción
  client.setCallback(receivedCallback);  
  //Llama a la función de este programa que realiza la conexión con Mosquitto
  mqtt_connect();
}

/**
 * Verifica si ya es momento de hacer las mediciones de las variables.
 * si ya es tiempo, mide y envía las mediciones.
 */
void measure() {
  if ((millis() - measureTime) >= MEASURE_INTERVAL * 1000 ) {
    //Serial.println("\nMidiendo variables...");
    measureTime = millis();    
    temp = readTemperatura();
    humi = readHumedad();
    //------[#INCLUIDO RETO 6]
    lux = readLuminosidad(); 
    // Se chequea si los valores son correctos
    if (checkMeasures(temp, humi, lux)) {
      // Se envían los datos
      sendSensorData(temp, humi, lux); 
    }
  }
}

/////////////////////////////////////
//         FUNCIONES ARDUINO       //
/////////////////////////////////////

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  listWiFiNetworks();
  lcd.init();
  displayConnecting(ssid);
  startWiFi();
  dht.begin();
  setTime();
  configureMQTT();   
  lcd.backlight(); 
}

void loop() {
  checkWiFi();
   if (!client.connected())
    {
      mqtt_connect();
    }
    else
    {
      client.loop();
    }
  String message = checkAlert();
  measure();  
}
