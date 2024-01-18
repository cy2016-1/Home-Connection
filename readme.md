# [Home-Connection ](https://gitee.com/yangfei_addoil/home-connection)



## 介绍

家庭互联(Home Connection)，初步是电脑桌互联生态搭建，如，检测人、自动&定时上下电一些设备；强关联项目：[Desktop-Robot](https://gitee.com/yangfei_addoil/desktop-robot)



## 开发思路

- [ ] 初步esp32（复杂处理）+esp8266（简单处理），使用ESP-MESH

- [ ] 后续添加蓝牙控制

- [ ] 统一搭建开发框架，VScode+Platform IO

- [ ] 手机扫一扫配网

- [ ] 串口配置从机名称

- [ ] 手机扫一扫配置从机名称

- [ ] 数据外发

- [ ] 添加显示终端

- [ ] esp32通过红外检测进行esp8266的控制

      ​




## 软件



## 硬件

1. 先用模块，ESP32-C3 Super Mini（带typec） 、ESP-01S模块（兼容继电器模块）
2. 各传感器模块线接
3. PCB：检测电池电压，两边多引出来电源引脚，多留外接引脚，两种外框，大的和18650座子一样（单面板，两边是esp32和充电模块，中间接陀螺仪和扩展），小的刚能放电池即可（双面板，typec右上角对齐，后面放凹处电池，边缘是扩展和陀螺仪插接口）；ADC分压检测/示波器，陀螺仪尽量兼容oled接口；都向外引出一段PCB（不用掰掉）




## 注意事项

1. esp-01s模块进入普通下载模式：需要将IO0和RST拉低再上电，其它引脚除了串口和电源都悬空，下载显示connecting的时候拔掉RST上的GND即可；如果是esp-01会有区别
2. 使用Esp-01s自动下载器进行下载，需要在**Platformio.ini里添加如下行**：upload_resetmethod = nodemcu
3. [VScode+Platform IO环境搭建](https://blog.csdn.net/qlexcel/article/details/121449441)
4. 下载的时候需要修改platformio.ini中的串口号：upload_port
5. VsCode PlatformIo 插件新建项目下载慢的解决办法 见 material - VsCode PlatformIo 插件新建项目下载慢的解决办法.pdf，实测ESP32新建项目大概十分钟左右，缓存二百多M，ESP8266要更快一些
6. ​




## 其它

演示视频：待上传B站

工程附件：待完成

B站：“大饼酱人”

CSDN：“大饼酱人”（https://blog.csdn.net/Fei_Yang_YF）

公众号：“大饼匠人”

微信交流群：待创建

众筹链接：待创建

可定制化或自行修改源仓库（https://gitee.com/yangfei_addoil/home-connection）

