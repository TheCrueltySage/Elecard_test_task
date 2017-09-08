#define HAVE_GETOPT_LONG

#if defined(HAVE_GETOPT_LONG)
#include <getopt.h>
#elif defined(WIN32) && !defined(HAVE_GETOPT_LONG)
#define HAVE__STRICMP
#define HAVE__STRNICMP
#define ULTRAGETOPT_REPLACE_GETOPT
#include "getopt_extern/getopt.h"
#elif !defined(WIN32) && !defined(HAVE_GETOPT_LONG)
#define ULTRAGETOPT_REPLACE_GETOPT
#include "getopt_extern/getopt.h"
#endif
