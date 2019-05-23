/*
 GSM_2560.cpp - library for Smartswitch.
 
 Fernand Verstraete 18-4-2016
 
 Based on library for the GSM Playground - GSM Shield for Arduino
 Released under the Creative Commons Attribution-Share Alike 3.0 License
 http://www.creativecommons.org/licenses/by-sa/3.0/
 www.hwkitchen.com
 
#####################################################################
CHANGE LOG

1-12-2016
	added comment
	deleted obsolete lines
11-5-2017
	GSM::pulse deleted, now in turnon
	included pins definition in pin_def_ext.h
20-3-2018
	reset WDT before long waits to avoid restasrt program
	re-include At echo reset i param set 0
13/4
	wdt reset at start of rxfinished covers all delays
	
#####################################################################
*/  
#include "Arduino.h"
#include "pin_def_ext.h"
#include "GSM_2560.h"
#include <avr/wdt.h>

extern "C" {
  #include <string.h>
}

#ifdef DEBUG_LED_ENABLED
  int DEBUG_LED = 25;                // LED connected to digital pin 25

void  GSM::BlinkDebugLED (byte num_of_blink)
  {
    byte i;

    pinMode(DEBUG_LED, OUTPUT);      // sets the digital pin as output
    for (i = 0; i < num_of_blink; i++) {
      digitalWrite(DEBUG_LED, HIGH);   // sets the LED on
      delay(50);
      digitalWrite(DEBUG_LED, LOW);   // sets the LED off
      delay(500);
    }
  }
#endif

#ifdef DEBUG_PRINT
/**********************************************************
Two methods print out debug information to the standard output
- it means to the serial line.
First method prints string.
Second method prints integer numbers.

Note:
=====
The serial line is connected to the GSM module and is 
used for sending AT commands. There is used "trick" that GSM
module accepts not valid AT command strings because it doesn't
understand them and still waits for some valid AT command.
So after all debug strings are sent we send just AT<CR> as
a valid AT command and GSM module responds by OK. So previous 
debug strings are overwritten and GSM module is not influenced
by these debug texts 



string_to_print:  pointer to the string to be print out
last_debug_print: 0 - this is not last debug info, we will
                      continue with sending... so don't send
                      AT<CR>(see explanation above)
                  1 - we are finished with sending debug info 
                      for this time and finished AT<CR> 
                      will be sent(see explanation above)

**********************************************************/
void GSM::DebugPrint(const char *string_to_print, byte last_debug_print)
{
  if (last_debug_print) {
    Serial.println(string_to_print);
    SendATCmdWaitResp("AT", 500, 50, "OK", 1);
  }
  else Serial.print(string_to_print);
}

void GSM::DebugPrint(int number_to_print, byte last_debug_print)
{
  Serial.println(number_to_print);
  if (last_debug_print) {
    SendATCmdWaitResp("AT", 500, 50, "OK", 1);
  }
}
#endif

/**********************************************************
Method returns GSM library version

return val: 100 means library version 1.00
            101 means library version 1.01
**********************************************************/
int GSM::LibVer(void)
{
  return (GSM_LIB_VERSION);
}

/**********************************************************
  Constructor definition
***********************************************************/

GSM::GSM(void)
{
  
	// not registered yet
	module_status = STATUS_NONE;
	pinMode(GSM_DTR, OUTPUT); // sets normal mode
	pinMode(GSM_RESET, OUTPUT); 
	// set pullup on int line
	digitalWrite(GSM_RNG,HIGH);



}

/**********************************************************
  Initializes receiving process

  start_comm_tmout    - maximum waiting time for receiving the first response
                        character (in msec.)
  max_interchar_tmout - maximum tmout between incoming characters 
                        in msec.
  if there is no other incoming character longer then specified
  tmout(in msec) receiving process is considered as finished
**********************************************************/
void GSM::RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
  rx_state = RX_NOT_STARTED;
  start_reception_tmout = start_comm_tmout;
  interchar_tmout = max_interchar_tmout;
  prev_time = millis();
  comm_buf[0] = 0x00; // end of string
  p_comm_buf = &comm_buf[0];
  comm_buf_len = 0;
  SIM800Flush(); // erase rx circular buffer
}

