/*====================================================*/
/* Jacob Parmer, Austin Harris							   */
/* ELEC 3040/3050 - Lab 6, Program 1 						*/
/* Count up from 0 - 9.9 at 0.1 second intervals      */
/*====================================================*/
#include "STM32L1xx.h" /* Microcontroller information */

typedef enum {false, true} bool;

/* Define global variables */

uint16_t count_integer;
uint16_t count_decimal;
uint16_t key_selected;
bool timer_enabled = false;
bool action_taken = false;
uint16_t test, test2;

/*---------------------------------------------------*/
/* Initialize GPIO pins used in the program */
/* PA1, PA2 = input switches */
/* PC[0:7] = output LED's to display value of count. */
/*---------------------------------------------------*/
void pinSetup () {

	RCC->AHBENR |= 0x07; /* Enable GPIOA clock (bits 2-0) */
	GPIOA->MODER &= ~(0x00000000C); /* General purpose input mode for PA1 */
	
	GPIOC->MODER &= ~(0x0000FFFF);
	GPIOC->MODER |= 0x00005555; /* Output mode for PC[0:7] */
	
	GPIOB->MODER &= ~(0x0000FFFF); /* General purpose input mode for PB[0:7] */
	GPIOB->MODER |= 0x00005500; /* Output mode for PB[4:7] */
	
	GPIOB->PUPDR &= 0x00000000;
	GPIOB->PUPDR |= 0x00000055; // pulls up pins PB[0:3]
	GPIOB->ODR &= ~(0x00F0);

}

/*--------------------------------------------------------*/
/* Initialize interrupt pins needed to trigger interrupts */
/* Sets up interrupts EXTI1_IRQ and TIM10_IRQ             */
/*--------------------------------------------------------*/
void interruptSetup() {
	
	SYSCFG->EXTICR[0] &= ~(0x000F);
	
	EXTI->FTSR |= 0x000002; // allows for interrupts at clock falling edge
	EXTI->IMR |= 0x000002; // enables interrupt request lines for EXTI1.
	
	NVIC_EnableIRQ(EXTI1_IRQn); // enable external interrupt EXTI1
	NVIC_SetPriority(EXTI1_IRQn, 1); // sets priority of EXTI1
	
	NVIC_EnableIRQ(TIM10_IRQn); // enable external interrupt TIM10
	NVIC_SetPriority(TIM10_IRQn, 2); // sets priority of TIM10
	
	__enable_irq(); // global interrupt enable

}

/*--------------------------------------------------------*/
/* Initializes timer enables and prescales					 */
/*--------------------------------------------------------*/
void timerSetup() {
	
	RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;	// enables clock timer
	TIM10->DIER |= TIM_DIER_UIE;  // enables TIM10
	
	TIM10->ARR &= 0x0000;
	TIM10->ARR |= 0x1FFF; // sets Auto Reload value
	
	TIM10->PSC &= 0x0000;
	TIM10->PSC |= 0x000F; // sets Prescale value
}

/*----------------------------------------------------------*/
/* Delay function - do nothing for delayTime */
/* delayTime = 20,000 = roughly a second. */
/*----------------------------------------------------------*/
void delay (int delayTime) {
	int i,j,n;
	for (i=0; i<20; i++) { //outer loop
		for (j=0; j<delayTime; j++) { //inner loop
			n = j; //dummy operation for single-step test
		} //do nothing
	}
}

/*---------------------------------------------------*/
/* Increments count_decimal from 0 - 9               */
/* Upon count_decimal = 9, increments count_integer  */
/*---------------------------------------------------*/
void counter() {
	if (count_decimal >= 0x0009) {
		count_decimal = 0x0000;
		if (count_integer >= 0x0090) {
			count_integer = 0x0000;
		} else {
			count_integer = count_integer + 0x0010;
		}
	} else {
		count_decimal++;
	}
}

