/*

――――――――――――――――――――――――――――――――――――
□ I/Oピン設定
――――――――――――――――――――――――――――――――――――

PB0 … 7セグLED点灯用
PB1 … 7セグLED点灯用
PB2 … 7セグLED点灯用
PB3 … 7セグLED点灯用
PB4 … 7セグLED点灯用
PB5 … 7セグLED点灯用
PB6 … 7セグLED点灯用
PB7 … 7セグLED点灯用 小数点表示兼、タイマーモード表示用LED点灯用

PC0 … 温度センサーアナログ入力
PC1 … 圧電スピーカー
PC2 … LED 保温中表示
PC3 … ボタン CHANGE
PC4 … ボタン UP
PC5 … ボタン DOWN

PD0 … 7セグLEDダイナミック点灯用トランジスタ制御
PD1 … 7セグLEDダイナミック点灯用トランジスタ制御
PD2 … ボタン START
PD3 … LED タイマーセット表示
PD4 … ソリッドステートリレーON/OFF
PD5 … LED 設定温度(TARGET)モード表示
PD6 … LED 現在温度(MONITOR)モード表示
PD7 … LED ヒーターON表示兼、機械式リレーON/OFF

*/

#include<avr/io.h>
#include<util/delay.h>
#include<avr/interrupt.h>
#include<avr/eeprom.h>

#define MODE_TARGET 1
#define MODE_MONITOR 2
#define MODE_TIMER 3

//最大値・最小値の設定
#define TARGET_MAX 50
#define TARGET_MIN 0
#define TIMER_MAX 95
#define TIMER_MIN 0

//設定温度の初期値
#define TARGET_DEFAULT 40;

//ノンアークリレーのON/OFFずらし時間(ミリ秒)
#define ARC_PROTECT_DELAY 200



//---------------------------------
//グローバル変数の宣言
//---------------------------------

//スタートしているか否か
//0→未スタート
//1→スタート
//2→タイマーセット中
unsigned char started = 0;

//表示モード
unsigned char mode = 1;

//保温モードか否か
unsigned char is_keep_mood = 0;

//設定温度
unsigned char target;

//現在の温度
double temp = 0.0;

//現在、設定温度を上回ってるか（=1）下回ってるか（=0）
unsigned char is_over = 0;

//タイマーの時間
unsigned char timer = 0;

//時間計測用変数
unsigned long time_count = 0;

//時間計測用変数が30分を表す数値(≒ミリ秒)
unsigned long half_hour = 1838000;

//スタートLED点滅用(タイマーセット中)時間カウンタ
unsigned char wink_count = 0;

//7セグLEDに表示させる2桁の数字
volatile unsigned int count = 0;

//7セグLEDの0～9を点灯させるビットの配列
unsigned char seg[10] = {0b00111111, 0b00000110,
	0b01011011, 0b01001111,
	0b01100110, 0b01101101,
	0b01111101, 0b00100111,
0b01111111, 0b01101111};

//アラームを鳴らす関数(ピッピー)
void alarm(char num) {
	
	for (char i = 0; i < num; i++){

		PORTC = PORTC | 0b00000010;
		_delay_ms(100);
		PORTC = PORTC & 0b11111101;
		_delay_ms(100);
		PORTC = PORTC | 0b00000010;
		_delay_ms(400);
		PORTC = PORTC & 0b11111101;
		_delay_ms(400);
	}

}

//操作音を鳴らす関数2(ピッ)
void se(char num) {
	
	for (char i = 0; i < num; i++){

		PORTC = PORTC | 0b00000010;
		_delay_ms(50);
		PORTC = PORTC & 0b11111101;
		_delay_ms(50);
	}

}

void display (void) {

	switch( mode ) {

		case MODE_TARGET:
		count = target;
		break;

		case MODE_MONITOR:
		count = temp;
		break;

		case MODE_TIMER:
		count = timer;
		break;

	}
}

ISR (TIMER0_COMPA_vect) {

	static unsigned char sel = 0;
	unsigned char dig1, dig10;

	dig1  = seg[count % 10];
	dig10 = seg[(count / 10) % 10];

	switch( sel ) {

		case 0:
		PORTD = PORTD | 0b00000001;
		PORTD = PORTD & 0b11111101;
		PORTB = dig1;

		//タイマーモードなら"."を表示
		//この信号は7セグに繋がってないので実際には一番右の小数点は点灯しない。
		//しかし、タイマーモード表示用LEDと兼用のため、点灯の信号を送っている。
		if( mode == MODE_TIMER ) {
			PORTB = 0b10000000 | dig1;
			}else{
			PORTB = dig1;
		}

		break;
		case 1:
		PORTD = PORTD | 0b00000010;
		PORTD = PORTD & 0b11111110;

		//タイマーモードなら"."を表示
		if( mode == MODE_TIMER ) {
			PORTB = 0b10000000 | dig10;
			}else{
			PORTB = dig10;
		}
		
		break;

	}

	sel++;

	if( sel == 2 ) {
		sel = 0;
	}
	
	//タイマーセット中なら
	if( started == 2 ) {
		
		time_count += 5;
		
		//time_countが30分の値に達したら
		if( time_count >= half_hour ) {
			//カウントを0に戻し、
			time_count = 0;
			//タイマーの時間を0.5時間減らす
			if ( timer != 0 ) {
				timer -= 5;
			}
			//7セグLEDを表示
			display();
			
			//タイマーの時間が0になったら
			if( timer == 0 ) {
				//スタート
				started = 1;
				PORTD = PORTD | 0b00001000;//ON
			}
		}

		//スタートLEDを点滅させる
		wink_count++;
		if( wink_count == 128 ) {
			wink_count =0;
			if( PORTD == (PORTD & 0b11110111) ) {
				PORTD =   PORTD | 0b00001000;
				}else{
				PORTD =   PORTD & 0b11110111;
			}
		}
	}
	
}

