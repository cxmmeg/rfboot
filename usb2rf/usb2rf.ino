// WARNING: the USB-to-RF  module does NOT have rfboot as bootloader
// But the bootloader wich is preinstalled with the module
// Normally this is a ProMini 3.3V with ATmegaBOOT
// Again: Do not replace the bootloader of the USB-to-RF module

// WARNING:
// Each FTDI chip has a unique Serial ID wich allows us to use the
// /dev/serial/by-id/usb-FTDI_FT232R_USB_UART_XXXXXXXX-if00-port0
// as a permanenent device name no matter what other devices are connected
// to the PC
// Some USB to serial adapters do not
// have a unique serial ID. If we only have one such module then is OK
// but if we have more than one connected to the PC at the same time
// it will be hard to choose the correct one.
// Invest  1-2$ more and get a
// module with a FTDI(or equivalent to FTDI) chip
// with a unique device : /dev/serial/by-id/ddddddd

#include <CC1101.h>
CC1101 rf;

// a flag that a wireless packet has been received
//volatile bool rf.interrupt = false;

// Handle interrupt from CC1101 (INT0) gdo0 on pin2
void cc1101signalsInterrupt(void) {
	// set the flag that a packet is available
	rf.interrupt = true;
}

#define PAYLOAD 32

uint8_t packet[64];

// Seems the AltSoftSerial does better than SoftSerial @ 8MHz
#include <AltSoftSerial.h>
AltSoftSerial debug_port;

// The digitalRead function is slow
// and we use it extensivelly for the debug function
// so we prefer digitalReadFast.
// https://github.com/NicksonYap/digitalWriteFast
#include <digitalWriteFast.h>

#define RESET_TRIGGER 3

#define DEBUG_PIN 4

#define debug not digitalReadFast(DEBUG_PIN)

void(* resetFunc) (void) = 0;
uint32_t silence_timer ;

void upload_code() {

}

void execCmd(uint8_t* cmd , uint8_t cmd_len ) {

	switch (cmd[0]) {

		case 'A':
			if (cmd_len==3) {
				rf.setSyncWord(cmd[1],cmd[2]);
				if (debug) {
					debug_port.print(F("Syncword = "));
					debug_port.print(cmd[1]) ;
					debug_port.print(",");
					debug_port.println(cmd[2]);
				}
			}
			else {
				if (debug) {
					debug_port.print(F("cc1101 wrong cmd size "));
					debug_port.println(cmd_len);
				}
			}
		break;
		
		case 'C':  // We set channel
			{
				if (cmd_len!=2) {
					if (debug) {
						debug_port.print(F("Channel command wrong size : "));
						debug_port.println(cmd_len);
					}
				}
				else {
					uint8_t channel = cmd[1];

					{
						rf.setChannel(channel);
						if (debug) {
							debug_port.print(F("channel="));
							debug_port.println(channel);
						}
					}
				}
			}
		break;

		case 'Q':
			if (cmd_len==1) {
				if (debug) {
					debug_port.println(F("Silent for 50ms"));
					silence_timer = millis();
				}
				resetFunc();
			}
			else {
				if (debug) {
					debug_port.print(F("Silent command, bad length : "));
					debug_port.println(cmd_len);
				}
			}
		break;

		case 'R': // MCU hardware reset
			{
				if (cmd_len!=1) {
					if (debug) {
						debug_port.print(F("MCU reset command wrong size : "));
						debug_port.println(cmd_len);
					}
				}
				else {
					if (debug) {
						debug_port.println(F("USB to RF Reset"));
					}
					rf.setSyncWord(0,0);

					digitalWriteFast(RESET_TRIGGER,LOW);
					pinModeFast(RESET_TRIGGER, OUTPUT); // reset the module because D4 is connected with RESET pin. See circuit diagram
					// the following command will be executed only if the module fails to reset
					debug_port.println(F("Usb2rf module failed to reset"));
				}
			}
		break;

		case 'U':
			// Upload mode
			// Offloads some of the work rftool does
			// to improve latency
			// TODO

			if (cmd_len==3) {
				if (debug) {
					debug_port.println(F("Switch to upload mode"));
					//debug_port.flush();
				}
				
				// Perimeno size
				uint16_t code_idx=cmd[1]+cmd[2]*256;
				byte i=0;
				uint32_t timer = millis();
				byte inpacket[64];
				byte outpacket[64];
				bool rfboot_waiting = true;
				while (code_idx) { // and (millis()-timer<1000) TODO
					
					if ( Serial.available() ) {
						if (i<PAYLOAD) {
							outpacket[i] = Serial.read();
							i++;
							if (Serial.available()<=32) Serial.write('S');
						}
					}
					
					if (rfboot_waiting and i==PAYLOAD) {
						bool succ = rf.sendPacket(outpacket,PAYLOAD);
						i=0;
						rfboot_waiting=false;
					}

					if (rf.interrupt) {
						byte pkt_size = rf.getPacket(inpacket);
						rf.interrupt = false;
						if (pkt_size==3 and rf.crc_ok) {
							// we just got a byte packet from rfboot
						}
					};

				}

				// loop stelnoume paketa
				// molis o buffer exei 32bytes i ligotera zitame paketo
				// perimenoume apantisi. an to idx pou paroume einai mikrotero apo
				// to proigoumeno stelnoume to epomeno paketo aliws to idio
				// an paroume lathos to proothoume sto PC kai feugoume
				// an teleiosoun ta paketa perimenoume CRC report kai to proothoume
				
			}
			else {
				if (debug) {
					debug_port.print(F("Upload code command, bad length : "));
					debug_port.println(cmd_len);
				}
			}
		break;

		case 'W':
			// TODO
			// send wake up 1 sec pulse
		break;

		case 'Z':
			if (cmd_len==1) {
				if (debug) {
					debug_port.println(F("Software reset"));
					debug_port.flush();
				}
				rf.setSyncWord(0,0);
				resetFunc();
			}
			else {
				if (debug) {
					debug_port.print(F("Software reset command, bad length : "));
					debug_port.println(cmd_len);
				}
			}
		break;

		default:
			if (debug) {
				debug_port.print(F("Unknown command "));
				debug_port.write(cmd,cmd_len);
				debug_port.println();
			}
			break;
		}
}


