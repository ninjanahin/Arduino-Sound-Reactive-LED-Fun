//#include <AudioFrequencyMeter.h>
#include <fix_fft.h>
#include <FastLED.h>

#define NUM_LEDS 64
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define DATA_PIN 7
CRGB leds[NUM_LEDS]; //Initialize LED Array

uint8_t programMode = 1;
int buttonCurrent = LOW;
int buttonPrevious = LOW;


//Microphone Setup
#define MIC_PIN A0 //Analog input area of the Arduino

//Button Setup
#define BUTTON_PIN 13

//FFT Transform variables
char im[128];         //These are the set of imaginary numbers we will be working with
char data[128];       //These are the set of real numbers we will be working with
char data_avgs[8];    //These are the averages of the FFT outputs we will take
int audio_val;        //This is the audio value read in from the microphone
#define SAMPLES 128   //The number of samples we will use to create the FFT

//For animation simplicity
typedef struct{ uint8_t x; uint8_t y; } pos;   //To contain co-ordinates for pixel location --> use with XY() function
uint16_t state = 0;
uint16_t global_colour = 0;

void setup() {
  // put your setup code here, to run once:
    
  pinMode(MIC_PIN, INPUT);                                              //Microphone Setup
  pinMode(BUTTON_PIN, INPUT);                                           //Button Setup

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);     //FastLED Setup (Using WS812 5050 RGB LED'S)
  Serial.begin(9600);                                                   //Serial Output Setup (For debugging)

  anim_start();                                                         //Introductory Animation
}

void loop() {
  // put your main code here, to run repeatedly:
  
  buttonCurrent = digitalRead(BUTTON_PIN);  //Detect button press                                  
  
  if(buttonCurrent == HIGH && buttonPrevious == LOW)                       
  {
    programMode++;                          //Change the mode
    state = 0;                              //Reset Animation state for next function
    global_colour = 0;                      //Reset Global Colour variable for animations
    LEDS.clear();                           //Clear the LEDS from the previous mode
    
    if(programMode > 7)                     //If the program mode clicks past 7, reset back to the first 
    {
      programMode= 1;
    }
  }  
  buttonPrevious = buttonCurrent; //This is to prevent re-entering the if statement again
  
  switch(programMode)
  {
    case 1 :
      anim_randPixel();
      break;
    case 2 :
      anim_wave();
      break;
    case 3 :
      anim_equalizer();
      break;
    case 4 :
      anim_rain();
      break;
    case 5 :
      anim_growingBox();
      break;
    case 6 :
      anim_rotatingBox();
      break;
    case 7 :
      anim_fireworks();
      break;
  }

}

/* ---------------------------------------------------------- */
/* ------------------- HELPER FUNCTIONS --------------------- */
/* ---------------------------------------------------------- */

uint16_t setRandColor()
{
  //We select a number from 0-255 as a HUE value for the HSV value [Hue, Saturation, value]
  //In order to do this we return a rand value between the range (0-255)
  //As C doesn't have a ranged rand value, we must take the approach below
  
  return (rand() % (255-0 +1) + 0);
}

uint16_t XY( int x, int y)
{
  //A Helper Function for calculating Pixel number based on XY co-ordinates for
  //the 8x8 LED Matrix display
  
  return x*8 + y;
}

bool in_list(pos xy, pos list[], int numElements)
{
  //A function to determine whether or not a given POS struct object {int x, int y}
  //is in a given list of POS struct objects
  //Returns True if a match is found, and false otherwise
  
  bool found = false;
  for(uint16_t i=0; i<numElements; i++)
  {
    if((xy.x == list[i].x) && (xy.y == list[i].y))    //If x and y are both found to match a given list object --> return true
    {
      found = true;
    }
  }
  return found;
}

void leds_from_struct(pos posArray[], int numElements, int hue, int sat, int bright)
{
  //A function for lighting up LEDS using an array of POS struct coordinates {int x, int y}
  //The Hue, Saturation and Brightness values are used to set the LED to the desired state
  //If the Hue is -1 , it will turn off the given LED (Set it to black)
  
  for(int i=0; i< numElements; i++)
  {
    if(XY(posArray[i].x, posArray[i].y) < 64)
    {
      if(hue != -1)
      {
        leds[XY(posArray[i].x, posArray[i].y)] = CHSV(hue, sat, bright);
      }
      else
      {
        leds[XY(posArray[i].x, posArray[i].y)] = CRGB::Black;
      }
    }
  }
}