/*---------------------------------------------------*/
/* Scans keypad to determine what key was pressed    */
/* Called during interrupt triggered by keypress     */
/*---------------------------------------------------*/
void scan() {
	bool keypress_found = false;
	uint16_t keypad[4][4] = {
		{0x0001, 0x0002, 0x0003, 0x000A},
		{0x0004, 0x0005, 0x0006, 0x000B},
		{0x0007, 0x0008, 0x0009, 0x000C},
		{0x000E, 0x0000, 0x000F, 0x000D}
	};
	uint16_t row_selected;
	int i;
	
	test = GPIOB->IDR & 0x000F;
	GPIOB->MODER &= ~(0x0000FFFF); /* General purpose input mode for PB[0:7] */
	GPIOB->MODER |= 0x00005500; /* Output mode for PB[4:7] */
	
	
	GPIOB->PUPDR &= 0x000000FF;
	GPIOB->ODR = 0xFF7F;
	
	test2 = GPIOB->IDR & 0x000F;
	for (i=0; i<4; i++) { // loops through 4 columns from right to left
		
		delay(5);
		
		switch (GPIOB->IDR & 0x000F) {
			case 0x000E:
				row_selected = 3;
				keypress_found = true;
				break;
			case 0x000D:
				row_selected = 2;
				keypress_found = true;
				break;
			case 0x000B:
				row_selected = 1;
				keypress_found = true;
				break;
			case 0x0007:
				row_selected = 0;
				keypress_found = true;
				break;
			default:
				break;
		}
		
		if (keypress_found == true) {
			key_selected = keypad[row_selected][i];
			keypress_found = false;
		
			if (key_selected == 0x0000) { // checks for keypress of key 0
				if (timer_enabled == true) {
					timer_enabled = false;
					TIM10->CR1 &= ~TIM_CR1_CEN; // disables timer clock
					GPIOC->ODR &= 0x00;
					GPIOC->ODR |= (count_integer | count_decimal); // Outputs count to pins PC[0:7].
				} else {
					timer_enabled = true;
					TIM10->CR1 |= TIM_CR1_CEN; // enables timer clock
					GPIOC->ODR &= 0x00;
					GPIOC->ODR |= (count_integer | count_decimal); // Outputs count to pins PC[0:7].
				}
			}
			
			if (key_selected == 0x0001) { // checks for keypress of key 1
				if (timer_enabled == false) {
					count_integer = 0x0000;
					count_decimal = 0x0000;
					GPIOC->ODR &= 0x00;
					GPIOC->ODR |= (count_integer | count_decimal); // Outputs count to pins PC[0:7].
				}
			}			
		}
			
		GPIOB->ODR = GPIOB->ODR>>1;
	} 
}

/*---------------------------------------------------*/
/* Interrupt triggered by keypad					        */
/* Initializes scan, resets pins to original state   */
/*---------------------------------------------------*/
void EXTI1_IRQHandler(void) {

	scan();
	delay(5000); // delay to prevent retriggering interrupt due to bouncing
	
	pinSetup();
	
	action_taken = false;
	EXTI->PR |= 0x000002; // clears interrupt pending bits
	NVIC_ClearPendingIRQ(EXTI1_IRQn);
}

/*---------------------------------------------------*/
/* */
void TIM10_IRQHandler(void) {
	
	counter();
	GPIOC->ODR &= 0x00;
	GPIOC->ODR |= (count_integer | count_decimal); // Outputs count to pins PC[0:7].
	TIM10->SR &= ~TIM_SR_UIF;
	EXTI->PR |= 0x000002; // clears interrupt pending bits
	NVIC_ClearPendingIRQ(TIM10_IRQn);
}

/*------------------------------------------------*/
/* 					Main program					  	  */
/*------------------------------------------------*/
int main(void) {

	pinSetup(); // Configure GPIO pins
	interruptSetup(); // Configure interrupt pins
	key_selected = 0xFFFF;
	
	count_integer = 0x0000;
	count_decimal = 0x0000;

	while (key_selected != 0x0000) {} // do nothing
	
	action_taken = true; // setting this to true prevents while (1) running keypress multiple times
	timer_enabled = true; // allows us to check if timer is currently running
	timerSetup();	// Configure timer pins
	TIM10->CR1 |= TIM_CR1_CEN; 
	
 /* Endless loop */
	while (1) {

	} /* repeat forever */
}
