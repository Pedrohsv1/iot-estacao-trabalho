#include <Arduino.h>
#include <FS.h>
#include "SPIFFS.h"

#include <WiFi.h>
#include "time.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <PubSubClient.h>

const char* ntpServer = "a.st1.ntp.br";  // You can change this to your preferred NTP server

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

#define SOUND_SPEED 0.034

const char* ssid     = "NPITI-IoT";
const char* password = "NPITI-IoT";

#define IO_USERNAME  "Pedrohsv1";
#define IO_KEY       "random-key";

const char* mqttserver = "io.adafruit.com";
const int mqttport = 1883;
const char* mqttUser = IO_USERNAME;
const char* mqttPassword = IO_KEY;

WiFiClient espClient;
PubSubClient client(espClient);

// Define the GMT offset (in seconds)
const long gmt = -10800;  // For UTC-5 (adjust according to your time zone)

int led1 = 23;
int led2 = 13;

int trigPin2 = 19;
int echoPin2 = 21;

int trigPin = 22;
int echoPin = 18;

int estado = 0;

String s;

int tempo_digitando = 0;
int tempo_inicial_digitando = 0;

int tempo_trabalhando = 0;
int tempo_inicial_trabalhando = 0;

long duracaoPulso1;
float distanciaCm1;

long duracaoPulso2;
float distanciaCm2;

String formattedDate;
String dayStamp;
String timeStamp;


void writeFile(String state, String path) { //escreve conteúdo em um arquivo
  File rFile = SPIFFS.open(path, "a");
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo!");
  }
  else {
    Serial.print("Tamanho");
    Serial.println(rFile.size());
    rFile.println(state);
    Serial.print("Gravou: ");
    Serial.println(state);
  }
  rFile.close();
}

String readFile(String path) {
  Serial.println("Read file");
  File rFile = SPIFFS.open(path, "r");//r+ leitura e escrita
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo!");
  }
  else {
    Serial.print("----------Lendo arquivo ");
    Serial.print(path);
    Serial.println("  ---------");
    while (rFile.position() < rFile.size())
    {
      s = rFile.readStringUntil('\n');
      s.trim();
      Serial.println(s);
    }
    rFile.close();
    Serial.print("----------Final arquivo ");
    Serial.print(path);
    Serial.println("  ---------");
    
    return s;
  }
}

String readFileFive(String path) {
  Serial.println("Read file");
  File rFile = SPIFFS.open(path, "r"); // r+ leitura e escrita
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo!");
    return "";  // Retorna uma string vazia em caso de erro
  } else {
    Serial.print("----------Lendo arquivo ");
    Serial.print(path);
    Serial.println("  ---------");

    // Buffer para armazenar as últimas 5 linhas
    String lines[5];
    int lineCount = 0;

    // Lê o arquivo linha por linha
    while (rFile.position() < rFile.size()) {
      String s = rFile.readStringUntil('\n');
      s.trim();
      if (lineCount < 5) {
        lines[lineCount] = s;
        lineCount++;
      } else {
        // Desloca as linhas para "guardar" as 5 últimas
        for (int i = 1; i < 5; i++) {
          lines[i - 1] = lines[i];
        }
        lines[4] = s;  // Adiciona a nova linha
      }
    }
    rFile.close();

    // Agora, vamos construir a string com as últimas 5 linhas, numeradas
    String result = "";
    for (int i = 0; i < lineCount; i++) {
      result += String(i + 1) + ". " + lines[i] + "\n";
    }

    return result;
  }
}


void formatFile() {
  Serial.println("Formantando SPIFFS");
  SPIFFS.format();
  Serial.println("Formatou SPIFFS");
}