/* ----------------------------------------------------------- */
/* ------------------ ANIMATION FUNCTIONS -------------------- */
/* ----------------------------------------------------------- */

void anim_start()
{
  pos three[13] = {{1,2}, {1,3}, {1,4}, {1,5}, {2,5}, {3,3}, {3,4}, {3,5}, {4,5}, {5,2}, {5,3}, {5,4}, {5,5}};
  pos two[14] = {{1,2}, {1,3}, {1,4}, {1,5}, {2,5}, {3,5}, {3,4}, {3,3}, {3,2}, {4,2}, {5,2}, {5,3}, {5,4}, {5,5}};
  pos one[8] = {{1,3}, {1,4}, {2,4}, {3,4}, {4,4}, {5,4}, {5,3}, {5,5}};
  pos go[19] = {{2,1}, {2,2}, {2,3}, {3,1}, {4,1}, {5,1}, {5,2}, {5,3}, {4,3}, {2,5}, {3,5}, {4,5}, {5,5}, {5,6}, {5,7}, {4,7}, {3,7}, {2,7}, {2,6}};
  pos border[16] = {{0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}, {7,0}, {7,1}, {7,2}, {7,3}, {7,4}, {7,5}, {7,6}, {7,7}};

  leds_from_struct(border, 16, 64, 255, 100);       // Set a yellow border that persists across all stages of the animation (TOP/BOTTOM ROW)
    
  leds_from_struct(three, 13, 0, 255, 255);
  LEDS.show();
  delay(1000);
  leds_from_struct(three, 13, -1, 0, 0);
  leds_from_struct(two, 14, 0, 255, 255);
  LEDS.show();
  delay(1000);
  leds_from_struct(two, 14, -1, 0, 0);
  leds_from_struct(one, 8, 0, 255, 255);
  LEDS.show();
  delay(1000);
  leds_from_struct(one, 8, -1, 0, 0);
  leds_from_struct(go, 19, 160, 255, 255);
  LEDS.show();
  delay(1000);
  LEDS.clear();                   
}


void anim_randPixel()
{
      int multiplier;
      audio_val = analogRead(MIC_PIN);
      
      switch(audio_val)
      {
        case 0 ... 320:
          multiplier= 0;
          break;
        case 321 ... 370:
          multiplier = 1;
          break;
        case 371 ... 440:
          multiplier = 2;
          break;
        case 441 ... 600:
          multiplier = 3;
          break;
        case 601 ... 1023:
          multiplier = 4;
          break;
      }
      if(multiplier > 0)
      {
        uint16_t numPixels = (rand() % ((7-0+1)));

        for(int i=0; i<numPixels; i++)
        {
          uint16_t randX = (rand() % ((7-0+1)));
          uint16_t randY = (rand() % ((7-0+1)));
          leds[XY(randX, randY)].setHSV(setRandColor(), 255, 127);
        }
        LEDS.show();
        delay(20);
        LEDS.clear();
      }
}

