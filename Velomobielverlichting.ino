#include "avr/sleep.h"
#include "Streaming.h"

const int       AantalKnippers  = 10;

const byte	StadslichtSterkte	  = 10;		// Stadslicht op 4% van max
const byte      DimlichtVolSterkte        = 255;
const byte      GrootlichtVolSterkte      = 255;
const byte      AchterlichtRemSterkte     = 255;
const byte      AchterlichtNormaalSterkte = 10;
const byte      KnipperSterkte            = 255;

const int	BlinkDuration	= 333;		// 333 ms on/off, 1,5 Hz
const int       DebounceTime    = 50;

/***************************************************************************************************

Pinout Leonardo Nano Pro Mini / LadyAda Atmega32u4 Breakout Board:
3V          5V
E6          ARef
B0          F0-A5
B1          F1-A4
B2          F4-A3
B3          F5-A2
D0-3-P      F6-A1
D1-2        F7-A0
D2-0        C7-13-P
D3-1        C6-5-P
D4-A        B7-11-P
D5          B6-10-AP
D6-12-A     B5-9-AP
D7-8-AP     B4-8-A
GND         GND

***************************************************************************************************/

// ingangen
const byte      MinPin    = 3;
const byte      PlusPin   = 2;
const byte      LinksPin  = 0;
const byte      RechtsPin = 1;
const byte      RemPin    = 12;
const byte      BatSensePin = A5;
// uitgangen
const byte      AchterlichtPin = 13;
const byte      DimlichtPin    = 5;
const byte      GrootlichtPin  = 11;
const byte      KnipperLPin    = 10;
const byte      KnipperRPin    = 9;



// Constanten voor de toestandsmachine
enum { LT_OFF, LT_OFF_PLUS, LT_DIM_MIN, LT_DIM, LT_DIM_PLUS, LT_GROOT_MIN, LT_GROOT };
enum { BL_OFF, BL_ON, BL_PAUSE };

volatile int    Clock, OldClock;

class TDebouncedPin {
  byte    Debounced;
  byte    PinNo;
  int     TimeOfLast;
public:
  TDebouncedPin(byte Pin);
  operator byte() { return Debounced; }
  void debounce(void);
};

TDebouncedPin::TDebouncedPin(byte Pin) {
  PinNo  = Pin;
  pinMode(Pin, INPUT_PULLUP);
  Debounced   = digitalRead(Pin);
}

void TDebouncedPin::debounce(void) {
  char  New = digitalRead(PinNo);
  if (New==Debounced) TimeOfLast=Clock;
  else if (Clock-TimeOfLast > DebounceTime) Debounced=New;
}

TDebouncedPin	Min(MinPin), Plus(PlusPin), Links(LinksPin), Rechts(RechtsPin), Rem(RemPin);

inline void SetAchterlicht(byte Sterkte) {
  analogWrite(AchterlichtPin, Sterkte);
}

inline void SetDimlicht(byte Sterkte) {
  analogWrite(DimlichtPin, Sterkte);
}

inline void SetGrootlicht(byte Sterkte) {
  analogWrite(GrootlichtPin, Sterkte);
}

inline void SetKnipperL(byte Sterkte) {
  analogWrite(KnipperLPin, Sterkte);
}

inline void SetKnipperR(byte Sterkte) {
  analogWrite(KnipperRPin, Sterkte);
}


