//========================================================================
//
// OPVPOutputDev.cc
// 	Based SplashOutputDev.cc
//
// Copyright 2005 AXE,Inc.
//
// 2007 Modified by BBR Inc.
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include <math.h>
#include "gfile.h"
#include "GlobalParams.h"
#include "Error.h"
#include "Object.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "Link.h"
#include "CharCodeToUnicode.h"
#include "FontEncodingTables.h"
#include "FoFiTrueType.h"
#include "SplashMath.h"
#include "CMap.h"
#include "SplashBitmap.h"
#include "SplashGlyphBitmap.h"
#include "SplashPattern.h"
#include "SplashScreen.h"
#include "SplashPath.h"
#include "SplashState.h"
#include "SplashErrorCodes.h"
#include "SplashFontEngine.h"
#include "SplashFont.h"
#include "SplashFontFile.h"
#include "SplashFontFileID.h"
#include "OPRS.h"
#include "OPVPOutputDev.h"

#define SLICE_FOR_PATTERN 1000

//------------------------------------------------------------------------
// Font substitutions
//------------------------------------------------------------------------

struct SplashOutFontSubst {
  char *name;
  double mWidth;
};

// index: {symbolic:12, fixed:8, serif:4, sans-serif:0} + bold*2 + italic
static SplashOutFontSubst splashOutSubstFonts[16] = {
  {"Helvetica",             0.833},
  {"Helvetica-Oblique",     0.833},
  {"Helvetica-Bold",        0.889},
  {"Helvetica-BoldOblique", 0.889},
  {"Times-Roman",           0.788},
  {"Times-Italic",          0.722},
  {"Times-Bold",            0.833},
  {"Times-BoldItalic",      0.778},
  {"Courier",               0.600},
  {"Courier-Oblique",       0.600},
  {"Courier-Bold",          0.600},
  {"Courier-BoldOblique",   0.600},
  {"Symbol",                0.576},
  {"Symbol",                0.576},
  {"Symbol",                0.576},
  {"Symbol",                0.576}
};

//------------------------------------------------------------------------

#define soutRound(x) ((int)(x + 0.5))

//------------------------------------------------------------------------
// SplashOutFontFileID
//------------------------------------------------------------------------

class SplashOutFontFileID: public SplashFontFileID {
public:

  SplashOutFontFileID(Ref *rA) { r = *rA; substIdx = -1; }

  ~SplashOutFontFileID() {}

  GBool matches(SplashFontFileID *id) {
    return ((SplashOutFontFileID *)id)->r.num == r.num &&
           ((SplashOutFontFileID *)id)->r.gen == r.gen;
  }

  void setSubstIdx(int substIdxA) { substIdx = substIdxA; }
  int getSubstIdx() { return substIdx; }

private:

  Ref r;
  int substIdx;
};

//------------------------------------------------------------------------
// T3FontCache
//------------------------------------------------------------------------

struct T3FontCacheTag {
  Gushort code;
  Gushort mru;			// valid bit (0x8000) and MRU index
};

class T3FontCache {
public:

  T3FontCache(Ref *fontID, double m11A, double m12A,
	      double m21A, double m22A,
	      int glyphXA, int glyphYA, int glyphWA, int glyphHA,
	      GBool aa);
  ~T3FontCache();
  GBool matches(Ref *idA, double m11A, double m12A,
		double m21A, double m22A)
    { return fontID.num == idA->num && fontID.gen == idA->gen &&
	     m11 == m11A && m12 == m12A && m21 == m21A && m22 == m22A; }

  Ref fontID;			// PDF font ID
  double m11, m12, m21, m22;	// transform matrix
  int glyphX, glyphY;		// pixel offset of glyph bitmaps
  int glyphW, glyphH;		// size of glyph bitmaps, in pixels
  int glyphSize;		// size of glyph bitmaps, in bytes
  int cacheSets;		// number of sets in cache
  int cacheAssoc;		// cache associativity (glyphs per set)
  Guchar *cacheData;		// glyph pixmap cache
  T3FontCacheTag *cacheTags;	// cache tags, i.e., char codes
};

T3FontCache::T3FontCache(Ref *fontIDA, double m11A, double m12A,
			 double m21A, double m22A,
			 int glyphXA, int glyphYA, int glyphWA, int glyphHA,
			 GBool aa) {
  int i;

  fontID = *fontIDA;
  m11 = m11A;
  m12 = m12A;
  m21 = m21A;
  m22 = m22A;
  glyphX = glyphXA;
  glyphY = glyphYA;
  glyphW = glyphWA;
  glyphH = glyphHA;
  if (aa) {
    glyphSize = glyphW * glyphH;
  } else {
    glyphSize = ((glyphW + 7) >> 3) * glyphH;
  }
  cacheAssoc = 8;
  if (glyphSize <= 256) {
    cacheSets = 8;
  } else if (glyphSize <= 512) {
    cacheSets = 4;
  } else if (glyphSize <= 1024) {
    cacheSets = 2;
  } else {
    cacheSets = 1;
  }
  cacheData = (Guchar *)gmalloc(cacheSets * cacheAssoc * glyphSize);
  cacheTags = (T3FontCacheTag *)gmalloc(cacheSets * cacheAssoc *
					sizeof(T3FontCacheTag));
  for (i = 0; i < cacheSets * cacheAssoc; ++i) {
    cacheTags[i].mru = i & (cacheAssoc - 1);
  }
}

T3FontCache::~T3FontCache() {
  gfree(cacheData);
  gfree(cacheTags);
}

struct T3GlyphStack {
  Gushort code;			// character code
  double x, y;			// position to draw the glyph

  //----- cache info
  T3FontCache *cache;		// font cache for the current font
  T3FontCacheTag *cacheTag;	// pointer to cache tag for the glyph
  Guchar *cacheData;		// pointer to cache data for the glyph

  //----- saved state
  SplashBitmap *origBitmap;
  OPRS *origOPRS;
  double origCTM4, origCTM5;

  T3GlyphStack *next;		// next object on stack
};

//------------------------------------------------------------------------
// OPVPOutputDev
//------------------------------------------------------------------------

OPVPOutputDev::OPVPOutputDev()
{
  xref = 0;
  bitmap = 0;
  fontEngine = 0;
  nT3Fonts = 0;
  t3GlyphStack = 0;
  font = NULL;
  needFontUpdate = gFalse;
  textClipPath = 0;
  underlayCbk = 0;
  underlayCbkData = 0;
  scaleWidth = scaleHeight = -1;
  leftMargin = 0;
  bottomMargin = 0;
  rotate = 0;
  sliceHeight = 0;
  yoffset = 0;
  oprs = 0;
}

void OPVPOutputDev::setScale(double w, double h,
  double leftMarginA, double bottomMarginA, int rotateA,
  int yoffsetA, int sliceHeightA)
{
  scaleWidth = w;
  scaleHeight = h;
  leftMargin = leftMarginA;
  bottomMargin = bottomMarginA;
  rotate = rotateA;
  yoffset = yoffsetA;
  sliceHeight = sliceHeightA;
}

int OPVPOutputDev::init(SplashColorMode colorModeA,
				 GBool reverseVideoA,
				 SplashColor paperColorA,
				 GBool rasterModeA,
                                 const char *driverName,
				 int outputFD,
				 char *printerModel,
				 int paperWidthA, int paperHeightA,
				 int nOptions,
				 char *optionKeys[],
				 char *optionVals[]) {
  int result;

  oprs = new OPRS();

  if ((result = oprs->init(driverName, outputFD, printerModel,
       rasterModeA,nOptions,optionKeys,optionVals)) < 0) {
    error(-1,"OPRS initialization fail");
    return result;
  }
  colorMode = colorModeA;
  if ((result = oprs->setColorMode(colorMode)) < 0) {
    error(-1,"Can't setColorMode");
    return result;
  }
  reverseVideo = reverseVideoA;
  splashColorCopy(paperColor,paperColorA);
  rasterMode = oprs->getRasterMode();
  paperWidth = paperWidthA;
  paperHeight = paperHeightA;

  return 0;
}

OPVPOutputDev::~OPVPOutputDev() {
  int i;

  for (i = 0; i < nT3Fonts; ++i) {
    delete t3FontCache[i];
  }
  if (fontEngine) {
    delete fontEngine;
  }
  if (oprs) {
    delete oprs;
  }
  if (bitmap) {
    delete bitmap;
  }
}

void OPVPOutputDev::startDoc(XRef *xrefA) {
  int i;

  xref = xrefA;
  if (fontEngine) {
    delete fontEngine;
  }
  fontEngine = new SplashFontEngine(
#if HAVE_T1LIB_H
				    globalParams->getEnableT1lib(),
#endif
#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
				    globalParams->getEnableFreeType(),
#endif
				    globalParams->getAntialias());
  for (i = 0; i < nT3Fonts; ++i) {
    delete t3FontCache[i];
  }
  nT3Fonts = 0;
}

