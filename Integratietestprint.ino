 /****************************************************************************************************************************
   .-'''-. ,---.    ,---.   ____    .-------. ,---------.   .-'''-. .--.      .--..-./`) ,---------.   _______   .---.  .---.
  / _     \|    \  /    | .'  __ `. |  _ _   \\          \ / _     \|  |_     |  |\ .-.')\          \ /   __  \  |   |  |_ _|
 (`' )/`--'|  ,  \/  ,  |/   '  \  \| ( ' )  | `--.  ,---'(`' )/`--'| _( )_   |  |/ `-' \ `--.  ,---'| ,_/  \__) |   |  ( ' )
(_ o _).   |  |\_   /|  ||___|  /  ||(_ o _) /    |   \  (_ o _).   |(_ o _)  |  | `-'`"`    |   \ ,-./  )       |   '-(_{;}_)
 (_,_). '. |  _( )_/ |  |   _.-`   || (_,_).' __  :_ _:   (_,_). '. | (_,_) \ |  | .---.     :_ _: \  '_ '`)     |      (_,_)
.---.  \  :| (_ o _) |  |.'   _    ||  |\ \  |  | (_I_)  .---.  \  :|  |/    \|  | |   |     (_I_)  > (_)  )  __ | _ _--.   |
\    `-'  ||  (_,_)  |  ||  _( )_  ||  | \ `'   /(_(=)_) \    `-'  ||  '  /\  `  | |   |    (_(=)_)(  .  .-'_/  )|( ' ) |   |
 \       / |  |      |  |\ (_ o _) /|  |  \    /  (_I_)   \       / |    /  \    | |   |     (_I_)  `-'`-'     / (_{;}_)|   |
  `-...-'  '--'      '--' '.(_,_).' ''-'   `'-'   '---'    `-...-'  `---'    `---` '---'     '---'    `._____.'  '(_,_) '---'
******************************************************by Fernand Verstraete*************************************************/


#include "pin_def_ext.h"
#include "GSM_2560.h"
#include <string.h>


#include <LiquidCrystal.h>

// GSM module pins definition

const int GSM_RESET = A8; // GSM Module RESET on pin 89
const int  GSM_RNG	= 2; // Sim800 RNG on INT4
const int GSM_DTR = 9; // GSM Module DTR on pin 88
#define SIM800 	Serial2 // SIM800 RX,TX itf on Serial2

#define LedPin				13 // arduino led
#define LCDLedPin			A0 // LCD led


// LCD constructor
LiquidCrystal lcd(A6, A5, A4, A3, A2, A1);
//         END definitions V4+


// instance of GSM class
GSM gsm;

// GSM variables
  char TESTBOX[20]= "+32474022239"; // nr testbox
// +32474022239
// String SimOut = ""; // not used any more
int error;
volatile bool SmsReceived = false;
char number_incoming[20];
char sms_rx[122]; //Received text SMS
char GsmTime[21]; // time in Gsm module
char SignalStrength[2]; // strength of received Gsm signal


//Sms fields
byte type_sms = SMS_ALL;      //Type of SMS
char GatewayTime[21] = ""; //  char or string? FV
String SmsMessType = "";
String SmsPortNum = "";
char SmsCustGsmNum[14];
long SmsCredits = 0;
String SmsDummy1 = "";          
String SmsDummy2 = "";          
long SmsID;
long IdBuffer[3];
uint8_t IdBufferindex = 0;
uint8_t MaxIdBuffer = 3;
boolean SmsIsOK;


volatile int SmsStackIndex = -1;
String ResendSmsStack[10]; // Stack to hold unsent SMS's
volatile int ResendSmsStackIndex = -1; // index to ResendSmsStack

int selectA = 22;
int selectB = 24;
int selectC = 26;
int enable = 28;

String rapportMeting= "";

