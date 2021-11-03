#include <stdio.h>
#include <string.h>

#include "utility.h"

void changePrintColor(char *color) {
    if (!strncmp(color, "red", 3)) printf("\033[0;31m");
    else if (!strncmp(color, "bold-red", 8)) printf("\033[1;31m");
    else if (!strncmp(color, "green", 5)) printf("\033[0;32m");
    else if (!strncmp(color, "bold-green", 10)) printf("\033[1;32m");
    else if (!strncmp(color, "yellow", 6)) printf("\033[0;33m");
    else if (!strncmp(color, "bold-yellow", 11)) printf("\033[01;33m");
    else if (!strncmp(color, "blue", 4)) printf("\033[0;34m");
    else if (!strncmp(color, "bold-blue", 9)) printf("\033[1;34m");
    else if (!strncmp(color, "magenta", 7)) printf("\033[0;35m");
    else if (!strncmp(color, "bold-magenta", 12)) printf("\033[1;35m");
    else if (!strncmp(color, "cyan", 4)) printf("\033[0;36m");
    else if (!strncmp(color, "bold-cyan", 9)) printf("\033[1;36m");
    else if (!strncmp(color, "white", 5)) printf("\033[0m");
    else printf("\033[0m");
    fflush(stdout);
}