# Autoiler motorcycle chains (Автосмазчик цепи мотоцикла)
Auto lubricator for automatic lubrication of motorcycle chains
The control module chain lubrication on the basis of the STM32F103C8T6 (BluePill).
Works on the principle of processing interrupts from the hall sensor from the wheel to calculate the number of revolutions.

Модуль управления смазкой цепи на базе STM32F103C8T6 (BluePill).
Работает по принциру обработки прерывания от датчика холла с коллеса для подсчета количества оборотов.

## Principle of operation
When an interrupt is received from the hall sensor (wheel rotation), the variable is incremented.
When the specified threshold number of revolutions is reached, the static pump is switched on through the TIP120 transistor.

## Managed settings
1. The duration of pump operation (lube)
2. The lubrication frequency (number of revolutions of the wheel)

## Принцип работы
При поступлении прерывания от датчика холла (оборот колеса) происходит приращение переменной.
При достижении заданного порогового количества совершенных оборотов, происходит включение перестатического насоса через транзистор TIP120.

## Управляемые параметры
1. Длительность работы насоса (смазки)
2. Частота смазки (количество оборотов колеса)  

## Scheme (Схема устройства)

![scheme](https://github.com/leech001/Autoiler/tree/master/Scheme/Schematic_AutoOiler_Sheet-1_20190103125312.png )

## Android Application (Приложение для управления устройством)

![scheme](https://github.com/leech001/Autoiler/tree/master/Scheme/Schematic_AutoOiler_Sheet-1_20190103125312.png )