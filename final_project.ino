#include <SPI.h>
#include <FreeStack.h>
#include <SdFat.h>
#include <vs1053_SdFat.h>

SdFat sd;
vs1053 MP3player;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//remapped sd pin on arduino mega
const int sdPin = 53;
//init laser pins - digital
int laser1 = 37;
int laser2 = 35;
int laser3 = 33;
int laser4 = 31;
int laser5 = 29;
int laser6 = 27;
int laser7 = 25;
int laser8 = 23;
//init switch pins - digital
int switch1Pin = 24;
int switch1state = 0;
int switch2Pin = 26;
int switch2state = 0;
int switch3Pin = 28;
int switch3state = 0;
//init photo resistor pins - analog 
int photo1Pin = A7;
int photo1 = 0;
int photo2Pin = A8;
int photo2 = 0;
int photo3Pin = A9;
int photo3 = 0;
int photo4Pin = A10;
int photo4 = 0;
int photo5Pin = A11;
int photo5 = 0;
int photo6Pin = A12;
int photo6 = 0;
int photo7Pin = A13;
int photo7 = 0;
int photo8Pin = A14;
int photo8 = 0;

int photoVal = 300; // value that photocells must go below in order to play track
bool isPaused = false;
int songVal = 1;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
void setup() {
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //setup lasers
  pinMode(laser1, OUTPUT);
  pinMode(laser2, OUTPUT);
  pinMode(laser3, OUTPUT);
  pinMode(laser4, OUTPUT);
  pinMode(laser5, OUTPUT);
  pinMode(laser6, OUTPUT);
  pinMode(laser7, OUTPUT);
  pinMode(laser8, OUTPUT);
  //setup switches
  pinMode(switch1Pin, INPUT);
  pinMode(switch2Pin, INPUT);
  pinMode(switch3Pin, INPUT);
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  uint8_t result;
  Serial.begin(115200);
  //Initialize the SdCard.
  if(!sd.begin(sdPin, SPI_FULL_SPEED)) sd.initErrorHalt();
  if(!sd.chdir("/")) sd.errorHalt("sd.chdir");

  //Initialize the MP3 Player Shield
  result = MP3player.begin();
  Serial.println(F("Use the \"d\" command to verify SdCard can be read")); // can be removed for space, if needed.


#if defined(__BIOFEEDBACK_MEGA__) // or other reasons, of your choosing.
  // Typically not used by most shields, hence commented out.
  Serial.println(F("Applying ADMixer patch."));
  if(MP3player.ADMixerLoad("admxster.053") == 0) {
    Serial.println(F("Setting ADMixer Volume."));
    MP3player.ADMixerVol(-3);
  }
#endif

  help();
}



void loop() {
 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //read in switch values 
  switch1state = digitalRead(switch1Pin);
  switch2state = digitalRead(switch2Pin);
  switch3state = digitalRead(switch3Pin);

  //if all lasers have been turned on, run function to play songs

  //write all of the lasers high in the begining or by switches
  digitalWrite(laser3, HIGH); 
  digitalWrite(laser4, HIGH);
  digitalWrite(laser5, HIGH);

  if(switch1state == HIGH)
  {
    digitalWrite(laser1,HIGH);
    digitalWrite(laser7, HIGH);
  }

  if(switch2state == HIGH)
  {
    digitalWrite(laser2, HIGH);
    digitalWrite(laser6, HIGH);
  }
  
  if(switch1state == 1 && switch2state == 1 && switch3state == 1)
  {
  playJams();
  }
  else
  {
    digitalWrite(laser1,LOW);
    digitalWrite(laser7, LOW);
    digitalWrite(laser2, LOW);
    digitalWrite(laser6, LOW);
    
  }



  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if(Serial.available()) {
    parse_menu(Serial.read()); // get command from serial input
  }

  delay(100);
}

uint32_t  millis_prv;

//------------------------------------------------------------------------------

