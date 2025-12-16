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

const char promptstring[] = "Choose a sequence to convert:\n\ra for sequence a\n\rb for sequence b\n\r";

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

volatile unsigned char temp;

/*****************************************************************************
*****************************************************************************/

// Variables globales
volatile uint32_t counter_20hz = 0;

// Variable globale (doit être accessible)
volatile uint32_t counter_div = 0;

void MRT_Init_Master(void) {
    LPC_SYSCON->SYSAHBCLKCTRL0 |= (1 << 10);
    LPC_SYSCON->PRESETCTRL0 &= ~(1 << 10);
    LPC_SYSCON->PRESETCTRL0 |= (1 << 10);

    // Timer réglé à 40 kHz (une interruption toutes les 25µs)
    LPC_MRT->Channel[0].INTVAL = (SystemCoreClock / 10000) & 0x7FFFFFFF;
    LPC_MRT->Channel[0].CTRL = (1 << 0);
    NVIC_EnableIRQ(MRT_IRQn);
}

void MRT_IRQHandler(void) {
    if (LPC_MRT->Channel[0].STAT & (1 << 0)) {
        LPC_MRT->Channel[0].STAT = (1 << 0); // Acquitter l'interruption
        counter_div++;

        if (counter_div >= 500) {
            counter_div = 0;
            // On prend la config de base ET on ajoute START, sans corrompre la variable globale
            LPC_ADC->SEQB_CTRL = current_seqb_ctrl | (1 << 26);
            LPC_DAC0->CTRL = 0;
            LPC_DAC0->CR = (((seqb_buffer[2] >> 4) & 0xFFF)/4)<<6;
        }
        else {
            LPC_ADC->SEQA_CTRL = current_seqa_ctrl | (1 << 26);
        }
    }
}