//SSR ON→機械式リレーON→SSR OFFの順にヒーターをONにする関数
void heater_on(void) {
	
	//機械式リレーがOFFなら
	if( (PORTD & 0b10000000) == 0 ) {
		//まずSSRをON
		PORTD = PORTD | 0b00010000;//SSR ON

		//やや遅れて機械式リレーON
		_delay_ms(ARC_PROTECT_DELAY);
		PORTD = PORTD | 0b10000000;//機械式リレーON
		
		//さらにやや遅れてSSRをOFF
		_delay_ms(ARC_PROTECT_DELAY * 2);
		PORTD = PORTD & 0b11101111;//SSR OFF
	}
}

//SSR ON→機械式リレーOFF→SSR OFFの順にヒーターをOFFにする関数
void heater_off(void) {
	
	//機械式リレーがONなら
	if( (PORTD & 0b10000000) != 0 ) {
		//まずSSRをON
		PORTD = PORTD | 0b00010000;//SSR ON
		
		//まず機械式リレーOFF
		_delay_ms(ARC_PROTECT_DELAY);
		PORTD = PORTD & 0b01111111;//機械式リレーOFF

		//さらにやや遅れてSSRをOFF
		_delay_ms(ARC_PROTECT_DELAY);
		PORTD = PORTD & 0b11101111;//SSR OFF
	}
}

//ヒーター(PD4)のON/OFFを制御する関数
void heater_control(void) {
	
	//スタート状態なら
	if( started == 1 ) {
		//現在温度が設定温度より上回っているなら
		if( is_over ) {
			//現在温度が設定温度より低ければ
			if( temp <= ((float)target)  ) {
				heater_on();
				is_over = 0;
				}else{
				heater_off();
			}
			}else{
			//現在温度が設定温度より下回っているなら
			//現在温度が設定温度+0.5℃より低ければ
			if( temp <= ((float)target + 0.5) ) {
				heater_on();
				}else{
				heater_off();
				is_over = 1;
			}
		}
		}else{
		//スタート状態じゃなかったら
		heater_off();
	}
}




//スタートとストップを切り替える関数
void change_start (void) {
	
	//切り替える
	if( started == 0 ) {
		//タイマーに数値がセットされていなければ1、セットされていれば2。
		if( timer == 0 ) {
			started = 1;
			}else{
			started = 2;
			time_count = 0;
		}
		se(1);
		PORTD = PORTD | 0b00001000;//ON

		}else{
		started = 0;
		se(2);
		is_keep_mood = 0;
		PORTC = PORTC & 0b11111011;//保温モードLED OFF
		PORTD = PORTD & 0b11110111;//スタートLED OFF
	}

	//ヒーターのON・OFF制御
	heater_control();
	_delay_ms(300);
}

//モードを切り替える関数
void change_mode (void) {
	
	//モードを切り替える（1→2→3トグル）
	//モード表示用LEDも同時に切り替える。
	switch( mode ) {
		
		case MODE_TARGET:
		mode = MODE_MONITOR;
		PORTD = PORTD & 0b10011111;
		PORTD = PORTD | 0b01000000;
		break;

		case MODE_MONITOR:
		mode = MODE_TIMER;
		//タイマーモードは7セグLEDの「.」表示LEDにつかわれるIOピンで
		//モード表示用LEDも点灯させるため、ここでは点灯設定不要
		PORTD = PORTD & 0b10011111;
		break;
		
		case MODE_TIMER:
		mode = MODE_TARGET;
		PORTD = PORTD & 0b10011111;
		PORTD = PORTD | 0b00100000;
		break;
		
	}

	//7セグLEDを表示
	display();
	_delay_ms(300);
}

