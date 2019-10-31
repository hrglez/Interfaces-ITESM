/*
 * main implementation: use this 'C' sample to create your own application
 *
 */

#include "derivative.h" /* include peripheral declarations */

char DONE = 0;
int i;
int cont=0;
int msg_sz=91;
char mensaje_vacio[]={"Acelerometro: EJE-X= %%%%%% EJE-Y= %%%%%% EJE-Z= %%%%%% ANGULOX=%%% ANGULOY=%%% ANGULOZ=%%%"};
char mensaje[]={"Acelerometro: EJE-X= %%%%%% EJE-Y= %%%%%% EJE-Z= %%%%%% ANGULOX=%%% ANGULOY=%%% ANGULOZ=%%%"};

#define NOP()                              \
		for(i=100;i!=0;i--)               \

void LPTM_init (void)
{
	SIM_SCGC5 |= 1;
	LPTMR0_PSR = 0b101;
}

void  delay_ms (unsigned char ms)
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

void I2C_Config(void)
{
	SIM_SCGC4 = (1<<6); //encender clock del modulo i2c
	SIM_SCGC5 = (1<<13); //HABILITAR CLOCK DEL PUERTO E
	PORTE_PCR24 = 0x500; //alternativa 5 i2c0_SCL
	PORTE_PCR25 = 0x500; //Alternativa 5 I2C0_SDA


	NVICISER1 = (1<<8); //activar interrupciones i2c
	I2C0_F = 0b10001111; //configurar velocidades del i2c
	I2C0_C1 = 0b11001000; //activar modulo i2c
}

void UART3_Status_IRQHandler()
{
	unsigned char dummy;
	if(UART3_S1 & 0x40) //checar bandera de transmision
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
			UART3_C2 = 0; //APAGAR UART Y TRANSMISION
			cont = 0;
			int idx;
			for(idx=0; idx < msg_sz; idx++)
			{
				mensaje[idx] = mensaje_vacio[idx];
			}
			DONE = 1;
		}
	}
}

char byte_read(char control, char address)
{
	char dato;
	I2C0_C1 |= (3<<4); //activar modo maestro y transmit //start bit

	I2C0_D = (control<<1);
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;

	I2C0_D = address;
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;

	I2C0_C1 |= (1<<2);

	I2C0_D = (control<<1)+1;
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;

	I2C0_C1 &= ~(1<<4);
	I2C0_C1 |= (1<<3);
	dato = I2C0_D;

	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;

	I2C0_C1 &= ~(3<<4); //modo slave //stop bit
	dato = I2C0_D;
	NOP();
	return dato;
}

void Read_IMU (void)
{
	char X_H = byte_read(0x68, 59);
	char X_L = byte_read(0x68, 60);
	char Y_H = byte_read(0x68, 61);
	char Y_L = byte_read(0x68, 62);
	char Z_H = byte_read(0x68, 63);
	char Z_L = byte_read(0x68, 64);

	short X = (X_H<<8)+X_L;
	short Y = (Y_H<<8)+Y_L;
	short Z = (Z_H<<8)+Z_L;

	char x_angulo = X/177;
	char y_angulo = Y/183;
	char z_angulo = Z/189;

	//checar signos
	if(X&0x8000)
	{
		mensaje[21]='-';
		mensaje[64]='-';
		X *= -1;
		x_angulo *= -1;
	} else 
	{
		mensaje[21]='+';
		mensaje[64]='+';
	}
	if(Y&0x8000)
	{
		mensaje[35]='-';
		mensaje[76]='-';
		Y *= -1;
		y_angulo *= -1;
	}else 
	{
		mensaje[35]='+';
		mensaje[76]='+';
	}
	if(Z&0x8000)
	{
		mensaje[49]='-';
		mensaje[88]='-';
		Z *= -1;
		z_angulo *= -1;
	}else 
	{
		mensaje[49]='+';
		mensaje[88]='+';
	}

	int idx;
	char X_serial[5];
	char Y_serial[5];
	char Z_serial[5];
	int decimal = 10000;
	for(idx=0;idx<5;idx++)
	{
		X_serial[idx] = (X/decimal)+0x30;
		Y_serial[idx] = (Y/decimal)+0x30;
		Z_serial[idx] = (Z/decimal)+0x30;
		X -= decimal*(X/decimal);
		Y -= decimal*(Y/decimal);
		Z -= decimal*(Z/decimal);
		decimal /=10;
	}

	char X_angulo_serial[2];
	char Y_angulo_serial[2];
	char Z_angulo_serial[2];
	decimal=10;
	for(idx=0;idx<2;idx++)
	{
		X_angulo_serial[idx] = (x_angulo/decimal)+0x30;
		Y_angulo_serial[idx] = (y_angulo/decimal)+0x30;
		Z_angulo_serial[idx] = (z_angulo/decimal)+0x30;
		x_angulo -= decimal*(x_angulo/decimal);
		y_angulo -= decimal*(y_angulo/decimal);
		z_angulo -= decimal*(z_angulo/decimal);
		decimal /=10;
	}

	for(idx=0;idx<5;idx++)
	{
		while(mensaje[cont]!='%')
		{
			cont++;
		}
		mensaje[cont] = X_serial[idx];
		cont++;
	}
	for(idx=0;idx<5;idx++)
	{
		while(mensaje[cont]!='%')
		{
			cont++;
		}
		mensaje[cont] = Y_serial[idx];
		cont++;
	}
	for(idx=0;idx<5;idx++)
	{
		while(mensaje[cont]!='%')
		{
			cont++;
		}
		mensaje[cont] = Z_serial[idx];
		cont++;
	}
	for(idx=0;idx<2;idx++)
	{
		while(mensaje[cont]!='%')
		{
			cont++;
		}
		mensaje[cont] = X_angulo_serial[idx];
		cont++;
	}
	for(idx=0;idx<2;idx++)
	{
		while(mensaje[cont]!='%')
		{
			cont++;
		}
		mensaje[cont] = Y_angulo_serial[idx];
		cont++;
	}
	for(idx=0;idx<2;idx++)
	{
		while(mensaje[cont]!='%')
		{
			cont++;
		}
		mensaje[cont] = Z_angulo_serial[idx];
		cont++;
	}

	cont = 0;
	UART_TRANSMISION_CONFIG();
	do{}while(DONE == 0);
	DONE = 0;

	delay_ms(250);
	delay_ms(250);
	delay_ms(250);
	delay_ms(250);
}

int main(void)
{
	I2C_Config();
	UART_INIT();
	LPTM_init();
	while(1)
	{
		Read_IMU();	
	}

	return 0;
}
