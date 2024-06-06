/*
 * 30 Days - Lost in Space
 * Day 20 - Creative Day - Alarm clock
 *
 * Rob Allen
 */

/*
 * Features:
 * - Set time
 * - Select military/normal
 * - Keep time
 * - Set alarm time
 * - Sound alarm and be able to silence
 */

// Explicitly include Arduino.h
#include "Arduino.h"

// Include TM1637 library file
#include <TM1637Display.h>

// Include BasicEncoder library file
#include <BasicEncoder.h>

// The Rotary Encoder will be used to control the clock using
// interrupts (explained below).  Since the HERO board only supports interrupts on
// pins 2 and 3, we MUST use those pins for rotary encoder inputs.
const byte CLOCK_CONTROL_CLK_PIN = 2;  // HERO interrupt pin connected to encoder CLK input
const byte CLOCK_CONTROL_DT_PIN = 3;   // HERO interrupt pin connected to encoder DT input
const byte CLOCK_CONTROL_PSH_BUT_PIN = 8;  // Push button input to control the clock

// Create BasicEncoder instance for our clock control inputs
BasicEncoder clock_control(CLOCK_CONTROL_CLK_PIN, CLOCK_CONTROL_DT_PIN);

// Define the display connection pins:
const byte DISPLAY_CLK_PIN = 6;
const byte DISPLAY_DIO_PIN = 5;

// Our TM1637 4-digit 7-segment display will be used as our clock display.
TM1637Display clock_display = TM1637Display(DISPLAY_CLK_PIN, DISPLAY_DIO_PIN);

const byte BUZZER_PIN = 10;   // Alert buzzer

const byte BLINK_COUNT = 3;   // blink depth gauge this many times for attention.

// You can set the individual segments per digit to spell words or create other symbols:
//const byte done[] = {
//  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,          // d
//  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // O
//  SEG_C | SEG_E | SEG_G,                          // n
//  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G           // E
//};

unsigned int clk_hours;
unsigned int clk_minutes;
unsigned int clk_seconds;

// function prototypes
void updateEncoder();
int selectMode();

void setup() 
{
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CLOCK_CONTROL_PSH_BUT_PIN, INPUT);

  // Setup Serial Monitor
  Serial.begin(9600);
  delay(1000);

  clock_display.setBrightness(7);  // Set depth gauge brightness to max (values 0-7)

  // Call Interrupt Service Routine (ISR) updateEncoder() when any high/low change
  // is seen on A (CLOCK_CONTROL_CLK_PIN) interrupt  (pin 2), or B (CLOCK_CONTROL_DT_PIN) interrupt (pin 3)
  attachInterrupt(digitalPinToInterrupt(CLOCK_CONTROL_CLK_PIN), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLOCK_CONTROL_DT_PIN), updateEncoder, CHANGE);

  clk_hours = 12;
  clk_minutes = 0;
  clk_seconds = 0;
}

enum
{
  DISPLAY_TIME_E,   // Display the current time
  SELECT_MODE_E,    // Enter mode selection
  SET_TIME_E        // Set Time Mode
};

void loop() 
{
  static int mode = DISPLAY_TIME_E;
  int display_time = 0;
  bool button_held = false;

  // mangage the time
  updateTime();

  switch( mode )
  {
    case DISPLAY_TIME_E:
      display_time = ( clk_hours * 100 ) + clk_minutes;
      clock_display.showNumberDecEx(display_time, 0b01000000);

      // Only way to leave this mode is by long button press
      button_held = buttonLongPress();

      if( button_held == true )
      {
        mode = SELECT_MODE_E;
      }

      break;

    case SELECT_MODE_E:   // Select what mode we should be in
      mode = selectMode();
      break;

    case SET_TIME_E:
      setTime();

      mode = DISPLAY_TIME_E;  // Go back to display time after setting time
      break;

    default:
      mode = DISPLAY_TIME_E;
      break;
  }
}