void anim_wave()
{
   //Wave animation
   pos wave[8] = {{2,0}, {3,1}, {4,2}, {5,3}, {5,4}, {4,5}, {3,6}, {2,7}};        //Basically a sine wave shape
   pos border_top[8] = {{0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}};   //Top Border
   pos border_bot[8] = {{7,0}, {7,1}, {7,2}, {7,3}, {7,4}, {7,5}, {7,6}, {7,7}};   //Bottom Border
   int multiplier = 1;
   
   audio_val = analogRead(MIC_PIN);
   switch(audio_val)
   {
      case 320 ... 340:
        multiplier = 2;
        break;
      case 341 ... 360:
        multiplier = 3;
        break;
      case 361 ... 380:
        multiplier = 4;
        break;
      case 381 ... 1023:
        multiplier = 5;
        break;
   }

   leds_from_struct(border_top, 8, 234, 4, 36);                                   //Set the top border to grey
   leds_from_struct(border_bot, 8, 234, 4, 36);                                   //Set the bottom border to grey
   leds[XY(border_top[state*2].x, border_top[state*2].y)] = CHSV(0,0, 255);       //Create a white dash that runs along the top grey border (Pixel 1)
   leds[XY(border_top[state*2+1].x, border_top[state*2+1].y)] = CHSV(0,0, 255);   //Create a white dash that runs along the top grey border (Pixel 2)
   
   leds[XY(border_bot[state*2].x, border_bot[state*2].y)] = CHSV(0,0, 255);       //Create a white dash that runs along the bottom grey border (Pixel 1)
   leds[XY(border_bot[state*2+1].x, border_bot[state*2+1].y)] = CHSV(0,0, 255);   //Create a white dash that runs along the bottom border (Pixel 2)
   
   //Every state change --> Change colour --> transition colour
   shift_right(wave, 8, state);                                               //Shift right by X places based on the current animation state
   leds_from_struct(wave, 8, global_colour, 255, 127);                        //Once the leds have been shifted -> Set them through the helper function based on global colour
   leds[XY(wave[state].x, wave[state].y)] = CHSV(0, 0, 255);                  //Set one led to white
   LEDS.show();
   delay((40/ multiplier));                                                     //Set the speed based on the multiplier --> Louder music, faster moving
   leds_from_struct(wave, 8, -1, 0, 0);                                       //Reset back to black / Turn off LEDS
   
   state++;                                                                   //Increment state to control the number of shifts right
   if(state > 7)                                                              //If this number is greater than 7, then it's looped, so set to 0
   {
      state = 0;
   }
   
   global_colour +=5;                                                         //Increment the global colour to create a rainbow effect
   if(global_colour > 255)                                                    //If this number is greater than 255, then reset back to 0
   {
      global_colour = 0;
   }
}

void shift_right(pos posArray[], int numElements, int numShifts)
{
  for(int i=0; i< (numShifts % 8); i++)         //There is a maximum of 7 shifts across the board that can be made without repeating
  {
    for(int j=0; j< numElements; j++)
    {
      posArray[j].y +=1;
      if(posArray[j].y > 7)
      {
        posArray[j].y = 0;
      }
    }
  }
}

void anim_rain()
{
  bool add_new = false;
  if(analogRead(MIC_PIN) > 300)
  {
    add_new = true;                                                       //Set the add_new_led flag to true so it knows to add a new rain droplet
  }

  for(int x =7; x > -1 ; x--)
  {
    for(int y=7 ; y > -1; y--)
    {
      //SHIFT EXISTING NODES
      if(x == 7)                                                          //If dealing with the last row, just clear the leds
      {
        leds[XY(x,y)] = CRGB::Black;                                      //Turn off the led
      }
      else if(leds[XY(x,y)])                                              //If the LED is on and the next row down exists then shift the LED down one space
      {
        leds[XY((x+1), y)] = leds[XY(x, y)];                              //Set the next LED to the current LED
        leds[XY((x+1), y)] /= 2;                                          //Set the next LED to half the brightness of the current one
        if(leds[XY(x-1,y)])                                                        //If the previous row LED exists then blank it out.
        {
          leds[XY(x-1, y)] = CRGB::Black;                                 //Blank out the previous row LED --> Cleanup                                            
        }
        else if((x==1) && (leds[XY((x+1), y)]) && (leds[XY(x-1, y)]))     //If on the second row and the rows above and below are lit, remove the first row led
        {
          leds[XY(x-1, y)] = CRGB::Black;
        }
        else if (leds[XY((x-1), y)] == CRGB(0,0,0))
        {
          leds[XY(x, y)] = CRGB::Black;
        }
      }

      //ADD NEW NODES
      if(add_new == true)
      {
        int starting_point = (rand() % (7-0+1));                              //Pick a random starting point between 0-7 if sound is detected
        if(! leds[XY(0, starting_point)] && ! leds[XY(1, starting_point)])    //If there is no LED lit on that particular starting point or the next led down, set the new LED
        {
          leds[XY(0, starting_point)] = CHSV(160, 255, 255);                  //Set the starting point LED on the first row
        }
        add_new = false;                                                      //Reset add new flag
      }
    }
  }

  LEDS.show();
  delay(20);
}


