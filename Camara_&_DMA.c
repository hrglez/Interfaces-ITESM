/*
 * Interacción con cámara serial PTC08
 * Los datos se guardan en la memoria por DMA
 * Los datos se envían a PC por UART por DMA
 */

#include "derivative.h" /* include peripheral declarations */

#define takePhoto() UART_send(capture, captureSz); \
		while(ready==0);
#define camReset() UART_send(reset, resetSz); \
		while(ready==0);

#define UART3_DMA() UART3_C5 |= (1<<5)
#define UART3_Interrupt() UART3_C5 &= ~(1<<5)
#define ch0Start() DMAMUX_CHCFG0 |= (1<<7)
#define ch0Stop() DMAMUX_CHCFG0 &= ~(1<<7)

#define ch1Start() DMAMUX_CHCFG1 |= (1<<7)
#define ch1Stop() DMAMUX_CHCFG1 &= ~(1<<7)
#define ch1Req() DMA_TCD1_CSR |= 1
#define sendPC() UART1_C2 = 0x48

#define memoryStart 0x20002800
#define UART3data 0x4006D007
#define UART1data 0x4006B007

unsigned char reset[] = {0x56, 0, 0x26, 0};
unsigned char resetSz = 4;
unsigned char capture[] = {0x56, 0, 0x36, 1, 0};
unsigned char captureSz = 5;
unsigned char length[] = {0x56, 0, 0x34, 1, 0};
unsigned char lengthSz = 5;
unsigned char read[] =  {0x56, 0x00, 0x32, 0x0C, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xAA, 0x00, 0xFF};
unsigned char readSz = 16;

unsigned char msg[16], rec[16];
unsigned char sz=0;
unsigned char idx=0, idy=0;
unsigned char ready=0, leerL=0;
unsigned int fotoSz = 0, idz=0;

void UART_send (unsigned char dato[], unsigned char sz);
void UART_CAM_init(void);
void UART_PC_init(void);
void pin_config(void);
void DMA_rec_init(void);
void DMA_send_init(void);
void getLength(void);
void getData(void);

void DMA0_IRQHandler(void)
{
	DMA_CINT = 0;
}

void UART1_Status_IRQHandler(void)
{
	if( (UART1_S1 & (1<<6) ) )
	{
		UART1_C2 &= ~(1<<3); //Apagar transmisión
		UART1_C2 |= (1<<3);
		
		if( idz<fotoSz )
		{
			ch1Req();
			idz++;
		}
		else
		{
			UART1_C2 = 0;
			idz=0;
		}
	}

}

void UART3_Status_IRQHandler(void)
{
	if( (UART3_S1 & (1<<5)) ) //Recepcion
	{
		rec[0] = UART3_D;
		if(leerL)
		{
			rec[idy] = UART3_D;
			idy++;
			if(idy==9) leerL=0;
		}

	}
	if( (UART3_S1 & (1<<6)) ) //Transmision
	{
		if(idx<sz)
		{
			UART3_D = msg[idx];
			idx++;
		}
		else if (idx==sz)
		{
			for(idx=0;idx<sz;idx++)
			{
				msg[idx] = 0;
			}
			UART3_C2 &= ~(1<<3); //Apagar bandera mediante apagado y encendido de transmision
			UART3_C2 |= (1<<3);
			ready=1;
		}
	}
}

void UART_send (unsigned char dato[], unsigned char largo)
{
	sz=largo;
	ready=0;
	for(idx=0;idx<sz;idx++)
	{
		msg[idx] = dato[idx];
	}
	idx=1;
	UART3_D = msg[0];
}

void UART_CAM_init(void)
{
	SIM_SCGC4 |= (1<<13); //activar clock de UART3
	NVICISER1 = (1<<5); //Interrupciones. IRQ=37
	UART3_BDH = 0; //baudrate, es encesario escribir en BDH andtes que BDL
	UART3_BDL = 34; //Baud rate: reloj/(16*BDL) -> 21Mhz por default

	UART3_C2 = 0x6C; //Activar transmision y recepcion, y la interrupcion de recepcion
}

void UART_PC_init(void)
{
	SIM_SCGC4 |= (1<<11); //activar clock de UART1
	NVICISER1 = (1<<1); //Interrupciones. IRQ=33
	UART1_BDH = 0; //baudrate, es encesario escribir en BDH andtes que BDL
	UART1_BDL = 34; //Baud rate: 38400

	//UART1_C2 = (1<<6); //Activar interrupcion/DMA request transmission complete
	//UART1_C5 = (1<<6); //Habilitar DMA request
}

