中文 | [English](./README.md)

---

# RTL8720DN-WiFi嗅探器
使用 [RTL8720DN](https://www.amebaiot.com/en/amebad-bw16-arduino-getting-started) 2.4G/5G 双频 Wifi 模块进行无线设备检测

## 特征
* 无需连接，无需密码
* 被动扫描
* 扫描`STA`使用的信道
* 仅扫描一系列信道
* 修改`scanTimePerChannel`
* 自动重复扫描

## 使用的库
* [Ameba Arduino SDK](https://github.com/ambiot/ambd_arduino)
* [SimpleCLI](https://github.com/SpacehuhnTech/SimpleCLI) (从Arduino IDE中下载）
* [RTC](https://github.com/ambiot/ambd_arduino/blob/94b2bae9114552276e61581620aa5e3645e4de36/Arduino_package/hardware/libraries/RTC/examples/RTC/RTC.ino)

## 硬件
* [RTL8720DN](https://www.amebaiot.com/en/amebad-bw16-arduino-getting-started) (BW16)

## 用法
以下命令可通过Arduino IDE的串口监视器（115200 波特率）来执行：
* `filter [mac]` -> 设置搜索设备的[MAC-Address](https://kb.wisc.edu/helpdesk/page.php?id=79258)

* `station [-v]` -> 列出 2.4G/5G 频谱中的所有可用 WiFi `Station`

* `scan [from] [to] [-v] [-r]` -> 在选定的[信道](https://en.wikipedia.org/wiki/List_of_WLAN_channels) 上查找 MAC 地址
  - 通道 1..13 用于 2.4 GHz (802.11b/g/n/ax)
  - 用于 5 GHz 的通道 32..173 (802.11a/h/j/n/ac/ax)
  - 使用 `-r` 重复此命令
  - 使用 `-v` 显示所有找到的 MAC 地址
  - 例如： `scan 1 13 -v -r` -> 会扫描2.4GHz 频段的信道，显示更多信息，并重复执行
  
* `time [h] [m] [s]` -> 设置内部时钟

## 用途
* 查看周围信道的拥挤程度
* 查看某一无线设备是否就在附近
* 等等...
