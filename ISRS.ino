/*************************************************************
// interrupt service routine, called when RNG goes from 1 to 0
**************************************************************/
void SmsInterrupt ()
{
   digitalWrite(GSM_DTR, LOW); // gsm in normal mode
  SmsReceived = true;   // set flag so main loop knows
}  // end of SmsInterrupt
