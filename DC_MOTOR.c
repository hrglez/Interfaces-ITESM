//Programa para control de motor DC por medio de PWM

#include "derivative.h" /* include peripheral declarations */

void pin_config(void)
{
	SIM_SCGC5 |= (1<<11); //Reloj del puerto C
	PORTC_PCR2 = (1<<10); //Alternativa 4 para el pin PTC2 (TMR0), canal 1
	PORTC_PCR3 = (1<<10); //Alternativa 4 para el pin PTC3 (TMR0), canal 2
	PORTC_PCR4 = (1<<8); //Inicializar pin 2 de puerto C como GPIO (general purpose I/O)
}

void PWM_config(void)
{
	SIM_SCGC6 |= (1<<24); //Habilitar reloj de FTM0
	FTM0_SC = 0x23; //Center aligned PWM, no encender timer, habilitar preescaler entre 128
	FTM0_CNT = 0; //Inicializar contador del timer en cero
	FTM0_MOD = 0x7FFF; //Valor de comparación mayor (no debe superar 0x7FFF)
	FTM0_C2SC = (1<<3); //Habilitar PWM center aligned
	FTM0_C2V = 0; //Valor del canal 2 para PWM
	FTM0_CNTIN = 0; //Valor referencia inicial para PWM

	FTM0_C1SC = (1<<3); //Habilitar PWM center aligned
	FTM0_C1V = 0; //Valor del canal 2 para PWM

	FTM0_SC |= 8; //Encender timer
}

void ADC_config(void)
{
	SIM_SCGC3 |= (1<<27); //Habilitar reloj de ADC1
	ADC1_CFG1 = 0xC; //preescaler/1 , 16 bits, bus clock
	ADC1_SC3 |= 7;
	ADC1_SC1A = 0x12; //single-ended, AD18
}

int main(void)
{
	pin_config();
	PWM_config();
	ADC_config();

	while(1)
	{
		do{}while( (ADC1_SC1A & (1<<7)) == 0 );

		if(GPIOC_PDIR & (1<<4))
		{
			FTM0_C2V = 0;
			FTM0_C1V = (ADC1_RA)/2; //Dividir ADC1_RA entre dos para no superar el valor de MOD

		}
		else
		{
			FTM0_C1V = 0;
			FTM0_C2V = (ADC1_RA)/2;
		}

		ADC1_SC1A |= 0; //Leer SC1x inicia nuevamente una conversión

	}

	return 0;
}
