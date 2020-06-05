#include <fix_fft.h>
#include <FastLED.h>

//-- LED MATRIX SETUP --//
#define NUM_LEDS 64         //The number of LEDS on the Matrix/LED Strip
#define LED_TYPE WS2812     //The type of LEDS
#define COLOR_ORDER GRB     //The Color order --> Took this from a tutorial
#define DATA_PIN 7          //The port in which the LED Matrix is plugged into (Digital Input)
CRGB leds[NUM_LEDS];        //Initialization of the LED Array


//-- Menu Setup --//
uint8_t programMode = 1;    //This tracks which animation is currently running
int buttonCurrent = LOW;    //These variables are to establish accurate button state detection
int buttonPrevious = LOW;   //These variables are to establish accurate button state detection


//-- Microphone Setup --//
#define MIC_PIN A0          //Defining which port the Microphone is plugged into


//-- Button Setup --//
#define BUTTON_PIN 13       //Defining which data port the button is plugged into (Digital Input)


//-- FFT Transform variables --//
char im[128];               //These are the set of imaginary numbers we will be working with
char data[128];             //These are the set of real numbers we will be working with
char data_avgs[8];          //These are the averages of the FFT outputs we will take
int audio_val;              //This is the audio value read in from the microphone
#define SAMPLES 128         //The number of samples we will use to create the FFT


//-- Global Animation variables --//
typedef struct{ uint8_t x; uint8_t y; } pos;   //To contain co-ordinates for pixel location --> use with XY() function
uint16_t state = 0;                            //Animation state --> If it increments , the position variables used are likely to change too to create an animation
uint16_t global_colour = 0;                    //In some cases, it may be useful to have a global colour instead of setting it manually everytime

