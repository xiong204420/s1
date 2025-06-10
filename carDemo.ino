#include <Arduino.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <MQ135.h>
#include <DHT.h>

#define SERVO_PIN D5
#define MOTO_IN1_PIN D7
#define MOTO_IN2_PIN D8
#define MOTO_ENA D2
#define DHT_PIN D6
#define MQ135_PIN A0

#define DHTTYPE DHT11

// 接线
// | ESP8266  | MG90S(舵机) |
// | -------- | ----------- |
// | GND      | 棕色        |
// | VCC      | 红色        |
// | 模拟引脚  | 橙色        |

// wifi info
const char *SSID = "1502-1";         // wifi 名字
const char *PASSWORD = "84864520";  // wifi密码

// MQTT info https://www.emqx.com/zh/mqtt/public-mqtt5-broker https://www.cnblogs.com/sjie/p/16328706.html
const char *MQTT_SERVER = "nas.atoo.top";  // MQTT 服务器地址
const int MQTT_PROT = 1883;                // MQTT 端口
const char *MQTT_USERNAME = "user";
const char *MQTT_PASSWORD = "0001";

// mqtt 主题
const char *MQTT_TOPIC_PUB_DATA = "car/pm25";     // 发布，小车运动数据
const char *MQTT_TOPIC_SUB_DATA = "car/control";  // 订阅，"速度，方向"
const char *CLIENT_ID = "esp8266-car-";

// SG90
int SG90_i;

float f_pm25 = 0.0;
float temperature = 20.0;  // 从DHT22获取
float humidity = 85.0;     // 从DHT22获取
int i_speed = 0, i_direction = 0;

// ticker.attach(s秒数, 函数名)
Ticker ticker;  // 定时调用某一个函数
WiFiClient espClient;
PubSubClient client(espClient);
Servo servo;
MQ135 mq135_sensor(MQ135_PIN);
DHT dht(DHT_PIN, DHTTYPE);

void init_wifi();                                                         // 初始化wifi
void mqtt_reconnect();                                                    // 重新连接wifi
void mqtt_msg_callback(char *topic, byte *payload, unsigned int length);  // mqtt 消息回调
void mqtt_pub_pm25();                                                     // 发布PM2.5
void SG90_reset();                                                        // SG 90 初始化
void SG90_update(int data);                                               // sg90
void Speed_update(int data);

void setup() {
  Serial.begin(115200);

  pinMode(MOTO_IN1_PIN, OUTPUT);
  pinMode(MOTO_IN2_PIN, OUTPUT);
  pinMode(MOTO_ENA, OUTPUT);

  dht.begin();
  /*
    语法
    servo.attach(pin)
    servo.attach(pin, min, max)
    参数说明
    servo，一个类型为servo的变量
    pin，连接至舵机的引脚编号
    min(可选)，舵机为最小角度（0度）时的脉冲宽度，单位为微秒，默认为544
    max(可选)，舵机为最大角度（180度时）的脉冲宽度，单位为微秒，默认为2400
  */
  // 第二个参数是0度脉冲宽度 第三个是180度的脉冲宽度 最后两个参数自己微调
  servo.attach(SERVO_PIN, 700, 2500);
  SG90_reset();  // SG90 初始化

  init_wifi();                               // 初始化 wifi
  client.setServer(MQTT_SERVER, MQTT_PROT);  // 设置mqtt 服务和端口
  client.setCallback(mqtt_msg_callback);     // 设置mqtt 回调函数

  ticker.attach(5, mqtt_pub_pm25);  // 心跳
  Speed_update(1);                  // 停车
}

void loop() {
  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  // if (isnan(humidity))
  //   humidity = 85.0;
  // if (isnan(temperature)) {
  //   temperature = 25.0;
  // }
}

void init_wifi() {
  Serial.println("Connecting to");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // 第一步: 创建连接
    String clientId = CLIENT_ID;
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC_SUB_DATA);  // 监听
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqtt_msg_callback(char *topic, byte *payload, unsigned int length) {
  // Serial.print("Message arrived [");
  // Serial.print(topic);  // 打印主题信息
  // Serial.print("] ");

  // ************** string 转 int **************
  if (strcmp(MQTT_TOPIC_SUB_DATA, topic) == 0) {
    String str = "";
    for (int i = 0; i < length; i++) {
      str += (char)payload[i];
    }

    int delimiterPos = str.indexOf(",");  // 2,100
    // Serial.println(str);
    String s_speed = str.substring(0, delimiterPos);
    String s_direction = str.substring(delimiterPos + 1, length);
    i_speed = s_speed.toInt();
    i_direction = s_direction.toInt();

    Speed_update(i_speed);
    SG90_update(i_direction);
  }
}

void mqtt_pub_pm25() {
  if (client.connected()) {
    f_pm25 = mq135_sensor.getCorrectedPPM(temperature, humidity);  // ppm

    String msg = "";
    msg += String(humidity) + ",";
    msg += String(temperature) + ",";
    msg += String(f_pm25);
    client.publish(MQTT_TOPIC_PUB_DATA, msg.c_str());  // 发布 PM2.5 值
    Serial.println(msg);
  }
}

void SG90_update(int data) {
  if (data > 180) data = 180;
  if (data < 0) data = 0;
  servo.write(data);
  //servo.writeMicroseconds(data);
  Serial.print("方向:");
  Serial.println(data);
  delay(30);
}

void SG90_reset() {
  servo.write(90);
  delay(30);
}

void Speed_update(int data) {
  // L298N驱动直流电机
  int SPEED_PWM = 20;  // 电机转动速度，根据控制板与电机测试来确定该值
  switch (data) {
    case 0:
      // 倒车控制
      digitalWrite(MOTO_IN1_PIN, LOW);
      digitalWrite(MOTO_IN1_PIN, HIGH);
      analogWrite(MOTO_ENA, SPEED_PWM);
      Serial.println("倒车.");
      break;
    case 1:
      // 停车控制
      digitalWrite(MOTO_IN1_PIN, LOW);
      digitalWrite(MOTO_IN1_PIN, LOW);
      analogWrite(MOTO_ENA, SPEED_PWM);
      Serial.println("停车.");
      break;
    case 2:
      // 前进控制
      digitalWrite(MOTO_IN1_PIN, HIGH);
      digitalWrite(MOTO_IN1_PIN, LOW);
      analogWrite(MOTO_ENA, SPEED_PWM);
      Serial.println("1档.");
      break;
    case 3:
      // 前进控制
      digitalWrite(MOTO_IN1_PIN, HIGH);
      digitalWrite(MOTO_IN1_PIN, LOW);
      analogWrite(MOTO_ENA, SPEED_PWM+100);
      Serial.println("2档.");
      break;
    case 4:
      // 前进控制
      digitalWrite(MOTO_IN1_PIN, HIGH);
      digitalWrite(MOTO_IN1_PIN, LOW);
      analogWrite(MOTO_ENA, SPEED_PWM+200);
      Serial.println("3档.");
      break;
  }
}