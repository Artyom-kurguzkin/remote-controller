// working coms

#define BarH_EN PORTBbits.RB0
#define BarL_EN PORTAbits.RA9
#define POT     A1             // Slider POT pin in Arduino. Also RB13
#define IR_EN   PORTBbits.RB7  // infra-red sensor
#define IR_TX   PORTBbits.RB9
#define IR_RX_d PORTBbits.RB1
#define PORTC   LATC
#define SW_R    PORTBbits.RB2  // pushbuttons
#define SW_L    PORTBbits.RB3

#define HALL      PORTAbits.RA0   // Analog hall sensor 
#define BRAKE     PORTAbits.RA1   // Motro Brake 
#define M_PWM     PORTAbits.RA8   // Motor PWM 
#define M_DIR     PORTBbits.RB4   // Motor Direction 
#define E_DIR     PORTBbits.RB5   // Encoder Direction 
#define CHB       PORTBbits.RB14  // Encoder Channel B 
#define CHA       PORTCbits.RC8   // Encoder Channel A 
 
#define IR_PWM    10              // IR_PWM pin in Arduino 
#define Motor_PWM 11              // Motor PWM pin in Arduino 
 
#define S1        SW_L            // S1 same as SW_L  
#define S2        SW_R            // S2 same as SW_R  
 
#define INT2_IF   IFS0bits.INT2IF // INT2 for switch S2 (RB2) 
#define INT4_IF   IFS0bits.INT4IF // INT4 for switch S1 (RB3) 
#define INT1_IF   IFS0bits.INT1IF // INT1 for CHB (RB14)  
#define INT3_IF   IFS0bits.INT3IF // INT3 for CHA (RC8)  
 
#define S2_INT    INT2_IF         // better names for interrupt 
#define S1_INT    INT4_IF 
#define CHB_INT   INT1_IF 
#define CHA_INT   INT3_IF 

#define Buzzer PORTCbits.RC9    
#define IN 1                    
#define OUT 0                   

#define IR_RX_m  PORTBbits.RB8   // Modulate IR Rx  
#define Tx       IR_TX            // Tx refers to IR_TX already defined 
#define Rx       IR_RX_m          // Rx refers to IR_RX_m defined before 


// Button patterns
const unsigned char PATTERN_BOTH = 0b10011110;   
const unsigned char PATTERN_LS1  = 0b10011000;
const unsigned char PATTERN_LS2  = 0b10010110;
const unsigned char PATTERN_HS1  = 0b11111000;
const unsigned char PATTERN_HS2  = 0b11100110;
                

void setup() { 
  TRISC = 0x00; 
  TRISBbits.TRISB0 = OUT;  // BAR_High Enable 
  TRISAbits.TRISA9 = OUT;  // BAR_Low Enable 
  TRISBbits.TRISB13 = IN;  // Slider POT as input (A1) 
  TRISBbits.TRISB7 = OUT;  // IR Enable 
  TRISBbits.TRISB9 = OUT;  // IR Tx 
  TRISBbits.TRISB1 = IN;   // Direct IR Rx 
  TRISBbits.TRISB8 = IN;   // Modulate IR Rx 
  TRISBbits.TRISB2 = IN;   // Right SW 
  TRISBbits.TRISB3 = IN;   // Left SW 
  TRISCbits.TRISC9 = OUT;  // Buzzer     

  TRISBbits.TRISB5 = IN;       // Encoder Direction 
  TRISBbits.TRISB14 = IN;      // Encoder Channel B 
  TRISCbits.TRISC8 = IN;       // Encoder Channel A 

  // Configure LED bar display pins
  TRISBbits.TRISB0 = OUT;      // BarH_EN 
  TRISAbits.TRISA9 = OUT;      // BarL_EN 

  // Configure interrupts - MISSING in a1!
  INTCONbits.INT2EP = 1;       // rising edge for INT2 (S2) 
  INTCONbits.INT4EP = 1;       // rising edge for INT4 (S1) 
  INTCONbits.INT1EP = 0;       // falling edge for INT1 (CHB) 
  INTCONbits.INT3EP = 0;       // falling edge for INT3 (CHA) 
 
  INT2R = 4;                   // map INT2 to RB2  (S2) 
  INT4R = 1;                   // map INT4 to RB3  (S1) 
  INT1R = 1;                   // map INT1 to RB14 (CHB) 
  INT3R = 6;                   // map INT3 to RC8  (CHA)

  Serial.begin( 9600 );

  analogWriteFrequency( 38000 );
  analogWrite( IR_PWM, 32 );      // Further reduced power for less sensitivity      
}


int sound = 0;
int channel = 1;