//-- Wave Animation 2 variables --//
pos wave2[8] = {{4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {4,7}};   //The wave Animation model
uint8_t wave_dir = 1;                         //The direction of the foremost pixel currently being calculated.
int wave_height = 0;                          //The max height calculated from the upwards direction part of the Sine Wave 
int amplitude = 0;                            //The current amplitude (from X=5) of the foremost pixel [Y=7] --> The last pixel in the WAVE model ^
uint8_t wave_audio_level = 0;                 //A value of 0-3 determining whether or not the foremost pixel should move further up based on volume.  

//-- Genesis Animation variables --//
bool construct = true;
uint8_t num_constructed = 0;
uint8_t num_attempts = 0;

//-- Global Pong Game variables --//
pos paddle[3] = {{7,0}, {6,0}, {5,0}};        //The position struct containing the starting X and Y points for the paddle
pos ball[1] = {{6,3}};                        //The position struct containing the starting X and Y points for the ball 
uint8_t ball_direction = 1;                   //A value from 1-4 that reflects the ball direction --> More info, see shift_diagonal() in Helper Function area
int previous_audio_val = 0;                   //Previously determined audio value for normalizing read in audio values
bool game_over = false;                       //A boolean for determining whether or not the game is over. 


void setup() {
  // put your setup code here, to run once:
    
  pinMode(MIC_PIN, INPUT);                                              //Microphone Setup
  pinMode(BUTTON_PIN, INPUT);                                           //Button Setup

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);     //FastLED Setup (Using WS812 5050 RGB LED'S)
  Serial.begin(9600);                                                   //Serial Output Setup (For debugging)

  //anim_start();                                                         //Introductory Animation --> See anim_start() in Animation function area
  /*do{
    anim_genesis();
  }while(1); */
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
    
    if(programMode > 11)                     //If the program mode clicks past 7, reset back to the first 
    {
      programMode= 1;
    }
  }  
  buttonPrevious = buttonCurrent;           //This is to prevent re-entering the IF statement on long press of the button

  // -- ANIMATION MENU -- //
  switch(programMode)                       //Switch statement containing which animation functions are called based on the programMode
  {
    case 1 :
      anim_randPixel();                     //1. Randomly generated coloured Pixels based on sound detection
      break;
    case 2 :
      anim_wave1();                         //2. A Sine wave that changes colour and speeds up based on sound detected
      break;
    case 3 :
      anim_wave2();                         //3. An variable wave based on information from a the presetFFT function (Just one band)
      break;
    case 4 :
      anim_spike();                         //4. A horizontal voice spike that moves from the center to the outer edge , based on presetFFT(Just one band)
      break;
    case 5 :
      anim_equalizer();                     //5. A colourful Equalizer that incorporates the FastFFT Library functions
      break;
    case 6 :
      anim_rain();                          //6. Randomly generated rain that falls based on detected sound
      break;
    case 7 :
      anim_growingBox();                    //7. A colourful pattern that has different states based on sound level
      break;
    case 8 :
      anim_rotatingBox();                   //8. An animation test showing a dot moving around a box
      break;
    case 9 :
      anim_genesis();                       //9. An animation where LEDS are randomly lit with a random brightness then slowly faded out to black then repeated
      break;
    case 10 :
      anim_fireworks();                     //10. Randomly generated fireworks on a night sky backdrop (NOT SOUND BASED)
      break;
    case 11 :
      anim_cycle();                         //11. An animation of a Sun/Moon cycle (Sun up/down, then Moon up/down)
      break;
    case 12 :
      game_pong();                          //12. A simple game of pong where the paddle moves based 
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

uint16_t incrementGlobalColour(uint8_t increment=5)
{
  //Increments the global colour by a given increment with the default being 5 (If no import is given)
  //Global Colour has is meant to be used in conjunction with the FastLED CHSV value, so it's range is 0-255
  //If it goes over this value, loop it back around.
  global_colour +=5;
  if(global_colour > 255)
  {
    global_colour = 0 + (global_colour - 255);  
  }
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

void leds_from_struct(pos posArray[], int numElements, int hue, int sat, int bright, int rgb= false)
{
  //A function for lighting up LEDS using an array of POS struct coordinates {int x, int y}
  //The Hue, Saturation and Brightness values are used to set the LED to the desired state
  //If the Hue is -1 , it will turn off the given LED (Set it to black)
  //If RGB is set to true (optional variable with false by default), It will use the
  //Hue, Sat and Bright values as RGB values respectively.
  
  for(int i=0; i< numElements; i++)
  {
    if(posArray[i].x < 8 && posArray[i].y <8)
    {
      if(hue != -1)
      {
        if(rgb == true)
        {
          leds[XY(posArray[i].x, posArray[i].y)] = CRGB(hue, sat, bright);
        }
        else
        {
          leds[XY(posArray[i].x, posArray[i].y)] = CHSV(hue, sat, bright);
        }
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
  //Given a POS array most likely containing an animation model,
  //this function will shift those pixels up (X--) on the LED Matrix board
  
  for(int i=0; i< (numShifts % 8); i++)     //There are a maximum of 7 shifts that can be made as that would loop back to the original position
  {
    for(int j=0; j< numElements; j++) 
    {
      posArray[j].x -=1;                    //Reduce X by 1, this results in a shift up on the LED matrix board.
      if(posArray[j].x < 0)                 //If X is less than 0 , it is no longer displayed on the board or valid, 
      {                                     //In this case, shift it to the bottom most position (x=7)
        posArray[j].x = 7;
      }
    }
  }
}

void shift_down(pos posArray[], int numElements, int numShifts)
{
  //Given a POS array most likely containing an animation model,
  //this function will shift those pixels down (X++) on the LED Matrix board
  
  for(int i=0; i< (numShifts % 8); i++)     //There are a maximum of 7 shifts that can be made as that would loop back to the original position
  {
    for(int j=0; j< numElements; j++)
    {
      posArray[j].x +=1;                    //Increase X by 1, this results in a shift down on the LED matrix board.
      if(posArray[j].x > 7)                 //If X is greater than 7, it is no longer displayed on the board or valid
      {                                     //In this case, shift it to the top most position (x=0)
        posArray[j].x = 0;
      }
    }
  }
}

void shift_right(pos posArray[], int numElements, int numShifts)
{
  //Given a POS array most likely containing an animation model,
  //this function will shift those pixels right (Y++) on the LED Matrix board
  
  for(int i=0; i< (numShifts % 8); i++)     //There are a maximum of 7 shifts that can be made as that would loop back to the original position
  {
    for(int j=0; j< numElements; j++)
    {
      posArray[j].y +=1;                    //Increase Y by 1, this results in a shift right on the LED matrix board
      if(posArray[j].y > 7)                 //If Y is greater than 7, it is no longer displayed on the board or valid
      {                                     //In this case, shift it to the left most position (y=0)
        posArray[j].y = 0;
      }
    }
  }
}

void shift_left(pos posArray[], int numElements, int numShifts)
{
  //Given a POS array most likely containing an animation model,
  //this function will shift those pixels left (Y--) on the LED Matrix board
  
  for(int i=0; i< (numShifts % 8); i++)     //There are a maximum of 7 shifts that can be made as that would loop back to the original position
  {
    for(int j=0; j< numElements; j++)
    {
      posArray[j].y -=1;                    //Decrease Y by 1, this results in a shift left on the LED matrix board
      if(posArray[j].y < 0)                 //If Y is less than 0, it is  no longer displayer on the board or valid
      {                                     //In this case, shift it to the right most position (y=7)
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
    posArray[i].x += (incrementX * numShifts);    //Perform the diagonal shifts (X)
    posArray[i].y += (incrementY * numShifts);    //Perform the diagonal shifts (Y)
  }
}

void preset_FFT(uint8_t constraint_max)
{
  //This is to make the FFT algorithm more publically available across all functions
  //This was taken from the Equalizer setup and replaced the 0-7 constraint with a 0-> imported Constraint Max
  
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
    data_avgs[i] = constrain(data_avgs[i], 0, constraint_max);       //Force the avgs to fall within 0-constraint_max so they may be displayed accurately
  }
}
/* ----------------------------------------------------------- */
/* ------------------ ANIMATION FUNCTIONS -------------------- */
/* ----------------------------------------------------------- */

void anim_start()
{
  // -- Animation Models -- //
  pos three[13] = {{1,2}, {1,3}, {1,4}, {1,5}, {2,5}, {3,3}, {3,4}, {3,5}, {4,5}, {5,2}, {5,3}, {5,4}, {5,5}};
  pos two[14] = {{1,2}, {1,3}, {1,4}, {1,5}, {2,5}, {3,5}, {3,4}, {3,3}, {3,2}, {4,2}, {5,2}, {5,3}, {5,4}, {5,5}};
  pos one[8] = {{1,3}, {1,4}, {2,4}, {3,4}, {4,4}, {5,4}, {5,3}, {5,5}};
  pos go[19] = {{2,1}, {2,2}, {2,3}, {3,1}, {4,1}, {5,1}, {5,2}, {5,3}, {4,3}, {2,5}, {3,5}, {4,5}, {5,5}, {5,6}, {5,7}, {4,7}, {3,7}, {2,7}, {2,6}};
  pos border[16] = {{0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}, {7,0}, {7,1}, {7,2}, {7,3}, {7,4}, {7,5}, {7,6}, {7,7}};

  leds_from_struct(border, 16, 64, 255, 100);       //Set a yellow border that persists across all stages of the animation (TOP/BOTTOM ROW)
    
  leds_from_struct(three, 13, 0, 255, 255);         //Light the '3' --> Set as RED
  LEDS.show();                                      
  delay(1000);
  leds_from_struct(three, 13, -1, 0, 0);            //Turn off the LEDS that made up the '3'
  leds_from_struct(two, 14, 0, 255, 255);           //Light the '2' --> Set as RED
  LEDS.show();
  delay(1000);
  leds_from_struct(two, 14, -1, 0, 0);              //Turn off the LEDS that made up the '2'
  leds_from_struct(one, 8, 0, 255, 255);            //Light the '1' --> Set as RED
  LEDS.show();
  delay(1000);
  leds_from_struct(one, 8, -1, 0, 0);               //Turn off the LEDS that made up the '1'
  leds_from_struct(go, 19, 160, 255, 255);          //Light the 'GO' --> Set as BLUE
  LEDS.show();
  delay(1000);
  LEDS.clear();                                     //Clear all the LEDS (Turn off the LEDS that made up the 'GO')
}


void anim_randPixel()
{
      int multiplier;                              //A multiplier that we will use to determine the number of random pixels
      audio_val = analogRead(MIC_PIN);             //Set the audio value based on the value read in from the microphone
      
      switch(audio_val)                            //Our switch for determining the multiplier value
      {                                            //It is based on the audio_value , HIGHER VALUE = HIGHER MULTIPLIER 
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
      if(multiplier > 0)                          //As long as the multiplier isn't 0 (Min. Audio val: 321)
      {                                           //Set a random number of pixels (0-7 * multiplier), set a random location for each and light as a random colour
        uint16_t numPixels = (rand() % ((7-0+1))) * multiplier;

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

void anim_wave1()
{
   // -- Animation Models -- //
   pos wave[8] = {{2,0}, {3,1}, {4,2}, {5,3}, {5,4}, {4,5}, {3,6}, {2,7}};        //Basically a sine wave shape
   int multiplier = 1;                                                            //A multiplier that we will use to determine the speed of the animation
   
   audio_val = analogRead(MIC_PIN);                                               //Set the audio value based on the value read in from the microphone
   switch(audio_val)                                                              //Our switch for determining multiplier value, HIGHER VALUE = HIGHER MULTIPLIER
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
   shift_right(wave, 8, state);                                                   //Shift right by X places based on the current animation state
   leds_from_struct(wave, 8, global_colour, 255, 127);                            //Once the leds have been shifted -> Set them through the helper function based on global colour
   leds[XY(wave[state].x, wave[state].y)] = CHSV(0, 0, 255);                      //Set one led to white
   LEDS.show();
   delay((40/ multiplier));                                                       //Set the speed based on the multiplier --> Louder music, faster moving
   leds_from_struct(wave, 8, -1, 0, 0);                                           //Reset back to black / Turn off LEDS
   
   state++;                                                                       //Increment state to control the number of shifts right
   if(state > 7)                                                                  //If this number is greater than 7, then it's looped, so set to 0
   {
      state = 0;
   }
   
   global_colour +=5;                                                             //Increment the global colour to create a rainbow effect
   if(global_colour > 255)                                                        //If this number is greater than 255, then reset back to 0
   {                                                                              //The colour value for HUE or RGB can only be 0-255
      global_colour = 0;
   }
}

void anim_wave2()
{
  EVERY_N_MILLIS(30)
  {
    LEDS.clear();                                                   //Clear previous LEDS

    preset_FFT(6);                                                   //Calculate the Fast Fourier Transform and constrain the output values to be between 0-4
    wave_audio_level = (data_avgs[2] - 1);                          //Set the wave_audio level to the values from one of the audio bins and reduce sensitivity
                                                                    //by reducing the value by 1 (effectively cutting the bottom 25% of the response)
    for(uint8_t i = 0; i < 7 ; i++)
    {                                                               //Shift All the (X,Y) values in the WAVE model to the left removing the value at i=0
      wave2[i].x = wave2[i+1].x;                                      //This will help keep the wave animation in motion                                                                
    } 
    
    if(wave_dir == 1)                                               // --- UPWARDS MOVEMENT --- //
    {
      if(amplitude == 3 || (wave_audio_level <= amplitude && amplitude !=0)) //If the amplitude is 3 (The max possible) or if the audio level isn't loud enough to permit creating a bigger wave
      {                                                                      //then it's time to start heading downwards.
        wave_dir = 2;                                                           //1. Change the wave direction to head DOWNWARDS
        wave2[7].x += 1;                                                        //2. Shift the foremost pixel down
        wave_height = amplitude;                                                //3. Set the height of the wave to the current amplitude --> To know how far down to go to create an even SINE WAVE
        amplitude--;                                                            //4. Decrement the current amplitude
      }
      else if(wave_audio_level > amplitude)                                 //If the Audio level is sufficient enough to keep going up...
      {
        wave2[7].x -=1;                                                         //1. Shift the foremost pixel up
        amplitude++;                                                            //2. Increment the current amplitude
      }
    }
    else                                                            // --- DOWNWARDS MOVEMENT --- //
    {
      if(amplitude == -3 || amplitude == (wave_height*-1))                  //If the amplitude is either at MAX or the height of the wave (determined by upwards movement)
      {                                                                     //then it's time to head back up.
        wave_dir = 1;                                                           //1. Change the wave direction to head UPWARDS 
        wave2[7].x -=1;                                                         //2. Shift the foremost pixel up
        amplitude++;                                                            //3. Increment the current amplitude
      }
      else                                                                 //If the height of the wave hasn't been reached yet and we can keep going down
      {
        wave2[7].x +=1;                                                         //1. Shift the foremost pixel down
        amplitude--;                                                            //2. Decrement the current amplitude
      }
    }

    leds_from_struct(wave2, 8, global_colour, 255, 127);            //Light the LEDS based on the positions from the WAVE model
    LEDS.show();                                                    //Show the LEDS
    incrementGlobalColour();                                             //Increment the global colour to create a rainbow effect
  }
}

void anim_genesis()
{
                    //1. Fill spaces until full
 EVERY_N_MILLIS(2)
 {
    if(construct == true)                                       //ALL 64 pixels are OFF/unlit, so it's time to construct                                     
    {
       bool found = false;                                      //This is our exit condition for the while loop
       while(found == false)
       {
         uint16_t randX = (rand() % ((7-0+1)));                     //Pick a random X and Y to light up with a randomly chosen brightness
         uint16_t randY = (rand() % ((7-0+1)));
         uint16_t randBrt = (rand() % ((200 - 100 + 1) + 100));     //The randomly chosen brightness

         if(!leds[XY(randX, randY)])                                //If the LED is off for the random X,Y then set the LED with the global colour and random Brightness
         {
            leds[XY(randX, randY)] = CHSV(global_colour, 255, randBrt);
            found = true;                                           //Make sure we exit the loop as necessary
            num_constructed +=1;                                    //and Increase our constructed pixel counter to keep track
            Serial.println(num_constructed, DEC);
         }

         if(num_constructed == 64)                                  //Once the constructed pixel counter reaches 64, we have filled the board
         {                                                          //So set the mode to deconstruct
            construct = false;
         }
       }
       
    }

    else if(construct == false)                                 //ALL 64 pixels are constructed, so it's time to DECONSTRUCT
    {
      bool found = false;                                       //This is our exit condition for the while loop
      while(found == false)
      {
        if(num_constructed == 1)                                //If Dealing with the final point --> Don't rely on random selection
        {
          for(uint8_t i=0; i<64; i++)
          {
            if(leds[i])                                         //Find the final point
            {
              while(leds[i])                                      //While the final point is lit...
              {                                                   //Fade the final point until it's black CRGB(0,0,0) 
                leds[i].fadeToBlackBy(32);                        //At which point leds[i] would be false. 
                LEDS.show();
              }
                                                                //After the final point is black
              num_constructed = 0;                                //Reset constructed pixels counter
              construct = true;                                   //Tell the program to start constructing again
              found = true;                                       //Set our exit condition for the while loop
              global_colour = setRandColor();                     //Set a random colour for the next genesis cycle
            }
          }
        }
        else                                                    //If NOT dealing with the final point --> Rely on random selection
        {
          uint16_t randX = (rand() % ((7-0+1)));                //Pick a random X
          uint16_t randY = (rand() % ((7-0+1)));                //Pick a random Y
        
          if(leds[XY(randX, randY)])                            //If the LED is on for the picked random co-ordinate, fade that LED by 12.5% (32/256)
          {
            leds[XY(randX, randY)].fadeToBlackBy(32);             //Fade the LED's brightness by 32/256 (12.5%)
            found = true;                                         //Set our exit condition for the while loop
 
            if(!leds[XY(randX, randY)])                         //If the LED is now OFF (black / crgb(0,0,0) ), reduce our constructed counter by 1
            {
              num_constructed -=1;                                //Reduce the constructed counter
              Serial.println(num_constructed, DEC);               //Debugging: Keep track of the number of constructed pixels
            }
          }
        }
        
      }
    }

    LEDS.show();
 }
}

void anim_spike()
{
  EVERY_N_MILLIS(20)                                                //Every 20 milliseconds repeat the code block
  {                                   
    for(uint8_t i=0; i<8; i++)                                      //Set the baseline across X=4)
    {                                                               //Set it to the global colour with half saturation and brightness
      leds[XY(4, i)] = CHSV(global_colour, 200, 127);                
    }
                                                                    //***SHIFT PREVIOUS VALUES LEFT AND RIGHT TOWARDS OUTSIDE EDGE***//
                                                                    //-------------------------------------------------------------//
    for( int x=1; x< 8; x++)                                        //All the LEDS will be checked between x=1 and x=7 (3 above/below x=4)
    {
      if(x!=4)                                                      //X=4 is our baseline, so these LEDS don't change
      {
                                                                    //1. SHIFTING RIGHT (y > 4) //                               
        for( int y=7; y > 4; y--)                                     //Note: We have to check from right to left
        {                                                             //We check if the led to the left is lit, if it is, the current led is lit and the other turned off
          if(y==7 && leds[XY(x, y)])                                  //Before we shift, we check that the rightmost LED is on or not, then we turn it off before moving the rest
          {                                                           //This is essentially the same as moving it right off the edge of the matrix.
            leds[XY(x, y)] = CRGB(0,0,0); 
          }
          else if(leds[XY(x, (y-1))])
          {    
            leds[XY(x, y-1)] = CRGB(0,0,0);                           //**Shifting essentially involves turning off the previous LED location and turning on the current LED
            leds[XY(x, y)] = CHSV(global_colour, 200, 127);           //Set to Global Colour, 50% saturation, 50% brightness
          }
        }
  
                                                                    //2. SHIFTING LEFT (Y < 3) //
        for( int y=0; y <3; y++)                                      //Note: We have to check from left to right
        {                                                             //We check if the led to the right is lit, if it is, the current led is lit and the other turned off
          if(y==0 && leds[XY(x, y)])                                  //Before we shift, we check that the leftmost LED is on or not, then we turn it off before moving the rest
          {                                                           //This is essentially the same as moving it left off the edge of the matrix
            leds[XY(x, y)] = CRGB(0,0,0); 
          }
          else if(leds[XY(x, (y+1))])
          {
            leds[XY(x, (y+1))] = CRGB(0,0,0);                         //**Shifting essentially involves turning off the previous LED location and turning on the current LED
            leds[XY(x, y)] = CHSV(global_colour, 200, 127);           //Set to Global colour, 50% saturation, 50% brightness
          }
        }
      }
    }
                                                                    //*** CALCULATE NEW VALUES ***//
    preset_FFT(4);                                                  //3. Perform the FFT with a constraint max of 4. (This means the values we will get back are in the range of 0-4
    
    for(uint8_t i=0; i< (data_avgs[2]-1); i++)                     //4. For the value given in one of the bands output by FastFFT with our constraint of 4
    {                                                               //   set the LEDs above and below X=4 for Y=3 and Y=4 (the middle two LED Columns)
      leds[XY(4+(i+1), 3)] = CHSV(global_colour, 200, 127);         //   e.g. This value is 2, we subtract the bottom 25% values (minus 1 from 4) so it's between 1-3
      leds[XY(4-(i+1), 3)] = CHSV(global_colour, 200, 127);         //        and we make the output a little less sensitive. Then we light X=5,6 and X= 2,3 which
                                                                    //        are the 2 points above and below X=4. On the next iteration, these points will be shifted
      leds[XY(4+(i+1), 4)] = CHSV(global_colour, 200, 127);         //        left and right respectively. ( <-- Y=3 | Y=4 --> )
      leds[XY(4-(i+1), 4)] = CHSV(global_colour, 200, 127);
    }
  
    LEDS.show();                                                    //5. After all the LEDS are set, display them.
    incrementGlobalColour();                                        //6. Increment the global colour so the colour transitions on the next iteration
  }
}

void anim_rain()
{
  bool add_new = false;
  int multiplier = 0;                                                     //A multiplier to determine the number of raindrops to fall
  
  if(analogRead(MIC_PIN) > 300)                                           //If a sound above the 300 threshold is heard, create a new raindroplet
  {                                                                       //after the shifting logic.
    add_new = true;                                                       //Set the add_new_led flag to true so it knows to add a new rain droplet
    multiplier = 1;                                                       //If it's between 301-340 , add just one droplet
  }
  if(analogRead(MIC_PIN) > 340)
  {
    multiplier = 2;                                                       //If it's between 341-380 , add two droplets
  }
  if(analogRead(MIC_PIN) > 380)
  {
    multiplier = 3;                                                       //If it's above 380, add three droplets
  }

  // -- RAINDROP SHIFTING LOGIC -- //
  for(int x =7; x > -1 ; x--)
  {
    for(int y=7 ; y > -1; y--)
    {
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
    }
  }

  // -- ADD NEW RAINDROPS -- //
  if(add_new == true)
  {
    for(uint8_t i=0; i< multiplier; i++)                                    //Repeat multiplier times to add multiplier chances at a raindrop falling.
    {
      int starting_point = (rand() % (7-0+1));                              //Pick a random starting point between 0-7 if sound is detected
      if(! leds[XY(0, starting_point)] && ! leds[XY(1, starting_point)])    //If there is no LED lit on that particular starting point or the next led down, set the new LED
      {
        leds[XY(0, starting_point)] = CHSV(160, 255, 255);                  //Set the starting point LED on the first row
      }
    }
    add_new = false;                                                      //Reset add new flag
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
  // -- Animation Models -- //
  pos center[4] =  {{3, 3},{3,4},{4,3}, {4,4}};
  pos s1[4] =  {{2,2}, {2,5},{5,2}, {5,5}}; 
  pos s2[4] = {{1,3}, {3,6}, {6,4}, {4,1}};
  pos s3[4] = {{1,4}, {4,6}, {6,3}, {3,1}};
  
  audio_val = analogRead(MIC_PIN);                  //Read in the audio level from the microphone

  leds_from_struct(center, 4, 96, 255, 127);        //Light up the center box first with a green colour
  
  if(audio_val > 300)                               //If the Audio value is above 300, then move between animation states
  {                                               
    state++;
    if(state > 3)
    {
      state = 1;                                    //If the final state is reached, reset and loop again
    }
  }
  
  switch(state)                                     //As the models change based on the state, it creates the animation effect
  {                                                 //The different models provide the location of the pixels
    case 1:                                         //The different variables passed through to leds_from_struct determine Colour/Saturation and brightness
      leds_from_struct(s1, 4, 64, 255, 127);        //The colour stays the same and the saturation changes to give off a blinking effect.
      break;                                        
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
  leds_from_struct(background, 8, 209, 18, 64);                                  //Set the LEDS for the Moon

  for(uint8_t i=0; i<NUM_LEDS; i++)
  {
    if(leds[i] == CRGB(0,0,0))
    {
      leds[i] = CRGB(0,0, 128);                                                 //For the other LEDS that are off, turn it into the night sky
      leds[i] %= 1;
    }
  }
  LEDS.show();

  /* ------------------------------------------------ */
  /* -------- DETERMINE STARTING VARIABLES ---------- */
  uint8_t starting_pointX = (rand() % (5-0+1));                                 //Set the starting point for the fireworks (X) (max X is 5 so the firework actually goes up)
  uint8_t starting_pointY = (rand() % (7-0+1));                                 //Set the starting point for the fireworks (Y)
  uint8_t limit = (rand() % (3-1 +1)) + 1;                                      //Set a random limit between 1 and 4 (to determine the fireworks size)
  uint16_t starting_color = setRandColor();                                     //Set a random starting color
  uint16_t next_color;                                                          //The next_colour variable which will be used to determine the colour of the next ring in the firework

  uint8_t multiplier = 0;
  audio_val = analogRead(MIC_PIN);                                              //Set the audio value based on read in values from the microphone
  switch(audio_val)                                                             //A multiplier switch based on read in audio_value which will determine delay speed.
  {
    case 300 ... 325:
      multiplier = 1;
      break;
    case 326 ... 350:
      multiplier = 2;
      break;
    case 351 ... 420:
      multiplier = 3;
      break;
    case 421 ... 1023:
      multiplier = 4;
      break;
  }
  if(multiplier > 0)
  {
    /* ------------------------------------------------ */
    /* -------- Watch the firework shoot up ----------- */
    for(uint8_t x = 7; x > starting_pointX; x--)
    {
      leds[XY(x, starting_pointY)] =  CHSV(0,0,100);                            //For each X value from 7 (the bottom of the grid) to the randomly chosen Pixel
      LEDS.show();                                                              //Set the colour of the led at (X, Y) to white then change the colour back to what it was before
      delay(40/multiplier);
      if(in_list((pos) {x, starting_pointY}, background, 8) == true)            //If the pixel coordinate matches the moon's location, it was part of the moon prior to lighting
      {
        leds[XY(x, starting_pointY)] = CHSV(209, 18, 64);                       //In that case, set the colour back to the colour of the moon
      }
      else                                                                      //If the pixel coordinate isn't part of the moon's location, it was part of the night sky background
      {
        leds[XY(x, starting_pointY)] = CRGB(0,0, 128);                          //In that case, set the colour back to the colour of the night sky and dim the brightness back to what it was before.
        leds[XY(x, starting_pointY)] %= 1;
      }
    }
  
    /* ------------------------------------------------- */
    /* -------------- Time for the Bang ---------------- */
    leds[XY(starting_pointX, starting_pointY)] = CHSV(starting_color, 255, 127);//Light the center point of the firework to the randomly chosen starting colour
    LEDS.show();
    delay(50/multiplier);
  
  
    // -- Fireworks exploding logic Variable initialization -- //
                                                                                // These variables help to keep track of values and help determine what points to light up next                                                                           
    bool previous_array[8][8];                                                  //Initialize array to store previous values FOR CALCULATION
    memset(previous_array, 0, 64*sizeof(bool));                                 //Set all values to 0
    previous_array[starting_pointX][starting_pointY] = true;                    //Set first value to our first pixel location
  
  
    // -- Fireworks Expanding/Exploding logic -- //
    for(uint8_t i=0; i<limit ; i++)
    {
      bool next_array[8][8];                                                    //Initialize array TO STORE calculated values
      memset(next_array, 0, 64*sizeof(bool));                                   //Ensure array is blank
      next_color = starting_color + 60*(i+1);                                   //Determine the next colour based on the current i and the starting colour
      if( next_color > 255)                                                     //The range of Hue is 0-255, each value represents a different colour
      {
        next_color = 0 + (next_color - 255);
      }
      
      //Determine next pixels using PREVIOUS AND NEXT ARRAYS
      for(uint8_t x=0; x<8; x++)                                                //Using the previous array, calculate what pixels to light next
      {                                                                         //Check all pixels, if the pixel is adjacent to one in the previous_array
        for(uint8_t y=0; y<8; y++)                                              //and hasn't already been lit (marked true), light it (mark as true)
        {                                                                       //and set it to the colour of next_colour
          if(previous_array[x][y] == true)                                      //Adjacent variables are either (x-1), (x+1), (y-1), (y+1)
          {                                                                     //As long as they are within the boundaries of the 8x8 grid.
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
      for(uint8_t x=0; x<8; x++)                                                //Add the recently calculated values to the previous_array
      {                                                                         //so that during the next iteration, the adjacent pixels can once again be calculated
        for(uint8_t y=0; y<8; y++)                                              //based on comparison with previous_array.
        {
          if(next_array[x][y] == true)
          {
            previous_array[x][y] = true;
          }
        }
      }
  
      LEDS.show();                                                              //With all the values set to be lit, turn on the relevant LEDS
      delay(50/multiplier);                                                     //delay by 50ms (based on multiplier), then repeat the calculation
    }
    
    delay(100/multiplier);
    LEDS.clear();
  }
}

void anim_daytime()
{
  // --- Animation Models --- //
  pos sun[4] = {{1,1}, {1,2}, {2,1}, {2,2}};                                  //The sun body
  pos sun_1[8] = {{3,1}, {0,2}, {2,0}, {1,3}, {1,0}, {2,3}, {3,2}, {0,1}};    //The points for the sun ray animation
  pos tree_1[6] = {{2,5}, {2,6}, {3,4}, {3,5}, {3,6}, {3,7}};                 //The leaves of the tree
  pos tree_2[8] = {{4,5}, {4,6}, {5,5}, {5,6}, {6,4}, {6,5}, {6,6}, {6,7}};   //The base of the tree

  // --- Setting Models --- //
  leds_from_struct(sun, 4,  64, 200, 100);                                    //Set the Colour of the sun
  leds_from_struct(sun_1, 8, 64, 200, 90);                                    //Set the colour of the sun rays (non-animated part)
  leds_from_struct(tree_1, 6, 96, 255, 200);                                  //Set the colour of the leaves
  leds_from_struct(tree_2, 8, 30, 255, 80);                                   //Set the colour of the Tree base
  
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
      else if(leds[XY(x,y)].r == 0 && leds[XY(x, y)].g == 0 && leds[XY(x, y)].b == 0)    
      {                                                                        //Check to see if the pixel is on (black)
        leds[XY(x,y)] = CRGB::DeepSkyBlue;                                     //Set the colour of the sky
        leds[XY(x,y)] %= 10;                                                   //Fade the colour immensely
      }
    }
  }

  //  -- Animate the Sun -- //
  if(state > 3)
  {
    state = 0;                                                                //Check that the state isn't greater than 3 (from other modes, or incrementing)
  }
  leds[XY(sun_1[state*2].x, sun_1[state*2].y)] = CRGB::Gold;                  //Set the colour of two of the points within the sun's animation points based on the current state
  leds[XY(sun_1[state*2 +1].x, sun_1[state*2 + 1].y)] = CRGB::Gold;
  state++;                                                                    //Increment the state so the colour cycles around and animates
  
  LEDS.show();
  delay(50);
  LEDS.clear();
}

void anim_cycle()
{
  //  --- Animation Models --- //
  pos sun[4] = {{7,7}, {7,8}, {8,7}, {8,8}};                                  //Start the animation in the bottom left corner only showing one pixel
  pos moon[8] = {{7,7}, {7,8}, {8,7}, {8,8}, {9,8}, {9,9}, {6,8}, {6,9}};     //Rely on the leds_from_struct logic to not turn on any LEDS outside 8x8 grid parameters
  
  // --- Logic --- //
  
  if(state > 19)                                                              //There are 20 states total -> 9 for Sun/Moon each and 1 each as a gap
  {                                                                           //If it exceeds these 20 states, reset back to the first state
    state = 0;
    
  }

  if(state == 0 || state == 10)                                               //On the first and middle state, show only the background
  {
    //Show background only
  }
  else if(state > 0 && state < 6)                                             //On state 1-5 Shift the sun upwards
  {
    shift_diagonal(sun, 4, state-1, 3);                                       //Shift the sun upwards
    leds_from_struct(sun, 4, 64, 255, 127);                                   //Colour the  sun
    for(uint8_t i=0; i<64; i++)                                               //For each of the LEDS, set the brightness based on the current STATE value
    {                                                                         //The higher the sun, the brighter it becomes
      if(! leds[i])
      {
        leds[i] = CHSV(160, 255, 20+ state*30);
      }
    }
  }
  else if(state > 5 && state < 10)                                            //On state 6-9 Shift the sun Upwards then downwards based on the current state
  {
    shift_diagonal(sun, 4, 4, 3);                                             //Shift the sun upwards first
    shift_diagonal(sun, 4, (state-1)-4, 4);                                   //Then shift the sun downwards
    leds_from_struct(sun, 4, 64, 255, 127);                                   //Colour the sun
    for(uint8_t i=0; i<64; i++)                                               //For each of the LEDS, set the brightness based on the current STATE value
    {                                                                         //The higher the sun, the brighter it becomes
      if(! leds[i])
      {
        leds[i] = CHSV(160, 255, 170 - (state-4)*30);
      }
    }
  }
  else if(state > 10 && state < 16)                                           //On state 11-15 Shift the moon upwards based on the current state
  {
    shift_diagonal(moon, 8, (state-11), 3);                                   //Shift the moon upwards
    leds_from_struct(moon, 8, 0, 0, 127);                                     //Colour the moon
    for(uint8_t i=0; i<64; i++)                                               //For each of the LEDS, set the colour and dimmness of the night sky
    {
      if(! leds[i])
      {
        leds[i] = CHSV(160, 255, 20);
      }
    }
  }               
  else if(state > 15 && state < 20)                                           //On state 16-19 Shift the moon upwards then downwards base don the current state
  {
    shift_diagonal(moon, 8, 4, 3);                                            //Shift the moon upwards first
    shift_diagonal(moon, 8, (state-11)-4, 4);                                 //Then shift the moon downwards
    leds_from_struct(moon, 8, 0, 0, 127);                                     //Colour the moon
    for(uint8_t i=0; i<64; i++)                                               //For each of the LEDS, set the colour and dimmness of the night sky
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
  state++;                                                                    //After displaying the LEDS and clearing them, increment the state so the next stage of the animation can occur  
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
  // -- Animation Models -- //
  // -- Other models located in Global Variables Area at the top of this file -- //
  pos wall[8] = {{0,7}, {1,7}, {2,7}, {3,7}, {4,7}, {5,7}, {6,7}, {7,7}};
  pos game_over_face[20] = {{0,1}, {0,3}, {0,4}, {0,6}, {1,2}, {1,5}, {2,1}, {2,3}, {2,4}, {2,6}, 
                            {6,1}, {5,1}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {5,6}, {6,6}};

  if(game_over == true)                                                           //If the game has ended and the function has been called again
  {                                                                               //Reset the global variables that the game uses
    //anim_start();
    ball[0].x = (rand() % (6-1 +1) + 1);
    ball[0].y = (rand() % (5-4 +1) + 4);
    ball_direction = (rand() % (4-1 +1) + 1);
    game_over = false;
  }
  while(digitalRead(BUTTON_PIN) != HIGH && game_over == false)                    //As the loop stays within the function, set an exit condition
  {
    EVERY_N_MILLIS(40)                                                            //Every 40 milliseconds (The slower this is, the slower it refreshes)       
    {                                                                             //Grab the audio value & Shift the paddle according to the value
      if(digitalRead(BUTTON_PIN) == HIGH && buttonPrevious == LOW)                // Added the menu change logic within the function as the button press
      {                                                                           // Logic isn't detected within the function otherwise 
        //Detect button press --> exit game
        programMode++;                                                                //Change the mode
        state = 0;                                                                    //Reset Animation state for next function
        global_colour = 0;                                                            //Reset Global Colour variable for animations
        LEDS.clear();                                                                 //Clear the LEDS from the previous mode
        
        if(programMode > 11)                                                          //If the program mode clicks past 7, reset back to the first 
        {
          programMode= 1;
        }
        buttonPrevious = buttonCurrent;                                               //This is to prevent re-entering the IF statement on long press of the button
      } 
      
      
      preset_FFT(6);                                                              //In order to determine the audio, we call the FFT algorithm with a constraint of (0-6)                                           
      int8_t move_val = data_avgs[2] - 3;                                         //We collect the result from one particular band, in this case, the 3rd one, and remove
      if (move_val < 0)                                                           //the bottom 50% of values to remove noise and sensitivity.
      {                                                                           //If the move value collected is below 0 after removing 3 , then set it to 0.
        move_val = 0;
      }
      
      if(move_val == 1 || move_val == 0)                                          //If the move value is either 1 or 0, move the paddle down
      {                                                                           //As long as the paddle is not resting on the BOTTOM of X          
                                                                                  
        if(in_list((pos){7,0}, paddle, 3) == false)                               //Check if (7,0) [the bottom] doesn't exist in the paddles position variables
        {
          shift_down(paddle, 3, 1);                                               //If it isn't, then it's not at the bottom, so shift down 1 space
        }
      }
      else if(move_val == 3 || move_val == 2)                                     //If the move value is 2 or 3, shift the paddle up
      {                                                                           //As long as the paddle isn't sitting at the TOP of X 
        //Check to see if it's not resting on the TOP of X
        if(in_list((pos){0,0}, paddle, 3) == false)                               //Check if (0,0) doesn't exist in the paddles position variables
        {                                      
          shift_up(paddle, 3, 1);                                                 //If it doesn't exist, it isn't sitting at the TOP, so shift up 1 space
        }
      }

      // -- Light the corresponding models based on their calculated positions -- //
      leds_from_struct(paddle, 3, 100, 255, 255);                                 //Set the lights for the Paddle
      leds_from_struct(wall, 8, 100, 255, 255);                                   //Set the lights for the Wall
      leds_from_struct(ball, 1, 100, 255, 255);                                   //Set the lights for the Ball
  
      LEDS.show();                                                                //Turn on the LEDS for all SET leds
      LEDS.clear();                                                               //Clear lights for next iteration
    }
    EVERY_N_MILLIS(200)
    {
      // -- IF THE BALL HITS A WALL --> CHANGE THE BALL DIRECTION -- //
      
      if(ball[0].y == 1 && (in_list((pos) {ball[0].x, 0}, paddle, 3) == false))   //This logic basically determines which direction the ball will bounce once a ball hits a wall
      {                                                                           //Firstly, if the ball reaches Y=0, that means it didn't bounce off the Paddle and thus it's game over.
        game_over = true;                           
      }
      else if(ball[0].y == 6 && ball[0].x ==0 || ball[0].y == 6 & ball[0].x == 7 || ball[0].y == 1 && ball[0].x == 0 || ball[0].y == 1 && ball[0].x == 7)
      {
        switch(ball_direction)                                                    //Here it checks to see if the ball landed in the corner against the wall or the paddle
        {                                                                         //For example, the paddle is one pixel wide, thus if the ball bounces off the paddle, it will be
          case 1:                                                                 //in position Y=1, if it's in X=0, Y=1, then it's in the bottom corner
            ball_direction = 4;                                                   //The outgoing direction of the ball then would be the inverse direction of the incoming direction
            break;                                                                //That's what this switch statement determines. (The outgoing direction)
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
      else if(ball[0].y == 6 || ball[0].y==1)                                     //This switch statement is for when the ball hits the PADDLE or the WALL and is not perfectly in a corner
      {                                                                           //This would result in the ball being deflected rather than being sent in the opposite direction
        switch(ball_direction)                                                    //That's what this switch statement determines. (The outgoing direction)
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
      else if(ball[0].x == 0 || ball[0].x == 7)                                   //This switch statement is for when the ball hits the top or bottom (THE SIDES) of the field
      {                                                                           //The ball would be deflected but still in a forward moving direction 
        switch(ball_direction)                                                    //That's what this switch statement determines. (The outgoing direction)
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
      shift_diagonal(ball, 1, 1, ball_direction);                                 //Once the direction has been determined, move the ball 1 space to it's next location
    }
  }

  if(game_over == true)
  {                                                                               //If the Ball reaches Y=0, then game_over will become true
    leds_from_struct(game_over_face, 20, 0, 255, 127);                            //Set the Game over screen
    LEDS.show();                                                                  //Show the Game over screen
    delay(2000);                                                                  //Wait 2 seconds
    LEDS.clear();                                                                 //Clear and reset
  }
  
}
