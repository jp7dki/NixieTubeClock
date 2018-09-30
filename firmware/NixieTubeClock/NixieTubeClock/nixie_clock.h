/*
 * nixie_clock.h
 *
 * Created: 2018/09/08 20:35:30
 * Author : _7dki
*/

#ifndef NIXIE
#define NIXIE

#include <avr/io.h>

//---- �萔��` ----
// �j�L�V�[�ǃC���^�[�t�F�[�X(�V�t�g���W�X�^)�M����`
#define SER (0x40)
#define OE_N (0x20)
#define RCLK (0x10)
#define SRCLK (0x02)
#define SRCLR_N (0x01)

//---- �֐��v���g�^�C�v ----
void sendserdata(uint32_t data);			// Send 24-bit data
void sendnum(uint16_t num, uint16_t digit);	// Send Num
uint16_t cod_num(uint16_t num);				// decimal value to code
uint16_t cod_digit(uint16_t digit);			// decimal digit to code
void addsec(void);
void addmin(void);
void addhour(void);
void addtime(void);
void disptime(void);

void led_off(uint8_t n);
void led_on(uint8_t n);
void led_alloff(void);
void led_allon(void);

#endif