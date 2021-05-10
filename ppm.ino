#include <Servo_Rc_PWM.h>
Servo servo_roll, servo_pitch;

#define AVR_RC_INPUT_NUM_CHANNELS 10

volatile uint16_t _pulse_capt[AVR_RC_INPUT_NUM_CHANNELS] = {0};
volatile uint8_t _valid = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  servo_pitch.attach(12, 1200, 1800, 1500);
  servo_pitch.writeMicroseconds(1500);
  inits();
}

void loop() {
  // put your main code here, to run repeatedly:
  servo_pitch.writeMicroseconds(_pulse_capt[0]);
  Serial.println(_pulse_capt[0]);
}


void inits(void) {
  /* Arduino pin 48 is ICP1 / PL1,  timer 1 input capture */
  pinMode(8, INPUT);
  /**
     WGM: 1 1 1 1. Fast WPM, TOP is in OCR1A
     COM all disabled
     CS11: prescale by 8 => 0.5us tick
     ICES1: input capture on rising edge
     OCR1A: 40000, 0.5us tick => 2ms period / 50hz freq for outbound
     fast PWM.
  */
  TCCR1A = _BV(WGM10) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11) | _BV(ICES1);
  OCR1A  = 40000;

  /* OCR1B and OCR1C will be used by RCOutput_APM2. init to nil output
    OCR1B  = 0xFFFF;
    OCR1C  = 0xFFFF;
  */
  /* Enable input capture interrupt */
  TIMSK1 |= _BV(ICIE1);
}


/* private callback for input capture ISR */
ISR(TIMER1_CAPT_vect) {

  static uint16_t icr1_prev;
  static uint8_t  channel_ctr;
  const uint16_t icr1_current = ICR1;
  uint16_t pulse_width;

  if (icr1_current < icr1_prev) {
    /* ICR1 rolls over at TOP=40000 */
    pulse_width = icr1_current + 40000 - icr1_prev;
  } else {
    pulse_width = icr1_current - icr1_prev;
  }

  if (pulse_width > 8000) {
    /* sync pulse detected */
    channel_ctr = 0;
  } else {
    if (channel_ctr < AVR_RC_INPUT_NUM_CHANNELS) {
      _pulse_capt[channel_ctr] = (pulse_width >> 1);
      channel_ctr++;
      if (channel_ctr == AVR_RC_INPUT_NUM_CHANNELS) {
        _valid = AVR_RC_INPUT_NUM_CHANNELS;
      }
    }
  }
  icr1_prev = icr1_current;
}
