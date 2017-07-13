/**
 * @file
 * @brief gp2-i2c, firmware capteur gp2-i2c
 *
 * This software is governed by the CeCILL license <http://www.cecill.info>
 */
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <avrio/delay.h>
#include <avrio/mutex.h>
#include <avrio/usislave.h>
#include <util/atomic.h>
#include "config.h"

/* constants ================================================================ */
typedef enum {
  eStateFalling = 0, // LED Capteur Off
  eStateRising, // LED Capteur On avant échantillonage
  eStateSampling // LED Capteur On après échantillonage
} eState;

/* structures =============================================================== */
typedef union {
  uint16_t value;
  uint8_t raw[2];
} uAdcValue;

/* private variables ======================================================== */
static volatile eState state;
static uAdcValue adc_mean; // Valeur moyennée tension en mV
static uint32_t adc_sum;
static xMutex update_mutex = MUTEX_LOCK; // signale une mise à jour du buffer

/* private functions ======================================================== */
//------------------------------------------------------------------------------
// Temps t1: Led On avant échantillonnage
// Réglage Timer pour 280µs
INLINE void
vSetT1 (void) {

  // Set t1
  OCR0A = 70;
  TCCR0B = 3;// N=64 donc T=280µs
  state = eStateRising;
}

//------------------------------------------------------------------------------
// Temps t2: Led On après échantillonnage
// Réglage Timer pour 40µs
INLINE void
vSetT2 (void) {

  OCR0A = 80;
  TCCR0B = 2;// N=8 donc T=40µs
  state = eStateSampling;
}

//------------------------------------------------------------------------------
// Temps t3: Led Off
// Réglage Timer pour 9.6 ms = 9600µs
INLINE void
vSetT3 (void) {

  OCR0A = 150;
  TCCR0B = 5;// N=1024 donc T=9600µs
  state = eStateFalling;
}

//------------------------------------------------------------------------------
// Démarrage du timer pour effectuer une séquence de AVERAGE_TERMS mesures
INLINE void
vMesStart (void) {

  TIMSK |= _BV (OCIE0A);
  vSetT3();
}

//------------------------------------------------------------------------------
// Arrêt mesure
INLINE void
vMesStop (void) {

  TCCR0B = 0;
  TCNT0 = 0;
}

/* internal public functions ================================================ */
void vInit (void);

/* main ===================================================================== */
int
main (void) {

  vInit ();

  for (;;) {

    if (xMutexTryLock (&update_mutex) == 0) { // Le buffer a été mis à jour

      vMesStop();

      // Calcul de la moyenne
      adc_sum = (adc_sum * 5) / AVERAGE_TERMS; // adc_sum en mV

      ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
        adc_mean.value = (uint16_t) adc_sum;
        adc_sum = 0;
      }
#if WD_ENABLE == 0
      vMesStart();
#endif
    }

    vUsiSlavePoll();
  }
  return 0;
}

//------------------------------------------------------------------------------
static void
vRxCallback (const uint8_t *buffer, uint8_t buffer_length) {

  if (buffer_length > 0) {
    
    vUsiSlaveSetTxBufferIndex (buffer[0]);
  }
}

//------------------------------------------------------------------------------
void
vInit (void) {
  uint16_t w;

  wdt_disable(); // On désactive le watchdog au démarrage

  /*
   * LED Capteur connectée à PB1
   * Gestion temporelle par Timer0 en mode normal sous interruption OCIE0A
   */
  DDRB |= _BV (1);

  /*
   * ADC: capteur connecté sur ADC2 - ADC3, Vref = 2.56V sans capa
   * Comme on divise la sortie du capteur par 2, le quantum est de 5mV
   */
  DIDR0 = 0x18;
  ADMUX = 0x96;

  // Démarrage d'une première convertion bidon
  ADCSRA = 0x87 | _BV (ADSC);
  while (ADCSRA & _BV (ADSC))
    ;
  w = ADC;

  // Clear et validation interruption ADC
  ADCSRA |= _BV (ADIF);
  ADCSRA |= _BV (ADIE);

  vUsiSlaveInit (OWN_ADDRESS, 0, (uint8_t *) &adc_mean, sizeof (adc_mean), vRxCallback);

#if WD_ENABLE
  // L'interruption watchdog va déclencher les mesures toutes les 8s
  wdt_enable (WDTO_8S);
  WDTCR |= _BV (WDIE); // on valide le mode interruption du watchdog
#else
  vMesStart();
#endif

  sei();
}

#if WD_ENABLE
//------------------------------------------------------------------------------
// Routine d'interruption du watchdog
ISR (WDT_vect) {

  WDTCR |= _BV (WDIE); // on valide le mode interruption du watchdog
  /*
   * Démmarage du Timer pour t3 (LED Off)
   */
  vMesStart();
}
#endif

//------------------------------------------------------------------------------
// Routine interruption ADC (mesure effectuée)
ISR (ADC_vect) {
  static uint8_t adc_index;

  adc_sum += ADC; // stockage mesure dans buffer
  adc_index++;
  if (adc_index >= AVERAGE_TERMS) {
    // buffer complet
    adc_index = 0; // remise à zéro index
    vMutexUnlock (&update_mutex);
  }
}

//------------------------------------------------------------------------------
// Routine interruption Timer (compare matched, délai écoulé)
// Cette routine cadence le processus de mesure et génère les impulsions LED.
ISR (TIM0_COMPA_vect) {

  vMesStop();

  if (state == eStateRising) {

    //  Sampling ADC
    ADCSRA |= _BV (ADSC);
    vSetT2();
  }
  else {

    // Basculement LED Capteur
    PORTB ^= _BV (1);

    if (state == eStateSampling) {

      vSetT3();
    }
    else { // eStateFalling

      vSetT1();
    }
  }
}
/* ========================================================================== */
