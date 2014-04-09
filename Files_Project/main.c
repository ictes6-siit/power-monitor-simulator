/**
  ******************************************************************************
  * @file    main.c
  * @author  Pong Cpr.E.
  * @version v1.1
  * @date    07-Feb-2013
  * @brief   Generate PWM for sim. power sag
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "debug_cmd.h"


#define MAX_USART1_STRLEN 100 //maximum USART1 string length in characters (Debug)
volatile char usart1_received_string[MAX_USART1_STRLEN+1]; // this will hold the recieved string
uint16_t usart1_str_cnt = 0; // this counter is used to determine the string length

const uint16_t TIM3_period = 1679; // 1679 for 1000x (50kHz)
uint16_t TIM3_CCR_val_idx = 0;

CCRTab_Type CCR1_tab;
CCRTab_Type CCR2_tab;
CCRTab_Type CCR3_tab;
__IO uint32_t CCR1_SagPtrnDrtnCnt = 0;	//CCR1 Pattern Sag Duration Count
__IO uint32_t CCR2_SagPtrnDrtnCnt = 0;
__IO uint32_t CCR3_SagPtrnDrtnCnt = 0;

CallingGenDutyTab_Type CCR1_CallGDT = { false, TabType_Norminal, 0, false };
CallingGenDutyTab_Type CCR2_CallGDT = { false, TabType_Norminal, 0, false };
CallingGenDutyTab_Type CCR3_CallGDT = { false, TabType_Norminal, 0, false };

uint8_t genDutyTab_result = 0;
char sendingMsg[70] = {'\0'};

//int fputc(int ch, FILE *f)
//{
//  return ITM_SendChar(ch);
//}


/*--------------- main function ------------------------------------------------------------*/
int main(void) 
{		
	
	// initial Duty-cycle table for the 3 phases
	CCRTab_init( &CCR1_tab, 0, TIM3_period, TIM3_period/2.0f, TIM3_period/2.0f, 0 );
	CCRTab_init( &CCR2_tab, 0, TIM3_period, TIM3_period/2.0f, TIM3_period/2.0f, 120*PI/180 );
	CCRTab_init( &CCR3_tab, 0, TIM3_period, TIM3_period/2.0f, TIM3_period/2.0f, 240*PI/180 );
		
	// init systems
	init();	
	
	USART_puts( USART1, "Initiated\n" );

  while (1)
	{
		genDutyTabPolling();		
	}
}

void init() 
{	
	Peripheral_MCU_Config();
	
	initExtInterrupt();		//USR BTN
	initSysTickClk();
	init_USART1(115200); 		//PB6 TX, PB7 RX	 (debug port)
					
	TIM3_Config();				
	TIM3_PWM_Config(TIM3_period);		
}


/*--------------- USART -----------------------------------------------------------------------------------*/
/**
  * @brief  used to transmit a string of characters via the USART specified in USARTx.
  * @param  USARTx: can be any of the USARTs e.g. USART1, USART2 etc.
	* 				*s: the string you want to send.
  * @retval None
  * Note: The string has to be passed to the function as a pointer because
  * 		 the compiler doesn't know the 'string' data type. In standard
  * 		 C a string is just an array of characters
  * 
  * Note 2: At the moment it takes a volatile char because the received_string variable
  * 		   declared as volatile char --> otherwise the compiler will spit out warnings
  */
void USART_puts(USART_TypeDef* USARTx, volatile char *s)
{
	
	while(*s){
		// wait until data register is empty
		while( !(USARTx->SR & 0x00000040) ); 
		USART_SendData(USARTx, *s);
		*s++;
	}
}

/**
  * @brief  used to transmit a string of characters via the USART specified in USARTx.
  * @param  USARTx: can be any of the USARTs e.g. USART1, USART2 etc.
	* 				*s: the string you want to send.
	* 				len: length of the string.
  */
void USART_print(USART_TypeDef* USARTx, volatile char *s, uint16_t len)
{
	
	while(len > 0){
		// wait until data register is empty
		while( !(USARTx->SR & 0x00000040) ); 
		USART_SendData(USARTx, *s);
		*s++;
		len--;
	}
}


