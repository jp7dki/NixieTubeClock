/*
* nixie3_firmware.c
*
* Created: 2018/09/08 20:35:30
* Author : _7dki
*/

//---- CPUクロック周波数の定義(16MHz) ----
#define F_CPU 16000000UL

//---- ファイルのインクルード ----
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <avr/wdt.h>
#include "nixie_clock.h"

//---- 定数定義 ----
//動作モード
#define MODE_CLOCK 0			// 時計モード
#define MODE_TIMEADJ 1			// 時刻あわせモード
#define MODE_RANDNUM 2			// 乱数表示モード
#define MODE_POWERSAVE 3		// 省電力モード

// SW Inteface
#define SWA (PINB&0x01)
#define SWB (PINB&0x02)
#define SWC (PINB&0x04)
#define PUSHED 0				// SWは押されたときに「L」
#define COUNT_LONGPUSH 100			// 長押し判定時間(x10ms)

//---- グローバル変数定義 ----
extern uint16_t dispdigit;
extern uint8_t dispnum[6];
extern uint8_t hour,min,sec;
extern uint8_t state;

uint8_t led_duty[6];			// LEDのデューティ比
uint8_t led_count=0;

// wdt対策
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));
void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr(void)
{
	mcusr_mirror = MCUSR;
	MCUSR = 0;
	wdt_disable();
}

//---- 割り込み処理 ----
// タイマ0オーバフロー割り込み(LEDの点灯処理)
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

// タイマ1 インプットキャプチャ割り込み(ニキシー管点灯処理)
ISR(TIMER1_CAPT_vect)
{
	sei();
	// 一旦カソードも含めてすべてOFFする
	sendnum(0x00,0x00);
	
	// レジスタへのデータ転送
	PORTF |= RCLK;
	PORTF &= ~RCLK;

	// 表示桁に数値データを表示
	sendnum(cod_num(dispnum[dispdigit]),cod_digit(dispdigit));
	
	// レジスタへのデータ転送
	PORTF |= RCLK;
	PORTF &= ~RCLK;
	
	// 表示桁を変更：ダイナミック点灯
	dispdigit++;
	if(dispdigit > 5) dispdigit=0;
}

// タイマ1 コンペアマッチB割り込み(ニキシー管消灯処理)
ISR(TIMER1_COMPB_vect)
{
	sei();
	// ニキシー管を消灯(カソードはオン：ゴースト対策)
	sendnum(0xff,0x00);
	// データをレジスタに転送
	PORTF |= RCLK;
	PORTF &= ~RCLK;
}

// タイマ3 インプットキャプチャ割り込み(時間処理)
ISR(TIMER3_CAPT_vect)
{
	// 動作モードに応じた処理
	switch(state){
		// 時計合わせモードでは時刻のインクリメントは行わない
		case MODE_TIMEADJ:
			break;
		// 時刻表示モード：時刻のインクリメントとニキシー管への表示
		case MODE_CLOCK:
			addtime();
			disptime();
			break;
		// そのほかのモードでは時刻更新のみ実施
		default:
			addtime();
			break;
	}
}

// 外部割込み
ISR(INT6_vect)
{
	uint8_t i;

	if(state != MODE_POWERSAVE){
		state = MODE_POWERSAVE;

		// LED消灯
		for(i=0;i<6;i++){
			led_duty[i]=0;
		}
	}


}

