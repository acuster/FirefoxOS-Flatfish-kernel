#include "pm_types.h"
#include "pm.h"

//for io-measure time
#define PORT_E_CONFIG (SW_VA_PORTC_IO_BASE + 0x90)
#define PORT_E_DATA (SW_VA_PORTC_IO_BASE + 0xa0)
#define PORT_CONFIG PORT_E_CONFIG
#define PORT_DATA PORT_E_DATA

volatile int  print_flag = 0;

void busy_waiting(void)
{
#if 1
	volatile __u32 loop_flag = 1;
	while(1 == loop_flag);

#endif
	return;
}

void fake_busy_waiting(void)
{
#if 1
	volatile __u32 loop_flag = 2;
	while(1 == loop_flag);

#endif
	return;
}






/*
 * notice: dependant with perf counter to delay.
 */
void io_init(void)
{
	//config port output
	*(volatile unsigned int *)(PORT_CONFIG)  = 0x111111;

	return;
}

void io_init_high(void)
{
	__u32 data;

	//set port to high
	data = *(volatile unsigned int *)(PORT_DATA);
	data |= 0x3f;
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

void io_init_low(void)
{
	__u32 data;

	data = *(volatile unsigned int *)(PORT_DATA);
	//set port to low
	data &= 0xffffffc0;
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

/*
 * set pa port to high, num range is 0-7;
 */
void io_high(int num)
{
	__u32 data;
	data = *(volatile unsigned int *)(PORT_DATA);
	//pull low 10ms
	data &= (~(1<<num));
	*(volatile unsigned int *)(PORT_DATA) = data;
	delay_us(10000);
	//pull high
	data |= (1<<num);
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}