/**
  * @brief  initializes the USART1 peripheral (debug usart).
  * @param  baudrate: the baudrate at which the USART is supposed to operate.
  * @retval None
  */
void init_USART1(uint32_t baudrate)
{
	
	GPIO_InitTypeDef GPIO_InitStructure; // this is for the GPIO pins used as TX and RX
	USART_InitTypeDef USART_InitStructure; // this is for the USART1 initilization
	NVIC_InitTypeDef NVIC_InitStructure; // this is used to configure the NVIC (nested vector interrupt controller)
	
	/* enable APB1 peripheral clock for USART1 
	 * note that only USART1 and USART6 are connected to APB2
	 * the other USARTs are connected to APB1
	 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	/* enable the peripheral clock for the pins used by 
	 * USART1, PB6 for TX and PB7 for RX
	 */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	/* This sequence sets up the TX and RX pins 
	 * so they work correctly with the USART1 peripheral
	 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // Pins 6 (TX) and 7 (RX) are used
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 			// the pins are configured as alternate function so the USART peripheral has access to them
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		// this defines the IO speed and has nothing to do with the baudrate!
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;			// this defines the output type as push pull mode (as opposed to open drain)
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;			// this activates the pullup resistors on the IO pins
	GPIO_Init(GPIOB, &GPIO_InitStructure);					// now all the values are passed to the GPIO_Init() function which sets the GPIO registers
	
	/* The RX and TX pins are now connected to their AF
	 * so that the USART1 can take over control of the 
	 * pins
	 */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);
	
	/* Now the USART_InitStruct is used to define the 
	 * properties of USART1 
	 */
	USART_InitStructure.USART_BaudRate = baudrate;			// the baudrate
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;// we want the data frame size to be 8 bits (standard)
	USART_InitStructure.USART_StopBits = USART_StopBits_1;		// we want 1 stop bit (standard)
	USART_InitStructure.USART_Parity = USART_Parity_No;		// we don't want a parity bit (standard)
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
	USART_Init(USART1, &USART_InitStructure);					// again all the properties are passed to the USART_Init function which takes care of all the bit setting
	
	
	/* Here the USART1 receive interrupt is enabled
	 * and the interrupt controller is configured 
	 * to jump to the USART1_IRQHandler() function
	 * if the USART1 receive interrupt occurs
	 */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // enable the USART1 receive interrupt 
	
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		 // we want to configure the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x12;// this sets the priority group of the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;		 // this sets the subpriority inside the group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&NVIC_InitStructure);							 // the properties are passed to the NVIC_Init function which takes care of the low level stuff	

	// finally this enables the complete USART1 peripheral
	USART_Cmd(USART1, ENABLE);
}


// this is the interrupt request handler (IRQ) for ALL USART1 interrupts
void USART1_IRQHandler(void)
{	
	// check if the USART1 receive interrupt flag was set
	if( USART_GetITStatus(USART1, USART_IT_RXNE) )
	{		
		char t = USART1->DR; // the character from the USART1 data register is saved in t
		
		/* check if the received character is not the LF character (used to determine end of string) 
		 * or the if the maximum string length has been been reached 
		 */
		if ( usart1_str_cnt < MAX_USART1_STRLEN )
		{
			if( t != '\n' )
			{ 
				if ( t == 0x08 ) // backspace
				{
					if ( usart1_str_cnt>0 )
						usart1_str_cnt--;
				}
				else
				{
					usart1_received_string[usart1_str_cnt] = t;
					usart1_str_cnt++;
				}
			}
			else 
			{ // otherwise reset the character counter and print the received string
	//			USART_print(USART1, usart1_received_string, usart1_str_cnt);
				interpretCMD( usart1_received_string, usart1_str_cnt );
				usart1_str_cnt = 0;
			}
		}
		else
		{
			char text[60];
			sprintf((char*)text, "USART buffer full!\t (MAX_INPUT: %d)\nBuffer will be clear\n", MAX_USART1_STRLEN);			
			USART_puts(USART1, text);		
			
			usart1_str_cnt = 0;	
		}
	}
}







/*--------------- EXTI -------------------------------------------------------------------------------------*/
/**
  * @brief  Configures EXTI Line0 (connected to PA0 pin USER_BUTTON) in interrupt mode
  * @param  None
  * @retval None
  */
