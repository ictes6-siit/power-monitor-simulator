/**
  ******************************************************************************
  * @file    debug_cmd.h
  * @author  Pong Cpr.E.
  * @version v0.1
  * @date    22-Mar-2014
  * @brief   Header file for debug_cmd.c
  ******************************************************************************
  */
	
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DEBUG_CMD_H
#define __DEBUG_CMD_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stdbool.h"


/* CMD list define -----------------------------------------------------------*/
#define CMD_HELP 						"help"
#define CMD_STATUS 					"status"
#define CMD_GEN_SAG 				"sag"
// #define CMD_GEN_SWELL 		"swell"
#define CMD_STOP_SAG 				"stopsag"
#define CMD_GEN_PATTERN 		"pattern"
#define CMD_STOP_PATTERN 		"stoppattern"


/* Function prototype define -------------------------------------------------*/
void interpretCMD(volatile char *msg, uint16_t len);
bool isInteger(const char * s);
bool isFloat(const char * s);

#endif // __DEBUG_CMD_H
