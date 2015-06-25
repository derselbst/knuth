#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

short hlCtrl = 0;
short hlUnicode = 0;
short breakOnNl = 0;
char* seperator = " ";

unsigned int bpl = 16; // Bytes Per Line
unsigned int currentPosition = 0;
unsigned int lineStart = 0;

char* temp;
char* ascBuf;

void flushLine() {
	if (currentPosition == lineStart) return;

	unsigned int bytesLeft = bpl - (currentPosition - lineStart);

	if (bytesLeft > 0) {
		int spacesLeft = ((bytesLeft) * 3) - 1 + (bytesLeft / 8);
		while(spacesLeft--) printf(" ");
	}	

	printf(" | %s\n", ascBuf, currentPosition);
	
	ascBuf[0]='\0';
	lineStart = currentPosition;
}

void setStyle(char* style) {
	sprintf(temp, "\x1B[%sm", style);
	printf(temp);
	strcat(ascBuf, temp);
}

void putChars(unsigned char* data, unsigned int length, char* style) {
	int i;
	int style_enabled = 0;

	for(i = 0; i < length; i++) {

		if (currentPosition == lineStart) {
			if (style_enabled) setStyle("");
			printf("%08X | ", currentPosition);
			style_enabled = 0;
		}

		if (!style_enabled && style) {
			setStyle(style);
			style_enabled = 1;
		}

		printf("%02X", data[i]);

		if (data[i] == 0) {
			strcat(ascBuf, ".");
		} else if (data[i] >= 0x20 && data[i] < 0x7F) {
			sprintf(temp, "%c", data[i]);
			strcat(ascBuf, temp);
		} else {
			strcat(ascBuf, ".");
		}

		currentPosition++;

		if (i == length - 1 && style_enabled) {
			setStyle("");
		}

		if ((currentPosition - lineStart) >= bpl) {
			if (style_enabled) setStyle("");
			flushLine();
			style_enabled = 0;
		} else {
			if ((currentPosition - lineStart) % 8 == 0) {
				printf(seperator);
			}
			printf(seperator);
		}

	}
}

int main(int argc, char* argv[])
{
	char opt;
	FILE* source = stdin;

	while ((opt = getopt (argc, argv, "cun")) != -1) {
		switch (opt) {
			case 'c':
				hlCtrl = 1;
				break;
			case 'u':
				hlUnicode = 1;
				break;
			case 'n':
				breakOnNl = 1;
				break;
			default:
				fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;
		}
	}

	temp = (char*)malloc(30);
	ascBuf = (char*)malloc(bpl * 16);

	unsigned char* c = (char*)malloc(15);
	unsigned charLength = 1;

	while(fread(c, 1, 1, stdin) == 1) {
		RETRY:
		charLength = 1;
		if (*c == 0) {
			putChars(c, 1, "2");
		} else if (*c < 32 && hlCtrl) {
			putChars(c, 1, "1;33");
		} else if (*c > 0x7F && hlUnicode) {
			int sequenceLength = 0;
			int foundLength = 1;
			char* style="44";
			if (*c < 0xC2) { // continuation or overlong
				sequenceLength = 1;
				style = "41";
			} else if (*c < 0xE0) {
				sequenceLength = 2;
			} else if (*c < 0xF0) {
				sequenceLength = 3;
			} else if (*c < 0xF8) {
				sequenceLength = 4;
			} else if (*c < 0xFC) {
				sequenceLength = 5;
			} else if (*c < 0xFE) {
				sequenceLength = 6;
			} else if (*c < 0xFF) {
				sequenceLength = 7;
			} else {
				sequenceLength = 1;
				style = "41";
			}

			while(charLength < sequenceLength) {
				if (fread(c + charLength, 1, 1, stdin) < 1) break;
				charLength++;
				if ((*(c + charLength - 1) & 0xC0) == 0x80) {
					foundLength++;
				} else {
					break;
				}
			}

			if (foundLength != sequenceLength) style = "41";

			putChars(c, foundLength, style);

			if (foundLength != charLength) {
				*c = *(c + charLength - 1);
				goto RETRY;
			}
		} else {
			putChars(c, 1, 0);
			if (breakOnNl && *c == 0x0A) flushLine();
		}

		fflush(stdout);
	}

	flushLine();

	return 0;
}
