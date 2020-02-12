/*-
 * Copyright (c) 2009 Panda Papanda <papanda@papa.to>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#define CONVERT2UTF8
#include <stdio.h>
#include <string.h>
#ifndef OS_WIN32
#include <iconv.h>
#endif

static int STRMINLEN = 1024;

static unsigned char JIS_KANJI_IN[] = { 0x1B, 0x24, 0x42 };
static unsigned char JIS_KANJI_OUT[] = { 0x1B, 0x28, 0x42 };
static unsigned char GSET_KANJI = 0x42;
static unsigned char GSET_ASCII = 0x4A;
static unsigned char GSET_HIRAGANA = 0x30;
static unsigned char GSET_KATAKANA = 0x31;
static unsigned char GSET_MOSAIC_A = 0x32;
static unsigned char GSET_MOSAIC_B = 0x33;
static unsigned char GSET_MOSAIC_C = 0x34;
static unsigned char GSET_MOSAIC_D = 0x35;
static unsigned char GSET_PRO_ASCII = 0x36;
static unsigned char GSET_PRO_HIRAGANA = 0x37;
static unsigned char GSET_PRO_KATAKANA = 0x38;
static unsigned char GSET_JIS_X0201_KATAKANA = 0x49;
static unsigned char GSET_JIS_KANJI1 = 0x39;
static unsigned char GSET_JIS_KANJI2 = 0x3A;
static unsigned char GSET_GAIJI = 0x3B;
static unsigned char APD[2] = "\\n";
static unsigned char APR[2] = "\\r";
static unsigned char APF[2] = "\\t";
static unsigned char BEL[2] = "\\a";

static int GSET_BYTES(unsigned char mode)
{
  switch (mode) {
  case 0x42:
  case 0x39:
  case 0x3A:
  case 0x3B:
    return 2;
  case 0x4A:
  case 0x30:
  case 0x31:
  case 0x32:
  case 0x34:
  case 0x35:
  case 0x36:
  case 0x37:
  case 0x38:
  case 0x49:
    return 1;
  }
  return 0;
}

static unsigned short PRO_ASCII[] = {
	0x2121, 0x212a, 0x2149, 0x2174, 0x2170, 0x2173, 0x2175, 0x2147,
	0x214a, 0x214b, 0x2176, 0x215c, 0x2124, 0x215d, 0x2125, 0x213f,
	0x2330, 0x2331, 0x2332, 0x2333, 0x2334, 0x2335, 0x2336, 0x2337,
	0x2338, 0x2339, 0x2127, 0x2128, 0x2163, 0x2161, 0x2164, 0x2129,
	0x2177, 0x2341, 0x2342, 0x2343, 0x2344, 0x2345, 0x2346, 0x2347,
	0x2348, 0x2349, 0x234a, 0x234b, 0x234c, 0x234d, 0x234e, 0x234f,
	0x2350, 0x2351, 0x2352, 0x2353, 0x2354, 0x2355, 0x2356, 0x2357,
	0x2358, 0x2359, 0x235a, 0x214e, 0x216f, 0x214f, 0x2130, 0x2132,
	0x212e, 0x2361, 0x2362, 0x2363, 0x2364, 0x2365, 0x2366, 0x2367,
	0x2368, 0x2369, 0x236a, 0x236b, 0x236c, 0x236d, 0x236e, 0x236f,
	0x2370, 0x2371, 0x2372, 0x2373, 0x2374, 0x2375, 0x2376, 0x2377,
	0x2378, 0x2379, 0x237a, 0x2150, 0x2143, 0x2151, 0x2131
}; 

static unsigned short HIRAGANA_EX[] = { 0x2135, 0x2136, 0x213C, 0x2123, 0x2156, 0x2157, 0x2122, 0x2126 };
static unsigned short KATAKANA_EX[] = { 0x2133, 0x2134, 0x213C, 0x2123, 0x2156, 0x2157, 0x2122, 0x2126 };

typedef struct binaryatom *Binary;

typedef struct binaryatom {
  unsigned char *buf_;
  int buflen_;
  int pos_;
  int max_;
} BinaryAtom;

static Binary NewBinary(int max)
{
  Binary temp;

  temp = (Binary)malloc(sizeof(BinaryAtom));
  if (temp == NULL) {
    return NULL;
  }
  temp->max_ = (max > STRMINLEN) ? max : STRMINLEN;

  temp->buf_ = (unsigned char *)malloc(temp->max_);
  if (temp->buf_ == NULL) {
    free(temp);
    return NULL;
  }
  memset(temp->buf_, 0, temp->max_);
  temp->buflen_ = 0;
  temp->pos_ = 0;
  return temp;

}

static unsigned char GetIntBinary(Binary obj)
{
  unsigned char retval;

  if (obj == NULL || obj->buf_ == NULL || obj->pos_ >= obj->buflen_) return 0x00;
  retval = obj->buf_[obj->pos_];
  (obj->pos_)++;
  return retval;
}

static int ResetBinary(Binary obj) {
  if (obj == NULL) return -1;

  obj->buflen_ = 0;
  obj->pos_ = 0;

  return 0;
}

static int PutBinary(Binary obj, unsigned char *add, int addlen)
{
  if (obj == NULL || obj->buf_ == NULL || add == NULL || addlen < 1 || (obj->buflen_ + addlen) >= obj->max_) return -1;
  memcpy(obj->buf_ + obj->buflen_, add, addlen);
  obj->buflen_ += addlen;
  *(obj->buf_ + obj->buflen_) = '\0';
  return 0;
}

static int PutShortBinary(Binary obj, unsigned short c)
{
  if (obj == NULL || obj->buf_ == NULL || (obj->buflen_ + sizeof(unsigned short)) >= obj->max_) return -1;

  *(obj->buf_ + obj->buflen_) = (unsigned char)(c >> 8);
  *(obj->buf_ + obj->buflen_ + 1) = (unsigned char)(c & 0xff);
  obj->buflen_ += sizeof(unsigned short);
  return 0;
}

static void DeleteBinary(Binary obj)
{
  if (obj != NULL) {
    if (obj->buf_ != NULL) {
      free(obj->buf_);
      obj->buf_ = NULL;
    }
    free(obj);
  }
}

typedef struct aribstringatom *ARIBString;

typedef struct aribstringatom {
  Binary buf_;
  Binary put_;
  Binary jis_;
  Binary tmp_;
#ifdef OS_WIN32
  int cd_;
#else
  iconv_t cd_;
#endif
  int kanji_;
  int max_;
} ARIBStringAtom;

static ARIBString NewARIBString(int maxbytes)
{
  ARIBString temp;

  temp = (ARIBString)malloc(sizeof(ARIBStringAtom));
  if (temp == NULL) {
    return NULL;
  }
  if (maxbytes <= STRMINLEN) {
    maxbytes = STRMINLEN;
  }
#ifdef OS_WIN32  
  temp->cd_ = -1;
#else
  temp->cd_ = iconv_open("UTF-8", "ISO-2022-JP");
#endif
  temp->kanji_ = 0;
  temp->max_ = maxbytes;
  temp->buf_ = NewBinary(temp->max_);
  if (temp->buf_ == NULL) {
    free(temp);
    return NULL;
  }
  
  temp->put_ = NewBinary(temp->max_);
  if (temp->put_ == NULL) {
    DeleteBinary(temp->buf_);
    free(temp);
    return NULL;
  }

  temp->jis_ = NewBinary(temp->max_);
  if (temp->jis_ == NULL) {
    DeleteBinary(temp->put_);
    DeleteBinary(temp->buf_);
    free(temp);
    return NULL;
  }

  temp->tmp_ = NewBinary(temp->max_);
  if (temp->tmp_ == NULL) {
    DeleteBinary(temp->jis_);
    DeleteBinary(temp->put_);
    DeleteBinary(temp->buf_);
    free(temp);
    return NULL;
  }

  return temp;
}

int SetKanjiARIBString(ARIBString obj, unsigned short c)
{
  if (obj == NULL) return -1;

  if (obj->kanji_ == 0) {
    ResetBinary(obj->jis_);
#if !defined(OS_WIN32) && defined(CONVERT2UTF8)
    PutBinary(obj->jis_, JIS_KANJI_IN, sizeof(JIS_KANJI_IN));
#endif
    obj->kanji_ = 1;
  }
  PutShortBinary(obj->jis_, c);
  return 0;
}

int EndKanjiARIBString(ARIBString obj)
{
  if (obj == NULL) return -1;

  if (obj->kanji_ == 1) {
#if defined(CONVERT2UTF8)
    // JIS to UTF8 used iconv.
    PutBinary(obj->jis_, JIS_KANJI_OUT, sizeof(JIS_KANJI_OUT));
    {
      char *inbuf;
      char *out;
      char *outbuf;
      size_t inbuflen;
      size_t outbuflen;
      
      inbuf = obj->jis_->buf_;
      inbuflen = obj->jis_->buflen_;
      outbuflen = obj->max_;
      out = outbuf = obj->tmp_->buf_;
      if (iconv(obj->cd_, &inbuf, &inbuflen, &outbuf, &outbuflen) != (size_t)-1) {
	unsigned char *nyoro = NULL;
	unsigned char nyoro1[] = { 0xe3, 0x80, 0x9c, 0x00 };
	unsigned char nyoro2[] = { 0xef, 0xbd, 0x9e, 0x00 };

	if ((nyoro = strstr(out, nyoro1)) != NULL) {
	  memcpy(nyoro, nyoro2, 3);
	}
	PutBinary(obj->put_, out, outbuf - out);
      }
    }
#else
    {
      int i;
      // JIS to SJIS
      for (i = 0; i < obj->jis_->buflen_; i += 2) {
	unsigned char c1, c2;

	c1 = (obj->jis_->buf_)[i];
	c2 = (obj->jis_->buf_)[i + 1];
	if ((c1 % 2) == 1) {
	  c1 = ((c1 + 1) / 2) + 0x70;
	  c2 = c2 + 0x1f;
	} else {
	  c1 = (c1 / 2) + 0x70;
	  c2 += 0x7d;
	}
	if (c1 >= 0xa0) { c1 += 0x40; }
	if (c2 >= 0x7f) { c2 += 1; }
	(obj->tmp_->buf_)[i] = c1;
	(obj->tmp_->buf_)[i + 1] = c2;
      }
    }
    PutBinary(obj->put_, obj->tmp_->buf_, obj->jis_->buflen_);
#endif
    obj->kanji_ = 0;
  }
  return 0;
}

int SetStringARIBString(ARIBString obj, unsigned char *str)
{
  if (obj == NULL || str == NULL) return -1;
  EndKanjiARIBString(obj);
  PutBinary(obj->put_, str, strlen(str));
  return 0;
}

int SetAsciiARIBString(ARIBString obj, unsigned short c)
{
  unsigned char buf[1];
  
  if (obj == NULL) return -1;
  buf[0] = (unsigned char)(c & 0xff);
  PutBinary(obj->put_, buf, 1);
  return 0;
}

int SetCTRLARIBString(ARIBString obj, unsigned short c)
{
  // ARIB STD-B24 part1.volume2. APPDX. Table.7-14
  if (obj == NULL) return -1;
  EndKanjiARIBString(obj);
  if (c == 0x07) {
    // BEL
    PutBinary(obj->put_, BEL, 2);
  } else if (c == 0x09) {
    // APF
    PutBinary(obj->put_, APF, 2);
  } else if (c == 0x0a) {
    // APD
    PutBinary(obj->put_, APD, 2);
  } else if (c == 0x0d) {
    // APR
    PutBinary(obj->put_, APR, 2);
  }
  return 0;
}

int GmapARIBString(ARIBString obj, unsigned char gset, unsigned short c)
{
  if (obj == NULL) return -1;

  if (gset == GSET_JIS_KANJI1) {
    SetKanjiARIBString(obj, c);
  } else if (gset == GSET_ASCII) {
    SetAsciiARIBString(obj, c);
  } else if (gset == GSET_PRO_ASCII) {
    SetKanjiARIBString(obj, PRO_ASCII[c - 0x20]);
  } else if (gset == GSET_HIRAGANA || gset == GSET_PRO_HIRAGANA) {
    if (c < 0x77) {
      SetKanjiARIBString(obj, c + 0x2400);
    } else {
      SetKanjiARIBString(obj, HIRAGANA_EX[c - 0x77]);
    }
  } else if (gset == GSET_KATAKANA || gset == GSET_PRO_KATAKANA) {
    if (c < 0x77) {
      SetKanjiARIBString(obj, c + 0x2500);
    } else {
      SetKanjiARIBString(obj, KATAKANA_EX[c - 0x77]);
    }
  } else if (gset == GSET_GAIJI) {
    unsigned char ku = (c >> 8) - 0x20;
    unsigned char ten = (c & 0xFF) - 0x20;
    unsigned char unkbuf[12]; /* magic */

    // see ARIB-STD-B24 part1.volume1. Table.7-10 
    // [hoka] is c==0x7a74, ku=90, ten=84
    sprintf(unkbuf, "[UNK%02d%02d]", ku, ten);
    SetStringARIBString(obj, unkbuf);
  } else {
    // error.
    return -1;
  }
  return 0;
}

