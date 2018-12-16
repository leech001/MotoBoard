import gc
import config
import uasyncio as asyncio
import machine
from machine import UART
from machine import WDT
import utime as time
import ujson as json

gc.enable()

LubeCount = 0
LubeDelay = 500
WheelRotateCount = 0
WheelRotateCountPrev = 0
WheelRotateLimitBase = 300
WheelRotateLimit = 0
WheelSpeed = 0
SpeedDelay = 5

data = 0

wdt = WDT()

status = {
    "WRL": config.WheelRotateLimitBase,
    "WRC": config.LubeCount,
    "LD": config.LubeDelay,
    "LC": config.LubeCount,
    "SP": config.WheelSpeed}

uart_ble = UART(0)

hall = machine.Pin(5, machine.Pin.IN)


def load_status():
    try:
        with open("status.json") as f:
            f_status = json.loads(f.read())
    except (OSError, ValueError):
        print("Couldn't load config.json")
        save_status()
    else:
        status.update(f_status)
        print("Loaded config from config.json")


def save_status():
    try:
        with open("status.json", "w") as f:
            f.write(json.dumps(status))
    except OSError:
        print("Couldn't save config.json")


def hall_irq():
    global WheelRotateCount
    WheelRotateCount += 1


hall.irq(trigger=machine.Pin.IRQ_RISING, handler=hall_irq)


async def lube_limit_correct():
    global WheelSpeed, WheelRotateCountPrev, WheelRotateLimit, WheelRotateLimitBase
    while True:
        WheelSpeed = ((WheelRotateCount - WheelRotateCountPrev) * config.lenghtWheel / SpeedDelay) * 3600 / 1000
        WheelRotateLimit = WheelRotateLimitBase
        if WheelSpeed > 100:
            WheelRotateLimit = WheelRotateLimitBase * 0.95
        if WheelSpeed > 140:
            WheelRotateLimit = WheelRotateLimitBase * 0.9
        if WheelSpeed > 180:
            WheelRotateLimit = WheelRotateLimitBase * 0.85
        if WheelSpeed > 210:
            WheelRotateLimit = WheelRotateLimitBase * 0.8
        if WheelSpeed > 240:
            WheelRotateLimit = WheelRotateLimitBase * 0.75
        WheelRotateCountPrev = WheelRotateCount
#        print("Speed correct")
        await asyncio.sleep(1)


async def lube_chain():
    global LubeCount, LubeDelay
    config.lubePomp_1.on()
    await asyncio.sleep(config.LubeDelay)
    config.lubePomp_1.off()
    LubeCount += 1


async def ble_transmit():
    while True:
        print(json.dumps(status))
        await asyncio.sleep(0.1)


async def default_task():
    global WheelRotateCount, WheelRotateCountPrev
    while True:
        if WheelRotateCount > WheelRotateLimit:
            WheelRotateCount = 0
            WheelRotateCountPrev = 0
            await lube_chain()
        ble_in = uart_ble.readline()
        if ble_in is not None:
            print(ble_in)
        await asyncio.sleep(0.1)
        wdt.feed()


try:
    loop = asyncio.get_event_loop()
    loop.create_task(default_task())
    loop.create_task(lube_limit_correct())
    loop.create_task(ble_transmit())
    loop.create_task(lube_chain())
    loop.run_forever()
except Exception as e:
    print("Error: [Exception] %s: %s" % (type(e).__name__, e))
    time.sleep(60)
    machine.reset()