void OPVPOutputDev::startPage(int pageNum, GfxState *state) {
  int w, h;

  if (state) {
    if (scaleWidth > 0 && scaleHeight > 0) {
      double *ctm = state->getCTM();

      switch (rotate) {
      case 90:
	state->setCTM(0,ctm[1],ctm[2],0,leftMargin,bottomMargin-yoffset);
	break;
      case 180:
	state->setCTM(ctm[0],0,0,ctm[3],paperWidth-leftMargin,
	  bottomMargin-yoffset);
	break;
      case 270:
	state->setCTM(0,ctm[1],ctm[2],0,paperWidth-leftMargin,
	  -bottomMargin+paperHeight-yoffset);
	break;
      default:
	state->setCTM(ctm[0],0,0,ctm[3],leftMargin,
	  -bottomMargin+paperHeight-yoffset);
	break;
      }
      state->concatCTM(scaleWidth,0.0,0.0,scaleHeight,0,0);
    }
    w = (int)(state->getPageWidth()+0.5);
    h = (int)(state->getPageHeight()+0.5);
  } else {
    w = h = 1;
  }
  if (rasterMode) {
      if (!bitmap || w != bitmap->getWidth() || h != bitmap->getHeight()) {
	if (bitmap) {
	  delete bitmap;
	}
	bitmap = new SplashBitmap(w, h, 4, colorMode);
      }
      if (oprs->setBitmap(bitmap) < 0) {
	error(-1,"SetBitmap fail");
	return;
      }
  }
  oprs->initGS(colorMode,w,h,paperColor);

  if (underlayCbk) {
    (*underlayCbk)(underlayCbkData);
  }
}

void OPVPOutputDev::endPage() {
  oprs->endPage();
}

void OPVPOutputDev::drawLink(Link *link, Catalog *catalog) {
}

void OPVPOutputDev::saveState(GfxState *state) {
  oprs->saveState();
}

void OPVPOutputDev::restoreState(GfxState *state) {
  oprs->restoreState();
  needFontUpdate = gTrue;
}

void OPVPOutputDev::updateAll(GfxState *state) {
  updateLineDash(state);
  updateLineJoin(state);
  updateLineCap(state);
  updateLineWidth(state);
  updateFlatness(state);
  updateMiterLimit(state);
  updateFillColor(state);
  updateStrokeColor(state);
  needFontUpdate = gTrue;
}

void OPVPOutputDev::updateCTM(GfxState *state, double m11, double m12,
				double m21, double m22,
				double m31, double m32) {
  updateLineDash(state);
  updateLineJoin(state);
  updateLineCap(state);
  updateLineWidth(state);
}

void OPVPOutputDev::updateLineDash(GfxState *state) {
  double *dashPattern;
  int dashLength;
  double dashStart;
  SplashCoord dash[20];
  SplashCoord phase;
  int i;

  state->getLineDash(&dashPattern, &dashLength, &dashStart);
  if (dashLength > 20) {
    dashLength = 20;
  }
  for (i = 0; i < dashLength; ++i) {
    dash[i] =  (SplashCoord)state->transformWidth(dashPattern[i]);
    if (dash[i] < 1) {
      dash[i] = 1;
    }
  }
  phase = (SplashCoord)state->transformWidth(dashStart);
  oprs->setLineDash(dash, dashLength, phase);
}

void OPVPOutputDev::updateFlatness(GfxState *state) {
  oprs->setFlatness(state->getFlatness());
}

void OPVPOutputDev::updateLineJoin(GfxState *state) {
  oprs->setLineJoin(state->getLineJoin());
}

void OPVPOutputDev::updateLineCap(GfxState *state) {
  oprs->setLineCap(state->getLineCap());
}

void OPVPOutputDev::updateMiterLimit(GfxState *state) {
  oprs->setMiterLimit(state->getMiterLimit());
}

void OPVPOutputDev::updateLineWidth(GfxState *state) {
  oprs->setLineWidth(state->getTransformedLineWidth());
}

void OPVPOutputDev::updateFillColor(GfxState *state) {
  GfxGray gray;
  GfxRGB rgb;

  state->getFillGray(&gray);
  state->getFillRGB(&rgb);
  oprs->setFillPattern(getColor(gray, &rgb));
}

void OPVPOutputDev::updateStrokeColor(GfxState *state) {
  GfxGray gray;
  GfxRGB rgb;

  state->getStrokeGray(&gray);
  state->getStrokeRGB(&rgb);
  oprs->setStrokePattern(getColor(gray, &rgb));
}

