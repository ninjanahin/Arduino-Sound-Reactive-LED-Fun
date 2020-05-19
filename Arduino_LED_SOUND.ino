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

//For Pong Game
pos paddle[3] = {{7,0}, {6,0}, {5,0}};
pos ball[1] = {{6,3}};
uint8_t ball_direction = 1;
int previous_audio_val = 0;
bool game_over = false;

void setup() {
  // put your setup code here, to run once:
    
  pinMode(MIC_PIN, INPUT);                                              //Microphone Setup
  pinMode(BUTTON_PIN, INPUT);                                           //Button Setup

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);     //FastLED Setup (Using WS812 5050 RGB LED'S)
  Serial.begin(9600);                                                   //Serial Output Setup (For debugging)

  //anim_start();                                                         //Introductory Animation
  do{
    game_pong();
  }while(1);
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
    
    if(programMode > 8)                     //If the program mode clicks past 7, reset back to the first 
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
    case 8 :
      anim_cycle();
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
    if(posArray[i].x < 8 && posArray[i].y <8)
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

void shift_up(pos posArray[], int numElements, int numShifts)
{
  for(int i=0; i< (numShifts % 8); i++)
  {
    for(int j=0; j< numElements; j++)
    {
      posArray[j].x -=1;
      if(posArray[j].x < 0)
      {
        posArray[j].x = 7;
      }
    }
  }
}