void initExtInterrupt(void)
{
  
  GPIO_InitTypeDef   GPIO_InitStructure;
  NVIC_InitTypeDef   NVIC_InitStructure;
	EXTI_InitTypeDef   EXTI_InitStructure;

  /* Enable GPIOA clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  /* Enable SYSCFG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = USER_BUTTON;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Connect EXTI Line0 to PA0 pin */
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);

  /* Configure EXTI Line0 */
  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x13;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void initSysTickClk() 
{
	SysTick_Config(SystemCoreClock / 1000);	// tick every 1ms (1 kHz)
  NVIC_SetPriority(SysTick_IRQn, 0x10);
}

/**
  * @brief  This function handles External line 0 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line0) != RESET)
  {
		
		
		CCR1_tab.sagAmp = CCR1_tab.normAmp*0.5;	// amplitude
		CCR1_tab.sagDuration = 1000;						// sag duration
		
		CCR1_CallGDT.tabType = TabType_Sag;
		CCR1_CallGDT.callFromUsr = true;						
		CCR1_CallGDT.isCall = true;					
				
    /* Clear the EXTI line 0 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line0);
  }
}


/*--------------- TIMER ------------------------------------------------------------------------------------*/
/**
  * @brief  Configure the TIM3 Ouput Channels.
  * @param  None
  * @retval None
  */
void TIM3_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  /* GPIOC and GPIOB clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE);
  
  /* GPIOC Configuration: TIM3 CH1 (PA6), TIM3 CH2 (PA7) */ 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
  GPIO_Init(GPIOA, &GPIO_InitStructure); 
  
  /* GPIOA Configuration:  TIM3 CH3 (PB0) */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
  GPIO_Init(GPIOB, &GPIO_InitStructure); 

  /* Connect TIM3 pins to AF2 */  
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_TIM3);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_TIM3); 
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_TIM3);	
}

/**
  * @brief  Setup TIM3 interval timer. Using on USART3 Rx timeout(RFID)
  * @param  None
  * @retval None
  */
void TIM3_PWM_Config(uint16_t period)
{
  NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	TIM_DeInit(TIM3);
	
  /* Enable the TIM3 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = period;
  TIM_TimeBaseStructure.TIM_Prescaler = 0; 
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	
	/* PWM1 Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = CCR1_tab.table[CCR1_tab.crrTab][0];
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

  TIM_OC1Init(TIM3, &TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
  /* PWM1 Mode configuration: Channel2 */
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = CCR2_tab.table[CCR2_tab.crrTab][0];

  TIM_OC2Init(TIM3, &TIM_OCInitStructure);

  TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
  /* PWM1 Mode configuration: Channel3 */
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = CCR3_tab.table[CCR3_tab.crrTab][0];

  TIM_OC3Init(TIM3, &TIM_OCInitStructure);

  TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
  /* TIM IT enable */
  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
  /* TIM3 enable counter */
  TIM_Cmd(TIM3, ENABLE);
}

void TIM3_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
  {						
		// change	duty cycle of PWM					
		TIM3->CCR1 = CCR1_tab.table[CCR1_tab.crrTab][TIM3_CCR_val_idx];
		TIM3->CCR2 = CCR2_tab.table[CCR2_tab.crrTab][TIM3_CCR_val_idx];
		TIM3->CCR3 = CCR3_tab.table[CCR3_tab.crrTab][TIM3_CCR_val_idx];		
		
		TIM3_CCR_val_idx = ( TIM3_CCR_val_idx < SAMPLER-1 ) ? TIM3_CCR_val_idx+1 : 0;		// update counter				
		
		
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
  }
}



/*--------------- GENERAL FUNCTIONS ------------------------------------------------------------------------*/
void Delay(__IO uint32_t nTime, DelayUnit_Type unit)
{
	switch ( unit )
	{
//		case DelayUnit_us : TimingDelay = nTime;					break;
		case DelayUnit_ms : TimingDelay = nTime;			break;
		case DelayUnit_s  : TimingDelay = nTime*1000;	break;
		default : break;
	}
 
  while(TimingDelay != 0) 
		__NOP();
}