void setup_general_adc(){

	  // Step 1. Power up and reset the ADC, and enable clocks to peripherals
	  LPC_SYSCON->PDRUNCFG &= ~(ADC_PD);
	  LPC_SYSCON->PRESETCTRL0 &= (ADC_RST_N);
	  LPC_SYSCON->PRESETCTRL0 |= ~(ADC_RST_N);
	  LPC_SYSCON->SYSAHBCLKCTRL0 |= (ADC | GPIO0 | GPIO_INT | SWM);
	  LPC_SYSCON->ADCCLKDIV = 1;                 // Enable clock, and divide-by-1 at this clock divider
	  LPC_SYSCON->ADCCLKSEL = ADCCLKSEL_FRO_CLK; // Use fro_clk as source for ADC async clock


	  // Step 3. Configure the external pins we will use

	  LPC_SWM->PINENABLE0 &= ~(ADC_0|ADC_1|ADC_2|ADC_3|ADC_5|ADC_6|ADC_7|ADC_8|ADC_9|ADC_10|ADC_11); // Fixed pin analog functions enabled
	  LPC_SWM->PINENABLE0 |= (ADC_4); // Movable digital functions enabled



	  // Assign the ADC pin trigger functions to the input port pins
	  // For LPC82x and LPC83x : ADC pin triggers are movable digital functions assignable through the SWM
	  // For LPC84x, LPC80x : ADC and DAC pin triggers driven by GPIO_INT
	  // Configure pinint0_irq and pinint1_irq for high level sensitive, then assign them in seq_ctrl[14:12]
	  // Important Note! We must configure the pin interrupts to generate the ADC pin triggers, but we don't use the pin interrupts as interrupts.

	  // Configure the pin interrupt mode register (a.k.a ISEL) for level-sensitive on PINTSEL1,0 ('1' = level-sensitive)
	  LPC_PIN_INT->ISEL |= 0x3; // level-sensitive

	  // Configure the active level for PINTSEL1,0 to active-high ('1' = active-high)
	  LPC_PIN_INT->IENF |= 0x3;  // active high

	  // Enable interrupt generation for PINTSEL1,0
	  LPC_PIN_INT->SIENR = 0x3;  // enabled

	  // Step 4. Configure the ADC for the appropriate analog supply voltage using the TRM register
	  // For a sampling rate higher than 1 Msamples/s, VDDA must be higher than 2.7 V (on the Max board it is 3.3 V)
	  LPC_ADC->TRM &= ~(1<<ADC_VRANGE); // '0' for high voltage



	  // Calibration mode = false, low power mode = false, CLKDIV = appropriate for desired sample rate.
	  LPC_ADC->CTRL = ( (0<<ADC_CALMODE) | (0<<ADC_LPWRMODE) | (1<<ADC_CLKDIV) );

	  // Step 5. Assign some ADC channels to each sequence
	  current_seqa_ctrl = ((1<<0)|(1<<1)|(1<<3)|(1<<5)|(1<<6)|(1<<8)|(1<<9)|(1<<10)|(1<<11))<<ADC_CHANNELS;
	  LPC_ADC->SEQA_CTRL = current_seqa_ctrl; // Appliquer immédiatement
	  current_seqb_ctrl = ((1<<2)|(1<<8))<<ADC_CHANNELS;
	  LPC_ADC->SEQB_CTRL = current_seqb_ctrl; // Appliquer immédiatement

	  // Step 6. Select a trigger source for each of the sequences
	  // Let sequence A trigger = PININT0_IRQ. Connected to an external pin using PINTSELs above.
	  // Let sequence B trigger = PININT1_IRQ. Connected to an external pin using PINTSELs above.

	  current_seqa_ctrl |= NO_TRIGGER<<ADC_TRIGGER;
	  current_seqb_ctrl |= NO_TRIGGER<<ADC_TRIGGER;


	  // Step 8. Choose (1) to bypass, or (0) not to bypass the hardware trigger synchronization
	  // Since our trigger sources are on-chip, system clock based, we may bypass.
	  current_seqa_ctrl |= 1<<ADC_SYNCBYPASS;
	  current_seqb_ctrl |= 1<<ADC_SYNCBYPASS;

	  // Step 9. Choose burst mode (1) or no burst mode (0) for each of the sequences
	  // Let sequences A and B use no burst mode
	  current_seqa_ctrl |= 0<<ADC_BURST;
	  current_seqb_ctrl |= 0<<ADC_BURST;

	  // Step 10. Choose single step (1), or not (0), for each sequence.
	  // Let sequences A and B use no single step
	  current_seqa_ctrl |= 0<<ADC_SINGLESTEP;
	  current_seqb_ctrl |= 0<<ADC_SINGLESTEP;

	  // Step 11. Choose A/B sequence priority
	  // Use default
	  current_seqa_ctrl |= 0<<ADC_LOWPRIO;

	  // Step 12. Choose end-of-sequence mode (1), or end-of-conversion mode (0), for each sequence
	  // Let sequences A and B use end-of-sequence mode
	  current_seqa_ctrl |= 1<<ADC_MODE;
	  current_seqb_ctrl |= 1<<ADC_MODE;

	  // Step 13. Set the low and high thresholds for both sets of threshold compare registers
	  // Note: This step is included for completeness, threshold crossings are not currently used in this example.
	  // Let both low thresholds be (1/3)*0xfff
	  // Let both high thresholds be (2/3)*0xfff
	  #define low_thresh (0xFFF / 3)
	  #define high_thresh ((2 * 0xFFF) / 3)
	  LPC_ADC->THR0_LOW = low_thresh;
	  LPC_ADC->THR1_LOW = low_thresh;
	  LPC_ADC->THR0_HIGH = high_thresh;
	  LPC_ADC->THR1_HIGH = high_thresh;

	  // Step 14. For each channel, choose which pair of threshold registers (0 or 1) conversion results should be compared against.
	  // Note: This step is included for completeness, threshold crossings are not currently used in this example.
	  // For sequence A (channels 1 - 6), compare against threshold register set 0
	  // For sequence B (channels 7, 9, 10, 11), compare against threshold register set 1
	  LPC_ADC->CHAN_THRSEL = (0<<1)|(0<<2)|(0<<3)|(0<<4)|(0<<5)|(0<<6)|(1<<7)|(1<<9)|(1<<10)|(1<<11);

	  // Step 15. Choose which interrupt conditions will contribute to the ADC interrupt/DMA triggers
	  // Only the sequence A and sequence B interrupts are enabled for this example
	  // SEQA_INTEN = true
	  // SEQB_INTEN = true
	  // OVR_INTEN = false
	  // ADCMPINTEN0 to ADCMPEN11 = false
	  LPC_ADC->INTEN = (1<<SEQA_INTEN)  |
	                   (1<<SEQB_INTEN)  |
	                   (0<<OVR_INTEN)   |
	                   (0<<ADCMPINTEN0) |
	                   (0<<ADCMPINTEN1) |
	                   (0<<ADCMPINTEN2) |
	                   (0<<ADCMPINTEN3) |
	                   (0<<ADCMPINTEN4) |
	                   (0<<ADCMPINTEN5) |
	                   (0<<ADCMPINTEN6) |
	                   (0<<ADCMPINTEN7) |
	                   (0<<ADCMPINTEN8) |
	                   (0<<ADCMPINTEN9) |
	                   (0<<ADCMPINTEN10)|
	                   (0<<ADCMPINTEN11);


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

	// 2. Allumer le DAC
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

  // The main loop
  while(1) {
          // --- Gestion Séquence A (Rapide) ---
          // On évite d'afficher 40000 fois par seconde, on affiche juste de temps en temps
          // ou on fait du traitement de signal ici.
          if (seqa_handshake) {
              seqa_handshake = false;
          }

          // --- Gestion Séquence B (Lente - 20Hz) ---
          if (seqb_handshake) {
              seqb_handshake = false;

              for (int n=0; n<12; n++) {
                   // Vérifie si le canal est actif dans seqB
                   if ((current_seqb_ctrl >> n) & 0x1) {
                       uint32_t raw = (seqb_buffer[n] >> 4) & 0xFFF;
                       printf("Ch%d:%d%%\n\r", n, (raw * 100)/4095);
                   }
              }
          }
      }

} // end of main

