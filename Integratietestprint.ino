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


GSM gsm;

// GSM variables
  char TESTBOX[20]= "+32491736777"; // nr testbox
// +32474022239
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

String jowbro;
String rapportMeting [8];
bool smsFeedback;

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

	lcd.begin(20, 4);
	SetBacklightOn();

  #ifndef V2
    pinMode(LCDLedPin, OUTPUT); // V4+
  #endif

  KioskGsmRegistration();
  KioskRoutine(0,50, false, false, false, true);
  // KioskPowerUsageSet(true,false,false,true);

    
}  // end setup ***************************************

void lcdStart() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Integratietestprint");
}


void KioskGsmRegistration() {


  lcdStart();
	lcd.setCursor(0, 2);
  delay(200);
	lcd.print("Initializing GSM");

  gsm.TurnOn(9600);	//module reset power on; factory reset, disable echo
  Serial.println("End GSM Reset, trying to connect");
  
  delay(7000); // wait 7 seconds, then see if registration ok
	//GetRegistered();
	gsm.CheckRegistration(); // updates status gsm.IsRegistered(), does InitParam(PARAM_SET_1) if needed
  Serial.println("GSM Init Done");


  for (int i = 0; i < 2; i++)
  {
    if (gsm.IsRegistered())
    {
      Serial.println("GSM is registered");
      Serial.println(gsm.IsRegistered());
      lcdStart();
      delay(200);
	    lcd.setCursor(0, 2);
	    lcd.print("Connection OK");

      smsFeedback = true;
      Check_SMS();
      delay(500);
      i++;
      KioskPowerUsageReset();
    }
    
    else if (i <1)
    {
        Serial.println("GSM not registerd, opnieuw proberen ...");
        lcdStart();
        lcd.setCursor(0, 2);
        lcd.print("Connection not OK");
        lcd.setCursor(0,3);
        lcd.print("check again");

        Serial.println(gsm.IsRegistered());
        delay(500);
        gsm.TurnOn(9600);
        delay(7000);
        gsm.CheckRegistration();
        delay(500);
    }
    else
    {
      Serial.println("GSM not registerd");
      lcdStart();
      delay(200);
      lcd.setCursor(0, 2);
      lcd.print("Connection not OK");
      lcd.setCursor(0,3);
      lcd.print("check antenne");
      delay(500);

      smsFeedback = false;
      KioskPowerUsageReset();
      
    }
  }
}

void KioskRoutine(int poort, int credit, bool A, bool B, bool C, bool E) {

  if (gsm.IsRegistered() && smsFeedback == true) {

      KioskInit("19/05/20,15:30:32+00;ik;+228;;;;;775135380");
      KioskControlSmsFeedback("ikOK");
        
      KioskKlantInit(poort,credit);

    if (gsm.IsRegistered() && smsFeedback == true) {
      KioskPowerUsageSet(A, B, C, E);
    }

      delay(2000);
    if (gsm.IsRegistered() && smsFeedback == true) {
      KioskPowerUsageReset();
    }

      KioskAskStatus("19/05/21,16:17:43+00;rcs;;;;;;942583047");
      KioskControlSmsFeedback("rcsOK");

    if (gsm.IsRegistered() && smsFeedback == true) {
      KioskCalculateCredit(poort);
    }

      KioskDeleteClient(poort);
    
    if (gsm.IsRegistered() && smsFeedback == true) {
      KioskRapport();
    }
  }
}

void KioskCalculateCredit(int poort) { // poort begin bij 0 tot en met 7, door de index van een array


      int val[16];
      int x = 0;
      // String hond = "17,23,80,40,0,0,25,25,";
      // hond.toInt();
      // Serial.println(hond.toInt());
      jowbro;
      Serial.println(jowbro);
      for (int i = 0; i < 8; i++)
      {
        val[x] = jowbro.toInt();

        if (val[x] < 10)
        {
          jowbro.remove(0,2);
        }
        else if (val[x] > 10)
        {
          jowbro.remove(0,3);
        }

        // Serial.println(hond);
        Serial.print(val[x]);
        x++;
      }

      String meting [4] = {"Stroomafname perfect", "Stroomafname binnen de marge", "slechte callibratie", "slechte bekabeling" };
      // charAt(index) haal een karakter uit een string met index
      
      if (val[poort] == 0) {
        // Serial.println(meting[0]);
        rapportMeting[poort] = meting[0];
      }
      
      else if (val[poort] < 3)
      {
        // Serial.println(meting[1]);
        rapportMeting[poort] = meting[1];
      }
      else if (val[poort] < 25)
      {
        // Serial.println(meting[2]);
        rapportMeting[poort] = meting[2];
      }
      else if (val[poort] > 25)
      {
        // Serial.println(meting[3]);
        rapportMeting[poort] = meting[3];
      }
}