/**
  * @brief  Decrements the TimingDelay variable.
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
	
	if (UsrBtn_Debnce_cnt != 0x00)
		UsrBtn_Debnce_cnt--;
}
			





/*--------------- PWM GENERATOR FUNCTIONS ------------------------------------------------------------------*/
void CCRTab_init( CCRTab_Type *CCRTab, uint16_t min, uint16_t max, float amp, float offset, float startDeg )
{
	uint16_t i;
	
	CCRTab->max = max;
	CCRTab->min = min;
	CCRTab->normAmp = amp;
	CCRTab->offset = offset;	
	CCRTab->numOfPtrn = 0;
	CCRTab->crrPtrn = 0;
	CCRTab->crrTab = 1;
	CCRTab->state = State_Norminal;
	
	for ( i=0; i<SAMPLER; i++ )
	{
		CCRTab->sin_table[i] = sin( i*_2PI_BY_SAMPLER + startDeg ) ;			
	}
	
	genDutyTab(CCRTab, 0, TabType_Norminal);
}

/**
  * @brief  Generate duty cycle table
  * @param  ...
  * @retval None
  */
uint8_t genDutyTab(CCRTab_Type *CCRTab, uint8_t ptrnIdx , CCRTabType_Type tabType)
{
	int16_t i;
	uint16_t tmp, tabIdx = !CCRTab->crrTab;
	uint8_t result = 0;

//	LED( Orange, ON );
	
	switch ( tabType )
	{
		case TabType_Norminal:
			for ( i=0; i<SAMPLER; i++ )
			{
				tmp = CCRTab->normAmp*CCRTab->sin_table[i] + CCRTab->offset;
				if ( tmp < CCRTab->min )
						CCRTab->table[tabIdx][i] = CCRTab->min;
				else if ( tmp > CCRTab->max )
						CCRTab->table[tabIdx][i] = CCRTab->max;
				else
						CCRTab->table[tabIdx][i] = tmp;
			}			
			CCRTab->crrTab = tabIdx;	// change current table that using to generate PWM	
			CCRTab->state = State_Norminal;
			break;
		
		case TabType_Sag:	
			if ( CCRTab->state == State_Sag || CCRTab->state == State_StopingSag )				
				result = 1;				// nested sag!
			else if ( CCRTab->state == State_Pattern || CCRTab->state == State_StopingPattern )				
				result = 2;				// CCR is now generating pattern!
			else
			{
				for ( i=0; i<SAMPLER; i++ )
				{
					tmp = CCRTab->sagAmp*CCRTab->sin_table[i] + CCRTab->offset;
					if ( tmp < CCRTab->min )
							CCRTab->table[tabIdx][i] = CCRTab->min;
					else if ( tmp > CCRTab->max )
							CCRTab->table[tabIdx][i] = CCRTab->max;
					else
							CCRTab->table[tabIdx][i] = tmp;
				}	
				CCRTab->crrTab = tabIdx;	// change current table that using to generate PWM
				CCRTab->state = State_Sag;
			}	
			break;
		
		case TabType_Pattern:		
			if ( CCRTab->state == State_Sag || CCRTab->state == State_StopingSag )				
				result = 3;				// CCR is now generating sag!
			else
			{	
				for ( i=0; i<SAMPLER; i++ )
				{
					tmp = CCRTab->ptrnAmp[ptrnIdx]*CCRTab->sin_table[i] + CCRTab->offset;
					if ( tmp < CCRTab->min )
							CCRTab->table[tabIdx][i] = CCRTab->min;
					else if ( tmp > CCRTab->max )
							CCRTab->table[tabIdx][i] = CCRTab->max;
					else
							CCRTab->table[tabIdx][i] = tmp;
				}
				CCRTab->crrPtrn = ptrnIdx;
				CCRTab->crrTab = tabIdx;	// change current table that using to generate PWM
				CCRTab->state = State_Pattern;
			}
			break;
		
		default: break;
	}
		
//	LED( Orange, OFF );
	return result;
}



