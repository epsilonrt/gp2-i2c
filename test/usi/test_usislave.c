/*
 * led_usislave.c
 * Test esclave USI
 * 
 * This software is governed by the CeCILL license <http://www.cecill.info>
 */
#include <avrio/usislave.h>

/* constants ================================================================ */
#define DEVICE_ID (0x46)

/* private variables ======================================================== */
static uint8_t txbuffer[2] = { 0x34, 0x12 };

/* private functions ======================================================== */
static void
vRxCallback (const uint8_t *buffer, uint8_t buffer_length) {

  if (buffer_length > 0) { //we're getting an instruction from master
      txbuffer[0]++;
  }
}

/* main ===================================================================== */
int
main (void) {

  vUsiSlaveInit (DEVICE_ID, 0, txbuffer, 2, vRxCallback);
  sei();

  for (;;) {
    
    vUsiSlavePoll();
  }
  return 0;
}
/* ========================================================================== */
