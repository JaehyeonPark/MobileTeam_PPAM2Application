#define CS_MCP3208 RPI_GPIO_P1_22

#define SINGLE_INPUT 1
#define DIFFERENCE_INPUT 0

void bcm2835_spi_init (void);
int read_mcp3208_adc (int, int);
