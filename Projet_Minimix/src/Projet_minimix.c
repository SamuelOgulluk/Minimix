#include <stdio.h>
#include "LPC8xx.h"
#include "adc.h"
#include "dac.h"
#include "swm.h"
#include "syscon.h"
#include "utilities.h"
#include "iocon.h"
#include "gpio.h"
#include "chip_setup.h"

uint32_t SystemCoreClock = 15000000;

void setup_debug_uart(void);

volatile uint32_t seqa_buffer[12];
volatile uint32_t seqb_buffer[12];
volatile enum {false, true} seqa_handshake, seqb_handshake;
uint32_t current_seqa_ctrl, current_seqb_ctrl;

#define OUTPORT_B P0_10
#define INPORT_B  P0_18
#define OUTPORT_A P0_14
#define INPORT_A  P0_19

volatile uint32_t current_clkdiv, val, temp_data, n;

// Acquisition des low,mid,high pour les 2 canaux
// Tout sur le CN5 dans l'ordre de bas en haut
#define ADC_HIGH2 (1<<10) //pin_1
#define ADC_MID2 (1<<21) //pin_4
#define ADC_LOW2 (1<<11) //pin7
#define ADC_LOW1 (1<<19) //pin17
#define ADC_MID1 (1<<13) //pin 16
#define ADC_HIGH1 (1<<17) //pin 10

// Acquisition des gains 1 et 2 sur CN8
#define ADC_GAIN1 (1<<16) //pin11
#define ADC_GAIN2 (1<<20)//pin13

//Acquisition du signal pas stereo
#define ADC_IN1L (1<<12) //pin14
#define ADC_IN2L (1<<18) //pin15

#define pinss {"HIGH2" , "MID2" , "LOW2","HIGH1" , "MID1" , "LOW1", "GAIN1", "GAIN2"}
#define low_thresh (0xFFF / 3)
#define high_thresh ((2 * 0xFFF) / 3)


volatile unsigned char temp;

volatile uint32_t counter_20hz = 0;

volatile uint32_t gain1=1,gain2=1,low1=1,low2=1,high1=1,high2=1,mid1=1,mid2=1;

// Coefficients optimisés pré-calculés

volatile int32_t C_L1 = 0, C_M1 = 0, C_H1 = 0;
volatile int32_t C_L2 = 0, C_M2 = 0, C_H2 = 0;

volatile uint32_t counter_div = 0;



void MRT_Init_Master(void) {
    LPC_SYSCON->SYSAHBCLKCTRL0 |= (1 << 10);
    LPC_SYSCON->PRESETCTRL0 &= ~(1 << 10);
    LPC_SYSCON->PRESETCTRL0 |= (1 << 10);

    // Timer réglé à 40 kHz (une interruption toutes les 25µs)
    LPC_MRT->Channel[0].INTVAL = (SystemCoreClock / 40000) & 0x7FFFFFFF;
    LPC_MRT->Channel[0].CTRL = (1 << 0);
    NVIC_EnableIRQ(MRT_IRQn);
}

