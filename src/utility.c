#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "utility.h"

#define NPTRS 2     // initial number of pointers to allocate (must be > 0)

// change terminal print color
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

// Print a size_t type variable
void printSizet(size_t a) {
    printf("%ld\n", a);
}

// Print an integer
void printInt(int a) {
    printf("%d\n", a);
}

// Print a string (char[], char *)
void printString(char *str) {
    printf("%s\n", str);
}

// Scan a char immediately
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

// split a string into an array
char **strsplit(const char *src, const char *delim)
{
    int i = 0, in = 0, nptrs = NPTRS; // index, in/out flag, ptr count
    char **result = NULL; // ptr-to-ptr to allocate/fill
    const char *ptr = src, *eptr = ptr; // pointer and end-pointer
    size_t len; // length of token

    // allocate/validate nptrs pointers for result
    if (!(result = malloc(nptrs * sizeof *result))) {
        perror("malloc-result");
        exit(1);
    }

    *result = NULL; // set first pointer as sentinel NULL

    for (;;) {
        if (!*eptr || strchr(delim, *eptr)) {
            // if at nul-char or delimiter char
            len = eptr - ptr;
            if (in && len) {
                // in-word and chars in token
                if (i == nptrs - 1) {
                    // if used pointer == allocated - 1

                    // realloc result to temporary pointer/validate
                    void *tmp = realloc(result, 2 * nptrs * sizeof *result);
                    if (!tmp) {
                        perror("realloc-result");
                        break; // don't exit, original result still valid
                    }
                    result = tmp; // assign reallocated block to result
                    nptrs *= 2; // increment allocated pointer count
                }
                
                // allocate/validate storage for token
                if (!(result[i] = malloc(len + 1))) {
                    perror("malloc-result[i]");
                    break;
                }

                memcpy(result[i], ptr, len); // copy len chars to storage
                result[i++][len] = 0; // nul-terminate, advance index
                result[i] = NULL; // set next pointer NULL
            }
            if (!*eptr) {
                // if at end, break
                break;
            }

            // set in-word flag 0 (false)
            in = 0;
        } else {
            // normal word char
            if (!in) {
                // if not in-word, then update start to end-pointer
                ptr = eptr;
            }

            // set in-word flag 1 (true)
            in = 1;
        }

        // advance to next character
        eptr++;
    }

    return result;
}

// concat char to string
void strcatChar(char *str , const char c) {
    char arr[2] = { c , '\0' };
    strcat(str , arr);
}

// return substring before the target char in src string
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

// return substring after the target string in src string
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