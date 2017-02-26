/*
 * File:   main.c
 * Author: user
 *
 * Created on 20 лютого 2017, 22:38
 */
#include <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <pic16f688.h>
#include "usart.h"


#define _XTAL_FREQ 4000000

// PIC16LF88 Configuration Bit Settings

// 'C' source line config statements

// CONFIG
#pragma config FOSC = INTOSCIO // Oscillator Selection bits (INTOSC oscillator: CLKOUT function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF       // MCLR Pin Function Select bit (MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Detect (BOR disabled)
#pragma config IESO = ON        // Internal External Switchover bit (Internal External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

void ADC_Init()
{
  ADCON0 = 0x10000001;               //Turn ON ADC and Clock Selection
  ADCON1 = 0b001;
}

unsigned int ADC_Read(unsigned char channel)
{
  if(channel > 7)              //Channel range is 0 ~ 7
    return 0;
  
  ADCON0 = 0b10000001;         //Clearing channel selection bits
  ADCON0 |= channel<<2;        //Setting channel selection bits
  __delay_ms(2);               //Acquisition time to charge hold capacitor
  GO_nDONE = 1;                //Initializes A/D conversion
  while(GO_nDONE) NOP();             //Waiting for conversion to complete
  return ((ADRESH<<8)+ADRESL); //Return result
}

int ulitsa = 0, tseh = 0, obratka = 0, kotel = 0;   // переменные для хранения температуры
char a[16] = ""; // буфер для команд UART
int i= 0; // просто счётчик
char c;  // символ для работы UART

#define obratka_xolodnaya   RC1
#define peregrev            RC2
#define ventil              RC3 
#define elektro             RA3
#define nasos               RA5   
// входы контроля светодиодов

void main(void) {

    // Подготавливаем всё для прерываний и ком-порта
    INTCON = 0b11000000;; //разрешить прерывания от периферии
    RCIE=1;    // разрешаем прерывание по приему байта UART
    init_comms(); // запускаем UART
    
    TRISA = 0b00101111;           //Port A as input
    TRISC = 0b00101111;           //Port C as INPUT
    ANSEL = 0b00010111;           // Аналоговые входы выбраны
    ADC_Init();                   //Initialize ADC
    
    RA4 = 0;    
    __delay_ms(1000);
    RA4 = 1;
    
    while (1) {
        

        
        ulitsa = ADC_Read(0)*1000/208;          //Read Analog Channel 0
        tseh = ADC_Read(1)*1000/208;            //Read Analog Channel 1
        obratka = ADC_Read(2)*1000/208;         //Read Analog Channel 2
        kotel = ADC_Read(4)*1000/208;           //Read Analog Channel 4
        
        //printf("%d|%d|%d|%d|%d\r\n", obratka_xolodnaya, peregrev, ventil, elektro, nasos );

        __delay_ms(1000);
        
    };
};

void interrupt isr(void) {
    
    if ((RCIE)&&(RCIF)) { // если что-то пришло в приёмник UART и принимать можно
        if(!RX9D && !OERR && !FERR) {   // если нет ошибок
            c = RCREG;
            if ( c  == 0x0d || c  == 0x0a ) { //введен символ конца строки или возврат каретки
                a[i] = '\0';
                // =============================================================
                // Начинаем распознавать и выполнять комманды, которые пришли
                // =============================================================
                if (a[0]=='A' && a[1]=='T') { //если это комманда, то обрабатываем   
                    if (a[2]=='\0') { // тестовая комманда которая возвращает просто ОК
                        print_to_uart("OK");
                    };
                    if (a[2]==' ' && a[3]=='T' && a[4]=='\0') { //AT T  - текущая температура
                        printf("%d.%d|", ulitsa/10,(ulitsa - (ulitsa/10)*10));
                        printf("%d.%d|", tseh/10,(tseh - (tseh/10)*10));
                        printf("%d.%d|", obratka/10,(obratka - (obratka/10)*10));
                        printf("%d.%d\r\n", kotel/10,(kotel - (kotel/10)*10));
                    };
                    if (a[2]==' ' && a[3]=='D' && a[4]=='\0') { //AT D - состояние светодиодов
                        printf("%d|%d|%d|%d|%d\r\n", obratka_xolodnaya, peregrev, ventil, elektro, nasos );
                        //print_to_uart("2\n");
                    };
                }; 
                // если другой набор данных - игнорируем и не отвечаем
                a[0] = '\0'; //записываем пустую строку
                i = 0;       //сбрасываем указатель символа на 0
            } else {
                //Заполняем буффер следующим символом
                if (i==16){ // если переполнение то сбрасываем на первый символ
                    a[0] = c;
                    i = 0;
                } else { //увеличиваем команду на 1 единицу
                    a[i] = c;
                    i++;
                };
            };
            
        } else {
            //сброс ошибки приёмника
            c = RCREG;
            c = RCREG;
            CREN = 0;
            NOP();
            NOP();
            CREN = 1;
        };
    };
};
