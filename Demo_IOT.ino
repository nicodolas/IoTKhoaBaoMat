#include <WiFi.h>
#include <PubSubClient.h>
#include <Keypad.h>
#include <ESP32Servo.h>

const char *ssid = "Wokwi-GUEST";
const char *password = "";
const char *mqtt_server = "172.17.66.131";

WiFiClient espClient;
PubSubClient client(espClient);

// ====== Keypad setup ======
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {19, 18, 5, 17};
byte colPins[COLS] = {16, 4, 0, 2};
Keypad keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ====== Servo + GPIO ======
Servo doorServo;
int greenLED = 22;
int redLED = 23;
int buzzerPin = 15;

// ====== MQTT topics ======
const char *topic_cmd = "demo/door/command";
const char *topic_status = "demo/door/status";

// ====== Variables ======
String inputCode = "";
String correctCode = "1234";
int wrongCount = 0;
unsigned long lastUnlockTime = 0;
bool isUnlocked = false;

// ====== Function prototypes ======
void lockDoor();
void unlockDoor();
void alarmBuzzer();
void toneSuccess();
void toneError();

// ====== MQTT callback ======
void callback(char *topic, byte *payload, unsigned int length)
{
  String msg;
  for (unsigned int i = 0; i < length; i++)
    msg += (char)payload[i];
  Serial.printf("[MQTT] %s -> %s\n", topic, msg.c_str());

  if (msg == "LOCK")
  {
    Serial.println("🔒 Khoá từ web");
    lockDoor();
  }
  else if (msg == "UNLOCK")
  {
    Serial.println("🔓 Mở từ web");
    unlockDoor();
  }
  else if (msg == "SIREN")
  {
    Serial.println("🚨 Siren triggered manually!");
    alarmBuzzer();
  }
}

// ====== WiFi + MQTT setup ======
void setup_wifi()
{
  WiFi.begin(ssid, password);
  Serial.print("WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(400);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("MQTT...");
    if (client.connect("ESP32_Lock"))
    {
      Serial.println("connected!");
      client.subscribe(topic_cmd);
    }
    else
    {
      Serial.print("fail, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ====== Door control ======
void lockDoor()
{
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, HIGH);
  doorServo.write(0);
  client.publish(topic_status, "LOCKED");
  isUnlocked = false;
  Serial.println("🔒 Door locked");
}

void unlockDoor()
{
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, HIGH);
  doorServo.write(90);
  client.publish(topic_status, "UNLOCKED");
  isUnlocked = true;
  lastUnlockTime = millis(); // Bắt đầu đếm auto lock
  Serial.println("🔓 Door unlocked (auto-lock in 5s)");
}

// ====== Âm thanh phản hồi ======
void toneSuccess()
{
  // Hai âm ngắn cao độ tăng dần
  tone(buzzerPin, 1200, 100);
  delay(120);
  tone(buzzerPin, 1600, 100);
  delay(100);
  noTone(buzzerPin);
}

void toneError()
{
  // Hai âm trầm ngắn
  tone(buzzerPin, 400, 150);
  delay(200);
  tone(buzzerPin, 350, 150);
  delay(200);
  noTone(buzzerPin);
}

void alarmBuzzer()
{
  Serial.println("🚨 Intrusion detected! Triggering alarm...");
  client.publish(topic_status, "INTRUSION_ALERT");
  for (int i = 0; i < 3; i++)
  {
    tone(buzzerPin, 800);
    delay(1000);
    noTone(buzzerPin);
    delay(100);
  }
  digitalWrite(redLED, HIGH);
}

// ====== Setup ======
void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  doorServo.attach(21);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  lockDoor();
  Serial.println("🔐 Smart Lock Ready");
}

// ====== Loop ======
void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  // Auto-lock sau 5 giây
  if (isUnlocked && millis() - lastUnlockTime >= 5000)
  {
    Serial.println("⏳ Auto lock  after 5s");
    lockDoor();
  }

  char key = keypad.getKey();
  if (key)
  {
    tone(buzzerPin, 1000, 60); // tiếng phím bấm
    if (key == '#')
    {
      Serial.printf("Entered: %s\n", inputCode.c_str());
      if (inputCode == correctCode)
      {
        wrongCount = 0;
        Serial.println("✅ Correct code");
        toneSuccess();
        unlockDoor();
      }
      else
      {
        wrongCount++;
        Serial.printf("❌ Wrong code (%d/3)\n", wrongCount);
        toneError();
        client.publish(topic_status, "WRONG_PASSWORD");
        digitalWrite(redLED, HIGH);
        delay(100);
        digitalWrite(redLED, LOW);

        if (wrongCount >= 3) // 3 lần sai mã liên tiếp sẽ kích hoạt báo động
        {
          alarmBuzzer();
          wrongCount = 0;
        }
      }
      inputCode = "";
    }
    else if (key == '*')
    {
      inputCode = "";
      Serial.println("Cleared input");
    }
    else
    {
      inputCode += key;
    }
  }
}
