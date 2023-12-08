# Ambiente de desenvolvimento

## Instalação do Pico C/C++ SDK

O Pico C/C++ SDK é necessário para desenvolver software para a Raspberry Pi Pico. Você pode instalá-lo manualmente em (<https://github.com/raspberrypi/pico-sdk.git>) ou usando um script de instalação.

### Instalação usando o Script

Para simplificar o processo de instalação, você pode usar o script fornecido pela Raspberry Pi Foundation.

   ```bash
   cd ~/
   ```

   ```bash
   wget https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh
   ```

   ```bash
   chmod +x pico_setup.sh
   ```

   ```bash
   ./pico_setup.sh
   ```

   ```bash
   sudo reboot
   ```

## Compilação do projeto

```bash
mkdir build
```

```bash
cd build/ 
```

```bash
cmake -DPICO_BOARD=pico ../src/
```

```bash
make -j4
```

```bash
cp app.uf2 /media/isaac/RPI-RP2/
```

## pico-lora

LoRa communication library for Raspberry Pi RP2040 Microcontroller.

This library is written for RP2040 C++ SDK.

## Supported Hardware

* RP2040 boards
  * Raspberry Pi Pico
* Semtech SX1276/77/78/79 based LoRa boards

### Default Pinout

| Raspberry Pi Pico-Zero | Semtech SX1276 |
| ----------------- | -------------- |
| 3.3V | VCC |
| GND | GND |
| GPIO 14 | SCK |
| GPIO 15 | MOSI |
| GPIO 12 | MISO |
| GPIO 27 | DIO0 / G0 |
| GPIO 13 | NSS / CS |
| GPIO 26 | RESET |

Default Pinout can be overrided with setPins() function

| Raspberry Pi Pico-Zero | GPS NEO-6M |
| ----------------- | -------------- |
| 3.3V | VCC |
| GND | GND |
| GPIO 8 | TX |
| GPIO 9 | RX |
