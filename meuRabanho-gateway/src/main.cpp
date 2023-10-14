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

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "admin"
#define WIFI_PASSWORD "admin"
#define API_KEY ""      // firebase cloud
#define DATABASE_URL "" // firebase cloud

#include <SPI.h>
#include <LoRa.h>

// Definindo as portas usadas pelo modulo receptor
#define ss 5    // Definindo o pino SS (Selecao de Escravo)
#define rst 14  // Definindo o pino de reset
#define dio0 2   // Definindo o pino DIO0 (Interrupcao de recepcao)

FirebaseData fdbo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMilis = 0;
bool signupOk = false;

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
  if (Firebase.RTDB.setString(&fdbo, "Animal/GPS/Coordenadas", coordenadas)) {
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

  while (!Serial);
  Serial.println("Iniciando o LoRa Receiver");

  /* Configurando o modulo transceptor LoRa */
  LoRa.setPins(ss, rst, dio0);

  /* Substituindo o argumento LoRa.begin(---E-) com a frequência da sua região */
  /* 433E6 para Ásia */
  /* 866E6 para Europa */
  /* 915E6 para América do Norte */
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }

  /* Definindo a palavra de sincronização (sync word) para corresponder ao transmissor */
  /* A palavra de sincronização garante que você não receba mensagens LoRa de outros transceptores LoRa */
  /* Variando de 0 a 0xFF */
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Inicializado com Sucesso!");
  delay(3000);

  /* Exibindo uma mensagem enquanto o dispositivo está estabelecendo uma conexão Wi-Fi */
  Serial.println("Conectando ao Wi-Fi...");

  /* Iniciando a conexão Wi-Fi com as credenciais definidas em WIFI_SSID e WIFI_PASSWORD */
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  /* Aguardando até que a conexão Wi-Fi seja estabelecida, mostrando um ponto a cada tentativa de conexão */
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    /* Aguardando por 300 milissegundos antes de verificar novamente o status da conexão */
    delay(300);
  }

  /* Indicando o término da tentativa de conexão com uma nova linha */
  Serial.println();

  /* Exibindo uma mensagem informando que a conexão Wi-Fi foi bem-sucedida e que o IP será apresentado a seguir */
  Serial.print("Conexão Wi-Fi estabelecida com IP: ");

  /* Mostrando o endereço IP local atribuído ao dispositivo Wi-Fi */
  Serial.println(WiFi.localIP());

  /* Configurando as informações da chave de API e da URL do banco de dados do Firebase */
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  /* Verificando se o registro no Firebase foi bem-sucedido e apresentando uma mensagem de sucesso */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Registrando com Sucesso");
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
  delay(5000);
}
