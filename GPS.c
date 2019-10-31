/*
 * main implementation: use this 'C' sample to create your own application
 *
 */

#include "derivative.h" /* include peripheral declarations */

char lectura,latitud,longitud;
int dia,mes,year,latitud_grados,latitud_minutos,longitud_grados,longitud_minutos,hora,minutos,segundos,latitud_segundos,longitud_segundos;
int cont = 0;
int cont2 = 0;
int msg_idx;
int msg_sz=98;
char mensaje_vacio[] = {"HORA: %%:%%:%%%%%% Validation: % LATITUD: %%* %%%%%%%' % LONGITUD: %%%* %%%%%%%' % FECHA: %%/%%/%%"};
char mensaje[] = {"HORA: %%:%%:%%%%%% Validation: % LATITUD: %%* %%%%%%%' % LONGITUD: %%%* %%%%%%%' % FECHA: %%/%%/%%"};

void UART_TRANSMISION_CONFIG(void)
{
	UART3_C2 = 0b01001000; //activar interrupciones de transmision y habilitar modo transmision
}

void UART_RECEPTION_CONFIG(void)
{
	UART3_C2 = 0b00100100; //activar interrupciones de recepcion y habilitar modo recepcion
}

void parsel(void)
{
	if(cont == 0 && lectura=='$')
	{
		cont++;
	}
	if(cont == 1 && lectura=='G')
	{
		cont++;
	}
	if(cont == 2 && lectura=='P')
	{
		cont++;
	}
	if(cont == 3 && lectura=='R')
	{
		cont++;
	}
	if(cont == 4 && lectura=='M')
	{
		cont++;
	}
	if(cont == 5 && lectura=='C')
	{
		cont++;
		msg_idx = 0;
	}
	if(cont >= 6 && lectura==',')
	{
		cont++;
	}
	if(cont == 7 && lectura!=',')
	{
		while(mensaje[msg_idx]!='%')
		{
			msg_idx++;
		}
		mensaje[msg_idx] = lectura;
	}
	if(cont == 8 && lectura!=',')
	{
		while(mensaje[msg_idx]!='%')
		{
			msg_idx++;
		}
		mensaje[msg_idx] = lectura;
	}
	if(cont == 9 && lectura!=',')
	{
		while(mensaje[msg_idx]!='%')
		{
			msg_idx++;
		}
		mensaje[msg_idx] = lectura;
	}
	if(cont == 10 && lectura!=',')
	{
		while(mensaje[msg_idx]!='%')
		{
			msg_idx++;
		}
		mensaje[msg_idx] = lectura;
	}
	if(cont == 11 && lectura!=',')
	{
		while(mensaje[msg_idx]!='%')
		{
			msg_idx++;
		}
		mensaje[msg_idx] = lectura;
	}
	if(cont == 12 && lectura!=',')
	{
		while(mensaje[msg_idx]!='%')
		{
			msg_idx++;
		}
		mensaje[msg_idx] = lectura;
	}
	if(cont == 15 && lectura!=',')
	{
		while(mensaje[msg_idx]!='%')
		{
			msg_idx++;
		}
		mensaje[msg_idx] = lectura;
	}
	if(cont == 16)
	{
		cont=0;
		UART_TRANSMISION_CONFIG();
	}
}

void save(void)
{
	hora = (mensaje[6]-0x30)*10;
	hora += mensaje[7]-0x30;
	hora = (hora+18)%24;
	minutos = (mensaje[9]-0x30)*10;
	minutos += mensaje[10]-0x30;
	segundos = (mensaje[12]-0x30)*10;
	segundos += mensaje[13]-0x30;

	latitud_grados = (mensaje[42]-0x30)*10;
	latitud_grados += (mensaje[43]-0x30);
	latitud_minutos = (mensaje[46]-0x30)*10;
	latitud_minutos += (mensaje[47]-0x30);
	latitud_segundos = (mensaje[49]-0x30)*10;
	latitud_segundos += (mensaje[50]-0x30);
	latitud_segundos *= 60;
	latitud_segundos /= 100;
	latitud = mensaje[55];
	
	longitud_grados = (mensaje[67]-0x30)*100;
	longitud_grados += (mensaje[68]-0x30)*10;
	longitud_grados += (mensaje[69]-0x30);
	longitud_minutos = (mensaje[72]-0x30)*10;
	longitud_minutos += (mensaje[73]-0x30);
	longitud_segundos = (mensaje[75]-0x30)*10;
	longitud_segundos += (mensaje[76]-0x30);
	longitud_segundos *= 60;
	longitud_segundos /= 100;
	longitud = mensaje[81];

	dia = (mensaje[90]-0x30)*10;
	dia += mensaje[91]-0x30;
	mes = (mensaje[93]-0x30)*10;
	mes += mensaje[94]-0x30;
	year = 2000;
	year += (mensaje[96]-0x30)*10;
	year += mensaje[97]-0x30;
}

void UART3_Status_IRQHandler()
{
	unsigned char dummy;
	if(UART3_S1 & 0x20) //checar bandera de recepcion
	{
		dummy = UART3_S1;
		lectura = UART3_D; //leer recepcion y apagar bandera
		parsel();
	}else if(UART3_S1 & 0x40) //checar bandera de transmision
	{
		dummy = UART3_S1; //linea necesaria para apagar bandera de transmision
		if(cont < msg_sz)
		{
			UART3_D = mensaje[cont];
			cont++;
		}else if(cont == msg_sz)
		{
			UART3_D = '\n';
			cont++;
		}else if(cont == (msg_sz+1))
		{
			UART3_D = '\r';
			cont++;
		}else
		{
			UART3_D = 0; //lienea necesaria para apagar bandera
			cont = 0;
			save();
			int idx;
			for(idx=0; idx < msg_sz; idx++)
			{
				mensaje[idx] = mensaje_vacio[idx];
			}
			UART_RECEPTION_CONFIG();
		}
	}
}

void UART_INIT(void)
{
	SIM_SCGC4 |= (1<<13); //activar clock de UART3
	SIM_SCGC5 |= (1<<11); //activar clock de puerto C
	PORTC_PCR16 = (3<<8); //alternativa 3 de PORTC16
	PORTC_PCR17 = (3<<8); //alternativa 3 de PORTC17
	UART3_BDH = 0; //condiguracion de baudrate
	UART3_BDL = 0x88; //configuracion de baudrate
	NVICISER1 = (1<<5); //activar interrupciones de UART. El shift se determina con el IRQ%32, el IRQ esta en una tabal del manual
}

int main(void)
{
	UART_INIT();
	//UART_TRANSMISION_CONFIG();
	UART_RECEPTION_CONFIG();	
	while(1);
	return 0;
}
