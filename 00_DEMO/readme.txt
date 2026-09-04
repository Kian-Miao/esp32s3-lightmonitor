ESP32-S3 无线光照监控与控制终端 工作日志

一、项目完成情况

本项目已经完成从光照采集、本地显示、本地按键控制，到 Wi-Fi/MQTT 云端通信、BLE 近场通信的完整联调。当前系统可以通过 BH1750 实时采集光照强度，在 LCD 上显示当前状态，并支持按键、MQTT、BLE 三种方式控制传感器开机、关机和复位。

项目不是单个函数堆在 main 里跑，而是拆成多个 ESP-IDF component，并用 FreeRTOS 任务和 Queue 做数据流转。这样主程序只负责初始化和创建任务，具体的采集、显示、通信、控制逻辑都放到对应模块里，后续继续扩展也比较清楚。

二、实现了什么功能

1. 光照采集功能

使用 ESP32-S3 通过 I2C 与 BH1750 通信，完成传感器初始化、控制命令发送和光照原始数据读取。读取到的两字节原始数据经过换算后得到 lux 光照强度，并保存到 sensor_data_t 结构体的 lux_data 成员中。

2. LCD 本地显示功能

LCD 通过 SPI 驱动，能够显示 light:、实时光照值、off 和 fail 等状态。传感器正常开启时显示实时光照强度；传感器关闭时显示 off；读取失败时显示 fail。显示 off/fail 时用空格覆盖旧数据，避免出现 off 和旧光照数字混在一起的情况。

3. 本地按键控制功能

使用 GPIO6、GPIO7、GPIO8 作为三个按键输入，分别对应 BH1750 的关机、复位、开机测量。按键扫描结果进入 sensor_switch()，由该函数统一修改 sensor_enable 状态并向 BH1750 发送 PowerDown、PowerOn、Reset、HResolutionMode 等命令。

4. FreeRTOS 多任务与 Queue 通信

项目拆分为 sensor_task 和 data_manager_task 两个主要任务。sensor_task 负责按键/MQTT/BLE 命令处理、传感器状态更新和光照采集；data_manager_task 负责从 Queue 接收 sensor_data_t 数据，并统一完成 LCD 显示、MQTT 上报和 BLE 数据更新。

Queue 传递的是结构体数据的副本。sensor_task 每次循环把当前 send_data 发送进队列，data_manager_task 再取出 receive_data 使用。这样两个任务不需要直接共享同一个结构体变量，数据关系更清楚，也减少了混乱修改状态的风险。

5. Wi-Fi 联网与状态判断

ESP32-S3 配置为 Wi-Fi STA 模式，连接路由器后通过事件回调获取 IP 地址。使用 FreeRTOS Event Group 保存 Wi-Fi 是否已经拿到 IP，外部模块通过 wifi_check() 判断当前是否可以进行网络通信。后续还加入了断线重连逻辑，Wi-Fi 断开后会自动重新连接。

6. MQTT 云端通信功能

项目接入 EMQX Cloud。ESP32-S3 连接 MQTT Broker 后，向 esp32/light 主题上报光照数据，同时订阅 esp32/control 主题接收远程控制命令。MQTTX 发送 on、off、reset 后，ESP32-S3 能够解析命令并交给 sensor_switch() 执行，使云端控制和本地按键控制走同一套传感器状态逻辑。

7. BLE GATT 近场通信功能

项目加入 NimBLE GATT Server，手机可以通过 BLE 读取当前光照数据和传感器状态，也可以写入 on、off、reset 控制命令。BLE 接收到的命令不直接操作 BH1750，而是先保存为待处理命令，再由 sensor_task 统一取出执行，保证 BLE、MQTT、按键三种控制方式不会各自修改状态造成冲突。

三、项目架构整理

当前工程主要 component 如下：

main：程序入口，负责初始化 LCD、Sensor、Wi-Fi、MQTT、BLE，创建 Queue 和 FreeRTOS 任务。

APP_TASK：存放 sensor_task 和 data_manager_task，负责项目运行时的主要任务逻辑。

SENSOR：上层传感器接口，封装 sensor_init、sensor_read、sensor_switch，统一管理传感器读取和开关状态。

BH1750：BH1750 底层驱动，负责发送传感器命令和读取原始光照数据。

IIC：I2C 总线初始化和底层通信支持。

LCD / SPI：LCD 显示和 SPI 通信驱动。

KEY：按键 GPIO 初始化和按键扫描。

WIFISTA：Wi-Fi STA 初始化、事件处理、自动重连和连接状态判断。

MQTT_APP：MQTT 初始化、连接事件处理、主题订阅、数据发布和远程命令解析。

MYBLE：BLE GATT Server 初始化、手机读写特征处理、蓝牙命令解析和光照状态缓存。

四、核心运行流程

程序启动后，app_main() 先完成 LCD、传感器、Wi-Fi、MQTT、BLE 的初始化。Wi-Fi 拿到 IP 后再启动 MQTT，避免网络还没准备好就开始 TLS 连接。

