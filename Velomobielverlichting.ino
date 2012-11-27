#include "Streaming.h"

const int       AantalKnippers  = 10;

const byte	StadslichtSterkte	  = 10;		// Stadslicht op 4% van max
const byte      DimlichtVolSterkte        = 255;
const byte      GrootlichtVolSterkte      = 255;
const byte      AchterlichtRemSterkte     = 255;
const byte      AchterlichtNormaalSterkte = 10;

const int	BlinkDuration	= 333;		// 333 ms on/off, 1,5 Hz

// Constanten voor de toestandsmachine
enum { LT_OFF, LT_OFF_PLUS, LT_DIM_MIN, LT_DIM, LT_DIM_PLUS, LT_GROOT_MIN, LT_GROOT };
enum { BL_OFF, BL_ON, BL_PAUSE };

char		Min, Plus, Links, Rechts, Rem;

volatile int    LinksLat, RechtsLat, Clock;           // moet nog aangepast worden aan Arduino

void SetDimlicht(byte Sterkte) {
}

void SetGrootlicht(byte Sterkte) {
}

void SetAchterlicht(byte Sterkte) {
}

void SetKnipperL(byte Sterkte) {
}

void SetKnipperR(byte Sterkte) {
}


void HandleKnipper() {
	static int	State, NextTime;
	static char	LKnippers, RKnippers;
	
	switch (State) {
		case BL_OFF:
			if (Links) {
				LKnippers	= AantalKnippers;
				LinksLat	= 1;
				NextTime	= Clock+BlinkDuration;
				State		= BL_ON;
			} 
			if (Rechts) {
				RKnippers	= AantalKnippers;
				RechtsLat	= 1;
				NextTime	= Clock+BlinkDuration;
				State		= BL_ON;
			}
			break;
			
		case BL_ON:
			if (Clock-NextTime>=0) {
				NextTime	+=BlinkDuration;
				LinksLat	= 0;
				RechtsLat	= 0;
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
				if (LKnippers) LinksLat	= 1;
				if (RKnippers) RechtsLat	= 1;
				State		= LKnippers || RKnippers ? BL_ON : BL_OFF;
			}
			break;
			
		default:
			LinksLat	= 0;
			RechtsLat	= 0;
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
			}
			break;
			
		case LT_DIM:
			if (Min) {
				State	= LT_DIM_MIN;
				SetDimlicht(StadslichtSterkte);
			}
			if (Plus) {
				State	= LT_DIM_PLUS;
				SetGrootlicht(GrootlichtVolSterkte);
			}
			break;
			
		case LT_DIM_PLUS:
			if (!Plus) {
				State	= LT_GROOT;
			}
			break;
			
		case LT_GROOT_MIN:
			if (!Min) {
				State	= LT_DIM;
			}
			break;
			
		case LT_GROOT:
			if (Min) {
				State	= LT_GROOT_MIN;
				SetGrootlicht(0);
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
  Serial.begin(115200);
  while (!Serial) ;
  Serial << "TCCR0B: " << _HEX(TCCR0B) << endl << "TCCR1B: " << _HEX(TCCR1B) << endl<< "TCCR3B: " << _HEX(TCCR3B) << endl << "TCCR4B: " << _HEX(TCCR4B) << endl; 
  SetClockTo2MHz();
  Serial << "TCCR0B: " << _HEX(TCCR0B) << endl << "TCCR1B: " << _HEX(TCCR1B) << endl<< "TCCR3B: " << _HEX(TCCR3B) << endl << "TCCR4B: " << _HEX(TCCR4B) << endl; 
  
  pinMode(13, OUTPUT);
}


void loop(void) {
  analogWrite(13, 10);
  delay(500);
  analogWrite(13,128);
  delay(500);
}