int main() {

	init(); // mandatory, for arduino functions to work
	
	pinMode(DEBUG_PIN,INPUT_PULLUP);
	Serial.begin(57600);
	
	debug_port.begin(19200);
	delay(1);

	rf.init();
	//rf.setCarrierFreq(CFREQ_433);
	rf.disableAddressCheck();
	rf.setSyncWord(57,232);
	attachInterrupt(0, cc1101signalsInterrupt, FALLING);

	debug_port.println(F("Usb2rf debug port at 19200 bps"));

	uint8_t idx = 0;
	uint32_t timer = micros();
	bool cmdmode = false;
	delay(5);
	Serial.write("USB2RF" );
	
	bool last_debug = not debug;

	while (1) {
		if (debug != last_debug) {
			last_debug = debug;
			debug_port.print(F("debug messages "));
			if (debug) debug_port.println(F("enabled"));
			else debug_port.println(F("disabled"));
		}
		if (cmdmode) {
			if (Serial.available()) {

				uint8_t msg = Serial.read();
				packet[idx]=msg;
				idx++;
				if (idx==32) {
					idx=0;
					cmdmode=false;
				}
				timer = micros();
			}
			else if (micros()-timer>2000) {
				if (idx>0) {
					execCmd(packet,idx);
				}
				cmdmode=false;
				idx=0;
				timer=micros();
			}
		}
		else {
			if (Serial.available()) {

				uint8_t msg = Serial.read();

				packet[idx]=msg;
				idx++;
				if (idx==32) {


					if (debug) debug_port.write("out 32");

					bool succ = rf.sendPacket(packet,32);

					if ( debug ) {
						if (succ)  debug_port.write("\r\n");
						else debug_port.write(" F\r\n");
					}
					while (! rf.interrupt);
					rf.interrupt = false;


					if ( debug and (!succ) ) debug_port.println(F("Sending packet failed"));

					idx=0;
				}
				else if (idx==5 and memcmp(packet,"COMMD",5)==0 ) {
					cmdmode = true;
					idx=0;
				}
				timer = micros();
			}

			else if (idx==0) {
				timer=micros();
			}

			else if (micros() - timer > 2000) {
				 {

					if (debug) {
						debug_port.write("out ");
						debug_port.print(idx);
					}
					
					bool succ;
					
					succ = rf.sendPacket(packet,idx);
					
					while (! rf.interrupt);
					rf.interrupt = false;
					if ( debug ) {
						if (succ)  debug_port.write("\r\n");
						else debug_port.write(" F\r\n");
					}
				}
				timer = micros();
				idx=0;
			}
		}

		if (rf.interrupt) {
			byte pkt_size = rf.getPacket(packet);
			rf.interrupt = false;
			//rf.setRxState();
			if ( pkt_size > 0) {
				if (not rf.crc_ok) {
					if (debug) debug_port.write("in CRC error\r\n");
				}
				else {
					// Afti i grammi diorthonei to 5088 bug pou akoma den kserw pou ofeiletai
					// pantos o bootloader stelnei olososto paketo kai o kwdikas to vlepei kanonika
					// wstoso to 4,224,19 stanei sto PC mono san 4,224
					// if (ccpacket.length==3 and ccpacket.data[0]==4 and ccpacket.data[2]<=19) ccpacket.data[2]+=128;
					
					/* if (pkt_size==8) {
						for (byte i=0;i<8;i++) {
							if (packet[i]<16) Serial.write('0');
							Serial.print(packet[i], HEX);
							Serial.write(' ');
						}
						Serial.println();
					}
					else {
						Serial.write(packet, pkt_size);
					} */
					Serial.write(packet, pkt_size);
					
					if (debug) {
						debug_port.write("in ");
						//if (pkt_size != 3)
						debug_port.println(pkt_size);
					}
				}
			}
			else if (debug) {
				debug_port.println("in 0 !");
			}
			
		}

	}
}

