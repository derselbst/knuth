#include <iostream>
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

const int NUMBER_OF_STYLES = 3;
const int NUMBER_OF_COLORS = 15;
string style[NUMBER_OF_STYLES][NUMBER_OF_COLORS]=
{
    {FORE_BLACK";"BACK_WHITE,               FORE_RED,               FORE_GREEN,                 FORE_YELLOW,                FORE_BLUE";"BACK_WHITE                   FORE_MAGENTA,                FORE_CYAN,                 BACK_BLACK";"FORE_WHITE,                BACK_RED,                   BACK_GREEN,                 BACK_YELLOW,                BACK_BLUE,                  BACK_MAGENTA,                   BACK_CYAN,                  BACK_WHITE},
    {STYLE_BOLD";"FORE_BLACK";"BACK_WHITE,  STYLE_BOLD";"FORE_RED,  STYLE_BOLD";"FORE_GREEN,    STYLE_BOLD";"FORE_YELLOW,   STYLE_BOLD";"FORE_BLUE";"BACK_WHITE,     STYLE_BOLD";"FORE_MAGENTA,   STYLE_BOLD";"FORE_CYAN,    STYLE_BOLD";"BACK_BLACK";"FORE_WHITE,   STYLE_BOLD";"BACK_RED,      STYLE_BOLD";"BACK_GREEN,    STYLE_BOLD";"BACK_YELLOW,   STYLE_BOLD";"BACK_BLUE,     STYLE_BOLD";"BACK_MAGENTA,      STYLE_BOLD";"BACK_CYAN,     STYLE_BOLD";"BACK_WHITE},
    {STYLE_FAINT";"FORE_BLACK";"BACK_WHITE, STYLE_FAINT";"FORE_RED, STYLE_FAINT";"FORE_GREEN,   STYLE_FAINT";"FORE_YELLOW,  STYLE_FAINT";"FORE_BLUE";"BACK_WHITE,    STYLE_FAINT";"FORE_MAGENTA,  STYLE_FAINT";"FORE_CYAN,   STYLE_FAINT";"BACK_BLACK";"FORE_WHITE,  STYLE_FAINT";"BACK_RED,     STYLE_FAINT";"BACK_GREEN    STYLE_FAINT";"BACK_YELLOW,  STYLE_FAINT";"BACK_BLUE,    STYLE_FAINT";"BACK_MAGENTA,     STYLE_FAINT";"BACK_CYAN,    STYLE_FAINT";"BACK_WHITE},
};


bool colorize = 0;
bool breakOnNl = 0;
char* seperator = " ";

unsigned int bpl = 16; // Bytes Per Line
unsigned int currentPosition = 0;
unsigned int lineStart = 0;

char* temp;
char* ascBuf;

typedef multimap<unsigned char, vector<string> > patternMap;
patternMap pattern;

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

void setStyle(const char*const style)
{
    sprintf(temp, "\x1B[%sm", style);
    printf("%s", temp);
    strcat(ascBuf, temp);
}

void putChars(unsigned char* data, unsigned int length, const char*const style)
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


void readPatternFile(istream& textFile, patternMap& map)
{
    string s;
    vector<string> bytes;

    while(textFile.good())
    {
RETRY:
        getline(textFile, s);
        if(textFile.eof())
        {
            break;
        }
        else if(s.size()<=0 || s[0]=='#')
        {
            goto RETRY;
        }

        bytes = split(s, ';');

        // use first token as key
        map.insert(std::pair<unsigned char, vector<string> >(static_cast<unsigned char>(strtol(bytes[0].c_str(), NULL, 16)),bytes));
    }
}


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
void* fileToMem(const char* filename, int& fd, int& len)
{
    fd = open (filename, O_RDONLY);
    if (fd == -1)
    {
        perror ("open");
        return NULL;
    }

    struct stat sb;
    if (fstat (fd, &sb) == -1)
    {
        perror ("fstat");
        return NULL;
    }

    if (!S_ISREG (sb.st_mode))
    {
        fprintf (stderr, "%s is not a file\n", filename);
        return NULL;
    }

    len=sb.st_size;
    void* p = mmap (0, len, PROT_READ, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED)
    {
        perror ("mmap");
        return NULL;
    }

    return p;
}

void fileToMemFree(void *p, int& fd, int& len)
{
    if (munmap (p, len) == -1)
    {
        perror ("munmap");
    }

    if (close (fd) == -1)
    {
        perror ("close");
    }
}

int main(int argc, char* argv[])
{
    char opt;

    while ((opt = getopt (argc, argv, "snc:p:")) != -1)
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
        case 'p':
        {
            ifstream patternFile;
            patternFile.open(optarg, ios::in);
            if(!patternFile.good())
            {
                fprintf(stderr,"some error with pattern");
            }
            else
            {
                readPatternFile(patternFile, pattern);
            }
            break;
        }
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
    memset(ascBuf, 0, sizeof(ascBuf));

    int len;
    int fd;
    unsigned char* file = static_cast<unsigned char*>(fileToMem(argv[optind], fd, len));
    for(int i=0; i<len; i++)
    {
        unsigned char& ch = file[i];

        pair <patternMap::iterator, patternMap::iterator> ret;
        ret = pattern.equal_range(ch);

        int matchFound=0;
        for (patternMap::iterator it=ret.first; it!=ret.second; ++it)
        {
            matchFound++;

            vector<string>& bytes = it->second;

            for(int j=1; j<bytes.size(); j++)
            {
                if(bytes[j]=="X");
                else if(file[i+j]!=static_cast<unsigned char>(strtol(bytes[j].c_str(), NULL, 16)))
                {
                    break;
                }
                matchFound++;
            }

            if(matchFound==bytes.size())
            {
                break;
            }
            matchFound=0;

        }

        if(matchFound!=0)
        {
            // colorize
            putChars(file+i, matchFound, style[ch%NUMBER_OF_STYLES][ch%NUMBER_OF_COLORS].c_str());
        }
        else
        {
            // only write current byte to file
            putChars(file+i, 1, NULL);
        }

        // skip the bytes we ve already printed
        i+=matchFound;
    }
    flushLine();
    cout << endl;

    fileToMemFree(file, fd, len);

//
//    while(fread(c, 1, 1, source) == 1)
//    {
//
//        if (*c == 0)
//        {
//            putChars(c, 1, STYLE_FAINT);
//        }
//        else if (*c < 32 && colorize)
//        {
//            putChars(c, 1, STYLE_BOLD";"FORE_YELLOW);
//        }
//        else if (*c > 0x7F && colorize)
//        {
//
//        }
//        else
//        {
//            putChars(c, 1, NULL);
//        }
//
//        if (breakOnNl && *c == 0x0A) flushLine();
//
//        fflush(stdout);
//    }
//
//    flushLine();

    delete [](ascBuf);
    delete [](temp);

    return 0;
}