void MRT_IRQHandler(void) {
    if (LPC_MRT->Channel[0].STAT & (1 << 0)) {
        LPC_MRT->Channel[0].STAT = (1 << 0);

        // =========================================================
        // 1. TRAITEMENT AUDIO (CENTRÉ SUR 0)
        // =========================================================
        static int32_t f1_low = 0, f1_wide = 0;
        static int32_t f2_low = 0, f2_wide = 0;
        register int32_t r_in, r_mid, r_high;
        register int32_t total_mix = 0;

        // --- Voie 1 ---
        // 1. On récupère le signal brut (0 à 4095) et on le centre

        r_in = ((seqb_buffer[2] >> 4) & 0xFFF) - 2048;

        f1_low  += (r_in - f1_low) >> 5;
        f1_wide += (r_in - f1_wide) >> 2;

        r_mid  = f1_wide - f1_low;
        r_high = r_in - f1_wide;

        total_mix += (f1_low * C_L1);
        total_mix += (r_mid  * C_M1);
        total_mix += (r_high * C_H1);

        // --- Voie 2 ---
        r_in = ((seqb_buffer[8] >> 4) & 0xFFF) - 2048; // Centrage sur 0

        f2_low  += (r_in - f2_low) >> 5;
        f2_wide += (r_in - f2_wide) >> 2;

        r_mid  = f2_wide - f2_low;
        r_high = r_in - f2_wide;

        total_mix += (f2_low * C_L2);
        total_mix += (r_mid  * C_M2);
        total_mix += (r_high * C_H2);

        // --- Sortie ---
        // Mise à l'échelle (Division par 16384 via shift)
        total_mix = total_mix >> 14;

        // On rajoute 2048 pour centrer le signal pour le DAC
        total_mix += 2048;

        // Saturation
        if (total_mix > 4095) total_mix = 4095;
        else if (total_mix < 0) total_mix = 0;

        // Envoi au DAC (12 bits -> 10 bits)
        LPC_DAC0->CR = total_mix << 4;

        // =========================================================
        // 2. GESTION DES GAINS
        // =========================================================
        counter_div++;

        if (counter_div < 20000) {
            LPC_ADC->SEQB_CTRL = current_seqb_ctrl | (1 << 26);
        }
        else if (counter_div == 10000) {
            LPC_ADC->SEQA_CTRL = current_seqa_ctrl | (1 << 26);
        }
        else {
            counter_div = 0;

            int32_t raw_g1 = (seqa_buffer[6] >> 4)  & 0xFFF;
            int32_t raw_g2 = (seqa_buffer[10] >> 4) & 0xFFF;

            gain1 = (raw_g1 * 100) >> 12;
            gain2 = (raw_g2 * 100) >> 12;

            low1  = ((seqa_buffer[9] >> 4)  & 0xFFF) * 100 >> 12;
            mid1  = ((seqa_buffer[3] >> 4)  & 0xFFF) * 100 >> 12;
            high1 = ((seqa_buffer[7] >> 4)  & 0xFFF) * 100 >> 12;

            low2  = ((seqa_buffer[1] >> 4)  & 0xFFF) * 100 >> 12;
            mid2  = ((seqa_buffer[11] >> 4) & 0xFFF) * 100 >> 12;
            high2 = ((seqa_buffer[0] >> 4)  & 0xFFF) * 100 >> 12;

            C_L1 = low1 * gain1; if(C_L1 < 0) C_L1 = 0;
            C_M1 = mid1 * gain1; if(C_M1 < 0) C_M1 = 0;
            C_H1 = high1 * gain1; if(C_H1 < 0) C_H1 = 0;

            C_L2 = low2 * gain2; if(C_L2 < 0) C_L2 = 0;
            C_M2 = mid2 * gain2; if(C_M2 < 0) C_M2 = 0;
            C_H2 = high2 * gain2; if(C_H2 < 0) C_H2 = 0;

            LPC_ADC->SEQB_CTRL = current_seqb_ctrl | (1 << 26);
        }
    }
}

