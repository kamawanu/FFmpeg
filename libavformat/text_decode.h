/**
	@file text_decode.h
	@author papanda@papa.to
*/
#ifndef __text_decode_h__
#define __text_decode_h__

int text_decode(unsigned char *buf, int len, FILE *wfp);
int text_decode2(unsigned char *buf, int len, unsigned char *obuf, int *olen);

#endif
