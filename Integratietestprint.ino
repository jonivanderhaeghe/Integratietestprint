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
  char TESTBOX[20]= "+32474022239"; // nr testbox
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
int button = 30;

String CopySmsPortNum;
String rapportMeting [8];
bool smsFeedback;
int restart = 0;
int restartTimes = 3;
int buttonState = 0;

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
  pinMode(button, INPUT);

	lcd.begin(20, 4);
	SetBacklightOn();

  #ifndef V2
    pinMode(LCDLedPin, OUTPUT); // V4+
  #endif

  KioskGsmRegistration();

  // KioskFraude("19/06/04,16:36:6+00;rk;0;;;;;422083565", 0);
  KiosFraudekRoutine(0,5, false, false, false, true);
  KioskRoutine(1,20, true, false, false, true);
  // KioskCalculateCredit(0);
  // KioskCalculateCredit(1);
  // KioskCalculateCredit(2);
  // KioskCalculateCredit(3);
  // KioskCalculateCredit(4);
  // KioskCalculateCredit(5);
  // KioskCalculateCredit(6);
  // KioskCalculateCredit(7);
  // KioskRapport();
    
}  // end setup ***************************************

void KioskLCDTestprint() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Integratietestprint");
}


void KioskGsmRegistration() {


  KioskLCDTestprint();
	lcd.setCursor(0, 2);
  delay(200);
	lcd.print("Initializing GSM");

  gsm.TurnOn(9600);
  Serial.println("End GSM Reset, trying to connect");
  
  delay(7000);
	gsm.CheckRegistration();
  Serial.println("GSM Init Done");


  for (int i = 0; i < 2; i++)
  {
    if (gsm.IsRegistered())
    {
      Serial.println("GSM Registered");
      KioskPowerReset();
      smsFeedback = true;
      delay(2000);
      Check_SMS();
      delay(500);

      KioskLCDTestprint();
      delay(200);
	    lcd.setCursor(0, 2);
	    lcd.print("Connection OK");
      i++;
      delay(2000);

    }
    
    else if (i < 1)
    {
        Serial.println("GSM not registered, try again ...");
        KioskLCDTestprint();
        lcd.setCursor(0, 2);
        lcd.print("Connection pas OK");
        lcd.setCursor(0,3);
        lcd.print("verifier a nouveau");

        delay(500);
        gsm.TurnOn(9600);
        delay(7000);
        gsm.CheckRegistration();
        delay(500);
    }
    else
    {
      smsFeedback = false;
      KioskPowerReset();
      Serial.println("GSM not registered");

      KioskLCDTestprint();
      lcd.setCursor(0, 2);
      lcd.print("Connection pas OK");
      lcd.setCursor(0,3);
      lcd.print("controler l'antenne");
    }
  }
}


void KiosFraudekRoutine(int poort, int credit, bool A, bool B, bool C, bool E) {
  Serial.println(buttonState);
  do
  {
    buttonState = digitalRead(button);
  } while (buttonState == LOW);

  if (buttonState == HIGH) {
  Serial.println(buttonState);
    if (gsm.IsRegistered() && smsFeedback == true) {
  Serial.println(buttonState);

        KioskInit("19/05/20,15:30:32+00;ik;+228;;;;;775135380");
        KioskControlSmsFeedback("ikOK",poort);
          
        KioskKlantInit(poort,credit);

      if (gsm.IsRegistered() && smsFeedback == true) {
        KioskPowerSet(A, B, C, E);
      }

        delay(2000);
      if (gsm.IsRegistered() && smsFeedback == true) {
        KioskPowerReset();
      }

        KioskAskStatus("19/05/21,16:17:43+00;rcs;;;;;;942583047");
        KioskControlSmsFeedback("rcsOK",poort);

      if (gsm.IsRegistered() && smsFeedback == true) {
        KioskCalculateCredit(poort);
      }

      KioskFraude("19/06/04,15:58:12+00;rk;0;;;;;705920096", poort);

        KioskDeleteClient(poort);
    }
  }
}


