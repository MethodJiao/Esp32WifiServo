# Esp32WifiServo
基于esp32c3的wifi功能驱动舵机，并且加入深度休眠电源管理策略
![微信图片_20230309113250](https://user-images.githubusercontent.com/45934872/223909874-de474ec6-529e-4284-8bde-63d4e1c5b0c5.jpg)
# 引脚说明
1.请修改 ssid和password为自己的wifi名和密码

2.请修改 WiFi.config(IPAddress(192, 168, 11, 3), IPAddress(192, 168, 11, 1), IPAddress(255, 255, 255, 0));中服务网页地址，网关，掩码

3.请修改 configTime(60 * 60 * 8, 0, "192.168.11.1", "ntp.tencent.com", "ntp.aliyun.com"); ntp服务器，我在网关上开启了ntp服务所以我写192.168.11.1

4.myservoPin = 4; 4号管脚为舵机信号线，我的舵机是sg90

![image](https://user-images.githubusercontent.com/45934872/223910276-7b23176c-772a-4a29-9995-133cf561ccc9.png)

我自己写了舵机脉宽调制函数因为servo库不适用合宙esp32c3，sg90是50hz周期0.5ms-2.5ms脉宽控制0-180°旋转，其他舵机型号请修改该函数

5.1号管脚给低电平可以临时唤醒深度睡眠5分钟

![image](https://user-images.githubusercontent.com/45934872/223910548-823a27a6-373f-44bd-981b-036fbccfe4b1.png)