void KioskRapport () {

    lcdStart();

    String rapportPoortnummer [8] = {"Poort 1 : ", "Poort 2 : ", "Poort 3 : ", "Poort 4 : ", "Poort 5 : ", "Poort 6 : ", "Poort 7 : ", "Poort 8 : "};
    Serial.println("Rapport ...");
    Serial.println(rapportPoortnummer[0] + rapportMeting[0] );
    Serial.println(rapportPoortnummer[1] + rapportMeting[1] );
    Serial.println(rapportPoortnummer[2] + rapportMeting[2] );
    Serial.println(rapportPoortnummer[3] + rapportMeting[3] );
    Serial.println(rapportPoortnummer[4] + rapportMeting[4] );
    Serial.println(rapportPoortnummer[5] + rapportMeting[5] );
    Serial.println(rapportPoortnummer[6] + rapportMeting[6] );
    Serial.println(rapportPoortnummer[7] + rapportMeting[7] );

    lcd.setCursor(0,2);
    lcd.print("Rapport ...");
    lcd.setCursor(0,3);
    lcd.print(rapportPoortnummer[0] + rapportMeting[0] );
}

void KioskControlSmsFeedback(String smsType) {

  if (gsm.IsRegistered() && smsFeedback == true) {
    lcdStart();
    for (int i = 0; i < 2; i++)
    {
      if (SmsMessType == smsType)
      {
        Serial.println("SMS " + smsType + " ontvangen");
        lcd.setCursor(0,2);
        lcd.print("sms" + smsType + "ontvangen");
        smsFeedback = true;
        delay (2000);
      }
      
      else if (i < 1)
      {
        for (int i = 0; i < 1; i++)
        {
          Serial.println("SMS " + smsType + " mislukt, opnieuw proberen ...");
          lcdStart();
          lcd.setCursor(0,2);
          lcd.print("SMS " + smsType + " mislukt");
          lcd.setCursor(0,3);
          lcd.print("opnieuw proberen ...");
          delay(10000);
          Check_SMS();
        }
      }
      else
      {
        Serial.println("SMS 'OK' mislukt");
        smsFeedback = false;
        Serial.println("Fout met sms " + smsType);
        lcdStart();
        lcd.setCursor(0,2);
        lcd.print("Fout met sms " + smsType);
        delay(2000);
        
      }
    }
  }
}

