// Game information
// ----------------------------------------------------------------------------
// Created by: Steven Smethurst
// Last updated: April 30, 2017
//
// ToDo:
// - Reset button instead of pulling the power.
// - Rethink the way that speed is done.

#include "FastLED.h"

// Game Settings
// ----------------------------------------------------------------------------
// The number of LEDs in this strip.
static const unsigned char SETTINGS_NUM_LEDS = 30;

// Speed
// Ms per LED change. Lower is faster.
static const unsigned short SETTINGS_SPEED_STARTING = 200;
static const unsigned short SETTINGS_SPEED_INCREMENT = 50;
static const unsigned short SETTINGS_SPEED_LIMIT = 10 ; 

// In hue
// https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors
// 0 = Red
// 96 = Green
static const unsigned char SETTING_COLOR_OUT_OF_BOUNDS = 0;
static const unsigned char SETTING_COLOR_CURSOR = 96;

// Pins
static const unsigned char SETTING_PIN_LED_DATA = D4;
static const unsigned char SETTING_PIN_PLAYER_BUTTON = D6;

// Debug
static const unsigned short SETTING_SERIAL_BAUD_RATE = 9600;

// Game variables
// ----------------------------------------------------------------------------
bool currentDirection; // What direction the cursor is currently moving.
static const bool DIRECTION_UP = true; // Default
static const bool DIRECTION_DOWN = false;

unsigned char mode; // What mode the game is currently in
static const unsigned char MODE_DEMO = 0; // Default
static const unsigned char MODE_GAME_PLAYING = 1;
static const unsigned char MODE_GAME_OVER = 2;

// Limits for the out of bounds areas
unsigned short limitHigh;
unsigned short limitLow;

unsigned short currentPosition;
int currentSpeed;
unsigned long timerGameStart; // time in ms when the game started. This is used to calculate the score
unsigned long timerGameEnd;

// Timer between frames.
unsigned long timerLastMove = 0;

// The current status of all the LEDS
CRGB leds[SETTINGS_NUM_LEDS];

void setup()
{
    pinMode(SETTING_PIN_PLAYER_BUTTON, INPUT_PULLUP);
    FastLED.addLeds<NEOPIXEL, SETTING_PIN_LED_DATA>(leds, SETTINGS_NUM_LEDS);

    // initialize serial communications
    Serial.begin(SETTING_SERIAL_BAUD_RATE);

    reset();
}

void reset()
{
    mode = MODE_DEMO;
    currentDirection = DIRECTION_UP;
    limitHigh = SETTINGS_NUM_LEDS - 1;
    limitLow = 1;
    currentPosition = limitLow + 1;
    currentSpeed = SETTINGS_SPEED_STARTING;
    timerLastMove = 0;
    timerGameStart = 0;
    timerGameEnd = 0;

    // Set all the LEDS to black
    for (unsigned short ledOffset = 0; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
        leds[ledOffset] = CRGB::Black;
    }

    Serial.println("Reset()");
    DebugPrintCurrentStatus(); 
}

void DebugPrintCurrentStatus()
{
    Serial.print(String(millis()) + " " ); 
    Serial.print(" Dir=" + String( currentDirection == DIRECTION_UP ? "Up" : "Down" ));
    Serial.print(" Pos=" + String(currentPosition));
    Serial.print(" limitHigh=" + String(limitHigh));
    Serial.print(" limitLow=" + String(limitLow));
    Serial.print(" Speed=" + String(currentSpeed));
    Serial.print(" NextMove=" + String( (millis() - timerLastMove)) );
    Serial.print(" Score=" + String(GetScore()));    
    
    Serial.println("");
}

void DoRender()
{
    // Update the LEDs on the screen based on the current game settings.

    // Middle
    for (unsigned short ledOffset = limitLow; ledOffset < limitHigh; ledOffset++) {
        leds[ledOffset] = CRGB::Black;
    }

    // Cursor
    // Make the cursor a little bigger then a single LED. Make the fridges half as bright.
    leds[currentPosition - 1] = CHSV(SETTING_COLOR_CURSOR, 255, 255 / 8);
    leds[currentPosition] = CHSV(SETTING_COLOR_CURSOR, 255, 255);
    leds[currentPosition + 1] = CHSV(SETTING_COLOR_CURSOR, 255, 255 / 8);


    // Low limits
    for (unsigned short ledOffset = 0; ledOffset <= limitLow; ledOffset++) {
        leds[ledOffset] = CHSV(SETTING_COLOR_OUT_OF_BOUNDS, 255, 255);
    }
    // High limits
    for (unsigned short ledOffset = limitHigh; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
        leds[ledOffset] = CHSV(SETTING_COLOR_OUT_OF_BOUNDS, 255, 255);
    }

    // Push the data to the LEDS.
    FastLED.show();
}

void RenderScore()
{
    // todo
}

unsigned long GetScore()
{
    if( timerGameEnd > 0 ) { 
        return timerGameEnd - timerGameStart;
    } else {
        return millis() - timerGameStart;
    }
}

