/*
 * Spectrum Analyzing Pop-Art
 * 
 * This code is written to control an 8x8 RGB LED Matrix as an audio spectrum analyzer.
 * Using an FFT library to analyze an audio signal connected to A2 on the arduino
 * Nano. 
 * 
 * Source:
 *    Matrix 10x10: http://www.instructables.com/id/Arduino-Spectrum-Analyzer-on-a-10x10-RGB-LED-Matri/
 *    FFT Library : http://wiki.openmusiclabs.com/wiki/ArduinoFFT
 *    LED FastLED : https://github.com/FastLED/FastLED/wiki/Basic-usage
 */

/*
  Author: Rionaldy Triantoro
*/

#define LOG_OUT 0         //set output of FFT library to linear not logarithmical
#define LIN_OUT 1
#define FFT_N 256         //set to 256 point fft

#include <FFT.h>          //include the FFT library
#include <FastLED.h>      //include the FastLED Library
#include <math.h>         //include library for mathematic funcions

//LED 
#define DATA_PIN 6        //LED Data PIN
#define NUM_LEDS 64      //Number of leds on the matrix
CRGB leds[NUM_LEDS];  
unsigned char colHeight[8] = {0,0,0,0,0,0,0,0}; //set initial column height
float hue = 0;        //hue value of the colors

//FFT Frequency Range
unsigned char freq[9] = {0,5,7,9,11,16,24,32,69};

//Displaying the LED Matrix
void setMatrix(unsigned char column, unsigned char height)
{
  //Map the recieved data from FFT
  unsigned char h = (unsigned char)map(height, 0, 255, 0, 8);

  //Setting height per column
  if (h < colHeight[column])
    colHeight[column]--;
  else if (h > colHeight[column])
    colHeight[column] = h;

  //Change the color spectrum if amplitude of signal is high enough
  if (height > 250)
  {
    hue += 2;
    if(hue > 25) hue = 0; //Resets the hue back to 0
  }

  //Set colors of pixels according to column and hue
  for(unsigned char y = 0; y < 8; y++)
  {    
       //LED turns on if magnitude is high enough                
       if(colHeight[column] > y)
       {
        delayMicroseconds(25);
        leds[y+(column*8)] = CHSV((hue*8)+(column*8), 255, 200);
       } else {
        delayMicroseconds(650);
        leds[y+(column*8)] = CRGB::Black;
       }
  }
}

//Initialization
void setup() 
{
  FastLED.addLeds<WS2812B, DATA_PIN, GRB> (leds, NUM_LEDS); //Initialize LEDs
  Serial.begin(115200);                       //use the serial port
  TIMSK0 = 0;                                 //turn off timer0 for lower jitter
  ADCSRA = 0xe5;                              //set the adc to free running mode
  ADMUX = 0b00000010;                         //use pin A2 and set aref to external
  DIDR0 = 0x02;                               //turn off the digital input for A2
}

//Main Program Loop
void loop() {
  //Using while(1) for faster processing 
  while(1)
  {
    cli();  //RX/TX Interrupt can slow down processing, so all interrupts deactivated

    //Sampling to save 256 samples
    for (int i = 0 ; i < 512 ; i += 2)
    {
      while(!(ADCSRA & 0x10));       //wait for adc to be ready
      ADCSRA = 0xf5;                 //restart adc
      byte m = ADCL;                 //fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m;          //form into an int
      k -= 0x0200;                   //form into a signed int
      k <<= 6;                       //form into a 16b signed int
      fft_input[i] = k;              //put real data into even bins
    }

    fft_window();                    // window the data for better frequency response
    fft_reorder();                   // reorder the data before doing the fft
    fft_run();                       // process the data in the fft
    fft_mag_lin();                   // take the output of the fft
    
    sei();                           //Reactivate interrupt

    //Set Bin 0 and 1 to 0 for better spectrum result
    fft_lin_out[0] = 0;
    fft_lin_out[1] = 0;

    //Determine frequency ranges of collected sample and display to Matrix
    for(unsigned char i = 0; i < 9; i++)
    {
      unsigned char maxHeight = 0;  //Max height

      //Determine Frequency Ranges
      for(unsigned char x = freq[i]; x < freq[i+1]; x++)
      {
        if ((unsigned char)fft_lin_out[x] > maxHeight)
          maxHeight = (unsigned char)fft_lin_out[x];
      }

      //Send the processed value to the matrix adjuster
      setMatrix(i, maxHeight);

      //For displaying on the serial plotter
      Serial.print(maxHeight);
      Serial.print(" ");
    }

    Serial.println("");
    TIMSK0 = 1;       //Timer must be reactivated briefly for LED Control
    FastLED.show();   //Display the LEDs
    TIMSK0 = 0;       
  }
}
