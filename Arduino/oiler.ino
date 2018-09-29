#include <Thread.h>
#include <EEPROM.h>
#include <Timer.h>
//#include <Wire.h>
//#include <LiquidCrystal_I2C.h>

//LiquidCrystal_I2C lcd(0x27,16,2);

//Отдельный поток для смазки
Thread TOiler = Thread();

//Отдельный поток для скорости
Thread TSpeed = Thread();

//Отдельный поток для скорости
Thread TCHall = Thread();

Timer t;

void RError();
void CheckSensor();
void lCorrect(float);
void WheelRotate();
unsigned int EEPROMReadInt(int);
void EEPROMWriteInt(int,int);

boolean rotate=false;
boolean eRrotate=true;

unsigned long wr,wrs,wrsl;
double wrl,wrlb;

int lubeDelay;
int lubeCount;

const byte cLubePin=2;          //Цифровой пин электроклапана
const float lenghtWheel=1.96;   //Длинна окружности колеса

class Sensor
{
public:
    int maxV;
    int minV;
    int value;
    Sensor(byte pin);
    void Calibration();
    void Reset();
    void Read();

private:
    byte _pin;
};

Sensor oHall = Sensor(A0);

Sensor::Sensor(byte pin)
{
    _pin=pin;
}

void Sensor::Calibration()
{
    if (value<minV) minV=value;
    if (value>maxV) maxV=value;
}

void Sensor::Read()
{
    value=analogRead(_pin);
}

void Sensor::Reset()
{
    minV=maxV=value;
}



void setup()
{
    Serial.begin(19200);

    //lcd.init();
    //lcd.backlight();

    pinMode(cLubePin, OUTPUT);
    pinMode(13, OUTPUT);

    //Создание потока смазки
    TOiler.onRun(LubeChain);
    TOiler.setInterval(1000);

    //Создание потока для вычисления скорости
    TSpeed.onRun(Speed);
    TSpeed.setInterval(10000);

    //Создание потока для каллибровки датчика Холла
    TCHall.onRun(HReset);
    TCHall.setInterval(30000);


    oHall.Reset();

    wr=wrs=0;
    wrl=wrlb=2000;      //Базовое значение количества оборотов колеса между смазкой
    lubeDelay=500;     //Время смазки в мс

    t.after(600000, RError);

    //EEPROMWriteInt(0, 0);
    lubeCount=EEPROMReadInt(0);

    Serial.print("Wheel lube count= ");
    Serial.println(lubeCount);
}

void loop()
{
    CheckSensor();
    oHall.Calibration();
    if(TSpeed.shouldRun()) TSpeed.run();
    if(TCHall.shouldRun()) TCHall.run();
    t.update();
}

void RError()
{
    if (eRrotate) t.every(600000, LubeChain);
}

void HReset()
{
    oHall.Reset();
}

void CheckSensor()
{
    oHall.Read();
    if (oHall.value>oHall.maxV-100){
        WheelRotate();
    }
    else {
        rotate=false;
    }
    //lcd.setCursor(0,0);
    //lcd.print("Hall: ");
    //lcd.setCursor(6,0);
    //lcd.print(oHall.value);
}

float Speed(unsigned long wrs, unsigned long wrls)
{
    float _speed=((wrs-wrsl)*lenghtWheel/10)*3600/1000;   //Скорость в км/ч
    wrsl=wrs;

    Serial.print("Speed= ");
    Serial.println(_speed);

    lCorrect(_speed);
    return _speed;
}


void lCorrect(float speed)
{
    wrl=wrlb;
    if (speed>100) wrl=wrlb*0.95;
    if (speed>140) wrl=wrlb*0.9;
    if (speed>180) wrl=wrlb*0.85;
    if (speed>210) wrl=wrlb*0.8;
    if (speed>240) wrl=wrlb*0.75;
}

void WheelRotate()
{
    if (!rotate)
    {
        wrs+=1;
        wr+=1;
        if (wr >= wrl)
        {
            if(TOiler.shouldRun()) TOiler.run();
            wr=0;
        }
        Serial.print("Wheel rotate on circle= ");
        Serial.println(wr);
        //lcd.setCursor(0,1);
        //lcd.print("Circle: ");
        //lcd.setCursor(8,1);
        //lcd.print(wr);
    }
    rotate=true;
    eRrotate=false;
}

void LubeChain()
{
    digitalWrite(cLubePin, HIGH);    // Включить подачу смазки
    digitalWrite(13, HIGH);
    delay(lubeDelay);                // Держать открытым подачу смазки
    digitalWrite(cLubePin, LOW);     // Выключить подачу смазки
    digitalWrite(13, LOW);
    lubeCount+=1;
    EEPROMWriteInt(0, lubeCount);

    Serial.print("Wheel lube count= ");
    Serial.println(lubeCount);
}

void EEPROMWriteInt(int p_address, int p_value)
{
    byte lowByte = ((p_value >> 0) & 0xFF);
    byte highByte = ((p_value >> 8) & 0xFF);
    EEPROM.write(p_address, lowByte);
    EEPROM.write(p_address + 1, highByte);
}

unsigned int EEPROMReadInt(int p_address)
{
    byte lowByte = EEPROM.read(p_address);
    byte highByte = EEPROM.read(p_address + 1);
    return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}