#ifdef SPLASH_CMYK
SplashPattern *OPVPOutputDev::getColor(double gray, GfxRGB *rgb,
     GfxCMYK *cmyk) {
#else
SplashPattern *OPVPOutputDev::getColor(GfxGray gray, GfxRGB *rgb) {
#endif
  SplashPattern *pattern;
  SplashColor color0, color1;
  GfxColorComp r, g, b;

  if (reverseVideo) {
    gray = gfxColorComp1 - gray;
    r = gfxColorComp1 - rgb->r;
    g = gfxColorComp1 - rgb->g;
    b = gfxColorComp1 - rgb->b;
  } else {
    r = rgb->r;
    g = rgb->g;
    b = rgb->b;
  }

  pattern = NULL; // make gcc happy
  switch (colorMode) {
  case splashModeMono1:
    color0[0] = 0;
    color1[0] = 1;
    pattern = new SplashHalftone(color0, color1,
				 oprs->getScreen()->copy(),
				 (SplashCoord)colToDbl(gray));
    break;
  case splashModeMono8:
    color1[0] = colToByte(gray);
    pattern = new SplashSolidColor(color1);
    break;
  case splashModeRGB8:
    color1[0] = colToByte(r);
    color1[1] = colToByte(g);
    color1[2] = colToByte(b);
    pattern = new SplashSolidColor(color1);
    break;
  default:
    error(-1, "no supported color mode");
    break;
  }

  return pattern;
}

void OPVPOutputDev::updateFont(GfxState *state) {
  GfxFont *gfxFont;
  GfxFontType fontType;
  SplashOutFontFileID *id;
  SplashFontFile *fontFile;
  SplashFontSrc *fontsrc = NULL;
  FoFiTrueType *ff;
  Ref embRef;
  Object refObj, strObj;
  GooString *fileName, *substName;
  char *tmpBuf;
  int tmpBufLen;
  Gushort *codeToGID;
  DisplayFontParam *dfp;
  CharCodeToUnicode *ctu;
  double m11, m12, m21, m22, w1, w2;
  SplashCoord mat[4];
  char *name;
  Unicode uBuf[8];
  int c, substIdx, n, code, cmap;
  int faceIndex = 0;

  needFontUpdate = gFalse;
  font = NULL;
  fileName = NULL;
  tmpBuf = NULL;
  substIdx = -1;
  dfp = NULL;

  if (!(gfxFont = state->getFont())) {
    goto err1;
  }
  fontType = gfxFont->getType();
  if (fontType == fontType3) {
    goto err1;
  }

  // check the font file cache
  id = new SplashOutFontFileID(gfxFont->getID());
  if ((fontFile = fontEngine->getFontFile(id))) {
    delete id;
  } else {
    // if there is an embedded font, write it to disk
    if (gfxFont->getEmbeddedFontID(&embRef)) {
      tmpBuf = gfxFont->readEmbFontFile(xref, &tmpBufLen);
      if (! tmpBuf)
	goto err2;
    // if there is an external font file, use it
    } else if (!(fileName = gfxFont->getExtFontFile())) {
      // look for a display font mapping or a substitute font
      dfp = NULL;
      if (gfxFont->getName()) {
        dfp = globalParams->getDisplayFont(gfxFont);
      }
      if (!dfp) {
	error(-1, "Couldn't find a font for '%s'",
	      gfxFont->getName() ? gfxFont->getName()->getCString()
	                         : "(unnamed)");
	goto err2;
      }
      switch (dfp->kind) {
      case displayFontT1:
	fileName = dfp->t1.fileName;
	fontType = gfxFont->isCIDFont() ? fontCIDType0 : fontType1;
	break;
      case displayFontTT:
	fileName = dfp->tt.fileName;
	fontType = gfxFont->isCIDFont() ? fontCIDType2 : fontTrueType;
	faceIndex = dfp->tt.faceIndex;
	break;
      }
      if (fileName != 0) {
	fprintf(stderr,"INFO: Using font file %s faceIndex=%d for %s\n",
	    fileName->getCString(), faceIndex,
	    gfxFont->getName() ? gfxFont->getName()->getCString()
					     : "(unnamed)");
      }
    }

    fontsrc = new SplashFontSrc;
    if (fileName)
      fontsrc->setFile(fileName, gFalse);
    else
      fontsrc->setBuf(tmpBuf, tmpBufLen, gTrue);

    // load the font file
    switch (fontType) {
    case fontType1:
      fontFile = fontEngine->loadType1Font(id, fontsrc, 
				     ((Gfx8BitFont *)gfxFont)->getEncoding());
      if (! fontFile) {
	error(-1, "Couldn't create a font for '%s'",
	      gfxFont->getName() ? gfxFont->getName()->getCString()
	                         : "(unnamed)");
	goto err2;
      }
      break;
    case fontType1C:
      fontFile = fontEngine->loadType1CFont(id, fontsrc,
				      ((Gfx8BitFont *)gfxFont)->getEncoding());
      if (! fontFile) {
	error(-1, "Couldn't create a font for '%s'",
	      gfxFont->getName() ? gfxFont->getName()->getCString()
	                         : "(unnamed)");
	goto err2;
      }
      break;
    case fontTrueType:
      if (fileName)
	ff = FoFiTrueType::load(fileName->getCString());
      else
	ff = new FoFiTrueType(tmpBuf, tmpBufLen, gFalse);
      if (ff) {
	codeToGID = ((Gfx8BitFont *)gfxFont)->getCodeToGIDMap(ff);
	n = 256;
	delete ff;
      } else {
	codeToGID = NULL;
	n = 0;
      }
      if (!(fontFile = fontEngine->loadTrueTypeFont(
			 id,
			 fontsrc,
			 codeToGID, n))) {
	error(-1, "Couldn't create a font for '%s'",
	    gfxFont->getName() ? gfxFont->getName()->getCString()
			       : "(unnamed)");
	goto err2;
      }
      break;
    case fontCIDType0:
    case fontCIDType0C:
      fontFile = fontEngine->loadCIDFont(id, fontsrc);
      if (! fontFile) {
	error(-1, "Couldn't create a font for '%s'",
	      gfxFont->getName() ? gfxFont->getName()->getCString()
	                         : "(unnamed)");
	goto err2;
      }
      break;
    case fontCIDType2:
      codeToGID = NULL;
      n = 0;
      if (((GfxCIDFont *)gfxFont)->getCIDToGID()) {
	n = ((GfxCIDFont *)gfxFont)->getCIDToGIDLen();
	if (n) {
	  codeToGID = (Gushort *)gmallocn(n, sizeof(Gushort));
	  memcpy(codeToGID, ((GfxCIDFont *)gfxFont)->getCIDToGID(),
		  n * sizeof(Gushort));
	}
      } else {
	if (fileName)
	  ff = FoFiTrueType::load(fileName->getCString());
	else
	  ff = new FoFiTrueType(tmpBuf, tmpBufLen, gFalse);
	if (! ff)
	  goto err2;
	codeToGID = ((GfxCIDFont *)gfxFont)->getCodeToGIDMap(ff, &n);
	delete ff;
      }
      if (!(fontFile = fontEngine->loadTrueTypeFont(
			   id,
			   fontsrc,
			   codeToGID, n, faceIndex))) {
	error(-1, "Couldn't create a font for '%s'",
	      gfxFont->getName() ? gfxFont->getName()->getCString()
	                         : "(unnamed)");
	goto err2;
      }
      break;
    default:
      // this shouldn't happen
      goto err2;
    }
  }

  // get the font matrix
  state->getFontTransMat(&m11, &m12, &m21, &m22);
  m11 *= state->getHorizScaling();
  m12 *= state->getHorizScaling();

  // create the scaled font
  mat[0] = m11;  mat[1] = -m12;
  mat[2] = m21;  mat[3] = -m22;
  if (fabs(mat[0] * mat[3] - mat[1] * mat[2]) < 0.01) {
    // avoid a singular (or close-to-singular) matrix
    mat[0] = 0.01;  mat[1] = 0;
    mat[2] = 0;     mat[3] = 0.01;
  }
  font = fontEngine->getFont(fontFile, mat);

  if (fontsrc && !fontsrc->isFile)
    fontsrc->unref();
  return;

err2:
  delete id;
err1:
  if (fontsrc && !fontsrc->isFile)
    fontsrc->unref();
  return;
}

GfxPath *OPVPOutputDev::bitmapToGfxPath(SplashBitmap *bitmapA,
    int width, int height)
{
  int x,y;
  GfxPath *path;
  int x1, x2;
  SplashColor pix;

  path =  new GfxPath();

  for (y = 0;y < height;y++) {
    for (x = 0;x < width;x++) {
      bitmapA->getPixel(x,y,pix);
      if (pix[0] == 0) {
	/* start */
	x1 = x;
	for (x++;x < width;x++) {
	  bitmapA->getPixel(x,y,pix);
	  if (pix[0] != 0) {
	    /* end */
	    break;
	  }
	}
	x2 = x-1;
	path->moveTo(x1,y);
	path->lineTo(x2,y);
	path->lineTo(x2,(y+1));
	path->lineTo(x1,(y+1));
	path->close();
      }
    }
  }
  return path;
}

void OPVPOutputDev::patternStroke(GfxState *state, SplashPath *orgPath) {
  int width, height;
  SplashBitmap *tbitmap;
  OPRS *toprs;
  SplashColor color;
  GfxPath *gpath, *savedPath;
  double savedCTM[6];
  double *ctm;
  int i,h;
  OPRS *savedOprs;
  SplashPath *path;
  double savedCurX, savedCurY;
  double savedLineX, savedLineY;

  /* Calling upper level routine. 
	This is very ugly, but I have no good idea else. */
  width = soutRound(state->getPageWidth());
  height = soutRound(state->getPageHeight());

  /* save GS */
  savedOprs = oprs;
  savedPath = state->getPath()->copy();
  savedCurX = state->getCurX();
  savedCurY = state->getCurY();
  savedLineX = state->getLineX();
  savedLineY = state->getLineY();
  /* save CTM */
  ctm = state->getCTM();
  for (i = 0;i < 6;i++) {
    savedCTM[i] = ctm[i];
  }

  for (h = 0;h < height;h += SLICE_FOR_PATTERN) {
    int th = height-h+1;

    if (th > SLICE_FOR_PATTERN) th = SLICE_FOR_PATTERN;
    tbitmap = new SplashBitmap(width,th,1,splashModeMono1);
    toprs = new OPRS();
    toprs->setBitmap(tbitmap);
    oprs = toprs;
    color[0] = 0;
    toprs->setStrokePattern(new SplashSolidColor(color));
    color[0] = 0xff;
    toprs->clear(color);

    /* update all splash state except Colors */
    updateLineDash(state);
    updateLineJoin(state);
    updateLineCap(state);
    updateLineWidth(state);
    updateFlatness(state);
    updateMiterLimit(state);

    path = orgPath->copy();
    path->offset(0,-h);
    toprs->stroke(path);
    delete path;
    oprs = savedOprs;
    gpath = bitmapToGfxPath(tbitmap,width, th);
    delete toprs;
    delete tbitmap;

    state->setCTM(1.0,0.0,0.0,1.0,0.0,h);
    state->setPath(gpath);
    state->getGfx()->doStrokePatternFill(gFalse);
    /* restore CTM */
    state->setCTM(savedCTM[0],savedCTM[1],savedCTM[2],savedCTM[3],
	savedCTM[4],savedCTM[5]);
  }

  /* restore GS */
  state->moveTo(savedCurX,savedCurY); /* restore curX, curY */
  state->textSetPos(savedLineX,savedLineY); /* restore lineX, lineY */
  state->setPath(savedPath);
}

void OPVPOutputDev::stroke(GfxState *state) {
  SplashPath *path;
  GfxColorSpace *cs;

  /* check None separate color */
  if ((cs = state->getStrokeColorSpace()) == NULL) return;
  if (cs->getMode() == csSeparation) {
    GooString *name;

    name = (dynamic_cast<GfxSeparationColorSpace *>(cs))->getName();
    if (name == NULL) return;
    if (name->cmp("None") == 0) return;
  }

  path = convertPath(state, state->getPath());
  if (cs->getMode() == csPattern) {
    patternStroke(state,path);
  } else {
    oprs->stroke(path);
  }
  delete path;
}

void OPVPOutputDev::fill(GfxState *state) {
  SplashPath *path;
  GfxColorSpace *cs;

  /* check None separate color */
  if ((cs = state->getFillColorSpace()) == NULL) return;
  if (cs->getMode() == csSeparation) {
    GooString *name;

    name = (dynamic_cast<GfxSeparationColorSpace *>(cs))->getName();
    if (name == NULL) return;
    if (name->cmp("None") == 0) return;
  }

  path = convertPath(state, state->getPath());
  oprs->fill(path, gFalse);
  delete path;
}

void OPVPOutputDev::eoFill(GfxState *state) {
  SplashPath *path;
  GfxColorSpace *cs;

  /* check None separate color */
  if ((cs = state->getFillColorSpace()) == NULL) return;
  if (cs->getMode() == csSeparation) {
    GooString *name;

    name = (dynamic_cast<GfxSeparationColorSpace *>(cs))->getName();
    if (name == NULL) return;
    if (name->cmp("None") == 0) return;
  }

  path = convertPath(state, state->getPath());
  oprs->fill(path, gTrue);
  delete path;
}

void OPVPOutputDev::clip(GfxState *state) {
  SplashPath *path;

  path = convertPath(state, state->getPath());
  oprs->clipToPath(path, gFalse);
  delete path;
}

void OPVPOutputDev::eoClip(GfxState *state) {
  SplashPath *path;

  path = convertPath(state, state->getPath());
  oprs->clipToPath(path, gTrue);
  delete path;
}

SplashPath *OPVPOutputDev::convertPath(GfxState *state, GfxPath *path) {
  SplashPath *sPath;
  GfxSubpath *subpath;
  double x1, y1, x2, y2, x3, y3;
  int i, j;

  sPath = new SplashPath();
  for (i = 0; i < path->getNumSubpaths(); ++i) {
    subpath = path->getSubpath(i);
    if (subpath->getNumPoints() > 0) {
      state->transform(subpath->getX(0), subpath->getY(0), &x1, &y1);
      sPath->moveTo((SplashCoord)x1, (SplashCoord)y1);
      j = 1;
      while (j < subpath->getNumPoints()) {
	if (subpath->getCurve(j)) {
	  state->transform(subpath->getX(j), subpath->getY(j), &x1, &y1);
	  state->transform(subpath->getX(j+1), subpath->getY(j+1), &x2, &y2);
	  state->transform(subpath->getX(j+2), subpath->getY(j+2), &x3, &y3);
	  sPath->curveTo((SplashCoord)x1, (SplashCoord)y1,
			 (SplashCoord)x2, (SplashCoord)y2,
			 (SplashCoord)x3, (SplashCoord)y3);
	  j += 3;
	} else {
	  state->transform(subpath->getX(j), subpath->getY(j), &x1, &y1);
	  sPath->lineTo((SplashCoord)x1, (SplashCoord)y1);
	  ++j;
	}
      }
      if (subpath->isClosed()) {
	sPath->close();
      }
    }
  }
  return sPath;
}

GfxPath *OPVPOutputDev::splashPathToGfxPath(SplashPath *path)
{
  GfxPath *gfxPath;
  int i;

  gfxPath = new GfxPath();

  if (path->length == 0) {
    return gfxPath;
  }
  for (i = 0;i < path->length;i++) {
    if ((path->flags[i] & splashPathFirst) != 0) {
      gfxPath->moveTo(path->pts[i].x, path->pts[i].y);
    } else if ((path->flags[i] & splashPathClosed) != 0 && i != 0) {
      gfxPath->close();
    } else if (i+2 < path->length && path->flags[i] == splashPathCurve) {
      gfxPath->curveTo(path->pts[i].x, path->pts[i].y,
	path->pts[i+1].x, path->pts[i+1].y,
	path->pts[i+2].x, path->pts[i+2].y);
      i += 2;
    } else {
      gfxPath->lineTo(path->pts[i].x, path->pts[i].y);
    }
  }

  return gfxPath;
}

void OPVPOutputDev::patternFillChar(GfxState *state,
  double x, double y, CharCode code)
{
  GfxPath *savedPath;
  SplashPath *path;
  int i;
  double savedCurX, savedCurY;
  double savedLineX, savedLineY;

  savedPath = state->getPath()->copy();
  savedCurX = state->getCurX();
  savedCurY = state->getCurY();
  savedLineX = state->getLineX();
  savedLineY = state->getLineY();
  if ((path = font->getGlyphPath(code))) {
    GfxPath *gfxPath;
    double savedCTM[6];
    double *ctm;

    path->offset((SplashCoord)x, (SplashCoord)y);
    /* Calling upper level routine. 
      This is very ugly, but I have no good idea else. */
    gfxPath = splashPathToGfxPath(path);
    /* save CTM */
    ctm = state->getCTM();
    for (i = 0;i < 6;i++) {
	savedCTM[i] = ctm[i];
    }

    state->setCTM(1.0,0.0,0.0,1.0,0.0,0.0);
    delete path;

    state->setPath(gfxPath);
    state->getGfx()->doPatternFill(gFalse);

    /* restore CTM */
    state->setCTM(savedCTM[0],savedCTM[1],savedCTM[2],savedCTM[3],
      savedCTM[4],savedCTM[5]);
  }
  state->moveTo(savedCurX,savedCurY); /* restore curX, curY */
  state->textSetPos(savedLineX,savedLineY); /* restore lineX, lineY */
  state->setPath(savedPath);
}

void OPVPOutputDev::closeAllSubPath(SplashPath *path)
{
  int i;
  int f = 0;

  for (i = 0;i < path->length;i++) {
    if ((path->flags[i] & splashPathFirst) != 0) {
      f = i;
    }
    if ((path->flags[i] & splashPathLast) != 0) {
      if (path->pts[f].x == path->pts[i].x
        && path->pts[f].y == path->pts[i].y) {
	path->flags[f] |= splashPathClosed;
	path->flags[i] |= splashPathClosed;
      }
    }
  }
}

void OPVPOutputDev::drawChar(GfxState *state, double x, double y,
			       double dx, double dy,
			       double originX, double originY,
			       CharCode code, int nBytes,
			       Unicode *u, int uLen) {
  double x1, y1;
  SplashPath *path;
  int render;

  if (needFontUpdate) {
    updateFont(state);
  }
  if (!font) {
    return;
  }

  // check for invisible text -- this is used by Acrobat Capture
  render = state->getRender();
  if (render == 3) {
    return;
  }

  x -= originX;
  y -= originY;
  state->transform(x, y, &x1, &y1);

  // fill
  if (!(render & 1)) {
    if (state->getFillColorSpace()->getMode() == csPattern) {
      patternFillChar(state,x1,y1,code);
    } else {
      oprs->fillChar((SplashCoord)x1, (SplashCoord)y1, code, font, u, fontMat);
    }
  }

  // stroke
  if ((render & 3) == 1 || (render & 3) == 2) {
    if ((path = font->getGlyphPath(code))) {
      closeAllSubPath(path);
      path->offset((SplashCoord)x1, (SplashCoord)y1);
      if (state->getStrokeColorSpace()->getMode() == csPattern) {
	patternStroke(state,path);
      } else {
	oprs->stroke(path);
      }
      delete path;
    } else {
      error(-1,"No glyph outline infomation");
    }
  }

  // clip
  if (render & 4) {
    if ((path = font->getGlyphPath(code)) != NULL) {
      path->offset((SplashCoord)x1, (SplashCoord)y1);
      if (textClipPath) {
	textClipPath->append(path);
	delete path;
      } else {
	textClipPath = path;
      }
    } else {
      error(-1,"No glyph outline infomation");
    }
  }
}

GBool OPVPOutputDev::beginType3Char(GfxState *state, double x, double y,
				      double dx, double dy,
				      CharCode code, Unicode *u, int uLen) {
  GfxFont *gfxFont;
  Ref *fontID;
  double *ctm, *bbox;
  T3FontCache *t3Font;
  T3GlyphStack *t3gs;
  double x1, y1, xMin, yMin, xMax, yMax, xt, yt;
  int i, j;

  /* When it is in a vector mode, cache is not needed */
  if (!rasterMode) return gFalse;

  if (!(gfxFont = state->getFont())) {
    return gFalse;
  }
  fontID = gfxFont->getID();
  ctm = state->getCTM();
  state->transform(0, 0, &xt, &yt);

  // is it the first (MRU) font in the cache?
  if (!(nT3Fonts > 0 &&
	t3FontCache[0]->matches(fontID, ctm[0], ctm[1], ctm[2], ctm[3]))) {

    // is the font elsewhere in the cache?
    for (i = 1; i < nT3Fonts; ++i) {
      if (t3FontCache[i]->matches(fontID, ctm[0], ctm[1], ctm[2], ctm[3])) {
	t3Font = t3FontCache[i];
	for (j = i; j > 0; --j) {
	  t3FontCache[j] = t3FontCache[j - 1];
	}
	t3FontCache[0] = t3Font;
	break;
      }
    }
    if (i >= nT3Fonts) {

      // create new entry in the font cache
      if (nT3Fonts == splashOutT3FontCacheSize) {
	delete t3FontCache[nT3Fonts - 1];
	--nT3Fonts;
      }
      for (j = nT3Fonts; j > 0; --j) {
	t3FontCache[j] = t3FontCache[j - 1];
      }
      ++nT3Fonts;
      bbox = gfxFont->getFontBBox();
      if (bbox[0] == 0 && bbox[1] == 0 && bbox[2] == 0 && bbox[3] == 0) {
	// broken bounding box -- just take a guess
	xMin = xt - 5;
	xMax = xMin + 30;
	yMax = yt + 15;
	yMin = yMax - 45;
      } else {
	state->transform(bbox[0], bbox[1], &x1, &y1);
	xMin = xMax = x1;
	yMin = yMax = y1;
	state->transform(bbox[0], bbox[3], &x1, &y1);
	if (x1 < xMin) {
	  xMin = x1;
	} else if (x1 > xMax) {
	  xMax = x1;
	}
	if (y1 < yMin) {
	  yMin = y1;
	} else if (y1 > yMax) {
	  yMax = y1;
	}
	state->transform(bbox[2], bbox[1], &x1, &y1);
	if (x1 < xMin) {
	  xMin = x1;
	} else if (x1 > xMax) {
	  xMax = x1;
	}
	if (y1 < yMin) {
	  yMin = y1;
	} else if (y1 > yMax) {
	  yMax = y1;
	}
	state->transform(bbox[2], bbox[3], &x1, &y1);
	if (x1 < xMin) {
	  xMin = x1;
	} else if (x1 > xMax) {
	  xMax = x1;
	}
	if (y1 < yMin) {
	  yMin = y1;
	} else if (y1 > yMax) {
	  yMax = y1;
	}
      }
      t3FontCache[0] = new T3FontCache(fontID, ctm[0], ctm[1], ctm[2], ctm[3],
				       (int)floor(xMin - xt),
				       (int)floor(yMin - yt),
				       (int)ceil(xMax) - (int)floor(xMin) + 3,
				       (int)ceil(yMax) - (int)floor(yMin) + 3,
				       colorMode != splashModeMono1);
    }
  }
  t3Font = t3FontCache[0];

  // is the glyph in the cache?
  i = (code & (t3Font->cacheSets - 1)) * t3Font->cacheAssoc;
  for (j = 0; j < t3Font->cacheAssoc; ++j) {
    if ((t3Font->cacheTags[i+j].mru & 0x8000) &&
	t3Font->cacheTags[i+j].code == code) {
      drawType3Glyph(t3Font, &t3Font->cacheTags[i+j],
		     t3Font->cacheData + (i+j) * t3Font->glyphSize,
		     xt, yt);
      return gTrue;
    }
  }

  // push a new Type 3 glyph record
  t3gs = new T3GlyphStack();
  t3gs->next = t3GlyphStack;
  t3GlyphStack = t3gs;
  t3GlyphStack->code = code;
  t3GlyphStack->x = xt;
  t3GlyphStack->y = yt;
  t3GlyphStack->cache = t3Font;
  t3GlyphStack->cacheTag = NULL;
  t3GlyphStack->cacheData = NULL;
  return gFalse;
}

void OPVPOutputDev::endType3Char(GfxState *state) {
  T3GlyphStack *t3gs;
  double *ctm;

  /* When it is in a vector mode, cache is not needed */
  if (!rasterMode) return;

  if (t3GlyphStack->cacheTag) {
    memcpy(t3GlyphStack->cacheData, bitmap->getDataPtr(),
	   t3GlyphStack->cache->glyphSize);
    delete bitmap;
    delete oprs;
    bitmap = t3GlyphStack->origBitmap;
    oprs = t3GlyphStack->origOPRS;
    ctm = state->getCTM();
    state->setCTM(ctm[0], ctm[1], ctm[2], ctm[3],
		  t3GlyphStack->origCTM4, t3GlyphStack->origCTM5);
    drawType3Glyph(t3GlyphStack->cache,
		   t3GlyphStack->cacheTag, t3GlyphStack->cacheData,
		   t3GlyphStack->x, t3GlyphStack->y);
  }
  t3gs = t3GlyphStack;
  t3GlyphStack = t3gs->next;
  delete t3gs;
}

void OPVPOutputDev::type3D0(GfxState *state, double wx, double wy) {
}

void OPVPOutputDev::type3D1(GfxState *state, double wx, double wy,
			      double llx, double lly, double urx, double ury) {
  double *ctm;
  T3FontCache *t3Font;
  SplashColor color;
  double xt, yt, xMin, xMax, yMin, yMax, x1, y1;
  int i, j;

  /* When it is in a vector mode, cache is not needed */
  if (!rasterMode) {
    return;
  }

  t3Font = t3GlyphStack->cache;

  // check for a valid bbox
  state->transform(0, 0, &xt, &yt);
  state->transform(llx, lly, &x1, &y1);
  xMin = xMax = x1;
  yMin = yMax = y1;
  state->transform(llx, ury, &x1, &y1);
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  state->transform(urx, lly, &x1, &y1);
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  state->transform(urx, ury, &x1, &y1);
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  if (xMin - xt < t3Font->glyphX ||
      yMin - yt < t3Font->glyphY ||
      xMax - xt > t3Font->glyphX + t3Font->glyphW ||
      yMax - yt > t3Font->glyphY + t3Font->glyphH) {
    error(-1, "Bad bounding box in Type 3 glyph");
    return;
  }

  // allocate a cache entry
  i = (t3GlyphStack->code & (t3Font->cacheSets - 1)) * t3Font->cacheAssoc;
  for (j = 0; j < t3Font->cacheAssoc; ++j) {
    if ((t3Font->cacheTags[i+j].mru & 0x7fff) == t3Font->cacheAssoc - 1) {
      t3Font->cacheTags[i+j].mru = 0x8000;
      t3Font->cacheTags[i+j].code = t3GlyphStack->code;
      t3GlyphStack->cacheTag = &t3Font->cacheTags[i+j];
      t3GlyphStack->cacheData = t3Font->cacheData + (i+j) * t3Font->glyphSize;
    } else {
      ++t3Font->cacheTags[i+j].mru;
    }
  }

  // save state
  t3GlyphStack->origBitmap = bitmap;
  t3GlyphStack->origOPRS = oprs;
  ctm = state->getCTM();
  t3GlyphStack->origCTM4 = ctm[4];
  t3GlyphStack->origCTM5 = ctm[5];

  // create the temporary bitmap
  if (colorMode == splashModeMono1) {
    bitmap = new SplashBitmap(t3Font->glyphW, t3Font->glyphH, 1,
      splashModeMono1);
    oprs = new OPRS();
    oprs->setBitmap(bitmap);
    color[0] = 0;
    oprs->clear(color);
    color[0] = 1;
  } else {
    bitmap = new SplashBitmap(t3Font->glyphW, t3Font->glyphH, 1,
      splashModeMono8);
    oprs = new OPRS();
    oprs->setBitmap(bitmap);
    color[0] = 0x00;
    oprs->clear(color);
    color[0] = 0xff;
  }
  oprs->setFillPattern(new SplashSolidColor(color));
  oprs->setStrokePattern(new SplashSolidColor(color));
  //~ this should copy other state from t3GlyphStack->origSplash?
  state->setCTM(ctm[0], ctm[1], ctm[2], ctm[3],
		-t3Font->glyphX, -t3Font->glyphY);
}

void OPVPOutputDev::drawType3Glyph(T3FontCache *t3Font,
				     T3FontCacheTag *tag, Guchar *data,
				     double x, double y) {
  SplashGlyphBitmap glyph;

  glyph.x = -t3Font->glyphX;
  glyph.y = -t3Font->glyphY;
  glyph.w = t3Font->glyphW;
  glyph.h = t3Font->glyphH;
  glyph.aa = colorMode != splashModeMono1;
  glyph.data = data;
  glyph.freeData = gFalse;
  oprs->fillGlyph((SplashCoord)x, (SplashCoord)y, &glyph);
}

void OPVPOutputDev::endTextObject(GfxState *state) {
  if (textClipPath) {
    oprs->clipToPath(textClipPath, gFalse);
    delete textClipPath;
    textClipPath = NULL;
  }
}

struct SplashOutImageMaskData {
  ImageStream *imgStr;
  GBool invert;
  int width, height, y;
};

GBool OPVPOutputDev::imageMaskSrc(void *data, SplashColorPtr line) {
  SplashOutImageMaskData *imgMaskData = (SplashOutImageMaskData *)data;
  Guchar *p;
  SplashColorPtr q;
  int x;

  if (imgMaskData->y == imgMaskData->height) {
    return gFalse;
  }
  for (x = 0, p = imgMaskData->imgStr->getLine(), q = line;
       x < imgMaskData->width;
       ++x) {
    *q++ = *p++ ^ imgMaskData->invert;
  }
  ++imgMaskData->y;
  return gTrue;
}

void OPVPOutputDev::patternFillImageMask(GfxState *state,
  SplashImageMaskSource src, void *srcData, int w, int h, SplashCoord *mat)
{
  SplashBitmap *maskBitmap;
  Splash *maskSplash;
  SplashColor color;
  SplashCoord cmat[6];
  double savedCTM[6];
  double *ctm;
  int i;
  GfxPath *gpath, *savedPath;
  double savedCurX, savedCurY;
  double savedLineX, savedLineY;

  /* save CTM */
  ctm = state->getCTM();
  for (i = 0;i < 6;i++) {
    savedCTM[i] = ctm[i];
  }

  maskBitmap = new SplashBitmap(w,h,1,splashModeMono1);
  maskSplash = new Splash(maskBitmap);
  color[0] = 1;
  maskSplash->clear(color);
  cmat[0] = w;
  cmat[1] = 0.0;
  cmat[2] = 0.0;
  cmat[3] = h;
  cmat[4] = 0.0;
  cmat[5] = 0.0;
  maskSplash->fillImageMask(src,srcData,w,h,cmat);
  gpath = bitmapToGfxPath(maskBitmap,w,h);
  delete maskSplash;
  delete maskBitmap;
  savedPath = state->getPath()->copy();
  savedCurX = state->getCurX();
  savedCurY = state->getCurY();
  savedLineX = state->getLineX();
  savedLineY = state->getLineY();
  state->setPath(gpath);
  state->setCTM(mat[0]/w,mat[1]/w,mat[2]/h,mat[3]/h,mat[4],mat[5]);
  state->getGfx()->doPatternFill(gFalse);
  state->moveTo(savedCurX,savedCurY); /* restore curX, curY */
  state->textSetPos(savedLineX,savedLineY); /* restore lineX, lineY */
  state->setPath(savedPath);
  state->setCTM(savedCTM[0],savedCTM[1],savedCTM[2],savedCTM[3],
      savedCTM[4],savedCTM[5]);
}

void OPVPOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
				    int width, int height, GBool invert,
				    GBool inlineImg) {
  double *ctm;
  SplashCoord mat[6];
  SplashOutImageMaskData imgMaskData;
  Guchar pix;

  ctm = state->getCTM();
  mat[0] = ctm[0];
  mat[1] = ctm[1];
  mat[2] = -ctm[2];
  mat[3] = -ctm[3];
  mat[4] = ctm[2] + ctm[4];
  mat[5] = ctm[3] + ctm[5];

  imgMaskData.imgStr = new ImageStream(str, width, 1, 1);
  imgMaskData.imgStr->reset();
  imgMaskData.invert = invert ? 0 : 1;
  imgMaskData.width = width;
  imgMaskData.height =  height;
  imgMaskData.y = 0;

  if (state->getFillColorSpace()->getMode() == csPattern) {
    patternFillImageMask(state,&imageMaskSrc, &imgMaskData, width, height, mat);
  } else {
    oprs->fillImageMask(&imageMaskSrc, &imgMaskData, width, height, mat);
  }
  if (inlineImg) {
    while (imgMaskData.y < height) {
      imgMaskData.imgStr->getLine();
      ++imgMaskData.y;
    }
  }

  delete imgMaskData.imgStr;
}