void anim_equalizer()
{
  for (int i=0; i< SAMPLES; i++)                        //Take 128 samples --> The more samples, the more accurate, but the more slow it becomes
  {                                                     //
    audio_val = analogRead(MIC_PIN);                    //Get audio from the Microphone
    data[i] = audio_val;                                //Each element of the data sample is sound data from the microphone
    im[i] = 0;                                          //Set each imaginary number in the imaginary data set to 0
  }
  
  fix_fft(data, im, 7, 0);                              //Perform the FFT on data using the fix_FFT library
  
  for(int i=0; i<(SAMPLES/2); i++)
  {
    data[i] = sqrt(data[i] * data[i] + im[i] * im[i]);  //Make the values positive -- Filter out noise and hum
  }                                                     

  for(int i = 0; i< 8 ; i++)                            //Average the numbers so they fit within 8 bins (64 data points, 8 bins -> 64/8=8)
  {                                                     
    data_avgs[i] = data[(i*4)] + data[(i*4 + 1)] + data[(i*4 + 2)] + data[(i*4 +3)];
    if(i == 0)
    {
      data_avgs[i] /= 2;                                                     
    }
    data_avgs[i] = map(data_avgs[i], 0, 30, 0, 7);      //Maps the averages to a value from 0-7 that we can display on the LED Matrix - Max expected ouput val of 20
    data_avgs[i] = constrain(data_avgs[i], 0, 7);       //Force the avgs to fall within 0-7 so they may be displayed accurately
  }

  int randColor = setRandColor();                       //Add a random color on each run to add EPILEPSY!
  for(int y=0; y<8; y++)                                //Map the Average data to the LED matrix starting from the bottom row (0, 7)
  {
      int limit = 7 - data_avgs[y] ;                    //We calculate how many LEDS we should turn on from the bottom (7 being the max as the range is 0-7[8 leds])
      for(int x=7 ; x > limit; x--)
      {
        leds[XY(x,y)] = CHSV(randColor, 255, 127);      //Set the HSV value for each LED, with the random color we assigned above
      }
  }
  LEDS.show();
  delay(1);
  LEDS.clear();
}

void anim_growingBox()
{
  pos tier1[4] =  {{3, 3},{3,4},{4,3}, {4,4}};                                                                            //These are the positions of each pixel for each animation state
  pos tier2[4] =  {{2,2}, {2,5},{5,2}, {5,5}};                                                                            //Each animation state will contain a particular pattern
  pos tier3[8] =  {{2,3}, {2,4}, {3,2}, {4,2}, {3,5}, {4,5}, {5,3}, {5,4}};                                               //Each pattern will have a different colour
  pos tier4[12] = {{1,1}, {1,2}, {2,1}, {1,5}, {1,6}, {2,6}, {5,1}, {6,1},{6,2}, {5,6}, {6,6}, {6,5}};
  pos tier5[8] =  {{1,3}, {1,4}, {3,1}, {4,1}, {3,6}, {4,6}, {6,3}, {6,4}};
  pos tier6[20] = {{0,0}, {0,1}, {1,0}, {0,3}, {0,4}, {0,6}, {0,7}, {1,7}, 
                   {6,0}, {7,0}, {7,1}, {3,0}, {4,0}, {7,6}, {7,7}, {6,7},
                   {3,7}, {4,7}, {7,3}, {7,4}};

  audio_val = analogRead(MIC_PIN);
  
  (audio_val > 320) ? leds_from_struct(tier1, 4, 0, 255, 127) : leds_from_struct(tier1, 4, -1, 0, 0);                     //Each of these is a Ternary operator (Same as if else)
  (audio_val > 340) ? leds_from_struct(tier2, 4, 160, 255, 127) : leds_from_struct(tier2, 4, -1, 0, 0);                   //If the audio value reaches the next value, light the LEDS in that tier  
  (audio_val > 360) ? leds_from_struct(tier3, 8, 96, 255, 127) : leds_from_struct(tier3, 8, -1, 0, 0);                    //Otherwise, turn the LED off by passing -1 as the hue value setting the LED to black
  (audio_val > 380) ? leds_from_struct(tier4, 12, 64, 255, 127) : leds_from_struct(tier4, 12, -1, 0, 0);                  //We turn off the LEDS in the statement instead of using LEDS.clear()
  (audio_val > 400) ? leds_from_struct(tier5, 8, 192, 255, 127) : leds_from_struct(tier5, 8, -1, 0, 0);                   //The leds_from_struct function takes a POS Array, PosArrayLength, Hue, Sat and Brightness
  (audio_val > 420) ? leds_from_struct(tier6, 20, 128, 255, 127) : leds_from_struct(tier6, 20, -1, 0, 0);

  LEDS.show();
}

