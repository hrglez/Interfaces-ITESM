/*
 * Control de GSM por UART por hardware
 */

#include "derivative.h" /* include peripheral declarations */

#define reset "AT+ATZ\n\r"
#define texto "AT+CMGF=1\n\r"
#define sms "AT+CMGS=\""
#define marcar "ATD"
#define contestar "ATA\n\r"
#define colgar "ATH\n\r"
#define Aranza "+15628267620"
#define Miguel "+5213334677013"
#define CtrlZ 0x1A
#define leerSMS "AT+CMGR="

unsigned char msg[100];
unsigned char rec[100];
unsigned char sz=0;
unsigned char idx=0, idy=0;
unsigned char ready=0;
unsigned int leer;
unsigned char parceo=0;
unsigned char parcEsp=0;

void UART_send (char dato[]);

void delay_ms (unsigned int ms) //400 ms maximo
{
	FTM0_MOD = FTM0_CNT + (unsigned int)(ms*164);
	FTM0_SC &= ~(1<<7);//Cerciorar que la bandera TOF esté apagada
	while( (FTM0_SC & (1<<7))==0 ); //delay por polling, revisar TOF
	FTM0_SC &= ~(1<<7); //Apagar bandera
	FTM0_MOD=0; //regresar a estado original con desborde en 0xFFFF
}


void FTM0_IRQHandler(void)
{
	if( (FTM0_C1SC & (1<<7)) ) //input capture
	{
		FTM0_C1SC &= ~(1<<7);//Apagar bandera canal 1
		UART_send(contestar);
	}
}

void UART3_Status_IRQHandler(void)
{
	unsigned char var;
	if( (UART3_S1 & (1<<5)) ) //Recepcion
	{
		var = UART3_D;

		if (parcEsp==2)
		{
			rec[idy] = var;
			idy++;
		}
		else if( var == 34 )
		{
			parceo++;
		}
		else if(parceo==8)
		{
			parcEsp++;
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
			UART3_C2 &= ~(1<<3);
			UART3_C2 |= (1<<3);
			ready=1;
		}
	}
}


void pin_config(void)
{
	SIM_SCGC5 |= (1<<11); //Reloj del puerto C
	PORTC_PCR2 = (1<<10); //pin PTC2 alternativa 4, FTM0 CH1
	PORTC_PCR16 = (3<<8); //alternativa 3 de PORTC16, UART3 RX
	PORTC_PCR17 = (3<<8); //alternativa 3 de PORTC17, UART3 TX
}

void TMR_config(void)
{
	SIM_SCGC6 |= (1<<24); //Habilitar reloj de FTM0
	NVICISER1 = (1<<10); //Interrupciones, IRQ=42
	FTM0_SC = 7; //no encender timer, preescaler 128

	FTM0_C1SC = 0x48; //Input capture on falling, interrupciones de canal

	FTM0_SC |= (1<<3); //Encender timer
}

void UART_init(void)
{
	SIM_SCGC4 |= (1<<13); //activar clock de UART3
	NVICISER1 = (1<<5); //Interrupciones. IRQ=37
	UART3_BDH = 0; //baudrate, es encesario escribir en BDH andtes que BDL
	UART3_BDL = 0x88; //Baud rate: reloj/(16*BDL)

	UART3_C2 = 0x6C; //Activar transmision y recepcion, y la interrupcion de recepcion
}

/*void AUX(void)
{
	SIM_SCGC4 |= (1<<11); //activar clock de UART1
	UART1_BDH = 0; //baudrate, es encesario escribir en BDH andtes que BDL
	UART1_BDL = 0x88; //Baud rate: reloj/(16*BDL)

	UART1_C2 = (1<<3); //Activar transmision

	PORTC_PCR4 = (3<<8); //alternativa 3 de PORTC4, UART1 TX
}*/

void UART_send (char dato[])
{
	sz = strlen(dato);
	ready=0;
	for(idx=0;idx<sz;idx++)
	{
		msg[idx] = dato[idx];
	}
	idx=1;
	UART3_D = msg[0];
}

void GSM_init (void)
{
	UART_send(reset);
	while(ready==0);
	delay_ms(100);
	UART_send(texto);
	while(ready==0);
	delay_ms(100);
}

void GSM_llamar (void)
{	
	UART_send(marcar);
	while(ready==0);
	UART_send(Miguel);
	while(ready==0);
	UART_send(";\n\r");
	while(ready==0);
}

void GSM_enviar_SMS (char enviar[])
{
	UART_send(sms);
	while(ready==0);
	UART_send(Miguel);
	while(ready==0);
	UART_send("\"\n\r");
	while(ready==0);
	UART_send(enviar);
	while(ready==0);
	delay_ms(100);
	UART3_D = CtrlZ;
	while(ready==0);
}

void GSM_leer_SMS (char num[])
{
	idy=0;
	parceo=0;
	parcEsp=0;
	UART_send(leerSMS);
	while(ready==0);
	UART_send(num);
	while(ready==0);
	UART_send("\n\r");
	while(ready==0);
}

int main(void)
{
	pin_config();
	TMR_config();
	UART_init();
	//GSM_init();
	//GSM_llamar();
	//GSM_enviar_SMS("We're jamming");
	//GSM_leer_SMS("7");
		
	return 0;
}