// Update time
void updateTime()
{
  // Initialize our time measurement
  unsigned long current_time;
  static unsigned long accum_time = 0;
  static unsigned long prev_time = 0;

  // Get current time
  current_time = millis();
  
  // Check for time rollover 
  if( current_time < prev_time )
  {
    accum_time += (0xFFFFFFFF - prev_time) + current_time;
  }
  else
  {
    accum_time += ( current_time - prev_time );
  }

  prev_time = current_time;

  // Check if accumulated time is more than 1000 msec, if so, update clock time
  if( accum_time > 1000 )
  {
    // add the integer number of seconds that have passed
    clk_seconds += accum_time / 1000;

    // keep the leftover to keep accurate time
    accum_time = accum_time % 1000;

    // Check for second roll over
    if( clk_seconds > 59 )
    {
      ++clk_minutes;
      clk_seconds = 0;

      // Check for minutes roll over
      if( clk_minutes > 59 )
      {
        ++clk_hours;
        clk_minutes = 0;

        // Check for hours roll over
        if( clk_hours > 23 )
        {
          clk_hours = 0;
        }
      }
    }
  }

  return;
}
/*****************************************************************/
// returns true if button has been pressed and held for 3 seconds
/*****************************************************************/
bool buttonLongPress()
{
  static bool first_call = true;
  static unsigned long button_hld_time;
  static unsigned long prev_time = 0;
  unsigned long time_since_last;

  int button_pressed;

  unsigned long current_time;

  bool button_held = false;

  // Get current time
  current_time = millis();

  // initialize button_hld_time if this is first call
  if( first_call == true )
  {
    button_hld_time = 0;

    first_call = false;
  }

  // Read current button state
  button_pressed = digitalRead( CLOCK_CONTROL_PSH_BUT_PIN );

  if(( button_pressed == LOW ) &&
     ( button_hld_time == 0 ))
  {
    // This is the first cycle of a press, start accumulating time by adding 1
    ++button_hld_time;

    // Also set prev_time to current_time
    prev_time = current_time;
  }
  else if( button_pressed == LOW )
  {
    // Compute time since last
    time_since_last = current_time - prev_time;
    prev_time = current_time;

    // button_hld_time is non-zero, so add time_since_last
    button_hld_time += time_since_last;

    // Check to see if button has been pressed for 3+ seconds
    if( button_hld_time > 3000 )
    {
      button_held = true;
      
      // Reset the button hold time
      button_hld_time = 0;
    }
    else
    {
      button_held = false;
    }
  }
  else
  {
    // button is NOT pressed
    button_hld_time = 0;
    button_held = false;
  }    
  
  return button_held;
}

/*****************************************************************/
/*****************************************************************/
const byte set_word[] = 
{
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,            // S
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,            // E
  SEG_A | SEG_E | SEG_F,                            // T
  0
};

const byte exit_word[] =
{
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,            // E
  SEG_A | SEG_D | SEG_G,                            // X
  SEG_E | SEG_F,                                    // I
  SEG_A | SEG_E | SEG_F,                            // T
};

// selectMode() is entered by a long press of the encoder switch
// and allows selection between the different modes:
//  DISPLAY_TIME_E,   // Display the current time
//  SELECT_MODE_E,    // Enter mode selection
//  SET_TIME_E,       // Set Time Mode
int selectMode()
{
  int current_mode = 0;
  int control_direction = 0;

  // display the initial 
  byte *display_words[] = { set_word, exit_word };
  int mode_return[] = { SET_TIME_E, DISPLAY_TIME_E };

  clock_display.setSegments( display_words[current_mode] );
  
  // Reset the control encoder
  clock_control.reset();

  // We get here because the button was being held down for 3+ seconds.
  // Wait until it is released before selecting the mode
  while(!digitalRead(CLOCK_CONTROL_PSH_BUT_PIN));

  // Stay in loop until mode is selecte by button press
  while(1)
  {
    if( clock_control.get_change())
    {
      // If there has been a change, determine which direction to select what to dispaly
      if( clock_control.get_count() > control_direction )
      {
        ++current_mode;
        if( current_mode > 3 )
          current_mode = 0;
      }
      else
      {
        --current_mode;
        if( current_mode < 0 )
          current_mode = 3;
      }

      control_direction = clock_control.get_count();

      clock_display.setSegments( display_words[current_mode] );
    }

    // Check the encoder switch to see if something has been selected
    if( !digitalRead(CLOCK_CONTROL_PSH_BUT_PIN))
    {
      Serial.print("Selected Mode: ");
      Serial.println(mode_return[current_mode]);
      break;
    }
  };

  // We get here because the button was pressed to select a mode.
  // Wait until it is released before returning.
  while(!digitalRead(CLOCK_CONTROL_PSH_BUT_PIN));

  return mode_return[current_mode];
}

