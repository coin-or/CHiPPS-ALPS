

/* include the COIN-OR-wide system specific configure header */
#include "configall_system.h"

/* include the public project specific macros */
#include "config_alps_default.h"

/***************************************************************************/
/*             HERE DEFINE THE PROJECT SPECIFIC MACROS                     */
/*    These are only in effect in a setting that doesn't use configure     */
/***************************************************************************/

/* Define to the debug sanity check level (0 is no test) */
#define COIN_ALPS_CHECKLEVEL 0

/* Define to the debug verbosity level (0 is no output) */
#define COIN_ALPS_VERBOSITY 0

/* Define to 1 if the CoinUtils package is used */
#define ALPS_HAS_COINUTILS 1

/* Define to 1 if the Clp package is used */
#define ALPS_HAS_CLP 1