void genDutyTabPolling(void)
{	
	uint8_t i;
	
	//<-- CCR1_CallGDT -------------------------------------------------------------
	if ( CCR1_CallGDT.isCall )
	{
		genDutyTab_result = genDutyTab( &CCR1_tab, CCR1_CallGDT.ptrnIdx, CCR1_CallGDT.tabType );
		
		if ( CCR1_CallGDT.callFromUsr )
		{
			// switch for response to USART
			switch ( CCR1_CallGDT.tabType )
			{
				case TabType_Norminal:
					break;
				
				case TabType_Sag:
					switch ( genDutyTab_result ) // checking return status
					{
						case 0:
							sprintf((char*)sendingMsg, "Generating sag:\n - CH 1\n - Percentage %.2f %%\n - Duration %d ms\n", CCR1_tab.sagAmp/CCR1_tab.normAmp*100, CCR1_tab.sagDuration);
							break;
						case 1:
							sprintf((char*)sendingMsg, "Generating sag:\n - Cannot generate nested sag!\n");
							break;
						case 2:
							sprintf((char*)sendingMsg, "Generating sag:\n - CH 1 is generating pattern now!\n Please call stoppattern before\n");
							break;
						default: break;
					}
								
					USART_puts(USART1, sendingMsg);
					break;
					
				case TabType_Pattern:
					switch ( genDutyTab_result ) // checking return status
					{
						case 0:
							USART_puts(USART1, "Generating pattern:\n - Ch 1 pattern generated\n");
						
							for ( i=0; i<CCR1_tab.numOfPtrn; i++ )
							{
								sprintf((char*)sendingMsg, " - Pattern %d: Percentage %.2f %%\n - Pattern %d: Duration %d ms\n", 
									i+1, CCR1_tab.ptrnAmp[i]/CCR1_tab.normAmp*100, i+1, CCR1_tab.ptrnDuration[i]);					
								USART_puts(USART1, sendingMsg);
							}
							break;
						case 3:
							USART_puts(USART1, "Generating pattern:\n - The system is generating sag now!\n");
							break;
						default:	break;
					}			
					break;
				
				default: break;
			}			
		}
		CCR1_CallGDT.isCall = false;
	}		

	
	//<-- CCR2_CallGDT -------------------------------------------------------------
	if ( CCR2_CallGDT.isCall )
	{
		genDutyTab_result = genDutyTab( &CCR2_tab, CCR2_CallGDT.ptrnIdx, CCR2_CallGDT.tabType );
				
		if ( CCR2_CallGDT.callFromUsr )
		{
			// switch for response to USART
			switch ( CCR2_CallGDT.tabType )
			{
				case TabType_Norminal:
					break;
				
				case TabType_Sag:
					switch ( genDutyTab_result ) // checking return status
					{
						case 0:
							sprintf((char*)sendingMsg, "Generating sag:\n - CH 2\n - Percentage %.2f %%\n - Duration %d ms\n", CCR2_tab.sagAmp/CCR2_tab.normAmp*100, CCR2_tab.sagDuration);
							break;
						case 1:
							sprintf((char*)sendingMsg, "Generating sag:\n - Cannot generate nested sag!\n");
							break;
						case 2:
							sprintf((char*)sendingMsg, "Generating sag:\n - CH 2 is generating pattern now!\n Please call stoppattern before\n");
							break;
						default: break;
					}
								
					USART_puts(USART1, sendingMsg);
					break;
					
				case TabType_Pattern:
					switch ( genDutyTab_result ) // checking return status
					{
						case 0:
							USART_puts(USART1, "Generating pattern:\n - Ch 2 pattern generated\n");					
							for ( i=0; i<CCR2_tab.numOfPtrn; i++ )
							{
								sprintf((char*)sendingMsg, " - Pattern %d: Percentage %.2f %%\n - Pattern %d: Duration %d ms\n", 
									i+1, CCR2_tab.ptrnAmp[i]/CCR2_tab.normAmp*100, i+1, CCR2_tab.ptrnDuration[i]);					
								USART_puts(USART1, sendingMsg);
							}
							break;
						case 3:
							USART_puts(USART1, "Generating pattern:\n - The system is generating sag now!\n");
							break;
						default:	break;
					}			
					break;
				
				default: break;
			}			
		}
		CCR2_CallGDT.isCall = false;
	}		

	
	//<-- CCR3_CallGDT -------------------------------------------------------------
	if ( CCR3_CallGDT.isCall )
	{
		genDutyTab_result = genDutyTab( &CCR3_tab, CCR3_CallGDT.ptrnIdx, CCR3_CallGDT.tabType );
		
		if ( CCR3_CallGDT.callFromUsr )
		{			
			// switch for response to USART
			switch ( CCR3_CallGDT.tabType )
			{
				case TabType_Norminal:
					break;
				
				case TabType_Sag:
					switch ( genDutyTab_result ) // checking return status
					{
						case 0:
							sprintf((char*)sendingMsg, "Generating sag:\n - CH 3\n - Percentage %.2f %%\n - Duration %d ms\n", CCR3_tab.sagAmp/CCR3_tab.normAmp*100, CCR3_tab.sagDuration);
							break;
						case 1:
							sprintf((char*)sendingMsg, "Generating sag:\n - Cannot generate nested sag!\n");
							break;
						case 2:
							sprintf((char*)sendingMsg, "Generating sag:\n - CH 3 is generating pattern now!\n Please call stoppattern before\n");
							break;
						default: break;
					}								
					USART_puts(USART1, sendingMsg);
					break;
					
				case TabType_Pattern:
					switch ( genDutyTab_result ) // checking return status
					{
						case 0:
							USART_puts(USART1, "Generating pattern:\n - Ch 3 pattern generated\n");					
							for ( i=0; i<CCR3_tab.numOfPtrn; i++ )
							{
								sprintf((char*)sendingMsg, " - Pattern %d: Percentage %.2f %%\n - Pattern %d: Duration %d ms\n", 
									i+1, CCR3_tab.ptrnAmp[i]/CCR3_tab.normAmp*100, i+1, CCR3_tab.ptrnDuration[i]);					
								USART_puts(USART1, sendingMsg);
							}
							break;
						case 3:
							USART_puts(USART1, "Generating pattern:\n - The system is generating sag now!\n");
							break;
						default:	break;
					}			
					break;
				
				default: break;
			}			
		}
		CCR3_CallGDT.isCall = false;
	}		

	
}

