#include <WiFi.h>
#include <PubSubClient.h>

const char *ssid = "Nicolodi2";     // Nome WiFi
const char *password = "fehe14699"; // Senha WiFi

const char *ID = "UTFPR-Balanca"; // Name of our device, must be unique

const char *BROKER_MQTT = "broker.mqtt-dashboard.com"; // URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883;

const char *TOPIC = "led/estado";          // Topico INSCRIÇÃO led
const char *STATE_TOPIC = "caster/angulo"; // Topico PUBLICAR caster

unsigned int casterAngle = 0;

#define LED_BUILTIN 22
#define CASTER_POT 32

WiFiClient wclient;

PubSubClient client(wclient); // Setup MQTT client

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
  if (response == "1") // LIGA O LED
  {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print(topic);
    Serial.println(": LIGADO");
  }
  else if (response == "0") // DESLIGA O LED
  {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print(topic);
    Serial.println(": DESLIGADO");
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
      Serial.println("Conectado");
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
}

void loop()
{
  if (!client.connected()) // Reconecta se desconectado
  {
    reconnect();
  }
  client.loop();
  caster();
}