// Shared timing variables for send/receive synchronization
const int PULSE_WIDTH = 5;         // Start pulse widt
const int PULSE_GAP = 5;           // Gap between start pulse
const int PRE_DATA_DELAY = 15;     // Delay before sending dat
const int BIT_DURATION = 100;      // Duration for each bit (ms) 
const int BIT_TIMEOUT = 125;       // Timeout for receiving bi
const int START_TIMEOUT = 30;      // Timeout for start sequenc

// Button debouncing
void update_button( unsigned char *button_history, bool current_state ) {
  // Receive reference to the history and the current press state
  // If current state is false (button pressed, active low) we add 
  // a 1 to the history.
  *button_history = ( *button_history << 1 ) | ( current_state ? 1 : 0 ); 
}

bool is_button_down( unsigned char button_history ) {
  return ( button_history == 0xFF );
}



void bar_h( byte num ){
  BarH_EN = 1;
  PORTC = num;
  BarH_EN = 0;
 
}


void bar_l( byte num ){
  BarL_EN = 1;
  PORTC = num;
  BarL_EN = 0;
  Serial.println( num );
}  


void bar( int value ) {
  unsigned char high = value / 256; // high byte
  unsigned char low  = value % 256; // low byte

  bar_h( high );
  bar_l( low );
}


int get_bar_pattern( int num ) {
  static int pattern [ 17 ] = {
    0b0000000000000000, // 0
    0b0000000000000001, // 1
    0b0000000000000011, // 2
    0b0000000000000111, // 3
    0b0000000000001111, // 4
    0b0000000000011111, // 5
    0b0000000000111111, // 6
    0b0000000001111111, // 7
    0b0000000011111111, // 8
    0b0000000111111111, // 9
    0b0000001111111111, // 10
    0b0000011111111111, // 11
    0b0000111111111111, // 12
    0b0001111111111111, // 13
    0b0011111111111111, // 14
    0b0111111111111111, // 15
    0b1111111111111111  // 16 (0xFFFF)
  };

  if ( num < 0 ) return pattern[ 0 ];
  if ( num > 16 ) return pattern[ 16 ];

  return pattern [ num ];
}


void Bargraph ( int num ) {
  bar ( get_bar_pattern( num ) );
}


void beep ( unsigned char Tone, int cycles ) {
  for ( int i = 0; i < cycles; i++ ) {
    Buzzer = 1;                     // Set buzzer HIGH
    delayMicroseconds( 10 * Tone );   // Delay for half period
    Buzzer = 0;                     // Set buzzer LOW
    delayMicroseconds( 10 * Tone );   // Delay for half period
  }
}


void send_code( unsigned char pattern ) {
  // Send start signal: 2 short pulses to indicate transmission beginning
  for ( int i = 0; i < 2; i++ ) {
    Tx = 0;  // Send signal
    delay( PULSE_WIDTH );  // Use shared timing variable
    Tx = 1;  // Stop signal
    delay( PULSE_GAP );    // Use shared timing variable
  }
  
  // Wait before sending actual data to give receiver time to sync
  delay( PRE_DATA_DELAY );  // Use shared timing variable
  
  unsigned char mask = 0b10000000;  // Start with MSB mask
  
  for ( int i = 0; i < 8; i++ ) {
    // Check the current bit of the pattern using the mask
    if ( pattern & mask ) {
      // Bit is 1: send 1 to Tx (deactivate)
      Tx = 1;
    } else {
      // Bit is 0: send 0 to Tx (activate)
      Tx = 0;
    }
    
    // Wait for bit duration
    delay( BIT_DURATION );  // Use shared timing variable
    
    // Shift the mask one bit to the right for the next bit
    mask = mask >> 1;
  }
  
  // Two stop bits
  Tx = 1; delay( BIT_DURATION );
  Tx = 1; delay( BIT_DURATION );
  
  // After sending all bits, return Tx to idle state (deactivated)
  Tx = 1;
}


