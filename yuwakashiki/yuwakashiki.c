/*

�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\
�� I/O�s���ݒ�
�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\�\

PB0 �c 7�Z�OLED�_���p
PB1 �c 7�Z�OLED�_���p
PB2 �c 7�Z�OLED�_���p
PB3 �c 7�Z�OLED�_���p
PB4 �c 7�Z�OLED�_���p
PB5 �c 7�Z�OLED�_���p
PB6 �c 7�Z�OLED�_���p
PB7 �c 7�Z�OLED�_���p �����_�\�����A�^�C�}�[���[�h�\���pLED�_���p

PC0 �c ���x�Z���T�[�A�i���O����
PC1 �c ���d�X�s�[�J�[
PC2 �c LED �ۉ����\��
PC3 �c �{�^�� CHANGE
PC4 �c �{�^�� UP
PC5 �c �{�^�� DOWN

PD0 �c 7�Z�OLED�_�C�i�~�b�N�_���p�g�����W�X�^����
PD1 �c 7�Z�OLED�_�C�i�~�b�N�_���p�g�����W�X�^����
PD2 �c �{�^�� START
PD3 �c LED �^�C�}�[�Z�b�g�\��
PD4 �c �\���b�h�X�e�[�g�����[ON/OFF
PD5 �c LED �ݒ艷�x(TARGET)���[�h�\��
PD6 �c LED ���݉��x(MONITOR)���[�h�\��
PD7 �c LED �q�[�^�[ON�\�����A�@�B�������[ON/OFF

*/

#include<avr/io.h>
#include<util/delay.h>
#include<avr/interrupt.h>
#include<avr/eeprom.h>

#define MODE_TARGET 1
#define MODE_MONITOR 2
#define MODE_TIMER 3

//�ő�l�E�ŏ��l�̐ݒ�
#define TARGET_MAX 50
#define TARGET_MIN 0
#define TIMER_MAX 95
#define TIMER_MIN 0

//�ݒ艷�x�̏����l
#define TARGET_DEFAULT 40;

//�m���A�[�N�����[��ON/OFF���炵����(�~���b)
#define ARC_PROTECT_DELAY 200



//---------------------------------
//�O���[�o���ϐ��̐錾
//---------------------------------

//�X�^�[�g���Ă��邩�ۂ�
//0�����X�^�[�g
//1���X�^�[�g
//2���^�C�}�[�Z�b�g��
unsigned char started = 0;

//�\�����[�h
unsigned char mode = 1;

//�ۉ����[�h���ۂ�
unsigned char is_keep_mood = 0;

//�ݒ艷�x
unsigned char target;

//���݂̉��x
double temp = 0.0;

//���݁A�ݒ艷�x�������Ă邩�i=1�j������Ă邩�i=0�j
unsigned char is_over = 0;

//�^�C�}�[�̎���
unsigned char timer = 0;

//���Ԍv���p�ϐ�
unsigned long time_count = 0;

//���Ԍv���p�ϐ���30����\�����l(���~���b)
unsigned long half_hour = 1838000;

//�X�^�[�gLED�_�ŗp(�^�C�}�[�Z�b�g��)���ԃJ�E���^
unsigned char wink_count = 0;

//7�Z�OLED�ɕ\��������2���̐���
volatile unsigned int count = 0;

//7�Z�OLED��0�`9��_��������r�b�g�̔z��
unsigned char seg[10] = {0b00111111, 0b00000110,
	0b01011011, 0b01001111,
	0b01100110, 0b01101101,
	0b01111101, 0b00100111,
0b01111111, 0b01101111};

//�A���[����炷�֐�(�s�b�s�[)
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

