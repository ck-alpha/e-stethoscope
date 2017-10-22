// Code for the E-Stethoscope - a stethoscope prototype capable of simulating clinical signs
// Cyrus Razavi and Juan Adriano
// Presented at RCoA / SCATA GasHack 21-22nd October 2017
//
// Based on example code from learn.adafruit.com - Wave Shield worked example
// New elements: Copyright 2017 Cyrus Razavi and Juan Adriano. All rights reserved

#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"

SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the filesystem on the card
FatReader f;      // This holds the information for the file we're play

WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

static byte previous;
#define DEBOUNCE 100  // Debouncer for capacitance board

// File names
char* filenames[] = {"1.WAV", "2.WAV", "3.WAV", "4.WAV", "5.WAV", "6.WAV", "7.WAV", "8.WAV", "9.WAV", "10.WAV", "11.WAV", "12.WAV", "13.WAV", "14.WAV", "15.WAV", "16.WAV", "17.WAV", "18.WAV", "19.WAV", "20.WAV"};
int playno = 0;

// this handy function will return the number of bytes currently free in RAM, great for debugging!   
int freeRam(void)
{
  extern int  __bss_end; 
  extern int  *__brkval; 
  int free_memory; 
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end); 
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval); 
  }
  return free_memory; 
} 

void sdErrorCheck(void)
{
  if (!card.errorCode()) return;
  putstring("\n\rSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  putstring(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}

void setup() {
  // set up serial port
  Serial.begin(9600);
  putstring_nl("E-Stethoscope by Cyrus Razavi and Juan Adriano");
  
  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!
  
  // Set the output pins for the DAC control. This pins are defined in the library
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
 
  // pin13 LED
  pinMode(13, OUTPUT);
 
  // Enable pull-up resistors on switch pins (analog inputs)
  digitalWrite(14, LOW);

  // Button to change the sound
  digitalWrite(19, HIGH);
  
  //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    putstring_nl("Card init. failed!");  // Something went wrong, lets print out why
    sdErrorCheck();
    while(1);                            // then 'halt' - do nothing!
  }
  
  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);
 
// Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {     // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                             // we found one, lets bail
  }
  if (part == 5) {                       // if we ended up not finding one  :(
    putstring_nl("No valid FAT partition!");
    sdErrorCheck();      // Something went wrong, lets print out why
    while(1);                            // then 'halt' - do nothing!
  }
  
  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(),DEC);     // FAT16 or FAT32?
  
  // Try to open the root directory
  if (!root.openRoot(vol)) {
    putstring_nl("Can't open root dir!"); // Something went wrong,
    while(1);                             // then 'halt' - do nothing!
  }
  
  // Whew! We got past the tough parts.
  putstring_nl("E-Stethoscope Ready!");
}

void loop()
{
  static long time;
  byte reading;
  byte pressed;
  reading = digitalRead(14);

    if (digitalRead(19) == LOW)
    {
      putstring("Next sound selected: number ");
      playno++;
      if (playno > 19) {
        playno = 0;
      }
      Serial.println(playno + 1);
      if (wave.isplaying) { // already playing something, so stop it!
         wave.stop(); // stop it
         playfile(filenames[playno]); //play new file
         putstring("Playing number ");
         Serial.println(playno + 1);
        }
      delay(500); // Debounces and prevents rapid cycling on holding down button
    }

   if (reading == HIGH && previous == LOW && millis() - time > DEBOUNCE)
      {  
        // switch pressed
        putstring("Playing number ");
        Serial.println(playno + 1);      
        time = millis();
        playfile(filenames[playno]);
        previous = reading;
      }
      else if (reading == LOW && previous == HIGH && millis() - time > DEBOUNCE)
      {
        // switch unpressed
        time = millis();
        if (wave.isplaying) { // already playing something, so stop it!
         wave.stop(); // stop it
        }
        previous = reading;
      }
      else if (reading == HIGH && previous == HIGH && millis() - time > DEBOUNCE)
      {
        if (!wave.isplaying) {
          playfile(filenames[playno]); // effectively loops the audio
        }
      }
      else
      { 
      }
    delay(20); // seems to be required to debounce capacitance sensor
}

void playfile(char *name) {
  // see if the wave object is currently doing something
  if (wave.isplaying) { // already playing something, so stop it!
    wave.stop(); // stop it
  }
  // look in the root directory and open the file
  if (!f.open(root, name)) {
    putstring("Couldn't open file "); Serial.print(name); return;
  }
  // OK read the file and turn it into a wave object
  if (!wave.create(f)) {
    putstring_nl("Not a valid WAV"); return;
  }
  
  // ok time to play! start playback
  wave.play();
}