void KioskRoutine(int poort, int credit, bool A, bool B, bool C, bool E) {

  if (buttonState == HIGH) {
  Serial.println(buttonState);
    if (gsm.IsRegistered() && smsFeedback == true) {
  Serial.println(buttonState);

        KioskInit("19/05/20,15:30:32+00;ik;+228;;;;;775135380");
        KioskControlSmsFeedback("ikOK",poort);
          
        KioskKlantInit(poort,credit);

      if (gsm.IsRegistered() && smsFeedback == true) {
        KioskPowerSet(A, B, C, E);
      }

        delay(2000);
      if (gsm.IsRegistered() && smsFeedback == true) {
        KioskPowerReset();
      }

        KioskAskStatus("19/05/21,16:17:43+00;rcs;;;;;;942583047");
        KioskControlSmsFeedback("rcsOK",poort);

      if (gsm.IsRegistered() && smsFeedback == true) {
        KioskCalculateCredit(poort);
      }

      KioskDeleteClient(poort);
    }
  }
}


void KioskCalculateCredit(int poort) { // poort begin bij 0 tot en met 7, door de index van een array


      int val[16];
      int x = 0;
      CopySmsPortNum;
      Serial.println(CopySmsPortNum);
      String doggy ("20;30;0;4;3;11;40;8;");
      for (int i = 0; i < 8; i++)
      {
        val[x] = doggy.toInt();

        if (val[x] < 10)
        {
          doggy.remove(0,2);
        }
        else if (val[x] > 10)
        {
          doggy.remove(0,3);
        }

        Serial.print(val[x]);
        x++;
      }

      String meting [3] = {"captage d'courant OK", "mauvaise calibration", "mauvaise cablage" };
      // charAt(index) haal een karakter uit een string met index
      
      
      if (val[poort] < 3)
      {
        // Serial.println(meting[1]);
        rapportMeting[poort] = meting[0];
      }
      else if (val[poort] < 25)
      {
        // Serial.println(meting[2]);
        rapportMeting[poort] = meting[1];
      }
      else if (val[poort] > 25)
      {
        // Serial.println(meting[3]);
        rapportMeting[poort] = meting[2];
      }
}


void KioskRapport () {

  int screenSelect=0;

  KioskLCDTestprint();
  while(1){

  buttonState = digitalRead(button);
  delay(2000);
  Serial.println(buttonState);
  if (buttonState == HIGH) {
    screenSelect++;
  }



    String rapportPoortnummer [8] = {"Port 1 : ", "Port 2 : ", "Port 3 : ", "Port 4 : ", "Port 5 : ", "Port 6 : ", "Port 7 : ", "Port 8 : "};
    Serial.println("Rapport ...");

    switch (screenSelect)
    {
    case 1:
      Serial.println(rapportPoortnummer[0] + rapportMeting[0] );
      lcd.setCursor(0,1);
      lcd.print("Rapport ...");
      lcd.setCursor(0,2);
      lcd.print(rapportPoortnummer[0]);
      lcd.setCursor(0,3);
      lcd.print(rapportMeting[0]);

      break;
    
    case 2:
      Serial.println(rapportPoortnummer[1] + rapportMeting[1] );
      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Rapport ...");
      lcd.setCursor(0,2);
      lcd.print(rapportPoortnummer[1]);
      lcd.setCursor(0,3);
      lcd.print(rapportMeting[1]);

      break;

    case 3:
      Serial.println(rapportPoortnummer[2] + rapportMeting[2] );
      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Rapport ...");
      lcd.setCursor(0,2);
      lcd.print(rapportPoortnummer[2]);
      lcd.setCursor(0,3);
      lcd.print(rapportMeting[2]);

      break; 

    case 4:
      Serial.println(rapportPoortnummer[3] + rapportMeting[3] );
      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Rapport ...");
      lcd.setCursor(0,2);
      lcd.print(rapportPoortnummer[3]);
      lcd.setCursor(0,3);
      lcd.print(rapportMeting[3]);

      break;

    case 5:
      Serial.println(rapportPoortnummer[4] + rapportMeting[4] );
      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Rapport ...");
      lcd.setCursor(0,2);
      lcd.print(rapportPoortnummer[4]);
      lcd.setCursor(0,3);
      lcd.print(rapportMeting[4]);

      break;

    case 6:
      Serial.println(rapportPoortnummer[5] + rapportMeting[5] );
      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Rapport ...");
      lcd.setCursor(0,2);
      lcd.print(rapportPoortnummer[5]);
      lcd.setCursor(0,3);
      lcd.print(rapportMeting[5]);

      break;

    case 7:
      Serial.println(rapportPoortnummer[6] + rapportMeting[6] );
      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Rapport ...");
      lcd.setCursor(0,2);
      lcd.print(rapportPoortnummer[6]);
      lcd.setCursor(0,3);
      lcd.print(rapportMeting[6]);

      break;

    case 8:
      Serial.println(rapportPoortnummer[7] + rapportMeting[7] );
      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Rapport ...");
      lcd.setCursor(0,2);
      lcd.print(rapportPoortnummer[7]);
      lcd.setCursor(0,3);
      lcd.print(rapportMeting[7]);

      break;
    }

    if (screenSelect == 8) {
      screenSelect = 0;
    }
  }
}