void KioskInit(String messageIK) {


  char ConvertedMessageIK[121] = "";
  
  messageIK.toCharArray(ConvertedMessageIK,121);

  if (gsm.IsRegistered() && smsFeedback == true){

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Testen ...");
      lcd.setCursor(0,2);
      lcd.print("Initialize");
      lcd.setCursor(0,3);
      lcd.print("SolergieBox");

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


  char ConvertedMessageIC[121] = "";
  messageIC.toCharArray(ConvertedMessageIC,121);
  if (gsm.IsRegistered() && smsFeedback == true){

      lcdStart();
      lcd.setCursor(0,1);
      lcd.print("Testen ...");
      lcd.setCursor(0,3);
      lcd.print("Klant toevoegen" );

      gsm.SendSMS(TESTBOX, ConvertedMessageIC);
      delay(2000);
      Serial.println("Message sent: ");
      Serial.print (ConvertedMessageIC);
      Serial.print(" to: ");
      Serial.println(TESTBOX);
  } 
}


void KioskAddCredit(String messageACC) {


  char ConvertedMessageACC[121] = "";
  
  messageACC.toCharArray(ConvertedMessageACC,121);

  if (gsm.IsRegistered() && smsFeedback == true){

      lcdStart();
      lcd.setCursor(0,1);
      lcd.print("Testen ...");
      lcd.setCursor(0,3);
      lcd.print("Credits toevoegen" );

      gsm.SendSMS(TESTBOX, ConvertedMessageACC);
      Serial.println("Message sent: ");
      Serial.print (ConvertedMessageACC);
      Serial.print(" to: ");
      Serial.println(TESTBOX);
  } 
}


void KioskKlantInit (int poort, int credit) {

  // int omzetten naar string met nieuwe variabele naam kanaalnumber en valueCredit
  String poortnummer = String(poort+1);
  String creditValue = String(credit);

  // String maken met keuze kanaal.
  String addClient = "19/05/20,15:59:46+00;ic;" + poortnummer + ";+000001;;;;907786589";
  String addCredit = "19/05/20,16:03:47+00;acc;" + poortnummer + ";;" + creditValue + ";;;805324419";

  if (gsm.IsRegistered() && smsFeedback == true) {
    KioskAddClient(addClient);
    delay(20000);
    Check_SMS();
    KioskControlSmsFeedback("icOK");
    KioskAddCredit(addCredit);
    delay(20000);
    Check_SMS();
    KioskControlSmsFeedback("accOK");
  }
}


void KioskPowerUsageSet (bool pinA, bool pinB, bool pinC, bool pinE) {
  
    lcdStart();
    lcd.setCursor(0,1);
    lcd.print("Testen ...");
    lcd.setCursor(0,3);
    lcd.print("Stroomafname");

    Serial.println("stroom aan");
    digitalWrite(selectA,pinA);
    digitalWrite(selectB, pinB);
    digitalWrite(selectC, pinC);
    digitalWrite(enable, pinE);
}


void KioskPowerUsageReset() {

    lcdStart();
    lcd.setCursor(0,2);
    lcd.print("reset Stroomafname");

    Serial.println("stroom uit");
    digitalWrite(selectA, LOW);
    digitalWrite(selectB, LOW);
    digitalWrite(selectC, LOW);
    digitalWrite(enable, LOW);
    delay(200);
}

void KioskAskStatus(String messageRCS) {


  char ConvertedMessageRCS[121] = "";
  
  messageRCS.toCharArray(ConvertedMessageRCS,121);

  if (gsm.IsRegistered() && smsFeedback == true){

      lcdStart();
      lcd.setCursor(0,1);
      lcd.print("Testen ...");
      lcd.setCursor(0,3);
      lcd.print("Status opvragen" );

      gsm.SendSMS(TESTBOX, ConvertedMessageRCS);
      Serial.println("Message sent: ");
      Serial.print (ConvertedMessageRCS);
      Serial.print(" to: ");
      Serial.println(TESTBOX);
      delay(20000);
      Check_SMS();
      delay(2000);
  }
}


void KioskDeleteClient(int poort) {

  if (gsm.IsRegistered() && smsFeedback == true){
      lcdStart();
      lcd.setCursor(0,1);
      lcd.print("Testen ...");
      lcd.setCursor(0,3);
      lcd.print("Klant verwijderen:" );

      char deleteClientArray[121] = "";
      String poortnummer = String(poort+1);
      String deleteClient = "19/05/20,16:3:47+00;dc;" + poortnummer +";;;;;448440965";

      deleteClient.toCharArray(deleteClientArray,121);

      gsm.SendSMS(TESTBOX, deleteClientArray);
      Serial.println("Message sent: ");
      Serial.print (deleteClientArray);
      Serial.print(" to: ");
      Serial.println(TESTBOX);
      delay(15000);
      Check_SMS();
      KioskControlSmsFeedback("dcOK");
  } 
}






/****************************************
lookup an Sms in Sim800 mem and process
****************************************/
void Check_SMS()  //Check if there is an sms 'type_sms'
{  
  Serial.println("check sms ");
  signed char pos_sms_rx = 1;  //Received SMS position 
  signed char result;    
  delay(1000);// delay nodig, anders wordt SMS niet geprocessed ??
  // check first SMS in Sim800 mem
  pos_sms_rx = gsm.IsSMSPresent(type_sms); // if present, result > 0
  if (pos_sms_rx > 0) { // SMS present
  
      Serial.println("check sms pos :");
      Serial.print(pos_sms_rx);
   
    //Read text/number/position of sms
    result = gsm.GetSMS(pos_sms_rx,number_incoming,sms_rx,120);

    // Serial.println(sms_rx);
        
    if (result == GETSMS_READ_SMS) {
      
        parseSms(sms_rx);
        delay(2000);
          //  Serial.print(SmsMessType);
           Serial.println("SmsID : ");
           Serial.println(SmsID);
          //  Serial.println(SmsMessType);
          //  Serial.println(SmsPortNum);
          //  Serial.println(SmsPortNum);
          //  Serial.println(SmsDummy1);
          //  Serial.println(sms_rx);
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

    delay(1000);
  }
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
  jowbro = SmsPortNum;
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