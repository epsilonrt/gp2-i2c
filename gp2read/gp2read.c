/**
 * @file
 * @brief g2pread, lecture capteur gp2-i2c
 *  g2pread -h for help, ie:

    ./g2pread -c 4
    Press Ctrl+C to abort ...
    1495359654,383,0x17F
    1495359665,400,0x190
    1495359676,379,0x17B
    1495359687,375,0x177

    --- chip 0x46 read statistics on /dev/i2c-1 ---
    4 request transmitted, 4 received, 0 errors, 0.0% request loss

 *
 * This software is governed by the CeCILL license <http://www.cecill.info>
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <getopt.h>
#include <limits.h>
#include <time.h>

#include <sysio/delay.h>
#include <sysio/i2c.h>
#include <sysio/log.h>
#include <sysio/string.h>
#include "version-git.h"

/* constants ================================================================ */
#define AUTHORS "epsilonRT"
#define WEBSITE "http://www.epsilonrt.fr/sysio"
#define DEFAULT_INTERVAL 11000 // délai en ms entre 2 lectures (11 nombre premier)
#define DEFAULT_CHIPADDR 0x46  // adresse i2c par défaut du capteur i2c

/* structures =============================================================== */
typedef struct statistics {

  int iTxCount, iRxCount, iErrorCount;
} statistics;

/* private variables ======================================================== */
const char sDefaultBus[] = "/dev/i2c-1";
const char * bus = sDefaultBus;
/* private variables ======================================================== */
static int fd;
/* statistiques */
static statistics stats;
static int slave = DEFAULT_CHIPADDR;
static bool verbose = true;

/* internal public functions ================================================ */
void vVersion (void);
void vHelp (FILE *stream, int exit_msg);
void vStatistics (void);
void vSigIntHandler (int sig);

/* main ===================================================================== */
int
main (int argc, char **argv) {
  int interval = DEFAULT_INTERVAL;
  bool rewind = false;

  const char *short_options = "b:c:i:wqhv";
  static const struct option long_options[] = {
    {"bus",  required_argument, NULL, 'b'},
    {"count",  required_argument, NULL, 'c'},
    {"interval",  required_argument, NULL, 'i'},
    {"quiet",  no_argument, NULL, 'q'},
    {"wr",  no_argument, NULL, 'w'},
    {"help",  no_argument, NULL, 'h'},
    {"version",  no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0} /* End of array need by getopt_long do not delete it*/
  };

  uint16_t voltage;
  int i;
  long n;
  int count = -1;

  do {
    i = getopt_long (argc, argv, short_options, long_options, NULL);

    switch (i) {

      case 'b':
        bus = optarg;
        break;

      case 'c':
        if (iStrToLong (optarg, &n, 0) < 0) {

          vHelp (stderr, EXIT_FAILURE);
        }
        if ( (n <= 0) || (n > INT_MAX) ) {

          vHelp (stderr, EXIT_FAILURE);
        }
        count = (int) n;
        break;

      case 'i':
        if (iStrToLong (optarg, &n, 0) < 0) {

          vHelp (stderr, EXIT_FAILURE);
        }
        interval = (int) n;
        break;

      case 'w':
        rewind = true;
        break;

      case 'q':
        verbose = false;
        break;

      case 'h':
        vHelp (stdout, EXIT_SUCCESS);
        break;

      case 'v':
        vVersion();
        break;

      case '?': /* An invalide option has been used,
        print help an exit with code EXIT_FAILURE */
        vHelp (stderr, EXIT_FAILURE);
        break;
    }
  }
  while (i != -1);

  if (optind < argc)    {

    if (iStrToLong (argv[optind], &n, 0) < 0) {

      vHelp (stderr, EXIT_FAILURE);
    }

    if ( (n < 0x03) || (n > 0x77) ) {

      vHelp (stderr, EXIT_FAILURE);
    }

    slave = (int) n;
  }

  fd = iI2cOpen (bus, slave);
  if (fd < 0) {

    if (verbose) {
      perror ("Failed to open i2c ! ");
    }
    exit (EXIT_FAILURE);
  }

  // vSigIntHandler() intercepte le CTRL+C
  signal (SIGINT, vSigIntHandler);
  signal (SIGTERM, vSigIntHandler);

  if ( (count != 1) && (verbose) ) {
    fprintf (stderr, "Press Ctrl+C to abort ...\n");
  }

  while (count != 0) {
    time_t t;

    t = time (NULL);

    stats.iTxCount++;
    if (rewind) {

      i = iI2cReadRegBlock (fd, 0, (uint8_t *) &voltage, 2);
    }
    else {

      i = iI2cReadBlock (fd, (uint8_t *) &voltage, 2);
    }
    if (i < 0) {

      if (verbose) {
        perror ("Unable to read i2c ! ");
      }
      continue;
    }
    stats.iRxCount++;

    if (voltage > 5000) {

      if (verbose) {
        fprintf (stderr, "Bad value: %d (0x%X)\n", voltage, voltage);
      }
      stats.iErrorCount++;
      continue;
    }

    if (verbose) {

      printf ("%ld,%d,0x%X\n", t, voltage, voltage);
    }
    else {

      printf ("%d\n", voltage);
    }
    fflush (stdout);
    if (count > 0) {
      count--;
    }
    
    if (count) {
      
        delay_ms (interval);
    }
  }

  vSigIntHandler (SIGTERM);

  return 0;
}