/**********************************************************
Method checks if receiving process is finished or not.
Rx process is finished if defined inter-character tmout is reached

returns:
        RX_NOT_FINISHED = 0,// not finished yet
        RX_FINISHED,        // finished - inter-character tmout occurred
        RX_TMOUT_ERR,       // initial communication tmout occurred
**********************************************************/
byte GSM::IsRxFinished(void)
{
  byte num_of_bytes;
  byte ret_val = RX_NOT_FINISHED;  // default not finished

  // Rx state machine
  // ----------------

  wdt_reset(); // **reset Watchdog timer** to avoid restart program
  
  if (rx_state == RX_NOT_STARTED) {
    // Reception is not started yet - check tmout
    if (!SIM800.available()) {
      // still no character received => check timeout
	 
	/* #ifdef DEBUG_GSMRX

		
			DebugPrint("\r\nDEBUG: reception timeout", 0);			
			Serial.print((unsigned long)(millis() - prev_time));	
			DebugPrint("\r\nDEBUG: start_reception_tmout\r\n", 0);			
			Serial.print(start_reception_tmout);	
			
		
	#endif */

      if ((unsigned long)(millis() - prev_time) >= start_reception_tmout) {
        // timeout elapsed => GSM module didn't start with response
        // so communication is takes as finished
/* 		
			#ifdef DEBUG_GSMRX		
				DebugPrint("\r\nDEBUG: RECEPTION TIMEOUT", 0);	
			#endif
		 */
        comm_buf[comm_buf_len] = 0x00;
        ret_val = RX_TMOUT_ERR;
      }
    }
    else {
      // at least one character received => so init inter-character 
      // counting process again and go to the next state
      prev_time = millis(); // init tmout for inter-character space
      rx_state = RX_ALREADY_STARTED;
    }
  }

  if (rx_state == RX_ALREADY_STARTED) {
    // Reception already started
    // check new received bytes
    // only in case we have place in the buffer
    num_of_bytes = SIM800.available();
    // if there are some received bytes postpone the timeout
    if (num_of_bytes) prev_time = millis();
      
    // read all received bytes      
    while (num_of_bytes) {
      num_of_bytes--;
      if (comm_buf_len < COMM_BUF_LEN) {
        // we have still place in the GSM internal comm. buffer =>
        // move available bytes from circular buffer 
        // to the rx buffer
        *p_comm_buf = SIM800.read();

        p_comm_buf++;
        comm_buf_len++;
        comm_buf[comm_buf_len] = 0x00;  // and finish currently received characters
                                        // so after each character we have
                                        // valid string finished by the 0x00
      }
      else {
        // comm buffer is full, other incoming characters
        // will be discarded 
        // but despite of we have no place for other characters 
        // we still must to wait until  
        // inter-character tmout is reached
        
        // so just readout character from circular RS232 buffer 
        // to find out when communication id finished(no more characters
        // are received in inter-char timeout)

        SIM800.read();
      }
    }

    // finally check the inter-character timeout 
	/*
	#ifdef DEBUG_GSMRX
		
			DebugPrint("\r\nDEBUG: intercharacter", 0);			
			Serial.print((unsigned long)(millis() - prev_time));	
			DebugPrint("\r\nDEBUG: interchar_tmout\r\n", 0);			
			Serial.print(interchar_tmout);	
			
		
	#endif
	*/
    if ((unsigned long)(millis() - prev_time) >= interchar_tmout) {
      // timeout between received character was reached
      // reception is finished
      // ---------------------------------------------
	  
		/* 
	  	#ifdef DEBUG_GSMRX
		
			DebugPrint("\r\nDEBUG: OVER INTER TIMEOUT", 0);					
		#endif
		 */
      comm_buf[comm_buf_len] = 0x00;  // for sure finish string again
                                      // but it is not necessary
      ret_val = RX_FINISHED;
    }
  }

	/* 	
  	#ifdef DEBUG_GSMRX
		if (ret_val == RX_FINISHED){
			DebugPrint("DEBUG: Received string\r\n", 0);
			for (int i=0; i<comm_buf_len; i++){
				Serial.print(byte(comm_buf[i]));	
			}
		}
	#endif */
	
  return (ret_val);
}

/**********************************************************
Method checks received bytes

compare_string - pointer to the string which should be find

return: 0 - string was NOT received
        1 - string was received
**********************************************************/
byte GSM::IsStringReceived(char const *compare_string)
{
  char *ch;
  byte ret_val = 0;

  if(comm_buf_len) {
  /*
		#ifdef DEBUG_GSMRX
			DebugPrint("DEBUG: Compare the string: \r\n", 0);
			for (int i=0; i<comm_buf_len; i++){
				Serial.print(byte(comm_buf[i]));	
			}
			
			DebugPrint("\r\nDEBUG: with the string: \r\n", 0);
			Serial.print(compare_string);	
			DebugPrint("\r\n", 0);
		#endif
	*/
//Serial.println((char *)comm_buf); //FV
    ch = strstr((char *)comm_buf, compare_string);
    if (ch != NULL) {
      ret_val = 1;
	  /*#ifdef DEBUG_PRINT
		DebugPrint("\r\nDEBUG: expected string was received\r\n", 0);
	  #endif
	  */
    }
	else
	{
	  /*#ifdef DEBUG_PRINT
		DebugPrint("\r\nDEBUG: expected string was NOT received\r\n", 0);
	  #endif
	  */
	}
  }

  return (ret_val);
}

/**********************************************************
Method waits for response

      start_comm_tmout    - maximum waiting time for receiving the first response
                            character (in msec.)
      max_interchar_tmout - maximum tmout between incoming characters 
                            in msec.  
return: 
      RX_FINISHED         finished, some character was received

      RX_TMOUT_ERR        finished, no character received 
                          initial communication tmout occurred
**********************************************************/
byte GSM::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
  byte status;

  RxInit(start_comm_tmout, max_interchar_tmout); 
  // wait until response is not finished
  do {
	// max wait time for start response is 7 seconds, for  next char is 5 sec
	status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);
  return (status);
}