/****************************************************
  ________  _______  _________  ___  ___  ________
  |\   ____\|\  ___ \|\___   ___\\  \|\  \|\   __  \
  \ \  \___|\ \   __/\|___ \  \_\ \  \\\  \ \  \|\  \
  \ \_____  \ \  \_|/__  \ \  \ \ \  \\\  \ \   ____\
  \|____|\  \ \  \_|\ \  \ \  \ \ \  \\\  \ \  \___|
    ____\_\  \ \_______\  \ \__\ \ \_______\ \__\
   |\_________\|_______|   \|__|  \|_______|\|__|
   \|_________|
*****************************************************/
void setup() {

	Serial.begin(115200);

	pinMode(GSM_RNG, INPUT_PULLUP); // GSm Ring interrupt
  pinMode(selectA,OUTPUT);
  pinMode(selectB,OUTPUT);
  pinMode(selectC,OUTPUT);
  pinMode(enable,OUTPUT);


	//Init Display
	lcd.begin(20, 4); // initialize the lcd

#ifndef V2
	pinMode(LCDLedPin, OUTPUT); // V4+

#endif

	SetBacklightOn();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Integratietestprint");
  delay(2000);

	// Reset GSM
	lcd.clear();	// clear display, set cursor position to zero
	lcd.setCursor(0, 0);
	lcd.print("Initializing GSM");
	gsm.TurnOn(9600);	//module reset power on; factory reset, disable echo
	

	Serial.println("End GSM Reset, trying to connect");

	
	// test connection with provider
	delay(7000); // wait 7 seconds, then see if registration ok
	//GetRegistered();
	gsm.CheckRegistration(); // updates status gsm.IsRegistered(), does InitParam(PARAM_SET_1) if needed
	
	
	Serial.println("GSM Init Done");

	delay(1000);
  
  Serial.println(gsm.IsRegistered());
	// look for received SMS
	 if (gsm.IsRegistered()) {
	    Serial.println("GSM is registered");
      // PowerUsageReset(false, false, false, false);
      // Check_SMS();
      // delay(2000);
      // KioskInit("19/05/20,15:30:32+00;ik;+228;;;;;775135380");
      // Channels(1,50);
      // delay(1000);
      // PowerUsageSet(false, false, false, true);
      // PowerUsageReset(false, false, false, false);
      // KioskAskStatus("19/05/21,16:17:43+00;rcs;;;;;;942583047");
      // KioskDeleteClient(1);
      // Channels(1, 500);
      // PowerUsage();
      // KioskDeleteClient(1);
      // delay(500);
      // delay(7000);
      // Check_SMS();
      // delay(1000);
      //KioskCalculateCredit();
      // KioskAddCredit("19/05/22,11:15:44+00;acc;1;;100;;;687412345");
	  }


  // lcd.clear();
  // lcd.setCursor(0,0);
  // lcd.print("Start testen");
  // delay(5000);

  // lcd.clear();
  // lcd.setCursor(0,0);
  // lcd.print("Testen ...");


  Serial.println("einde");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Einde");





}  // end setup ***************************************

