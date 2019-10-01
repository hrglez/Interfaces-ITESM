/*
 * File:   motor_a_pasos1.c
 * Author: Humberto
 *
 * Created on 25 de marzo de 2019, 11:04 AM
 */

#include <xc.h>
#include "PIC18F25k80.h"

#pragma config XINST=OFF
#pragma config MCLRE = OFF      // Master Clear Enable (MCLR Disabled, RG5 Enabled)
#pragma config SOSCSEL = DIG    // SOSC Power Selection and mode Configuration bits (Digital (SCLKI) mode)

#define period_overflowTMR 131  //we start with this value so the overflow of the timer takes 0.25 ms each time
#define valueToStop 240         //value from the ADC converter small enough to stop the motor

unsigned char secuencia[4] = {0x01, 0x02, 0x04, 0x08};
unsigned int time = 0;
unsigned char i = 0;

void interrupt ISR_alta (void)
{   
    if (TMR0IF)
    {
        TMR0IF = 0;
        TMR0L = period_overflowTMR;
        if (time == 0)
        {
            if (ADRESH > valueToStop) PORTC = 0;
            else
            {
            if (RC7 == 0) PORTC  = secuencia[(i++)%4];
            if (RC7 == 1) PORTC = secuencia[(i--)%4];
            }
            GO_DONE = 1;
        }
        else  --time;
    }  
    if (ADIF)
    {
        ADIF = 0;
        time = ADRESH;
    }
}

void main(void)
{
    IPEN = 1;                   //Habilitar niveles de prioridad
    GIEH = 1;                   //Habilitador global interrupciones de alta prioridad
    
    T0CON = 0b11000001;         //TMR 0 enable, con prescaler 1:4 y base tiempo interna... 2 us
    TMR0L = period_overflowTMR; //empieza en 131 para contar 2us 125 veces (0.25ms)
    TMR0IE = 1;                 //Habilitador local TMR0
    TMR0IP = 1;                 //Alta prioridad
    
    ADCON1 = 0;                 //conversión no diferencial
    ADCON0 = 0b00101000;        //canal 10 y todo apagado
    ADCON2 = 0b00010001;        // set de TACQ=4TAD, justificado a la izquierda y FOSC/8
    
    ADIE = 1;                   //Habilitador local ADC
    ADIP = 1;                   //Alta prioridad
    
    ADON = 1;                   //Habilitar el ADC
    TRISC = 0xF0;
    
    while (1)
    {
        ;
    }
}