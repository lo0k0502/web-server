#ifndef UTILITY_H
#define UTILITY_H

void changePrintColor(char *color);
char getch();
char **strsplit(const char *src, const char *delim);
void strcatChar(char *str , const char c);
char *stringBefore(char *src, const char target);
char *stringAfter(char *src, const char *target);

#endif