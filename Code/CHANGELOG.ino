/*********************************************************
  ______ ______       _______ _    _ _____  ______  _____ 
 |  ____|  ____|   /\|__   __| |  | |  __ \|  ____|/ ____|
 | |__  | |__     /  \  | |  | |  | | |__) | |__  | (___  
 |  __| |  __|   / /\ \ | |  | |  | |  _  /|  __|  \___ \ 
 | |    | |____ / ____ \| |  | |__| | | \ \| |____ ____) |
 |_|    |______/_/    \_\_|   \____/|_|  \_\______|_____/ 
****************(changelog further below)*****************                                                          

First section is a general description of this software and its features
Changelog further below is in chronological order from earliest->latest
All comments tagged -> FV or without tag  = Fernand Verstraete
                       A* or AN           = Alejandro Nan
                      
FV 18/4/2018

This program is for the SmartSwitch with mini Atmega 2560
The fuses are standard for Arduino:
  mega2560.bootloader.low_fuses=0xFF
  mega2560.bootloader.high_fuses=0xD8
  mega2560.bootloader.extended_fuses=0xFD
battery voltage measurement:
  V2 and V4+ smartswitch use same A/D limits
  analog input voltage adapted in hardware
  formula for V2:  Vbat / 3.7 /4.096 * 1024 (4.096Vref) or Vbat*67.57
  formula for V4+:  Vbat / 3.03 /5 * 1024 (5V ref) or Vbat*67.59
    ex: measurement 741 means 10.9V
EEprom memory layout:
  address 0:  servernum1
  address 20: servernum2
  address 40,41:  number 12Voutputs 
  address 42  number resets 
  address 50: client 0-8 info
Basis for the version: demo 10/10/2016
No sleep mode for processor and Sim800 module in this version
Unique sms nmbr in field8: max 9 digits < 32 bit long 
  store last 3 numbers, compare to avoid double response
  parse can be done with and without field8
Sms receive:
  at Sms interrupt, only one sms processed from Sim800 mem
  at WDT interrupt, one more sms processed
Sms send:
  when fails, stored in resend buffer, retry at WDT interrupt

*****************************************************************
   _____ _    _          _   _  _____ ______ _      ____   _____ 
  / ____| |  | |   /\   | \ | |/ ____|  ____| |    / __ \ / ____|
 | |    | |__| |  /  \  |  \| | |  __| |__  | |   | |  | | |  __ 
 | |    |  __  | / /\ \ | . ` | | |_ |  __| | |   | |  | | | |_ |
 | |____| |  | |/ ____ \| |\  | |__| | |____| |___| |__| | |__| |
  \_____|_|  |_/_/    \_\_| \_|\_____|______|______\____/ \_____|
*****************************************************************

11/10 
  servernummer gewijzigd naar FV server nr
  send SMS nr gewijzigd naar FV Gsm
  
2-10 changes for test:
  during setup: turn client power off if credit zero
  ac: creditafterupdate is sum of present credit and added credit
3-10
  small var name changes: StringToLong, CreditAfterUpdate
  rcs: Credits reported iso Credits/10
  changed check_SMS: process only if SMS existing iso process all SMS storage positions

13-10
  moved in setup: retrieve client status from eeprom, then check for unprocessd SmS
  moved check credit zero before check low credit
  switch power off before sending sms credit zero
  added check 25A fuse: send Sms, show on display
  diplay batt = 0,0V at startup solved
  deleted "Status" at kiosk display
  added storeAllInfoToEeprom at hold kiosk
  store client status at credit zero
  deleted some unnecessary lines
  refined some comments

30-10
  testservernumber and FV private number commented out
  Testservernumber 1 and 2 added, shown on serial during setup
  testservernumber1 is used for all reply and info SMS's
  store client info to eeprom at lowcredit detection
  
1-12
  scrolling display while connecting
  calculate _BatVolt for display
  measure Batvolt before checking LowBat&BatOk
  authorised server numbers not stored in Sim module
  AuthServerNumbers array, auth server number logic changed
  first info sms to AuthServerNumber [0]
  Check_SMS changed
  always delete checked sms
  delete unneeded comments
  added some comments
  TODO:  test
21-12 
  originNok sms deleted
  creditzero sms deleted
  initOK sms deleted
  on/off convertor: spare output switched together with SwitchPin
  
FUSES: setting needed? xtal clock, preserve EEprom, ext AD ref?

28-2 
  eeAddresses changed
  address 0:  servernum1
  address 20:   servernum2
  address 40  number 12Voutputs 
  address 50:   client 0-8 info

  this data is prepared per kiosk, in 3 separate .txt files
  they are combined to 1 file with a Dos batch file, and loaded in EEprom with avrdude

  function made for calculation of client eeAddress
  SmsField names "SmsFieldx" iso fx
  Deleted Pulsestart in switch power output
  
17-3
  more precise calculation for BatVolt for display and SMS
  27-3: corrected calc for display: 67.5 iso 67,5
  
28-3: 
  adapted SMS names to line up with server (spec)
  Credit0 now if Credits  <= 0 (was ==0)
  SwitchAllOff function added
  Unneeded comment lines deleted

4-4:
  set inputs with pullups (one line)
  init client at first use
  added SwitchAllOff at KioskFraud event
5-4:
  get AuthServerNumbers from eeProm in temp char array, then assign to String array
  Adapted StringToLong for negative values
6-4:
  send info sms: Low credit adapted, added signal strenth and bat volt for others
  hk and rk actions changed
10-4
  storeAllInfoToEeprom at any sms action iso store concerned client only
  cgwnum sms action pre-defined - not possible in this version  
14-4
  RestoreFromEeprom: criterium for switching on client kiosk state must be KioskOK
  Bat ok: does not put central switches on, but stores new state first, 
    then RestoreFromEeprom switches kiosk and clients on if needed
  added SwitchAllOff in setup, before GSM init
22-4
    hk: Added kioskfraud to possible previous states
  

4-5
  no more hk from vaultopen  state (22-4 change undone)
  rk from vaultopen & hk state, power to clientok & lowcredit
  cgwnum: change server num that did not send the sms
11-5
  AuthServerNumbers as char array iso String array, all copying changed
  cgwnum included
  all pins definitions in this file for easy change in HW version
  also pins extern in included pin_def_ext.h
  GSM_2560.h and GSM_2560.cpp now in wip directory
17-5: Batt measuring formula explained in intro
20-5: #ifdef / #else / #endif preprocessor for choosing V2 or V4+ PBA 
  lcd library, pin definitions, lcd backlight 
31-5: 
  include sms field 8: SmsID
  #ifdef for lcd constructor and init
  dc response: include rest Credits 
19-6
  input_pullup iso input  & write high
  fraud detection with interrupt iso polling
  watchdog interrupt set up - actions TBD
11-7
  big Credits: display client info adapted
31/7
  PowerMon pin def, init and actions defined: similar to LowBat/BatOk
23/8
  corrected dc response: rest Credits
25/8
  testing for check_SMS
28/8
  only one check sms, at Gsm and watchdog int
  2 sec delay before swith on convertor at bat ok after bat low
29/8
  Init lcd at every handle LCD, to clear (possible) strange characters
5/9
  Send SMS, Save sms in resend buffer, resend SMS toegevoegd, getste ok, zie logging SSW 5-9.txt
7/9
  store GWnums added at cgwnum
8/9
  added title section for each module
10/9
  continued adding titles
  init V4+ power interrupt pins 
14/9
  findNextField with *b variable StartChar, var begin no longer global
21/9
  max index resend sms buffer aangepast
  kiosk state ook terugzenden bij rcsOk
25/9
  Kiosk Fraud no interrupt, now polled
29/9
  Put kiosk status for rcs, fraud, convoff, convon Sms's in field 4 
  added SwitchClientsOff
  ConvOff and ConvOn sms sent on 230V check
  interrupt flags cleared faster, during loop iso handle int routine
  lcd backlight turnrd on faster to minimise light off period at re-init 
11/10
  corrected Vaultopen actions: change state first, then send sms
  SmsField4: kiosk status implemented
  
Bert 2017/10/06
  * Aanpassingen om enkel 12V uit te schakelen bij lowbat 
     (kwestie dat 220V niet uitgeschakeld wordt bij aanschakelen toestellen met grote belasting/opstartsn)
18-10
  added kiosk ok criterium to client switch on
20-10
  delete LowBatHold & ConvOffHold states, adapt status actions
  added kiok status to client state display if not kiosk ok
1-11
  power pulse interrupt: separate version for V2 and V4+
9-11
  alle overbodige code weg rond lowbat en conv off
  overal eerst status update voordat SMS met event verstuurd wordt
20/-3-2018
  10 seconds Timer ipv WDT 8sec acties
  WDT voor restart programma als niet op tijd gereset wordt (kiosk blokkeert)
  check registratie elke 10 sec
  registratie check via function
  lcd backlight on and off via functions
  Fraud interrupt lines deleted
  AT echo disabled in param set 0 ipv aparte instructie
29/3
  SMS fault byte changed to signed char iso uint_8
  unused code deleted
  SwitchClientPowerOff: extra switch off for reset coil relay
5/4
  reset counter at eeprom adress 41
  status reset counter in kiosk field of RCSOK reply, e.g.11/n n= numresets
8/4
  flash LCD backlight during registration
13/4
  in rcs response: kiosk status/numresets: "/" can be used to identify wip version
26/7
	check PulseStackDepth limit	to avoid overrun
31/7
	registration changed:
		setup not held up if not registered
		display possible if not registered
		new GetRegistered module
		check & send SMS only if IsRegistered
		show registration status on kiosk display
	10sec and display timer without MillisTimer
1/8
	 storeAllInfoToEeprom every hour
2/8
	add ESW version to field 4 of rcsOK sms
ic: >creditzero or poweron; avoid acc before ic in server or solve here?
	>>> if ClientState = StartClient
		kiosk sends accNoClient
		no credts added
	server will block acc if no ic done
	
TO DO
	in loop: switch light off only once: clientcounter>=0
	10 sec interrupt: 220V relais onpin, offpin LOW for extra safety?
	store, restore eeprom: include sms nbrs, not sent sms
	
	reset sw fuse for shown client
		display button release triggers next client 
		long push 2 secs resets fuse4
	
	store client status to EEPROM often, limit = 100k EEPROM write cycles
		10 years = 315.000.000 seconds
		with fixed eeprom address: 
			interval: 300M/100K = 3000sec ~1hr
		with var address: 
			avail mem  / length or 4k / 200 ? = 20 positions
			interval: secs / (maxnumwrites*positions) = 300M / 20*100k = 150 sec
		
	enable display reading while no connection
		program gets stuck in GetRegistered
		
	GetRegistered & bool registered; to be reviewed! 
		bool registered not used any more, use gsm.IsRegistered()
		no while loop for waiting registration
		check reg every 10 sec, result: gsm.IsRegistered() >0
		block all gsm actions if gsm.IsRegistered() == 0
			no info sms
			no sms reply, store in out buffer
		gsm.InitParam(PARAM_SET_1) is done by GetRegistered, no need to do it in setup
		show registration state on kiosk screen
		review setup: clients enabled if no gsm (at restart)  
#########################################################################
 
AN 7/5/2018

Started implementation of MillisTimer event and timer handler for MixedMode 
  MixedMode = 0, normal operation -> MixedMode = 1,Credits and other parameters may be bound to a specific time interval like 24hs)

The 3 MixedModeTimers are only for testing purposes ATM. 
  They will probably be only one to handle MixedMode but it's very easy to add if necessary and they function concurrently without problems

Adapted DisplayTimer and Timer10sec to work with MillisTimer

Divided code into tabs/files

All the handler functions of the MillisTimer objects are in the TIMERS.ino

SmsDummy1 and SmsDummy2 (were already present in the code, but not used as far as I know (placeholders?)) 
  Are now being used to parse new sms messagetypes

Added two new sms types "timers" and "pmc":
  "timers" is able to access the interface of all timers, to set or get parameters.
    SmsPortNum is the name of the timer object to access (only DisplayTimer is implemented for the moment, but the idea is to allow access to all internal timers, specially the PerodicMode ones)
    SmsDummy1 = is the "set" or "get" command
    SmsDummy2 = if the "set" command is issued, SmsDummy2 holds the parameter to be set (i.e "interval" to change the duration of the timer or "repeats" to set a finite number of 'repeats' for the timer).
    In case of a "get" command, the SmsFields will look like this (if the SmsPortNum = "DisplayTimer")
        SmsField3 = DisplayTimer.getTargetTime();
        SmsField4 = DisplayTimer.getRemainingTime();
        SmsField5 = DisplayTimer.getRemainingRepeats();
        SmsField6 = DisplayTimer.isRunning();
        SmsField7 = millis();
  "pmc" controls the status of the Mixed Mode for each client independantely or the entire kiosk as well (with only one SMS) 
        SmsPortNum is a binary string representing the status of each of the 8 clients (leftmost is client[1]), 
          in case of a string "11111111" the Kiosk (Client[0]) will also be flagged as in MixedMode
        MixedMode[9] is a boolean array representing the status of MixedMode for each client and the Kiosk as well

Function readClientData() adjusted to also display MixedMode status of each client + kiosk

#########################################################################

AN 25/5/2018 

GSM.ino -> SMS.ino

PeriodicMode -> MixedMode

Minor changes to other variable names

Upon MixedMode timer expiration, Credits are reset to MixedModeCredits

#########################################################################

AN 27/6/2018

Added "autorcs"  sms type

Added PowerMeterTimer and RcsOKTimer

#########################################################################

AN 17/7/2018

Started Power usage calculator

Added  PowerMeterTimer     // A* every 10 seconds take a count of elapsed Credits to average power use and take action
Added FuseBlown1Timer[7]   //A* different timers for each step of the process
Added FuseBlown2Timer[7]    //A* all settings and status should be accesible by sms 
Added GracePeriodTimer  // MillisTimer(GracePeriod);         //A* time in between fuseblowns/strikes/attempts (5 minutes by default)
Added FuseUnBlownTimer //   MillisTimer(FuseUnBlownPeriod);

#########################################################################

AN 27/7/2018

Started with extra long button press functionality

Basic handlers for the blown fuse logic

Added FV fix for pulse stack

#########################################################################

AN 6/8/2018

Cleanup of new FV version 0818

Removed all *A code except autorcs/setrcs to start fresh with new development

Removal of MillisTimer class and switch back to inline millis timing

#########################################################################

AN 22/8/2018

Rewrite of setrcs (formerly AutoRcs) routines to work without MillisTimer class

It uses only one sms type "setrcs" to enable/disable, configure the period, delayed start and random marge offset

Added startup information to reflect the new changes

#########################################################################
31/8
	changed sms resend buffer depth to 10 positions
8/9
	init Timers toegevoegd in setup en ik sms
10/9
	day credits and dayly limit programmation started
	DayCreditTimer introduced as "kiosk" property, not per client 
	
 #########################################################################
 12/9
  rearrange the readClientData() function to display all the new data

  readClientData() can be invoked from the serial terminal with the char 'd' or 'D'

  smsID debug tests removed (negative smsID problem solved)
 
 19/9
	when client info changes, no longer storeAllInfoToEeprom, but store only one client info
	todo: no more checking creditlow and creditzero in loop, is done at power pulse
27/9
  further changes to readClientData()
  added another test function to the serial terminal with the char 'e' or 'E' the EEPROM contents are dumped (first 300 pos)
29/9
	init all cust fields at ik sms
	added comments
	store daystartcredit for limit calculation
	split off loop from smartswitch.ino
	formatted readClientData
	small changes in Sms.ino
1/10
	numerous debug prints changed
	due to testing:
		changes in credits, sms, sswloop, fuse, smartswitch, test
3/10
	some debug prints deleted
	set action first, then print: some prints take too long
	added store changed info to eeprom 
6/10
	kiosk data now in boxData iso client¨[0]: major change!
	setdc & setrcs contain targed hour iso delay to target hour
27/10
	daily actions: switch on at credit zero, only if daily credit available
	Clear StartFuseCredits at dc to avoid fuseblown (StartFuseCredits can be high & credits cleared: diff > Maxfuse) 
16/1
	correct fault in RestoreFromEeprom: use Boxdata iso obsolete Client[0]
22/1
	delete obsolete comment blocks
26/1
	correct statements that cause compiler warnings:
	in SwitchOff12VClients: unsigned i <= Number12VOutputs
	in smartswitch.ino: 
		unsigned long DisplayTimer
		const char* ClientStates[], const char* KioskStates[]
		storeBoxInfoToEeprom()
	in IsAuthorised(char* incoming_nr)
		strcmp (AuthServerNumbers[i],incoming_nr
	in sendResponse: SmsID = 0
	in SendSmS byte result = 0
	in loop: storeAllInfoToEeprom();
4/02	
	unsigned int long delta ==> unsigned long delta
	FuseCheck(delta) before gsm registration action
5/02
 In Void DailyCreditActions
		added: StartFuseCredits[PortNum] = Client[PortNum].PaidCredits + Client[PortNum].DayCredits; 
6/02
	commenting line: 167
	SMARTSWITCH.ino
	changed delta from int to unsigned long
7/02
	SSWloop.ino
		in the powerMonitor if/else: println moved into testEnv if
	SMS.in
		check_SMS function: added wdt_reset() before delay(1000)
		
1/04/19 (Jeroen)
	SSWloop.ino: adding gsmReset function in 1 hour actions
	
	GSM_2560.ccp & GSM_2560.h: new function: gsmReset
	
		
		
*/
