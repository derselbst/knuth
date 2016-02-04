#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <unistd.h>
#include <cmath>
#include <map>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>


// more Select Graphic Rendition (SGR) parameters:
// https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_codes
#define STYLE_RESET      "0"
#define STYLE_BOLD       "1"
#define STYLE_FAINT      "2"
#define STYLE_ITALIC     "3"
#define STYLE_UNDERLINE  "4"
#define STYLE_BLINK      "5" // slow less than 2.5Hz
#define STYLE_BLINK_FAST "6" // fast, more than 2.5Hz
#define STYLE_INVERSE    "7" // swap foreground and background
#define STYLE_HIDDEN     "8"


// Foreground Colors
#define FORE_BLACK   "30"
#define FORE_RED     "31"
#define FORE_GREEN   "32"
#define FORE_YELLOW  "33"
#define FORE_BLUE    "34"
#define FORE_MAGENTA "35"
#define FORE_CYAN    "36"
#define FORE_WHITE   "37"

// Background Colours
#define BACK_BLACK   "40"
#define BACK_RED     "41"
#define BACK_GREEN   "42"
#define BACK_YELLOW  "43"
#define BACK_BLUE    "44"
#define BACK_MAGENTA "45"
#define BACK_CYAN    "46"
#define BACK_WHITE   "47"

using namespace std;

bool colorize = 0;
bool breakOnNl = 0;
char* seperator = " ";

unsigned int bpl = 16; // Bytes Per Line
unsigned int currentPosition = 0;
unsigned int lineStart = 0;

char* temp;
char* ascBuf;

void flushLine()
{
    if (currentPosition == lineStart) return;

    unsigned int bytesLeft = bpl - (currentPosition - lineStart);

    if (bytesLeft > 0)
    {
        int spacesLeft = ((bytesLeft) * 3) - 1 + ceil((bytesLeft - 1) / 8.0);
        while(spacesLeft--) printf(" ");
    }

    printf(" | %s\n", ascBuf);

    ascBuf[0]='\0';
    lineStart = currentPosition;
}

void setStyle(char* style)
{
    sprintf(temp, "\x1B[%sm", style);
    printf("%s", temp);
    strcat(ascBuf, temp);
}

void putChars(unsigned char* data, unsigned int length, char* style)
{
    int i;
    int style_enabled = 0;

    for(i = 0; i < length; i++)
    {

        if (currentPosition == lineStart)
        {
            if (style_enabled) setStyle("");
            printf("%08X | ", currentPosition);
            style_enabled = 0;
        }

        if (!style_enabled && style)
        {
            setStyle(style);
            style_enabled = 1;
        }

        printf("%02X", data[i]);

        if (data[i] == 0)
        {
            strcat(ascBuf, ".");
        }
        else if (data[i] >= 0x20 && data[i] < 0x7F)
        {
            sprintf(temp, "%c", data[i]);
            strcat(ascBuf, temp);
        }
        else
        {
            strcat(ascBuf, ".");
        }

        currentPosition++;

        if (i == length - 1 && style_enabled)
        {
            setStyle("");
        }

        if ((currentPosition - lineStart) >= bpl)
        {
            if (style_enabled) setStyle("");
            flushLine();
            style_enabled = 0;
        }
        else
        {
            if ((currentPosition - lineStart) % 8 == 0)
            {
                printf("%s", seperator);
            }
            printf("%s", seperator);
        }

    }
}

vector<string> split(const string &s, char delim)
{
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim))
    {
        tokens.push_back(item);
    }
    return tokens;
}

typedef multimap<unsigned char, vector<int> > patternMap;

void readPatternFile(istream& textFile, patternMap& map)
{
    while(textFile.good())
    {

        string s;
        do
        {

            getline(textFile, s);
        }
        while(s.size()<=0 || s[0]=='#');

        vector<string> bytes = split(s, ';');

        for(int i = 0; i<bytes.size(); i++)
        {
            int byte;

            if(bytes[i] == "X")
            {
                byte=-1;
            }
            else
            {
                 byte = strtol(bytes[i].c_str(), NULL, 16);
            }

            // use first token as key
            map.insert(std::pair<char,int>(static_cast<unsigned char>(strtol(bytes[0].c_str(), NULL, 16)),byte));
        }
    }
}

int main(int argc, char* argv[])
{
    char opt;

    while ((opt = getopt (argc, argv, "snc:")) != -1)
    {
        switch (opt)
        {
        case 's':
            colorize = true;
            break;
        case 'c':
            bpl = atoi(optarg);
            if (bpl < 2) bpl = 2;
            if (bpl > 1024) bpl = 1024;
            break;
        case 'n':
            breakOnNl = true;
            break;
        default:
            fprintf(stderr, "Syntax: %s [OPTIONS] FILE\n", argv[0]);
            fprintf(stderr, "        %s [OPTIONS] < FILE\n", argv[0]);
            fprintf(stderr, "Options: \n");
            fprintf(stderr, "\t-c n\tSet byte count per line (default 16)\n");
            fprintf(stderr, "\t-s\tHighlight special chars and UTF-8 sequences\n");
            fprintf(stderr, "\t-n\tBreak on LF / 0x0A\n");
            fprintf(stderr, "\n");
            return 1;
        }
    }

    FILE* source;
    if(!argv[optind])
    {
        puts("No file was specified, using stdin");
        source=stdin;
    }

    source = fopen(argv[optind], "rb");

    temp = new char[30];
    ascBuf = new char[bpl*16];

    unsigned char c;

    readPatternFile();

    while(fread(c, 1, 1, source) == 1)
    {
        if (*c == 0)
        {
            putChars(c, 1, STYLE_FAINT);
        }
        else if (*c < 32 && colorize)
        {
            putChars(c, 1, STYLE_BOLD";"FORE_YELLOW);
        }
        else if (*c > 0x7F && colorize)
        {

        }
        else
        {
            putChars(c, 1, NULL);
        }

        if (breakOnNl && *c == 0x0A) flushLine();

        fflush(stdout);
    }

    flushLine();

    free(ascBuf);
    free(temp);

    return 0;
}
