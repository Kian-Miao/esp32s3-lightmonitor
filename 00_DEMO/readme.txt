ESP32-S3 Wireless Light Monitoring Terminal 工作日志

一、当前阶段

当前项目处于第一阶段：BH1750 光照采集 + LCD 显示 + 初步模块化封装。

本阶段重点不是加入更多外设，而是先把已经跑通的 BH1750、I2C、LCD、按键控制整理成较清晰的模块结构，为后续 FreeRTOS 多任务、Queue、Data Manager、LVGL、BLE、Wi-Fi、MQTT 做准备。

二、已实现功能

1. BH1750 光照传感器读取
   - 使用 ESP32-S3 通过 I2C 与 BH1750 通信。
   - 读取 BH1750 两字节原始数据。
   - 将原始数据换算为 lux 光照强度。
   - 使用 esp_err_t 返回读取是否成功。
   - 使用指针参数 uint16_t *raw_data 将原始数据带出函数。

2. LCD 显示
   - 使用 SPI 驱动 LCD 屏幕。
   - 开机后 LCD 可以显示固定字符串 light:。
   - 根据当前状态显示 off、fail 或实时光照强度。
   - 显示 off/fail 时使用空格覆盖旧数据，避免短字符串和旧数字残留混在一起。

3. 按键控制
   - 使用 GPIO6、GPIO7、GPIO8 作为三个按键输入。
   - 按键采用内部上拉，按下时为低电平。
   - GPIO6：发送 BH1750 PowerDown 命令，系统显示 off。
   - GPIO7：发送 PowerOn + Reset + HResolutionMode，复位后继续进入测量模式。
   - GPIO8：发送 PowerOn + HResolutionMode，开启连续高分辨率测量。

4. 传感器状态管理
   - 在 sensor_data_t 结构体中加入 lux_data、sensor_ok、sensor_enable 成员。
   - lux_data 保存光照强度。
   - sensor_ok 表示本次读取是否成功。
   - sensor_enable 表示当前是否允许读取和显示光照数据。

三、当前搭建的代码架构

当前项目已经初步拆分为多个 ESP-IDF component：

- main：主程序入口，负责初始化、主循环和 LCD 显示逻辑。
- SENSOR：上层传感器接口，负责 sensor_init、sensor_read、sensor_switch。
- BH1750：BH1750 底层命令发送和数据读取。
- IIC：I2C 总线初始化。
- LCD：LCD 初始化、清屏、字符/数字/浮点数显示。
- SPI：SPI 总线初始化和数据发送。
- KEY：按键 GPIO 初始化和按键扫描。
- LED：LED GPIO 初始化和翻转函数。

当前数据流大致为：

main
  -> sensor_init()
       -> iic_init()
       -> key_init()
       -> led_init()

main while(1)
  -> sensor_switch(&data)
       -> key_scan()
       -> 根据按键发送 BH1750 控制命令
       -> 修改 data.sensor_enable

  -> 如果 data.sensor_enable == false
       -> LCD 显示 off

  -> 如果 data.sensor_enable == true
       -> sensor_read(&data)
       -> bh1750_read_data(&raw)
       -> data.lux_data = raw / 1.2f
       -> LCD 显示光照强度

四、下一步目标

1. 完善 sensor 模块接口
   - 继续整理 sensor_init、sensor_read、sensor_switch 的职责。
   - 让 main 尽量不直接关心 BH1750 的底层命令。
   - 后续可以把按键控制命令进一步抽象为更清晰的状态控制函数。

2. 改善按键扫描逻辑
   - 当前 key_scan 使用 while 等待按键松开，属于阻塞式扫描。
   - 后续可以优化为非阻塞扫描，避免主循环被按键长按卡住。
   - 可以加入 KEY_NONE、KEY_POWER_DOWN、KEY_RESET、KEY_POWER_ON 这样的宏定义，提高可读性。

3. 加入 FreeRTOS 数据骨架
   - 创建 Sensor Task，周期性读取 BH1750。
   - 创建 Queue，用于传递 sensor_data_t。
   - 创建 Data Manager Task，接收传感器数据并统一分发。
   - 第一版目标：串口能看到 Sensor Task 发送数据，Data Manager 接收数据。

4. 后续显示架构演进
   - 当前 LCD 由 main 直接调用显示函数。
   - 后续进入 LVGL 阶段后，应建立 UI Queue 和 LVGL Task。
   - 保证只有 LVGL Task 直接调用 LVGL API，避免线程安全问题。

五、遇到的问题与修复记录

1. sensor_data_t 写法错误：typedef struct 后没有正确给结构体起别名，导致 sensor_data_t 标红；改为在 sensor.h 中正确定义 typedef struct {...} sensor_data_t。

2. 结构体重复定义：sensor.h 和 sensor.c 中都定义了 sensor_data_t，导致 redefinition；删除 sensor.c 中重复的结构体定义，只在 sensor.h 中保留。

3. 缺少 esp_err_t 头文件：bh1750.h 和 sensor.h 使用 esp_err_t 但没有包含 esp_err.h；补充 #include "esp_err.h"。

4. 缺少 bool 头文件：sensor_data_t 使用 bool 类型但没有包含 stdbool.h；应在 sensor.h 中包含 #include <stdbool.h>。

5. 指针判断写成赋值：if(raw_data = NULL) 会把指针赋成 NULL；改为 if(raw_data == NULL)。

6. 空指针风险：在 sensor_switch 中只声明 sensor_data_t *data 没有指向有效结构体；改为 sensor_switch(sensor_data_t *data)，由 main 传入 &data。

7. 未初始化结构体变量：main 中 sensor_data_t data 没有初始值，导致 sensor_enable 随机为 false；改为创建时初始化 lux_data、sensor_ok、sensor_enable。

8. 未初始化按键返回值：key_scan 中 key_num 未赋初值，没按键时返回随机值；改为 uint8_t key_num = 3，3 表示无按键。

9. GPIO 引脚冲突：KEY 使用 GPIO11，LCD SPI MOSI 也使用 GPIO11，导致 LCD 不显示；把按键改为 GPIO6、GPIO7、GPIO8。

10. pin_bit_mask 写法错误：1ULL << (GPIO6) | GPIO7 | GPIO8 只正确配置了 GPIO6；改为每个 GPIO 单独左移后再按位或。

11. app_main 提前退出：显示 off/fail 后使用 return，导致程序停止循环，按键无法继续响应；删除 return，让 while 循环持续运行。

12. sensor_read 放在循环外：只读取一次光照，LCD 不会实时变化；改为在 while 循环中周期性调用 sensor_read(&data)。

13. PowerDown 后仍继续读取：按下关机后主循环仍无条件读取 BH1750；加入 sensor_enable 状态，关闭时只显示 off，不继续读取。

14. off 和数字混显示：LCD 显示 off 字符太短，没有覆盖旧光照数字；改为显示 "off      " 和 "fail      " 清理残留字符。

15. 按键随机成功：主循环延时较长，短按容易错过扫描时机；测试时延长按键时间，并调整循环/显示逻辑提高响应稳定性。

六、阶段总结

本阶段已经完成从“单纯能读取并显示光照”的 Demo，向“有模块划分、有状态结构体、有错误判断、有按键控制”的工程雏形过渡。

目前项目已经具备进入 FreeRTOS 数据骨架阶段的基础。下一步应优先建立 Sensor Task + Queue + Data Manager，而不是急着加入 BLE、Wi-Fi、MQTT 或 LVGL。
