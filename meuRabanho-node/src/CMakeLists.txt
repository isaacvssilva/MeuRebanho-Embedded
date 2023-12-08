# Definindo a versao minima necessaria do CMake
cmake_minimum_required(VERSION 3.13)

# Incluindo o SDK da Raspberry Pi Pico
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)
# Definindo o nome do projeto em uma variavel
set(PROJECT_NAME app)

# Configurando o projeto com as linguagens C, C++ e ASM
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Inicializando o SDK Pico
pico_sdk_init()

# Adicionando o executavel principal do projeto
add_executable(${PROJECT_NAME}
    main.cpp
    ../lib/LoRa-RP2040.cpp
    ../lib/Print.cpp
)

# Habilitando a saida USB padrao para o projeto
pico_enable_stdio_usb(${PROJECT_NAME} 0)

# Habilitando a saida UART padrao para o projeto
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# Adicionando saidas extras ao projeto (por exemplo, UF2)
pico_add_extra_outputs(${PROJECT_NAME})

# Configurando os diretorios de inclusao
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Vinculando as bibliotecas necessarias, como o SDK Pico e hardware especifico
target_link_libraries(${PROJECT_NAME} pico_stdlib pico_runtime hardware_gpio hardware_uart hardware_spi hardware_interp hardware_rtc hardware_rosc hardware_sleep)