struct SplashOutImageData {
  ImageStream *imgStr;
  GfxImageColorMap *colorMap;
  SplashColorPtr lookup;
  int *maskColors;
  SplashColorMode colorMode;
  int width, height, y;
};

GBool OPVPOutputDev::imageSrc(void *data, SplashColorPtr line)
{
  SplashOutImageData *imgData = (SplashOutImageData *)data;
  Guchar *p;
  SplashColorPtr q, col;
  GfxRGB rgb;
  GfxGray gray;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  int nComps, x;

  if (imgData->y == imgData->height) {
    return gFalse;
  }

  nComps = imgData->colorMap->getNumPixelComps();

  if (imgData->lookup) {
    switch (imgData->colorMode) {
  case splashModeMono1:
  case splashModeMono8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
	   x < imgData->width;
	   ++x, ++p) {
	*q++ = imgData->lookup[*p];
      }
    break;
  case splashModeRGB8:
    case splashModeBGR8:
    case splashModeRGB8Qt:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
	   x < imgData->width;
	   ++x, ++p) {
	col = &imgData->lookup[3 * *p];
	*q++ = col[0];
	*q++ = col[1];
	*q++ = col[2];
      }
    break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
	   x < imgData->width;
	   ++x, ++p) {
	col = &imgData->lookup[4 * *p];
	*q++ = col[0];
	*q++ = col[1];
	*q++ = col[2];
	*q++ = col[3];
      }
      break;
