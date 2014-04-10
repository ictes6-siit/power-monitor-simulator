/**
  ******************************************************************************
  * @file    main.h
  * @author  Pong Cpr.E.
  * @version v1.0
  * @date    07-Feb-2013
  * @brief   Header file for main.c
  ******************************************************************************
  */
	
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx.h"
#include "Peripheral_Config.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_adc.h"
#include "stdio.h"
#include "misc.h"
#include "stdbool.h"
#include "math.h"


/* Private define ------------------------------------------------------------*/
#define PI 3.14159265358979323

#define ON	1
#define OFF	0
#define GPIO_BitSet(A, B)		(A |= B)
#define GPIO_BitRes(A, B)		(A &= ~B)
#define GPIO_BitToggle(A, B)(A ^= B)
#define GPIO_ReadPin(A,B)		(A &= B)
#define USER_BUTTON	GPIO_Pin_0;
#define Green		GPIO_Pin_12
#define Orange	GPIO_Pin_13
#define Red			GPIO_Pin_14
#define Blue		GPIO_Pin_15
#define LED(A,B)	(B==ON) ? GPIO_BitSet(GPIOD->ODR, A) : GPIO_BitRes(GPIOD->ODR, A)

static const uint16_t SAMPLER = 1000;
static const float _2PI_BY_SAMPLER = 2*PI/(float)SAMPLER;

/** 
	*	\brief  Dutycycle table type type enumeration.
  */
typedef enum
{
	TabType_Norminal,
	TabType_Sag,
	TabType_Pattern
} CCRTabType_Type;

/** 
	*	\brief  Dutycycle table state type enumeration.
  */
typedef enum
{
	State_Norminal,
	State_Sag,
	State_StopingSag,
	State_Pattern,
	State_StopingPattern
} CCRTabState_Type;


typedef struct
{
	__IO bool isCall;
	CCRTabType_Type tabType;
	uint8_t ptrnIdx;
	bool callFromUsr;
} CallingGenDutyTab_Type;


/** 
  * \brief  Delay Unit on Systick delay type enumeration 
  */  
typedef enum 
{ 
//	DelayUnit_us,	
	DelayUnit_ms,	
	DelayUnit_s
}	DelayUnit_Type;


/** 
	*	\brief  Structure type to create or config dutycycle table.
  */
typedef struct
{
	uint16_t table[2][SAMPLER];	//	CCR value tables
	uint8_t crrTab;							// current table that using to generate PWM
	uint16_t min;
	uint16_t max;
	uint8_t numOfPtrn;					// number of pattern
	uint8_t crrPtrn;						// current pattern index
	float sin_table[SAMPLER];
	float offset;
	float normAmp;				
	float sagAmp;				
	float *ptrnAmp;							// array of pattern amplitude
	__IO uint32_t *ptrnDuration;
	__IO uint32_t ptrnDrtnCnt;
	__IO uint32_t sagDuration;
	CCRTabState_Type state;
} CCRTab_Type;


static __IO uint32_t TimingDelay;
static __IO uint32_t UsrBtn_Debnce_cnt;


/* Function prototypes ----------------------------------------------------------*/

void init(void);	// the centre of all initializations.

/*--------------- USART ---------------------------------------------------------*/
void init_USART1(uint32_t baudrate);
void USART_puts(USART_TypeDef* USARTx, volatile char *s);
void USART_print(USART_TypeDef* USARTx, volatile char *s, uint16_t len);

/*--------------- EXTI ----------------------------------------------------------*/
void initExtInterrupt(void);	// user button

/*--------------- TIMER ---------------------------------------------------------*/
void TIM2_Config(void);
void TIM3_Config(void);
void TIM3_PWM_Config(uint16_t period);

/*--------------- GENERAL -------------------------------------------------------*/
void initSysTickClk(void);
void TimingDelay_Decrement(void);
void Delay(__IO uint32_t nTime, DelayUnit_Type unit);

void CCRTab_init( CCRTab_Type *CCRTab, uint16_t min, uint16_t max, float amp, float offset, float startDeg );
uint8_t genDutyTab(CCRTab_Type *CCRTab, uint8_t ptrnIdx , CCRTabType_Type tabType);
CCRTab_Type* getCCRTab(uint8_t channel);
CallingGenDutyTab_Type* getGenDutyTabCaller(uint8_t channel);
void genDutyTabPolling(void);	// for main loop call
bool stopSag( uint8_t channel );
bool stopPattern( uint8_t channel );
void sagTimer(void);


#endif // __MAIN_H
