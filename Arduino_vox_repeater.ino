//Matthew Miller
//24-March-2013

/*
Arduino pins
#define voltSensePin 2
  //set config for radio A
  radioA.voxPin=0;
  radioA.micPin=2;
  radioA.pttPin=3;
  radioA.autoId=true;
  radioA.battMon=true;
  radioA.lastBattMonTime=idTimeout;
  radioA.lastIdTime=idTimeout;
  radioA.lastVoxTime=0;

  //set config for radio B  
  radioB.voxPin=1;
  radioB.micPin=12;
  radioB.pttPin=13;
  radioB.autoId=false;
  radioB.battMon=false;
  radioB.lastBattMonTime=idTimeout;
  radioB.lastIdTime=idTimeout;
  radioB.lastVoxTime=0;

ATTiny84 pins
#define voltSensePin 4
  //set config for radio A
  radioA.voxPin=10;
  radioA.micPin=9;
  radioA.pttPin=8;
  radioA.autoId=true;  //0
  radioA.battMon=true; //1
  radioA.lastBattMonTime=idTimeout;
  radioA.lastIdTime=idTimeout;
  radioA.lastVoxTime=0;

  //set config for radio B  
  radioB.voxPin=7;
  radioB.micPin=6;
  radioB.pttPin=5;
  radioB.autoId=false;  //2
  radioB.battMon=false; //3
  radioB.lastBattMonTime=idTimeout;
  radioB.lastIdTime=idTimeout;
  radioB.lastVoxTime=0;
*/

//Analog pin for voltage sense
#define voltSensePin 4

//define threshold for low battery
#define lowBattThreshold 11.5

//how many milliseconds to ID every
//600000 is 10 minutes in milliseconds
#define idTimeout 600000

//define input value for no-audio
#define normVal 511

//define deviation to activate VOX
#define devVal 4

//morse code "dit" base unit length in milliseconds
#define ditLen 60

//the pitch for the tone
#define tonePitch 800

//data structure for radio info
struct Radio
{
  //voxPin - Audio In (for VOX) - Analog Pin
  //micPin - MIC Mix (for tone) - Digital Pin
  //pttPin - PTT OUT (for TX)   - Digital Pin
  int micPin, pttPin, voxPin, autoId, battMon;
  long lastBattMonTime, lastIdTime, lastVoxTime;
};

//globals to store radio config
Radio radioA, radioB;

//declarations for functions which use radio struct
void configure(Radio &radio);
void vox(Radio &radio);
void txAutoId(Radio &radio);
void lowBattCheck(Radio &radio);
boolean isBusy(Radio &radio);

void setup() {
  
  radioA.voxPin=5;
  radioA.micPin=8;
  radioA.pttPin=9;
  radioA.autoId=true;  //0
  radioA.battMon=true; //1
  radioA.lastBattMonTime=idTimeout;
  radioA.lastIdTime=idTimeout;
  radioA.lastVoxTime=0;

  //set config for radio B
  radioB.voxPin=6;
  radioB.micPin=7;
  radioB.pttPin=3;
  radioB.autoId=false;  //2
  radioB.battMon=false; //3
  radioB.lastBattMonTime=idTimeout;
  radioB.lastIdTime=idTimeout;
  radioB.lastVoxTime=0;
  
  //apply config for radios (set pinmode/etc)
  configure(radioA);
  configure(radioB);
  
 morseCode(radioA.micPin,"@%");
  
  //broadcast ID if applicable
  txAutoId(radioA);
  txAutoId(radioB);
}

//configures pinmode and setup for radio
void configure(Radio &radio)
{
  pinMode(radio.micPin,OUTPUT);
  pinMode(radio.pttPin,OUTPUT);
  digitalWrite(radio.micPin,LOW);
  digitalWrite(radio.pttPin,LOW);
}

void loop()
{
  if(!isBusy(radioB)) //if the other radio is transmitting, this one must be receiving so don't key up
  {
    lowBattCheck(radioA);
    txAutoId(radioA);
    vox(radioA);
  }
    
  if(!isBusy(radioA)) //if the other radio is transmitting, this one must be receiving so don't key up 
  {
    lowBattCheck(radioB);
    txAutoId(radioB);
    vox(radioB);
  }
}

//checks if a radio's PTT pin is keyed
boolean isBusy(Radio &radio)
{
  return digitalRead(radio.pttPin);
}

//checks if feature is enabled (if pin is true/false)
boolean isEnabled(int pin)
{
  return pin; //temp just return true/false coded
  //return digitalRead(pin);
}

