//UART virtual

#include "derivative.h" /* include peripheral declarations */

unsigned char cont=0;
unsigned char dato;
unsigned char datoR=0;
unsigned char dataReady=0;
unsigned contr=0;

void FTM0_IRQHandler(void)
{
	if( (FTM0_C2SC & (1<<7)) )
	{
		FTM0_C2SC &= ~(1<<7);//Apagar bandera canal 2, output compare

		if(cont>1){
			if(dato & 1) FTM0_C2SC |= (1<<2);
			else FTM0_C2SC &= ~(1<<2);
			dato >>= 1;
			cont--;
		}
		else if(cont == 1){
			FTM0_C2SC |= (1<<2);
			cont--;
		}
		else{
			FTM0_C2SC &= ~(1<<6); //Apagar interrupciones
		}

		FTM0_C2V += 34; //Actualizar valor de referencia
	}

	if( (FTM0_C1SC & (1<<7)) )
	{
		FTM0_C1SC &= ~(1<<7);//Apagar bandera canal 1

		if( (FTM0_C1SC & (1<<4)) == 0 ) //Si es input capture
		{
			FTM0_C1SC = 0x50; //Habilitar interrupciones del canal y funcionamiento de output compare, sin utilizar el pin
			PORTC_PCR2 = (1<<8); //Habilitar el pin Rx como GPIO
			contr=10;
			FTM0_C1V += (34-1)+34/2;
		}
		if( (FTM0_C1SC & (1<<4)) ) //Si es output compare
		{
			if (contr>2){
				if( (GPIOC_PDIR&(1<<2)) ) datoR |= (1<<7);
				if(contr>3) datoR >>= 1;
				contr--;
				FTM0_C1V += 34;
			}
			else if (contr==2){
				if( (GPIOC_PDIR&(1<<2))==0 );//frame error
				else contr--;
				FTM0_C1V += 34;
			}
			else if (contr==1){
				dataReady=1;
				PORTC_PCR2 = (1<<10); //Pin como alternativa 4, CH1 FTM0, PTC2
				FTM0_C1SC = 0x48;//Configurar pin como input capture falling edge, habilitar interrupciones
			}
		}
	}
}

void vUART_init(void)
{
	SIM_SCGC5 |= (1<<11); //Reloj puerto C
	PORTC_PCR3 = (1<<8); //Inicializar pin 3 de puerto C como GPIO (general purpose I/O)
	GPIOC_PDDR = 8; //Configurar PTC3 (Tx) como salida
	SIM_SCGC6 |= (1<<24); //Habilitar reloj de FTM0
}

void vUART_config (void)
{
	GPIOC_PDOR = 8;//pin TXD (PTC3), valor inicial = 1
	FTM0_SC = 6; //timer sin reloj (no habilitado), dividir entre 64, 3.051 us
	FTM0_SC |= (1<<3); //Habilita timer para que comience a correr
	NVICISER1 = (1<<10); //Habilitar interrupciones de FTM0, IRQ=42
}

void  vUART_send(unsigned char envio)
{
	dato = envio;
	cont=(8+1);
	PORTC_PCR3 = (1<<10); //Pin como alternativa 4, CH2 FTM0, PTC3
	FTM0_C2SC &= ~(1<<2); //Preparar start bit	
	FTM0_C2SC |= 0x18;//Configurar pin como output compare (se cancela GPIO), encender ELS2B
	FTM0_C2V = FTM0_CNT +(34-1); //valor de referencia para OC (output compare), 34=104 us, se retrasa uno porque la bandera se activa al siguiente pulso
	FTM0_C2SC |= (1<<6); //Habilitar interrupciones de canal; canal 2; pin PTC3
}

void vUART_Rec_init (void)
{
	PORTC_PCR2 = (1<<10); //Pin como alternativa 4, CH1 FTM0, PTC2
	FTM0_C1SC = 0x48;//Configurar pin como input capture falling edge, hailitar interrupciones
}

int main (void)
{
	unsigned char msj[] = {"Hola Mundo\n\r"};//{'H','o','l','a','\n','\r'};
	unsigned char i;
	vUART_init();
	vUART_config();
	vUART_Rec_init();

	while(1){
		/*for(i=0;i<12;i++){
			vUART_send(msj[i]);
			while(FTM0_C2SC&(1<<6));
		}*/
		if(dataReady){
			dataReady=0;
			vUART_send(datoR);
			while(FTM0_C2SC&(1<<6));
		}

	}
	return 0;
}
