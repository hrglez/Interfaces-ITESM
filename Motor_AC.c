//Código para controlar motor AC a través de optoacoplador
//ADC- ADC18 ; Entrada de opto- PTC2 ; Salida a opto - PTC3

#include "derivative.h" /* include peripheral declarations */
unsigned char on=0;
unsigned int riseT;

void FTM0_IRQHandler(void)
{
	if( (FTM0_C2SC & (1<<7)) ) //output compare
	{
		FTM0_C2SC &= ~(1<<7); //Apagar bandera

		if(on)
		{
			if(ADC1_RA>0x7FFF)
			{
				FTM0_SC &= ~(1<<3);
				FTM0_C2V += 1400;			
				FTM0_SC |= (1<<3);
			}
			else
			{
				FTM0_SC &= ~(1<<3);
				FTM0_C2V += 300;			
				FTM0_SC |= (1<<3);
			}

			FTM0_C2SC &= ~(1<<2); //Preparar apagado
			on=0;
		}
		else
		{
			FTM0_C2SC &= ~(1<<6); //Deshabilitar interrupciones CH2
			FTM0_C1SC |= (1<<6);
			FTM0_C1SC = 0x44;
		}
	}

	if( (FTM0_C1SC & (1<<7)) ) //input capture
	{
		if(FTM0_C1SC & (1<<2))
		{
			FTM0_C1SC &= ~(1<<6);
			FTM0_C1SC &= ~(1<<7);//Apagar bandera canal 1

			riseT = FTM0_C1V;
			FTM0_C2SC |= (1<<2); //Preparar encendido
			FTM0_C2SC |= (1<<6); //Habilitar interrupciones CH2
			FTM0_SC &= ~(1<<3);
			FTM0_C2V = (riseT + 2300 - (ADC1_RA/31));
			FTM0_SC |= (1<<3);
			ADC1_SC1A |= 0; //Leer SC1x inicia nuevamente una conversión
			FTM0_C1SC = 0x48;
			on=1;	
		}
	}
}
/*
void ADC1_IRQHandler(void)
{
	ADC1_SC1A |= 0; //Leer SC1x inicia nuevamente una conversión
}
 */
void pin_config(void)
{
	SIM_SCGC5 |= (1<<11); //Reloj del puerto C
	PORTC_PCR3 = (1<<10); //pin PTC3 alternativa 4, FTM0 CH2
	PORTC_PCR2 = (1<<10); //pin PTC2 alternativa 4, FTM0 CH1
}

void TMR_config(void)
{
	NVICISER1 = (1<<10); //Interrupciones, IRQ=42
	SIM_SCGC6 |= (1<<24); //Habilitar reloj de FTM0
	FTM0_SC = 7; //no encender timer, preescaler 128

	FTM0_C1SC = 0x44; //Input capture on rising, interrupciones de canal

	FTM0_C2SC |= 0x18; //Output compare, clear on match

	FTM0_SC |= (1<<3); //Encender timer
}

void ADC_config(void)
{
	SIM_SCGC3 |= (1<<27); //Habilitar reloj de ADC1
	ADC1_CFG1 = 0xC; //preescaler/1 , 16 bits, bus clock
	ADC1_SC3 |= 4; //Promedio activado a 4 muestras
	ADC1_SC1A = 0x12; //single-ended, AD18
	//ADC1_SC1A |= (1<<6); //Habilitar interrupciones
	//NVICISER2 = (1<<9);//Habilitar interrupciones, IRQ 73
}

int main(void)
{
	pin_config();
	ADC_config();
	TMR_config();

	return 0;
}