void anim_rotatingBox()
{
  //Set Animation variables
  pos center[4] =  {{3, 3},{3,4},{4,3}, {4,4}};
  pos s1[4] =  {{2,2}, {2,5},{5,2}, {5,5}}; 
  pos s2[4] = {{1,3}, {3,6}, {6,4}, {4,1}};
  pos s3[4] = {{1,4}, {4,6}, {6,3}, {3,1}};

  //pos s1_1{{2,2}, {2,5},{5,2}, {5,5}};
  //pos s2_1{{2,2}, {2,5},{5,2}, {5,5}};
  //pos s3_1{{2,2}, {2,5},{5,2}, {5,5}};
  
  audio_val = analogRead(MIC_PIN);

  leds_from_struct(center, 4, 96, 255, 127);        //Light up the center box first with a red colour
  
  if(audio_val > 300)                               //If the Audio value is above 300, then move between animation states
  {                                               
    state++;
    if(state > 3)
    {
      state = 1;                                    //If the final state is reached, reset and loop again
    }
  }
  
  switch(state)
  {
    case 1:
      leds_from_struct(s1, 4, 64, 255, 127);        //The different animation states -> The colour stays the same and the saturation changes
      break;                                        //to give it a blinking effect.
    case 2:
      leds_from_struct(s2, 4, 64, 192, 127);
      break;
    case 3:
      leds_from_struct(s3, 4, 64, 192, 127);
      break;
  }
  
  LEDS.show();
  delay(30);
  LEDS.clear();  
}

