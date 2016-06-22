# beer-brew

This is a actually a arduino project, we build simple arduino device to control temperature of water and this is almost what we need when brewing beer.

Hardware parts:

![Pins](./docs/pins.png)

- arduino
  [arduino doc](https://www.arduino.cc/en/reference/board)
  [arduino pins](./docs/arduino_board.png)
- LCD screen with keyboard
  [LCD Keypad Shield pins](./docs/keypad-shield.jpg)
- Temperature sensor
  pin d2
- Heater
- relay to control the power of heater
  pin d3
- Clock
  SCL→A5
  SDA→A4
  VCC→5V
  GND→GND
  
# Device parts:
    1. controller x1
    2. tube x3
    3. heat exchange x1
    4. kettle x1
    5. magnective pump
    6. valve x1
    7. keg(60L) x1
    8. leaky bucket x1
    9. T type tube x1
    10. 法盘快装卡箍 x8
    11. 快装接头 x5
    12. 卡箍内丝 x6
    13. 卡箍垫圈 x8
    14. 传感器接头
    15. 电子称 (大麦和水)
    16. bucket桶 x3
    17. 消毒剂 x1
    18. 酵母培养皿 x1
    19. 量杯 x1
    20. 密度计(酒精度计) x1
    21. 温度计 x1 (optional)
    22. 对辊粉碎机 x1
    23. 单向空气阀 x1
    24. 啤酒花过滤器 x5
    25. 穿壁直通 x1
    26. 二氧化碳气瓶 x1

    
# 酿造原料
    1. barly大麦
    2. water 水
    3. yeast 酵母
    4. hops  啤酒花
    
# Processes
    1. sanitize components
    2. setup device
    3. prepare water according to your recipe
    4. set stage to 0 (mashing in) and and temperature to 72, heating up water, start the pump
    5. prepare gravy according to your recipe and crash it
    6. put crashed barly in
    7. set stage to mashing and temperature to 68, time to 1 hour
    8. prepare sparse water and heat up to 75 degree
    9. prepare hops and hops sockets
    10. lift gravy out
    11. set stage to mashing out, mashing out temperature about 75
    12. when reach target temperature 100, start timer to 1 hour
    13. put hops according to your recipe

