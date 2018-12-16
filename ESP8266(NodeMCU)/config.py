import device

led_1 = device.Led(2)
# hall_1 = device.HallSensor(5)
lubePomp_1 = device.LubePomp(16)
lenghtWheel = 1.96
ble_delay = 10

LubeCount = 0
LubeDelay = 500
WheelRotateCount = 0
WheelRotateCountPrev = 0
WheelRotateLimitBase = 300
WheelRotateLimit = 0
WheelSpeed = 0
SpeedDelay = 5