//---- メイン関数 -----
int main(void)
{
	uint16_t i,j;
	uint16_t disp_num=0, disp_digit=0;
	uint16_t count;

	// 配列の初期化
	for(i=0;i<6;i++){
		dispnum[i] = 0;
		led_duty[i] = 30;
	}
	led_count=0;
	
	// タイマ0初期化
	TCCR0A = 0b00000000;
	TCCR0B = 0b00000010;
	TIMSK0 = 0b00000001;		// enable timer/counter0 overflow interrupt enable

	// タイマ1初期化
	TCCR1A = 0b00000000;		// fast PWM mode WGM=0b1111
	TCCR1B = 0b00011010;
	TCCR1C = 0b00000000;
	TIMSK1 = 0b00100100;
	ICR1 = 8000;
	OCR1B = 4000;

	// タイマ3初期化
	TCCR3A = 0b00000000;
	TCCR3B = 0b00011100;
	TCCR3C = 0b00000000;
	TIMSK3 = 0b00100000;
	ICR3 = 62500;				// 1/(16MHz/256) * 62500 = 1.0000s

	// 入出力初期化
	DDRB = 0xf8;
	DDRC = 0xff;
	DDRF = 0xff;
	DDRE = 0xBf;
	PORTB = 0x07;
	PORTC = 0x00;
	PORTF = 0x00;
	PORTF |=  SRCLR_N;
	PORTE = 0x00;

	// 外部割込み設定
	EICRA = 0x00;
	EICRB = 0x10;		// INT6(PE6) both edge
	EIMSK = 0x40;		// INT6割り込み許可

	// スリープモードとしてIdle動作とする
	SMCR = 0x00;
	PRR0 = 0b10000101;
	PRR1 = 0b00010001;
	
	// 割り込み許可
	sei();

	// リセット後は時刻表示モード
	state = MODE_CLOCK;

	// 時間表示
	disptime();

	// メインループ
	while (1)
	{
		
		// 動作モードに応じた処理
		switch(state){
			// 時刻表示モード
			case MODE_CLOCK:
				if(SWA==PUSHED){
					// 長押し判定処理
					_delay_ms(100);					// チャタリング防止
					count=0;
					while((SWA==PUSHED) && (count!=COUNT_LONGPUSH)){
						count++;
						_delay_ms(10);
					}
					if(count==COUNT_LONGPUSH){
						state = MODE_TIMEADJ;
						for(i=0;i<6;i++){
							led_duty[i] = 0;		// 時刻調整ではLEDは消灯
						}
					}
					while(SWA==PUSHED);
				}else if(SWB==PUSHED){
					// 長押し判定処理
					_delay_ms(100);					// チャタリング防止
					count=0;
					while((SWB==PUSHED) && (count!=COUNT_LONGPUSH)){
						count++;
						_delay_ms(10);
					}
					if(count==COUNT_LONGPUSH){
						state = MODE_RANDNUM;
						for(i=0;i<6;i++){
							dispnum[i] = 0;	
							led_duty[i] = 0;		// LEDを消灯
						}
					}
					while(SWB==PUSHED);
				}
				break;

			// 時刻調整モード
			case MODE_TIMEADJ:
				if(SWA==PUSHED){
					// 長押し判定処理
					_delay_ms(100);					// チャタリング防止
					count=0;
					while((SWA==PUSHED) && (count!=COUNT_LONGPUSH)){
						count++;
						_delay_ms(10);
					}
					if(count==COUNT_LONGPUSH){
						state = MODE_CLOCK;
						for(i=0;i<6;i++){
							led_duty[i] = 30;		// LEDを再点灯
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
					_delay_ms(100);					// チャタリング防止
					while(SWB==PUSHED);
				}else if(SWC==PUSHED){
					addsec();
					disptime();
					TCNT3=0;
					_delay_ms(100);					// チャタリング防止
					while(SWC==PUSHED);
				}
				break;

			// 乱数表示モード
			case MODE_RANDNUM:
				if(SWA==PUSHED){
					// 長押し判定処理
					_delay_ms(100);					// チャタリング防止
					count=0;
					while((SWA==PUSHED) && (count!=COUNT_LONGPUSH)){
						count++;
						_delay_ms(10);
					}
					if(count==COUNT_LONGPUSH){
						state = MODE_CLOCK;
						for(i=0;i<6;i++){
							led_duty[i] = 30;		// LEDを再点灯
						}
						while(SWA==PUSHED);
					}else{
						// 短く押した処理
						// LEDを消灯し、乱数表示を開始
						srand(TCNT0);
						for(i=0;i<6;i++){
							led_duty[i] = 0;		// LEDは消灯
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
				// 電源チェック
				if((PINE&0x40)!=0){
					// 電源復帰ならば時計表示モードに戻す
					state = MODE_CLOCK;
					for(i=0;i<6;i++){
						led_duty[i] = 30;		// LEDを再点灯
					}
					
					// 時間表示
					disptime();
				}else{
					_delay_ms(100);
					// ニキシー管消灯
					sendnum(0xff,0x00);
					
					// レジスタへのデータ転送
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


