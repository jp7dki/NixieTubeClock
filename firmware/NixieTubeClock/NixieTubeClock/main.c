/*
* nixie3_firmware.c
*
* Created: 2018/09/08 20:35:30
* Author : _7dki
*/

//---- CPU�N���b�N���g���̒�`(16MHz) ----
#define F_CPU 16000000UL

//---- �t�@�C���̃C���N���[�h ----
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <avr/wdt.h>
#include "nixie_clock.h"

//---- �萔��` ----
//���샂�[�h
#define MODE_CLOCK 0			// ���v���[�h
#define MODE_TIMEADJ 1			// �������킹���[�h
#define MODE_RANDNUM 2			// �����\�����[�h
#define MODE_POWERSAVE 3		// �ȓd�̓��[�h

// SW Inteface
#define SWA (PINB&0x01)
#define SWB (PINB&0x02)
#define SWC (PINB&0x04)
#define PUSHED 0				// SW�͉����ꂽ�Ƃ��ɁuL�v
#define COUNT_LONGPUSH 100			// ���������莞��(x10ms)

//---- �O���[�o���ϐ���` ----
extern uint16_t dispdigit;
extern uint8_t dispnum[6];
extern uint8_t hour,min,sec;
extern uint8_t state;

uint8_t led_duty[6];			// LED�̃f���[�e�B��
uint8_t led_count=0;

// wdt�΍�
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));
void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr(void)
{
	mcusr_mirror = MCUSR;
	MCUSR = 0;
	wdt_disable();
}

//---- ���荞�ݏ��� ----
// �^�C�}0�I�[�o�t���[���荞��(LED�̓_������)
ISR(TIMER0_OVF_vect)
{
	uint8_t i;

	if(led_count>99){
		led_count=0;
		for(i=0;i<6;i++){
			if(led_duty[i]==0){
				led_off(i);
			}else{
				led_on(i);
			}
		}
	}else{
		for(i=0;i<6;i++){
			if(led_duty[i]==led_count){
				led_off(i);
			}
		}
		led_count++;
	}
	
}

// �^�C�}1 �C���v�b�g�L���v�`�����荞��(�j�L�V�[�Ǔ_������)
ISR(TIMER1_CAPT_vect)
{
	sei();
	// ��U�J�\�[�h���܂߂Ă��ׂ�OFF����
	sendnum(0x00,0x00);
	
	// ���W�X�^�ւ̃f�[�^�]��
	PORTF |= RCLK;
	PORTF &= ~RCLK;

	// �\�����ɐ��l�f�[�^��\��
	sendnum(cod_num(dispnum[dispdigit]),cod_digit(dispdigit));
	
	// ���W�X�^�ւ̃f�[�^�]��
	PORTF |= RCLK;
	PORTF &= ~RCLK;
	
	// �\������ύX�F�_�C�i�~�b�N�_��
	dispdigit++;
	if(dispdigit > 5) dispdigit=0;
}

// �^�C�}1 �R���y�A�}�b�`B���荞��(�j�L�V�[�Ǐ�������)
ISR(TIMER1_COMPB_vect)
{
	sei();
	// �j�L�V�[�ǂ�����(�J�\�[�h�̓I���F�S�[�X�g�΍�)
	sendnum(0xff,0x00);
	// �f�[�^�����W�X�^�ɓ]��
	PORTF |= RCLK;
	PORTF &= ~RCLK;
}

// �^�C�}3 �C���v�b�g�L���v�`�����荞��(���ԏ���)
ISR(TIMER3_CAPT_vect)
{
	// ���샂�[�h�ɉ���������
	switch(state){
		// ���v���킹���[�h�ł͎����̃C���N�������g�͍s��Ȃ�
		case MODE_TIMEADJ:
			break;
		// �����\�����[�h�F�����̃C���N�������g�ƃj�L�V�[�ǂւ̕\��
		case MODE_CLOCK:
			addtime();
			disptime();
			break;
		// ���̂ق��̃��[�h�ł͎����X�V�̂ݎ��{
		default:
			addtime();
			break;
	}
}

// �O��������
ISR(INT6_vect)
{
	uint8_t i;

	if(state != MODE_POWERSAVE){
		state = MODE_POWERSAVE;

		// LED����
		for(i=0;i<6;i++){
			led_duty[i]=0;
		}
	}


}