#endif
    case splashModeAMono8:
    case splashModeARGB8:
    case splashModeBGRA8:
#if SPLASH_CMYK
    case splashModeACMYK8:
#endif
      //~ unimplemented
    break;
  }
  } else {
    switch (imgData->colorMode) {
    case splashModeMono1:
    case splashModeMono8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
	   x < imgData->width;
	   ++x, p += nComps) {
	imgData->colorMap->getGray(p, &gray);
	*q++ = colToByte(gray);
      }
	break;
    case splashModeRGB8:
    case splashModeRGB8Qt:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
	   x < imgData->width;
	   ++x, p += nComps) {
	imgData->colorMap->getRGB(p, &rgb);
	*q++ = colToByte(rgb.r);
	*q++ = colToByte(rgb.g);
	*q++ = colToByte(rgb.b);
      }
      break;
    case splashModeBGR8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
	   x < imgData->width;
	   ++x, p += nComps) {
	imgData->colorMap->getRGB(p, &rgb);
	*q++ = colToByte(rgb.b);
	*q++ = colToByte(rgb.g);
	*q++ = colToByte(rgb.r);
      }
      break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
	   x < imgData->width;
	   ++x, p += nComps) {
	imgData->colorMap->getCMYK(p, &cmyk);
	*q++ = colToByte(cmyk.c);
	*q++ = colToByte(cmyk.m);
	*q++ = colToByte(cmyk.y);
	*q++ = colToByte(cmyk.k);
      }
      break;