void parse_menu(byte key_command) {

  uint8_t result; // result code from some function as to be tested at later time.


  Serial.print(F("Received command: "));
  Serial.write(key_command);
  Serial.println(F(" "));

  //if s, stop the current track
  if(key_command == 's') {
    Serial.println(F("Stopping"));
    MP3player.stopTrack();

  //if 1-9, play corresponding track
  } else if(key_command >= '1' && key_command <= '9') {
    //convert ascii numbers to real numbers
    key_command = key_command - 48;

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
    //tell the MP3 Shield to play a track
    result = MP3player.playTrack(key_command);

    //check result, see readme for error codes.
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to play track"));
    } 

  //if +/- to change volume
  } else if((key_command == '-') || (key_command == '+')) {
    union twobyte mp3_vol; // create key_command existing variable that can be both word and double byte of left and right.
    mp3_vol.word = MP3player.getVolume(); // returns a double uint8_t of Left and Right packed into int16_t

    if(key_command == '-') { // note dB is negative
      // assume equal balance and use byte[1] for math
      if(mp3_vol.byte[1] >= 254) { // range check
        mp3_vol.byte[1] = 254;
      } else {
        mp3_vol.byte[1] += 2; // keep it simpler with whole dB's
      }
    } else {
      if(mp3_vol.byte[1] <= 2) { // range check
        mp3_vol.byte[1] = 2;
      } else {
        mp3_vol.byte[1] -= 2;
      }
    }
    // push byte[1] into both left and right assuming equal balance.
    MP3player.setVolume(mp3_vol.byte[1], mp3_vol.byte[1]); // commit new volume
    Serial.print(F("Volume changed to -"));
    Serial.print(mp3_vol.byte[1]>>1, 1);
    Serial.println(F("[dB]"));

  //if < or > to change Play Speed
  } else if((key_command == '>') || (key_command == '<')) {
    uint16_t playspeed = MP3player.getPlaySpeed(); // create key_command existing variable
    // note playspeed of Zero is equal to ONE, normal speed.
    if(key_command == '>') { // note dB is negative
      // assume equal balance and use byte[1] for math
      if(playspeed >= 254) { // range check
        playspeed = 5;
      } else {
        playspeed += 1; // keep it simpler with whole dB's
      }
    } else {
      if(playspeed == 0) { // range check
        playspeed = 0;
      } else {
        playspeed -= 1;
      }
    }
    MP3player.setPlaySpeed(playspeed); // commit new playspeed
    Serial.print(F("playspeed to "));
    Serial.println(playspeed, DEC);

  /* Alterativly, you could call a track by it's file name by using playMP3(filename);
  But you must stick to 8.1 filenames, only 8 characters long, and 3 for the extension */
  } else if(key_command == 'f' || key_command == 'F') {
    uint32_t offset = 0;
    if (key_command == 'F') {
      offset = 2000;
    }

    //create a string with the filename
    char trackName[] = "track001.mp3";

#if USE_MULTIPLE_CARDS
    sd.chvol(); // assign desired sdcard's volume.
#endif
    //tell the MP3 Shield to play that file
    result = MP3player.playMP3(trackName, offset);
    //check result, see readme for error codes.
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to play track"));
    }

  /* Display the file on the SdCard */
  } else if(key_command == 'd') {
    if(!MP3player.isPlaying()) {
      Serial.println(F("Files found (name date time size):"));
      sd.ls(LS_R | LS_DATE | LS_SIZE);
    } else {
      Serial.println(F("Busy Playing Files, try again later."));
    }

  /* Get and Display the Audio Information */
  } else if(key_command == 'i') {
    MP3player.getAudioInfo();

  } else if(key_command == 'p') {
    if( MP3player.getState() == playback) {
      MP3player.pauseMusic();
      Serial.println(F("Pausing"));
    } else if( MP3player.getState() == paused_playback) {
      MP3player.resumeMusic();
      Serial.println(F("Resuming"));
    } else {
      Serial.println(F("Not Playing!"));
    }

  } else if(key_command == 't') {
    int8_t teststate = MP3player.enableTestSineWave(126);
    if(teststate == -1) {
      Serial.println(F("Un-Available while playing music or chip in reset."));
    } else if(teststate == 1) {
      Serial.println(F("Enabling Test Sine Wave"));
    } else if(teststate == 2) {
      MP3player.disableTestSineWave();
      Serial.println(F("Disabling Test Sine Wave"));
    }

  } else if(key_command == 'S') {
    Serial.println(F("Current State of VS10xx is."));
    Serial.print(F("isPlaying() = "));
    Serial.println(MP3player.isPlaying());

    Serial.print(F("getState() = "));
    switch (MP3player.getState()) {
    case uninitialized:
      Serial.print(F("uninitialized"));
      break;
    case initialized:
      Serial.print(F("initialized"));
      break;
    case deactivated:
      Serial.print(F("deactivated"));
      break;
    case loading:
      Serial.print(F("loading"));
      break;
    case ready:
      Serial.print(F("ready"));
      break;
    case playback:
      Serial.print(F("playback"));
      break;
    case paused_playback:
      Serial.print(F("paused_playback"));
      break;
    case testing_memory:
      Serial.print(F("testing_memory"));
      break;
    case testing_sinewave:
      Serial.print(F("testing_sinewave"));
      break;
    }
    Serial.println();

   } else if(key_command == 'b') {
    Serial.println(F("Playing Static MIDI file."));
    MP3player.SendSingleMIDInote();
    Serial.println(F("Ended Static MIDI file."));

#if !defined(__AVR_ATmega32U4__)
  } else if(key_command == 'm') {
      uint16_t teststate = MP3player.memoryTest();
    if(teststate == -1) {
      Serial.println(F("Un-Available while playing music or chip in reset."));
    } else if(teststate == 2) {
      teststate = MP3player.disableTestSineWave();
      Serial.println(F("Un-Available while Sine Wave Test"));
    } else {
      Serial.print(F("Memory Test Results = "));
      Serial.println(teststate, HEX);
      Serial.println(F("Result should be 0x83FF."));
      Serial.println(F("Reset is needed to recover to normal operation"));
    }

  } else if(key_command == 'e') {
    uint8_t earspeaker = MP3player.getEarSpeaker();
    if(earspeaker >= 3){
      earspeaker = 0;
    } else {
      earspeaker++;
    }
    MP3player.setEarSpeaker(earspeaker); // commit new earspeaker
    Serial.print(F("earspeaker to "));
    Serial.println(earspeaker, DEC);

  } else if(key_command == 'r') {
    MP3player.resumeMusic(2000);

  } else if(key_command == 'R') {
    MP3player.stopTrack();
    MP3player.vs_init();
    Serial.println(F("Reseting VS10xx chip"));

  } else if(key_command == 'g') {
    int32_t offset_ms = 20000; // Note this is just an example, try your own number.
    Serial.print(F("jumping to "));
    Serial.print(offset_ms, DEC);
    Serial.println(F("[milliseconds]"));
    result = MP3player.skipTo(offset_ms);
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to skip track"));
    }

  } else if(key_command == 'k') {
    int32_t offset_ms = -1000; // Note this is just an example, try your own number.
    Serial.print(F("moving = "));
    Serial.print(offset_ms, DEC);
    Serial.println(F("[milliseconds]"));
    result = MP3player.skip(offset_ms);
    if(result != 0) {
      Serial.print(F("Error code: "));
      Serial.print(result);
      Serial.println(F(" when trying to skip track"));
    }

  } else if(key_command == 'O') {
    MP3player.end();
    Serial.println(F("VS10xx placed into low power reset mode."));

  } else if(key_command == 'o') {
    MP3player.begin();
    Serial.println(F("VS10xx restored from low power reset mode."));

  } else if(key_command == 'D') {
    uint16_t diff_state = MP3player.getDifferentialOutput();
    Serial.print(F("Differential Mode "));
    if(diff_state == 0) {
      MP3player.setDifferentialOutput(1);
      Serial.println(F("Enabled."));
    } else {
      MP3player.setDifferentialOutput(0);
      Serial.println(F("Disabled."));
    }

  } else if(key_command == 'V') {
    MP3player.setVUmeter(1);
    Serial.println(F("Use \"No line ending\""));
    Serial.print(F("VU meter = "));
    Serial.println(MP3player.getVUmeter());
    Serial.println(F("Hit Any key to stop."));

    while(!Serial.available()) {
      union twobyte vu;
      vu.word = MP3player.getVUlevel();
      Serial.print(F("VU: L = "));
      Serial.print(vu.byte[1]);
      Serial.print(F(" / R = "));
      Serial.print(vu.byte[0]);
      Serial.println(" dB");
      delay(1000);
    }
    Serial.read();

    MP3player.setVUmeter(0);
    Serial.print(F("VU meter = "));
    Serial.println(MP3player.getVUmeter());

  } else if(key_command == 'T') {
    uint16_t TrebleFrequency = MP3player.getTrebleFrequency();
    Serial.print(F("Former TrebleFrequency = "));
    Serial.println(TrebleFrequency, DEC);
    if (TrebleFrequency >= 15000) { // Range is from 0 - 1500Hz
      TrebleFrequency = 0;
    } else {
      TrebleFrequency += 1000;
    }
    MP3player.setTrebleFrequency(TrebleFrequency);
    Serial.print(F("New TrebleFrequency = "));
    Serial.println(MP3player.getTrebleFrequency(), DEC);

  } else if(key_command == 'E') {
    int8_t TrebleAmplitude = MP3player.getTrebleAmplitude();
    Serial.print(F("Former TrebleAmplitude = "));
    Serial.println(TrebleAmplitude, DEC);
    if (TrebleAmplitude >= 7) { // Range is from -8 - 7dB
      TrebleAmplitude = -8;
    } else {
      TrebleAmplitude++;
    }
    MP3player.setTrebleAmplitude(TrebleAmplitude);
    Serial.print(F("New TrebleAmplitude = "));
    Serial.println(MP3player.getTrebleAmplitude(), DEC);

  } else if(key_command == 'B') {
    uint16_t BassFrequency = MP3player.getBassFrequency();
    Serial.print(F("Former BassFrequency = "));
    Serial.println(BassFrequency, DEC);
    if (BassFrequency >= 150) { // Range is from 20hz - 150hz
      BassFrequency = 0;
    } else {
      BassFrequency += 10;
    }
    MP3player.setBassFrequency(BassFrequency);
    Serial.print(F("New BassFrequency = "));
    Serial.println(MP3player.getBassFrequency(), DEC);

  } else if(key_command == 'C') {
    uint16_t BassAmplitude = MP3player.getBassAmplitude();
    Serial.print(F("Former BassAmplitude = "));
    Serial.println(BassAmplitude, DEC);
    if (BassAmplitude >= 15) { // Range is from 0 - 15dB
      BassAmplitude = 0;
    } else {
      BassAmplitude++;
    }
    MP3player.setBassAmplitude(BassAmplitude);
    Serial.print(F("New BassAmplitude = "));
    Serial.println(MP3player.getBassAmplitude(), DEC);

  } else if(key_command == 'M') {
    uint16_t monostate = MP3player.getMonoMode();
    Serial.print(F("Mono Mode "));
    if(monostate == 0) {
      MP3player.setMonoMode(1);
      Serial.println(F("Enabled."));
    } else {
      MP3player.setMonoMode(0);
      Serial.println(F("Disabled."));
    }
#endif

  } else if(key_command == 'h') {
    help();
  }

  // print prompt after key stroke has been processed.
  Serial.print(F("Time since last command: "));  
  Serial.println((float) (millis() -  millis_prv)/1000, 2);  
  millis_prv = millis();
  Serial.print(F("Enter s,1-9,+,-,>,<,f,F,d,i,p,t,S,b"));
  Serial.println(F(",h :"));
}


