#include <led.h>
#include <stm32f4xx.h>
#include <stm32f4xx_tim.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_exti.h>
#include <stm32f4xx_syscfg.h>
#include <misc.h>


#define LED1_USE_TIM    TIM2
#define LED1_GPIO_GRP   GPIOC
#define LED1_GPIO_PIN   GPIO_Pin_0

static uint8_t   isset1;
static uint8_t   isset2;


/* 
 * 将 led 1, 配置为闪烁模式, 使用定时器, 每 1s 闪烁一次认为系统未宕机;
 */
void init_led1_splash() {
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  
  // C0 配置为 led 控制引脚
  GPIO_InitTypeDef led1_gpio;
  led1_gpio.GPIO_Pin    = LED1_GPIO_PIN;
  led1_gpio.GPIO_Mode   = GPIO_Mode_OUT;
  led1_gpio.GPIO_OType  = GPIO_OType_PP;
  led1_gpio.GPIO_Speed  = GPIO_Speed_2MHz;
  led1_gpio.GPIO_PuPd   = GPIO_PuPd_NOPULL;
  GPIO_Init(LED1_GPIO_GRP, &led1_gpio);
  
  // 启动 TIM2 时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  // 配置时钟
  TIM_TimeBaseInitTypeDef led_time;
  led_time.TIM_ClockDivision  = 0;
  led_time.TIM_CounterMode    = TIM_CounterMode_Up;
  led_time.TIM_Prescaler      = (SystemCoreClock/4/10000)-1; // 10 kHz; 定时不准确!
  led_time.TIM_Period         = 10000/2;  //  2 Hz;
  TIM_TimeBaseInit(LED1_USE_TIM, &led_time);
  
  // 启动自动重载; 设置中断;
  TIM_ARRPreloadConfig(LED1_USE_TIM, ENABLE);
  TIM_ITConfig(LED1_USE_TIM, TIM_IT_Update, ENABLE);
  
  // 启动中断
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel                    = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 5;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 5;
  NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  // 启动定时器
  TIM_Cmd(LED1_USE_TIM, ENABLE);
}


/* 
 * 设置 led2 为按键控制, 按下 key2 切换
 */
void init_led2_key_ctrl() {
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  
  // 设置 C1(key2) 为按键
  GPIO_InitTypeDef key_gpio;
  key_gpio.GPIO_Pin    = GPIO_Pin_1;
  key_gpio.GPIO_Mode   = GPIO_Mode_IN;
  key_gpio.GPIO_OType  = GPIO_OType_PP;
  key_gpio.GPIO_Speed  = GPIO_Speed_2MHz;
  key_gpio.GPIO_PuPd   = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &key_gpio);
  GPIO_SetBits(GPIOC, GPIO_Pin_1);
  
  // 设置外部事件源
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource1);
  
  // EXTI 设置外部中断外设
  EXTI_InitTypeDef exdef;
  exdef.EXTI_Line     = EXTI_Line1;
  exdef.EXTI_Mode     = EXTI_Mode_Interrupt;
  exdef.EXTI_Trigger  = EXTI_Trigger_Rising_Falling;
  exdef.EXTI_LineCmd  = ENABLE;
  EXTI_Init(&exdef);
  
  // 启动中断
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel                    = EXTI1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority  = 4;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority         = 5;
  NVIC_InitStructure.NVIC_IRQChannelCmd                 = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  // D3 配置为 led2 控制引脚
  GPIO_InitTypeDef led2_gpio;
  led2_gpio.GPIO_Pin    = GPIO_Pin_3;
  led2_gpio.GPIO_Mode   = GPIO_Mode_OUT;
  led2_gpio.GPIO_OType  = GPIO_OType_PP;
  led2_gpio.GPIO_Speed  = GPIO_Speed_2MHz;
  led2_gpio.GPIO_PuPd   = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &led2_gpio);
}


void TIM2_IRQHandler() {
  if (SET == TIM_GetFlagStatus(LED1_USE_TIM, TIM_FLAG_Update)) {
    if (isset1) {
      GPIO_SetBits(LED1_GPIO_GRP, LED1_GPIO_PIN);
      isset1 = 0;
    } else {
      GPIO_ResetBits(LED1_GPIO_GRP, LED1_GPIO_PIN);
      isset1 = 1;
    }
    TIM_ClearFlag(LED1_USE_TIM, TIM_FLAG_Update);
  }
}

void EXTI1_IRQHandler() {
  if (isset2) {
    GPIO_SetBits(GPIOD, GPIO_Pin_3);
    isset2 = 0;
  } else {
    GPIO_ResetBits(GPIOD, GPIO_Pin_3);
    isset2 = 1;
  }
}