void KioskCalculateCredit() {
  int val[8];
  int j = 0;
  String hond = "1,2,3,4,5,6,7,8,";
  // char lalal = hond.charAt(0);
  // int joow = lalal - '0';
  for (int i = 0; i < 14; i=i+2)
  {
  char lalal = hond.charAt(i);
  int joow = lalal - '0';
    val[j] = joow; 
    j++;
  }

  Serial.println(val[8]);

  String meting [4] = {"Stroomafname perfect", "Stroomafname binnen de marge", "slechte callibratie", "slechte bekabeling" }
  // SmsPortNum
  // charAt(index) haal een karakter uit een string met index

  if (val[1] == 0) {
    Serial.println(meting[0]);
    rapportMeting = meting[0];
  }
  
  else if (val[1] < 0.5)
  {
    Serial.println(meting[1]);
    rapportMeting = meting[01;
  }
  else if (val[1] < 10)
  {
    Serial.println(meting[2]);
    rapportMeting = meting[2];
  }
  else
  {
    Serial.println(meting[3]);
    rapportMeting = meting[3];
  }
}

void rapport () {
  Serial.println(rapportMeting);
}
void KioskInit(String messageIK) {

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Testen ...");
  lcd.setCursor(0,2);
  lcd.print("Initialize");
  lcd.setCursor(0,3);
  lcd.print("SolergieBox");

  char ConvertedMessageIK[121] = "";
  
  messageIK.toCharArray(ConvertedMessageIK,121);

  if (gsm.IsRegistered()){
      gsm.SendSMS(TESTBOX, ConvertedMessageIK);
      Serial.println("Message sent: ");
      Serial.print (ConvertedMessageIK);
      Serial.print(" to: ");
      Serial.println(TESTBOX);
      delay(15000);
      Check_SMS();
      delay(2000);
  } 
}


void KioskAddClient(String messageIC) {

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Testen ...");
  lcd.setCursor(0,2);
  lcd.print("Klant toevoegen" );

  char ConvertedMessageIC[121] = "";
  Serial.println("1");
  messageIC.toCharArray(ConvertedMessageIC,121);
  Serial.println("2");
  if (gsm.IsRegistered()){
    Serial.println("3");
      gsm.SendSMS(TESTBOX, ConvertedMessageIC);
      delay(2000);
      Serial.println("4");
      Serial.println("Message sent: ");
      Serial.print (ConvertedMessageIC);
      Serial.print(" to: ");
      Serial.println(TESTBOX);
  } 
}


void KioskAddCredit(String messageACC) {

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Testen ...");
  lcd.setCursor(0,2);
  lcd.print("Credits toevoegen" );

  char ConvertedMessageACC[121] = "";
  
  messageACC.toCharArray(ConvertedMessageACC,121);

  if (gsm.IsRegistered()){
      gsm.SendSMS(TESTBOX, ConvertedMessageACC);
      Serial.println("Message sent: ");
      Serial.print (ConvertedMessageACC);
      Serial.print(" to: ");
      Serial.println(TESTBOX);
  } 
}


void Channels (int kanaal, int credit) {

  // int omzetten naar string met nieuwe variabele naam kanaalnumber en valueCredit
  String kanaalnummer = String(kanaal);
  String creditValue = String(credit);

  // String maken met keuze kanaal.
  String addClient = "19/05/20,15:59:46+00;ic;" + kanaalnummer + ";+000001;;;;907786589";
  String addCredit = "19/05/20,16:03:47+00;acc;" + kanaalnummer + ";;" + creditValue + ";;;805324419";

  KioskAddClient(addClient);
  delay(15000);
  Check_SMS();
  delay(2000);
  KioskAddCredit(addCredit);
  delay(15000);
  Check_SMS();
  delay(2000);
}


void PowerUsageSet (bool pinA, bool pinB, bool pinC, bool pinE) {

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Testen ...");
  lcd.setCursor(0,2);
  lcd.print("Stroomafname");

  Serial.println("stroom aan");
  digitalWrite(selectA,pinA);
  digitalWrite(selectB, pinB);
  digitalWrite(selectC, pinC);
  digitalWrite(enable, pinE);
  delay(20000);
}

void PowerUsageReset(bool pinA, bool pinB, bool pinC, bool pinE) {
  Serial.println("stroom uit");
  digitalWrite(selectA,pinA);
  digitalWrite(selectB, pinB);
  digitalWrite(selectC, pinC);
  digitalWrite(enable, pinE);
  delay(200);
}

void KioskAskStatus(String messageRCS, int kanaal ) {

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Testen ...");
  lcd.setCursor(0,2);
  lcd.print("Status opvragen" );

  char ConvertedMessageRCS[121] = "";
  
  messageRCS.toCharArray(ConvertedMessageRCS,121);

  if (gsm.IsRegistered()){
      gsm.SendSMS(TESTBOX, ConvertedMessageRCS);
      Serial.println("Message sent: ");
      Serial.print (ConvertedMessageRCS);
      Serial.print(" to: ");
      Serial.println(TESTBOX);
      delay(20000);
      Check_SMS();
      delay(2000);
  }
  String test = ",0,0,0,0,0,0,0,0,";
  Serial.println(test.charAt(2));
}


void KioskDeleteClient(int kanaal) {


  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Testen ...");
  lcd.setCursor(0,2);
  lcd.print("Klant verwijderen:" );

  
  if (gsm.IsRegistered()){

  char deleteClientArray[121] = "";
  String kanaalnummer = String(kanaal);
  String deleteClient = "19/05/20,16:3:47+00;dc;" + kanaalnummer +";;;;;448440965";

      deleteClient.toCharArray(deleteClientArray,121);

      gsm.SendSMS(TESTBOX, deleteClientArray);
      Serial.println("Message sent: ");
      Serial.print (deleteClientArray);
      Serial.print(" to: ");
      Serial.println(TESTBOX);
      delay(15000);
      Check_SMS();
      delay(2000);
  } 
}


/****************************************
lookup an Sms in Sim800 mem and process
****************************************/
void Check_SMS()  //Check if there is an sms 'type_sms'
{  Serial.println("check sms ");
  signed char pos_sms_rx = 1;  //Received SMS position 
  signed char result;    
  delay(1000);// delay nodig, anders wordt SMS niet geprocessed ??
  // check first SMS in Sim800 mem
  pos_sms_rx = gsm.IsSMSPresent(type_sms); // if present, result > 0
  if (pos_sms_rx > 0) { // SMS present
  
      Serial.println("check sms pos ");
      Serial.println(pos_sms_rx);
   
    //Read text/number/position of sms
    result = gsm.GetSMS(pos_sms_rx,number_incoming,sms_rx,120);

    // Serial.println(sms_rx);
    if (result == GETSMS_READ_SMS) {
      
        parseSms(sms_rx);
          //  Serial.print(SmsMessType);
           Serial.println("** SmsID ");
           Serial.println(SmsID);
           Serial.println(SmsMessType);
           Serial.println(SmsPortNum);
           Serial.println(SmsPortNum);
           Serial.println(SmsDummy1);
           Serial.println(SmsDummy2);
    }
    error = gsm.DeleteSMS(pos_sms_rx);
   
      if (error ==1){
        Serial.print("SMS deleted pos: ");      
        Serial.println(pos_sms_rx);

      }

      else{
        Serial.print("SMS not deleted pos: ");
        Serial.print(pos_sms_rx);
        Serial.print("\t");
        Serial.print("error: ");
        Serial.println(error);
      }

    Serial.println("yow");
    delay(1000);
  }
  Serial.println("woy");
}

/*********************************************
Separates all SMS fields and store invariables
convert Strings to correct variable types
**********************************************/ 
void parseSms(String _SmsText) {
 
  Serial.println(_SmsText); // for test
  
  int StartChar = 0;
  findNextField(_SmsText, StartChar).toCharArray(GatewayTime,21);
  SmsMessType = findNextField(_SmsText, StartChar);
  SmsPortNum = (findNextField(_SmsText, StartChar));
  findNextField(_SmsText, StartChar).toCharArray(SmsCustGsmNum,14);
  SmsCredits = StringToLong(findNextField(_SmsText, StartChar));
  SmsDummy1 = findNextField(_SmsText, StartChar);           //A* using these dummies for parsing of new SMS message type
  SmsDummy2 = findNextField(_SmsText, StartChar);
  SmsID = StringToLong(findNextField(_SmsText, StartChar));
}



/*************************
Subroutine for parsing Sms
**************************/ 
String findNextField(String S, int &b) {
  int e = S.indexOf(';',b);
  String R = S.substring(b, e);
  b= e +1;
  return R;
}


 /********************************************
 Converts credit string to signed credit long
*********************************************/
long StringToLong(String value) {
 long outLong = 0;
  long inLong = 1;
  int c = 0;
  int idx=value.length()-1;
  int sign = 1;
  for(int i=0;i<=idx;i++){
    c=(int)value[idx-i];
    if (c == 45) { // first digit is "-"
      sign =-1;
    }
    else {
      outLong+=inLong*(c-48);
      inLong*=10;
    }
    outLong =outLong*sign;
  }
  return outLong;
}