void KioskSmsFouten(int poort) {
  
  String poortnummer = String(poort+1);
  KioskLCDTestprint();

  Serial.println(SmsMessType);
  
    if (SmsMessType == "ikOK") {
      smsFeedback = false;
      Serial.println("Fout sms poort" + poortnummer + ": initialize kiosk sms");
      lcd.setCursor(0,2);
      lcd.print("Faux sms port" + poortnummer);
      lcd.setCursor(0,3);
      lcd.print(": initialiser kiosk");
    }

    else if (SmsMessType == "icOK") {
        smsFeedback = false;
        Serial.println("Fout sms poort" + poortnummer + ": klant toevoegen");
        lcd.setCursor(0,2);
        lcd.print("Faux sms port" + poortnummer);
        lcd.setCursor(0,3);
        lcd.print(": ajouter client");
    }
    else if (SmsMessType == "accOK") {
      smsFeedback = false;
      Serial.println("Fout sms poort " + poortnummer + ": credits toevoegen");
      lcd.setCursor(0,2);
      lcd.print("Faux sms port" + poortnummer);
      lcd.setCursor(0,3);
      lcd.print(": ajouter credits");
    }

    else if (SmsMessType == "rcsOK") {
      smsFeedback = false;
      Serial.println("Fout sms poort" + poortnummer + ": status");
      lcd.setCursor(0,2);
      lcd.print("Faux sms port" + poortnummer );
      lcd.setCursor(0,3);
      lcd.print(":situation de credit");
    }

    else if (SmsMessType == "dcOK") {
      smsFeedback = false;
      Serial.println("Fout sms poort" + poortnummer + ": delete klant");
      lcd.setCursor(0,2);
      lcd.print("Faux sms port" + poortnummer);
      lcd.setCursor(0,3);
      lcd.print(": demenager client");
    }

    else if (SmsMessType == "rkOK") {
      smsFeedback = false;
      Serial.println("Fout sms poort" + poortnummer + ": release kiosk");
      lcd.setCursor(0,2);
      lcd.print("Faux sms port" + poortnummer);
      lcd.setCursor(0,3);
      lcd.print(":relacher Box");
    }

    else
    {
      smsFeedback = false;
      Serial.println("Fout sms poort" + poortnummer + "feedback niet ontvangen");
      lcd.setCursor(0,2);
      lcd.print("Faux sms port" + poortnummer);
      lcd.setCursor(0,3);
      lcd.print(": pas de reaction");
    } 
}


