power-monitor-simulator
=======================

1. STM32F4 Pin connection
  This project uses 2 major port groups.

  1.1) USART command line connection
    Use TTL UART serial communication @Baudrate 115200
    - PB6 for TX
    - PB7 for RX
    - And GND
    
  1.2) PWM output for generating 3 channel sine waves.
    Every channel will generate output from 0v to VDD (about 3.3v).  
    Each channel is different on phase by 120°.
    - PA6 for CH1
    - PA7 for CH2
    - PB0 for CH3
    

2. Specification
  This program will genenrate the varying PWM for 1000 values of 1 sine wave(50Hz).
So, each PWM value will be generated every 50kHz.


3. Command line
  To generate any sag of output, you can use these command lines with the \n (0x0A or LF) 
as the EOL of any command.

  3.1) Generate sag

       Usage: sag [CH] [PERCENTAGE] [DURATION]
       Parameter:
        > CH          - desire channel.
        > PERCENTAGE  - scale of output compare with the nominal ampitude.
        > DURATION    - duration of this sag.
        
       Example: sag 1 0.5 300     - generating sag on channel 1 with 50% of norminal amplitude
                                    by 300 ms.


	3.2) Generate pattern

       Usage: pattern [CH] [PERCENTAGE_1] [DURATION_1] ... [PERCENTAGE_n] [DURATION_n]
       Parameter:
        > CH          - desire channel.
        > PERCENTAGE  - scale of output compare with the nominal ampitude.
        > DURATION    - duration of this sag.
        
       Example: pattern 1 0.5 300 .75 50 1 2000    
        - Looping generate sag on channel 1 with 50% of norminal amplitude by 300 ms. 
          Then, 75% by 50 ms. And then, 100% by 2 s.
       
	3.3) Stop sag

       Usage: stopsag [CH]
       Parameter:
        > CH          - desire channel to stop the current sag.
        
       Example: stopsag 1   - stoping sag on channel 1.
	
	3.4) Stop pattern

       Usage: stoppattern [CH]
       Parameter:
        > CH          - desire channel.
        
       Example: stoppattern 1   - stoping the pattern generating on channel 1 and going to nominal.
       
	3.5) Show help

       Usage: help
        > CH          - desire channel.
        
