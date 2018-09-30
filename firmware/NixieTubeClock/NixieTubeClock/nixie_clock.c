/*
 * nixie_clock.c
 *
 * Created: 2018/09/08 20:35:30
 * Author : _7dki
*/

//---- �w�b�_�t�@�C���C���N���[�h ----
#include "nixie_clock.h"

//---- �O���[�o���ϐ���` ----
uint16_t dispdigit = 0;
uint8_t dispnum[6];
uint8_t hour=0,min=0,sec=0;
uint8_t state=0;

//---- �j�L�V�[�Ǔ_�������p�֐� ----

// void senddata(uint32_t data)
// �����F�V�t�g���W�X�^��24�r�b�g�̃f�[�^�𑗐M
// �����F
//   uint32_t data�F���M�f�[�^(����24bit�𑗐M)
// �߂�l�F
//   �Ȃ�
void sendserdata(uint32_t data)
{
	uint8_t i;
	
	for(i=0; i<18; i++){
		if((data & (uint32_t)0x00020000 >> i) == 0){
			PORTF &= ~SER;
		}else{
			PORTF |= SER;
		}
		
		// rise clk
		PORTF |= SRCLK;
		// fall clk
		PORTF &= ~SRCLK;
	}
	
	return;
}

// void sendnum(uint16_t num, uint16_t digit)
// �����F�C�ӂ̌��ɔC�ӂ̌���\��
// �����F
//   uint16_t num�F�\���f�[�^
//   uint16_t digit�F�\����
// �߂�l�F
//   �Ȃ�
void sendnum(uint16_t num, uint16_t digit)
{
	uint32_t data = (((uint32_t)digit) << 12) | (uint32_t)num;

	sendserdata(data);
}

// uint16_t cod_num(uint16_t num)
// �����F�\�i������j�L�V�[�ǐ��l�\���R�[�h�ɕϊ�
// �����F
//   uint16_t num�F�ϊ����鐔�l(�\�i��)
// �߂�l�F
//   �V�t�g���W�X�^�������ݗp�R�[�h(�\�����l)
uint16_t cod_num(uint16_t num)
{
	return (0x0002 << num);
}

// uint16_t cod_digit(uint16_t digit)
// �����F�\�i������j�L�V�[�Ǖ\�����R�[�h�ɕϊ�
// �����F
//   uint16_t num�F�ϊ����鐔�l(�\�i��)
// �߂�l�F
//   �V�t�g���W�X�^�������ݗp�R�[�h(�\����)
uint16_t cod_digit(uint16_t digit)
{
	return (0x0001 << digit);
}

// void addsec(void)
// �����F���v�̕b��1�i�߂�
// �����F
//   �Ȃ�
// �߂�l�F
//   �Ȃ�
void addsec(void)
{
	sec++;
	if(sec>59){
		sec=0;
	}
	return;
}

// void addmin(void)
// �����F���v�̕���1�i�߂�
// �����F
//   �Ȃ�
// �߂�l�F
//   �Ȃ�
void addmin(void)
{
	min++;
	if(min>59){
		min=0;
	}
	return;
}

// void addhour(void)
// �����F���v�̎���1�i�߂�
// �����F
//   �Ȃ�
// �߂�l�F
//   �Ȃ�
void addhour(void)
{
	hour++;
	if(hour>23){
		hour=0;
	}
	return;
}

// void addtime(void)
// �����F���v��1�b�i�߂�
// �����F
//   �Ȃ�
// �߂�l�F
//   �Ȃ�
void addtime(void)
{
	sec++;
	if(sec>59){
		sec=0;
		min++;
		if(min>59){
			min=0;
			hour++;
			if(hour>23){
				hour=0;
			}
		}
	}
	return;
}

// void disptime(void)
// �����F���v���j�L�V�[�ǂɕ\������
// �����F
//   �Ȃ�
// �߂�l�F
//   �Ȃ�
void disptime(void)
{
	dispnum[5] = hour/10;
	dispnum[4] = hour%10;
	dispnum[3] = min/10;
	dispnum[2] = min%10;
	dispnum[1] = sec/10;
	dispnum[0] = sec%10;

	return;
}

// void led_off(uint8_t n)
// �����FLED����������
// �����F
//   ��������LED�̌�(�\�i��)
// �߂�l�F
//   �Ȃ�
void led_off(uint8_t n)
{
	uint8_t shift = 5-n;
	PORTB &= ~(0x10<<shift);
	PORTC &= ~(0x04<<shift);

	return;
}

// void led_on(uint8_t n)
// �����FLED��_������
// �����F
//   �_������LED�̌�(�\�i��)
// �߂�l�F
//   �Ȃ�
void led_on(uint8_t n)
{
	uint8_t shift = 5-n;
	PORTB |= (0x10<<shift);
	PORTC |= (0x04<<shift);

	return;
}

// void led_alloff(void)
// �����F���ׂĂ�LED����������
// �����F
//   �Ȃ�
// �߂�l�F
//   �Ȃ�
void led_alloff(void)
{
	PORTB &= 0x07;
	PORTC = 0x00;
	return;
}

// void led_allon(void)
// �����F���ׂĂ�LED��_������
// �����F
//   �Ȃ�
// �߂�l�F
//   �Ȃ�
void led_allon(void)
{
	PORTB |= 0xf8;
	PORTC = 0xff;
	return;
}