void setup_general_adc(){

	  LPC_SYSCON->PDRUNCFG &= ~(ADC_PD);
	  LPC_SYSCON->PRESETCTRL0 &= (ADC_RST_N);
	  LPC_SYSCON->PRESETCTRL0 |= ~(ADC_RST_N);
	  LPC_SYSCON->SYSAHBCLKCTRL0 |= (ADC | GPIO0 | GPIO_INT | SWM);
	  LPC_SYSCON->ADCCLKDIV = 1;                 // Enable clock, and divide-by-1 at this clock divider
	  LPC_SYSCON->ADCCLKSEL = ADCCLKSEL_FRO_CLK; // Use fro_clk as source for ADC async clock

	  LPC_SWM->PINENABLE0 &= ~(ADC_0|ADC_1|ADC_2|ADC_3|ADC_5|ADC_6|ADC_7|ADC_8|ADC_9|ADC_10|ADC_11); // Fixed pin analog functions enabled
	  LPC_SWM->PINENABLE0 |= (ADC_4); // Movable digital functions enabled

	  LPC_PIN_INT->ISEL |= 0x3;

	  LPC_PIN_INT->IENF |= 0x3;  // active high

	   LPC_PIN_INT->SIENR = 0x3;  // enabled

	  LPC_ADC->TRM &= ~(1<<ADC_VRANGE); // '0' for high voltage

	  LPC_ADC->CTRL = ( (0<<ADC_CALMODE) | (0<<ADC_LPWRMODE) | (1<<ADC_CLKDIV) );

	  current_seqa_ctrl = ((1<<0)|(1<<1)|(1<<3)|(1<<5)|(1<<6)|(1<<9)|(1<<10)|(1<<11))<<ADC_CHANNELS;
	  LPC_ADC->SEQA_CTRL = current_seqa_ctrl;
	  current_seqb_ctrl = ((1<<2)|(1<<8))<<ADC_CHANNELS;
	  LPC_ADC->SEQB_CTRL = current_seqb_ctrl;

	  current_seqa_ctrl |= (NO_TRIGGER << ADC_TRIGGER) | (1 << ADC_SYNCBYPASS) | (0 << ADC_BURST) | (0 << ADC_SINGLESTEP) | (1 << ADC_MODE);
	  current_seqb_ctrl |= (NO_TRIGGER << ADC_TRIGGER) | (1 << ADC_SYNCBYPASS) | (0 << ADC_BURST) | (0 << ADC_SINGLESTEP) | (0 << ADC_LOWPRIO) | (1 << ADC_MODE);



	  LPC_ADC->THR0_LOW = low_thresh;
	  LPC_ADC->THR1_LOW = low_thresh;
	  LPC_ADC->THR0_HIGH = high_thresh;
	  LPC_ADC->THR1_HIGH = high_thresh;

	  LPC_ADC->INTEN = (1<<SEQA_INTEN)|(1<<SEQB_INTEN);


	  // Write the sequence control word with enable bit set for both sequences
	  current_seqa_ctrl |= 1U<<ADC_SEQ_ENA;
	  LPC_ADC->SEQA_CTRL = current_seqa_ctrl;

	  current_seqb_ctrl |= 1U<<ADC_SEQ_ENA;
	  LPC_ADC->SEQB_CTRL = current_seqb_ctrl;

	  LPC_ADC->INTEN |= (1 << 0) | (1 << 1);

	  // Enable ADC interrupts in the NVIC
	  NVIC_EnableIRQ(ADC_SEQA_IRQn);
	  NVIC_EnableIRQ(ADC_SEQB_IRQn);
	  setvbuf(stdout, NULL, _IONBF, 0);

}


void DAC_Init(void) {
	LPC_SYSCON->SYSAHBCLKCTRL0 |= (DAC0|IOCON);
	LPC_SWM->PINENABLE0 &= ~(1<<23);

	LPC_IOCON->DACOUT_PIN|= (0<<IOCON_MODE)|(1<<IOCON_DAC_ENABLE);

	LPC_SYSCON->PDRUNCFG &= ~(DAC0_PD);

	LPC_DAC0->CTRL = 0;
	LPC_DAC0->CR = 0;
}


int main(void) {
  SystemCoreClockUpdate();
  setup_debug_uart();
  setup_general_adc();
  setvbuf(stdout, NULL, _IONBF, 0);
  DAC_Init();
  printf("Test UART: Demarrage...\n\r");
  MRT_Init_Master();

  while(1) {

          if (seqa_handshake) {
              seqa_handshake = false;
          }

          if (seqb_handshake) {
              seqb_handshake = false;

          }
      }

} // end of main

