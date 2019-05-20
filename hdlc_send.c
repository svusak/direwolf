
//
//    This file is part of Dire Wolf, an amateur radio packet TNC.
//
//    Copyright (C) 2011, 2013, 2014  John Langner, WB2OSZ
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "direwolf.h"

#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "hdlc_send.h"
#include "audio.h"
#include "gen_tone.h"
#include "textcolor.h"
#include "fcs_calc.h"

#define UNSCRAMBLED 1
#define SCRAMBLED 0
#define NO_NRZI 1
#define NRZI 0


static void send_control (int, int, int, int);
static void send_data (int, int, int, int);
static void send_bit (int, int, int, int);
static int calculate_len(unsigned char *, int);
static int calc_data (int);

static int number_of_bits_sent[MAX_CHANS];		// Count number of bits sent by "hdlc_send_frame" or "hdlc_send_flags"

uint8_t flip(uint8_t b)
{
    b = (b&0xF0) >> 4 | (b&0x0F) << 4;
    b = (b&0xCC) >> 2 | (b&0x33) << 2;
    b = (b&0xAA) >> 1 | (b&0x55) << 1;
    return b;
}


/*-------------------------------------------------------------
 *
 * Name:	hdlc_send
 *
 * Purpose:	Convert HDLC frames to a stream of bits.
 *
 * Inputs:	chan	- Audio channel number, 0 = first.
 *
 *		fbuf	- Frame buffer address.
 *
 *		flen	- Frame length, not including the FCS.
 *
 *		bad_fcs	- Append an invalid FCS for testing purposes.
 *
 * Outputs:	Bits are shipped out by calling tone_gen_put_bit().
 *
 * Returns:	Number of bits sent including "flags" and the
 *		stuffing bits.  
 *		The required time can be calculated by dividing this
 *		number by the transmit rate of bits/sec.
 *
 * Description:	Convert to stream of bits including:
 *			start flag
 *			bit stuffed data
 *			calculated FCS
 *			end flag
 *		NRZI encoding
 *
 * 
 * Assumptions:	It is assumed that the tone_gen module has been
 *		properly initialized so that bits sent with 
 *		tone_gen_put_bit() are processed correctly.
 *
 *--------------------------------------------------------------*/


int hdlc_send_frame (int chan, unsigned char *fbuf, int flen, int bad_fcs)
{
	int j, fcs;
	

	number_of_bits_sent[chan] = 0;


#if DEBUG
	text_color_set(DW_COLOR_DEBUG);
	dw_printf ("hdlc_send_frame ( chan = %d, fbuf = %p, flen = %d, bad_fcs = %d)\n", chan, fbuf, flen, bad_fcs);
	fflush (stdout);
#endif


	send_control (chan, 0x7e, SCRAMBLED, NRZI);	/* Start frame */
	send_control (chan, 0x7e, SCRAMBLED, NRZI);	/* Start frame */
	send_control (chan, 0x7e, SCRAMBLED, NRZI);	/* Start frame */
	send_control (chan, 0x7e, SCRAMBLED, NRZI);	/* Start frame */
	
	for (j=0; j<flen; j++) {
	  send_data (chan, fbuf[j], SCRAMBLED, NRZI);
	}

	fcs = fcs_calc (fbuf, flen);

	if (bad_fcs) {
	  /* For testing only - Simulate a frame getting corrupted along the way. */
	  send_data (chan, (~fcs) & 0xff, SCRAMBLED, NRZI);
	  send_data (chan, ((~fcs) >> 8) & 0xff, SCRAMBLED, NRZI);
	}
	else {
	  send_data (chan, fcs & 0xff, SCRAMBLED, NRZI);
	  send_data (chan, (fcs >> 8) & 0xff, SCRAMBLED, NRZI);
	}

	send_control (chan, 0x7e, SCRAMBLED, NRZI);	/* End frame */
	send_control (chan, 0x7e, SCRAMBLED, NRZI);	/* End frame */

	return (number_of_bits_sent[chan]);
}


/*-------------------------------------------------------------
 *
 * Name:	hdlc_send_flags
 *
 * Purpose:	Send HDLC flags before and after the frame.
 *
 * Inputs:	chan	- Audio channel number, 0 = first.
 *
 *		nflags	- Number of flag patterns to send.
 *
 *		finish	- True for end of transmission.
 *			  This causes the last audio buffer to be flushed.
 *
 * Outputs:	Bits are shipped out by calling tone_gen_put_bit().
 *
 * Returns:	Number of bits sent.  
 *		There is no bit-stuffing so we would expect this to
 *		be 8 * nflags.
 *		The required time can be calculated by dividing this
 *		number by the transmit rate of bits/sec.
 *
 * Assumptions:	It is assumed that the tone_gen module has been
 *		properly initialized so that bits sent with 
 *		tone_gen_put_bit() are processed correctly.
 *
 *--------------------------------------------------------------*/

