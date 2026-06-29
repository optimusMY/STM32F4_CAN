# STM32F4_CAN
CANBUS Baremetal driver for stm32f446re
First  create an "EMPTY" project using STM32Cubeide  (my version is old, 1.6.1)
make pin config then generate code.
After that replace all files with this repo.


In order to build properly, add path and symbols to your project :

<PROJ> properties->C/C++ General -> Paths and Symbols  -->Includes -->Add following paths:
${ProjDirPath}\chip_headers\CMSIS\Include
${ProjDirPath}\chip_headers\CMSIS\Device\ST\STM32F4xx\Include

<PROJ> properties->C/C++ General -> Paths and Symbols  -->Symbols -->Add following symbols:
#DEBUG
#STM32
#STM32F4
#STM32F446xx
#STM32F446RETx



