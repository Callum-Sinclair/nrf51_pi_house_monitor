#include "nrf51.h"
#include "system_nrf51.h"
#include "nrf51_bitfields.h"

#define RED_LED     16
#define GREEN_LED   12
#define BLUE_LED    15

void gpio_pin_init(uint8_t pin)
{
    NRF_GPIO->PIN_CNF[pin] = GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos;
    NRF_GPIO->DIRSET = 1UL << pin;
}

void set_pin(uint8_t pin)
{
    NRF_GPIO->OUTSET = 1UL << pin;
}

void clear_pin(uint8_t pin)
{
    NRF_GPIO->OUTCLR = 1UL << pin;
}

volatile void do_not_use_this_delay(uint32_t num)
{
    volatile uint16_t busy = 1;
    volatile uint32_t i;
    
    for (i = 0; i < num; i++)
    {
        while (busy)
        {
            busy++;
        }
    }
}

int main(void)
{
    uint16_t i = 0;
    gpio_pin_init(RED_LED);
    gpio_pin_init(GREEN_LED);
    gpio_pin_init(BLUE_LED);
    for (i = 0; i < 100; i++)
    {
        clear_pin(RED_LED);
        do_not_use_this_delay(100);
        clear_pin(GREEN_LED);
        do_not_use_this_delay(100);
        clear_pin(BLUE_LED);
        do_not_use_this_delay(100);
        set_pin(RED_LED);
        do_not_use_this_delay(100);
        set_pin(BLUE_LED);
        do_not_use_this_delay(100);
        set_pin(GREEN_LED);
        do_not_use_this_delay(100);

    }
    NRF_POWER->SYSTEMOFF = POWER_SYSTEMOFF_SYSTEMOFF_Enter << POWER_SYSTEMOFF_SYSTEMOFF_Pos;
}