void HandleKnipper() {
	static int	State, NextTime;
	static char	LKnippers, RKnippers;
	
	switch (State) {
		case BL_OFF:
			if (Links) {
				LKnippers	= AantalKnippers;
				SetKnipperL(KnipperSterkte);
				NextTime	= Clock+BlinkDuration;
				State		= BL_ON;
			} 
			if (Rechts) {
				RKnippers	= AantalKnippers;
				SetKnipperR(KnipperSterkte);
				NextTime	= Clock+BlinkDuration;
				State		= BL_ON;
			}
			break;
			
		case BL_ON:
			if (Clock-NextTime>=0) {
				NextTime	+=BlinkDuration;
				SetKnipperL(0);
				SetKnipperR(0);
				State		= BL_PAUSE;
				if (LKnippers)	LKnippers--;
				if (RKnippers)	RKnippers--;
			} else {
				if (!LKnippers && Links)	{
					LKnippers	= 2;    // Verklaar dit nog even
					if (!Rechts) RKnippers	= 0;
				}
				if (!RKnippers && Rechts) {
					RKnippers	= 2;
					if (!Links) LKnippers	= 0;
				}
			}	
			break;
			
		case BL_PAUSE:
			if (Clock-NextTime>=0) {
				NextTime	+=BlinkDuration;
				if (!LKnippers && Links)	{
					LKnippers	= 1;
					if (!Rechts) RKnippers	= 0;
				}
				if (!RKnippers && Rechts) {
					RKnippers	= 1;
					if (!Links) LKnippers	= 0;
				}
				if (LKnippers) SetKnipperL(KnipperSterkte);
				if (RKnippers) SetKnipperR(KnipperSterkte);
				State		= LKnippers || RKnippers ? BL_ON : BL_OFF;
			}
			break;
			
		default:
			SetKnipperL(0);
			SetKnipperR(0);
			State		= BL_OFF;
			break;
	}
}


void HandleLicht() {
	static int State;
	
	switch (State) {
		case LT_OFF:		// Lichten uit, geen schakelaars
			if (Plus) {
				State	= LT_OFF_PLUS;
			}
			break;
			
		case LT_OFF_PLUS:
			if (!Plus) {
				State	= LT_DIM;
				SetDimlicht(DimlichtVolSterkte);
			}
			break;
			
		case LT_DIM_MIN:	// Licht was gedimd, 'Min' wordt ingedrukt
			if (!Min) {
				State	= LT_OFF;
				SetDimlicht(StadslichtSterkte);
			}
			break;
			
		case LT_DIM:
			if (Min) {
				State	= LT_DIM_MIN;
			}
			if (Plus) {
				State	= LT_DIM_PLUS;
			}
			break;
			
		case LT_DIM_PLUS:
			if (!Plus) {
				State	= LT_GROOT;
				SetGrootlicht(GrootlichtVolSterkte);
			}
			break;
			
		case LT_GROOT_MIN:
			if (!Min) {
				State	= LT_DIM;
				SetGrootlicht(0);
			}
			break;
			
		case LT_GROOT:
			if (Min) {
				State	= LT_GROOT_MIN;
			}
			break;
			
		default:
			State	= LT_OFF;
			break;
	}
}


void HandleRem() {
	if (Rem) SetAchterlicht(AchterlichtRemSterkte);
	else SetAchterlicht(AchterlichtNormaalSterkte);
}


void SetClockTo2MHz(void) {
  noInterrupts();
  CLKPR  = 128;    // Bereid schrijven naar clock pescaler voor
  CLKPR  = 3;      // Zet clock prescaler op delen door 8 (ipv 1)
  TCCR0B = (TCCR0B & ~7) | 2;  // Timer0 prescaler to 8
  TCCR1B = (TCCR1B & ~7) | 2;  // Timer1 prescaler to 8
  TCCR3B = (TCCR3B & ~7) | 2;  // Timer3 prescaler to 8
  TCCR4B = (TCCR4B & ~15)| 4;  // Timer4 prescaler to 8
  interrupts();
}


void setup(void) {
  SetClockTo2MHz();
}


void loop(void) {
  Clock  = (int)millis();       // we hebben de tijd nodig als int, niet als usigned long
  if (Clock!=OldClock) {        // voer de hoofdroutine 1x per milliseconde uit, en ga daarna weer slapen
    Clock = OldClock;
    Min.debounce();
    Plus.debounce();
    Links.debounce();
    Rechts.debounce();
    Rem.debounce();
    HandleKnipper();
    HandleLicht();
    HandleRem();
  }
  sleep_cpu();
}
