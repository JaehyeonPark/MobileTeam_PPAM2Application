#include <bcm2835.h>
#include <stdio.h>

#include "mcp3208_bcm2835.h"

int main (void)
{
    int adc_value;
    
    bcm2835_spi_init ();
    
    while(1){
        adc_value = read_mcp3208_adc(2, DIFFERENCE_INPUT);
        printf("adc : %4d\n",adc_value);
    }

    bcm2835_spi_end();
    bcm2835_close();

    return 0;
}

void bcm2835_spi_init (void)
{
    if(!bcm2835_init()) return -1;

    bcm2835_gpio_set (CS_MCP3208);
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder (BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setClockDivider (BCM2835_SPI_CLOCK_DIVIDER_65536);
    bcm2835_spi_setDataMode (BCM2835_SPI_MODE0);
//    bcm2835_spi_chipSelect (BCM2835_SPI_CS0);
}

int read_mcp3208_adc (int adcChannel, int mode)
{
    uint8_t buff[3] = { 0 };
    int adcValue = 0;
    
    buff[0] = (0x04 | ((adcChannel & 0x07) >> 7))+0x02*mode;
    buff[1] = ((adcChannel & 0x07) << 6);

//    bcm2835_spi_setChipSelectPolarity (BCM2835_SPI_CS0, LOW);
    bcm2835_gpio_clr (CS_MCP3208);
    bcm2835_spi_transfern (buff, 3);
//    bcm2835_spi_setChipSelectPolarity (BCM2835_SPI_CS0, HIGH);
    buff[1] = 0x0f & buff[1];
    adcValue =  (buff[1] << 8) | buff[2];
    bcm2835_gpio_set (CS_MCP3208);
    
    return adcValue;
}

/*
function list (bcm2835 general)
int   bcm2835_init (void)
int   bcm2835_close (void)

function list (spi)
void   bcm2835_spi_begin (void)
void   bcm2835_spi_end (void)
void   bcm2835_spi_setBitOrder (uint8_t order)
void   bcm2835_spi_setClockDivider (uint16_t divider)
void   bcm2835_spi_setDataMode (uint8_t mode)
void   bcm2835_spi_chipSelect (uint8_t cs)
void   bcm2835_spi_setChipSelectPolarity (uint8_t cs, uint8_t active)
uint8_t    bcm2835_spi_transfer (uint8_t value)
void   bcm2835_spi_transfernb (char *tbuf, char *rbuf, uint32_t len)
void   bcm2835_spi_transfern (char *buf, uint32_t len)
void   bcm2835_spi_writenb (char *buf, uint32_t len)
*/
