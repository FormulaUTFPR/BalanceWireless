#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <HX711_ADC.h>

const char *ssid = "minharede";     // Nome WiFi
const char *password = "minhasenha"; // Senha WiFi

const char *ID = "UTFPR-Balanca"; // Nome do dispositivo (deve ser único na rede)

const char *BROKER_MQTT = "broker.mqtt-dashboard.com"; // URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883;

const char *TOPIC = "tara/estado";          // Topico INSCRIÇÃO tara
const char *STATE_TOPIC = "caster/angulo"; // Topico PUBLICAR caster
const char *STATE_TOPIC_WEIGHT = "balanca/massa"; // Topico PUBLICAR massa

const char *char0 = "0";
const char *char1 = "1";

unsigned int casterAngle = 0;

unsigned long stabilizingtime = 2000; // Tempo para estabilizar
boolean _tare = true; // Se quiser tarar depois de inicializar, deixar verdadeiro, se não, falso

#define LED_BUILTIN 22
#define CASTER_POT 32
#define HX711_dout 4 //mcu > HX711 pino dout
#define HX711_sck 5 //mcu > HX711 pino sck

WiFiClient wclient;

PubSubClient client(wclient); // Setup MQTT client
HX711_ADC LoadCell(HX711_dout, HX711_sck); // Setup HX711

// Cabecalhos
void setupHX711();
void getWeight();

// TRATAMENTO DE MENSAGENS QUE CHEGAM
void callback(char *topic, byte *payload, unsigned int length)
{
  String response;

  for (int i = 0; i < length; i++)
  {
    response += (char)payload[i];
  }
  Serial.print("Mensagem chegou [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(response);
  
  // Tarar a balanca
  if (response == char1) {
    LoadCell.tareNoDelay();
    Serial.println("Aaaaaa");
  }

  // Checar se está tarada
  if (LoadCell.getTareStatus() == true) {
    client.publish("tara/estado", char0);
  }
}

// CONECTA WIFI
void setup_wifi()
{
  Serial.print("\nSe conectando a");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // CONECTA

  while (WiFi.status() != WL_CONNECTED)
  { // ESPERA CONEXÃO
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void caster() // Faz a medição do ângulo do caster
{
  unsigned int newCasterAngle = 0;
  unsigned int lastCasterAngle = casterAngle;
  unsigned int angleDif = 0;

  newCasterAngle = analogRead(CASTER_POT);
  angleDif = lastCasterAngle - newCasterAngle;

  if (angleDif > 30)
  {
    newCasterAngle = map(newCasterAngle, 0, 100, 0, 4096);
    char angleStr[8];
    dtostrf(newCasterAngle, 1, 2, angleStr);
    client.publish("caster/angulo", angleStr);
    Serial.println(angleDif);
  }
}

// RECONECTA O CLIENTE
void reconnect()
{
  // LAÇO ATÉ CONECTAR
  while (!client.connected())
  {
    Serial.print("Conectando ao MQTT... ");
    // TENTA CONECTAR
    if (client.connect(ID))
    {
      client.subscribe(TOPIC);
      Serial.println("Conectado ");
      Serial.print("Incrito em: ");
      Serial.println(TOPIC);
      Serial.println('\n');
    }
    else
    {
      Serial.println("Tente novamente em 5 segundos");
      // ESPERA 5 SEGUNDOS
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(9600);           // SERIAL A 9600 BAUD
  pinMode(LED_BUILTIN, OUTPUT); // Configura LED como saída
  pinMode(CASTER_POT, INPUT);   // Configura potenciometro caster como entrada
  delay(100);
  setup_wifi(); // Conecta na rede
  client.setServer(BROKER_MQTT, BROKER_PORT);
  client.setCallback(callback); // Inicializa a subrotina de callback

  Serial.println("Meio do Setup");
  setupHX711();
  Serial.println("Terminou o setup");
}

void loop()
{
  if (!client.connected()) // Reconecta se desconectado
  {
    reconnect();
  }
  client.loop();
  caster();
  getWeight();
}

void setupHX711(){
  
    LoadCell.begin();
    float calibrationValue = 696.0; // Valor de calibração
  
    LoadCell.start(stabilizingtime, _tare);
    if (LoadCell.getTareTimeoutFlag()) {
      while (1);
    }
    else {
      LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
      Serial.println("Startup is complete");
    }
 }

 void getWeight(){
  static boolean newDataReady = 0;

  if (LoadCell.update()) newDataReady = true; // Checar se tem atualização de peso

  if (newDataReady) {
      char mass[8];
      float i = LoadCell.getData();
      Serial.println("Peso: ");
      Serial.println(i);

      dtostrf(i, 1, 2, mass);
      client.publish("balanca/massa", mass);
  }
}