void listen_for_code( unsigned long current_time ) {
  static unsigned char received_pattern = 0;
  static int bit_count = 0;
  static unsigned long last_bit_time = 0;
  static bool receiving = false;
  static unsigned long last_beep_time = 0;
  static int start_pulse_count = 0;
  static unsigned long last_pulse_time = 0;
  static bool last_rx_state = true;
  static bool waiting_for_data = false;
  
  const unsigned long MIN_BEEP_INTERVAL = 500;
  bool current_rx_state = ( Rx == 0 );
  
  // Detect start sequence
  if ( !receiving && !waiting_for_data ) {
    if ( current_rx_state && !last_rx_state ) {
      if ( start_pulse_count == 0 || ( current_time - last_pulse_time ) < 100 ) { // Use fixed tolerance
        start_pulse_count++;
        last_pulse_time = current_time;
        
        if ( start_pulse_count >= 2 ) {
          waiting_for_data = true;
          start_pulse_count = 0;
          last_bit_time = current_time;
        }
      } else {
        start_pulse_count = 1;
        last_pulse_time = current_time;
      }
    }
    
    if ( start_pulse_count > 0 && ( current_time - last_pulse_time ) > START_TIMEOUT ) {
      start_pulse_count = 0;
    }
  }
  
  // Start receiving data
  if ( waiting_for_data && !receiving ) {
    if ( current_time - last_bit_time >= PRE_DATA_DELAY ) {  // Use shared timing variable
      receiving = true;
      waiting_for_data = false;
      received_pattern = 0;
      bit_count = 0;
      last_bit_time = current_time;
    }
  }
  
  // Receive bits
  if ( receiving && ( current_time - last_bit_time ) >= BIT_DURATION ) {  // Use shared timing variable
    if ( bit_count < 10 ) {  // 8 data + 2 stop bits
      if ( bit_count < 8 ) {
        int bit_value = current_rx_state ? 0 : 1;
        received_pattern = ( received_pattern << 1 ) | bit_value;
      }
      bit_count++;
      last_bit_time = current_time;
    } else {
      receiving = false;
      
      // Use the received pattern directly as beep frequency
      if ( current_time - last_beep_time >= MIN_BEEP_INTERVAL ) {
        // Use the received pattern value directly as the tone parameter
        // Scale it to a reasonable frequency range (add offset to avoid zero)
        unsigned char tone_freq = received_pattern / 4 + 20;
        beep( tone_freq, 100 );
        last_beep_time = current_time;
      }

      // For TV control - update variables only, no display
      if ( received_pattern == PATTERN_LS1 ) {  // decrease vollume
        if (sound > 0) sound--;
      } else if ( received_pattern == PATTERN_LS2 ) { // increase volume
        if ( sound < 16 ) sound++;
      } else if (received_pattern == PATTERN_HS1) { // decrease channel
        if ( channel > 1 ) { channel -= 1; } else { channel = 20; } 
      } else if ( received_pattern == PATTERN_HS2 ) { // increase channel
        if ( channel < 20 ) { channel += 1; } else { channel = 1; }
      } else {
        // Assume any other pattern is a pot value (0-16 range)
        if ( received_pattern <= 16 ) {
          sound = received_pattern; // Store pot value in sound variable
        }
      }
    }
  }
  
  // Reset on timeout
  if ( ( receiving || waiting_for_data ) && ( current_time - last_bit_time ) > ( BIT_DURATION + BIT_TIMEOUT ) ) {
    receiving = false;
    waiting_for_data = false;
    start_pulse_count = 0;
  }
  
  last_rx_state = current_rx_state;
}


void loop() {
  unsigned long current_time = millis();
  
  static bool transmitting = false;
  static unsigned long transmission_end_time = 0;

  // Button history for debouncing 
  static unsigned char s1_history = 0xFF; // Start assuming "up" state
  static unsigned char s2_history = 0xFF;
  
  // For TV 
  /*   */
  Bargraph( sound );
  Serial.print( "Sound Level: " );
  Serial.print( sound );
  Serial.print( " Channel: " );
  Serial.print( channel );
  Serial.println();

  // Read current button states (active-low)
  bool s1_current = ( S1 == 0 );
  bool s2_current = ( S2 == 0 );
  
  // Update button histories with current states
  update_button( &s1_history, s1_current );
  update_button( &s2_history, s2_current );


  // Check if transmission is complete
  if ( transmitting && current_time >= transmission_end_time ) {
    transmitting = false;
  }

  // Handle button presses (only transmit when not already transmitting)
  if ( !transmitting ) {
    unsigned char pattern = 0;
    bool should_transmit = false;
    int pot_reading = ( analogRead( A1 ) * 16 ) / 1023;
    
    // Check for simultaneous button press (both buttons currently held down)
    if ( is_button_down( s1_history ) && is_button_down( s2_history ) ) {
      // Both buttons held down - send pot value
      pattern = pot_reading;
      should_transmit = true;
    }
    else {
      // Handle individual button press events using interrupt flags
      
      // S1 press
      if ( S1_INT ) {
        if( pot_reading > 8 ) {
          pattern = PATTERN_HS1;
        }
        else {
          pattern = PATTERN_LS1;
        }   
        should_transmit = true;
        S1_INT = 0;  // Clear interrupt flag
      }
      
      if ( S2_INT ) {
        if( pot_reading > 8 ) {
          pattern = PATTERN_HS2;
        }
        else {
          pattern = PATTERN_LS2;
        }
        should_transmit = true;
        S2_INT = 0;
      }
    }

    if ( should_transmit ) {
      transmitting = true;
      transmission_end_time = current_time + 1000;
      send_code(pattern);
      delay( 10 ); // Minimal post-transmission delay
    }
  }
  
  // Listen for incoming codes when not transmitting
  if ( !transmitting ) {
    listen_for_code( current_time );
  }
  
  delay( 2 ); 
}