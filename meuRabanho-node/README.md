# Ambiente de desenvolvimento

## Instalação do Pico C/C++ SDK

O Pico C/C++ SDK é necessário para desenvolver software para a Raspberry Pi Pico. Você pode instalá-lo manualmente em (<https://github.com/raspberrypi/pico-sdk.git>) ou usando um script de instalação.

### Instalação usando o Script

Para simplificar o processo de instalação, você pode usar o script fornecido pela Raspberry Pi Foundation.

   ```bash
   cd ~/

   wget https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh

   chmod +x pico_setup.sh

   ./pico_setup.sh

sudo reboot
```

## Compilação do projeto

```bash
cd build/ 
cmake -DPICO_BOARD=pico_w ../src/
make -j4
cp app.uf2 /media/isaac/RPI-RP2/
```
