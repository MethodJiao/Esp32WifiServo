#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <esp_sleep.h>

const char* ssid = "SuperAP";
const char* password = "1407abcd";
const int led = 13;
const int myservoPin = 4;

int iowake = -1;
WebServer server(80);
struct tm timeinfo;               //定义存放时间的结构体
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 3600        /* Time ESP32 will go to sleep (in seconds) */

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<header>
    <meta charset="utf-8" />
    <title>Http Server Get-Start</title>
    <!-- fetch api-->
    <script type="text/javascript">
        function led(key) {
            fetch("/led/" + key)
        }
    </script>
</header>
<body>
    <div style="width:100%;height:100%;position:absolute" >
        <div style="width:100%;height:50%">
            <button onclick="led(1)" style="width:100%;height:100%;font-size:100px">启动</button>
        </div>
        <div style="width:100%;height:50%">
            <button onclick="led(0)" style="width:100%;height:100%;font-size:100px">关闭</button>
        </div>
    </div>
</body>
</html>)rawliteral";

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}
//舵机脉宽调制
void servoSweep(int servoPin, int angel) {
  for (int i = 0; i <= 50; i++) {
    int pulseWidth = map(angel, 0, 180, 500, 2480);
    digitalWrite(servoPin, HIGH);   //将舵机接口电平至高
    delayMicroseconds(pulseWidth);  //延时脉宽值的微秒数
    digitalWrite(servoPin, LOW);    //将舵机接口电平至低
    delayMicroseconds(20000 - pulseWidth);
  }
}

void setup(void) {
  pinMode(myservoPin, OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.config(IPAddress(192, 168, 11, 3), IPAddress(192, 168, 11, 1), IPAddress(255, 255, 255, 0));
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //设置时间服务器
  configTime(60 * 60 * 8, 0, "192.168.11.1", "ntp.tencent.com", "ntp.aliyun.com");
  if (getLocalTime(&timeinfo)) {

    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
      case ESP_SLEEP_WAKEUP_GPIO: iowake = millis(); break;
      case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
      case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
      case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
      case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
      default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
    }
  }
  //根节点处理
  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });
  //关闭按钮
  server.on("/led/0", []() {
    Serial.println("led0");
    digitalWrite(led, 0);
    server.send(200, "text/plain", "OK");
  });
  //启动按钮
  server.on("/led/1", []() {
    Serial.println("led1");
    digitalWrite(led, 1);
    servoSweep(myservoPin, 180);
    delay(200);
    servoSweep(myservoPin, 0);
    delay(200);
    digitalWrite(led, 0);
    server.send(200, "text/plain", "OK");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  //配置唤醒源
  const uint64_t WAKEUP_PIN_BITMASK = 0b0010;
  gpio_deep_sleep_hold_dis();  //在深度睡眠时禁用所有数字gpio pad保持功能。
  esp_deep_sleep_enable_gpio_wakeup(WAKEUP_PIN_BITMASK, ESP_GPIO_WAKEUP_GPIO_LOW);
  gpio_set_direction(GPIO_NUM_1, GPIO_MODE_INPUT);  //GPIO定向，设置为输入或输出

  Serial.println("HTTP server started");
}

void loop(void) {
  if (!getLocalTime(&timeinfo))  //无ntp时间 睡眠功能失效 但web响应正常
  {
    Serial.println("Failed to obtain time");
    digitalWrite(led, 1);
    delay(200);
    digitalWrite(led, 0);
    delay(200);
    server.handleClient();
  } else  //正常逻辑
  {
    if (iowake == -1)  //不是按键唤醒
    {
      if (timeinfo.tm_hour < 17) {
        int surplusMin = 60 - timeinfo.tm_min;
        Serial.println("ready to sleep：" + String(surplusMin) + " min");
        Serial.flush();
        esp_sleep_enable_timer_wakeup(surplusMin * 60 * uS_TO_S_FACTOR);  //小时整点唤醒
        esp_deep_sleep_start();
      }
    } else  //按键唤醒
    {
      if (millis() - iowake > 1000 * 60 * 5)  //唤醒5分钟
      {
        iowake = -1;
        return;
      }
    }
    // Serial.println(&timeinfo, "%A, %Y-%m-%d %H:%M:%S");
    server.handleClient();
  }
  delay(2);  //allow the cpu to switch to other tasks
}