// -----------------------------------------------------------------------------
void
vStatistics (void) {

  if ( (stats.iTxCount > 1) && (verbose) ) {
    fprintf (stderr, "\n--- chip 0x%02X read statistics on %s ---\n", slave, bus);
    fprintf (stderr, "%d request transmitted, %d received, %d errors, "
             "%.1f%% request loss\n",
             stats.iTxCount,
             stats.iRxCount,
             stats.iErrorCount,
             (double) (stats.iTxCount - stats.iRxCount) * 100.0 /
             (double) stats.iTxCount);
  }
}

// -----------------------------------------------------------------------------
void
vVersion (void)  {
  fprintf (stderr, "you are running version %s\n", VERSION_SHORT);
  fprintf (stderr, "this program was developped by %s.\n", AUTHORS);
  fprintf (stderr, "you can find some information on this project page at %s\n", WEBSITE);
  exit (EXIT_SUCCESS);
}

// -----------------------------------------------------------------------------
void
vHelp (FILE *stream, int exit_msg) {

  if ( (exit_msg == EXIT_SUCCESS) || ( (exit_msg == EXIT_FAILURE) && (verbose) ) ) {
    fprintf (stream, "usage : %s [ options ] [ chip-address ] [ options ]\n\n", __progname);
    fprintf (stream,
             //01234567890123456789012345678901234567890123456789012345678901234567890123456789
             "Read gp2-i2c sensor voltage in mV.\n"
             "  chip-address specifies the address of the chip on that bus, and is an integer\n"
             "  between 0x03 and 0x77. The default is 0x%02X\n", DEFAULT_CHIPADDR);
    fprintf (stream, "valid options are :\n");
    fprintf (stream,
             //01234567890123456789012345678901234567890123456789012345678901234567890123456789
             "  -b  --bus       \tSpecifies the i2c bus.\n"
             "                  \tThe default is %s.\n"
             "  -c  --count     \tStop after sending count resquets.\n"
             "                  \tThe default is infinite.\n"
             "  -i  --interval  \tWait interval milliseconds between sending each packet.\n"
             "                  \tThe default is %d ms.\n"
             "  -w  --wr        \tRead with a Write-Read I2c sequence\n"
             "                  \t(StartW, Write reg-addr (0), ReStartR, Read 2 bytes, Stop\n"
             "  -q  --quiet     \tPrint only voltage in mV (No error and no stats reported)\n"
             "  -h  --help      \tPrint this message\n"
             "  -v  --version   \tPrint version and exit\n"
             , sDefaultBus
             , DEFAULT_INTERVAL);
  }
  exit (exit_msg);
}

// -----------------------------------------------------------------------------
void
vSigIntHandler (int sig) {

  if ( (sig == SIGINT) || (sig == SIGTERM) ) {
    vStatistics();
    if (iI2cClose (fd) != 0) {
      perror ("iI2cClose");
      exit (EXIT_FAILURE);
    }
    if ( (sig == SIGINT) && (verbose) ) {
      fprintf (stderr, "\neverything was closed.\nHave a nice day !\n");
    }
    exit (EXIT_SUCCESS);
  }
}

/* ========================================================================== */
