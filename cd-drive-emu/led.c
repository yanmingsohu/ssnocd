#include "led.h"
#include "stm32f4xx.h"
#include <stm32f4xx_tim.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>
#include <misc.h>


#define LED1_USE_TIM    TIM2
#define LED1_GPIO_GRP   GPIOC
#define LED1_GPIO_PIN   GPIO_Pin_0
#define LED1_AHB1       RCC_AHB1Periph_GPIOC

static uint8_t   isset1;
static uint32_t  cc = 1;


/* 
 * 将 led 1, 配置为闪烁模式, 使用定时器;
 * # 将 led 2 配置为运行提示, 每 1s 闪烁一次认为系统未宕机; 
 */
void init_led_splash() {
  RCC_AHB1PeriphClockCmd(LED1_AHB1, ENABLE);
  
  GPIO_InitTypeDef  led1_gpio;
  led1_gpio.GPIO_Pin   = LED1_GPIO_PIN;
  led1_gpio.GPIO_Mode  = GPIO_Mode_OUT;
  led1_gpio.GPIO_OType = GPIO_OType_PP;
  led1_gpio.GPIO_Speed = GPIO_Speed_2MHz;
  led1_gpio.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(LED1_GPIO_GRP, &led1_gpio);
  
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  TIM_TimeBaseInitTypeDef led1;
  led1.TIM_ClockDivision  = 0;
  led1.TIM_CounterMode    = TIM_CounterMode_Up;
  led1.TIM_Period         = 0xFFFF;
  led1.TIM_Prescaler      = ((SystemCoreClock /2) / 1000000) - 1;
  TIM_TimeBaseInit(LED1_USE_TIM, &led1);
  
  //TIM_PrescalerConfig(LED1_USE_TIM, 0xFFFF, TIM_PSCReloadMode_Immediate);
  TIM_ARRPreloadConfig(LED1_USE_TIM, ENABLE);
  TIM_ITConfig(LED1_USE_TIM, TIM_IT_Update, ENABLE);
  
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel                    = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  TIM_Cmd(LED1_USE_TIM, ENABLE);
}



/* 
 * 在主循环中调用, 指示系统正常运行 
 */
void led_sys_update() {
}


void TIM2_IRQHandler() {
  if (--cc != 0) {
    // 这样做会消耗 cpu 周期
    return;
  }
  cc = 100000;
  
  if (isset1) {
    GPIO_SetBits(LED1_GPIO_GRP, LED1_GPIO_PIN);
    isset1 = 0;
  } else {
    GPIO_ResetBits(LED1_GPIO_GRP, LED1_GPIO_PIN);
    isset1 = 1;
  }
}