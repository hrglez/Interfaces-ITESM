//I2C por hardware para comunicar con un RTC (reloj en tiempo real)

#include "derivative.h" /* include peripheral declarations */

#define RTC_ID 0xD0
#define DIR_SEC 0
#define DIR_MIN 1
#define DIR_HR 2
#define DIR_WKDY 3
#define DIR_DAY 4
#define DIR_MTH 5
#define DIR_YEAR 6


void LPTM_init (void)
{
	SIM_SCGC5 |= 1;
	LPTMR0_PSR = 0b101;
}

void pin_config(void)
{
	SIM_SCGC5 = (1<<13); //puerto E
	PORTE_PCR24 |= 0x500; //alternativa 5 de pin, SCL
	PORTE_PCR25 |= 0x500; //alternativa 5 de pin, SDA
}

void I2C_init(void)
{
	SIM_SCGC4 = (1<<6); //Reloj de I2C0
	NVICISER1 = (1<<8);  //Habilitar interrupciones de I2C0, vector=40

	I2C0_F = 0x9F; //factor mul en 4; baud rate en 77 kHz
	I2C0_C1 = 0xC8; //Enable I2C, Slave mode, interrupt enable, transmit mode, no acknowledge
}

unsigned char dec_bcd(unsigned char dec)
{
	unsigned char bcd;

	bcd = dec/10;
	bcd <<= 4;
	bcd += (dec%10);

	return bcd;
}

unsigned char bcd_dec (unsigned char bcd)
{
	unsigned char dec;

	dec = (bcd>>4)*10;
	dec += (bcd & 0xF);

	return dec;
}

void  delay_ms (unsigned char ms)
{
	LPTMR0_CMR = ms-1; //El timer cuenta hasta -1
	LPTMR0_CSR = 1;	//Encender timer
	do{
	} while ((LPTMR0_CSR&(1<<7))==0);
	LPTMR0_CSR = (1<<7); //La bandera se apaga al escribirle un 1, al mismo tiempo se apaga el timer
}

void byte_write(unsigned char ident, unsigned char direccion, unsigned char dato)
{
	ident = (ident>>1) <<1;

	I2C0_C1 |= (3<<4); //Configurar a Maestro y Transmit //Start bit
	I2C0_D = ident; //Escribir identificador y bit Read/NotWrite
	while( (I2C0_S & 2) ==0);
	I2C0_S |= 2; 	
	/*I2C0_D = (direccion>>8);
	while( (I2C0_S & 2) ==0);
	I2C0_S |= 2; 	*/
	I2C0_D = direccion;
	while( (I2C0_S & 2) ==0);
	I2C0_S |= 2; 	
	I2C0_D = dato;
	while( (I2C0_S & 2) ==0);
	I2C0_S |= 2; 	
	I2C0_C1 &= ~(3<<4); //Cambiar a esclavo y receive //Stop bit

	delay_ms(5);
}

unsigned char byte_read(unsigned char ident, unsigned short direccion)
{
	unsigned char dato;

	ident = (ident>>1) <<1;

	I2C0_C1 |= (3<<4); //Configurar a Maestro y Transmit
	I2C0_D = ident; //Escribir identificador y bit Read/NotWrite
	while( (I2C0_S & 2) ==0);
	I2C0_S |= 2; 	
	/*I2C0_D = (direccion>>8);
	while( (I2C0_S & 2) ==0);
	I2C0_S |= 2; 	*/
	I2C0_D = direccion;
	while( (I2C0_S & 2) ==0);
	I2C0_S |= 2; 	

	I2C0_C1 |= (1<<2); //CRepeat start
	I2C0_D = (ident+1); //Escribir identificador y bit Read/NotWrite
	while( (I2C0_S & 2) ==0);
	I2C0_S |= 2; 	

	I2C0_C1 &= ~(1<<4);
	I2C0_C1 |= (1<<3);

	dato = I2C0_D;

	while( (I2C0_S & 2) ==0);
	I2C0_S |= 2; 	

	I2C0_C1 &= ~(3<<4); //Cambiar a esclavo y receive //Stop bit

	dato = I2C0_D;

	return dato;
}

void write_bcd (unsigned char direccion, unsigned char dato)
{
	unsigned char temp;
	temp = dec_bcd(dato);
	byte_write(RTC_ID, direccion, temp);
}

unsigned char read_bcd (unsigned char direccion)
{
	unsigned char temp;
	temp = byte_read (RTC_ID, direccion);
	temp = bcd_dec(temp);
	return temp;
}

int main(void)
{
	unsigned char dato;

	pin_config();
	I2C_init();
	LPTM_init();

	write_bcd(0,55);
	write_bcd(1,59);
	write_bcd(2,9);
	write_bcd(3,7);
	write_bcd(4,2);
	write_bcd(5,3);
	write_bcd(6,19);

	dato = read_bcd(DIR_SEC);	
	/*dato = read_bcd(DIR_MIN);
	dato = read_bcd(DIR_HR);
	dato = read_bcd(DIR_WKDY);	
	dato = read_bcd(DIR_DAY);
	dato = read_bcd(DIR_MTH);
	dato = read_bcd(DIR_YEAR);	*/

	return 0;
}
