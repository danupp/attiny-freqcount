// Targets an ATtiny817

#define F_CPU 20000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

#define DEB_pin PIN4_bm // PA4

volatile uint32_t freqval;
volatile uint8_t new_val_flag = 0;


ISR(RTC_PIT_vect) { 
  
  static uint32_t countval = 0;
  uint32_t addval;
  static uint16_t lastval = 0;
  static uint32_t slot = 0;
  uint16_t capt;

  RTC.PITINTFLAGS = 0x01; // clear flag
  PORTA.OUT |= DEB_pin;

  if (TCD0.INTFLAGS & 0x04) { // Capture completed
    
    TCD0.INTFLAGS = 0x04;
    capt = TCD0.CAPTUREA;
     
    if(capt < lastval)
      addval = 4096;
    else
      addval = 0;

    addval += capt;
    addval -= lastval;

    // if(slot==1000)  // 10 Hz
    //  addval = addval >> 1;  // *0.5

    countval += addval;
    lastval = capt;
  }
 
// if (slot<7812)   // 10 Hz 
  if (slot<78124) // 1 Hz
    slot++;
  else {
    freqval = countval;
    countval = 0x00000000;
    new_val_flag = 0x01;  
    slot = 0;
  }
  PORTA.OUT &= ~DEB_pin;
}

void main () {
  
  char buffer[50];
  uint8_t uart_n;
  uint16_t freq_MHz, freq_kHz, freq_Hz;

  PORTA.DIR = DEB_pin | PIN1_bm; // outputs
  PORTB.DIR = PIN5_bm;
  PORTA.OUT = 0x00;
  
  _delay_ms(200);

  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB,0x00); // Main clock divide by 1

  //_PROTECTED_WRITE(CLKCTRL.MCLKCTRLA,0x80); // Clockout for test
  
  //EVSYS.ASYNCCH3 = 0x0D; // PIT_DIV1024
  //EVSYS.ASYNCCH3 = 0x0E; // PIT_DIV512
  EVSYS.ASYNCCH3 = 0x10; // PIT_DIV128
  EVSYS.ASYNCUSER6 = 0x06; // ASYNCCH3 -> TCD0_EV0 

  TCD0.EVCTRLA = 0b10000101;
  TCD0.CMPBCLR = 0x0FFF; // count to max
  TCD0.CTRLA = 0b01000000; // EXTCLK
  while (!(TCD0.STATUS & 0x01)); // ENRDY
  TCD0.CTRLA = 0b01000001; // EXTCLK, enable

  PORTMUX.CTRLB = 0x01; // Alternative pins for USART0

  /* Baud rate compensated with factory stored frequency error */
  int8_t sigrow_val = SIGROW.OSC20ERR5V;
  int32_t baud_reg_val = (int32_t)8334;  // 9600 baud at 20 MHz

  baud_reg_val *= (1024 + sigrow_val);
  baud_reg_val /= 1024;
  USART0.BAUD = (int16_t) baud_reg_val;

  //USART0.CTRLA = 0b10000000; // RXCIE
  USART0.CTRLB = 0b01000000; // TX enable
  //rx_ptr=0;

  _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA,0x07); // XOSC32K enable, used for 10 MHz input
  
  while(RTC.STATUS);
  RTC.CLKSEL = 0x02; // Use clock from XOSC32K/TOSC1
  while(RTC.PITSTATUS);
  RTC.PITINTCTRL = 0x01; // enable interrupt
  while(RTC.PITSTATUS);
  //RTC.PITCTRLA = 0b01001001; // Enable, interrupt after 1024 cycles => ~10 kHz at 10 MHz
  //RTC.PITCTRLA = 0b01000001; // Enable, interrupt after 512 cycles => ~20 kHz at 10 MHz
  RTC.PITCTRLA = 0b00110001; // Enable, interrupt after 128 cycles => ~80 kHz at 10 MHz

  sprintf(buffer,"** Start **\n\n");
  uart_n = 0;

  sei();
  
  while(1) {
    if (new_val_flag) {
      new_val_flag = 0x00;
      // freqval = freqval * 10;  // for 10 Hz update
      freq_MHz = freqval/(uint32_t)1e6;
      freq_kHz = (freqval % (uint32_t)1e6)/(uint16_t)1e3;
      freq_Hz = freqval % (uint16_t)1e3;    
      if (freq_MHz)
        sprintf(buffer,"%d.%03d %03d MHz \n",freq_MHz, freq_kHz, freq_Hz);
      else if(freq_kHz)
        sprintf(buffer,"%d.%03d kHz \n",freq_kHz, freq_Hz);
      else
        sprintf(buffer,"%d Hz \n",freq_Hz);
      //sprintf(buffer,"%lu Hz\n",freqval);
      uart_n = 0; // starts transmission of buffer
    }
    else if(buffer[uart_n]) {
      if (USART0.STATUS & USART_DREIF_bm) {
        USART0.TXDATAL = buffer[uart_n];
        uart_n++;
      }
    }
  }
}
