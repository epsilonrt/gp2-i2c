/*
 * led_test.c
 * 
 * This software is governed by the CeCILL license <http://www.cecill.info>
 */
#include <avrio/led.h>
#include <avrio/delay.h>

int
main (void) {
  
  vLedInit ();

  for (;;) {

    vLedSet (LED_LED1);
    delay_us (320);
    vLedClear(LED_LED1);
    delay_us (9680);
  }
  return 0;
}