void shift_down(pos posArray[], int numElements, int numShifts)
{
  for(int i=0; i< (numShifts % 8); i++)
  {
    for(int j=0; j< numElements; j++)
    {
      posArray[j].x +=1;
      if(posArray[j].x > 7)
      {
        posArray[j].x = 0;
      }
    }
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

void shift_left(pos posArray[], int numElements, int numShifts)
{
  for(int i=0; i< (numShifts % 8); i++)         //There is a maximum of 7 shifts across the board that can be made without repeating
  {
    for(int j=0; j< numElements; j++)
    {
      posArray[j].y -=1;
      if(posArray[j].y < 0)
      {
        posArray[j].y = 7;
      }
    }
  }
}

void shift_diagonal(pos posArray[], int numElements, int numShifts, uint8_t dir)
{
  //This function is for altering variables in a given position array to shift them diagonally by numShift spaces
  //dir is the direction (1 == ascending positive , 2 = descending positive, 3 = ascending negative, 4 = descending negative)
  //This function is useful for animating models determined in a posArray when used in conjunction with the global state variable
  //There is no check to see if the values generated are valid, If they are invalid, they won't be displayed when leds_from_struct() is called

  int8_t incrementX, incrementY;  //The increment amount for X and Y which helps to shape direction
  
  switch(dir)
  {
    case 1:
      incrementX = -1;    //You start from bottom left and work to top right
      incrementY = 1;     //This means decreasing X and increasing Y
      break;
    case 2:
      incrementX = 1;     //You start from top left and work to bottom right
      incrementY = 1;     //This means increasing X and Y
      break;
    case 3:
      incrementX = -1;    //You start from bottom right and work to top left
      incrementY = -1;    //This means decreasing X and Y
      break;
    case 4:
      incrementX = 1;     //You start from top right and work to bottom left
      incrementY = -1;    //This means increasing X and decreasing Y
      break;

    default:
      incrementX = 0;    //In case dir isn't set
      incrementY = 0;    
      break;
  }

  for(uint8_t i=0; i< numElements; i++)
  {
    posArray[i].x += (incrementX * numShifts);
    posArray[i].y += (incrementY * numShifts);
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
  uint8_t limit = (rand() % (3-1 +1)) + 1;               //Set a random limit between 1 and 4 (to determine the fireworks size)
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
  //Set a 2d array for storing + keeping track of values
  
  bool previous_array[8][8];                                //Initialize array to store previous values FOR CALCULATION
  memset(previous_array, 0, 64*sizeof(bool));               //Set all values to 0
  previous_array[starting_pointX][starting_pointY] = true;  //Set first value to our first pixel location


  //TEST AREA
  for(uint8_t i=0; i<limit ; i++)
  {
    bool next_array[8][8];                      //Initialize array TO STORE calculated values
    memset(next_array, 0, 64*sizeof(bool));     //Ensure array is blank
    next_color = starting_color + 60*(i+1);
    if( next_color > 255)
    {
      next_color = 0 + (next_color - 255);
    }
    
    //Determine next pixels using PREVIOUS AND NEXT ARRAYS
    for(uint8_t x=0; x<8; x++)
    {
      for(uint8_t y=0; y<8; y++)
      {
        if(previous_array[x][y] == true)
        {
          if(previous_array[x+1][y] == false &&  (x+1 < 8))
          {
            next_array[x+1][y] = true;
            leds[XY(x+1, y)] = CHSV(next_color, 255, 127);
          }
          if(previous_array[x-1][y] == false && (x-1 > -1))
          {
            next_array[x-1][y] = true;
            leds[XY(x-1, y)] = CHSV(next_color, 255, 127);
          }
          if(previous_array[x][y+1] == false && (y+1 < 8))
          {
            next_array[x][y+1] = true;
            leds[XY(x, y+1)] = CHSV(next_color, 255, 127);
          }
          if(previous_array[x][y-1] == false && (y-1 > -1))
          {
            next_array[x][y-1] = true;
            leds[XY(x, y-1)] = CHSV(next_color, 255, 127);
          }
        }
      }
    }
    //Merge next_array into previous array
    for(uint8_t x=0; x<8; x++)
    {
      for(uint8_t y=0; y<8; y++)
      {
        if(next_array[x][y] == true)
        {
          previous_array[x][y] = true;
        }
      }
    }

    LEDS.show();
    delay(50);
  }
  
  delay(500);
  LEDS.clear();
}

void anim_daytime()
{
  // --- Models --- //
  pos sun[4] = {{1,1}, {1,2}, {2,1}, {2,2}};                                //The sun body
  pos sun_1[8] = {{3,1}, {0,2}, {2,0}, {1,3}, {1,0}, {2,3}, {3,2}, {0,1}};  //The points for the sun ray animation
  pos tree_1[6] = {{2,5}, {2,6}, {3,4}, {3,5}, {3,6}, {3,7}};               //The leaves of the tree
  pos tree_2[8] = {{4,5}, {4,6}, {5,5}, {5,6}, {6,4}, {6,5}, {6,6}, {6,7}}; //The base of the tree

  // --- Setting Models --- //
  leds_from_struct(sun, 4,  64, 200, 100);                                  //Set the Colour of the sun
  leds_from_struct(sun_1, 8, 64, 200, 90);                                  //Set the colour of the sun rays (non-animated part)
  leds_from_struct(tree_1, 6, 96, 255, 200);                                //Set the colour of the leaves
  leds_from_struct(tree_2, 8, 30, 255, 80);                                //Set the colour of the Tree base
  
  // --- Setting background --- //
  for(uint8_t x=0; x<8; x++)
  {
    for(uint8_t y=0; y<8; y++)
    {
      if(x == 7)
      {
        leds[XY(x,y)] = CRGB::LawnGreen;                                       //Set the grass
        leds[XY(x,y)] %= 10;                                                   //Fade the colour immensely
      }
      else if(leds[XY(x,y)].r == 0 && leds[XY(x, y)].g == 0 && leds[XY(x, y)].b == 0)    //Check to see if the pixel is on (black)
      {
        leds[XY(x,y)] = CRGB::DeepSkyBlue;                                         //Set the colour of the sky
        leds[XY(x,y)] %= 10;                                                    //Fade the colour immensely
      }
    }
  }

  //  -- Animate the Sun -- //
  if(state > 3)
  {
    state = 0;                                                                //Check that the state isn't greater than 3 (from other modes, or incrementing)
  }
  leds[XY(sun_1[state*2].x, sun_1[state*2].y)] = CRGB::Gold;                //Set the colour of two of the points within the sun's animation points based on the current state
  leds[XY(sun_1[state*2 +1].x, sun_1[state*2 + 1].y)] = CRGB::Gold;
  state++;                                                                    //Increment the state so the colour cycles around and animates
  
  LEDS.show();
  delay(50);
  LEDS.clear();
}

void anim_cycle()
{
  //  --- Models --- //
  pos sun[4] = {{7,7}, {7,8}, {8,7}, {8,8}};                                    //Start the animation in the bottom left corner only showing one pixel
  pos moon[8] = {{7,7}, {7,8}, {8,7}, {8,8}, {9,8}, {9,9}, {6,8}, {6,9}};     //Rely on the leds_from_struct logic to not turn on any LEDS outside 8x8 grid parameters

  CRGB colours[2] = {CRGB::Gold, CRGB::Silver};

  Serial.println("Cycle entered");
  
  // --- Logic --- //
  
  if(state > 19)      //There are 20 states total -> 9 for Sun/Moon each and 1 each as a gap
  {
    state = 0;
    
  }

  if(state == 0 || state == 10)
  {
    //Show background only
  }
  else if(state > 0 && state < 6)
  {
    shift_diagonal(sun, 4, state-1, 3);           //Shift the sun upwards
    leds_from_struct(sun, 4, 64, 255, 127);     //Colour the  sun
    for(uint8_t i=0; i<64; i++)
    {
      if(! leds[i])
      {
        leds[i] = CHSV(160, 255, 20+ state*30);
      }
    }
  }
  else if(state > 5 && state < 10)
  {
    shift_diagonal(sun, 4, 4, 3);               //Shift the sun upwards first
    shift_diagonal(sun, 4, (state-1)-4, 4);     //Then shift the sun downwards
    leds_from_struct(sun, 4, 64, 255, 127);     //Colour the sun
    for(uint8_t i=0; i<64; i++)
    {
      if(! leds[i])
      {
        leds[i] = CHSV(160, 255, 170 - (state-4)*30);
      }
    }
  }
  else if(state > 10 && state < 16)
  {
    shift_diagonal(moon, 8, (state-11), 3);           //Shift the moon upwards
    leds_from_struct(moon, 8, 0, 0, 127);       //Colour the moon
    for(uint8_t i=0; i<64; i++)
    {
      if(! leds[i])
      {
        leds[i] = CHSV(160, 255, 20);
      }
    }
  }
  else if(state > 15 && state < 20)
  {
    shift_diagonal(moon, 8, 4, 3);               //Shift the moon upwards first
    shift_diagonal(moon, 8, (state-11)-4, 4);     //Then shift the moon downwards
    leds_from_struct(moon, 8, 0, 0, 127);       //Colour the moon
    for(uint8_t i=0; i<64; i++)
    {
      if(! leds[i])
      {
        leds[i] = CHSV(160, 255, 20);
      }
    }
  }
  
  LEDS.show();
  delay(100);
  LEDS.clear();
  state++;  
}

void anim_dancing_man()
{
  // -- Models -- //
  //pos disco_ball = {{0,3}, {0,4}, {1,3}, {1,4}};
  //pos disco_ray = {{2,5}, {3,6}, {4,7}, {2,2}, {3,1}, {4,0}};
  //pos disco_ray2 = {{2,4}, {3,5}, {4,6}, {5,7}, {2,3}, {3,2}, {2,1}, {1,0}};
 // pos man_1 = {{7,2}, {7,4}, {6,3}, {5,3}, {5,2}, {5,4}, {4,3}};
  
}

void game_pong()
{ 
  //Serial.println(audio_val, DEC);
  pos wall[8] = {{0,7}, {1,7}, {2,7}, {3,7}, {4,7}, {5,7}, {6,7}, {7,7}};
  pos game_over_face[20] = {{0,1}, {0,3}, {0,4}, {0,6}, {1,2}, {1,5}, {2,1}, {2,3}, {2,4}, {2,6}, 
                            {6,1}, {5,1}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {5,6}, {6,6}};
  //bool game_over = false;

  if(game_over == true)
  {
    anim_start();
    ball[0].x = (rand() % (6-1 +1) + 1);
    ball[0].y = (rand() % (5-2 +1) + 2);
    ball_direction = (rand() % (4-1 +1) + 1);
    game_over = false;
  }
  while(digitalRead(BUTTON_PIN) != HIGH && game_over == false)
  {
    EVERY_N_MILLIS(5)
    {
      // -- Grab the audio value & Shift the board according to the value --//
      audio_val = analogRead(MIC_PIN);
      
      if(audio_val > 310 && audio_val < 330)
      {
        //Check to see if it's not resting on the BOTTOM of X
        if(in_list((pos){7,0}, paddle, 3) == false)
        {
          //Serial.println("Shift_down");
          shift_down(paddle, 3, 1);
        }
      }
      else if(audio_val > 350)
      {
        //Check to see if it's not resting on the TOP of X
        if(in_list((pos){0,0}, paddle, 3) == false)
        {
          //Serial.println("Shift_up");
          shift_up(paddle, 3, 1);
        }
      }

      // -- Light the corresponding models based on their calculated positions -- //
      leds_from_struct(paddle, 3, 100, 255, 255);
      leds_from_struct(wall, 8, 100, 255, 255);
      leds_from_struct(ball, 1, 100, 255, 255);
  
      LEDS.show();
      LEDS.clear();
    }
    EVERY_N_MILLIS(200)
    {
      // -- IF THE BALL HITS A WALL --> CHANGE THE BALL DIRECTION -- //
      
      if(ball[0].y == 1 && (in_list((pos) {ball[0].x, 0}, paddle, 3) == false))
      {
        game_over = true;
      }
      else if(ball[0].y == 6 && ball[0].x ==0 || ball[0].y == 6 & ball[0].x == 7 || ball[0].y == 1 && ball[0].x == 0 || ball[0].y == 1 && ball[0].x == 7)
      {
        switch(ball_direction)
        {
          case 1:
            ball_direction = 4;
            break;
          case 2:
            ball_direction = 3;
            break;
          case 3:
            ball_direction = 2;
            break;
          case 4:
            ball_direction = 1;
            break;
        }
      }
      else if(ball[0].y == 6 || ball[0].y==1)
      {
        switch(ball_direction)
        {
          case 1:
            ball_direction = 3;
            break;

          case 2:
            ball_direction = 4;
            break;

          case 3:
            ball_direction = 1;
            break;

          case 4:
            ball_direction = 2;
            break;
        }
      }
      else if(ball[0].x == 0 || ball[0].x == 7)
      {
        switch(ball_direction)
        {
          case 1: 
            ball_direction = 2;
            break;
          case 2:
            ball_direction = 1;
            break;
          case 3:
            ball_direction = 4;
            break;
          case 4:
            ball_direction = 3;
            break;
        }
      }
      
      // -- SHIFT THE BALL BASED ON BALL DIRECTION -- //
      shift_diagonal(ball, 1, 1, ball_direction);
    }
  }

  if(game_over == true)
  {
    // -- Display game over -- //
    leds_from_struct(game_over_face, 20, 0, 255, 127);
    LEDS.show();
    delay(2000);
    LEDS.clear();
    //break;
  }
  
}
