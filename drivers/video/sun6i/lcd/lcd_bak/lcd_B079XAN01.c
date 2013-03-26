
#include "lcd_B079XAN01.h"

void spi_24bit_3wire(__u32 tx)
{
	__u8 i;

	spi_csx_set(0);

	for(i=0;i<24;i++)
	{
		LCD_delay_us(1);
		spi_sck_set(0);
		LCD_delay_us(1);
		if(tx & 0x800000)
			spi_sdi_set(1);
		else
			spi_sdi_set(0);
		LCD_delay_us(1);
		spi_sck_set(1);
		LCD_delay_us(1);
		tx <<= 1;
	}
	spi_sdi_set(1);
	LCD_delay_us(1);
	spi_csx_set(1);
	LCD_delay_us(3);
}

void lp079x01_init(__panel_para_t * info)
{
	spi_24bit_3wire(0x7000B1);  //VSA=50, HAS=64
	spi_24bit_3wire(0x723240);

	spi_24bit_3wire(0x7000B2); //VBP=30+50, HBP=56+64
	spi_24bit_3wire(0x725078);

	spi_24bit_3wire(0x7000B3); //VFP=36, HFP=60
	spi_24bit_3wire(0x72243C);

	spi_24bit_3wire(0x7000B4); //HACT=768
	spi_24bit_3wire(0x720300);

	spi_24bit_3wire(0x7000B5); //VACT=1240
	spi_24bit_3wire(0x720400);

	spi_24bit_3wire(0x7000B6);
	spi_24bit_3wire(0x720009); //0x720009:burst mode, 18bpp packed
														 //0x72000A:burst mode, 18bpp loosely packed
							   						 //0x72000B:burst mode, 24bpp 
							   						 
	spi_24bit_3wire(0x7000DE); //no of lane=4
	spi_24bit_3wire(0x720003);

	spi_24bit_3wire(0x7000D6); //RGB order and packet number in blanking period
	spi_24bit_3wire(0x720005);

	spi_24bit_3wire(0x7000B9); //disable PLL
	spi_24bit_3wire(0x720000);

	spi_24bit_3wire(0x7000BA); //lane speed=560
	spi_24bit_3wire(0x72C013); //may modify according to requirement, 500Mbps to  560Mbps, (n+1)*lcd_dclk_freq
	
	spi_24bit_3wire(0x7000BB); //LP clock
	spi_24bit_3wire(0x720008);

	spi_24bit_3wire(0x7000B9); //enable PPL
	spi_24bit_3wire(0x720001);

	spi_24bit_3wire(0x7000c4); //enable BTA
	spi_24bit_3wire(0x720001);

	spi_24bit_3wire(0x7000B7); //enter LP mode
	spi_24bit_3wire(0x720342);

	spi_24bit_3wire(0x7000B8); //VC
	spi_24bit_3wire(0x720000);

	spi_24bit_3wire(0x7000BC); //set packet size
	spi_24bit_3wire(0x720000);

	spi_24bit_3wire(0x700011); //sleep out cmd

	LCD_delay_ms(200);
	spi_24bit_3wire(0x700029); //display on

	LCD_delay_ms(200);
	spi_24bit_3wire(0x7000B7); //video mode on
	spi_24bit_3wire(0x72030b);
}


