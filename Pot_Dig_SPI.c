/*
	 * main implementation: use this 'C' sample to create your own application
	 *
	 */
	#include "derivative.h"
	
	void SPI_init(){
		SIM_SCGC6 |= (1<<12); /* Enable SPI0 clock */
		SIM_SCGC5 |= (1<<12)+ (1<<11); /* Enable PORTD + PORTC clock */
		PORTD_PCR1 |= 0x200; /* SCK */
		PORTD_PCR2 |= 0x200; /* SIN (MOSI) */
		PORTD_PCR3 |= 0x200; /* SOUT (MISO)*/
		PORTD_PCR0 |= 0X200; /* SPI0 PCS0 */
		NVICISER0 = (1<<26); /* Enable SPI0 Interrupts */
	}
	
	void SPI_config(){
	
		SPI0_MCR = 0x803F0000; /* Enable Master Mode and CS active on low */
		/* CTAR Register 
		 * 
		 * Frame Size = 16 bits (FMSZ + 1), CPOL = CPHA = 1, ???
		 * TCSC = 0.14 us (min 40 ns), TASC = 0.14 us (min 40 ns),
		 * TDT - 0.14 us (min 40 ns), SCK Frequency = 5.24 MHz (10 Mhz max),
		 * Baudrate = 2621440 */
	
		//SPI0_CTAR1 |= 0x7E000001;
		SPI0_CTAR1 |= 0x7E000001;
		SPI0_CTAR0 |= 0x7E000001;
		SPI0_CTAR0_SLAVE |= 0x17E000001;
	
		//SPI0_RSER |= 0x80000000; /*Enable transmission complete xflag */
	
	}
	
	void SPI_transfer(unsigned char dato){
	
		/* SPI0_PUSHR 
		 * 
		 * Keep PCS asserted between transfers, using CTAR0, SPI data is last data to transfer (bit 27),
		 * Keep PCS asserted, 
		 */
		//0001 0001 0001 1010 1010
		SPI0_PUSHR = (0x11100) + dato;
		//SPI0_PUSHR |= 8011000;
	
	
	}
	
	
	int main(void)
	{
		SPI_init();
		SPI_config();
		SPI_transfer(0x80);
	
		return 0;
		
		while(1){
			
		}
	}
