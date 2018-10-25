/**
 ******************************************************************************
 * @file    main.c
 * @author  Ac6
 * @version V1.0
 * @date    01-December-2013
 * @brief   Default main function.
 ******************************************************************************
 */

#include "stm32f0xx.h"
void setupVT1();
void setupVT23();

volatile uint8_t brightness = 16;

void setOuputBrightness(int pin, int value) {
	switch (pin) {
	case 1:
		TIM1->CCR3 = value;
		break;
	case 2:
		TIM1->CCR2 = value;
		break;
	case 3:
		TIM3->CCR4 = value;
		break;
	case 4:
		TIM3->CCR2 = value;
		break;
	case 5:
		TIM3->CCR1 = value;
		break;
	case 6:
		GPIOA->ODR = (GPIOA->ODR & ~(GPIO_ODR_5))
				| (((value >= brightness) ? 1 : 0) << 5);
		break;
	}
}

void setBrightness(uint8_t value) {
	brightness = value;
}

uint8_t translatePin(uint8_t pin) {
	return pin;
}

void output(uint8_t pin, uint8_t val) {
	setOuputBrightness(translatePin(pin), ((val > 0) ? brightness : 0));
}

typedef enum {
	MODE_BOTH = 3, MODE_LEFT = 2, MODE_RIGHT = 1
} ArrowMode_TypeDef;

volatile ArrowMode_TypeDef mode = MODE_BOTH;

volatile uint8_t strobeState = 0;
volatile uint8_t strobeIndex = 0;
void TIM17_IRQHandler() {
	if (strobeState > 2) {
		strobeIndex++;
		strobeState = 0;
	}
	if (strobeIndex > 3)
		strobeIndex = 0;
	uint8_t strobePin = ((strobeIndex >> 1) & 0x01);
	switch (strobeState) {
	case 0:
		output(strobePin + 1, 1);
		break;
	case 1:
		output(strobePin + 1, 1);
		break;
	case 2:
		output(strobePin + 1, 0);
		break;
	}
	strobeState++;
	TIM17->SR = 0;
}
volatile uint8_t arrowState = 0;
void TIM16_IRQHandler() {
	if (arrowState > 2)
		arrowState = 0;
	switch (arrowState) {
	case 0:
		if (mode & MODE_LEFT)
			output(3, 1);
		if (mode & MODE_RIGHT)
			output(4, 1);
		break;
	case 1:
		if (mode & MODE_LEFT)
			output(3, 1);
		if (mode & MODE_RIGHT)
			output(4, 1);
		break;
	case 2:
		output(3, 0);
		output(4, 0);
		break;
	default:
		arrowState = 0xFF;
		break;
	}
	arrowState++;
	TIM16->SR = 0;
}
void setupStrobesAndArrows() {
	RCC->APB2ENR |= RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM17EN;
	//prescaler for arrows
	TIM16->PSC = SystemCoreClock / 1000;
	TIM16->ARR = 1075;
	//prescaler for strobes
	TIM17->PSC = SystemCoreClock / 1000;
	TIM17->ARR = 70;

	TIM16->DIER |= TIM_DIER_UIE;
	TIM17->DIER |= TIM_DIER_UIE;

	NVIC_SetPriority(TIM16_IRQn, 4);
	NVIC_SetPriority(TIM17_IRQn, 3);
	NVIC_EnableIRQ(TIM16_IRQn);
	NVIC_EnableIRQ(TIM17_IRQn);

	TIM16->CR1 |= TIM_CR1_CEN;
	TIM17->CR1 |= TIM_CR1_CEN;

	TIM16->EGR |= TIM_EGR_UG;
	TIM17->EGR |= TIM_EGR_UG;

}

void setupButton() {
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR3_0;
}

uint8_t readButton() {
	return (GPIOA->IDR & (0x01 << 3)) == 0;
}

int main(void) {
	HAL_Init();

	setupVT1();
	setupVT23();

	for (int i = 0; i < 2; i++) {
		HAL_Delay(1000);
		for (int chan = 1; chan <= 6; chan++) {
			output(chan, i & 0x01);
		}
	}
	setBrightness(8);

	for (int i = 0; i < 2; i++) {
		HAL_Delay(1000);
		for (int chan = 1; chan <= 6; chan++) {
			output(chan, i & 0x01);
		}
	}

	setupStrobesAndArrows();

	setupButton();

	for (;;) {
		if (readButton()) {
			HAL_Delay(100);
			if (readButton()) {
				// Wait for button to be released
				while (readButton())
					;
				switch (mode) {
				case MODE_BOTH:
					mode = MODE_LEFT;
					break;
				case MODE_LEFT:
					mode = MODE_RIGHT;
					break;
				case MODE_RIGHT:
					mode = MODE_BOTH;
					break;
				default:
					mode = MODE_BOTH;
					break;
				}
			}
			HAL_Delay(300);
		}
		HAL_Delay(100);
	}
}

// Transistor VT1

void setupVT1() {
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

	GPIOA->MODER &= ~(GPIO_MODER_MODER9_Msk | GPIO_MODER_MODER10_Msk);
	GPIOA->MODER |= (GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1);

	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9 | GPIO_OSPEEDER_OSPEEDR10;

	GPIOA->AFR[1] |= (GPIO_AF2_TIM1 << GPIO_AFRH_AFSEL9_Pos)
			| (GPIO_AF2_TIM1 << GPIO_AFRH_AFSEL10_Pos);

	TIM1->PSC = 400;
	TIM1->ARR = 16;
//PA9
	TIM1->CCR2 = 4;
//PA10
	TIM1->CCR3 = 4;

// PA9 / Channel 2
	TIM1->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE;

//PA10 / Channel3
	TIM1->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3PE;

	TIM1->CCER |= TIM_CCER_CC2E | TIM_CCER_CC3E;

	TIM1->BDTR |= TIM_BDTR_MOE;

	TIM1->CR1 |= TIM_CR1_CEN;

	TIM1->EGR |= TIM_EGR_UG;
}

void setupVT23() {
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

	GPIOA->MODER &= ~(GPIO_MODER_MODER6_Msk | GPIO_MODER_MODER7_Msk);
	GPIOA->MODER |= (GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1);

	GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR6 | GPIO_OSPEEDER_OSPEEDR7;

	GPIOA->AFR[0] |= (GPIO_AF1_TIM3 << GPIO_AFRL_AFSEL6_Pos)
			| (GPIO_AF1_TIM3 << GPIO_AFRL_AFSEL7_Pos);

	GPIOB->MODER &= ~(GPIO_MODER_MODER1_Msk);
	GPIOB->MODER |= (GPIO_MODER_MODER1_1);

	GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR1;

	GPIOB->AFR[0] |= (GPIO_AF1_TIM3 << GPIO_AFRL_AFSEL1_Pos);

	TIM3->PSC = 400;
	TIM3->ARR = 16;

//PA6
	TIM3->CCR1 = 4;
//PA7
	TIM3->CCR2 = 4;
//PB1
	TIM3->CCR4 = 4;

// PA6 / Channel 1
// PA7 / Channel2

	TIM3->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE
			| TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE;

//PB1 / Channel4

	TIM3->CCMR2 |= TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4PE;

	TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC4E;

	TIM3->BDTR |= TIM_BDTR_MOE;

	TIM3->CR1 |= TIM_CR1_CEN;

	TIM3->EGR |= TIM_EGR_UG;

}