//設定値をカウントアップする関数
void count_up (void) {
	
	switch( mode ) {
		
		//設定温度モードなら
		case MODE_TARGET:
		if( target < TARGET_MAX ) {
			target++;

			//EEPROMにデータ保存
			eeprom_busy_wait();/* 読み書き可能になるまで待つ */
			eeprom_write_byte(0x00, target);/* 値0xAAをEEPROMの0番地に書き込む */
		}
		
		break;

		//タイマーモードなら
		case MODE_TIMER:
		if( timer < TIMER_MAX ) {
			timer += 5;
		}
		break;
		
	}

	//7セグLEDを表示
	display();
	_delay_ms(300);
}

//設定値をカウントダウンする関数
void count_down (void) {
	
	switch( mode ) {
		
		//設定温度モードなら
		case MODE_TARGET:
		if( target > TARGET_MIN ) {
			target--;
			
			//EEPROMにデータ保存
			eeprom_busy_wait();/* 読み書き可能になるまで待つ */
			eeprom_write_byte(0x00, target);/* 値0xAAをEEPROMの0番地に書き込む */
		}
		
		break;

		//タイマーモードなら
		case MODE_TIMER:
		if( timer > TIMER_MIN ) {
			timer -= 5;
		}
		break;
		
	}

	//7セグLEDを表示
	display();
	_delay_ms(300);
}

//ボタンが押されたかを検知する関数
void button_sensor (void) {

	//STARTボタンが押されたら
	if( bit_is_clear(PIND, PD2) ) {
		change_start();
	}

	//モード切り替えボタンが押されたら
	if( bit_is_clear(PINC, PC3) ) {
		change_mode();
	}

	//UPボタンが押されたら
	if( bit_is_clear(PINC, PC4) ) {
		count_up();
	}

	//DOWNボタンが押されたら
	if( bit_is_clear(PINC, PC5) ) {
		count_down();
	}
}

//AD変換を利用し温度を測定し、測定した温度(℃)をfloat型の数値で返す関数
float get_temp (void) {

	int x,y;

	//温度測定(初回1回)
	ADMUX  = 0b00001110;
	_delay_ms(5);

	//AD変換開始
	ADCSRA = ADCSRA | 0b01000000;
	//変換中はループ
	while(ADCSRA & 0b01000000);

	y = ADC;

	ADMUX  = 0b00000000;
	_delay_ms(5);

	//AD変換開始
	ADCSRA = ADCSRA | 0b01000000;
	//変換中はループ
	while(ADCSRA & 0b01000000);

	x = ADC;

	return 100 * ( (float)x / (float)y * 1.1 );

}

int main(void) {

	//入出力モード設定
	DDRB  = 0b11111111; //ポートB
	DDRC  = 0b11000110; //ポートC
	DDRD  = 0b11111011; //ポートD
	
	//出力の初期化
	PORTB = 0b00000000; //PBは0番だけを1にすることで内部のプルアップ抵抗を有効にする
	PORTC = 0b00111000; //PCすべてLow
	PORTD = 0b00100100; //PDすべてLow
	
	//タイマー0(7セグダイナミック点灯用タイマー)
	TCCR0A = 0b00000010;//CTCモード
	TCCR0B = 0b00000101;//約1kHz
	OCR0A  = 4;			//5msで割り込み
	TIMSK0 = 0b00000010;//COMPA割り込み

	//AD変換の設定
	ADCSRA = 0b10000100; //62.5kHz
	ADMUX  = 0b00000000; //ADC0 AREF 右

	_delay_ms(5);
	
	sei();//全体の割り込み許可

	//EEPROMから設定温度を読み込み
	eeprom_busy_wait();                /* 読み書き可能になるまで待つ */
	target = eeprom_read_byte(0x00); //EEPROMから(0x00)番地を読み込み
	//設定温度が最大・最小値の外だったら40℃に設定。
	//どうも初回起動時はこの番地(0x00)のメモリの値は大概255になる模様。なのでこれは初回起動時用の設定。
	if( (target < TARGET_MIN) || (target > TARGET_MAX) ) {
		target = TARGET_DEFAULT;
	}

	//7セグLEDを表示
	display();


	//温度測定(起動時に素早く表示するために1回だけ測定する)
	temp = get_temp();

	double z;

	while(1) {
		
		z = 0.0;

		for( unsigned char i = 0; i < 100; i++ ){
			
			//ボタンが押されたかを検知する
			button_sensor();
			
			//計測した温度をz変数に繰り返し加算
			z += get_temp();

		}

		temp = z / 100;

		//ヒーターのON・OFF制御
		heater_control();
		
		//7セグLEDを表示
		display();


		//---------------------------------
		// 設定温度に達したらアラームを鳴らす
		//---------------------------------
		
		//スタートしており、かつ保温モードでなければ
		if( started == 1 && is_over == 1 && (! is_keep_mood) ) {
			
			//アラームを鳴らす
			alarm(3);

			//保温モードにする
			is_keep_mood = 1;
		}

		//---------------------------------
		// 保温モードなら保温LED点灯
		//---------------------------------
		if( is_keep_mood ) {
			PORTC = PORTC | 0b00000100;
			}else{
			PORTC = PORTC & 0b11111011;
		}
	}

	return 0;
}