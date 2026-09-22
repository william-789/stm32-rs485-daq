uint32_t adc_value = 0;
uint32_t pwm_value = 0;

float voltage = 0.0f;

/* Start PWM on TIM3 Channel 1 */
HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

/* Start TIM2 interrupt */
HAL_TIM_Base_Start_IT(&htim2);

/* Start with LED off */
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);


while (1)
{
    /* Start ADC conversion */
    HAL_ADC_Start(&hadc1);

    /* Wait for conversion to finish */
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        /* Read ADC value (0 to 4095) */
        adc_value = HAL_ADC_GetValue(&hadc1);

        /* Convert ADC value to voltage */
        voltage = (3.3f * adc_value) / 4095.0f;

        /* Map ADC reading to PWM duty cycle */
        pwm_value = adc_value * 1000U / 4095U;

        /* Update PWM duty cycle */
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm_value);
    }

    HAL_ADC_Stop(&hadc1);

    HAL_Delay(10);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}