bool ButtonPressed()
{
    bool currentButtonState = digitalRead(SETTING_PIN_PLAYER_BUTTON) ; 
    static bool lastButtonState = true; 
    if( currentButtonState == lastButtonState ) {
        // Nothing changed 
        return false; 
    }
    
    lastButtonState = currentButtonState ; 
    if( currentButtonState ) {
        Serial.println("Player button pressed.");
        return true ; 
    }
    
    return false; 
}

void DoDemo()
{
    // See if the user has pressed the button
    if (ButtonPressed()) {
        // The user has pressed the button, start the game.
        mode = MODE_GAME_PLAYING;
        timerGameStart = millis();

        Serial.println("Start game");
        DebugPrintCurrentStatus(); 
        return ; 
    }
    Serial.println("Doing demo");

    // ToDo: Fill the LEDs with intersting patterns.

    // Rainbow
    static unsigned char hue = 0;
    hue += 1;

    for (unsigned short ledOffset = 0; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
        leds[ledOffset] = CHSV(hue + ledOffset *2, 255, 255);
    }

    // Push the data to the LEDS.
    FastLED.show();
    
}

void DoGameOver()
{
    // Check to see if we need to do anything
    if (timerLastMove + 500 > millis()) {
        // Not time yet, nothing to do.
        return;
    }

    static bool blinkStatus = true;

    // Flash limit that the user hit
    if (currentPosition <= limitLow) {
        // Check to see if it is in the Off/black
        if (!blinkStatus) {
            // Currently off. Turn it back on
            for (unsigned short ledOffset = 0; ledOffset < limitLow; ledOffset++) {
                leds[ledOffset] = CHSV(SETTING_COLOR_OUT_OF_BOUNDS, 255, 255);
            }
        } else {
            // Currently on, Turn it off
            for (unsigned short ledOffset = 0; ledOffset < limitLow; ledOffset++) {
                leds[ledOffset] = CRGB::Black;
            }
        }
    } else if (currentPosition >= limitHigh) {
        // Check to see if it is in the Off/black
        if (blinkStatus) {
            // Currently off. Turn it back on
            for (unsigned short ledOffset = limitHigh; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
                leds[ledOffset] = CHSV(SETTING_COLOR_OUT_OF_BOUNDS, 255, 255);
            }
        } else {
            // Currently on, Turn it off
            for (unsigned short ledOffset = limitHigh; ledOffset < SETTINGS_NUM_LEDS; ledOffset++) {
                leds[ledOffset] = CRGB::Black;
            }
        }
    }

    blinkStatus = !blinkStatus;

    // we have updated the LEDs.
    timerLastMove = millis();
    // Push the data to the LEDS.
    FastLED.show();

    // Show the game score.
    RenderScore();
}

void DoMove()
{
    // This calculates the fames per second that we are getting.
    // ToDo: Is this needed?, we will only be off by one frame at most. The cursor will be moving so fast that the user probably won't even notice it.
    static unsigned long lastFame = 0;
    lastFame = millis() - lastFame;

    // We want to use a fame timer to move.
    // This make the code work the same on different speed proccessors
    if (timerLastMove + currentSpeed > millis()) {
        // Not time to move the cursor yet.
        return;
    }

    // Move the cursor
    if (DIRECTION_UP == currentDirection) {
        currentPosition++;
    } else {
        currentPosition--;
    }

    // We have moved the cursor, update the timer.
    timerLastMove = millis();

    // The position of the LEDs has changed, re-render the LEDs.
    DoRender();
}

void loop()
{
    // Check to see what mode we are in.
    if (MODE_DEMO == mode) {
        DoDemo();
        return;
    } else if (MODE_GAME_OVER == mode) {
        DoGameOver();
        return;
    }

    DebugPrintCurrentStatus(); 

    // Check for game ending condition.
    // This needs to happend before we check for the button press. 
    if (currentPosition <= limitLow || currentPosition >= limitHigh) {
        // The game has ended, show score and wait for user input.
        Serial.println("Game over");
        timerGameEnd = millis();
        mode = MODE_GAME_OVER;
        return;
    }    

    // Check to see if the user has done anything in this loop
    if (ButtonPressed()) {
        // The button has been pressed.
        // Depending on the direction of the cursor, set the limits
        if (DIRECTION_UP == currentDirection) {
            limitHigh = currentPosition;
            currentPosition--; // Move the cursor off the current limit
        } else {
            limitLow = currentPosition;
            currentPosition++; // Move the cursor off the current limit
        }

        // Toggle the movement direction
        currentDirection = !currentDirection;

        // Increase the speed
        currentSpeed -= SETTINGS_SPEED_INCREMENT;
        if( currentSpeed < SETTINGS_SPEED_LIMIT) {
            currentSpeed = SETTINGS_SPEED_LIMIT ; 
        }
       
        DoRender();
        return ; 
    }


    DoMove();
    RenderScore();
}