#endif
    case splashModeAMono8:
    case splashModeARGB8:
    case splashModeBGRA8:
#if SPLASH_CMYK
    case splashModeACMYK8:
#endif
      //~ unimplemented
      break;
    }
  }

  ++imgData->y;
  return gTrue;
}

GBool OPVPOutputDev::alphaImageSrc(void *data, SplashColorPtr line) {
  SplashOutImageData *imgData = (SplashOutImageData *)data;
  Guchar *p;
  SplashColorPtr q, col;
  GfxRGB rgb;
  GfxGray gray;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar alpha;
  int nComps, x, i;

  if (imgData->y == imgData->height) {
    return gFalse;
  }

  nComps = imgData->colorMap->getNumPixelComps();

  for (x = 0, p = imgData->imgStr->getLine(), q = line;
       x < imgData->width;
       ++x, p += nComps) {
    alpha = 0;
    for (i = 0; i < nComps; ++i) {
      if (p[i] < imgData->maskColors[2*i] ||
	  p[i] > imgData->maskColors[2*i+1]) {
	alpha = 0xff;
	break;
      }
    }
    if (imgData->lookup) {
      switch (imgData->colorMode) {
      case splashModeMono1:
      case splashModeMono8:
	*q++ = alpha;
	*q++ = imgData->lookup[*p];
	break;
      case splashModeRGB8:
      case splashModeRGB8Qt:
	*q++ = alpha;
	col = &imgData->lookup[3 * *p];
	*q++ = col[0];
	*q++ = col[1];
	*q++ = col[2];
	break;
      case splashModeBGR8:
	col = &imgData->lookup[3 * *p];
	*q++ = col[0];
	*q++ = col[1];
	*q++ = col[2];
	*q++ = alpha;
	break;
#if SPLASH_CMYK
      case splashModeCMYK8:
	*q++ = alpha;
	col = &imgData->lookup[4 * *p];
	*q++ = col[0];
	*q++ = col[1];
	*q++ = col[2];
	*q++ = col[3];
	break;
#endif
      case splashModeAMono8:
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeACMYK8:
#endif
	//~ unimplemented
	break;
      }
    } else {
      switch (imgData->colorMode) {
      case splashModeMono1:
      case splashModeMono8:
	imgData->colorMap->getGray(p, &gray);
	*q++ = alpha;
	*q++ = colToByte(gray);
	break;
      case splashModeRGB8:
      case splashModeRGB8Qt:
	imgData->colorMap->getRGB(p, &rgb);
	*q++ = alpha;
	*q++ = colToByte(rgb.r);
	*q++ = colToByte(rgb.g);
	*q++ = colToByte(rgb.b);
	break;
      case splashModeBGR8:
	imgData->colorMap->getRGB(p, &rgb);
	*q++ = colToByte(rgb.b);
	*q++ = colToByte(rgb.g);
	*q++ = colToByte(rgb.r);
	*q++ = alpha;
	break;
#if SPLASH_CMYK
      case splashModeCMYK8:
	imgData->colorMap->getCMYK(p, &cmyk);
	*q++ = alpha;
	*q++ = colToByte(cmyk.c);
	*q++ = colToByte(cmyk.m);
	*q++ = colToByte(cmyk.y);
	*q++ = colToByte(cmyk.k);
	break;
#endif
      case splashModeAMono8:
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeACMYK8:
#endif
	//~ unimplemented
	break;
      }
    }
  }

  ++imgData->y;
  return gTrue;
}

void OPVPOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
				int width, int height,
				GfxImageColorMap *colorMap,
				int *maskColors, GBool inlineImg) {
  double *ctm;
  SplashCoord mat[6];
  SplashOutImageData imgData;
  SplashColorMode srcMode;
  SplashImageSource src;
  GfxGray gray;
  GfxRGB rgb;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar pix;
  int n, i;

  ctm = state->getCTM();
  mat[0] = ctm[0];
  mat[1] = ctm[1];
  mat[2] = -ctm[2];
  mat[3] = -ctm[3];
  mat[4] = ctm[2] + ctm[4];
  mat[5] = ctm[3] + ctm[5];

  imgData.imgStr = new ImageStream(str, width,
				   colorMap->getNumPixelComps(),
				   colorMap->getBits());
  imgData.imgStr->reset();
  imgData.colorMap = colorMap;
  imgData.maskColors = maskColors;
  imgData.colorMode = colorMode;
  imgData.width = width;
  imgData.height = height;
  imgData.y = 0;

  // special case for one-channel (monochrome/gray/separation) images:
  // build a lookup table here
  imgData.lookup = NULL;
  if (colorMap->getNumPixelComps() == 1) {
    n = 1 << colorMap->getBits();
    switch (colorMode) {
    case splashModeMono1:
    case splashModeMono8:
      imgData.lookup = (SplashColorPtr)gmalloc(n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getGray(&pix, &gray);
	imgData.lookup[i] = colToByte(gray);
      }
      break;
    case splashModeRGB8:
    case splashModeRGB8Qt:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getRGB(&pix, &rgb);
	imgData.lookup[3*i] = colToByte(rgb.r);
	imgData.lookup[3*i+1] = colToByte(rgb.g);
	imgData.lookup[3*i+2] = colToByte(rgb.b);
      }
      break;
    case splashModeBGR8:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getRGB(&pix, &rgb);
	imgData.lookup[3*i] = colToByte(rgb.b);
	imgData.lookup[3*i+1] = colToByte(rgb.g);
	imgData.lookup[3*i+2] = colToByte(rgb.r);
      }
      break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      imgData.lookup = (SplashColorPtr)gmalloc(4 * n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getCMYK(&pix, &cmyk);
	imgData.lookup[4*i] = colToByte(cmyk.c);
	imgData.lookup[4*i+1] = colToByte(cmyk.m);
	imgData.lookup[4*i+2] = colToByte(cmyk.y);
	imgData.lookup[4*i+3] = colToByte(cmyk.k);
      }
      break;
#endif
    default:
      //~ unimplemented
      break;
    }
  }

  switch (colorMode) {
  case splashModeMono1:
  case splashModeMono8:
    srcMode = maskColors ? splashModeAMono8 : splashModeMono8;
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    srcMode = maskColors ? splashModeARGB8 : splashModeRGB8;
    break;
  case splashModeBGR8:
    srcMode = maskColors ? splashModeBGRA8 : splashModeBGR8;
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    srcMode = maskColors ? splashModeACMYK8 : splashModeCMYK8;
    break;
#endif
  default:
    //~ unimplemented
    srcMode = splashModeRGB8;
    break;
  }  
  src = maskColors ? &alphaImageSrc : &imageSrc;
  oprs->drawImage(src, &imgData, srcMode, width, height, mat);
  if (inlineImg) {
    while (imgData.y < height) {
      imgData.imgStr->getLine();
      ++imgData.y;
    }
  }

  gfree(imgData.lookup);
  delete imgData.imgStr;
  str->close();
}

struct SplashOutMaskedImageData {
  ImageStream *imgStr;
  GfxImageColorMap *colorMap;
  SplashBitmap *mask;
  SplashColorPtr lookup;
  SplashColorMode colorMode;
  int width, height, y;
};

GBool OPVPOutputDev::maskedImageSrc(void *data, SplashColorPtr line) {
  SplashOutMaskedImageData *imgData = (SplashOutMaskedImageData *)data;
  Guchar *p;
  SplashColor maskColor;
  SplashColorPtr q, col;
  GfxRGB rgb;
  GfxGray gray;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar alpha;
  int nComps, x;

  if (imgData->y == imgData->height) {
    return gFalse;
  }

  nComps = imgData->colorMap->getNumPixelComps();

  for (x = 0, p = imgData->imgStr->getLine(), q = line;
       x < imgData->width;
       ++x, p += nComps) {
    imgData->mask->getPixel(x, imgData->y, maskColor);
    alpha = maskColor[0] ? 0xff : 0x00;
    if (imgData->lookup) {
      switch (imgData->colorMode) {
      case splashModeMono1:
      case splashModeMono8:
	*q++ = alpha;
	*q++ = imgData->lookup[*p];
	break;
      case splashModeRGB8:
      case splashModeRGB8Qt:
	*q++ = alpha;
	col = &imgData->lookup[3 * *p];
	*q++ = col[0];
	*q++ = col[1];
	*q++ = col[2];
	break;
      case splashModeBGR8:
	col = &imgData->lookup[3 * *p];
	*q++ = col[0];
	*q++ = col[1];
	*q++ = col[2];
	*q++ = alpha;
	break;
#if SPLASH_CMYK
      case splashModeCMYK8:
	*q++ = alpha;
	col = &imgData->lookup[4 * *p];
	*q++ = col[0];
	*q++ = col[1];
	*q++ = col[2];
	*q++ = col[3];
	break;
#endif
      case splashModeAMono8:
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeACMYK8:
#endif
	//~ unimplemented
	break;
      }
    } else {
      switch (imgData->colorMode) {
      case splashModeMono1:
      case splashModeMono8:
	imgData->colorMap->getGray(p, &gray);
	*q++ = alpha;
	*q++ = colToByte(gray);
	break;
      case splashModeRGB8:
      case splashModeRGB8Qt:
	imgData->colorMap->getRGB(p, &rgb);
	*q++ = alpha;
	*q++ = colToByte(rgb.r);
	*q++ = colToByte(rgb.g);
	*q++ = colToByte(rgb.b);
	break;
      case splashModeBGR8:
	imgData->colorMap->getRGB(p, &rgb);
	*q++ = colToByte(rgb.b);
	*q++ = colToByte(rgb.g);
	*q++ = colToByte(rgb.r);
	*q++ = alpha;
	break;
#if SPLASH_CMYK
      case splashModeCMYK8:
	imgData->colorMap->getCMYK(p, &cmyk);
	*q++ = alpha;
	*q++ = colToByte(cmyk.c);
	*q++ = colToByte(cmyk.m);
	*q++ = colToByte(cmyk.y);
	*q++ = colToByte(cmyk.k);
	break;
#endif
      case splashModeAMono8:
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeACMYK8:
#endif
	//~ unimplemented
	break;
      }
    }
  }

  ++imgData->y;
  return gTrue;
}

void OPVPOutputDev::drawMaskedImage(GfxState *state, Object *ref,
				      Stream *str, int width, int height,
				      GfxImageColorMap *colorMap,
				      Stream *maskStr, int maskWidth,
				      int maskHeight, GBool maskInvert) {
  double *ctm;
  SplashCoord mat[6];
  SplashOutMaskedImageData imgData;
  SplashOutImageMaskData imgMaskData;
  SplashColorMode srcMode;
  SplashBitmap *maskBitmap;
  Splash *maskSplash;
  SplashColor maskColor;
  GfxGray gray;
  GfxRGB rgb;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar pix;
  int n, i;

  //----- scale the mask image to the same size as the source image

  mat[0] = (SplashCoord)width;
  mat[1] = 0;
  mat[2] = 0;
  mat[3] = (SplashCoord)height;
  mat[4] = 0;
  mat[5] = 0;
  imgMaskData.imgStr = new ImageStream(maskStr, maskWidth, 1, 1);
  imgMaskData.imgStr->reset();
  imgMaskData.invert = maskInvert ? 0 : 1;
  imgMaskData.width = maskWidth;
  imgMaskData.height = maskHeight;
  imgMaskData.y = 0;
  maskBitmap = new SplashBitmap(width, height, 1, splashModeMono1);
  maskSplash = new Splash(maskBitmap);
  maskColor[0] = 0;
  maskSplash->clear(maskColor);
  maskColor[0] = 1;
  maskSplash->setFillPattern(new SplashSolidColor(maskColor));
  maskSplash->fillImageMask(&imageMaskSrc, &imgMaskData,
			    maskWidth, maskHeight, mat);
  delete imgMaskData.imgStr;
  maskStr->close();
  delete maskSplash;

  //----- draw the source image

  ctm = state->getCTM();
  mat[0] = ctm[0];
  mat[1] = ctm[1];
  mat[2] = -ctm[2];
  mat[3] = -ctm[3];
  mat[4] = ctm[2] + ctm[4];
  mat[5] = ctm[3] + ctm[5];

  imgData.imgStr = new ImageStream(str, width,
				   colorMap->getNumPixelComps(),
				   colorMap->getBits());
  imgData.imgStr->reset();
  imgData.colorMap = colorMap;
  imgData.mask = maskBitmap;
  imgData.colorMode = colorMode;
  imgData.width = width;
  imgData.height = height;
  imgData.y = 0;

  // special case for one-channel (monochrome/gray/separation) images:
  // build a lookup table here
  imgData.lookup = NULL;
  if (colorMap->getNumPixelComps() == 1) {
    n = 1 << colorMap->getBits();
    switch (colorMode) {
    case splashModeMono1:
    case splashModeMono8:
      imgData.lookup = (SplashColorPtr)gmalloc(n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getGray(&pix, &gray);
	imgData.lookup[i] = colToByte(gray);
      }
      break;
    case splashModeRGB8:
    case splashModeRGB8Qt:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getRGB(&pix, &rgb);
	imgData.lookup[3*i] = colToByte(rgb.r);
	imgData.lookup[3*i+1] = colToByte(rgb.g);
	imgData.lookup[3*i+2] = colToByte(rgb.b);
      }
      break;
    case splashModeBGR8:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getRGB(&pix, &rgb);
	imgData.lookup[3*i] = colToByte(rgb.b);
	imgData.lookup[3*i+1] = colToByte(rgb.g);
	imgData.lookup[3*i+2] = colToByte(rgb.r);
      }
      break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      imgData.lookup = (SplashColorPtr)gmalloc(4 * n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getCMYK(&pix, &cmyk);
	imgData.lookup[4*i] = colToByte(cmyk.c);
	imgData.lookup[4*i+1] = colToByte(cmyk.m);
	imgData.lookup[4*i+2] = colToByte(cmyk.y);
	imgData.lookup[4*i+3] = colToByte(cmyk.k);
      }
      break;
#endif
    default:
      //~ unimplemented
      break;
    }
  }

  switch (colorMode) {
  case splashModeMono1:
  case splashModeMono8:
    srcMode = splashModeAMono8;
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    srcMode = splashModeARGB8;
    break;
  case splashModeBGR8:
    srcMode = splashModeBGRA8;
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    srcMode = splashModeACMYK8;
    break;
#endif
  default:
    //~ unimplemented
    srcMode = splashModeARGB8;
    break;
  }  
  oprs->drawImage(&maskedImageSrc, &imgData, srcMode, width, height, mat);

  delete maskBitmap;
  gfree(imgData.lookup);
  delete imgData.imgStr;
  str->close();
}

