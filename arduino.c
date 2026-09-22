#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>


/*
 * ATmega328P
 *
 * ADC:
 *     ADC0 / A0
 *
 * PWM:
 *     OC1A / D9
 *
 * 1 Hz square wave:
 *     D13 / PB5
 */


/* =========================================================
   ADC INITIALIZATION
   ========================================================= */

void ADC_init(void)
{
    /*
     * AVcc = ADC reference voltage
     *
     * REFS1:0 = 01
     *
     * ADC input = ADC0
     */
    ADMUX = (1 << REFS0);

    /*
     * Enable ADC
     *
     * ADC clock prescaler = 128
     *
     * 16 MHz / 128 = 125 kHz
     *
     * This is within the recommended ADC clock range.
     */
    ADCSRA =
        (1 << ADEN)  |
        (1 << ADPS2) |
        (1 << ADPS1) |
        (1 << ADPS0);
}


/* =========================================================
   ADC READ
   ========================================================= */

uint16_t ADC_read(void)
{
    /*
     * Start conversion
     */
    ADCSRA |= (1 << ADSC);

    /*
     * Wait until conversion finishes
     *
     * ADSC becomes 0 when conversion is complete.
     */
    while (ADCSRA & (1 << ADSC))
    {
    }

    /*
     * ADC is a 10-bit value:
     *
     * 0    -> 0 V
     * 1023 -> 5 V
     */
    return ADC;
}


/* =========================================================
   PWM INITIALIZATION
   ========================================================= */

void PWM_init(void)
{
    /*
     * D9 = PB1 = OC1A
     *
     * Configure PB1 as output.
     */
    DDRB |= (1 << PB1);


    /*
     * Timer1 Fast PWM
     *
     * Mode 14:
     *
     * WGM13:0 = 1110
     *
     * TOP = ICR1
     *
     * Non-inverting PWM on OC1A
     */

    TCCR1A =
        (1 << COM1A1) |
        (1 << WGM11);

    TCCR1B =
        (1 << WGM13) |
        (1 << WGM12) |
        (1 << CS11);


    /*
     * CPU clock = 16 MHz
     *
     * Prescaler = 8
     *
     * Timer clock = 2 MHz
     *
     * TOP = 1999
     *
     * PWM frequency:
     *
     * 2 MHz / (1999 + 1)
     * = 1000 Hz
     */
    ICR1 = 1999;


    /*
     * Start with 0% duty cycle.
     */
    OCR1A = 0;
}


/* =========================================================
   PWM DUTY CYCLE
   ========================================================= */

void PWM_set(uint16_t adc_value)
{
    /*
     * ADC:
     *
     * 0    -> 0%
     * 1023 -> 100%
     *
     * Timer TOP = 1999
     *
     * Therefore:
     *
     * OCR1A = ADC * 1999 / 1023
     */

    OCR1A = ((uint32_t)adc_value * 1999) / 1023;
}


/* =========================================================
   1 Hz TIMER
   ========================================================= */

void Timer2_init(void)
{
    /*
     * D13 = PB5
     *
     * Configure as output.
     */
    DDRB |= (1 << PB5);


    /*
     * Timer2 in CTC mode.
     *
     * Interrupt every 500 ms.
     */

    TCCR2A =
        (1 << WGM21);


    /*
     * Prescaler = 1024
     */
    TCCR2B =
        (1 << CS22) |
        (1 << CS21) |
        (1 << CS20);


    /*
     * Timer2 clock:
     *
     * 16 MHz / 1024
     * = 15625 Hz
     *
     * We want 500 ms:
     *
     * 15625 * 0.5 = 7812.5
     *
     * Timer2 is only 8-bit, so we cannot
     * directly count to 7812.
     *
     * Instead we generate a shorter interrupt
     * and count interrupts in software.
     */

    OCR2A = 155;


    /*
     * Enable Output Compare A interrupt.
     */
    TIMSK2 =
        (1 << OCIE2A);
}


/* =========================================================
   TIMER2 INTERRUPT
   ========================================================= */

ISR(TIMER2_COMPA_vect)
{
    static uint16_t counter = 0;

    counter++;

    /*
     * Timer frequency:
     *
     * 15625 / (155 + 1)
     * = approximately 100 Hz
     *
     * Therefore 50 interrupts = 500 ms.
     */

    if (counter >= 50)
    {
        /*
         * Toggle D13.
         *
         * PINB is used to toggle an AVR GPIO.
         */
        PINB = (1 << PB5);

        counter = 0;
    }
}


/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    uint16_t adc_value;


    /*
     * Initialize peripherals.
     */
    ADC_init();

    PWM_init();

    Timer2_init();


    /*
     * Enable global interrupts.
     */
    sei();


    /*
     * Main loop
     */
    while (1)
    {
        /*
         * Read potentiometer.
         */
        adc_value = ADC_read();


        /*
         * Use ADC value to control LED brightness.
         */
        PWM_set(adc_value);
    }


    return 0;
}