/**********************************************************
Method waits for response with specific response string
    
      start_comm_tmout    - maximum waiting time for receiving the first response
                            character (in msec.)
      max_interchar_tmout - maximum tmout between incoming characters 
                            in msec.  
      expected_resp_string - expected string
return: 
      RX_FINISHED_STR_RECV,     finished and expected string received
      RX_FINISHED_STR_NOT_RECV  finished, but expected string not received
      RX_TMOUT_ERR              finished, no character received 
                                initial communication tmout occurred
**********************************************************/
byte GSM::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, 
                   char const *expected_resp_string)
{
  byte status;
  byte ret_val;

  RxInit(start_comm_tmout, max_interchar_tmout); 
  // wait until response is not finished
  do {
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // ---------------------------------------------
	
    if(IsStringReceived(expected_resp_string)) {
      // expected string was received
      // ----------------------------
      ret_val = RX_FINISHED_STR_RECV; 
//Serial.println(expected_resp_string); // FV	  
    }
    else ret_val = RX_FINISHED_STR_NOT_RECV;
  }
  else {
    // nothing was received
    // --------------------
    ret_val = RX_TMOUT_ERR;
  }
  return (ret_val);
}


/**********************************************************
Method sends AT command and waits for response

return: 
      AT_RESP_ERR_NO_RESP = -1,   // no response received
      AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
      AT_RESP_OK = 1,             // response_string was included in the response
**********************************************************/
char GSM::SendATCmdWaitResp(char const *AT_cmd_string,
                uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                char const *response_string,
                byte no_of_attempts)
{
  byte status;
  char ret_val = AT_RESP_ERR_NO_RESP;
  byte i;

  for (i = 0; i < no_of_attempts; i++) {
    // delay 500 msec. before sending next repeated AT command 
    // so if we have no_of_attempts=1 tmout will not occurred
    if (i > 0) delay(500); 
//   Serial.println(AT_cmd_string);  //FV

//onnodig, FV	wdt_reset(); // **reset Watchdog timer** to avoid restart program
	
   SIM800.println(AT_cmd_string);
    status = WaitResp(start_comm_tmout, max_interchar_tmout); 
    if (status == RX_FINISHED) {
      // something was received but what was received?
      // ---------------------------------------------
// Serial.println(response_string);	// FV  
     if(IsStringReceived(response_string)) {
        ret_val = AT_RESP_OK;      
        break;  // response is OK => finish
      }
      else ret_val = AT_RESP_ERR_DIF_RESP;
    }
    else {
      // nothing was received
      // --------------------
      ret_val = AT_RESP_ERR_NO_RESP;
    }
    
  }


  return (ret_val);
}


/**********************************************************
Methods return the state of corresponding
bits in the status variable

- these methods do not communicate with the GSM module

return values: 
      0 - not true (not active)
      >0 - true (active)
**********************************************************/
byte GSM::IsRegistered(void)
{
  return (module_status & STATUS_REGISTERED);
}

byte GSM::IsInitialized(void)
{
  return (module_status & STATUS_INITIALIZED);
}

/**********************************************************
  Turn GSM module on
  Checks if the GSM module is responding to the AT command

  - if YES nothing is made 
  - if NO GSM module is turned on again
**********************************************************/
void GSM::TurnOn(long baud_rate)
{
  SetCommLineStatus(CLS_ATCMD);
  SIM800.begin(baud_rate); // overbodig?
  
/*   #ifdef DEBUG_PRINT
    // parameter 0 - because module is off so it is not necessary 
    // to send finish AT<CR> here
    DebugPrint("DEBUG: baud ", 0);
	DebugPrint(baud_rate, 0);
#endif */
  
  if (AT_RESP_OK != SendATCmdWaitResp("AT", 500, 100, "OK", 5)) {		//check correct respose, if not ok, turn the module on again

  
/* 		#ifdef DEBUG_PRINT
			// parameter 0 - because module is off so it is not necessary 
			// to send finish AT<CR> here
			DebugPrint("DEBUG: GSM module is off\r\n", 0);
			DebugPrint("DEBUG: start the module\r\n", 0);
		#endif */
		
		// generate turn on or reset pulse
		wdt_reset(); // **reset Watchdog timer** to avoid restart program
		digitalWrite(GSM_RESET, HIGH);
		delay(1200);
		digitalWrite(GSM_RESET, LOW);
		wdt_reset(); // **reset Watchdog timer** to avoid restart program
		delay(5000);

	}

	else
	{
		#ifdef DEBUG_PRINT
			DebugPrint("DEBUG: 2 GSM module is on and baud is ok\r\n", 0);
		#endif
  
	}
  SetCommLineStatus(CLS_FREE);

  // send collection of first initialization parameters for the GSM module    
  InitParam(PARAM_SET_0);
}