/*****************************************************************/
// This function allows the user to set the time in stages:
//    1. Set the hours - clkwise + / cntr clkwise -
//    2. Depress switch to accept
//    3. Set the minutes - clkwise + / cntr clkwise -
//    4. Depress switch to accept - exits automatically
/*****************************************************************/
const byte blank_2_digits[] = 
{
  0,
  0
};

void setTime()
{
  bool on_off = false;      // on = true / off = false
  bool hrs_or_mins = false; // hrs = false / min = true
  bool exit = false;        // used to control the loop
  unsigned long current_time;
  unsigned long prev_time;
  unsigned long blink_time;

  int tmp_hours;
  int tmp_minutes;
  
  int encoder_count = 0;
 
  // Set temporary time to current clock time, except seconds
  tmp_hours = clk_hours;
  tmp_minutes = clk_minutes;
  
  // Display the temporary time
  clock_display.showNumberDecEx( tmp_hours, 0b01000000, false, 2, 0);
  clock_display.showNumberDecEx( tmp_minutes, 0b01000000, true, 2, 2);

  // Reset the control encoder
  clock_control.reset();

 // Get current time
  prev_time = millis();
  
  // Loop while setting the time
  while( exit == false )
  {
    // See if it's time to blink the display
    current_time = millis();
    blink_time = current_time - prev_time;

    // Check if it's time to blink the display on or off
    if( blink_time > 500 )
    {
      prev_time = current_time;

      // on if true / off if false
      if( on_off == false )
      {
        // Set state for next blink period
        on_off = true;

        // Are we blanking hrs or minutes
        // hrs = false / min = true
        if( hrs_or_mins == false )
        {
          clock_display.setSegments( blank_2_digits, 2, 0 );
        }
        else
        {
          clock_display.setSegments( blank_2_digits, 2, 2 );
        }
      }
      else
      {
        // Set state for next blink period
        on_off = false;
        
        // Update hours
        clock_display.showNumberDecEx( tmp_hours, 0b01000000, false, 2, 0);

        // Update minutes
        clock_display.showNumberDecEx( tmp_minutes, 0b01000000, true, 2, 2);
      }
    }

    // Determine if the rotary encoder is being turned and which way
    if( clock_control.get_change())
    {
      // If there has been a change, determine which direction to change
      // the currently selected value (hrs or mins)
      encoder_count = clock_control.get_count();
      
      // reset the count, so the next adjustment is 0 based
      clock_control.reset();

      if( hrs_or_mins == false )
      {
        tmp_hours = tmp_hours + encoder_count;
        
        // Check if past limits and set to limit
        if( tmp_hours > 23 )
          tmp_hours = 23;
        else if ( tmp_hours < 0 )
          tmp_hours = 0;
      }
      else
      {
        tmp_minutes = tmp_minutes + encoder_count;

        // Check if past limits and set to limit
        if( tmp_minutes > 59 )
          tmp_minutes = 59;
        else if ( tmp_minutes < 0 )
          tmp_minutes = 0;
      }
    }

    // Check if to see if encoder switch has been pressed
    if(!digitalRead(CLOCK_CONTROL_PSH_BUT_PIN))
    {
      if( hrs_or_mins == false)
      {
        hrs_or_mins = true;

        // We get here because the button was pressed to set the hours.
        // Wait until it is released before going to minutes.
        while(!digitalRead(CLOCK_CONTROL_PSH_BUT_PIN));
      }
      else
      {
        exit = true;
      }
    }
  }
  
  // We get here because the button was pressed to set the minutes.
  // Wait until it is released before returning.
  while(!digitalRead(CLOCK_CONTROL_PSH_BUT_PIN));

  // Set current clock time to temporary time, and clear the seconds
  clk_hours = tmp_hours;
  clk_minutes = tmp_minutes;
  clk_seconds = 0;

  return;
}

/*****************************************************************/
// Interrupt Service Routine (ISR).  Let BasicEncoder library handle the rotator changes
/*****************************************************************/
void updateEncoder() 
{
  clock_control.service();  // Call BasicEncoder library .service()
}