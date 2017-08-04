# CD Drive Emu

使用 stm32f407vet6 构建.


# keil 项目

使用 keil kde 编译, 不依赖 keil 库, 使用 `STM32F4xx_DSP_StdPeriph_Lib_V1.8.0` 库.
将 ST 库释放到上层目录(不在 git 中同步).
stm 库与 ide 是分开的, 可以选择使用 keil 库, 但是这些库依赖 keil 并且收费.
stm 的项目, 库与编译器是分离的, 库以源代码的形式提供, 所以当成项目的一部分进行编译即可.
keil 项目需要: 1.项目代码, 2.ST 库的所有源代码(芯片不同代码不同), 3.ST 库头文件,
4.ST 初始化汇编文件; 将这些文件加入项目中编译.

config 目录中的配置文件覆盖到 uv4 目录, 修改编辑器样式.