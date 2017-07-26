# Sega Saturn cd-drive

sage saturn(ss) 光驱与主板连接协议分析.
ss 光驱与主板连接线分为 21 针与 20 针.


# 开发模拟器硬件

晒邦 DUE R3 首款ARM32位主控沉金工艺 送线(C6B3)
https://item.taobao.com/item.htm?id=43733379697&ali_refid=a3_420434_1006:1103831585:N:arduino:391e7a6e920ff266d64f69c6d3a9c61e&ali_trackid=1_391e7a6e920ff266d64f69c6d3a9c61e&spm=a230r.1.1957635.48.ebb2eb26jTXUw

Based on the ARM® Cortex®-M3 processor, the Microchip's SAM3X8E runs at 84MHz 
and features 512KB of flash memory in 2 x 256KB banks and 100KB of SRAM in 
64KB +32KB banks, with an additional 4KB as NFC (NAND Flash controller) SRAM.


# CD 光驱主板上的 ic

1. m56754sp     - 4 通道马达控制器.
2. M37477m8     - 8kB rom, 192byte ram. 单片机;
3. MN662724RPE  - CD 信号处理器. 该芯片没有详细文档, 可以参考 MN66279RSC.
4. AN8809SB     - ?


# cd 光盘规范

2352  一个扇区, 第一个扇区在 00, 从 0x10 (偏移 0x9300) 扇区开始光盘基本信息
ss cdrom 直接读取光盘上的信息.

在CD-DA中，立体声有两个通道，每次采样有2个16位的样本，
左右通道的每个16位数据分别组成2个8位字节，6次采样共24字节组成一帧?
一帧有一个8位的控制字节.

TOC: 
    Every CD (Compact Disc) has a TOC (Table Of Contents) section located 
    in the "lead-in" area on the CD. For an CD-DA, (audio CD), the "lead-in" area 
    is located between radius 23mm and 25mm(光盘最内圈)

    The size of the "lead-in" area is about, 4,500 sectors with a capacity of about 9 MB.
    The TOC in the "lead-in" area contains the total length of the recording session, a list 
    of tracks and their starting addresses and some other information.

    Note that on a multi-session CD, each session will have its own "lead-in" area and TOC.
    On a recordable/writable CD (CD-R), there is an extra area called PMA (Program Memory Area)
    before the "lead-in" area. The PMA is located between radius 22.35mm and radius 23m.

    The PMA is used by the recording process to store a temporary copy TOC, 
    before the session is closed. After the session is closed, 
    the TOC is written to the "lead-in" area.
    
C2 ERRORS
    C2 Errors refer to bytes in a frame (24 bytes per frame, 98 frames per block) and is an 
    indication of a CD player's attempt to use error correction to recover lost data.  
    C2 errors can be serious.  In theory, a CD player should correct them.   
    C2 errors are usually an indication of poor media quality, 
    or the failure of a CD burner to produce a quality burn (see conclusion).


## 21 pin

* 光驱仓开关在电源连接线上.
* 信号线分为2组, 控制组与数据组; 


1. OSCSW    - 33.33Mhz 时钟, 从开机开始发送, 下降 10ns, 上升 20ns, 持续14次传输13字节, 空闲 0.2um
2. [GND]

3. CDATA    - 启动后维持0; 在p6拉低前拉高, p7时钟开始前恢复0?
4. HDATA    - cd-drive 返回的状态.
5. COMREQ   - 下拉开始一个字节, 几乎与6同时下拉
6. COMSYNC  - 下拉启动一个命令序列, 几乎与5同时下拉, 持续一个字节后上拉
7. COMCLK   - 时钟, 50kHZ(10um), 当 5低时发送, 高时停止, 在上升沿读取数据
8. [GND]

! 9.  ??    - 直通 14 ?
! 10. SI    - 音乐盘 始终为 0; Frame(帧) 同步时钟, 下降沿开始, 一帧有98个小帧, 与p13 下降沿同步.

11. RESET   - 开机后 reset 为低, 拉高 0.3ms 后复位结束
12. [GND]
13. mpx     - 左右声道切换/串行数据字节信号 44.25khz, 在 p14 高时改变; cdda 中32bit切换
14. CKX     - 串行数据时钟 cdda:2.7mhz, 数据:4.167mhz; 上升沿时读取数据;
15. SDAS    - 串行数据, p14 上升沿时读取数据; [参考](MN66279RSC.pdf p58)
16. [GND]

