#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>

#include <wiringPi.h>
#include <wiringSerial.h>
#include <wiringPiSPI.h>

#include <bcm2835.h>

#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include "fileMake.h"
#include "ppam.h"
#include "mcp3208_bcm2835.h"