int hdlc_send_flags (int chan, int nflags, int finish)
{
	int j;
	

	number_of_bits_sent[chan] = 0;


#if DEBUG
	text_color_set(DW_COLOR_DEBUG);
	dw_printf ("hdlc_send_flags ( chan = %d, nflags = %d, finish = %d )\n", chan, nflags, finish);
	fflush (stdout);
#endif

	/* The
	 * AX.25 spec states that when the transmitter is on but not sending data */
	/* it should send a continuous stream of "flags." */

	for (j=0; j<nflags; j++) {
	  send_control (chan, 0x7e, SCRAMBLED, NRZI);
	}

/* Push out the final partial buffer! */

	if (finish) {
	  audio_flush(ACHAN2ADEV(chan));
	}

	return (number_of_bits_sent[chan]);
}


int hdlc_send_header (int chan, unsigned  char *fbuf, int flen) {

	number_of_bits_sent[chan] = 0;
    //Preamble
    int i;
    for (i = 0; i < 8; i++) {
        send_control (chan, 0xAA, UNSCRAMBLED, NO_NRZI);
    }

    //Seq
	send_control (chan, ~0x7c, UNSCRAMBLED, NO_NRZI);
	send_control (chan, ~0x56, UNSCRAMBLED, NO_NRZI);

	//Length
    int frame_len = (calculate_len(fbuf, flen)+7+16+24)>>3;

    send_control (chan,~flip((frame_len>>8)&0xff),UNSCRAMBLED, NO_NRZI);
    send_control (chan,~flip((frame_len&0xff)),UNSCRAMBLED, NO_NRZI );

	return (number_of_bits_sent[chan]);
}




	static int stuff[MAX_CHANS];// Count number of "1" bits to keep track of when we
								// need to break up a long run by "bit stuffing."
								// Needs to be array because we could be transmitting
								// on multiple channels at the same time.




static void send_control (int chan, int x, int scr, int no_rzi)
{

	int i;

	for (i=0; i<8; i++) {

	    send_bit (chan, x & 1, scr, no_rzi);
	    x >>= 1;
	}
	
	stuff[chan] = 0;
}

 /*
  * Always send scrambled bits and transmit all bits
  * Used only for frame transmitting (without flags)
 */
static void send_data (int chan, int x, int scr, int nrzi)
{

	int i;

	for (i=0; i<8; i++) {
	    send_bit (chan, x & 1, scr, nrzi);
	    if (x & 1) { //  if the last bit is 1, the result of x & 1 is 1; otherwise, it is 0
	        stuff[chan]++;
	        if (stuff[chan] == 5) { // bit stuffing
	            send_bit (chan, 0, scr, nrzi);
	            stuff[chan] = 0;
	        }
	    } else {
	        stuff[chan] = 0;
	    }
	    x >>= 1;
	}
}

/*
 * NRZI encoding.
 * data 1 bit -> no change.
 * data 0 bit -> invert signal.
 */
static void send_bit (int chan, int b, int scr, int nrzi) {
	static int output[MAX_CHANS];

    if (nrzi) {
        output[chan] = b;
    } else {
        if (b == 0) {
            output[chan] = ! output[chan];
        }
    }

	number_of_bits_sent[chan]++;

	tone_gen_put_bit (chan, output[chan], scr, nrzi);

}


static int calculate_len(unsigned char *fbuf, int flen) {

	double bits_num = 0;

    int j, fcs;


    for (j=0; j<flen; j++) {
        bits_num += calc_data (fbuf[j]);
    }

    fcs = fcs_calc (fbuf, flen);


    bits_num += calc_data (fcs & 0xff);
    bits_num += calc_data ((fcs >> 8) & 0xff);

	return bits_num;
}


static int calc_data (int x)
{
    int bits_num = 0;
    int i;

    int stuffing = 0;
    for (i=0; i<8; i++) {
        bits_num += 1;
        if (x & 1) {
            stuffing += 1;
            if (stuffing == 5) {
                bits_num += 1;
                stuffing = 0;
            }
        } else {
            stuffing = 0;
        }
        x >>= 1;
    }
    return bits_num;

}


/* end hdlc_send.c */