void DeleteARIBString(ARIBString obj)
{
  if (obj != NULL) {
#ifndef OS_WIN32
    if (obj->cd_ != (iconv_t)-1) {
      iconv_close(obj->cd_);
    }
#endif
    DeleteBinary(obj->tmp_);
    DeleteBinary(obj->jis_);
    DeleteBinary(obj->put_);
    DeleteBinary(obj->buf_);
    free(obj);
  }
}

int ParseARIBString(ARIBString obj, unsigned char *buf, int buflen)
{
  int g[4];
  int gl = 0;
  int gr = 2;
  int ss = 0;

  if (obj == NULL || buf == NULL || buflen >= (obj->max_ / 2)) {
    return -1;
  }
  ResetBinary(obj->buf_);
  ResetBinary(obj->put_);
  ResetBinary(obj->jis_);
  if (PutBinary(obj->buf_, buf, buflen) != 0) {
    return -2;
  }
  
  // ARIB TR-B14 part1.volume2. 4.2 Table.4-8
  g[0] = GSET_JIS_KANJI1;
  g[1] = GSET_PRO_ASCII;  
  g[2] = GSET_HIRAGANA;
  g[3] = GSET_KATAKANA;

  while (obj->buf_->pos_ < obj->buf_->buflen_) {
    unsigned short c = GetIntBinary(obj->buf_);
    
    // ARIB TR-B14 part1.volume2. 4.2 Table.4-3
    if (c == 0x0F) {
      // LS0
      gl = 0;
    } else if (c == 0x0E) {
      // LS1
      gl = 1;
    } else if (c == 0x1B) {
      unsigned char d = GetIntBinary(obj->buf_);
      // ESCin.
      if (d == 0x6E) {
	// LS2
	gl = 2;
      } else if (d == 0x6F) {
	// LS3
	gl = 3;
      } else if (d == 0x7E) {
	// LS1R
	gr = 1;
      } else if (d == 0x7D) {
	// LS2R
	gr = 2;
      } else if (d == 0x7C) {
	// LS3R
	gr = 3;
      } else if (d == 0x24) {
	unsigned short e = GetIntBinary(obj->buf_);
	// ARIB TR-B14 part1.volume2. 4.2 Table.4-4.
	if (e == 0x29) {
	  // ESC 02/4 02/9F
	  g[1] = GetIntBinary(obj->buf_);
	} else if (e == 0x2A) {
	  // ESC 02/4 02/10F
	  g[2] = GetIntBinary(obj->buf_);
	} else if (e == 0x2B) {
	  // ESC 02/4 02/11F
	  g[3] = GetIntBinary(obj->buf_);
	} else {
	  // ESC 02/4F
	  g[0] = e;
	}
      } else if (d == 0x28) {
	// ARIB TR-B14 part1.volume2. 4.2 Table.4-4. 
	// ESC 02/8F
	g[0] = GetIntBinary(obj->buf_);
      } else if (d == 0x29) {
	// ESC 02/9F
	g[1] = GetIntBinary(obj->buf_);
      } else if (d == 0x2A) {
	// ESC 02/10F
	g[2] = GetIntBinary(obj->buf_);
      } else if (d == 0x2B) {
	// ESC 02/11F
	g[3] = GetIntBinary(obj->buf_);
      } else {
	/* not supported esc */
      }
    } else if (c == 0x19) {
      // SS2
      ss = 2;
    } else if (c == 0x1D) {
      // SS3
      ss = 3;
    } else {
      // other
      if (c < 0x20) {
	// CTRL
	SetCTRLARIBString(obj, c);	
      } else if (c == 0x20) {
	// SP
	GmapARIBString(obj, GSET_PRO_ASCII, c);
      } else if (0x20 < c && c < 0x7F) {
	int gt = gl;
	int gset = 0;
	int bytes = 0;

	if (ss > 0) {
	  gt = ss;
	  ss = 0;
	}
	gset = g[gt];
	bytes = GSET_BYTES(gset);
	if (bytes == 2) {
	  unsigned char d = GetIntBinary(obj->buf_);
	  c = (c << 8) | d;
	}
	GmapARIBString(obj, gset, c);
      } else if (c == 0x7F) {
	// DEL
      } else if (0x7F < c && c < 0xA0) {
	// C1 
	SetCTRLARIBString(obj, c);	
      } else if (0xA0 < c) {
	// GR
	int gset = g[gr];
	int bytes = GSET_BYTES(gset);
	
	if (bytes == 2) {
	  unsigned char d = GetIntBinary(obj->buf_);
	  c = (c << 8) | d;
	}
	c &= 0x7F7F;
	GmapARIBString(obj, gset, c);
      }
    }
  }
  EndKanjiARIBString(obj);
  return 0;
}

int text_decode(unsigned char *buf, int len, FILE *wfp)
{
  int retval = 0;

  ARIBString obj = NewARIBString(2050); /* magic */

  if (ParseARIBString(obj, buf, len) == 0) {
    fwrite(obj->put_->buf_, 1, obj->put_->buflen_, wfp);
    retval = 0;
  } else {
    retval = -1;
  }
  DeleteARIBString(obj);
  return retval;
}

int text_decode2(unsigned char *buf, int len, unsigned char *obuf, int *olen)
{
  int retval = 0;

  ARIBString obj = NewARIBString(2050); /* magic */

  if (ParseARIBString(obj, buf, len) == 0) {
    memcpy(obuf, obj->put_->buf_, obj->put_->buflen_);
    obuf[obj->put_->buflen_] = '\0';
    *olen = obj->put_->buflen_;
    retval = 0;
  } else {
    retval = -1;
  }
  DeleteARIBString(obj);
  return retval;
}