//---- ���C���֐� -----
int main(void)
{
	uint16_t i,j;
	uint16_t disp_num=0, disp_digit=0;
	uint16_t count;

	// �z��̏�����
	for(i=0;i<6;i++){
		dispnum[i] = 0;
		led_duty[i] = 30;
	}
	led_count=0;
	
	// �^�C�}0������
	TCCR0A = 0b00000000;
	TCCR0B = 0b00000010;
	TIMSK0 = 0b00000001;		// enable timer/counter0 overflow interrupt enable

	// �^�C�}1������
	TCCR1A = 0b00000000;		// fast PWM mode WGM=0b1111
	TCCR1B = 0b00011010;
	TCCR1C = 0b00000000;
	TIMSK1 = 0b00100100;
	ICR1 = 8000;
	OCR1B = 4000;

	// �^�C�}3������
	TCCR3A = 0b00000000;
	TCCR3B = 0b00011100;
	TCCR3C = 0b00000000;
	TIMSK3 = 0b00100000;
	ICR3 = 62500;				// 1/(16MHz/256) * 62500 = 1.0000s

	// ���o�͏�����
	DDRB = 0xf8;
	DDRC = 0xff;
	DDRF = 0xff;
	DDRE = 0xBf;
	PORTB = 0x07;
	PORTC = 0x00;
	PORTF = 0x00;
	PORTF |=  SRCLR_N;
	PORTE = 0x00;

	// �O�������ݐݒ�
	EICRA = 0x00;
	EICRB = 0x10;		// INT6(PE6) both edge
	EIMSK = 0x40;		// INT6���荞�݋���

	// �X���[�v���[�h�Ƃ���Idle����Ƃ���
	SMCR = 0x00;
	PRR0 = 0b10000101;
	PRR1 = 0b00010001;
	
	// ���荞�݋���
	sei();

	// ���Z�b�g��͎����\�����[�h
	state = MODE_CLOCK;

	// ���ԕ\��
	disptime();

	// ���C�����[�v
	while (1)
	{
		
		// ���샂�[�h�ɉ���������
		switch(state){
			// �����\�����[�h
			case MODE_CLOCK:
				if(SWA==PUSHED){
					// ���������菈��
					_delay_ms(100);					// �`���^�����O�h�~
					count=0;
					while((SWA==PUSHED) && (count!=COUNT_LONGPUSH)){
						count++;
						_delay_ms(10);
					}
					if(count==COUNT_LONGPUSH){
						state = MODE_TIMEADJ;
						for(i=0;i<6;i++){
							led_duty[i] = 0;		// ���������ł�LED�͏���
						}
					}
					while(SWA==PUSHED);
				}else if(SWB==PUSHED){
					// ���������菈��
					_delay_ms(100);					// �`���^�����O�h�~
					count=0;
					while((SWB==PUSHED) && (count!=COUNT_LONGPUSH)){
						count++;
						_delay_ms(10);
					}
					if(count==COUNT_LONGPUSH){
						state = MODE_RANDNUM;
						for(i=0;i<6;i++){
							dispnum[i] = 0;	
							led_duty[i] = 0;		// LED������
						}
					}
					while(SWB==PUSHED);
				}
				break;

			// �����������[�h
			case MODE_TIMEADJ:
				if(SWA==PUSHED){
					// ���������菈��
					_delay_ms(100);					// �`���^�����O�h�~
					count=0;
					while((SWA==PUSHED) && (count!=COUNT_LONGPUSH)){
						count++;
						_delay_ms(10);
					}
					if(count==COUNT_LONGPUSH){
						state = MODE_CLOCK;
						for(i=0;i<6;i++){
							led_duty[i] = 30;		// LED���ē_��
						}
						while(SWA==PUSHED);
					}else{
						addhour();
						disptime();
						TCNT3=0;
					}
				}else if(SWB==PUSHED){
					addmin();
					disptime();
					TCNT3=0;
					_delay_ms(100);					// �`���^�����O�h�~
					while(SWB==PUSHED);
				}else if(SWC==PUSHED){
					addsec();
					disptime();
					TCNT3=0;
					_delay_ms(100);					// �`���^�����O�h�~
					while(SWC==PUSHED);
				}
				break;

			// �����\�����[�h
			case MODE_RANDNUM:
				if(SWA==PUSHED){
					// ���������菈��
					_delay_ms(100);					// �`���^�����O�h�~
					count=0;
					while((SWA==PUSHED) && (count!=COUNT_LONGPUSH)){
						count++;
						_delay_ms(10);
					}
					if(count==COUNT_LONGPUSH){
						state = MODE_CLOCK;
						for(i=0;i<6;i++){
							led_duty[i] = 30;		// LED���ē_��
						}
						while(SWA==PUSHED);
					}else{
						// �Z������������
						// LED���������A�����\�����J�n
						srand(TCNT0);
						for(i=0;i<6;i++){
							led_duty[i] = 0;		// LED�͏���
						}
						for(j=0;j<780;j++){
							for(i=0;i<6;i++){
								if(j<((i*80)+300)){
									dispnum[i] = rand()%10;
								}
								if(j==((i*80)+300)){
									led_duty[i] = 50;
								}
							}
							_delay_ms(10);
						}
					}
				}
				break;
			
			case MODE_POWERSAVE:
				// �d���`�F�b�N
				if((PINE&0x40)!=0){
					// �d�����A�Ȃ�Ύ��v�\�����[�h�ɖ߂�
					state = MODE_CLOCK;
					for(i=0;i<6;i++){
						led_duty[i] = 30;		// LED���ē_��
					}
					
					// ���ԕ\��
					disptime();
				}else{
					_delay_ms(100);
					// �j�L�V�[�Ǐ���
					sendnum(0xff,0x00);
					
					// ���W�X�^�ւ̃f�[�^�]��
					PORTF |= RCLK;
					PORTF &= ~RCLK;
					TIMSK0 = 0b00000000;		// enable timer/counter0 overflow interrupt enable
					TIMSK1 = 0b00000000;
					sleep_enable();
					sleep_cpu();
					sleep_disable();
					TIMSK0 = 0b00000001;		// enable timer/counter0 overflow interrupt enable
					TIMSK1 = 0b00100100;
				}
			default:
				break;
		}
	}
}


