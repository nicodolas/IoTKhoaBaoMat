#include <Keypad.h>
#include <ESP32Servo.h>
// arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir build .
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {19, 18, 5, 17};
byte colPins[COLS] = {16, 4, 0, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

Servo doorServo;
int greenLED = 22;
int redLED = 23;
String inputCode = "";
String correctCode = "1234";

void setup()
{
  Serial.begin(115200);
  doorServo.setPeriodHertz(50);
  doorServo.attach(21, 500, 2500);
  doorServo.attach(21);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);
  doorServo.write(0); // cửa đóng
  Serial.println("Smart Door Lock Ready!");
}

void loop()
{
  char key = keypad.getKey();
  if (key)
  {
    if (key == '#')
    {
      Serial.print("Entered code: ");
      Serial.println(inputCode);
      if (inputCode == correctCode)
      {
        unlockDoor();
      }
      else
      {
        wrongCode();
      }
      inputCode = ""; // reset
    }
    else if (key == '*')
    {
      inputCode = "";
      Serial.println("Code cleared");
    }
    else
    {
      inputCode += key;
      Serial.print("*");
    }
  }
}

void unlockDoor()
{
  Serial.println("\n✅ Correct code! Door unlocked!");
  digitalWrite(greenLED, HIGH);
  digitalWrite(redLED, LOW);
  doorServo.write(90);
  delay(3000);
  lockDoor();
}

void lockDoor()
{
  Serial.println("🔒 Door locked.");
  doorServo.write(0);
  digitalWrite(greenLED, LOW);
}

void wrongCode()
{
  Serial.println("\n🚨 Wrong code!");
  digitalWrite(redLED, HIGH);
  digitalWrite(greenLED, LOW);
  doorServo.write(0);
  delay(1500);
  digitalWrite(redLED, LOW);
}
