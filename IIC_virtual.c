#include "derivative.h" /* include peripheral declarations */

//NOP retarda 2 us
#define NOP() 			\
		for(i=1;i!=0;i--) 	\

char i;

void vIIC_init(void)
{
	SIM_SCGC5 = (1<<11);
	PORTC_PCR2 = (1<<8); //Inicializar pin 2 de puerto C como GPIO (general purpose I/O)
	PORTC_PCR3 = (1<<8);
	GPIOC_PDDR = 0xC; //Configurar pines 2 y 3 (SDA y SCL) como salida
}

void LPTM_init (void)
{
	SIM_SCGC5 |= 1;
	LPTMR0_PSR = 0b101;
}

void  delay_ms (unsigned char ms)
{
	LPTMR0_CMR = ms-1; //El timer cuenta hasta -1
	LPTMR0_CSR = 1;	//Encender timer
	do{
	} while ((LPTMR0_CSR&(1<<7))==0);
	LPTMR0_CSR = (1<<7); //La bandera se apaga al escribirle un 1, al mismo tiempo se apaga el timer
}

void vIIC_start (void)
{
	GPIOC_PDOR = 0xC; //SDA y SCL en alto
	NOP(); //Esperar tiempo de respuesta del dispositivo, para memoria es 600 ns 
	GPIOC_PDOR = 8;//Apagar SDA (pin 2) y dejar encendido SCL
	NOP();
	GPIOC_PDOR = 0; //Apagar SCL (pin 3)
}

void vIIC_stop (void){
	GPIOC_PDOR = 8; //Subir SCL
	NOP(); //esperar 't7' -> unos cuantos "NOP's" asm ("nop");
	GPIOC_PDOR = 0xC;
	NOP(); //esperar 't6'
	GPIOC_PDOR = 0;
}

void vIIC_send_byte (unsigned char dato)
{
	unsigned char cont=8;
	do{ //SCL debe estar en cero mientras cambia el dato
		if(dato&0x80) GPIOC_PDOR = 4;//Mascara para rescatar el bit más significativo; SDA=1
		else GPIOC_PDOR = 0; //SDA=0;
		NOP();
		GPIOC_PDOR |= 8; //SCL=1
		NOP();
		GPIOC_PDOR &= 0xFFFFFFF7; //SCL=0
		NOP();
		dato<<=1;
	}while (--cont);
	GPIOC_PDOR = 0;
}

unsigned char vIIC_rec_byte (void)
{
	unsigned char cont=8;
	unsigned char temp;
	GPIOC_PDDR = 8; //SDA como entrada, SCL salida
	do{
		GPIOC_PDOR = 8; //SCL=1
		temp<<=1;
		if (GPIOC_PDIR &= 4) temp++;
		GPIOC_PDOR = 0;
		NOP();
	}while (--cont);
	GPIOC_PDDR = 0xC; //SDA como salida
	return temp;
}

void vIIC_ack_output (unsigned char ack_bit)
{
	if (ack_bit) GPIOC_PDOR = 4; //SDA=1
	GPIOC_PDOR = 0xC; //Subir SCL
	NOP();
	GPIOC_PDOR = 0;
	NOP();
}

unsigned char vIIC_ack_input (void){
	unsigned char temp=0;
	GPIOC_PDDR = 8; //Definir SDA como entrada y SCL como salida
	NOP();
	GPIOC_PDOR = 8;	//SCL=1
	NOP();
	if (GPIOC_PDIR &= 4) temp=1;
	GPIOC_PDOR = 0;//SCL=0;
	GPIOC_PDDR = 0xC;//Definir SDA como salida;
	NOP();
	return temp;
}

void vIIC_byte_write(unsigned short direccion, unsigned char dato)
{
	vIIC_start();
	vIIC_send_byte(0xA0);
	if (vIIC_ack_input ()==0)
	{
		vIIC_send_byte(direccion>>8);
		if(vIIC_ack_input()==0)
		{
			vIIC_send_byte(direccion);
			if(vIIC_ack_input()==0)
			{
				vIIC_send_byte(dato);
				if(vIIC_ack_input()==0)
				{
					vIIC_stop();
					delay_ms(5);
				}
			}
		}
	}
}

unsigned char vIIC_byte_read (unsigned short direccion)
{
	unsigned char temp=0;

	vIIC_start();
	vIIC_send_byte (0xA0);
	NOP();
	if (vIIC_ack_input ()==0)
	{
		vIIC_send_byte(direccion>>8);
		if(vIIC_ack_input()==0)
		{
			vIIC_send_byte(direccion);
			if(vIIC_ack_input()==0)
			{
				vIIC_start();
				vIIC_send_byte(0xA1);
				if(vIIC_ack_input()==0)
				{
					temp = vIIC_rec_byte();
					vIIC_ack_output(1);
					vIIC_stop();
				}
			}
		}
	}
	return temp;
}

int main (void){
unsigned char dato;	
	vIIC_init();
	LPTM_init();
	//dato=vIIC_byte_read(0xAAA);
	//vIIC_byte_write(0xAAA,77);
	dato=vIIC_byte_read(0xAAA);
	return 0;
}
