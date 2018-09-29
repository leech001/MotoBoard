from machine import Pin


class HallSensor:
    def __init__(self, pin):
        self.pin = pin
        Pin(self.pin, Pin.IN)


class LubePomp:
    def __init__(self, pin):
        self.pin = pin

    def on(self):
        Pin(self.pin, Pin.OUT, value=1)
        return print("Lube pomp on")

    def off(self):
        Pin(self.pin, Pin.OUT, value=0)
        return print("Lube pomp off")


class Led:
    def __init__(self, pin):
        self.pin = pin

    def on(self):
        Pin(self.pin, Pin.OUT, value=1)
        return print("Led on")

    def off(self):
        Pin(self.pin, Pin.OUT, value=0)
        return print("Led off")