//trigger PTT based on VOX input and delay
void vox(Radio &radio)
{
  // read the input on analog pins
  int voxVal = analogRead(radio.voxPin);
  
  // test if the pin has audio
  if(voxVal>(normVal+devVal) || voxVal<(normVal-devVal))
  {
    //vox active
    digitalWrite(radio.pttPin,HIGH);
    radio.lastVoxTime=millis();
  }
  else
  {
    if(millis()-radio.lastVoxTime < 500)
    {
      //vox delay
    }
    else
    {
      digitalWrite(radio.pttPin,LOW);
    }
  }
  delay(1); //stability to prevent bouncing
}

//broadcast ID if applicable
void txAutoId(Radio &radio)
{
  if(isEnabled(radio.autoId) && (millis()-radio.lastIdTime) > idTimeout)
  {
    boolean tx=digitalRead(radio.pttPin);
    digitalWrite(radio.pttPin,HIGH);
    delay(500);
    //kk4nde(micPin);
    morseCode(radio.micPin,"kk4nde");
    radio.lastIdTime=millis();
    digitalWrite(radio.pttPin,tx);
  }
}

//broadcast low battery if applicable
void lowBattCheck(Radio &radio)
{
  float voltage=getPowerVoltage(voltSensePin);
  if(isEnabled(radio.battMon) && voltage < lowBattThreshold && voltage > 9 && (millis()-radio.lastBattMonTime) > idTimeout)
  {
    boolean tx=digitalRead(radio.pttPin);
    digitalWrite(radio.pttPin,HIGH);
    delay(500);
    
    //encode low battery morse code message
//    char temp[]="lb ";
//    strcat(temp,voltage);
//    strcat(temp,"v");
//    morseCode(radio.micPin,temp);
    morseCode(radio.micPin,"lb");
    //morseCode(radio.micPin,"lb "+ toString(voltage) + "v");
    
    radio.lastBattMonTime=millis();
    digitalWrite(radio.pttPin,tx);
  }
}

//for floats from ~1 to ~19
void strcat(char * appendTo, float input)
{
  char temp[]="x";
  if(input > 10)
  {
    strcat(appendTo,"1");
    input-=10;
  }
  temp[0]=(char)((int)input+48); //add 48 to shift to ascii value
  strcat(appendTo,temp); //append to output
  input-=(int)input; //take off whole value
  strcat(appendTo,".");
  input*=10; //iterate to first place past decimal
  if((input-(int)input)>.4) //round (because here we will drop everything after decimal)
    input++;
  temp[0]=(char)((int)input+48); //add 48 to shift to ascii value
  strcat(appendTo,temp); //append to output
}

