#include "log.h"

// Dumps raw memory in hex byte and printable split format
void dump(const unsigned char *data_buffer, const unsigned int length) {
	unsigned char byte;
	unsigned int i, j;
	for(i=0; i < length; i++) {
		byte = data_buffer[i];
		logit("%02x ", data_buffer[i]); // Display byte in hex.
		
		if ((i+1)%4==0) {logit(" ");} // 4-byte DWORD separator
		//if ((i+1)%8==0) {printf(" ");} // 8-byte dual DWORD separator

		if(((i%16)==15) || (i==length-1)) { // end of line - print human-readable
			for(j=0; j < 15-(i%16); j++)	logit(" ");
			logit("|  ");
			for(j=(i-(i%16)); j <= i; j++) { // Display printable bytes from line.
				byte = data_buffer[j];
				if((byte > 31) && (byte < 127)) // Outside printable char range
					logit("%c", byte);
				else
					// MODIFIED
					logit("%c", byte+48);
				if ((j+1)%8==0) {logit(" ");} // 8-byte dual-DWORD separator
			}
			logit("\n"); // End of the dump line (each line is 16 bytes)

		} // End if
	} // End for
}