随后创建 sensor_queue，并启动两个 FreeRTOS 任务。sensor_task 周期性检查按键、MQTT 命令和 BLE 命令，根据命令更新 sensor_enable。如果传感器处于开启状态，就读取 BH1750 光照数据；如果处于关闭状态，就不再占用 I2C 读取。每轮循环结束后，把当前 sensor_data_t 发送到 Queue。

data_manager_task 阻塞等待 Queue 数据。收到数据后，根据 sensor_enable 和 sensor_ok 决定 LCD 显示内容，同时把实时光照或 off 状态发布到 MQTT，并把最新数据更新给 BLE，供手机读取。

五、遇到的错误、困难和解决方式

1. 结构体 typedef 写法错误，导致 sensor_data_t 标红；后来统一在 sensor.h 中正确定义结构体类型。

2. sensor.c 和 sensor.h 重复定义结构体，导致 redefinition；删除 c 文件里的重复定义，只保留头文件声明。

3. 使用 esp_err_t、bool 时缺少对应头文件；补充 esp_err.h 和 stdbool.h 后解决。

4. 指针判断写成 raw_data = NULL，实际变成赋值；改为 raw_data == NULL。

5. 在 sensor_switch() 中只声明结构体指针但没有指向有效变量，存在野指针；改为由外部传入 sensor_data_t *data。

6. 对 data、&data、*data 的含义理解不清，导致函数传参混乱；通过统一传结构体地址，让函数内部用 data->成员 修改同一份状态。

7. key_scan() 没按键时返回值不确定，导致按键状态随机；给默认返回值，表示无按键。

8. 按键 GPIO 与 LCD SPI 引脚冲突，导致 LCD 无法显示；重新选择 GPIO6、GPIO7、GPIO8 作为按键输入。

9. GPIO pin_bit_mask 写法错误，只正确配置了一个引脚；改为每个 GPIO 都单独左移后再按位或。

10. app_main 里显示 off/fail 后直接 return，程序退出导致按键不再响应；删除 return，让主循环和任务持续运行。

11. LCD 显示 off 后残留旧光照数字，出现 off3.102 这类混合显示；显示短字符串时补空格覆盖旧内容。

12. 早期把 sensor_read 放在不合适的位置，导致关机后仍然读取或显示旧数据；改为先判断 sensor_enable，再决定是否读取 BH1750。

13. 最初所有逻辑都放在 main 里，功能变多后不好维护；拆出 APP_TASK、SENSOR、WIFISTA、MQTT_APP、MYBLE 等模块。

14. Queue 创建和任务创建缺少返回值判断，失败时不好定位；增加 xQueueCreate 和 xTaskCreate 的返回值检查。

15. 任务循环中缺少合适 delay，曾触发 Task Watchdog；在任务循环中加入 vTaskDelay，让系统调度有机会运行其他任务。

16. data_manager_task 打印过快、不按预期一秒一次；把采集周期放在 sensor_task 中控制，Queue 接收任务只负责处理收到的数据。

17. Wi-Fi 事件组一开始写在事件回调里创建，导致事件组被反复创建、状态容易丢失；改为在 wifista_init() 中只创建一次。

18. IP 事件数据类型使用不准确，打印 IP 容易出问题；改为使用 ip_event_got_ip_t 读取 event->ip_info.ip。

19. MQTTX 显示已连接但 ESP32 没反应，原因是 MQTTX 连接成功不等于 ESP32 已连接；后来通过串口确认 ESP32 是否打印 mqtt:connected 和 mqtt_receive。

20. MQTT 连接云端时出现 getaddrinfo 失败，说明域名解析或网络还没准备好；改为等待 Wi-Fi 获取 IP 后再启动 MQTT。

21. 加入 BLE 后 MQTT TLS 报 mbedtls_ssl_setup returned -0x7F00，属于 TLS 内存分配失败；降低 mbedTLS 缓冲区，开启动态缓冲，并让 mbedTLS/NimBLE 尽量使用 PSRAM。

22. 加入 BLE 后固件超过默认 app 分区大小；切换到更大的 app 分区配置后解决。

23. BLE 写入一开始按字节判断命令，手机调试助手使用不方便；改为解析 text 文本 on、off、reset。

24. BLE 如果直接在回调里控制 BH1750，会绕过 sensor_task 的状态管理；改为 BLE 只保存命令，由 sensor_task 统一执行。

25. MQTT/BLE/off 状态曾出现重复发布；通过保存上一次 sensor_enable 状态，只在状态变化时发布 off，实时光照则按周期发布。

六、阶段总结

这个项目从最开始的 BH1750 读数和 LCD 显示，逐步扩展到了多任务、队列通信、Wi-Fi、MQTT 和 BLE。过程中最大的问题不是某一个函数怎么写，而是多个模块同时运行后，状态到底由谁管理、数据到底从哪里流向哪里。

最终的处理思路是：sensor_task 统一管理传感器状态和控制命令，data_manager_task 统一负责显示和通信输出，Queue 负责在两个任务之间传递传感器数据。这样按键、MQTT、BLE 虽然入口不同，但最后都会回到同一套控制逻辑里，项目整体更稳定，也更像一个完整的嵌入式小系统。