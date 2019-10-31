/*
 * main implementation: use this 'C' sample to create your own application
 *
 */

#include "derivative.h" /* include peripheral declarations */

long i;

#define NOP()                              \
		for(i=100;i!=0;i--)               \

char lectura;
int transmision_cont, cont, cont_esclavo=0,cont_error;
char role[] = {"AT+ROLE=1"};
int role_sz = 9;
char mode[] = {"AT+CMODE=0"};
int mode_sz = 10;
char bind[] = {"AT+BIND=0000,00,000000"};
int bind_sz = 22;
char link1[] = {"AT+LINK=98D3,33,811615"};
char link2[] = {"AT+LINK=98D3,32,70CF48"};
char disc[] = {"AT+DISC"};
int disc_sz = 7;
char mensaje1[50] = {"HOLA"};
char mensaje2[50] = {"HOLA"};
int mensaje_sz=50;

void delay_ms (unsigned char ms)
{
	LPTMR0_CMR = ms-1; //El timer cuenta hasta -1
	LPTMR0_CSR = 1;          //Encender timer
	do{
	} while ((LPTMR0_CSR&(1<<7))==0);
	LPTMR0_CSR = (1<<7); //La bandera se apaga al escribirle un 1, al mismo tiempo se apaga el timer
}

void UART_TRANSMISION_CONFIG(void)
{
	UART3_C2 = 0b01001000; //activar interrupciones de transmision y habilitar modo transmision
}

void UART_RECEPTION_CONFIG(void)
{
	UART3_C2 = 0b00100100; //activar interrupciones de recepcion y habilitar modo recepcion
}

void Send_Message(char message[], int size)
{
	if(cont < size)
	{
		UART3_D = message[cont];
		cont++;
	}
	else if(cont == size)
	{
		UART3_D = 0xD;
		cont++;
	}else if (cont == size+1)
	{
		UART3_D = 0xA;
		cont++;
	}else if (cont > size+1)
	{
		cont = 0;
		cont_error=0;
		UART_RECEPTION_CONFIG();
	}
}

void Check_Ok(void)
{
	if(cont==0 && lectura=='O')
	{
		cont++;
	}
	if(cont==1 && lectura=='K')
	{
		cont++;
	}
	if(cont==2 && lectura==0xD)
	{
		cont++;
	}
	if(cont==3 && lectura==0xA)
	{
		cont=0;
		cont_error=0;
		transmision_cont++;
		UART_TRANSMISION_CONFIG();
	}
}

void Check_Error(void)
{
	if(cont_error==0 && lectura=='E')
	{
		cont_error++;
	}
	if(cont_error==1 && lectura=='R')
	{
		cont_error++;
	}
	if(cont_error==2 && lectura=='R')
	{
		cont_error++;
	}
	if(cont_error==3 && lectura=='O')
	{
		cont_error++;
	}
	if(cont_error==4 && lectura=='R')
	{
		cont_error++;
	}
	if(cont_error==5 && lectura==':')
	{
		cont_error++;
	}
	if(cont_error==6 && lectura=='(')
	{
		cont_error++;
	}
	if(cont_error==7 && lectura=='0')
	{
		cont_error++;
	}
	if(cont_error==8 && lectura==')')
	{
		cont_error++;
	}
	if(cont_error==9 && lectura==0xD)
	{
		cont_error++;
	}
	if(cont_error==10 && lectura==0xA)
	{
		cont=0;
		cont_error=0;
		delay_ms(10);
		UART_TRANSMISION_CONFIG();
	}
}

void Save_Message(char message[], int size)
{
	if(lectura==0xA || cont >= size)
	{
		cont=0;
		transmision_cont++;
		UART_TRANSMISION_CONFIG();
	}else if(cont < size)
	{
		message[cont] = lectura;
		cont++;
	}
}

void Clear_Message(char message[], int size)
{
	for(i = 0; i<size;i++)
	{
		message[i] = 0;
	}
}