//send morse code message by applying tone to codePin
void morseCode(int codePin, char* message)
{
  // message.trim();
  //message.toLowerCase();
  int code;
  int length;
   for(unsigned int x=0; x < strlen(message); x++)
   {
     //shift case
     if(message[x] < 91 && message[x] > 64)
       message[x]+=32;
       
     //encode morse code
     switch(message[x])
     {
       case 'a':
                 //strcpy(temp,".-");
                 code=B01;
                 length=2;
                 break;
       case 'b':
                 //strcpy(temp,"-...");
                 code=B1000;
                 length=4;
                 break;
       case 'c':
                 //strcpy(temp,"-.-.");
                 code=B1010;
                 length=4;
                 break;
       case 'd':
                 //strcpy(temp,"-..");
                 code=B100;
                 length=3;
                 break;
       case 'e':
                 //strcpy(temp,".");
                 code=B0;
                 length=1;
                 break;
       case 'f':
                 //strcpy(temp,"..-.");
                 code=B0010;
                 length=4;
                 break;
       case 'g':
                 //strcpy(temp,"--.");
                 code=B110;
                 length=3;
                 break;
       case 'h':
                 //strcpy(temp,"....");
                 code=B0000;
                 length=4;
                 break;
       case 'i':
                 //strcpy(temp,"..");
                 code=B00;
                 length=2;
                 break;
       case 'j':
                 //strcpy(temp,".---");
                 code=B0111;
                 length=4;
                 break;
       case 'k':
                 //strcpy(temp,"-.-");
                 code=B101;
                 length=3;
                 break;
       case 'l':
                 //strcpy(temp,".-..");
                 code=B0100;
                 length=4;
                 break;
       case 'm':
                 //strcpy(temp,"--");
                 code=B11;
                 length=2;
                 break;
       case 'n':
                 //strcpy(temp,"-.");
                 code=B10;
                 length=2;
                 break;
       case 'o':
                 //strcpy(temp,"---");
                 code=B111;
                 length=3;
                 break;
       case 'p':
                 //strcpy(temp,".--.");
                 code=B0110;
                 length=4;
                 break;
       case 'q':
                 //strcpy(temp,"--.-");
                 code=B1101;
                 length=4;
                 break;
       case 'r':
                 //strcpy(temp,".-.");
                 code=B010;
                 length=3;
                 break;
       case 's':
                 //strcpy(temp,"...");
                 code=B000;
                 length=3;
                 break;
       case 't':
                 //strcpy(temp,"-");
                 code=B1;
                 length=1;
                 break;
       case 'u':
                 //strcpy(temp,"..-");
                 code=B001;
                 length=3;
                 break;
       case 'v':
                 //strcpy(temp,"...-");
                 code=B0001;
                 length=4;
                 break;
       case 'w':
                 //strcpy(temp,".--");
                 code=B011;
                 length=3;
                 break;
       case 'x':
                 //strcpy(temp,"-..-");
                 code=B1001;
                 length=4;
                 break;
       case 'y':
                 //strcpy(temp,"-.--");
                 code=B1011;
                 length=4;
                 break;
       case 'z':
                 //strcpy(temp,"--..");
                 code=B1100;
                 length=4;
                 break;
       case '0':
                 //strcpy(temp,"-----");
                 code=B11111;
                 length=5;
                 break;
       case '1':
                 //strcpy(temp,".----");
                 code=B01111;
                 length=5;
                 break;
       case '2':
                 //strcpy(temp,"..---");
                 code=B00111;
                 length=5;
                 break;
       case '3':
                 //strcpy(temp,"...--");
                 code=B00011;
                 length=5;
                 break;
       case '4':
                 //strcpy(temp,"....-");
                 code=B00001;
                 length=5;
                 break;
       case '5':
                 //strcpy(temp,".....");
                 code=B00000;
                 length=5;
                 break;
       case '6':
                 //strcpy(temp,"-....");
                 code=B10000;
                 length=5;
                 break;
       case '7':
                 //strcpy(temp,"--...");
                 code=B11000;
                 length=5;
                 break;
       case '8':
                 //strcpy(temp,"---..");
                 code=B11100;
                 length=5;
                 break;
       case '9':
                 //strcpy(temp,"----.");
                 code=B11110;
                 length=5;
                 break;
       case ' ':
                 //strcpy(temp,"");
                 code=B0;
                 length=0;
                 delay(7*ditLen);
                 break;
       case '.':
                 //strcpy(temp,".-.-.-");
                 code=B010101;
                 length=6;
                 break;
       case '/':
                 //strcpy(temp,"-..-.");
                 code=B10010;
                 length=5;
                 break;
       case '-':
                 //strcpy(temp,"-....-");
                 code=B100001;
                 length=6;
                 break;
       case '?':
                 //strcpy(temp,"..--..");
                 code=B001100;
                 length=6;
                 break;
       case '@': //debug symbol
                 code=analogRead(radioA.voxPin)/4;
                 length=8;
                 break;
       case '%': //debug symbol
                 code=analogRead(radioB.voxPin)/4;
                 length=8;
                 break;
       default:
                 //strcpy(temp,"");
                 code=B0;
                 length=0;
                 break;
     }
     
     while(length > 0)
     {
       //determine if it's a dit or a dot
       if(code & bitMask(length))
       {
         //1 is a dit
         tone(codePin,tonePitch);
         delay(3*ditLen);
         noTone(codePin);
         delay(ditLen);
       }
       else
       {
         //0 is a dot
         tone(codePin,tonePitch);
         delay(ditLen);
         noTone(codePin);
         delay(ditLen);
       }
       length--;
     }
     delay(ditLen);
   }
}

//generates a "bit mask" for getting nth bit from int
int bitMask(int bitNumber)
{
  int value=1;
  for(int x=1; x < bitNumber; x++)
    value*=2;
  return value;
}

//gets voltage from analog input pin
float getPowerVoltage(int pin)
{
  // R1 = 2200 (Vin to midpoint)
  // R2 = 1000 (midpoint to gnd)
  // put 5.1v protectioin zener in parallel for R2 to protect arduino input against overvolt
  // formula:
  //     ( value/1023 * (arduino vcc) ) / (  R2   /     (R2 + R1)    ) + (Vin diode drop)
  //return ((analogRead(pin)/1023.0)*5.0) / (1000.0 / (2200.0 + 1000.0)) + 0.76 ;
  return (analogRead(pin)*0.0156402737047898) + 0.76; //simplified
}
