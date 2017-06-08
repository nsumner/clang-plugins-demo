
/*
 * These examples come from "How to write a checker in 24 hours" from
 * Anna Zaks and Jordan Rose, available at:
 *   http://llvm.org/devmtg/2012-11/Zaks-Rose-Checker24Hours.pdf
 *
 */


#include <stdio.h>


void
writeCharToLog(char* data) {
  FILE* file = fopen("mylog.txt", "w");
  if (file != NULL) {
    if (!data) {
      return;
    }
    fputc(*data, file);
    fclose(file);
  }
  return;
}


void
checkDoubleClose(int* data) {
  FILE* file = fopen("myfile.txt", "w");

  if (!data) {
    fclose(file);
  } else {
    fputc(*data, file);
  }
  fclose(file);
}


int
checkLeak(int* data) {
  FILE* file = fopen("myfile.txt", "w");
  fputc(*data, file);
  return *data;
}


