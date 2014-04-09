/**
  ******************************************************************************
  * @file    debug_cmd.c
  * @author  Pong Cpr.E.
  * @version v0.1
  * @date    22-Mar-2014
  * @brief   --
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "debug_cmd.h"
#include "main.h"
#include "strtod.h"

//int fputc(int ch, FILE *f)
//{
//  return ITM_SendChar(ch);
//}

void interpretCMD(volatile char *msg, uint16_t len){
	
	char *msgBuf = (char*) malloc(len+1);	// use to compare
	
  char **argv = (char**) malloc( sizeof(char*) ); // allocate for 1 argv at first
	uint8_t argc;
	uint8_t i, j, k, numOfPtrn;
	
	char text[70];
	
	// copy string to buffer
	for(i=0; i<len; i++)
	{
		msgBuf[i] = msg[i];		
	}
	msgBuf[i] = '\0';
	

  /* split received data into each part */
	for( i=0, j=0, argc=1; i<len; i++ )
	{
		if ( msgBuf[i] == ' ') 	// part divider in msg.
		{
			argv[argc-1] = (char *) malloc( sizeof(char)*((i-j)+1) );
			memset(argv[argc-1], '\0', sizeof(char)*((i-j)+1));       // initialize all '\0'

			for ( k=0; j<i; j++, k++ )
					argv[argc-1][k] = msgBuf[j];
			j++; // for skip a space ' '

			argc++;
			argv = (char **) realloc( argv, sizeof(char*)*argc );   // increase memory allocation
		}
	}
  argv[argc-1] = (char *) malloc( sizeof(char)*((i-j)+1) );
  memset(argv[argc-1], '\0', sizeof(char)*((i-j)+1));
	for ( k=0; j<len; j++, k++ )                            // get the last arg
		argv[argc-1][k] = msgBuf[j];

	
	/* Distinguish cmd and do operation */
	if ( strcmp( argv[0], CMD_GEN_SAG)==0 )
	{
		if ( argc == 4 && isInteger(argv[1]) && isFloat(argv[2]) && isInteger(argv[3]) )
		{
			getCCRTab(atoi(argv[1]))->sagAmp = getCCRTab(atoi(argv[1]))->normAmp*atof(argv[2]);	// initial amplitude
			getCCRTab(atoi(argv[1]))->sagDuration = atoi(argv[3]);									// initial sag duration
						
			// generating the sag table for the given channel
			switch ( atoi(argv[1]) )	
			{
				case 1: 						
					getGenDutyTabCaller(1)->tabType = TabType_Sag;
					getGenDutyTabCaller(1)->callFromUsr = true;						
					getGenDutyTabCaller(1)->isCall = true;					
					break;
				case 2: 
					getGenDutyTabCaller(2)->tabType = TabType_Sag;
					getGenDutyTabCaller(2)->callFromUsr = true;									
					getGenDutyTabCaller(2)->isCall = true;		
					break;
				case 3: 
					getGenDutyTabCaller(3)->tabType = TabType_Sag;	
					getGenDutyTabCaller(3)->callFromUsr = true;								
					getGenDutyTabCaller(3)->isCall = true;	
					break;
				default: 	
					sprintf((char*)text, "Generating sag:\n - Has no Ch. %s!!\n", argv[1]);
					USART_puts(USART1, text);
					// releasing memory allocation
					free(msgBuf);
					for ( i=0; i<argc; i++ )
						free(argv[i]);
					free(argv);
					return;
			}			
		}
		else
			USART_puts(USART1, "Generating sag:\n - Invalid usage!\nUsage: sag [CH] [PERCENTAGE] [DURATION]\n");
	}
	else if ( strcmp(argv[0], CMD_STOP_SAG)==0 )
	{
		if ( argc == 2 && isInteger(argv[1]) )
		{
			if ( stopSag( atoi(argv[1])) )
				sprintf((char*)text, "Stoping sag:\n - Ch %s sag stoped\n", argv[1]);
			else
				sprintf((char*)text, "Stoping sag:\n - Ch %s has already nominal!\n", argv[1]);
						
			USART_puts(USART1, text);
		}
		else
			USART_puts(USART1, "Stoping sag:\n - Invalid usage!\nUsage: stopsag [CH]\n");	
	}
	else if ( strcmp(argv[0], CMD_GEN_PATTERN)==0 )
	{
		if ( argc >= 4 && argc%2 == 0 && isInteger(argv[1]) )
		{			
			numOfPtrn = (argc/2)-1;
			
			// check whether valid input or not
			for ( i=0; i<numOfPtrn; i++ )
			{
				if ( !isFloat(argv[(i+1)*2]) || !isInteger(argv[(i+1)*2+1]) ) 
				{
					USART_puts(USART1, "Generating pattern:\n - Invalid usage!\nUsage: pattern [CH] [PERCENTAGE_1] [DURATION_1] ... [PERCENTAGE_n] [DURATION_n]\n");
					// releasing memory allocation
					free(msgBuf);
					for ( i=0; i<argc; i++ )
						free(argv[i]);
					free(argv);
					return;
				}
			}
			
			getCCRTab(atoi(argv[1]))->numOfPtrn = numOfPtrn;
			
			// clear old pattern amp and duration, if exists
			if ( getCCRTab(atoi(argv[1]))->state == State_Pattern )
			{				
				getCCRTab(atoi(argv[1]))->state = State_StopingPattern;	// preventing change another pattern while update the table
				free( getCCRTab(atoi(argv[1]))->ptrnAmp );
				free( getCCRTab(atoi(argv[1]))->ptrnDuration );	
			}
			
			// allocting new memory of amplitude and duration
			getCCRTab(atoi(argv[1]))->ptrnAmp = (float*) malloc(sizeof(float)*numOfPtrn);
			getCCRTab(atoi(argv[1]))->ptrnDuration = (__IO uint32_t*) malloc(sizeof(uint32_t)*numOfPtrn);	
			
			// assign new amp and duration 
			for ( i=0; i<numOfPtrn; i++ )
			{
				getCCRTab(atoi(argv[1]))->ptrnAmp[i] = getCCRTab(atoi(argv[1]))->normAmp*atof(argv[(i+1)*2]);
				getCCRTab(atoi(argv[1]))->ptrnDuration[i] = atoi(argv[(i+1)*2+1]);
			}
			
			// generating pattern table for the given channel 
			switch ( atoi(argv[1]) )	
			{
				case 1: 
					getGenDutyTabCaller(1)->tabType = TabType_Pattern;
					getGenDutyTabCaller(1)->ptrnIdx = 0;	
					getGenDutyTabCaller(1)->callFromUsr = true;								
					getGenDutyTabCaller(1)->isCall = true;		
					break;
				case 2: 
					getGenDutyTabCaller(2)->tabType = TabType_Pattern;
					getGenDutyTabCaller(2)->ptrnIdx = 0;	
					getGenDutyTabCaller(2)->callFromUsr = true;								
					getGenDutyTabCaller(2)->isCall = true;		
					break;
				case 3: 
					getGenDutyTabCaller(3)->tabType = TabType_Pattern;
					getGenDutyTabCaller(3)->ptrnIdx = 0;	
					getGenDutyTabCaller(3)->callFromUsr = true;								
					getGenDutyTabCaller(3)->isCall = true;	
					break;
				default: 
					sprintf((char*)text, "Generating pattern:\n - Has no Ch. %s!!\n", argv[1]);
					USART_puts(USART1, text);
					// releasing memory allocation
					free(msgBuf);
					for ( i=0; i<argc; i++ )
						free(argv[i]);
					free(argv);
					return;
			}			
		}
		else
			USART_puts(USART1, "Generating pattern:\n - Invalid usage!\nUsage: pattern [CH] [PERCENTAGE_1] [DURATION_1] ... [PERCENTAGE_n] [DURATION_n]\n");
	}
	else if ( strcmp(argv[0], CMD_STOP_PATTERN)==0 )
	{
		if ( argc == 2 && isInteger(argv[1]) )
		{			
			if ( stopPattern( atoi(argv[1])) )
				sprintf((char*)text, "Stoping pattern:\n - Ch %s pattern stoped\n", argv[1]);
			else
				sprintf((char*)text, "Stoping pattern:\n - Ch %s has already nominal!\n", argv[1]);
						
			USART_puts(USART1, text);
		}
		else
			USART_puts(USART1, "Stoping pattern:\n - Invalid usage!\nUsage: stoppattern [CH]\n");
	}
	else if ( strcmp(argv[0], CMD_HELP)==0 )
	{	
		USART_puts(USART1, "--- Help ---\nCommand list\n");
		USART_puts(USART1, " - Generate sag: sag [CH] [PERCENTAGE] [DURATION]\n");
		USART_puts(USART1, " - Generate pattern: pattern [CH] [PERCENTAGE_1] [DURATION_1] ... [PERCENTAGE_n] [DURATION_n]\n");
		USART_puts(USART1, " - Stop sag: stopsag [CH]\n");
		USART_puts(USART1, " - Stop pattern: stoppattern [CH]\n");
	}
	else
	{
		sprintf((char*)text, "Invalid command!!\n - Not have cmd : %s\n - Type cmd help for help\n", argv[0]);	
		USART_puts(USART1, text);
	}
	
	// releasing memory allocation
	free(msgBuf);
	for ( i=0; i<argc; i++ )
		free(argv[i]);
	free(argv);
}



bool isInteger(const char * s)
{   
	uint16_t num = atoi(s);
	return !(num == 0 && s[0] != '0');
}

bool isFloat(const char * s)
{   
	float num = atof(s);
	return !(num == 0 && s[0] != '0');
}

