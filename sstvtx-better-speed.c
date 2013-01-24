/*
 * sstvtx - Martin 1 mode SSTV generator
 *
 * $Id: sstvtx.c 82 2008-07-30 12:13:59Z dj1yfk $
 *
 * Copyright (c) 2008 Fabian Kurz, DJ1YFK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
 
/*
 * Hacked by Steve Randall to speed audio file generation up (and bug fix)
 * Compile with: gcc sstvtxx.c -lsndfile -ljpeg -lrt -lm -o sstvtx
 */
 
 
 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sndfile.h>
#include "jpeglib.h"
#include "jerror.h"
#include <setjmp.h>
#include <unistd.h>
 
#include <time.h> // timing profile debug

struct timespec tp[8]; // timing profile debug
int clock_gettime(clockid_t clk_id, struct timespec *tp);

#define PIXEL 1
#define SYNC 0

#define SYNC_FREQ	1200.0
#define BLACK_FREQ	1500.0
#define WHITE_FREQ	2300.0

#define OUT_LEVEL	25000.0 // peak sound level

#define TWO_PI		6.28318530717958647693

#define SAMPLE_RATE	48000 // sound file sanples per sec
#define PIX_WIDTH	320
#define PIX_HEIGHT	256
 
/* JPEG input image will be read into this array; picture size 320x256,
 * RGB components in separate rows */
static unsigned char pic[PIX_WIDTH][PIX_HEIGHT * 3];	
 
typedef struct {
	int samplerate;
	double sync_len;	// length of synch impulse in sec 
	double pix_len;		// length of normal pixel in sec
	int x_width;
	double end_phase;	// ending phase of pixel
	double next_time;	// time (in sec) to next sample
	char *infile;
	char *outfile;
} SSTV;
 
void die(char *err);
int read_JPEG_file (char *filename);
int tx_sync(SNDFILE *snd, SSTV *sstv);
int tx_pixel(unsigned char p, SNDFILE *snd, SSTV *sstv, int typ);
void help (void);

long get_ms(int os)
{
	long ta;
	long tb;

	ta = tp[os].tv_sec * 1000l; // convert secs to ms;
	ta += (tp[os].tv_nsec) / 1000000l; // add in usec converted to ms
	os++;
	tb = tp[os].tv_sec * 1000l; // convert secs to ms;
	tb += (tp[os].tv_nsec) / 1000000l; // add in usec converted to ms

	return(tb - ta);
}

void dump_times(void)
{
	fprintf(stderr,"time 0 to 1 %ld ms\n",get_ms(0));
	fprintf(stderr,"time 1 to 2 %ld ms\n",get_ms(1));
	fprintf(stderr,"time 2 to 3 %ld ms\n",get_ms(2));
}

 
int main (int argc, char **argv) {
	int x,y;
 
	SSTV sstv;
	SNDFILE *out;
	SF_INFO out_info;
 
	sstv.samplerate = SAMPLE_RATE;
	sstv.x_width = PIX_WIDTH;
	/* total pixels = 320 * 256 * 3 = 245760
	   total sync = 256 * 5ms = 1.28 sec
	   pixel length, in samples, total pixels 245760, 114s - 1.28s sync 
	   results in 0.45865885... ms / pixel */
	sstv.pix_len = 0.00045865885416666666;
	sstv.sync_len = 0.005; // sync pulse length 5ms
	sstv.end_phase = 0.0;
	sstv.next_time = 1.0 / (double)SAMPLE_RATE;
	sstv.outfile = "out.wav";
	sstv.infile = "input.jpg";

 clock_gettime(CLOCK_REALTIME, &tp[0]);
 
	while ((x = getopt(argc, argv, "s:i:o:x:h")) != -1) {
		switch (x) {
			case 's':
				sstv.samplerate = atoi(optarg);
				break;
			case 'i':
				sstv.infile = optarg;
				break;
			case 'o':
				sstv.outfile = optarg;
				break;
			case 'h':
				help();
				break;
			case 'x':
				sstv.x_width= atoi(optarg);
				break;
		}
	}
 
	out_info.samplerate = sstv.samplerate;
	out_info.channels = 1;
	out_info.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
 
	if ((out = sf_open(sstv.outfile, SFM_WRITE, &out_info)) == NULL) {
		die("Opening soundfile failed.");
	}

 clock_gettime(CLOCK_REALTIME, &tp[1]);
 
	read_JPEG_file(sstv.infile);

 clock_gettime(CLOCK_REALTIME, &tp[2]);
 
	for (y=0; y < (PIX_HEIGHT * 3); y++) 
	{
		for (x = 0; x < sstv.x_width; x++) 
		{ // generate the wav file
			tx_pixel(pic[x][y], out, &sstv, PIXEL);
		}
		if (!(y % 3)) 
		{ // insert sync pulse after line generated
			tx_pixel(0, out, &sstv, SYNC);
		}
	}

 clock_gettime(CLOCK_REALTIME, &tp[3]);
 
	sf_close(out);

dump_times();
 
return 0;
}
 