CCRTab_Type* getCCRTab(uint8_t channel)
{
	switch ( channel )
	{
		case 1: return &CCR1_tab;
		case 2: return &CCR2_tab;
		case 3: return &CCR3_tab;
		default: return 0x00;		
	}
}



CallingGenDutyTab_Type* getGenDutyTabCaller(uint8_t channel)
{
	switch ( channel )
	{
		case 1: return &CCR1_CallGDT;
		case 2: return &CCR2_CallGDT;
		case 3: return &CCR3_CallGDT;
		default: return 0x00;		
	}
}


void sagTimer(void)
{
	//<------- Sag on CCR1 ----------------------------------
  if ( CCR1_tab.state == State_Sag )
  { 
		if ( CCR1_tab.sagDuration != 0x00 )
			CCR1_tab.sagDuration--;
		else
		{
			stopSag(1);	
		}
  }
	else if ( CCR1_tab.state == State_Pattern )
	{		
		if ( CCR1_SagPtrnDrtnCnt < CCR1_tab.ptrnDuration[CCR1_tab.crrPtrn] )
			CCR1_SagPtrnDrtnCnt++;
		else
		{
			/* generate new pattern */		
			CCR1_CallGDT.ptrnIdx = ( CCR1_tab.crrPtrn	< CCR1_tab.numOfPtrn-1 ) ? CCR1_tab.crrPtrn+1 : 0;						
			CCR1_CallGDT.tabType = TabType_Pattern;
			CCR1_CallGDT.callFromUsr = false;	
			CCR1_CallGDT.isCall = true;			
			
			CCR1_SagPtrnDrtnCnt = 0;	// reset counter					
		}
	}
	
	//<------- Sag on CCR2 ----------------------------------
  if ( CCR2_tab.state == State_Sag )
  { 
		if ( CCR2_tab.sagDuration != 0x00 )
			CCR2_tab.sagDuration--;
		else
			stopSag(2);		
  }
	else if ( CCR2_tab.state == State_Pattern )
	{		
		if ( CCR2_SagPtrnDrtnCnt < CCR2_tab.ptrnDuration[CCR2_tab.crrPtrn] )
			CCR2_SagPtrnDrtnCnt++;
		else
		{
			/* generate new pattern */		
			CCR2_CallGDT.ptrnIdx = ( CCR2_tab.crrPtrn	< CCR2_tab.numOfPtrn-1 ) ? CCR2_tab.crrPtrn+1 : 0;						
			CCR2_CallGDT.tabType = TabType_Pattern;
			CCR2_CallGDT.callFromUsr = false;	
			CCR2_CallGDT.isCall = true;	
			
			CCR2_SagPtrnDrtnCnt = 0;	// reset counter					
		}
	}
  
	
	//<------- Sag on CCR3 ----------------------------------
  if ( CCR3_tab.state == State_Sag )
  { 
		if ( CCR3_tab.sagDuration != 0x00 )
			CCR3_tab.sagDuration--;
		else
			stopSag(3);		
  }
	else if ( CCR3_tab.state == State_Pattern )
	{		
		if ( CCR3_SagPtrnDrtnCnt < CCR3_tab.ptrnDuration[CCR3_tab.crrPtrn] )
			CCR3_SagPtrnDrtnCnt++;
		else
		{
			/* generate new pattern */		
			CCR3_CallGDT.ptrnIdx = ( CCR3_tab.crrPtrn	< CCR3_tab.numOfPtrn-1 ) ? CCR3_tab.crrPtrn+1 : 0;						
			CCR3_CallGDT.tabType = TabType_Pattern;
			CCR3_CallGDT.callFromUsr = false;	
			CCR3_CallGDT.isCall = true;	
			
			CCR3_SagPtrnDrtnCnt = 0;	// reset counter					
		}
	}
}