void pin_config(void)
{
	SIM_SCGC5 |= (1<<11); //Reloj del puerto C
	PORTC_PCR16 = (3<<8); //alternativa 3 de PORTC16, UART3 RX
	PORTC_PCR17 = (3<<8); //alternativa 3 de PORTC17, UART3 TX

	PORTC_PCR4 = (3<<8); //alternativa 3 de PORTC4, UART1 TX
}

void DMA_rec_init(void)
{
	SIM_SCGC6 |= (1<<1); //DMAMUX clock
	SIM_SCGC7 |= (1<<1); //DMA clock
	NVICISER0 |= (1<<16); //Interrupciones de DMA

	DMAMUX_CHCFG0 = 8; //Enlazar canal 0 con UART3 receiver como source

	DMA_TCD0_CITER_ELINKNO = 1; //Sin channel link, major loop = 1, single request
	DMA_TCD0_BITER_ELINKNO = 1; //Debe ser igual a CITER
	DMA_TCD0_NBYTES_MLNO = 1; //Minor byte count, bytes transferidos por request

	DMA_TCD0_SADDR = UART3data;	//Source address de UART3_D
	DMA_TCD0_SOFF = 0;			//Source address offset
	//DMA_TCD0_ATTR &= ~(7<<8); //Source data transfer size, en cero es 8 bits
	DMA_TCD0_ATTR = 0;	//Establecer los demas bits en cero
	DMA_TCD0_SLAST = 0;	//Valor añadido a source address después de un major loop, con segundo complemento

	DMA_TCD0_DADDR = memoryStart; //Destination address, memoria ram, empieza en 0x20000000, mide 192Kb, grupos de 4 bytes, comienza en [...]2800
	DMA_TCD0_DOFF = 1;	//Destination address offset
	DMA_TCD0_ATTR |= 0;	//Destination data transfer size, en cero es 8 bits
	DMA_TCD0_DLASTSGA = 0;	//Valor añadido a destination address después de un major iteration count, segundo complemento

	DMA_TCD0_CSR = 2; //Habilitar interrupción después de terminar un major iteration

	DMA_SERQ = 0; //Habilitar DMA request canal 0
}

void DMA_send_init(void)
{
	DMAMUX_CHCFG1 = 5; //Enlazar canal 1 con UART1 transmitter como source

	DMA_TCD1_CITER_ELINKNO = 1; //Sin channel link, major loop = 1, single request
	DMA_TCD1_BITER_ELINKNO = 1; //Debe ser igual a CITER
	DMA_TCD1_NBYTES_MLNO = 1; //Minor byte count, bytes transferidos por request

	DMA_TCD1_SADDR = (memoryStart+5);	//Source address de memoria
	DMA_TCD1_SOFF = 1;			//Source address offset
	//DMA_TCD1_ATTR &= ~(7<<8); //Source data transfer size, en cero es 8 bits
	DMA_TCD1_ATTR = 0;	//Establecer los demas bits en cero
	DMA_TCD1_SLAST = 0;	//Valor añadido a source address después de un major loop, con segundo complemento

	DMA_TCD1_DADDR = UART1data; //Destination address
	DMA_TCD1_DOFF = 0;	//Destination address offset
	DMA_TCD1_ATTR |= 0;	//Destination data transfer size, en cero es 8 bits
	DMA_TCD1_DLASTSGA = 0;	//Valor añadido a destination address después de un major iteration count, segundo complemento

	//DMA_TCD1_CSR = 2; //Habilitar interrupción después de terminar un major iteration

	DMA_SERQ = 1; //Habilitar DMA request canal 0

	ch1Start();
}

int main(void)
{
	pin_config();
	UART_CAM_init();
	UART_PC_init();
	DMA_rec_init();
	DMA_send_init();
	camReset();
	takePhoto();
	getLength();
	getData();
	sendPC();

	return 0;
}

void getLength(void)
{
	UART_send(length, lengthSz);
	leerL=1;
	idy=0;
	while(ready==0);
	while(leerL);
	read[12]=rec[7];
	read[13]=rec[8];
	fotoSz = ( (rec[7]<<8)+ rec[8] );
}

void getData(void)
{
	UART3_DMA();
	ch0Start();
	UART_send(read, readSz);
	while(ready==0);
}