void die(char *err) {
	fprintf(stderr, "Error: %s\n", err);
	exit(EXIT_FAILURE);
}
 
int read_JPEG_file (char *filename) {
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *infile;
	JSAMPARRAY buffer;
	int row_stride;
	int i, row=0;
 
	if ((infile = fopen(filename, "rb")) == NULL) {
		die("Can't open input file.");
	}
 
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void) jpeg_read_header(&cinfo, TRUE);
	(void) jpeg_start_decompress(&cinfo);
 
	if (cinfo.output_width != PIX_WIDTH || cinfo.output_height != PIX_HEIGHT) {
		die ("Input file format wrong. Need 320x256.");
	}
 
	row_stride = cinfo.output_width * cinfo.output_components;
 
	buffer = (*cinfo.mem->alloc_sarray)
			((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
 
	while (cinfo.output_scanline < cinfo.output_height) {
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
 
		/* each line in buffer: R,G,B,R,G,B -> put to array RRR/GGG/BBB */
		for (i=0; i < cinfo.output_width; i++) {
			pic[i][row] = buffer[0][3*i];
			pic[i][row+1] = buffer[0][3*i+1];
			pic[i][row+2] = buffer[0][3*i+2];
		}
		row+=3;
	}
 
	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
 
	fclose(infile);
 
	return 0;
}
  
// this routines generates the sound samples representing a single pixel
// p contains the pixel intensity 

int tx_pixel(unsigned char p, SNDFILE *snd, SSTV *sstv, int typ) {
	double freq;		// claculated frequency of sample (in rads/sec)			
	short int buf[500];	// buffer for audio samples
	int j;			// count of samples created
	double len;		// duration of sample to be created
	double end_phase;	// phase at sample end
	double ph_inc;		// phase increment (between audio samples)
	double phase;		// current phase
 
	// claculate the frequency associated with the pixel
	// normalize char to frequency range of 1500-2300Hz
	if (typ == PIXEL) {
		freq = TWO_PI * (BLACK_FREQ + (((WHITE_FREQ - BLACK_FREQ) * (double)p)/255.0));
		len = sstv->pix_len;
	}
	else {
		freq = TWO_PI * SYNC_FREQ;
		len = sstv->sync_len;
	}
 
	// calculate the phase at the end of the pixel/sync
	end_phase = sstv->end_phase + (freq * len); 

	// calculate phase increment between audio samples
	ph_inc = (freq / sstv->samplerate);

	// calculate phase at initial sample in pixel/sync
	phase = sstv->end_phase + (freq) * sstv->next_time;

	j = 0;
	while (phase < end_phase)
	{
		buf[j++] = (short int) (OUT_LEVEL * sin(phase));
		phase += ph_inc;
	}

	// claculate time next sample is beyond pixel/sync end
	sstv->next_time = (phase - end_phase) / (freq);

	// loop exits with j containing the number of samples written 
 
	while (end_phase > TWO_PI) { // phase can increment several cycles during sync
		end_phase -= TWO_PI; // keep in reasonable bounds
	}
	sstv->end_phase = end_phase; // new end_phase 

	if (sf_write_short(snd, buf, j) != j) {
		die("Writing Pixel failed.");
	}
	return 0;
}
 
 
void help (void) {
	printf("sstvtx - generates a SSTV Martin 1 image\n");
	printf("\n");
	printf("Usage: sstvtx -i input.jpg -o output.wav -x x_width -s samplerate\n");
	printf("\n");
	printf("Written by Fabian Kurz, DJ1YFK; http://fkurz.net/ham/stuff.html#sstvtx\n\n");
	exit(EXIT_SUCCESS);
}
