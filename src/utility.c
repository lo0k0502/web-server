#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "utility.h"

#define NPTRS 2     /* initial number of pointers to allocate (must be > 0) */

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

void printSizet(size_t a) {
    printf("%ld\n", a);
}

void printInt(int a) {
    printf("%d\n", a);
}

void printString(char *str) {
    printf("%s\n", str);
}

char getch() {
    char buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if(tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if(tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if(read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if(tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    return buf;
}

char **strsplit (const char *src, const char *delim)
{
    int i = 0, in = 0, nptrs = NPTRS;       /* index, in/out flag, ptr count */
    char **dest = NULL;                     /* ptr-to-ptr to allocate/fill */
    const char *p = src, *ep = p;           /* pointer and end-pointer */

    /* allocate/validate nptrs pointers for dest */
    if (!(dest = malloc (nptrs * sizeof *dest))) {
        perror ("malloc-dest");
        return NULL;
    }
    *dest = NULL;   /* set first pointer as sentinel NULL */

    for (;;) {  /* loop continually until end of src reached */
        if (!*ep || strchr (delim, *ep)) {  /* if at nul-char or delimiter char */
            size_t len = ep - p;            /* get length of token */
            if (in && len) {                /* in-word and chars in token */
                if (i == nptrs - 1) {       /* used pointer == allocated - 1? */
                    /* realloc dest to temporary pointer/validate */
                    void *tmp = realloc (dest, 2 * nptrs * sizeof *dest);
                    if (!tmp) {
                        perror ("realloc-dest");
                        break;  /* don't exit, original dest still valid */
                    }
                    dest = tmp;             /* assign reallocated block to dest */
                    nptrs *= 2;             /* increment allocated pointer count */
                }
                /* allocate/validate storage for token */
                if (!(dest[i] = malloc (len + 1))) {
                    perror ("malloc-dest[i]");
                    break;
                }
                memcpy (dest[i], p, len);   /* copy len chars to storage */
                dest[i++][len] = 0;         /* nul-terminate, advance index */
                dest[i] = NULL;             /* set next pointer NULL */
            }
            if (!*ep)                       /* if at end, break */
                break;
            in = 0;                         /* set in-word flag 0 (false) */
        }
        else {  /* normal word char */
            if (!in)                        /* if not in-word */
                p = ep;                     /* update start to end-pointer */
            in = 1;                         /* set in-word flag 1 (true) */
        }
        ep++;   /* advance to next character */
    }

    return dest;
}

void strcatChar(char *str , const char c) {
    char arr[2] = { c , '\0' };
    strcat(str , arr);
}

char *stringBefore(char *src, const char target) {
    char *res, i;

    res = malloc(strlen(src) * sizeof(char));

    for (i = 0; *src; i++) {
        if (*src == target) {
            break;
        }
        strcatChar(res, *src);
        src++;
    }
    
    return res;
}

char *stringAfter(char *src, const char *target) {
    char *res;

    res = malloc(strlen(src) * sizeof(char));

    strcpy(res, src);

    while (strncmp(res, target, strlen(target))) {
        res++;
    }

    res += strlen(target);

    return res;
}