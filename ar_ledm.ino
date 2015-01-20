// Open Illumination Hackathon
//
// 16.-18.1.2015
//
// Interactive lightning game
//
// Markus Palonen markus.palonen@aalto.fi
// Rasmus Eskola rasmus.eskola@aalto.fi
// Toni Malinen toni.malinen@aalto.fi

// Based on example:
/*****************************************************************************
* Example sketch for driving the PLAY-ZONE 16x16 LED Matrix (driven by four 74HC595)
*
* Designed specifically to work with the PLAY-ZONE 16x16 LED Matrix
*
* Uses the TimerOne-Library, which is available for download here:
* http://code.google.com/p/arduino-timerone/downloads/list
*
* V1.0: Initial Release, Dec. 24th 2013
*
* Written by Y. Puppetti (info@play-zone.ch, http://www.play-zone.ch )
* BSD license, all text above must be included in any redistribution
*
******************************************************************************/

#include "TimerOne.h"

typedef enum {
    UP,
    DOWN,
    LEFT,
    RIGHT
} direction_t;

typedef struct coord_s {
    int x;
    int y;
} Coordinate;

// Latch-Pin ("LT")
int latchPin = 4;

// Clock-Pin ("SK")
int clockPin = 3;

// Data-Pin ("R1I")
int dataPin = 2;

// Interrupts
int intLeft = 18;
int intRight = 19;
int intUp = 21;
int intDown = 20;

bool matrix[16][16] = {0};

// Spiral animation
/*
byte spiral[32] = {
B00111110,B11111100,
B00011110,B11111000,
B00000111,B11000000,
B00001110,B11100000,
B00111110,B11111000,
B01111100,B01111100,
B01111000,B00111100,
B01111100,B01111100,
B00111110,B11111000,
B00001110,B11100000,
B00000111,B11000000,
B00001110,B11100000,
B00111110,B11111000,
B00111100,B01111100,
B01111000,B00111100,
B01111100,B01111100,
};
*/

byte rows[32] = {
B01111111, B11111111,
B10111111, B11111111,
B11011111, B11111111,
B11101111, B11111111,
B11110111, B11111111,
B11111011, B11111111,
B11111101, B11111111,
B11111110, B11111111,
B11111111, B01111111,
B11111111, B10111111,
B11111111, B11011111,
B11111111, B11101111,
B11111111, B11110111,
B11111111, B11111011,
B11111111, B11111101,
B11111111, B11111110
};


byte counter = 0;
Coordinate coord;
volatile direction_t dir;
Coordinate coords[256];
int first = 0;
int last = 0;

bool end;
Coordinate point;
int points;

void setup() {

    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(intLeft, INPUT_PULLUP);
    pinMode(intRight, INPUT_PULLUP);
    pinMode(intUp, INPUT_PULLUP);
    pinMode(intDown, INPUT_PULLUP);
    pinMode(5, OUTPUT);

    attachInterrupt(3, handleDown, FALLING);
    attachInterrupt(2, handleUp, FALLING);
    attachInterrupt(4, handleRight, FALLING);
    attachInterrupt(5, handleLeft, FALLING);

    beginning();
    end = 0;

    Timer1.initialize(15000);
    Timer1.attachInterrupt(draw);
}

void loop() {

    if (!end) {
        if (points > 16) {
            points = 0;
            digitalWrite(5, HIGH);
            end = 1;
        }

        counter++;
        if (counter == 0xFF) {
            digitalWrite(5, LOW);
            if (coord.x == point.x && coord.y == point.y) {
                digitalWrite(5, HIGH);
                points++;
                point.x = random(0,15);
                point.y = random(0,15);
            }

        switch (dir) {
            case UP:
                coord.y = (coord.y - 1);
                if (coord.y < 0) {
                    end = 1;
                    beginning();
                }
                break;
            case DOWN:
                coord.y = (coord.y + 1);
                if (coord.y > 15) {
                    end = 1;
                    beginning();
                }
                break;
            case LEFT:
                coord.x = (coord.x - 1);
                if (coord.x < 0) {
                    end = 1;
                    beginning();
                }
                break;
            case RIGHT:
                coord.x = (coord.x + 1);
                if (coord.x > 15) {
                    end = 1;
                    beginning();
                }
                break;
            }
        }
    }
}

void beginning(void) {
    digitalWrite(5, LOW);
    points = 0;
    coord.x = 0;
    coord.y = 0;
    point.x = random(0,15);
    point.y = random(0,15);
    dir = RIGHT;
    memset(matrix, 0, 256);
}

void draw() {

    matrix[coord.x][coord.y] = 1;
    matrix[point.x][point.y] = 1;

    for (int i = 0; i < 16; i++) {
        int byte1 = 0x00;
        int byte2 = 0x00;
        int mask = 0x80;
        for (int j = 0; j < 8; j++) {
            if (matrix[i][j])
                byte1 |= mask;
            if (matrix[i][j+8])
                byte2 |= mask;
            mask = mask >> 1;
        }
        writeLine(byte1,  byte2 , rows[i*2], rows[i*2+1]);
    }
    matrix[coord.x][coord.y] = 0;
    matrix[point.x][point.y] = 0;
}


void writeLine(int byte1, int byte2, int byte3, int byte4) {

    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, byte1);
    shiftOut(dataPin, clockPin, MSBFIRST, byte2);
    shiftOut(dataPin, clockPin, MSBFIRST, byte3);
    shiftOut(dataPin, clockPin, MSBFIRST, byte4);
    digitalWrite(latchPin, HIGH);
}

void handleUp(void) {
    if (dir != DOWN)
        dir = UP;
}

void handleDown(void) {
    if (dir != UP)
        dir = DOWN;
}

void handleLeft(void) {
    if (dir != RIGHT)
        dir = LEFT;
}

void handleRight(void) {
    if (end) {
        end = 0;
        beginning();
    }
    if (dir != LEFT)
        dir = RIGHT;
}
