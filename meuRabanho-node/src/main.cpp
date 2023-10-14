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
 *       Compiler:  arm-linux-gnueabihf-
 *
 *         Author:  Isaac Vinicius, Gislan Souza
 *                  isaacvinicius2121@alu.ufc.br, gislansouza@alu.ufc.br
 *   Organization:  UFC-Quixadá
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/sleep.h"
#include "hardware/rtc.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "../lib/LoRa-RP2040.h"

#define UART_PRINT_ID uart0
#define UART_PRINT_TX 0
#define UART_PRINT_RX 1
#define UART_PRINT_BAUD_RATE 115200

#define UART_GPS_ID uart1
#define UART_GPS_TX 4
#define UART_GPS_RX 5
#define UART_GPS_BAUD_RATE 9600

#define BUFFER_SIZE 50            /* Definindo o tamanho do buffer para armazenar os caracteres recebidos */
char receive_buffer[BUFFER_SIZE]; /* Armazenando os caracteres recebidos em um buffer */
size_t receive_buffer_index = 0;  /* Mantendo um indice atual no buffer */

/* variaveis para capturar os dados do GPS */
volatile double lat, lon;
char latitude[20];
char longitude[20];
char Coordenadas[50];

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  convertToDegree
 *  Description:  Funcao para converter para os valores reais de coordenadas
 * Reference: https://microcontrollerslab.com/neo-6m-gps-module-raspberry-pi-pico-micropython/
 * =====================================================================================
 */