void OPVPOutputDev::drawSoftMaskedImage(GfxState *state, Object *ref,
					  Stream *str, int width, int height,
					  GfxImageColorMap *colorMap,
					  Stream *maskStr,
					  int maskWidth, int maskHeight,
					  GfxImageColorMap *maskColorMap) {
  double *ctm;
  SplashCoord mat[6];
  SplashOutImageData imgData;
  SplashOutImageData imgMaskData;
  SplashColorMode srcMode;
  SplashBitmap *maskBitmap;
  Splash *maskSplash;
  SplashColor maskColor;
  GfxGray gray;
  GfxRGB rgb;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar pix;
  int n, i;

  ctm = state->getCTM();
  mat[0] = ctm[0];
  mat[1] = ctm[1];
  mat[2] = -ctm[2];
  mat[3] = -ctm[3];
  mat[4] = ctm[2] + ctm[4];
  mat[5] = ctm[3] + ctm[5];

  //----- set up the soft mask

  imgMaskData.imgStr = new ImageStream(maskStr, maskWidth,
				       maskColorMap->getNumPixelComps(),
				       maskColorMap->getBits());
  imgMaskData.imgStr->reset();
  imgMaskData.colorMap = maskColorMap;
  imgMaskData.maskColors = NULL;
  imgMaskData.colorMode = splashModeMono8;
  imgMaskData.width = maskWidth;
  imgMaskData.height = maskHeight;
  imgMaskData.y = 0;
  n = 1 << maskColorMap->getBits();
  imgMaskData.lookup = (SplashColorPtr)gmalloc(n);
  for (i = 0; i < n; ++i) {
    pix = (Guchar)i;
    maskColorMap->getGray(&pix, &gray);
    imgMaskData.lookup[i] = colToByte(gray);
  }
  maskBitmap = new SplashBitmap(maskWidth,maskHeight,
				1, splashModeMono8);
  maskSplash = new Splash(maskBitmap);
  maskColor[0] = 0;
  maskSplash->clear(maskColor);
  maskSplash->drawImage(&imageSrc, &imgMaskData,
			splashModeMono8, maskWidth, maskHeight, mat);
  delete imgMaskData.imgStr;
  maskStr->close();
  gfree(imgMaskData.lookup);
  delete maskSplash;
  oprs->setSoftMask(maskBitmap);

  //----- draw the source image

  imgData.imgStr = new ImageStream(str, width,
				   colorMap->getNumPixelComps(),
				   colorMap->getBits());
  imgData.imgStr->reset();
  imgData.colorMap = colorMap;
  imgData.maskColors = NULL;
  imgData.colorMode = colorMode;
  imgData.width = width;
  imgData.height = height;
  imgData.y = 0;

  // special case for one-channel (monochrome/gray/separation) images:
  // build a lookup table here
  imgData.lookup = NULL;
  if (colorMap->getNumPixelComps() == 1) {
    n = 1 << colorMap->getBits();
    switch (colorMode) {
    case splashModeMono1:
    case splashModeMono8:
      imgData.lookup = (SplashColorPtr)gmalloc(n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getGray(&pix, &gray);
	imgData.lookup[i] = colToByte(gray);
      }
      break;
    case splashModeRGB8:
    case splashModeRGB8Qt:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getRGB(&pix, &rgb);
	imgData.lookup[3*i] = colToByte(rgb.r);
	imgData.lookup[3*i+1] = colToByte(rgb.g);
	imgData.lookup[3*i+2] = colToByte(rgb.b);
      }
      break;
    case splashModeBGR8:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getRGB(&pix, &rgb);
	imgData.lookup[3*i] = colToByte(rgb.b);
	imgData.lookup[3*i+1] = colToByte(rgb.g);
	imgData.lookup[3*i+2] = colToByte(rgb.r);
      }
      break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      imgData.lookup = (SplashColorPtr)gmalloc(4 * n);
      for (i = 0; i < n; ++i) {
	pix = (Guchar)i;
	colorMap->getCMYK(&pix, &cmyk);
	imgData.lookup[4*i] = colToByte(cmyk.c);
	imgData.lookup[4*i+1] = colToByte(cmyk.m);
	imgData.lookup[4*i+2] = colToByte(cmyk.y);
	imgData.lookup[4*i+3] = colToByte(cmyk.k);
      }
      break;
#endif
    default:
      //~ unimplemented
      break;
    }
  }

  switch (colorMode) {
  case splashModeMono1:
  case splashModeMono8:
    srcMode = splashModeMono8;
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    srcMode = splashModeRGB8;
    break;
  case splashModeBGR8:
    srcMode = splashModeBGR8;
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    srcMode = splashModeCMYK8;
    break;
#endif
  default:
    //~ unimplemented
    srcMode = splashModeRGB8;
    break;
  }  
  oprs->drawImage(&imageSrc, &imgData, srcMode, width, height, mat);

  oprs->setSoftMask(NULL);
  gfree(imgData.lookup);
  delete imgData.imgStr;
  str->close();
}

int OPVPOutputDev::getBitmapWidth() {
  return bitmap->getWidth();
}

int OPVPOutputDev::getBitmapHeight() {
  return bitmap->getHeight();
}

void OPVPOutputDev::xorRectangle(int x0, int y0, int x1, int y1,
				   SplashPattern *pattern) {
    /* no need in printing */
}

void OPVPOutputDev::setFillColor(int r, int g, int b) {
  GfxRGB rgb;
  GfxGray gray;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif

  rgb.r = byteToCol(r);
  rgb.g = byteToCol(g);
  rgb.b = byteToCol(b);
  gray = (GfxColorComp)(0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.g + 0.5);
  if (gray > gfxColorComp1) {
    gray = gfxColorComp1;
  }
#if SPLASH_CMYK
  cmyk.c = gfxColorComp1 - rgb.r;
  cmyk.m = gfxColorComp1 - rgb.g;
  cmyk.y = gfxColorComp1 - rgb.b;
  cmyk.k = 0;
  oprs->setFillPattern(getColor(gray, &rgb, &cmyk));
#else
  oprs->setFillPattern(getColor(gray, &rgb));
#endif
}

int OPVPOutputDev::OPVPStartJob(char *jobInfo)
{
    return oprs->OPVPStartJob(jobInfo);
}

int OPVPOutputDev::OPVPEndJob()
{
    return oprs->OPVPEndJob();
}

int OPVPOutputDev::OPVPStartDoc(char *docInfo)
{
    return oprs->OPVPStartDoc(docInfo);
}

int OPVPOutputDev::OPVPEndDoc()
{
    return oprs->OPVPEndDoc();
}

int OPVPOutputDev::OPVPStartPage(char *pageInfo, int rasterWidth)
{
    return oprs->OPVPStartPage(pageInfo,rasterWidth);
}

int OPVPOutputDev::OPVPEndPage()
{
    return oprs->OPVPEndPage();
}

int OPVPOutputDev::outSlice()
{
    return oprs->outSlice();
}

void OPVPOutputDev::psXObject(Stream *psStream, Stream *level1Stream)
{
  error(-1,"psXObject is found, but it is not supported");
}