void Check_Disc(void)
{
	if(cont==0 && lectura==':')
	{
		cont++;
	}
	if(cont==1 && lectura=='S')
	{
		cont++;
	}
	if(cont==2 && lectura=='U')
	{
		cont++;
	}
	if(cont==3 && lectura=='C')
	{
		cont++;
	}
	if(cont==4 && lectura=='C')
	{
		cont++;
	}
	if(cont==5 && lectura=='E')
	{
		cont++;
	}
	if(cont==6 && lectura=='S')
	{
		cont++;
	}
	if(cont==7 && lectura=='S')
	{
		cont++;
	}
	if(cont==8 && lectura==0xD)
	{
		cont++;
	}
	if(cont==9 && lectura==0xA)
	{
		cont=0;
		transmision_cont=3;
		cont_esclavo++;
		UART_TRANSMISION_CONFIG();
	}
}

void UART3_Status_IRQHandler()
{
	unsigned char dummy;
	if(UART3_S1 & 0x20) //checar bandera de recepcion
	{
		dummy = UART3_S1;
		lectura = UART3_D; //leer recepcion y apagar bandera
		if(transmision_cont <= 3)
		{
			Check_Ok();
			//Check_Error();
		}else if (transmision_cont==4)
		{
			if(cont_esclavo%2==0)
			{
				Save_Message(mensaje2,mensaje_sz);
				Clear_Message(mensaje1,mensaje_sz);
			}else
			{
				Save_Message(mensaje1,mensaje_sz);
				Clear_Message(mensaje2,mensaje_sz);
			}
		}else if (transmision_cont==5)
		{
			Check_Disc();
		}
	}else if(UART3_S1 & 0x40) //checar bandera de transmision
	{
		dummy = UART3_S1; //linea necesaria para apagar bandera de transmision
		if(transmision_cont==0)
		{
			Send_Message(mode,mode_sz);
		}else if(transmision_cont==1)
		{
			Send_Message(role,role_sz);
		}else if(transmision_cont==2)
		{
			Send_Message(bind,bind_sz);
		}else if(transmision_cont==3)
		{
			if(cont_esclavo%2==0)
			{
				Send_Message(link1,bind_sz);
			}else
			{
				Send_Message(link2,bind_sz);
			}
		}else if(transmision_cont==4)
		{
			GPIOC_PDOR = 0;
			if(cont_esclavo%2==0)
			{
				Send_Message(mensaje1,mensaje_sz);
			}else
			{
				Send_Message(mensaje2,mensaje_sz);
			}
		}else if(transmision_cont==5)
		{
			GPIOC_PDOR = 4;
			delay_ms(100);
			Send_Message(disc,disc_sz);
		}else
		{
			UART3_D = 0; //lienea necesaria para apagar bandera
			UART_RECEPTION_CONFIG();
		}
	}
}

void LPTM_init (void)
{
	SIM_SCGC5 |= 1;
	LPTMR0_PSR = 0b101;
}

void UART_INIT(void)
{
	SIM_SCGC4 |= (1<<13); //activar clock de UART3
	SIM_SCGC5 |= (1<<11); //activar clock de puerto C
	PORTC_PCR16 = (3<<8); //alternativa 3 de PORTC16
	PORTC_PCR17 = (3<<8); //alternativa 3 de PORTC17
	PORTC_PCR2 = (1<<8); //alternativa 3 de PORTC17
	GPIOC_PDDR = 4; //Poner PTC2 como salida
	GPIOC_PDOR = 0; //Asegurarse que la salida de PTC2 sea 0 logico
	UART3_BDH = 0; //condiguracion de baudrate
	UART3_BDL = 0x88; //configuracion de baudrate
	NVICISER1 = (1<<5); //activar interrupciones de UART. El shift se determina con el IRQ%32, el IRQ esta en una tabal del manual
}

void Bluetooth_Init(void)
{
	cont=0;
	transmision_cont=0;
	GPIOC_PDOR = 4;
	UART_TRANSMISION_CONFIG();
}

int main(void)
{
	LPTM_init();
	UART_INIT();
	//UART_TRANSMISION_CONFIG();
	//UART_RECEPTION_CONFIG();	
	//GPIOC_PDOR = 4;
	Bluetooth_Init();
	while(1);
	return 0;
}