//���쉹��炷�֐�2(�s�b)
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

		//�^�C�}�[���[�h�Ȃ�"."��\��
		//���̐M����7�Z�O�Ɍq�����ĂȂ��̂Ŏ��ۂɂ͈�ԉE�̏����_�͓_�����Ȃ��B
		//�������A�^�C�}�[���[�h�\���pLED�ƌ��p�̂��߁A�_���̐M���𑗂��Ă���B
		if( mode == MODE_TIMER ) {
			PORTB = 0b10000000 | dig1;
			}else{
			PORTB = dig1;
		}

		break;
		case 1:
		PORTD = PORTD | 0b00000010;
		PORTD = PORTD & 0b11111110;

		//�^�C�}�[���[�h�Ȃ�"."��\��
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
	
	//�^�C�}�[�Z�b�g���Ȃ�
	if( started == 2 ) {
		
		time_count += 5;
		
		//time_count��30���̒l�ɒB������
		if( time_count >= half_hour ) {
			//�J�E���g��0�ɖ߂��A
			time_count = 0;
			//�^�C�}�[�̎��Ԃ�0.5���Ԍ��炷
			if ( timer != 0 ) {
				timer -= 5;
			}
			//7�Z�OLED��\��
			display();
			
			//�^�C�}�[�̎��Ԃ�0�ɂȂ�����
			if( timer == 0 ) {
				//�X�^�[�g
				started = 1;
				PORTD = PORTD | 0b00001000;//ON
			}
		}

		//�X�^�[�gLED��_�ł�����
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

//SSR ON���@�B�������[ON��SSR OFF�̏��Ƀq�[�^�[��ON�ɂ���֐�
void heater_on(void) {
	
	//�@�B�������[��OFF�Ȃ�
	if( (PORTD & 0b10000000) == 0 ) {
		//�܂�SSR��ON
		PORTD = PORTD | 0b00010000;//SSR ON

		//���x��ċ@�B�������[ON
		_delay_ms(ARC_PROTECT_DELAY);
		PORTD = PORTD | 0b10000000;//�@�B�������[ON
		
		//����ɂ��x���SSR��OFF
		_delay_ms(ARC_PROTECT_DELAY * 2);
		PORTD = PORTD & 0b11101111;//SSR OFF
	}
}

//SSR ON���@�B�������[OFF��SSR OFF�̏��Ƀq�[�^�[��OFF�ɂ���֐�
void heater_off(void) {
	
	//�@�B�������[��ON�Ȃ�
	if( (PORTD & 0b10000000) != 0 ) {
		//�܂�SSR��ON
		PORTD = PORTD | 0b00010000;//SSR ON
		
		//�܂��@�B�������[OFF
		_delay_ms(ARC_PROTECT_DELAY);
		PORTD = PORTD & 0b01111111;//�@�B�������[OFF

		//����ɂ��x���SSR��OFF
		_delay_ms(ARC_PROTECT_DELAY);
		PORTD = PORTD & 0b11101111;//SSR OFF
	}
}

//�q�[�^�[(PD4)��ON/OFF�𐧌䂷��֐�
void heater_control(void) {
	
	//�X�^�[�g��ԂȂ�
	if( started == 1 ) {
		//���݉��x���ݒ艷�x�������Ă���Ȃ�
		if( is_over ) {
			//���݉��x���ݒ艷�x���Ⴏ���
			if( temp <= ((float)target)  ) {
				heater_on();
				is_over = 0;
				}else{
				heater_off();
			}
			}else{
			//���݉��x���ݒ艷�x��艺����Ă���Ȃ�
			//���݉��x���ݒ艷�x+0.5�����Ⴏ���
			if( temp <= ((float)target + 0.5) ) {
				heater_on();
				}else{
				heater_off();
				is_over = 1;
			}
		}
		}else{
		//�X�^�[�g��Ԃ���Ȃ�������
		heater_off();
	}
}




//�X�^�[�g�ƃX�g�b�v��؂�ւ���֐�
void change_start (void) {
	
	//�؂�ւ���
	if( started == 0 ) {
		//�^�C�}�[�ɐ��l���Z�b�g����Ă��Ȃ����1�A�Z�b�g����Ă����2�B
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
		PORTC = PORTC & 0b11111011;//�ۉ����[�hLED OFF
		PORTD = PORTD & 0b11110111;//�X�^�[�gLED OFF
	}

	//�q�[�^�[��ON�EOFF����
	heater_control();
	_delay_ms(300);
}

//���[�h��؂�ւ���֐�
void change_mode (void) {
	
	//���[�h��؂�ւ���i1��2��3�g�O���j
	//���[�h�\���pLED�������ɐ؂�ւ���B
	switch( mode ) {
		
		case MODE_TARGET:
		mode = MODE_MONITOR;
		PORTD = PORTD & 0b10011111;
		PORTD = PORTD | 0b01000000;
		break;

		case MODE_MONITOR:
		mode = MODE_TIMER;
		//�^�C�}�[���[�h��7�Z�OLED�́u.�v�\��LED�ɂ�����IO�s����
		//���[�h�\���pLED���_�������邽�߁A�����ł͓_���ݒ�s�v
		PORTD = PORTD & 0b10011111;
		break;
		
		case MODE_TIMER:
		mode = MODE_TARGET;
		PORTD = PORTD & 0b10011111;
		PORTD = PORTD | 0b00100000;
		break;
		
	}

	//7�Z�OLED��\��
	display();
	_delay_ms(300);
}

//�ݒ�l���J�E���g�A�b�v����֐�
void count_up (void) {
	
	switch( mode ) {
		
		//�ݒ艷�x���[�h�Ȃ�
		case MODE_TARGET:
		if( target < TARGET_MAX ) {
			target++;

			//EEPROM�Ƀf�[�^�ۑ�
			eeprom_busy_wait();/* �ǂݏ����\�ɂȂ�܂ő҂� */
			eeprom_write_byte(0x00, target);/* �l0xAA��EEPROM��0�Ԓn�ɏ������� */
		}
		
		break;

		//�^�C�}�[���[�h�Ȃ�
		case MODE_TIMER:
		if( timer < TIMER_MAX ) {
			timer += 5;
		}
		break;
		
	}

	//7�Z�OLED��\��
	display();
	_delay_ms(300);
}

//�ݒ�l���J�E���g�_�E������֐�
void count_down (void) {
	
	switch( mode ) {
		
		//�ݒ艷�x���[�h�Ȃ�
		case MODE_TARGET:
		if( target > TARGET_MIN ) {
			target--;
			
			//EEPROM�Ƀf�[�^�ۑ�
			eeprom_busy_wait();/* �ǂݏ����\�ɂȂ�܂ő҂� */
			eeprom_write_byte(0x00, target);/* �l0xAA��EEPROM��0�Ԓn�ɏ������� */
		}
		
		break;

		//�^�C�}�[���[�h�Ȃ�
		case MODE_TIMER:
		if( timer > TIMER_MIN ) {
			timer -= 5;
		}
		break;
		
	}

	//7�Z�OLED��\��
	display();
	_delay_ms(300);
}

//�{�^���������ꂽ�������m����֐�
void button_sensor (void) {

	//START�{�^���������ꂽ��
	if( bit_is_clear(PIND, PD2) ) {
		change_start();
	}

	//���[�h�؂�ւ��{�^���������ꂽ��
	if( bit_is_clear(PINC, PC3) ) {
		change_mode();
	}

	//UP�{�^���������ꂽ��
	if( bit_is_clear(PINC, PC4) ) {
		count_up();
	}

	//DOWN�{�^���������ꂽ��
	if( bit_is_clear(PINC, PC5) ) {
		count_down();
	}
}

//AD�ϊ��𗘗p�����x�𑪒肵�A���肵�����x(��)��float�^�̐��l�ŕԂ��֐�
float get_temp (void) {

	int x,y;

	//���x����(����1��)
	ADMUX  = 0b00001110;
	_delay_ms(5);

	//AD�ϊ��J�n
	ADCSRA = ADCSRA | 0b01000000;
	//�ϊ����̓��[�v
	while(ADCSRA & 0b01000000);

	y = ADC;

	ADMUX  = 0b00000000;
	_delay_ms(5);

	//AD�ϊ��J�n
	ADCSRA = ADCSRA | 0b01000000;
	//�ϊ����̓��[�v
	while(ADCSRA & 0b01000000);

	x = ADC;

	return 100 * ( (float)x / (float)y * 1.1 );

}

int main(void) {

	//���o�̓��[�h�ݒ�
	DDRB  = 0b11111111; //�|�[�gB
	DDRC  = 0b11000110; //�|�[�gC
	DDRD  = 0b11111011; //�|�[�gD
	
	//�o�͂̏�����
	PORTB = 0b00000000; //PB��0�Ԃ�����1�ɂ��邱�Ƃœ����̃v���A�b�v��R��L���ɂ���
	PORTC = 0b00111000; //PC���ׂ�Low
	PORTD = 0b00100100; //PD���ׂ�Low
	
	//�^�C�}�[0(7�Z�O�_�C�i�~�b�N�_���p�^�C�}�[)
	TCCR0A = 0b00000010;//CTC���[�h
	TCCR0B = 0b00000101;//��1kHz
	OCR0A  = 4;			//5ms�Ŋ��荞��
	TIMSK0 = 0b00000010;//COMPA���荞��

	//AD�ϊ��̐ݒ�
	ADCSRA = 0b10000100; //62.5kHz
	ADMUX  = 0b00000000; //ADC0 AREF �E

	_delay_ms(5);
	
	sei();//�S�̂̊��荞�݋���

	//EEPROM����ݒ艷�x��ǂݍ���
	eeprom_busy_wait();                /* �ǂݏ����\�ɂȂ�܂ő҂� */
	target = eeprom_read_byte(0x00); //EEPROM����(0x00)�Ԓn��ǂݍ���
	//�ݒ艷�x���ő�E�ŏ��l�̊O��������40���ɐݒ�B
	//�ǂ�������N�����͂��̔Ԓn(0x00)�̃������̒l�͑�T255�ɂȂ�͗l�B�Ȃ̂ł���͏���N�����p�̐ݒ�B
	if( (target < TARGET_MIN) || (target > TARGET_MAX) ) {
		target = TARGET_DEFAULT;
	}

	//7�Z�OLED��\��
	display();


	//���x����(�N�����ɑf�����\�����邽�߂�1�񂾂����肷��)
	temp = get_temp();

	double z;

	while(1) {
		
		z = 0.0;

		for( unsigned char i = 0; i < 100; i++ ){
			
			//�{�^���������ꂽ�������m����
			button_sensor();
			
			//�v���������x��z�ϐ��ɌJ��Ԃ����Z
			z += get_temp();

		}

		temp = z / 100;

		//�q�[�^�[��ON�EOFF����
		heater_control();
		
		//7�Z�OLED��\��
		display();


		//---------------------------------
		// �ݒ艷�x�ɒB������A���[����炷
		//---------------------------------
		
		//�X�^�[�g���Ă���A���ۉ����[�h�łȂ����
		if( started == 1 && is_over == 1 && (! is_keep_mood) ) {
			
			//�A���[����炷
			alarm(3);

			//�ۉ����[�h�ɂ���
			is_keep_mood = 1;
		}

		//---------------------------------
		// �ۉ����[�h�Ȃ�ۉ�LED�_��
		//---------------------------------
		if( is_keep_mood ) {
			PORTC = PORTC | 0b00000100;
			}else{
			PORTC = PORTC & 0b11111011;
		}
	}

	return 0;
}