void anim_fireworks()
{
  /* ------------------------------------------------- */
  /* ------------Setting the Stage-------------------- */
  pos background[8] ={{0,1}, {0,2}, {1,0}, {1,1}, {2,0}, {2,1}, {3,1}, {3,2}};   //A picture of a moon
  leds_from_struct(background, 8, 209, 18, 64);             //Set the LEDS for the Moon

  for(uint8_t i=0; i<NUM_LEDS; i++)
  {
    if(leds[i] == CRGB(0,0,0))
    {
      leds[i] = CRGB(0,0, 128);                                 //For the other LEDS that are off, turn it into the night sky
      leds[i] %= 1;
    }
  }
  LEDS.show();

  /* ------------------------------------------------ */
  /* -------- DETERMINE STARTING VARIABLES ---------- */
  uint8_t starting_pointX = (rand() % (7-0+1));           //Set the starting point for the firworks
  uint8_t starting_pointY = (rand() % (7-0+1));
  uint8_t limit = (rand() % (4-1 +1)) + 1;               //Set a random limit between 1 and 4 (to determine the fireworks size)
  uint16_t starting_color = setRandColor();                //Set a random starting color
  uint16_t next_color;


  /* ------------------------------------------------ */
  /* -------- Watch the firework shoot up ----------- */
  for(uint8_t x = 7; x > starting_pointX; x--)
  {
    leds[XY(x, starting_pointY)] =  CHSV(0,0,100);
    LEDS.show();
    delay(40);
    if(in_list((pos) {x, starting_pointY}, background, 8) == true)
    {
      leds[XY(x, starting_pointY)] = CHSV(209, 18, 64);   //If the pixel was part of the moon, set the color of the moon
    }
    else
    {
      leds[XY(x, starting_pointY)] = CRGB(0,0, 128);   //If the pixel wasn't part of the moon, set it back to the night sky and dim the colour back
      leds[XY(x, starting_pointY)] %= 1;
    }
  }

  /* ------------------------------------------------- */
  /* -------------- Time for the Bang ---------------- */
  Serial.println("Fireworks function entered");
  leds[XY(starting_pointX, starting_pointY)] = CHSV(starting_color, 255, 127);
  LEDS.show();
  delay(50);

  Serial.println("->First LED displayed");
  Serial.println("-->Entering For Loop");

  
  for(uint8_t i =0; i< limit ; i++)
  {
    pos next_tier_leds[25];    //Create position array to contain expected values (overfill it, just to be safe)
    uint8_t current = 0;       //This is used to keep track of the next empty space in the next_tier_leds array
                               //When new values are added, it increments

    next_color = starting_color+(60*(i+1));   //Set the color for the current tier of leds
    if(next_color > 255)                  //There is a color limit of 255, so make sure it falls between 0-255
    {
      next_color = 0 + (next_color -255);
    }
    
    Serial.print("--->I: ");
    Serial.print(i, DEC);
    Serial.print(" , Current: ");
    Serial.print(current, DEC);
    Serial.print("\n");

    for(int8_t x = 0; x< 8; x++)
    {
      for(int8_t y = 0; y < 8 ; y++)
      {
        if(leds[XY(x,y)].r != 0 && leds[XY(x,y)].g != 0 && leds[XY(x,y)].b != 128)
        {
          Serial.print("---->LED FOUND");
          if(x+1 < 8)
          {
            if(leds[XY(x+1, y)].r == 0 && leds[XY(x+1, y)].g == 0 && leds[XY(x+1, y)].b == 128)  //If the adjacent LED is off, check to see if the LED is in the list already
            {
              if(current !=0)
              {
                if(in_list((pos) {x+1, y}, next_tier_leds, current) == false)
                {
                  Serial.print(" - Didn't match , added to list");
                  next_tier_leds[current] = {x+1, y};
                  current++;
                }
              }
              else
              {
                Serial.print(" - Didn't match , added to list");
                next_tier_leds[current] = {x+1, y};
                current++;
              }
            }
          }
          
          if(x-1 > -1)
          {
            if(leds[XY(x-1, y)].r == 0 && leds[XY(x-1, y)].g == 0 && leds[XY(x-1, y)].b == 128 )
            {
              if(current !=0)
              {
                if(in_list((pos) {x-1, y}, next_tier_leds, current) == false)
                {
                  Serial.print(" - Didn't match , added to list");
                  next_tier_leds[current] = {x-1, y};
                  current++;
                }
              }
              else
              {
                Serial.print(" - Didn't match , added to list");
                next_tier_leds[current] = {x-1, y};
                current++;
              }
            }
          }

          if(y+1 < 8)
          {
            if(leds[XY(x, y+1)].r == 0 && leds[XY(x, y+1)].g == 0 && leds[XY(x, y+1)].b == 128)
            {
              if(current !=0)
              {
                if(in_list((pos) {x, y+1}, next_tier_leds, current) == false)
                {
                  Serial.print(" - Didn't match , added to list");
                  next_tier_leds[current] = {x, y+1};
                  current++;
                }
              }
              else
              {
                Serial.print(" - Didn't match , added to list");
                next_tier_leds[current] = {x, y+1};
                current++;
              }
            } 
          }

          if(y-1 > -1)
          {
            if(leds[XY(x, y-1)].r == 0 && leds[XY(x, y-1)].g == 0 && leds[XY(x, y-1)].b == 128)
            {
              if(current !=0)
              {
                if(in_list((pos) {x, y-1}, next_tier_leds, current) == false)
                {
                  Serial.print(" - Didn't match , added to list");
                  next_tier_leds[current] = {x, y-1};
                  current++;
                }
              }
              else
              {
                Serial.print(" - Didn't match , added to list");
                next_tier_leds[current] = {x, y-1};
                current++;
              }            
            }
          }
          
          Serial.print("\n");
        }
      }
    }

    Serial.print("NEXT_LEDS_AMOUNT: ");
    Serial.print(current, DEC);
    Serial.print("\n");
    leds_from_struct(next_tier_leds, current, next_color, 255, 127);   //Light the leds in the found pixels list
    LEDS.show();                    //Show and delay the leds before determining the next set of leds
    delay(50); 
  }
  
  delay(500);
  LEDS.clear();
}
