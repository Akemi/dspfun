/*
 * zoom - Interpolate images with a cosine basis at arbitrary scales/offsets.
 * Copyright 2013-2016 0x09.net.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <wand/MagickWand.h>
#include <getopt.h>

#include <fftw3.h>

#include "longmath.h"

enum basis {
	INTERPOLATED = 0,
	CENTERED,
	NATIVE,
	UNITARY
};

void usage(const char* self) {
	printf("usage: %s -s scale -p pos -v viewport --basis=interpolated,centered,native,unitary -c --showsamples=1(point),2(grid) input output\n",self);
	exit(0);
}

int main(int argc, char* argv[]) {
	long double vx = 0, vy = 0;
	size_t vw = 0, vh = 0;
	bool vflag = false, centered = false;
	int showsamples = 0;
	long double scale_num = 1;
	unsigned long long scale_den = 1;
	int basis = INTERPOLATED;
	int c;
	const struct option opts[] = {
		{"showsamples",optional_argument,NULL,1},
		{"basis",required_argument,NULL,2},
		{0}
	};

	while((c = getopt_long(argc,argv,"s:v:p:c",opts,&optind)) != -1) {
		switch(c) {
			case  0 : break;
			case 's': sscanf(optarg,"%Lf/%llu",&scale_num,&scale_den); break;
			case 'v': sscanf(optarg,"%zux%zu",&vw,&vh); break;
			case 'p': sscanf(optarg,"%Lfx%Lf",&vx,&vy); break;
			case 'c': centered = true; break;
			case 1: showsamples = strtol(optarg,NULL,10); break;
			case 2: {
				if(!strcmp(optarg,"centered"))
					basis = CENTERED;
				else if(!strcmp(optarg,"native"))
					basis = NATIVE;
				else if(!strcmp(optarg,"unitary"))
					basis = UNITARY;
			}; break;
			default: usage(argv[0]);
		}
	}

	if(argc-optind < 2)
		usage(argv[0]);

	const char* infile = argv[optind++],* outfile = argv[optind++];

	MagickWandGenesis();
	MagickWand* wand = NewMagickWand();
	if(MagickReadImage(wand,infile) == MagickFalse) {
		DestroyMagickWand(wand);
		MagickWandTerminus();
		return 1;
	}
	size_t width = MagickGetImageWidth(wand), height = MagickGetImageHeight(wand);
	if(!vw) vw = width*scale_num/scale_den;
	if(!vh) vh = height*scale_num/scale_den;

	double* coeffs = malloc(sizeof(double)*width*height*3);
	MagickExportImagePixels(wand,0,0,width,height,"RGB",DoublePixel,coeffs);

	if(centered) {
		vx = vx*scale_num/scale_den - vw/2.L;
		vy = vy*scale_num/scale_den - vh/2.L;
	}
	long double scale = scale_num / scale_den;
	size_t nwidth = round(width * scale_num / scale_den), nheight = round(height * scale_num / scale_den);
	size_t cwidth = nwidth < width ? nwidth : width, cheight = nheight < height ? nheight : height;
	unsigned char* icoeffs = malloc(vw*vh*3);
	double* tmp = malloc(sizeof(double)*cheight);
	double* twiddles[2] = {malloc(sizeof(double)*vw*(cwidth-1)),malloc(sizeof(double)*vh*(cheight-1))};

	switch(basis) {
		case INTERPOLATED: {
			for(size_t t1 = 0; t1 < vw; t1++)
				for(size_t t2 = 1; t2 < cwidth; t2++)
					twiddles[0][t1*(cwidth-1)+t2-1] = cos((t1*scale_den/scale_num+vx+0.5L) * t2 * M_PIl / width);
			for(size_t t1 = 0; t1 < vh; t1++)
				for(int t2 = 1; t2 < cheight; t2++)
					twiddles[1][t1*(cheight-1)+t2-1] = cos((t1*scale_den/scale_num+vy+0.5L) * t2 * M_PIl / height);
		}; break;
		case CENTERED: {
			for(size_t t1 = 0; t1 < vw; t1++)
				for(size_t t2 = 1; t2 < cwidth; t2++)
					twiddles[0][t1*(cwidth-1)+t2-1] = cos((t1*(width-1)*scale_den/(scale_num*width-scale_den)+vx+0.5L) * t2 * M_PIl / width);
			for(size_t t1 = 0; t1 < vh; t1++)
				for(int t2 = 1; t2 < cheight; t2++)
					twiddles[1][t1*(cheight-1)+t2-1] = cos((t1*(height-1)*scale_den/(scale_num*width-scale_den)+vy+0.5L) * t2 * M_PIl / height);
		}; break;
		case NATIVE: {
			for(size_t t1 = 0; t1 < vw; t1++)
				for(size_t t2 = 1; t2 < cwidth; t2++)
					twiddles[0][t1*(cwidth-1)+t2-1] = cos((t1+vx+0.5L) * t2 * M_PIl*scale_den / (scale_num*width));
			for(size_t t1 = 0; t1 < vh; t1++)
				for(int t2 = 1; t2 < cheight; t2++)
					twiddles[1][t1*(cheight-1)+t2-1] = cos((t1+vy+0.5L) * t2 * M_PIl*scale_den / (scale_num*height));
		}; break;
		case UNITARY: {
			for(size_t t1 = 0; t1 < vw; t1++)
				for(size_t t2 = 1; t2 < cwidth; t2++)
					twiddles[0][t1*(cwidth-1)+t2-1] = cos(((t2-1)*t1*scale_den/(scale_num*width)+vx+0.5L) * (t2-1) * M_PIl / t2);
			for(size_t t1 = 0; t1 < vh; t1++)
				for(int t2 = 1; t2 < cheight; t2++)
					twiddles[1][t1*(cheight-1)+t2-1] = cos(((t2-1)*t1*scale_den/(scale_num*height)+vy+0.5L) * (t2-1) * M_PIl / t2); 
		}; break;

	}

	fftw_plan p = fftw_plan_many_r2r(2,(int[]){height,width},3,coeffs,NULL,3,1,coeffs,NULL,3,1,(fftw_r2r_kind[]){FFTW_REDFT10,FFTW_REDFT10},FFTW_ESTIMATE);
	fftw_execute(p);
	fftw_destroy_plan(p);

	for(int z = 0; z < 3; z++) {
		for(int i = 0; i < vw; i++) {
			for(int row = 0; row < cheight; row++) {
				tmp[row] = coeffs[row*width*3+z]/2;
				for(int u = 1; u < cwidth; u++)
					tmp[row] += coeffs[(row*width+u)*3+z] * twiddles[0][i*(cwidth-1)+u-1];
			}
			for(int j = 0; j < vh; j++) {
				double s = tmp[0]/2;
				for(int v = 1; v < cheight; v++)
					s += tmp[v] * twiddles[1][j*(cheight-1)+v-1];
				s /= width*height;
				s *= 255;
				icoeffs[(j*vw+i)*3+z] = s>255?255:s<0?0:round(s);
			}
		}
	}

	if(showsamples==1)
		for(size_t y = scale-(size_t)vy%(int)scale; y < vh; y+=scale)
			for(size_t x = scale-(size_t)vx%(int)scale; x < vw; x+=scale)
				memcpy(icoeffs+(y*vh+x)*3,((unsigned char[]){0,255,0}),3);
	else if(showsamples==2) {
		for(size_t y = scale-(size_t)vy%(int)scale; y < vh; y+=scale)
			for(size_t x = 0; x < vw; x++)
				memcpy(icoeffs+(y*vh+x)*3,((unsigned char[]){0,255,0}),3);
		for(size_t y = 0; y < vh; y++)
			for(size_t x = scale-(size_t)vx%(int)scale; x < vw; x+=scale)
				memcpy(icoeffs+(y*vh+x)*3,((unsigned char[]){0,255,0}),3);
	}

	MagickSetImageExtent(wand,vw,vh);
	MagickImportImagePixels(wand,0,0,vw,vh,"RGB",CharPixel,icoeffs);
	MagickWriteImage(wand,outfile);

	free(coeffs);
	free(icoeffs);
	free(tmp);
	free(twiddles[0]);
	free(twiddles[1]);

	DestroyMagickWand(wand);
	MagickWandTerminus();

	return 0;
}
