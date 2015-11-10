#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>

short colorize = 0;
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
		int spacesLeft = ((bytesLeft) * 3) - 1 + ceil((bytesLeft - 1) / 8.0);
		while(spacesLeft--) printf(" ");
	}	

	printf(" | %s\n", ascBuf);

	ascBuf[0]='\0';
	lineStart = currentPosition;
}

void setStyle(char* style) {
	sprintf(temp, "\x1B[%sm", style);
	printf("%s", temp);
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
				printf("%s", seperator);
			}
			printf("%s", seperator);
		}

	}
}

int main(int argc, char* argv[])
{
	char opt;
	FILE* source = stdin;

	while ((opt = getopt (argc, argv, "snc:")) != -1) {
		switch (opt) {
			case 's':
				colorize = 1;
				break;
			case 'c':
				bpl = atoi(optarg);
				if (bpl < 2) bpl = 2;
				if (bpl > 1024) bpl = 1024;
				break;
			case 'n':
				breakOnNl = 1;
				break;
			default:
				fprintf(stderr, "Options: \n");
				fprintf(stderr, "\t-c n\tSet byte count per line (default 16)\n");
				fprintf(stderr, "\t-s\tHighlight special chars (\x1B[1;33mctrl\x1B[m/\x1B[44mUTF-8\x1B[m/\x1B[41minvalid\x1B[m)\n");
				fprintf(stderr, "\t-n\tBreak on LF / 0x0A\n");
				return 1;
		}
	}

	temp = (char*)malloc(30);
	ascBuf = (char*)malloc(bpl * 16);

	unsigned char* c = (char*)malloc(15);
	unsigned eatenLength = 1;

	while(fread(c, 1, 1, stdin) == 1) {
		RETRY:
		eatenLength = 1;
		if (*c == 0) {
			putChars(c, 1, "2");
		} else if (*c < 32 && colorize) {
			putChars(c, 1, "1;33");
		} else if (*c > 0x7F && colorize) {
			int expectedLength = 0;
			int validLength = 1;
			char* style="44";
			if (*c < 0xC2) { // continuation or overlong
				expectedLength = 1;
				style = "41";
			} else if (*c < 0xE0) {
				expectedLength = 2;
			} else if (*c < 0xF0) {
				expectedLength = 3;
			} else if (*c < 0xF8) {
				expectedLength = 4;
			} else if (*c < 0xFC) {
				expectedLength = 5;
			} else if (*c < 0xFE) {
				expectedLength = 6;
			} else if (*c < 0xFF) {
				expectedLength = 7;
			} else {
				expectedLength = 1;
				style = "41";
			}

			while(eatenLength < expectedLength) {
				if (fread(c + eatenLength, 1, 1, stdin) < 1) break;
				eatenLength++;
				if ((*(c + eatenLength - 1) & 0xC0) == 0x80) {
					validLength++;
				} else {
					break;
				}
			}

			if (validLength != expectedLength) style = "41";

			putChars(c, validLength, style);

			if (validLength != eatenLength) {
				*c = *(c + eatenLength - 1);
				goto RETRY;
			}
		} else {
			putChars(c, 1, 0);			
		}

		if (breakOnNl && *c == 0x0A) flushLine();

		fflush(stdout);
	}

	flushLine();

	return 0;
}