double convertToDegree(double coordinate) {
  // conversao de graus, minutos e segundos para graus decimais
  int degrees = (int)coordinate / 100;                // Obtem os graus inteiros
  double minutes = coordinate - degrees * 100;        // Obtem os minutos
  double decimalDegrees = degrees + (minutes / 60.0); // Calcula os graus decimais
  return decimalDegrees;
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getLocalizacao
 *  Description:  Funcao para processar os dados recebidos do GPS
 * =====================================================================================
 */
void getLocalizacao(void) {
  /* Desabilitando interrupcoes para processar os dados */
  uart_set_irq_enables(UART_GPS_ID, false, false);

  /* Dividindo a entrada em linhas */
  char *linha = strtok(receive_buffer, "\n");

  while (linha != NULL)
  {
    /* Verificando NMEA Sentences: Geographic position, latitude / longitude  */
    if (strstr(linha, "$GPGLL") == linha) {
      char *token;
      int i = 0;
      token = strtok(linha, ",");

      while (token != NULL) {
        // no indice 1 contem a latitude
        if (i == 1) {
          lat = strtod(token, NULL);
          lat = convertToDegree(lat); // Converte a latitude para graus decimais
        }
        if (i == 2 && *token == 'S') {
          lat = -lat;
        }
        // no indice 3 contem a longitude
        if (i == 3) {
          lon = strtod(token, NULL);
          lon = convertToDegree(lon); // Converte a longitude para graus decimais
        }
        if (i == 4 && *token == 'W') {
          lon = -lon;
        }
        token = strtok(NULL, ",");
        i++;
      }
    }

    /* Avançando para a proxima linha */
    linha = strtok(NULL, "\n");
  }

  /* sprintf para converter os valores float em strings */
  sprintf(latitude, "%.8f", lat);
  sprintf(longitude, "%.8f", lon);

  /* Limpando o buffer para a proxima mensagem */
  receive_buffer_index = 0;
  memset(receive_buffer, 0, sizeof(receive_buffer));

  /* Habilitando interrupcoes novamente */
  uart_set_irq_enables(UART_GPS_ID, true, false);
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  on_uart_gps_irq
 *  Description:  Interrupt Service Routine (ISR) para a UART do GPS
 * =====================================================================================
 */
void on_uart_gps_irq(void) {
  while (uart_is_readable(UART_GPS_ID)) {
    char received_char = uart_getc(UART_GPS_ID);

    /* Armazenando o caractere no buffer */
    if (receive_buffer_index < BUFFER_SIZE - 1) {
      receive_buffer[receive_buffer_index++] = received_char;
      receive_buffer[receive_buffer_index] = '\0'; // Terminando a string com um nulo
    }

    /* Verificando se o caractere recebido ha uma quebra de linha (indicando o fim de uma mensagem) */
    if (received_char == '\n') {
      /*Uma linha completa foi recebida, chamando getLocalizacao */
      getLocalizacao();
    }
  }
}

/* flag para indicar a o despertar do microcontrolador */
static bool awake;

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  sleep_callback
 *  Description:  Funcao de Callback apos acordar do modo de suspensao
 * =====================================================================================
 */
static void sleep_callback(void) {
  /* Exibindo uma mensagem indicando que o RTC está acordando o sistema */
  uart_puts(UART_PRINT_ID, "RTC nos acordou\n\n\r");
  uart_default_tx_wait_blocking();

  /* Chamando a funcao para lidar com os dados do GPS */
  on_uart_gps_irq();
  awake = true;
  return;
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  rtc_sleep
 *  Description:  Funcao que configura o RTC para entrar no modo de suspensao
 * Reference:https://ghubcoder.github.io/posts/awaking-the-pico/
 * =====================================================================================
 */
static void rtc_sleep(int8_t minute_to_sleep_to, int8_t second_to_sleep_to) {
  datetime_t t_alarm = {
      .year = 2020,
      .month = 06,
      .day = 05,
      .dotw = 5, /* 0 sendo domingo, então 5 é sexta-feira */
      .hour = 15,
      .min = minute_to_sleep_to,
      .sec = second_to_sleep_to};/* Definindo a data e a hora para o alarme RTC. */

  /* Exibindo uma mensagem indicando que o sistema esta entrando no modo de suspensao */
  uart_puts(UART_PRINT_ID, "Indo dormindo.......\n\r");
  uart_default_tx_wait_blocking();

  /* Entrando no modo de suspensao ate o tempo definido no alarme e chamando o callback de acordar */
  sleep_goto_sleep_until(&t_alarm, &sleep_callback);
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  recover_from_sleep
 *  Description:  Funcao responsavel por restaurar as configuracoes de clock padrao do
 *                microcontrolador
 * =====================================================================================
 */
void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig) {

  /* Reabilitando o controle do oscilador de anel */
  rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

  /* Restaurando o estado dos registradores para seus valores originais */
  scb_hw->scr = scb_orig;
  clocks_hw->sleep_en0 = clock0_orig;
  clocks_hw->sleep_en1 = clock1_orig;

  /* Reconfigurando os clocks para suas configurações padrão */
  clocks_init();
  stdio_init_all();

  return;
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  uart_config
 *  Description:  Funcao responsavel por realizar as configuracoes de uart
 * =====================================================================================
 */
void uart_config(void) {
  /* Setando os GPIOs para nivel logico baixo e multiplexando para UART */
  gpio_put(UART_PRINT_TX, 0);
  gpio_put(UART_PRINT_RX, 0);
  gpio_put(UART_GPS_TX, 0);
  gpio_put(UART_GPS_RX, 0);

  /* Inicializacao da UART para GPS e configuracao da taxa de baud */
  uart_init(UART_PRINT_ID, UART_PRINT_BAUD_RATE);
  gpio_set_function(UART_PRINT_TX, GPIO_FUNC_UART);
  uart_init(UART_GPS_ID, UART_GPS_BAUD_RATE);
  gpio_set_function(UART_GPS_RX, GPIO_FUNC_UART);

  /* Habilitando a interrupcao de Received para o UART_GPS_ID */
  uart_set_irq_enables(UART_GPS_ID, true, false);

  /* Definindo a ISR para UART_GPS_ID */
  irq_set_exclusive_handler(UART1_IRQ, on_uart_gps_irq);

  /* Habilitando a interrupcao UART */
  irq_set_enabled(UART1_IRQ, true);
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  lora_init
 *  Description:  Funcao responsavel por realizar as configuracoes do modulo LoRa
 * =====================================================================================
 */
uint8_t counter = 0;
void lora_init(void) {
  uart_puts(UART_PRINT_ID, "Inicializando LoRa...\n\r");
  /* 433E6 para Ásia */
  /* 866E6 para Europa */
  /* 915E6 para América do Norte */
  if (!LoRa.begin(915E6)) {
    uart_puts(UART_PRINT_ID, "Falha na inicializacao LoRa\n\r");
    while (1);
  }
  /* Definindo a palavra de sincronizacao (sync word) para corresponder ao transmissor */
  /* A palavra de sincronizacao garante que você não receba mensagens LoRa de outros transceptores LoRa */
  /* Variando de 0 a 0xFF */
  LoRa.setSyncWord(0xF3);
}

int main() {

  stdio_init_all();
  /* Inicializando o modulo LoRa. */
  lora_init();
  /* Configurando as portas UART para comunicacao serial */
  uart_config();

  /* Salvando os valores atuais dos processadores no modo padrao antes do sleep */
  uint scb_orig = scb_hw->scr;
  uint clock0_orig = clocks_hw->sleep_en0;
  uint clock1_orig = clocks_hw->sleep_en1;

  /* Redefinindo o relógio em tempo real para um valor específico */
  datetime_t t = {
      .year = 2020,
      .month = 06,
      .day = 05,
      .dotw = 5, /* 0 sendo domingo, então 5 é sexta-feira */
      .hour = 15,
      .min = 45,
      .sec = 00};

  /* Iniciando o relógio em tempo real */
  rtc_init();

  while (true) {
    uart_puts(UART_PRINT_ID, "Hello Sleep!\n\r");
    uart_default_tx_wait_blocking();

    awake = false;

    /* Entrando no modo de suspensão, aguardando acordar pelo RTC */
    sleep_run_from_xosc();

    /* Redefinindo o relógio em tempo real para um valor específico */
    rtc_set_datetime(&t);

    /* Entrando em um estado de dormência por um determinado período, neste caso, 30 segundos */
    rtc_sleep(45, 30);

    /* Aguardando até que o sistema seja acordado */
    while (!awake) {
      /* Esta parte do código não deve estar sendo executada durante o sono */
      printf("Deveria estar dormindo\n\r");
      uart_default_tx_wait_blocking();
    }

    /* Restaurando o processador e os clocks para os valores padrão */
    recover_from_sleep(scb_orig, clock0_orig, clock1_orig);

    /* Preparando as coordenadas para envio */
    strcpy(Coordenadas, latitude); /* Copiando a latitude para Coordenadas */
    strcat(Coordenadas, ",");      /* Adicionando uma vírgula para separar latitude e longitude */
    strcat(Coordenadas, longitude);

    /* Imprimindo as coordenadas via UART para debug */
    uart_puts(UART_PRINT_ID, Coordenadas);
    uart_default_tx_wait_blocking();
    uart_puts(UART_PRINT_ID, "\n\r");
    uart_default_tx_wait_blocking();
    sleep_ms(100);

    /* Iniciando um novo pacote de transmissao LoRa */
    LoRa.beginPacket();

    /* Adicionando as coordenadas ao pacote LoRa. O conteudo de 'Coordenadas' sera enviado */
    LoRa.print(Coordenadas);

    /* Adicionando o contador ao pacote LoRa, para rastrear o numero de pacotes enviados */
    LoRa.print(counter);

    /* Finalizando o pacote de transmissao, e o enviando pela rede LoRa */
    LoRa.endPacket();

    /* Incrementando o contador para o proximo pacote */
    counter++;

    /* Imprimindo mensagem via UART para debug */
    uart_puts(UART_PRINT_ID, "Coordenadas enviadas!\n\r");
    uart_default_tx_wait_blocking();
    sleep_ms(5000);
  }

  return 0;
}