/**********************************************************
  Sends parameters for initialization of GSM module

  group:  0 - parameters of group 0 - not necessary to be registered in the GSM
          1 - parameters of group 1 - it is necessary to be registered
**********************************************************/
void GSM::InitParam(byte group)
{

//Serial.println(group); //FV
  switch (group) {
    case PARAM_SET_0:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;

        Serial.println("PARAM_SET_0");
	  	#ifdef DEBUG_PRINT
			DebugPrint("DEBUG: configure the module PARAM_SET_0\r\n", 0);
		#endif
      SetCommLineStatus(CLS_ATCMD);

      // Reset to the factory settings
      SendATCmdWaitResp("AT&F", 1000, 50, "OK", 5);      
      // switch off echo
      SendATCmdWaitResp("ATE0", 500, 50, "OK", 5);

      SetCommLineStatus(CLS_FREE);
      break;

    case PARAM_SET_1:
      // check comm line
      if (CLS_FREE != GetCommLineStatus()) return;
	     Serial.println("PARAM_SET_1");
	  	#ifdef DEBUG_PRINT
			DebugPrint("DEBUG: configure the module PARAM_SET_1\r\n", 0);
		#endif
      SetCommLineStatus(CLS_ATCMD);
      // set the SMS mode to text 
      SendATCmdWaitResp("AT+CMGF=1", 500, 50, "OK", 5);

      InitSMSMemory();
      // select phonebook memory storage
      SendATCmdWaitResp("AT+CPBS=\"SM\"", 1000, 50, "OK", 5);
	  
	  // enable entering sleep mode
      SendATCmdWaitResp("AT+CSCLK=1", 1000, 50, "OK", 5);

      SetCommLineStatus(CLS_FREE);
      // InitSMSMemory(); // FV 12/11; double, deleted FV 25/1
      break;
  }
	wdt_reset(); // **reset Watchdog timer** to avoid restart program
}


/**********************************************************
Method checks if the GSM module is registered in the GSM net
- this method communicates directly with the GSM module
  in contrast to the method IsRegistered() which reads the
  flag from the module_status (this flag is set inside this method)

- must be called regularly - from 1sec. to cca. 10 sec.

return values: 
      REG_NOT_REGISTERED  - not registered
      REG_REGISTERED      - GSM module is registered
      REG_NO_RESPONSE     - GSM doesn't response
      REG_COMM_LINE_BUSY  - comm line between GSM module and Arduino is not free
                            for communication
**********************************************************/
byte GSM::CheckRegistration(void)
{Serial.println("CheckRegistration in cpp");
  byte status;
  byte ret_val = REG_NOT_REGISTERED;

  if (CLS_FREE != GetCommLineStatus()) return (REG_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
//onnodig, FV  wdt_reset(); // **reset Watchdog timer** to avoid restart program
  SIM800.println("AT+CREG?"); 
  // 5 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  status = WaitResp(5000, 50); 

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // ---------------------------------------------
    if(IsStringReceived("+CREG: 0,1") 
      || IsStringReceived("+CREG: 0,5")) {
      // it means module is registered in local network or roaming

      // ------------------------------------------------------
      module_status |= STATUS_REGISTERED;
    
    
      // in case GSM module is registered first time after reset
      // sets flag STATUS_INITIALIZED
      // it is used for sending some init commands which 
      // must be sent only after registration
      // --------------------------------------------
      if (!IsInitialized()) { Serial.println("sending init commands");
        module_status |= STATUS_INITIALIZED;
        SetCommLineStatus(CLS_FREE);
        InitParam(PARAM_SET_1);
      }
      ret_val = REG_REGISTERED;      
    }
    else {
      // NOT registered
      // --------------
      module_status &= ~STATUS_REGISTERED;
      ret_val = REG_NOT_REGISTERED;
    }
  }
  else {
    // nothing was received
    // --------------------
    ret_val = REG_NO_RESPONSE;
  }
  SetCommLineStatus(CLS_FREE);
	
  return (ret_val);
}

