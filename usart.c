#include <xc.h>
#include "usart.h"


void putch(unsigned char byte) {
    /* output one byte */
    while(!TXIF)    /* set when register is empty */
        continue;
    TXREG = byte;
}

void print_to_uart(char *text) {
	int i;
	for(i=0;text[i]!='\0';i++) {
	   putch(text[i]);
    };
    putch('\r');
    putch('\n');
}