17. ?       - 长时间在0, 未知的原因被拉高
18. subout  - p15有数据时, p21 拉底时拉底; 下降沿为开始一帧;
19. subck   - p18拉底后, 有8个脉冲(172.4khz); 每个帧开始发送 8 bit subcode; 数据cd为0;
20. c2f     - p13 频率的 1/2, 与p13 下降沿同步, 读取错误时有脉冲.
21. cfck    - 7.375k (6.44k) 小帧同步时钟; 上升沿为一小帧, 每一小帧 ? bit;


# 1-7 pin 协议

上电 0.5s 后 5,6 上拉, 1s 后 p3 上拉,
准备好后 req 和 sync 每16.63ms 拉低3.1ms (60hz) 来测试主机是否准备完毕,
直到检测到 COMCLK 开始发送时钟, 认为主机启动, 开始传送数据. 并将 CDATA 拉低.

sync 一个下降沿, sh1 准备发送时钟.
req 下降持续 8 个时钟, 当 req 下降时, sh1 发送时钟脉冲.
一个序列结束后休眠 14.24ms

当 3(cdrom到cpu) 脚发送一个序列, 下一个序列 4(cpu到cdrom) 脚才应答, 
所以请求序列可能重复发送.


# TOC 分析

抓取的数据:
point    adrctrl     iscd      msf
01        41          1        00 02 00
02        01          1        08 54 22
a0        41          1        01 00 00
a1        41          1        02 00 00
a2        41          1        09 02 21

模拟的数据:
TOC: 41(AT) 00(TNO) 01(PT)  00 00 00 00(0) 00 02 00
TOC: 01(AT) 00(TNO) 02(PT)  00 00 00 00(0) 08 36 16
TOC: 41(AT) 00(TNO) a0(PT)  00 00 00 00(0) 01 00 00
TOC: 01(AT) 00(TNO) a1(PT)  00 00 00 00(0) 02 00 00 @1
TOC: 01(AT) 00(TNO) a2(PT)  00 00 00 00(0) 09 02 15 @1

! @1 与抓取的数据不同, 不知道有没有影响


# CD 数据格式

在 cd 上物理坑道将每个字节 8 位转换为 14 位进行保存 
[数据编码](https://en.wikipedia.org/wiki/Compact_Disc_Digital_Audio#Technical_specifications), 
读取时由 cd 控制器还原为 8 位,

cd 光盘最小单位(T10-1363-D. p56) = 小帧 Small Frame = 588 bits 
	= 12 同步位 + 1 子通道数据 + 12 数据 + 4 CIRC + 12 数据 + 4 CIRC

一节 Sector = 98 小帧

一节的用户数据 = 98 * 24 (小帧中的数据, 其他部分被处理掉) = 2352 bytes

每节的同步位固定 12 字节:  (T10-1363-D. p64)
0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00

> 通过 cd 控制器接口读取的数据已经解析并格式化为一字节 8 位, 
> 并按照规范重新排列. (ST-040-R4-051795.pdf p10)

## MDF 文件

MDF 镜像中仅保存用户数据, 一帧 2352 字节.


## 每个序列13字节, 字节序

01        命令
02 - 03   ?? =80
04 - 11 	8 个字节
12      	crc 校验
13      	总是 0x00 停止位?? 有时 0x80


## pin3 CDATA / pin4 HDATA

首先传到的 bit 放在 byte 的最低位.
p3 由 cdrom 向 cpu 传送当前状态.
p4 由 cpu 向 cdrom 传送指令.


# 资料


[游戏](https://shop33762426.taobao.com/category-1032036304.htm?spm=a1z10.5-c.w4002-2168085120.103.dS8J73&_ksTS=1497180742372_218&callback=jsonp219&mid=w-2168085120-0&wid=2168085120&path=%2Fcategory-1032036304.htm&search=y&parentCatId=1032033745&parentCatName=%CA%C0%BC%CESEGA&catName=%C8%D5%B0%E6%CD%C1%D0%C7SS%D3%CE%CF%B7&catId=1032036304&pageNo=5#anchor)

[手柄文档](http://www.gamesx.com/controldata/saturn.htm)

[主机开发文档集](https://segaretro.org/Sega_DTS_Saturn_official_documentation)