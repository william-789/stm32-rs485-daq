
#include "stm32f4xx.h"
#include <stdint.h>

volatile uint32_t adc_value = 0;
volatile uint32_t pwm_value = 0;
volatile float voltage = 0.0f;

/* ---------- GPIO configuration ---------- */

void GPIO_Init(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* PA0: analog mode (ADC input) */
    GPIOA->MODER |= (3U << (0U * 2U));

    /* PA5: general-purpose output (blinking LED) */
    GPIOA->MODER &= ~(3U << (5U * 2U));
    GPIOA->MODER |=  (1U << (5U * 2U));

    /* PA6: alternate function mode (TIM3_CH1) */
    GPIOA->MODER &= ~(3U << (6U * 2U));
    GPIOA->MODER |=  (2U << (6U * 2U));

    /* PA6 alternate function AF2 = TIM3 */
    GPIOA->AFR[0] &= ~(0xFU << (6U * 4U));
    GPIOA->AFR[0] |=  (2U   << (6U * 4U));
}

/* ---------- ADC configuration ---------- */

void ADC_Init(void)
{
    /* Enable ADC1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* ADC prescaler: PCLK2 / 2 */
    ADC->CCR &= ~(3U << 16U);

    /* Right alignment, 12-bit resolution */
    ADC1->CR1 &= ~(3U << 24U);
    ADC1->CR2 &= ~(1U << 11U);

    /* Single conversion, channel 0 */
    ADC1->SQR1 = 0U;
    ADC1->SQR3 = 0U;

    /* Channel 0 sampling time: 84 cycles */
    ADC1->SMPR2 &= ~(7U << 0U);
    ADC1->SMPR2 |=  (4U << 0U);

    /* Enable ADC */
    ADC1->CR2 |= ADC_CR2_ADON;
}

/* ---------- Read ADC ---------- */

uint32_t ADC_Read(void)
{
    /* Start software conversion */
    ADC1->CR2 |= ADC_CR2_SWSTART;

    /* Wait until conversion is complete */
    while (!(ADC1->SR & ADC_SR_EOC))
    {
    }

    /* Return 12-bit result */
    return ADC1->DR;
}

/* ---------- TIM3 PWM configuration ---------- */

void PWM_Init(void)
{
    /* Enable TIM3 clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* Timer clock = 16 MHz
       PSC = 15 -> counter clock = 1 MHz
       ARR = 999 -> PWM frequency = 1 kHz */
    TIM3->PSC = 15U;
    TIM3->ARR = 999U;

    /* PWM mode 1 on channel 1 */
    TIM3->CCMR1 &= ~(7U << 4U);
    TIM3->CCMR1 |=  (6U << 4U);

    /* Enable preload for CCR1 */
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;

    /* Enable channel 1 output */
    TIM3->CCER |= TIM_CCER_CC1E;

    /* Start with LED off */
    TIM3->CCR1 = 0U;

    /* Enable auto-reload preload */
    TIM3->CR1 |= TIM_CR1_ARPE;

    /* Load timer registers */
    TIM3->EGR = TIM_EGR_UG;

    /* Start timer */
    TIM3->CR1 |= TIM_CR1_CEN;
}

/* ---------- TIM2 1 Hz square wave ---------- */

void TIM2_Init(void)
{
    /* Enable TIM2 clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* Timer clock = 16 MHz
       PSC = 15999 -> counter clock = 1 kHz
       ARR = 499 -> interrupt every 500 ms */
    TIM2->PSC = 15999U;
    TIM2->ARR = 499U;

    /* Generate update event to load prescaler */
    TIM2->EGR = TIM_EGR_UG;

    /* Clear pending update flag */
    TIM2->SR &= ~TIM_SR_UIF;

    /* Enable update interrupt */
    TIM2->DIER |= TIM_DIER_UIE;

    /* Enable TIM2 interrupt in NVIC */
    NVIC_EnableIRQ(TIM2_IRQn);

    /* Start timer */
    TIM2->CR1 |= TIM_CR1_CEN;
}

/* ---------- TIM2 interrupt handler ---------- */

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        /* Clear interrupt flag */
        TIM2->SR &= ~TIM_SR_UIF;

        /* Toggle PA5 */
        GPIOA->ODR ^= (1U << 5U);
    }
}

/* ---------- Main ---------- */

int main(void)
{
    GPIO_Init();
    ADC_Init();
    PWM_Init();
    TIM2_Init();

    while (1)
    {
        /* Acquire voltage */
        adc_value = ADC_Read();

        /* Convert ADC reading to volts */
        voltage = (3.3f * (float)adc_value) / 4095.0f;

        /* Convert ADC value to PWM compare value */
        pwm_value = (adc_value * 1000U) / 4095U;

        /* Update PWM duty cycle */
        TIM3->CCR1 = pwm_value;
    }
}