void help() {
  Serial.println(F("COMMANDS:"));
  Serial.println(F(" [1-7] to play a track"));
  Serial.println(F(" [s] to stop playing"));
  Serial.println(F(" [d] display directory of SdCard"));
  Serial.println(F(" [+ or -] to change volume"));
  Serial.println(F(" [> or <] to increment or decrement play speed by 1 factor"));
  Serial.println(F(" [i] retrieve current audio information (partial list)"));
  Serial.println(F(" [p] to pause."));
  Serial.println(F(" [S] Show State of Device."));
  Serial.println(F(" [b] Play a MIDI File Beep"));
  Serial.println(F(" [h] this help"));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void playJams()
{
  //read in photo resistor values
  photo1 = analogRead(photo1Pin);
  photo2 = analogRead(photo2Pin);
  photo3 = analogRead(photo3Pin);
  photo4 = analogRead(photo4Pin);
  photo5 = analogRead(photo5Pin);
  photo6 = analogRead(photo6Pin);
  photo7 = analogRead(photo7Pin);
  photo8 = analogRead(photo8Pin);

  //Serial.print(photo1);
  Serial.println(photo7);
 // Serial.print("photo2: ");
 // Serial.println(photo2);
 // Serial.print("photo3: ");
 // Serial.println(photo3);
  delay(500);
  //plays the first track
  if(photo1 < photoVal)
  {
    MP3player.playTrack(songVal);
  }
  //pauses the track 
  if(photo2 < photoVal && isPaused == false)
  {
    MP3player.pauseMusic();
    isPaused = true;
    
  }
  //resume the track 
  if(photo2 > photoVal && isPaused == true)
  {
    MP3player.resumeMusic();
    isPaused = false;
  }
  //stop the current track playing 
  if(photo3 < photoVal)
  {
    MP3player.stopTrack();
  }
  //skip to the next track 
  if(photo4 < photoVal)
  {
    MP3player.stopTrack();
    songVal++;
    MP3player.playTrack(songVal);
    Serial.println(songVal);
    //mp3 can only play track 1-9
    if(songVal > 9)
    {
      songVal = 1;
      
    }
  }
  //go back to the previous track 
  if(photo5 < photoVal)
  {
    MP3player.stopTrack();
    songVal--;
    MP3player.playTrack(songVal);
    //Serial.println(songVal);
    //mp3 can only play tracks 1-9
    if(songVal < 1)
    {
      songVal = 9;   
    }
  }

  //gets volume amount 
  union twobyte mp3_vol;
  mp3_vol.word = MP3player.getVolume();
  
  //increase the volume 
  if(photo6 < photoVal)
  {
    if(mp3_vol.byte[1] <= 2) { // range check
        mp3_vol.byte[1] = 2;
      } else {
        mp3_vol.byte[1] -= 2;
      }
      
    MP3player.setVolume(mp3_vol.byte[1], mp3_vol.byte[1]); // commit new volume
    Serial.print(F("Volume changed to -"));
    Serial.print(mp3_vol.byte[1]>>1, 1);
    Serial.println(F("[dB]"));
  }
  //decrease the volume
  if(photo7 < photoVal)
  {
    if(mp3_vol.byte[1] >= 254) { // range check
        mp3_vol.byte[1] = 254;
      } else {
        mp3_vol.byte[1] += 2; // keep it simpler with whole dB's
      }
    MP3player.setVolume(mp3_vol.byte[1], mp3_vol.byte[1]); // commit new volume
    Serial.print(F("Volume changed to -"));
    Serial.print(mp3_vol.byte[1]>>1, 1);
    Serial.println(F("[dB]"));


    
  } 

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