void openFS(void) {
  if (!SPIFFS.begin()) {
    Serial.println("\nErro ao abrir o sistema de arquivos");
  }
  else {
    Serial.println("\nSistema de arquivos aberto com sucesso!");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    // Create a random client ID
    String clientId = "ESP32 - Sensores";
    clientId += String(random(0xffff), HEX);
    // Se conectado
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("conectado");
      // Depois de conectado, publique um anúncio ...
      client.publish("Pedrohsv1/feeds/dist", "Iniciando Comunicação");
      //... e subscribe.
      client.subscribe("Pedrohsv1/feeds/dist"); // <<<<----- mudar aqui
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
 

  pinMode(trigPin, OUTPUT);//digital write
  pinMode(echoPin, INPUT);

  pinMode(trigPin2, OUTPUT);//digital write
  pinMode(echoPin2, INPUT);

  pinMode(led1, OUTPUT);//escrever no led
  pinMode(led2, OUTPUT);//escrever no led
  
  openFS();
  Serial.println("ler arquivo");
  String teste = readFile("/SPIFFS.txt");

  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_5dBm);
                      
  WiFi.begin(ssid, password);
  Serial.print("Conectando no WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!\n");

  timeClient.begin();
  timeClient.setTimeOffset(-10800);

  client.setServer(mqttserver, 1883); // Publicar

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  timeClient.forceUpdate();

  delay(1000);

  Serial.println(" ");

  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duracaoPulso1 = pulseIn(echoPin, HIGH);

  delay(100);
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  duracaoPulso2 = pulseIn(echoPin2, HIGH); 

  distanciaCm1 = duracaoPulso1*SOUND_SPEED/2;
  distanciaCm2 = duracaoPulso2*SOUND_SPEED/2;

  if(distanciaCm1 < 100) {
    if(tempo_inicial_trabalhando == 0) {
      tempo_inicial_trabalhando = millis();
    }

    digitalWrite(led1, LOW);
    delay(100);
    digitalWrite(led1, HIGH);
    delay(100);
    digitalWrite(led1, LOW);
    delay(100);

  } else if (tempo_inicial_trabalhando != 0) {
    tempo_trabalhando = (millis() - tempo_inicial_trabalhando);
    tempo_inicial_trabalhando = 0;
  }

  if(distanciaCm2 < 20) {
    if(tempo_inicial_digitando == 0) {
      tempo_inicial_digitando = millis();
    }

    digitalWrite(led2, LOW);
    delay(100);
    digitalWrite(led2, HIGH);
    delay(100);
    digitalWrite(led2, LOW);
    delay(100);
  } else if (tempo_inicial_digitando != 0) {
    tempo_digitando = (millis() - tempo_inicial_digitando);
    tempo_inicial_digitando = 0;
  }

  int estado_em_compilacao = 0;
  if(tempo_inicial_trabalhando != 0 || tempo_inicial_digitando!=0){
    estado_em_compilacao = 1;
  }
  if(tempo_inicial_trabalhando == 0 && tempo_inicial_digitando==0){
    estado_em_compilacao = 0;
  }

  
  char dis[8];
  dtostrf(distanciaCm1,1,2,dis);
  client.publish("Pedrohsv1/feeds/dist", dis);


  char dis2[8];
  dtostrf(distanciaCm2,1,2,dis2);
  client.publish("Pedrohsv1/feeds/dist-type", dis2);

  if(estado == 1 && estado_em_compilacao == 0){

    time_t t=timeClient.getEpochTime();
    // Saiu - Wednesday, 02/12/2024 HH - Tempo de Trabalho: Xs - Tempo digitando: Xs
    String strOut= "Saiu - " + String(day(t))+"/"+ String(month(t)) + "/" + String(year(t)) + " " + String(hour(t)) + ":" + String(minute(t)) + ":" + String(second(t)) + " - Tempo trabalhando:" + String(tempo_trabalhando/1000) + "s - Tempo digitando: " + String(tempo_digitando/1000) + "s";

    tempo_trabalhando = 0;
    tempo_digitando = 0;

    writeFile(strOut, "/historico.txt");
    estado = 0;
  }
  if(estado == 0 && estado_em_compilacao == 1){

    // Obtenha o tempo atual
    time_t t = timeClient.getEpochTime();

    // Montagem da string de data e hora
    String strEnter = "Entrou - " + String(day(t)) + "/" + String(month(t)) + "/" + String(year(t)) + " -- " +
                      String(hour(t)) + ":" + String(minute(t)) + ":" + String(second(t));

    // Exibe a string montada para verificar

    // Agora, escreve a string no arquivo
    writeFile(strEnter, "/historico.txt");
    estado = 1;
  }

  String result  = readFile("/historico.txt");

  Serial.print(result);
  delay(5000);
}
                                      
