/*
 * main implementation: use this 'C' sample to create your own application
 *
 */

#include "derivative.h" /* include peripheral declarations */

void I2C0_IRQHandler(){
	I2C0_S |= 0x2;
}

void LPTM_init (void)
{
	SIM_SCGC5 |= 1;
	LPTMR0_PSR = 0b101;
}

void delay_ms (unsigned char ms)
{
	LPTMR0_CMR = ms-1; //El timer cuenta hasta -1
	LPTMR0_CSR = 1;	//Encender timer
	do{
	} while ((LPTMR0_CSR&(1<<7))==0);
	LPTMR0_CSR = (1<<7); //La bandera se apaga al escribirle un 1, al mismo tiempo se apaga el timer
}

void I2C_Config(void){
	SIM_SCGC4 = (1<<6); //encender clock del modulo i2c
	SIM_SCGC5 = (1<<13); //HABILITAR CLOCK DEL PUERTO E
	PORTE_PCR24 = 0x500; //alternativa 5 i2c0_SCL
	PORTE_PCR25 = 0x500; //Alternativa 5 I2C0_SDA


	NVICISER1 = (1<<8); //activar interrupciones i2c
	I2C0_F = 0b10001111; //configurar velocidades del i2c
	I2C0_C1 = 0b11001000; //activar modulo i2c
}

void byte_write(char control, char address, char data)
{
	I2C0_C1 |= (3<<4); //activar modo maestro y transmit //start bit
	I2C0_D = control;
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;
	I2C0_D = address;
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;
	I2C0_D = data;
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;
	I2C0_C1 &= ~(3<<4); //modo slave //stop bit
	delay_ms(5);//ESPERAR 5ms
}

char byte_read(char control, char address)
{
	char dato;
	I2C0_C1 |= (3<<4); //activar modo maestro y transmit //start bit
	I2C0_D = control;
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;
	I2C0_D = address;
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;

	I2C0_C1 |= (1<<2);
	I2C0_D = control+1;
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;

	I2C0_C1 &= ~(1<<4);
	I2C0_C1 |= (1<<3);
	dato = I2C0_D;
	do{}while((I2C0_S&2) == 0);
	I2C0_S |= 2;
	I2C0_C1 &= ~(3<<4); //modo slave //stop bit

	dato = I2C0_D;
	return dato;
}

char decimal2BCD (char decimal)
{
	char BCD;
	BCD = decimal/10;
	BCD <<= 4;
	BCD += decimal%10;
	return BCD;
}

char BCD2decimal (char BCD)
{
	char decimal;
	decimal = (BCD>>4)*10;
	decimal += BCD & (0xF);
	
	return BCD;
}

void write_BCD(char address, char decimal)
{
	char BCD;
	BCD = decimal2BCD(decimal);
	byte_write(0xD0, address, BCD);
}

char read_BCD(char address)
{
	char dato;
	dato = byte_read(0xD0, address);
	dato = BCD2decimal(dato);
	
	return dato;
}



int main(void)
{
	LPTM_init();
	I2C_Config();
	char year,month,date,day,minutes,seconds;
	
	write_BCD(6,19);
	write_BCD(5,3);
	write_BCD(4,1);
	write_BCD(3,6);
	write_BCD(1,4);
	write_BCD(0,0);
	
	year = read_BCD(6);
	month = read_BCD(5);
	date = read_BCD(4);
	day = read_BCD(3);
	minutes = read_BCD(1);
	seconds = read_BCD(0);

	return 0;
}
