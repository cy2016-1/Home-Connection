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

1. 先使用的WIFI MESH，功耗太大而且不能外网远程；换成阿里云后，发现可视化的WEB收费了（阿里云物联网平台和飞燕平台都是），放弃；小米IOT，第一轮公测名额没了，蹲下一轮；ESP HOME + [Home Assistant](https://www.cnblogs.com/manastudent/p/17425773.html) 太复杂（我是弱鸡），还需要内网穿透和一个常开机的服务器端（有点费电和贵）；巴沙云和点灯科技，优点是可以联动米家，但是免费的就能搞几个设备，可玩性不太强，放弃；最后是ONENET，着实是无奈之举，先凑活着，它和阿里云大差不差，唯一的突出是免费版支持一个web可视化页面，优点是支持场景联动，支持的免费设备也不少，支持协议也不少（mqtt要比http省电），而且可玩性还行（esp8266和esp32都行），缺点是自成一体，目前还不知道怎么接入米家或联动别的设备
2. 综合对比了小米IOT、阿里云、ESPHOME、巴沙云、点灯科技、ONENET的开发难度、费用、扩展性、WEB可视化和个人的应用场景，决定先使用ONENET，因为小米IOT个人开发公测名额没了，后续再试试小米IOT
3. ​



## 硬件

1. 先用模块，ESP32-C3 Super Mini（带typec） 、ESP-01S模块（兼容继电器模块）

2. 各传感器模块线接

3. PCB：检测电池电压兼容ADC分压检测/示波器，两边多引出来电源引脚，多留外接引脚，两种外框，大的和18650座子一样（单面板，两边是esp32和充电模块，中间接陀螺仪和扩展），小的刚能放电池即可（双面板，typec右上角对齐，后面放凹处电池，边缘是扩展和陀螺仪插接口）；陀螺仪尽量兼容oled接口；oled0.96和0.91的位置区分；都向外引出一段PCB（不用掰掉）；加电源开关；第三种外框和usb成品外壳一致，引出USB公口；

   ​




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

