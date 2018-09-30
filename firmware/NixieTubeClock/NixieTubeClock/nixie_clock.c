/*
 * nixie_clock.c
 *
 * Created: 2018/09/08 20:35:30
 * Author : _7dki
*/

//---- ヘッダファイルインクルード ----
#include "nixie_clock.h"

//---- グローバル変数定義 ----
uint16_t dispdigit = 0;
uint8_t dispnum[6];
uint8_t hour=0,min=0,sec=0;
uint8_t state=0;

//---- ニキシー管点灯処理用関数 ----

// void senddata(uint32_t data)
// 説明：シフトレジスタに24ビットのデータを送信
// 引数：
//   uint32_t data：送信データ(下位24bitを送信)
// 戻り値：
//   なし
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
// 説明：任意の桁に任意の桁を表示
// 引数：
//   uint16_t num：表示データ
//   uint16_t digit：表示桁
// 戻り値：
//   なし
void sendnum(uint16_t num, uint16_t digit)
{
	uint32_t data = (((uint32_t)digit) << 12) | (uint32_t)num;

	sendserdata(data);
}

// uint16_t cod_num(uint16_t num)
// 説明：十進数からニキシー管数値表示コードに変換
// 引数：
//   uint16_t num：変換する数値(十進数)
// 戻り値：
//   シフトレジスタ書き込み用コード(表示数値)
uint16_t cod_num(uint16_t num)
{
	return (0x0002 << num);
}

// uint16_t cod_digit(uint16_t digit)
// 説明：十進数からニキシー管表示桁コードに変換
// 引数：
//   uint16_t num：変換する数値(十進数)
// 戻り値：
//   シフトレジスタ書き込み用コード(表示桁)
uint16_t cod_digit(uint16_t digit)
{
	return (0x0001 << digit);
}

// void addsec(void)
// 説明：時計の秒を1進める
// 引数：
//   なし
// 戻り値：
//   なし
void addsec(void)
{
	sec++;
	if(sec>59){
		sec=0;
	}
	return;
}

// void addmin(void)
// 説明：時計の分を1進める
// 引数：
//   なし
// 戻り値：
//   なし
void addmin(void)
{
	min++;
	if(min>59){
		min=0;
	}
	return;
}

// void addhour(void)
// 説明：時計の時を1進める
// 引数：
//   なし
// 戻り値：
//   なし
void addhour(void)
{
	hour++;
	if(hour>23){
		hour=0;
	}
	return;
}

// void addtime(void)
// 説明：時計を1秒進める
// 引数：
//   なし
// 戻り値：
//   なし
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
// 説明：時計をニキシー管に表示する
// 引数：
//   なし
// 戻り値：
//   なし
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
// 説明：LEDを消灯する
// 引数：
//   消灯するLEDの桁(十進数)
// 戻り値：
//   なし
void led_off(uint8_t n)
{
	uint8_t shift = 5-n;
	PORTB &= ~(0x10<<shift);
	PORTC &= ~(0x04<<shift);

	return;
}

// void led_on(uint8_t n)
// 説明：LEDを点灯する
// 引数：
//   点灯するLEDの桁(十進数)
// 戻り値：
//   なし
void led_on(uint8_t n)
{
	uint8_t shift = 5-n;
	PORTB |= (0x10<<shift);
	PORTC |= (0x04<<shift);

	return;
}

// void led_alloff(void)
// 説明：すべてのLEDを消灯する
// 引数：
//   なし
// 戻り値：
//   なし
void led_alloff(void)
{
	PORTB &= 0x07;
	PORTC = 0x00;
	return;
}

// void led_allon(void)
// 説明：すべてのLEDを点灯する
// 引数：
//   なし
// 戻り値：
//   なし
void led_allon(void)
{
	PORTB |= 0xf8;
	PORTC = 0xff;
	return;
}