bool stopSag( uint8_t channel )
{
	bool result = false;
	switch (channel)
	{
		case 1:	
			if ( CCR1_tab.state == State_Sag )
			{	
				CCR1_CallGDT.tabType = TabType_Norminal;
				CCR1_CallGDT.callFromUsr = false;		
				CCR1_tab.state = State_StopingSag;	
				CCR1_CallGDT.isCall = true;	
				
				result = true;
			}
			break;
		
		case 2:		
			if ( CCR2_tab.state == State_Sag )
			{		
				CCR2_CallGDT.tabType = TabType_Norminal;
				CCR2_CallGDT.callFromUsr = false;		
				CCR2_tab.state = State_StopingSag;	
				CCR2_CallGDT.isCall = true;	
			
				result = true;
			}
			break;
		
		case 3:		
			if ( CCR3_tab.state == State_Sag )
			{		
				CCR3_CallGDT.tabType = TabType_Norminal;
				CCR3_CallGDT.callFromUsr = false;			
				CCR3_tab.state = State_StopingSag;
				CCR3_CallGDT.isCall = true;	
			
				result = true;
			}
			break;
		
		default: break;			
	}	
	
	return result;
}


bool stopPattern( uint8_t channel )
{
	bool result = false;
	uint8_t i;
	
	switch (channel)
	{
		case 1:	
			if ( CCR1_tab.state == State_Pattern )
			{						
				CCR1_CallGDT.tabType = TabType_Norminal;
				CCR1_CallGDT.callFromUsr = false;			
				CCR1_tab.state = State_StopingPattern;
				CCR1_CallGDT.isCall = true;	
			
				result = true;
			}
			break;
		
		case 2:	
			if ( CCR2_tab.state == State_Pattern )
			{		
				CCR2_CallGDT.tabType = TabType_Norminal;
				CCR2_CallGDT.callFromUsr = false;		
				CCR2_tab.state = State_StopingPattern;	
				CCR2_CallGDT.isCall = true;	
						
				result = true;
			}
			break;
		
		case 3:	
			if ( CCR3_tab.state == State_Pattern )
			{		
				CCR3_CallGDT.tabType = TabType_Norminal;
				CCR3_CallGDT.callFromUsr = false;		
				CCR3_tab.state = State_StopingPattern;	
				CCR3_CallGDT.isCall = true;			
			
				result = true;
			}
			break;
		
		default: break;			
	}	
	
	return result;
}
