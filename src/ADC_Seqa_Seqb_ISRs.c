/*
 * Example_ADC_Seqa_Seqb_ISRs.c
 *
 *  Created on: August 19, 2016
 *      Author: 
 */

#include "LPC8xx.h"
#include "adc.h"
#include <stdio.h>


extern volatile uint32_t seqa_buffer[12];
extern volatile uint32_t seqb_buffer[12];
extern volatile enum {false, true} seqa_handshake, seqb_handshake;
extern uint32_t current_seqa_ctrl, current_seqb_ctrl;



// ============================================================================
// ADC Sequence A ISR
// Reads the data registers for the ADC channels assigned to sequence A
// and stores the contents in an array in SRAM. Then sets a handshake flag
// for main() and returns.
// ============================================================================
void ADC_SEQA_IRQHandler(void) {
  uint32_t n,val;
  val = current_seqa_ctrl;
  // For each channel assigned to this sequence, store the data in the buffer (not packed)
  for (n = 0; n < 12; n++) {
    if ((val>>n) & 0x1) {               // If set, store this channel's data in the SRAM array
      seqa_buffer[n] = LPC_ADC->DAT[n];
    }
  }

  LPC_ADC->FLAGS |= 1<<ADC_SEQA_INT;

  // Set the handshake flag for main
  seqa_handshake = true;

  // Return from interrupt
  return;
}



// ============================================================================
// ADC Sequence B ISR
// Reads the data registers for the ADC channels assigned to sequence B
// and stores the contents in an array in SRAM. Then sets a handshake flag
// for main() and returns.
// ============================================================================
void ADC_SEQB_IRQHandler(void) {
  seqb_buffer[2] = LPC_ADC->DAT[2];
  seqb_buffer[8] = LPC_ADC->DAT[8];

  LPC_ADC->FLAGS |=1<<ADC_SEQB_INT;

  seqb_handshake = true;

  return;
}



// ============================================================================
// ADC Threshold Compare ISR
// ============================================================================
void ADC_THCMP_IRQHandler(void) {
  while (1);                      // Add handler code here if desired
}



// ============================================================================
// ADC Overrun ISR
// ============================================================================
void ADC_OVR_IRQHandler(void) {
  while(1);                       // Add handler code here if desired
}
