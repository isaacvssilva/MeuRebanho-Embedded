/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  -
 *
 *        Version:  1.0
 *        Created:  24/09/2023 21:12:29
 *       Revision:  none
 *       Compiler:  -
 *
 *         Author:  Isaac Vinicius, Gislan Souza
 *                  isaacvinicius2121@alu.ufc.br, gislansouza@alu.ufc.br
 *   Organization:  UFC-Quixadá
 *
 * =====================================================================================
 */
#include "Arduino.h"
#include <WiFiManager.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define API_KEY ""      // firebase cloud
#define DATABASE_URL "" // firebase cloud

#include <SPI.h>
#include <LoRa.h>
#define LED_WIFI_ON 26 
#define LED_WIFI_OFF 27
#define LED_LORA_ON 16
#define LED_LORA_OFF 17
// Definindo as portas usadas pelo modulo receptor
#define ss 5    // Definindo o pino SS (Selecao de Escravo)
#define rst 4  // Definindo o pino de reset
#define dio0 2   // Definindo o pino DIO0 (Interrupcao de recepcao)

FirebaseData fdbo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMilis = 0;
bool signupOk = false;

void lora_init(void) {
  pinMode(LED_LORA_ON, OUTPUT);
  pinMode(LED_LORA_OFF, OUTPUT);
  while (!Serial);
  Serial.println("Iniciando o LoRa Receiver");

  /* Configurando o modulo transceptor LoRa */
  LoRa.setPins(ss, rst, dio0);
  digitalWrite(LED_LORA_OFF, HIGH);

  /* Substituindo o argumento LoRa.begin(---E-) com a frequência da sua região */
  /* 433E6 para Ásia */
  /* 866E6 para Europa */
  /* 915E6 para América do Norte */
  while (!LoRa.begin(915E6)) {}

  /* Definindo a palavra de sincronização (sync word) para corresponder ao transmissor */
  /* A palavra de sincronização garante que você não receba mensagens LoRa de outros transceptores LoRa */
  /* Variando de 0 a 0xFF */
  LoRa.setSyncWord(0xF3);
  digitalWrite(LED_LORA_OFF, LOW);
  digitalWrite(LED_LORA_ON, HIGH);
  Serial.println("LoRa Inicializado com Sucesso!");
}

void wifi_config(void) {
  /* Configurando os pinos dos LEDs como saídas */
  pinMode(LED_WIFI_ON, OUTPUT);
  pinMode(LED_WIFI_OFF, OUTPUT);
  digitalWrite(LED_WIFI_OFF, HIGH);
  /* Inicializando localmente o WiFiManager. Uma vez que ele concluir sua tarefa, não será necessário mantê-lo ativo. */
  WiFiManager wm;

  /* Redefinindo as configurações - apagando as credenciais armazenadas para testes. */
  /* Essas credenciais estão sendo armazenadas pela biblioteca do ESP. */
  wm.resetSettings();

  bool res;
  res = wm.autoConnect("MeuRebanho-Gateway","12345678"); /* AP protegido por senha */
  /* Mantendo o LED de WiFi desligado (LED_WIFI_OFF) aceso */
  if(!res) {
    Serial.println("Falha na conexão");
  } else {
    /* Se você estiver chegando aqui, a conexão WiFi está sendo estabelecida */
    Serial.println("Conectado...yeah :)");

    /* Desligando o LED de WiFi desligado (LED_WIFI_OFF) */
    digitalWrite(LED_WIFI_OFF, LOW);
    /* Acendendo o LED de WiFi ligado (LED_WIFI_ON) */
    digitalWrite(LED_WIFI_ON, HIGH);
  }
}

/* Salvando coordenadas no Firebase Realtime Database */
void saveCoordenadas(void) {
  String coordenadas = ""; /* Inicializando coordenadas com uma string vazia */

  /* Verificando se o Firebase está pronto, o registro foi bem-sucedido e o intervalo de envio de dados está sendo atingido */
  if (Firebase.ready() && signupOk && (millis() - sendDataPrevMilis > 5000 || sendDataPrevMilis == 0)) {
    sendDataPrevMilis = millis();
  }

  /* Aguardando até que um pacote LoRa esteja sendo recebido */
  while (LoRa.parsePacket() == 0) {
    delay(100); /* Aguardando 100 milissegundos antes de verificar novamente */
  }

  /* Pacote recebido */
  Serial.print("Recebendo pacote '");

  /* Lendo o pacote */
  while (LoRa.available()) {
    coordenadas = LoRa.readString();
    Serial.print(coordenadas);
  }

  /* Imprimindo a intensidade do sinal RSSI do pacote */
  Serial.print("' com RSSI ");
  Serial.println(LoRa.packetRssi());
  Serial.println(coordenadas);
  delay(2000);
  
  /* Verificando se a operação de definir uma string no Firebase Realtime Database foi bem-sucedida */
  if (Firebase.RTDB.setString(&fdbo, "Animal/marota1/GPS", coordenadas)) {
    Serial.println();
    /* Exibindo as coordenadas que foram salvas com sucesso */
    Serial.print(coordenadas);
    Serial.print("- Salvo com sucesso em: ");
    /* Exibindo o caminho onde as coordenadas foram armazenadas no Firebase */
    Serial.println(fdbo.dataPath());
    Serial.print(" ("); Serial.print(fdbo.dataType()); Serial.println(")");
  } else { /* Em caso de falha na operação de definição de uma string no Firebase Realtime Database. */
    Serial.print("Falha: ");
    /* Mostra a razão da falha, fornecida pelo Firebase. */
    Serial.println(fdbo.errorReason());
  }
}

void setup() {
  Serial.begin(115200);

  wifi_config();
  lora_init();
  /* Configurando as informações da chave de API e da URL do banco de dados do Firebase */
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  /* Verificando se o registro no Firebase foi bem-sucedido e apresentando uma mensagem de sucesso */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("SignUp com Sucesso");
   signupOk = true;
  } else { /* Em caso de falha no registro, exibindo uma mensagem de erro com detalhes */
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  
  /* Configurando um retorno de chamada (callback) para lidar com o status do token de autenticação */
  config.token_status_callback = tokenStatusCallback;

  /* Iniciando a comunicação com o Firebase utilizando as configurações definidas */
  Firebase.begin(&config, &auth);

  /* Solicitando a reconexão Wi-Fi ao Firebase, se necessário, permitindo a reconexão automática */
  Firebase.reconnectWiFi(true);
  
}

void loop() {
  saveCoordenadas();
  delay(500);
}