void KioskControlSmsFeedback(String smsType, int poort) {

  if (gsm.IsRegistered() && smsFeedback == true) {
    Serial.println(SmsMessType);
    delay(2000);

    KioskLCDTestprint();
    for (int i = 0; i < 2; i++)
    {
      if (SmsMessType == smsType)
      {
        Serial.println("SMS " + smsType + " ontvangen");
        lcd.setCursor(0,2);
        lcd.print("sms " + smsType + " recu");
        smsFeedback = true;
        delay (5000);
      }
      
      else if (i < 1)
      {
        for (int i = 0; i < 1; i++)
        {
          Serial.println("SMS " + smsType + " mislukt, opnieuw proberen ...");
          KioskLCDTestprint();
          lcd.setCursor(0,2);
          lcd.print("SMS " + smsType + " ne recu pas");
          lcd.setCursor(0,3);
          lcd.print("essayer a nouveau");
          delay(10000);
          Check_SMS();
        }
      }
      else
      {
        Serial.println("SMS 'OK' ne recu pas");
        KioskSmsFouten(poort);
        delay(5000);
      }
    }
  } 
}


void KioskInit(String messageIK) {

  KioskLCDTestprint();
  char ConvertedMessageIK[121] = "";
  
  messageIK.toCharArray(ConvertedMessageIK,121);

  if (gsm.IsRegistered() && smsFeedback == true){

      lcd.setCursor(0,1);
      lcd.print("Teste ...");
      lcd.setCursor(0,2);
      lcd.print("initialiser");
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

      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Teste ...");
      lcd.setCursor(0,3);
      lcd.print("ajouter client" );

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

      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Teste ...");
      lcd.setCursor(0,3);
      lcd.print("ajouter credit" );

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
    KioskControlSmsFeedback("icOK", poort);
    KioskAddCredit(addCredit);
    delay(20000);
    Check_SMS();
    KioskControlSmsFeedback("accOK", poort);
  }
}


void KioskPowerSet (bool pinA, bool pinB, bool pinC, bool pinE) {
  
    KioskLCDTestprint();
    lcd.setCursor(0,1);
    lcd.print("Teste ...");
    lcd.setCursor(0,3);
    lcd.print("captage du courant");

    Serial.println("stroom aan");
    digitalWrite(selectA,pinA);
    digitalWrite(selectB, pinB);
    digitalWrite(selectC, pinC);
    digitalWrite(enable, pinE);
}


void KioskPowerReset() {

    KioskLCDTestprint();
    lcd.setCursor(0,2);
    lcd.print("remmettre a zero");
    lcd.setCursor(0,3);
    lcd.print("captage du courant");

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

      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Teste ...");
      lcd.setCursor(0,3);
      lcd.print("situation de credit");

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
      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Teste ...");
      lcd.setCursor(0,3);
      lcd.print("demenager client" );

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
      KioskControlSmsFeedback("dcOK", poort);
  } 
}


void KioskFraude(String messageRK, int poort) {

      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Teste ...");
      lcd.setCursor(0,3);
      lcd.print("Faire fraude" );
      
      do
  {
    buttonState = digitalRead(button);
  } while (buttonState == LOW);
    if (gsm.IsRegistered() && smsFeedback == true && buttonState == HIGH){
      KioskLCDTestprint();
      lcd.setCursor(0,1);
      lcd.print("Teste ...");
      lcd.setCursor(0,3);
      lcd.print("Fraude detection" );

      
      delay(2000);
      char ConvertedMessageRK[121] = "";
      messageRK.toCharArray(ConvertedMessageRK,121);
        delay(10000);
        Check_SMS();
        delay(4000);
        KioskControlSmsFeedback("VaultOpen", poort);

        KioskLCDTestprint();
        lcd.setCursor(0,1);
        lcd.print("Teste ...");
        lcd.setCursor(0,3);
        lcd.print("relacher SolergieBox");

        gsm.SendSMS(TESTBOX, ConvertedMessageRK);
        delay(2000);
        Serial.println("Message sent: ");
        Serial.print (ConvertedMessageRK);
        Serial.print(" to: ");
        Serial.println(TESTBOX);

        delay(10000);
        Check_SMS();
        delay(500);
        KioskControlSmsFeedback("rkOK", poort);
        smsFeedback = true;

        KioskLCDTestprint();
        lcd.setCursor(0,1);
        lcd.print("Teste ...");
        lcd.setCursor(0,3);
        lcd.print("fraude test OK");
        delay(5000);
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
  CopySmsPortNum = SmsPortNum;
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