/**
  ******************************************************************************
  * @file    strtod.h
  * @author  Pong Cpr.E.
  * @version v1.0
  * @date    05-Apr-2014
  * @brief   Header file for strtod.c
  ******************************************************************************
  */
	
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STRTOD_H
#define __STRTOD_H

#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>


/* Function prototype define --------------------------------------------------*/
double strtod(const char *str, char **endptr);
float strtof(const char *str, char **endptr);
long double strtold(const char *str, char **endptr);
double atof(const char *str);

#endif // __STRTOD_H