/**********************************************************

Method sends SMS

number_str:   pointer to the phone number string
message_str:  pointer to the SMS text string


return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0 - SMS was not sent
        1 - SMS was sent


an example of usage:
        GSM gsm;
        gsm.SendSMS("00XXXYYYYYYYYY", "SMS text");
**********************************************************/
byte GSM::SendSMS(char *number_str, char *message_str) 
{
	byte ret_val = -1;
	byte i;

	if (CLS_FREE != GetCommLineStatus()) return (ret_val);
	SetCommLineStatus(CLS_ATCMD);  
	ret_val = 0; // still not send
	// try to send SMS 3 times in case there is some problem
	for (i = 0; i < 3; i++) {
		// send  AT+CMGS="number_str"
		SIM800.print("AT+CMGS=\"");
		SIM800.print(number_str);  
		SIM800.print("\"\r");
//onnodig, FV		wdt_reset(); // **reset Watchdog timer** to avoid restart program
		// 1000 msec. for initial comm tmout
		// 50 msec. for inter character timeout
		if (RX_FINISHED_STR_RECV == WaitResp(1000, 50, ">")) {
			// send SMS text
			SIM800.println(message_str); 

			SIM800.write(0x1a); // escape
//onnodig, FV			wdt_reset(); // **reset Watchdog timer** to avoid restart program
			
			//SIM800.flush(); // erase rx circular buffer 
			if (RX_FINISHED_STR_RECV == WaitResp(7000, 5000, "+CMGS")) {
				// SMS was send correctly 
				ret_val = 1;
				//Serial.println("SMS was send correctly \r\n");
				break;
			}
			else {
				//Serial.println("no CMGS");
//				Serial.println(RX_FINISHED_STR_RECV); //vr test FV
				continue;
			}
		} 
		else {
		// try again
			//Serial.println("no >");
			continue;
		}
	}

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method sends SMS to the specified SIM phonebook position

sim_phonebook_position:   SIM phonebook position <1..20>
message_str:              pointer to the SMS text string


return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - specified position must be > 0

        OK ret val:
        -----------
        0 - SMS was not sent
        1 - SMS was sent


an example of usage:
        GSM gsm;
        gsm.SendSMS(1, "SMS text");
**********************************************************/
char GSM::SendSMS(byte sim_phonebook_position, char *message_str) 
{
  char ret_val = -1;
  char sim_phone_number[20];

  ret_val = 0; // SMS is not send yet
  if (sim_phonebook_position == 0) return (-3);
  if (1 == GetPhoneNumber(sim_phonebook_position, sim_phone_number)) {
    // there is a valid number at the spec. SIM position
    // => send SMS
    // -------------------------------------------------
    ret_val = SendSMS(sim_phone_number, message_str);
  }
  return (ret_val);

}

/**********************************************************
Method initializes memory for the incoming SMS in the Sim800 (Telit ??)
module - SMSs will be stored in the SIM card

!!This function is used internally after first registration
so it is not necessary to used it in the user sketch

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free

        OK ret val:
        -----------
        0 - SMS memory was not initialized
        1 - SMS memory was initialized

**********************************************************/
char GSM::InitSMSMemory(void) 
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not initialized yet
  
  // Disable messages about new SMS from the GSM module 
  SendATCmdWaitResp("AT+CNMI=2,0", 1000, 50, "OK", 2);

  // send AT command to init memory for SMS in the SIM card
  // response:
  // +CPMS: <usedr>,<totalr>,<usedw>,<totalw>,<useds>,<totals>
  if (AT_RESP_OK == SendATCmdWaitResp("AT+CPMS=\"SM\",\"SM\",\"SM\"", 1000, 1000, "+CPMS:", 10)) {
//    Serial.println(AT_RESP_OK); // FV 12/11
    ret_val = 1;
  }
  else ret_val = 0;

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method finds out if there is present at least one SMS with
specified status

Note:
if there is new SMS before IsSMSPresent() is executed
this SMS has a status UNREAD and then
after calling IsSMSPresent() method status of SMS
is automatically changed to READ

required_status:  SMS_UNREAD  - new SMS - not read yet
                  SMS_READ    - already read SMS                  
                  SMS_ALL     - all stored SMS

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout

        OK ret val:
        -----------
        0 - there is no SMS with specified status
        1..20 - position where SMS is stored 
                (suitable for the function GetGSM())


an example of use:
        GSM gsm;
        char position;  
        char phone_number[20]; // array for the phone number string
        char sms_text[100];

        position = gsm.IsSMSPresent(SMS_UNREAD);
        if (position) {
          // read new SMS
          gsm.GetGSM(position, phone_num, sms_text, 100);
          // now we have phone number string in phone_num
          // and SMS text in sms_text
        }
**********************************************************/
char GSM::IsSMSPresent(byte required_status) 
{
  char ret_val = -1;
  char *p_char;
  byte status;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // still not present

  switch (required_status) {
    case SMS_UNREAD:
      SIM800.print("AT+CMGL=\"REC UNREAD\"\r");
      break;
    case SMS_READ:
      SIM800.print("AT+CMGL=\"REC READ\"\r");
      break;
    case SMS_ALL:
      SIM800.print("AT+CMGL=\"ALL\"\r");
      break;
  }

  // 5 sec. for initial comm tmout
  // and max. 1500 msec. for inter character timeout
  RxInit(5000, 1500); // FV: sets timeout params, used in IsRxFinished
  // wait response is finished
  do {
    if (IsStringReceived("OK")) { 
      // perfect - we have some response, but what:

      // there is either NO SMS:
      // <CR><LF>OK<CR><LF>

      // or there is at least 1 SMS
      // +CMGL: <index>,<stat>,<oa/da>,,[,<tooa/toda>,<length>]
      // <CR><LF> <data> <CR><LF>OK<CR><LF>
      status = RX_FINISHED;
      break; // so finish receiving immediately and let's go to 
             // to check response 
    }
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  


  switch (status) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED:
      // something was received but what was received?
      // ---------------------------------------------
      if(IsStringReceived("+CMGL:")) { 
        // there is some SMS with status => get its position
        // response is:
        // +CMGL: <index>,<stat>,<oa/da>,,[,<tooa/toda>,<length>]
        // <CR><LF> <data> <CR><LF>OK<CR><LF>
        p_char = strchr((char *)comm_buf,':');
        if (p_char != NULL) {
          ret_val = atoi(p_char+1);
        }
      }
      else {
        // other response like OK or ERROR
        ret_val = 0;
      }

      // here we have WaitResp() just for generation tmout 20msec. in case OK was detected
      // not due to receiving
      WaitResp(20, 20); 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method reads SMS from specified memory(SIM) position

position:     SMS position <1..20>
phone_number: a pointer where the phone number string of received SMS will be placed
              so the space for the phone number string must be reserved - see example
SMS_text  :   a pointer where SMS text will be placed
max_SMS_len:  maximum length of SMS text excluding also string terminating 0x00 character
              
return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - specified position must be > 0

        OK ret val:
        -----------
        GETSMS_NO_SMS       - no SMS was not found at the specified position
        GETSMS_UNREAD_SMS   - new SMS was found at the specified position
        GETSMS_READ_SMS     - already read SMS was found at the specified position
        GETSMS_OTHER_SMS    - other type of SMS was found 


an example of usage:
        GSM gsm;
        char position;
        char phone_num[20]; // array for the phone number string
        char sms_text[100]; // array for the SMS text string

        position = gsm.IsSMSPresent(SMS_UNREAD);
        if (position) {
          // there is new SMS => read it
          gsm.GetGSM(position, phone_num, sms_text, 100);
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }        
**********************************************************/
char GSM::GetSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len) 
{
  char ret_val = -1;
  char *p_char; 
  char *p_char1;
  byte len;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  phone_number[0] = 0;  // end of string for now
  SMS_text[0] = 0;  // end of string for now  FV
  ret_val = GETSMS_NO_SMS; // still no SMS
  
  //send "AT+CMGR=X" - where X = position
  SIM800.print("AT+CMGR=");
  SIM800.print((int)position);  
  SIM800.print("\r");

  // 5000 msec. for initial comm tmout
  // 100 msec. for inter character tmout
  switch (WaitResp(5000, 100, "+CMGR")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // OK was received => there is NO SMS stored in this position
      if(IsStringReceived("OK")) {
        // there is only response <CR><LF>OK<CR><LF> 
        // => there is NO SMS
        ret_val = GETSMS_NO_SMS;
      }
      else if(IsStringReceived("ERROR")) {
        // error should not be here but for sure
        ret_val = GETSMS_NO_SMS;
      }
      break;

    case RX_FINISHED_STR_RECV:
      // find out what was received exactly

      //response for new SMS:
      //<CR><LF>+CMGR: "REC UNREAD","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
		  //There is SMS text<CR><LF>OK<CR><LF>
      if(IsStringReceived("\"REC UNREAD\"")) { 
        // get phone number of received SMS: parse phone number string 
        // +XXXXXXXXXXXX
        // -------------------------------------------------------
        ret_val = GETSMS_UNREAD_SMS;
      }
      //response for already read SMS = old SMS:
      //<CR><LF>+CMGR: "REC READ","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
		  //There is SMS text<CR><LF>
      else if(IsStringReceived("\"REC READ\"")) {
        // get phone number of received SMS
        // --------------------------------
        ret_val = GETSMS_READ_SMS;
      }
      else {
        // other type like stored for sending.. 
        ret_val = GETSMS_OTHER_SMS;
      }

      // extract phone number string
      // ---------------------------
      p_char = strchr((char *)(comm_buf),',');
      p_char1 = p_char+2; // we are on the first phone number character
      p_char = strchr((char *)(p_char1),'"');
      if (p_char != NULL) {
        *p_char = 0; // end of string
        strcpy(phone_number, (char *)(p_char1));
      }


      // get SMS text and copy this text to the SMS_text buffer
      // ------------------------------------------------------
      p_char = strchr(p_char+1, 0x0a);  // find <LF>
      if (p_char != NULL) {
        // next character after <LF> is the first SMS character
        p_char++; // now we are on the first SMS character 

        // find <CR> as the end of SMS string
        p_char1 = strchr((char *)(p_char), 0x0d);  
        if (p_char1 != NULL) {
          // finish the SMS text string 
          // because string must be finished for right behaviour 
          // of next strcpy() function
          *p_char1 = 0; 
        }
        // in case there is not finish sequence <CR><LF> because the SMS is
        // too long (more then 130 characters) sms text is finished by the 0x00
        // directly in the WaitResp() routine

        // find out length of the SMS (excluding 0x00 termination character)
        len = strlen(p_char);

        if (len < max_SMS_len) {
          // buffer SMS_text has enough place for copying all SMS text
          // so copy whole SMS text
          // from the beginning of the text(=p_char position) 
          // to the end of the string(= p_char1 position)
          strcpy(SMS_text, (char *)(p_char));
        }
        else {
          // buffer SMS_text doesn't have enough place for copying all SMS text
          // so cut SMS text to the (max_SMS_len-1)
          // (max_SMS_len-1) because we need 1 position for the 0x00 as finish 
          // string character
          memcpy(SMS_text, (char *)(p_char), (max_SMS_len-1));
          SMS_text[max_SMS_len] = 0; // finish string
        }
      }
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method reads SMS from specified memory(SIM) position and
makes authorization - it means SMS phone number is compared
with specified SIM phonebook position(s) and in case numbers
match GETSMS_AUTH_SMS is returned, otherwise GETSMS_NOT_AUTH_SMS
is returned

position:     SMS position to be read <1..20>
phone_number: a pointer where the tel. number string of received SMS will be placed
              so the space for the phone number string must be reserved - see example
SMS_text  :   a pointer where SMS text will be placed
max_SMS_len:  maximum length of SMS text excluding terminating 0x00 character

first_authorized_pos: initial SIM phonebook position where the authorization process
                      starts
last_authorized_pos:  last SIM phonebook position where the authorization process
                      finishes

                      Note(important):
                      ================
                      In case first_authorized_pos=0 and also last_authorized_pos=0
                      the received SMS phone number is NOT authorized at all, so every
                      SMS is considered as authorized (GETSMS_AUTH_SMS is returned)
              
return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        GETSMS_NO_SMS           - no SMS was found at the specified position
        GETSMS_NOT_AUTH_SMS     - NOT authorized SMS found at the specified position
        GETSMS_AUTH_SMS         - authorized SMS found at the specified position


an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string
        char sms_text[100]; // array for the SMS text string

        // authorize SMS with SIM phonebook positions 1..3
        if (GETSMS_AUTH_SMS == gsm.GetAuthorizedSMS(1, phone_num, sms_text, 100, 1, 3)) {
          // new authorized SMS was detected at the SMS position 1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }

        // don't authorize SMS with SIM phonebook at all
        if (GETSMS_AUTH_SMS == gsm.GetAuthorizedSMS(1, phone_num, sms_text, 100, 0, 0)) {
          // new SMS was detected at the SMS position 1
          // because authorization was not required
          // SMS is considered authorized
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }
**********************************************************/
char GSM::GetAuthorizedSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len,
                           byte first_authorized_pos, byte last_authorized_pos)
{
  char ret_val = -1;
  byte i;

#ifdef DEBUG_PRINT
    DebugPrint("DEBUG GetAuthorizedSMS\r\n", 0);
    DebugPrint("      #1: ", 0);
    DebugPrint(position, 0);
    DebugPrint("      #5: ", 0);
    DebugPrint(first_authorized_pos, 0);
    DebugPrint("      #6: ", 0);
    DebugPrint(last_authorized_pos, 1);
#endif  

  ret_val = GetSMS(position, phone_number, SMS_text, max_SMS_len);
  if (ret_val < 0) {
    // here is ERROR return code => finish
    // -----------------------------------
  }
  else if (ret_val == GETSMS_NO_SMS) {
    // no SMS detected => finish
    // -------------------------
  }
  else if (ret_val == GETSMS_READ_SMS) {
    // now SMS can has only READ attribute because we have already read
    // this SMS at least once by the previous function GetSMS() or IsSMSPresent(..)
    //
    // new READ SMS was detected on the specified SMS position =>
    // make authorization now
    // ---------------------------------------------------------
    if ((first_authorized_pos == 0) && (last_authorized_pos == 0)) {
      // authorization is not required => it means authorization is OK
      // -------------------------------------------------------------
      ret_val = GETSMS_AUTH_SMS;
    }
    else {
      ret_val = GETSMS_NOT_AUTH_SMS;  // authorization not valid yet
      for (i = first_authorized_pos; i <= last_authorized_pos; i++) {
        if (ComparePhoneNumber(i, phone_number)) {
          // phone numbers are identical
          // authorization is OK
          // ---------------------------
          ret_val = GETSMS_AUTH_SMS;
          break;  // and finish authorization
        }
      }
    }
  }
  return (ret_val);
}


/**********************************************************
Method deletes SMS from the specified SMS position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - SMS was not deleted
        1 - SMS was deleted
**********************************************************/
char GSM::DeleteSMS(byte position) 
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not deleted yet
  
  //send "AT+CMGD=XY" - where XY = position
  SIM800.print("AT+CMGD=");
  SIM800.print((int)position);  
  SIM800.print("\r");


  // 5000 msec. for initial comm tmout
  // 20 msec. for inter character timeout
  switch (WaitResp(5000, 50, "OK")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      // OK was received => SMS deleted
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR => SMS was not deleted
      ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method reads phone number string from specified SIM position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0
        phone_number is empty string

        OK ret val:
        -----------
        0 - there is no phone number on the position
        1 - phone number was found
        phone_number is filled by the phone number string finished by 0x00
                     so it is necessary to define string with at least
                     15 bytes(including also 0x00 termination character)

an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string

        if (1 == gsm.GetPhoneNumber(1, phone_num)) {
          // valid phone number on SIM pos. #1 
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone number: ", 0);
            gsm.DebugPrint(phone_num, 1);
          #endif
        }
        else {
          // there is not valid phone number on the SIM pos.#1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG there is no phone number", 1);
          #endif
        }
**********************************************************/
char GSM::GetPhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  char *p_char; 
  char *p_char1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not found yet
  phone_number[0] = 0; // phone number not found yet => empty string
  
  //send "AT+CPBR=XY" - where XY = position
  SIM800.print("AT+CPBR=");
  SIM800.print((int)position);  
  SIM800.print("\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitResp(5000, 50, "+CPBR")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      // response in case valid phone number stored:
      // <CR><LF>+CPBR: <index>,<number>,<type>,<text><CR><LF>
      // <CR><LF>OK<CR><LF>

      // response in case there is not phone number:
      // <CR><LF>OK<CR><LF>
      p_char = strchr((char *)(comm_buf),'"');
      if (p_char != NULL) {
        p_char++;       // we are on the first phone number character
        // find out '"' as finish character of phone number string
        p_char1 = strchr((char *)(p_char),'"');
        if (p_char1 != NULL) {
          *p_char1 = 0; // end of string
        }
        // extract phone number string
        strcpy(phone_number, (char *)(p_char));
        // output value = we have found out phone number string
        ret_val = 1;
      }
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // only OK or ERROR => no phone number
      ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method writes phone number string to the specified SIM position

position:     SMS position <1..20>
phone_number: phone number string for the writing

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone number was not written
        1 - phone number was written
**********************************************************/
char GSM::WritePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY,"00420123456789"
  // where XY = position,
  //       "00420123456789" = phone number string
  SIM800.print("AT+CPBW=");
  SIM800.print((int)position);  
  SIM800.print(",\"");
  SIM800.print(phone_number);
  SIM800.print("\"\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitResp(5000, 50, "OK")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      break;

    case RX_FINISHED_STR_RECV:
      // response is OK = has been written
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method del phone number from the specified SIM position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone number was not deleted
        1 - phone number was deleted
**********************************************************/
char GSM::DelPhoneNumber(byte position)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY
  // where XY = position
  SIM800.print("AT+CPBW=");
  SIM800.print((int)position);  
  SIM800.print("\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitResp(5000, 50, "OK")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      break;

    case RX_FINISHED_STR_RECV:
      // response is OK = has been written
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}





/**********************************************************
Function compares specified phone number string 
with phone number stored at the specified SIM position

position:       SMS position <1..20>
phone_number:   phone number string which should be compare

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone numbers are different
        1 - phone numbers are the same


an example of usage:
        if (1 == gsm.ComparePhoneNumber(1, "123456789")) {
          // the phone num. "123456789" is stored on the SIM pos. #1
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are the same", 1);
          #endif
        }
        else {
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are different", 1);
          #endif
        }
**********************************************************/
char GSM::ComparePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;
  char sim_phone_number[20];

#ifdef DEBUG_PRINT
    DebugPrint("DEBUG ComparePhoneNumber\r\n", 0);
    DebugPrint("      #1: ", 0);
    DebugPrint(position, 0);
    DebugPrint("      #2: ", 0);
    DebugPrint(phone_number, 1);
#endif


  ret_val = 0; // numbers are not the same so far
  if (position == 0) return (-3);
  if (1 == GetPhoneNumber(position, sim_phone_number)) {
    // there is a valid number at the spec. SIM position
    // -------------------------------------------------
    if (0 == strcmp(phone_number, sim_phone_number)) {
      // phone numbers are the same
      // --------------------------
#ifdef DEBUG_PRINT
    DebugPrint("DEBUG ComparePhoneNumber: Phone numbers are the same", 1);
#endif
      ret_val = 1;
    }
  }
  return (ret_val);
}






/******************	****************************************
Function to enable or disable echo
Echo(1)   enable echo mode
Echo(0)   disable echo mode
**********************************************************/



void GSM::Echo(byte state)
{
	if (state == 0 or state == 1)
	{
	  SetCommLineStatus(CLS_ATCMD);
	  #ifdef DEBUG_PRINT
		DebugPrint("DEBUG Echo\r\n",1);
	  #endif
	  SIM800.print("ATE");
	  SIM800.print((int)state);    
	  SIM800.print("\r");
	  delay(500);
	  SetCommLineStatus(CLS_FREE);
//	  Serial.println(comm_line_status); // FV
	}
}

void GSM::SIM800Flush() {
	while (SIM800.available() >0) {
		char z = SIM800.read();
	}
}

char GSM::getTime(char *GsmTime){
	char *p_char; 
	char *p_char1;
	char ret_val = -1;
	if (AT_RESP_OK == SendATCmdWaitResp("AT+CCLK?", 1000, 50, "OK", 5)) {
		ret_val = 1;
		p_char = strchr((char *)(comm_buf),'"');
		p_char1 = p_char+1; // first character of time
		p_char = strchr((char *)(p_char1),'"');
		if (p_char != NULL) {
			*p_char = 0; // end of string
			strcpy(GsmTime, (char *)(p_char1));
		}
	}
	else ret_val = 0; 
	return ret_val;
}

void GSM::setTime(char *time){
/*	Serial.print("time= ");
	Serial.println(time);
	Serial.print("AT+CCLK=\"");
	Serial.print(time);    
	Serial.print("\"\r"); */
	if (CLS_FREE != GetCommLineStatus()) return;
	SetCommLineStatus(CLS_ATCMD);
	SIM800.print("AT+CCLK=\"");
	SIM800.print(time);    
	SIM800.print("\"\r");
	delay(500);
	SetCommLineStatus(CLS_FREE);

}

/**********************************************
try to registrate by resetting gsm chip, 
e.g in case Sim card was replaced
**********************************************/
void GSM::resetGSM(){
  
	#ifdef gsmTestEnv
		Serial.println(" Start reset GSM");
	#endif
	
	digitalWrite(GSM_RESET, HIGH);
	delay(1200);
	digitalWrite(GSM_RESET, LOW);
	wdt_reset(); // **reset Watchdog timer** to avoid restart program
	delay(5000);

	#ifdef gsmTestEnv
	Serial.println("status op nul!");
	#endif
	
	//Set module_status to zero so the chip will receive new settings
	module_status = STATUS_NONE;

}

byte GSM::getSignalStrenght(char *SignalStrength){
	char *p_char; 
	char *p_char1;
	char ret_val = -1;
	if (AT_RESP_OK == SendATCmdWaitResp("AT+CSQ", 1000, 50, "OK", 5)) {
		ret_val = 1;
		p_char = strchr((char *)(comm_buf),':');
		p_char1 = p_char+2; // first character of time
		p_char = strchr((char *)(p_char1),',');
		if (p_char != NULL) {
			*p_char = 0; // end of string
			strcpy(SignalStrength, (char *)(p_char1));
		}
	}
	else ret_val = 0; 
	return ret_val;
}
	
