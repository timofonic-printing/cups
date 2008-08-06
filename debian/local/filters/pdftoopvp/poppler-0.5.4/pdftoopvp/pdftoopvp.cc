//========================================================================
//
// pdftoopvp.cc
//
// Copyright 2005 AXE,Inc.
//
// 2007 Modified by BBR Inc.
//========================================================================

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#ifndef HAVE_CUPSLIB
#include "parseargs.h"
#endif
#include "gmem.h"
#include "GooString.h"
#include "GlobalParams.h"
#include "Object.h"
#include "PDFDoc.h"
#include "SplashBitmap.h"
#include "OPRS.h"
#include "OPVPOutputDev.h"
#include "config.h"
#include "Gfx.h"
#ifdef HAVE_CUPSLIB
#include <cups/cups.h>
#endif
#include "Error.h"
#include "mcheck.h"

#define MMPERINCH (25.4)

static int copies = 1;
static int firstPage = -1;
static int lastPage = -1;
static int resolution = 300;
static int hResolution = 0;
static int vResolution = 0;
static int sliceHeight = 400;
static GBool mono = gFalse;
static GBool reverse = gFalse;
static GBool gray = gFalse;
static char enableT1libStr[16] = "";
static char enableFreeTypeStr[16] = "";
static GBool quiet = gFalse;
static char cfgFileName[1024] = "";
static char outputOrderStr[256] = "";
static char pageRangesStr[1024] = "";
static char pageSetStr[256] = "";
#ifndef HAVE_CUPSLIB
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;
#endif
static GBool rasterMode = gFalse;
static GBool oldLipsDriver = gFalse;
static GBool HPDriver = gFalse;
static GBool NECDriver = gFalse;
static GBool clipPathNotSaved = gFalse;
static GBool noShearImage = gFalse;
static GBool noLineStyle = gFalse;
static GBool noImageMask = gFalse;
static GBool noClipPath = gFalse;
static GBool ignoreMiterLimit = gFalse;
static GBool noMiterLimit = gFalse;
static GBool duplex = gFalse;
static char printerDriver[1024] = "";
static char printerModel[1024] = "";
static char jobInfo[4096] = "";
static char docInfo[1024] = "";
static char pageInfo[1024] = "";
static int paperWidth = 0;
static int paperHeight = 0;
static int printableWidth = 0;
static int printableHeight = 0;
static double leftMargin = 0.0;
static double rightMargin = 0.0;
static double bottomMargin = 0.0;
static double topMargin = 0.0;
static GBool fit = gFalse;
static int rotate = 0;
#ifdef OPVP_SKIP_CHECK
static GBool skipCheck = gFalse;
#endif
static GBool noBitmapChar = gFalse;
static char bitmapCharThreshold[20] = "2000";
static char maxClipPathLength[20] = "2000";
static char maxFillPathLength[20] = "4000";

#ifndef HAVE_CUPSLIB
static ArgDesc argDesc[] = {
  {"-r",      argInt,      &resolution,    0,
   "resolution, in DPI (default is 300)"},
#if 0
  {"-hr",      argInt,      &hResolution,    0,
   "horizontal resolution, in DPI (default is 300)"},
  {"-vr",      argInt,      &vResolution,    0,
   "vertical resolution, in DPI (default is 300)"},
#endif
  {"-slice",      argInt,      &sliceHeight,    0,
   "slice height, in pixel (default is 100)"},
#if 0
  {"-paperWidth",      argInt,      &paperWidth,    0,
   "paper width, in mm"},
  {"-paperHeight",      argInt,      &paperHeight,    0,
   "paper height, in mm"},
#endif
  {"-rotate",      argInt,      &rotate,    0,
   "rotate, in degree"},
#if 0
  {"-mono",   argFlag,     &mono,          0,
   "generate a monochrome output"},
  {"-gray",   argFlag,     &gray,          0,
   "generate a grayscale output"},
#endif
  {"-raster",   argFlag,     &rasterMode,          0,
   "set raster mode"},
  {"-oldLipsDriver",   argFlag,     &oldLipsDriver,          0,
   "use old lips driver"},
  {"-HPDriver",   argFlag,     &HPDriver,          0,
   "use HP driver"},
  {"-NECDriver",   argFlag,     &HPDriver,          0,
   "use NEC driver"},
  {"-clipPathNotSaved",   argFlag,     &clipPathNotSaved,          0,
   "printer driver don't save clip path in GS"},
  {"-noShearImage",   argFlag,     &noShearImage,          0,
   "printer driver can't output shear image"},
  {"-noLineStyle",   argFlag,     &noLineStyle,          0,
   "printer driver can't handle LineStyle"},
  {"-noImageMask",   argFlag,     &noImageMask,          0,
   "printer driver can't handle imageMask"},
  {"-noClipPath",   argFlag,     &noClipPath,          0,
   "printer driver can't handle clipPath"},
  {"-noMiterLimit",   argFlag,     &noMiterLimit,          0,
   "printer driver can't handle miterLimit"},
  {"-ignoreMiterLimit",   argFlag,     &ignorenoMiterLimit,          0,
   "ignore miter limit , to handle miter limit appropriately is driver dependent"},
#ifdef OPVP_SKIP_CHECK
  {"-skipCheck",   argFlag,     &skipCheck,          0,
   "enable skip check"},
#endif
  {"-noBitmapChar",   argFlag,     &noBitmapChar,          0,
   "no char is out as a bitmask"},
  {"-bitmapCharThreshold",   argString,     bitmapCharThreshold,
    sizeof(bitmapCharThreshold),
   "threshold for out a char as a bitmap (default 2000)"},
  {"-maxClipPathLength",   argString,     maxClipPathLength,
    sizeof(maxClipPathLength),
   "max clip path length (default 2000)"},
  {"-maxFillPathLength",   argString,     maxFillPathLength,
    sizeof(maxFillPathLength),
   "max fill path length (default 4000)"},
#if 0
#if HAVE_T1LIB_H
  {"-t1lib",      argString,      enableT1libStr, sizeof(enableT1libStr),
   "enable t1lib font rasterizer: yes, no"},
#endif
#if HAVE_FREETYPE_FREETYPE_H | HAVE_FREETYPE_H
  {"-freetype",   argString,      enableFreeTypeStr, sizeof(enableFreeTypeStr),
   "enable FreeType font rasterizer: yes, no"},
#endif
  {"-q",      argFlag,     &quiet,         0,
   "don't print any messages or errors"},
#endif
  {"-cfg",        argString,      cfgFileName,    sizeof(cfgFileName),
   "configuration file to use in place of .xpdfrc"},
  {"-outputOrder",  argString,      outputOrderStr,    sizeof(outputOrderStr),
   "output page order (reverse or not)"},
  {"-page-ranges",  argString,      pageRangesStr,    sizeof(pageRangesStr),
   "output page ranges"},
  {"-page-set",  argString,      pageSetStr,    sizeof(pageSetStr),
   "output page set (even or odd)"},
  {"-driver",        argString,      printerDriver,    sizeof(printerDriver),
   "Vector Printer Driver Name"},
  {"-printer",        argString,      printerModel,    sizeof(printerModel),
   "Printer Model"},
  {"-jobInfo",        argString,      jobInfo,    sizeof(jobInfo),
   "Job Info"},
  {"-docInfo",        argString,      docInfo,    sizeof(docInfo),
   "Document Info"},
  {"-pageInfo",        argString,      pageInfo,    sizeof(pageInfo),
   "Page Info"},
#if 0
  {"-fit",      argFlag,      &fit,    0,
   "fit a content into a paper"},
  {"-paperSize",      argString,      paperSize,    sizeof(paperSize),
   "paper Size "},
  {"-v",      argFlag,     &printVersion,  0,
   "print copyright and version info"},
#endif
  {"-h",      argFlag,     &printHelp,     0,
   "print usage information"},
  {"-help",   argFlag,     &printHelp,     0,
   "print usage information"},
  {"--help",  argFlag,     &printHelp,     0,
   "print usage information"},
  {"-?",      argFlag,     &printHelp,     0,
   "print usage information"},
  {NULL}
};
#endif

static int checkRange(int page)
{
  const char *range;
  int lower, upper;

  if (pageSetStr[0] != '\0') {
    if (!strcasecmp(pageSetStr,"even") && (page % 2) == 1) {
      return (0);
    } else if (!strcasecmp(pageSetStr, "odd") && (page % 2) == 0) {
      return (0);
    }
  }

  if (pageRangesStr[0] == '\0') {
    return (1);
  }

  for (range = pageRangesStr; *range != '\0';) {
    if (*range == '-') {
      lower = 1;
      range ++;
      upper = strtol(range, (char **)&range, 10);
    } else {
      lower = strtol(range, (char **)&range, 10);
      if (*range == '-') {
        range ++;
	if (!isdigit(*range)) {
	  upper = 65535;
	} else {
	  upper = strtol(range, (char **)&range, 10);
	}
      } else {
        upper = lower;
      }
    }
    if (page >= lower && page <= upper) {
      return (1);
    }

    if (*range == ',') {
      range++;
    } else {
      break;
    }
  }
  return (0);
}

static int outOnePage(PDFDoc *doc, OPVPOutputDev *opvpOut, int pg)
{
  int slice;
  int tmpRotate = rotate;
  double rw = 1.0, rh = 1.0;
  double lm = 0, bm = 0;
  int rt = tmpRotate + doc->getPageRotate(pg);

  if (checkRange(pg) == 0) {
    return 0;
  }
  if (fit) {
    PDFRectangle *artBox,*cropBox;
    double aw, ah, cw, ch;
    int docWidth, docHeight;

    artBox = doc->getCatalog()->getPage(pg)->getArtBox();
    aw = artBox->x2-artBox->x1;
    ah = artBox->y2-artBox->y1;
    cropBox = doc->getCatalog()->getPage(pg)->getCropBox();
    cw = cropBox->x2-cropBox->x1;
    ch = cropBox->y2-cropBox->y1;
    if ((rt % 180) == 90) {
      double t;

      t = ah;
      ah = aw;
      aw = t;
      t = ch;
      ch = cw;
      cw = t;
    }

    if (rotate == 0 && ((aw > ah && paperWidth < paperHeight)
       || (aw < ah && paperWidth > paperHeight))) {
      double t;

      tmpRotate += 90;
      rt += 90;
      t = ah;
      ah = aw;
      aw = t;
      t = ch;
      ch = cw;
      cw = t;
#if 0
fprintf(stderr,"rotate\n");
#endif
    }
    switch (rt % 360) {
    case 90:
	lm = leftMargin*hResolution/72;
	bm = topMargin*vResolution/72;
	break;
    case 180:
	lm = rightMargin*hResolution/72;
	bm = topMargin*vResolution/72;
	break;
    case 270:
	lm = rightMargin*hResolution/72;
	bm = bottomMargin*vResolution/72;
	break;
    default:
	lm = leftMargin*hResolution/72;
	bm = bottomMargin*vResolution/72;
	break;
    }
    if (cw < aw) aw = cw;
    if (ch < ah) ah = ch;

    docWidth = (int)(aw*hResolution/72+0.5);
    docHeight = (int)(ah*vResolution/72+0.5);
#if 0
fprintf(stderr,"paperWidth=%d, paperHeight=%d, docWidth=%d, docHeight=%d\n",
  paperWidth, paperHeight, docWidth, docHeight);
fprintf(stderr,"printableWidth=%d, printableHeight=%d\n",
  printableWidth, printableHeight);
#endif
    if (docWidth > printableWidth || docHeight > printableHeight) {
      rw = ((double)printableWidth)/docWidth;
      rh = ((double)printableHeight)/docHeight;
      if (rw < rh) {
	rh = rw;
      } else {
	rw = rh;
      }
#if 0
fprintf(stderr,"scale %g,%g, %g, %g\n",rw,rh,lm,bm);
#endif
    }
  }

  if (opvpOut->OPVPStartPage(pageInfo,paperWidth) < 0) {
      error(-1,"Start Page failed");
      return 2;
  }
  if (!rasterMode) {
    /* When printer is a vector printer, we output all region of the page
       in once. */
    sliceHeight = paperHeight;
  }
#ifdef OPVP_SKIP_CHECK
  Gfx::opvpSkipCheck.clean(rasterMode && skipCheck);
#endif
  for (slice = 0;slice < paperHeight;slice += sliceHeight) {
      int sh = paperHeight-slice;

      if (sh > sliceHeight) sh = sliceHeight;
#ifdef OPVP_SKIP_CHECK
      Gfx::opvpSkipCheck.rewind(slice);
#endif
      opvpOut->setScale(rw,rh,lm,bm,rt,slice,sh);
      doc->displayPageSlice(opvpOut, pg, resolution, resolution,
	tmpRotate, gTrue, gTrue, gFalse,
	0,slice,paperWidth,sh);
      if (opvpOut->outSlice() < 0) {
	error(-1,"OutSlice failed");
	return 2;
      }
  }
  if (opvpOut->OPVPEndPage() < 0) {
      error(-1,"End Page failed");
      return 2;
  }
  return 0;
}

#define MAX_OPVP_OPTIONS 20

void CDECL myErrorFun(int pos, char *msg, va_list args)
{
  if (pos >= 0) {
    fprintf(stderr, "ERROR (%d): ", pos);
  } else {
    fprintf(stderr, "ERROR: ");
  }
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\n");
  fflush(stderr);
}

int main(int argc, char *argv[]) {
/* mtrace(); */
  int exitCode;
{
  PDFDoc *doc;
  SplashColor paperColor;
  OPVPOutputDev *opvpOut;
  GBool ok = gTrue;
  int pg;
  int c;
  char *optionKeys[MAX_OPVP_OPTIONS];
  char *optionVals[MAX_OPVP_OPTIONS];
  int nOptions = 0;
  int i;
  double pageWidth = -1, pageLength = -1;
  double pw = -1, ph = -1;
  GooString fileName;

  exitCode = 99;
  setErrorFunction(::myErrorFun);

  // parse args
#ifdef HAVE_CUPSLIB
  int num_options;
  cups_option_t *options;
  const char *val;
  char *ppdFileName;
  ppd_file_t *ppd = 0;
  ppd_attr_t *attr;
  GooString jobInfoStr;
  GooString docInfoStr;
  GooString pageInfoStr;


  if (argc < 6 || argc > 7) {
    error(-1,"ERROR: %s job-id user title copies options [file]",
      argv[0]);
    return (1);
  }

  if ((ppdFileName = getenv("PPD")) != 0) {
    if ((ppd = ppdOpenFile(ppdFileName)) != 0) {
      /* get attributes from PPD File */
      if ((attr = ppdFindAttr(ppd,"opvpJobInfo",0)) != 0) {
	jobInfoStr.append(attr->value);
      }
      if ((attr = ppdFindAttr(ppd,"opvpDocInfo",0)) != 0) {
	docInfoStr.append(attr->value);
      }
      if ((attr = ppdFindAttr(ppd,"opvpPageInfo",0)) != 0) {
	pageInfoStr.append(attr->value);
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpSlice",0)) != 0) {
	sliceHeight = atoi(attr->value);
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpRaster",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  rasterMode = gTrue;
	} else {
	  rasterMode = gFalse;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpOldLipsDriver",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  oldLipsDriver = gTrue;
	} else {
	  oldLipsDriver = gFalse;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpHPDriver",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  HPDriver = gTrue;
	} else {
	  HPDriver = gFalse;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpNECDriver",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  NECDriver = gTrue;
	} else {
	  NECDriver = gFalse;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpClipPathNotSaved",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  clipPathNotSaved = gTrue;
	} else {
	  clipPathNotSaved = gFalse;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpShearImage",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  noShearImage = gFalse;
	} else {
	  noShearImage = gTrue;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpLineStyle",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  noLineStyle = gFalse;
	} else {
	  noLineStyle = gTrue;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpImageMask",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  noImageMask = gFalse;
	} else {
	  noImageMask = gTrue;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpClipPath",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  noClipPath = gFalse;
	} else {
	  noClipPath = gTrue;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpMiterLimit",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  noMiterLimit = gFalse;
	} else {
	  noMiterLimit = gTrue;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpIgnoreMiterLimit",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  ignoreMiterLimit = gTrue;
	} else {
	  ignoreMiterLimit = gFalse;
	}
      }
#ifdef OPVP_SKIP_CHECK
      if ((attr = ppdFindAttr(ppd,"pdftoopvpSkipCheck",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  skipCheck = gTrue;
	} else {
	  skipCheck = gFalse;
	}
      }
#endif
      if ((attr = ppdFindAttr(ppd,"pdftoopvpBitmapCharThreshold",0)) != 0) {
	strncpy(bitmapCharThreshold,attr->value,
	  sizeof(bitmapCharThreshold)-1);
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpMaxClipPathLength",0)) != 0) {
	strncpy(maxClipPathLength,attr->value,
	  sizeof(maxClipPathLength)-1);
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpMaxFillPathLength",0)) != 0) {
	strncpy(maxFillPathLength,attr->value,
	  sizeof(maxFillPathLength)-1);
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpBitmapChar",0)) != 0) {
	if (strcasecmp(attr->value,"true") == 0) {
	  noBitmapChar = gFalse;
	} else {
	  noBitmapChar = gTrue;
	}
      }
      if ((attr = ppdFindAttr(ppd,"pdftoopvpConfig",0)) != 0) {
          strncpy(cfgFileName,attr->value,sizeof(cfgFileName)-1);
	  cfgFileName[sizeof(cfgFileName)-1] = '\0';
      }
      if ((attr = ppdFindAttr(ppd,"opvpDriver",0)) != 0) {
          strncpy(printerDriver,attr->value,sizeof(printerDriver)-1);
	  printerDriver[sizeof(printerDriver)-1] = '\0';
      }
      if ((attr = ppdFindAttr(ppd,"opvpModel",0)) != 0) {
          strncpy(printerModel,attr->value,sizeof(printerModel)-1);
	  printerModel[sizeof(printerModel)-1] = '\0';
      }
      ppdMarkDefaults(ppd);
    }
  }
  /* get attributes and options from command line option */
  num_options = cupsParseOptions(argv[5],0,&options);
  cupsMarkOptions(ppd,num_options,options);

  for (i = 0;i < num_options;i++) {
#ifdef NOPDFTOPDF
    if (strcasecmp(options[i].name,"OutputOrder") == 0) {
      strncpy(outputOrderStr,options[i].value,sizeof(outputOrderStr)-1);
      outputOrderStr[sizeof(outputOrderStr)-1] = '\0';
    } else if (strcasecmp(options[i].name,"page-ranges") == 0) {
      strncpy(pageRangesStr,options[i].value,sizeof(pageRangesStr)-1);
      pageRangesStr[sizeof(pageRangesStr)-1] = '\0';
    } else if (strcasecmp(options[i].name,"page-set") == 0) {
      strncpy(pageSetStr,options[i].value,sizeof(pageSetStr)-1);
      pageSetStr[sizeof(pageSetStr)-1] = '\0';
    } else if (strcasecmp(options[i].name,"Resolution") == 0) {
#else
    if (strcasecmp(options[i].name,"Resolution") == 0) {
#endif
      resolution = atoi(options[i].value);
#ifdef NOPDFTOPDF
    } else if (strcasecmp(options[i].name,"Duplex") == 0) {
      duplex = gTrue;
    } else if (strcasecmp(options[i].name,"pdftoopvpFit") == 0
      || strcasecmp(options[i].name,"fit") == 0) {
      fit = gTrue;
#endif
    } else if (strcasecmp(options[i].name,"pdftoopvpSlice") == 0) {
      sliceHeight = atoi(options[i].value);
#ifdef NOPDFTOPDF
    } else if (strcasecmp(options[i].name,"pdftoopvpRotate") == 0) {
      rotate = atoi(options[i].value);
#endif
    } else if (strcasecmp(options[i].name,"pdftoopvpRaster") == 0) {
      rasterMode = gTrue;
    } else if (strcasecmp(options[i].name,"pdftoopvpOldLipsDriver") == 0) {
      oldLipsDriver = gTrue;
    } else if (strcasecmp(options[i].name,"pdftoopvpHPDriver") == 0) {
      HPDriver = gTrue;
    } else if (strcasecmp(options[i].name,"pdftoopvpNECDriver") == 0) {
      NECDriver = gTrue;
    } else if (strcasecmp(options[i].name,"pdftoopvpClipPathNotSaved") == 0) {
      clipPathNotSaved = gTrue;
    } else if (strcasecmp(options[i].name,"pdftoopvpShearImage") == 0) {
      if (strcasecmp(options[i].value,"false") == 0) {
	noShearImage = gTrue;
      }
    } else if (strcasecmp(options[i].name,"pdftoopvpLineStyle") == 0) {
      if (strcasecmp(options[i].value,"false") == 0) {
	noLineStyle = gTrue;
      }
    } else if (strcasecmp(options[i].name,"pdftoopvpImageMask") == 0) {
      if (strcasecmp(options[i].value,"false") == 0) {
	noImageMask = gTrue;
      }
    } else if (strcasecmp(options[i].name,"pdftoopvpClipPath") == 0) {
      if (strcasecmp(options[i].value,"false") == 0) {
	noClipPath = gTrue;
      }
    } else if (strcasecmp(options[i].name,"pdftoopvpMiterLimit") == 0) {
      if (strcasecmp(options[i].value,"false") == 0) {
	noMiterLimit = gTrue;
      }
    } else if (strcasecmp(options[i].name,"pdftoopvpIgnoreMiterLimit") == 0) {
      if (strcasecmp(options[i].value,"true") == 0) {
	ignoreMiterLimit = gTrue;
      }
    }
  #ifdef OPVP_SKIP_CHECK
     else if (strcasecmp(options[i].name,"pdftoopvpSkipCheck") == 0) {
      skipCheck = gTrue;
    }
  #endif
     else if (strcasecmp(options[i].name,"pdftoopvpBitmapChar") == 0) {
      if (strcasecmp(options[i].value,"false") == 0) {
	noBitmapChar = gTrue;
      }
    } else if (strcasecmp(options[i].name,"pdftoopvpBitmapCharThreshold") == 0) {
      strncpy(bitmapCharThreshold,options[i].value,
        sizeof(bitmapCharThreshold)-1);
    } else if (strcasecmp(options[i].name,"pdftoopvpMaxClipPathLength") == 0) {
      strncpy(maxClipPathLength,options[i].value,
        sizeof(maxClipPathLength)-1);
    } else if (strcasecmp(options[i].name,"pdftoopvpMaxFillPathLength") == 0) {
      strncpy(maxFillPathLength,options[i].value,
        sizeof(maxFillPathLength)-1);
    } else if (strcasecmp(options[i].name,"pdftoopvpConfig") == 0) {
      strncpy(cfgFileName,options[i].value,sizeof(cfgFileName)-1);
      cfgFileName[sizeof(cfgFileName)-1] = '\0';
    } else if (strcasecmp(options[i].name,"opvpDriver") == 0) {
      strncpy(printerDriver,options[i].value,sizeof(printerDriver)-1);
      printerDriver[sizeof(printerDriver)-1] = '\0';
    } else if (strcasecmp(options[i].name,"opvpModel") == 0) {
      strncpy(printerModel,options[i].value,sizeof(printerModel)-1);
      printerModel[sizeof(printerModel)-1] = '\0';
    } else if (strcasecmp(options[i].name,"opvpJobInfo") == 0) {
      /* do nothing here */;
    } else if (strcasecmp(options[i].name,"opvpDocInfo") == 0) {
      /* do nothing here */;
    } else if (strcasecmp(options[i].name,"opvpPageInfo") == 0) {
      /* do nothing here */;
    }
  }
  if (ppd != 0) {
    int section;
    ppd_choice_t **choices;
    ppd_size_t *pagesize;

    if ((pagesize = ppdPageSize(ppd,0)) != 0) {
      pageWidth = pagesize->width;
      pageLength = pagesize->length;
      pw = pagesize->right - pagesize->left;
      ph = pagesize->top - pagesize->bottom;
      bottomMargin = pagesize->bottom;
      topMargin = pagesize->length - pagesize->top;
      leftMargin = pagesize->left;
      rightMargin = pagesize->width - pagesize->right;
    }

    for (section = (int)PPD_ORDER_ANY;
      section <= (int)PPD_ORDER_PROLOG;section++) {
      int n;

      n = ppdCollect(ppd,(ppd_section_t)section,&choices);
      for (i = 0;i < n;i++) {
	char buf[1024];

	if (strcasecmp(((ppd_option_t *)(choices[i]->option))->keyword,
	   "Resolution") == 0) {
	  resolution = atoi(choices[i]->choice);
#ifdef NOPDFTOPDF
	} else if (strcasecmp(((ppd_option_t *)(choices[i]->option))->keyword,
	   "Duplex") == 0) {
	  if (strcasecmp(choices[i]->choice, "no") != 0 
	     && strcasecmp(choices[i]->choice, "off") != 0
	     && strcasecmp(choices[i]->choice, "false") != 0)
	    duplex = gTrue;
	} else if (strcasecmp(((ppd_option_t *)(choices[i]->option))->keyword,
	   "pdftoopvpFit") == 0) {
	  if (strcasecmp(choices[i]->choice, "no") != 0 
	     && strcasecmp(choices[i]->choice, "off") != 0
	     && strcasecmp(choices[i]->choice, "false") != 0)
	    fit = gTrue;
#endif
	}
#ifdef NOPDFTOPDF
	snprintf(buf,1024,"pdftoopvp%s%s",
	  ((ppd_option_t *)(choices[i]->option))->keyword,
	  choices[i]->choice);
	if ((attr = ppdFindAttr(ppd,buf,0)) != 0) {
	  if (jobInfoStr.getLength() > 0) jobInfoStr.append(";");
	  jobInfoStr.append(attr->value);
	}
#endif
      }
      if (choices != 0) free(choices);
    }

    strncpy(jobInfo,jobInfoStr.getCString(),sizeof(jobInfo)-1);
    jobInfo[sizeof(jobInfo)-1] = '\0';
    strncpy(docInfo,docInfoStr.getCString(),sizeof(docInfo)-1);
    docInfo[sizeof(docInfo)-1] = '\0';
    strncpy(pageInfo,pageInfoStr.getCString(),sizeof(pageInfo)-1);
    pageInfo[sizeof(pageInfo)-1] = '\0';
    ppdClose(ppd);
  }
  if ((val = cupsGetOption("opvpJobInfo",num_options, options)) != 0) {
    /* override ppd value */
    strncpy(jobInfo,val,sizeof(jobInfo)-1);
    jobInfo[sizeof(jobInfo)-1] = '\0';
  }
  if ((val = cupsGetOption("opvpDocInfo",num_options, options)) != 0) {
    /* override ppd value */
    strncpy(docInfo,val,sizeof(docInfo)-1);
    docInfo[sizeof(docInfo)-1] = '\0';
  }
  if ((val = cupsGetOption("opvpPageInfo",num_options, options)) != 0) {
    /* override ppd value */
    strncpy(pageInfo,val,sizeof(pageInfo)-1);
    pageInfo[sizeof(pageInfo)-1] = '\0';
  }
#ifdef NOPDFTOPDF
  if ((val = cupsGetOption("landscape",num_options,options)) != NULL) {
    if (strcasecmp(val, "no") != 0 && strcasecmp(val, "off") != 0 &&
        strcasecmp(val, "false") != 0)
      rotate = 90;
  } else if ((val = cupsGetOption("orientation-requested",
     num_options, options)) != NULL) {
   /*
    * Map IPP orientation values to 0 to 3:
    *
    *   3 = 0 degrees   = 0
    *   4 = 90 degrees  = 1
    *   5 = -90 degrees = 3
    *   6 = 180 degrees = 2
    */

    rotate = atoi(val) - 3;
    if (rotate >= 2) {
      rotate ^= 1;
    }
    rotate *= 90;
  }
#endif

  cupsFreeOptions(num_options,options);
#if 0
  /* for debug parameters */
  fprintf(stderr,"WARNING:resolution=%d\n",resolution);
  fprintf(stderr,"WARNING:sliceHeight=%d\n",sliceHeight);
  fprintf(stderr,"WARNING:cfgFileName=%s\n",cfgFileName);
  fprintf(stderr,"WARNING:rasterMode=%d\n",rasterMode);
  fprintf(stderr,"WARNING:oldLipsDriver=%d\n",oldLipsDriver);
  fprintf(stderr,"WARNING:HPDriver=%d\n",HPDriver);
  fprintf(stderr,"WARNING:NECDriver=%d\n",NECDriver);
  fprintf(stderr,"WARNING:clipPathNotSaved=%d\n",clipPathNotSaved);
  fprintf(stderr,"WARNING:noShearImage=%d\n",noShearImage);
  fprintf(stderr,"WARNING:noLineStyle=%d\n",noLineStyle);
  fprintf(stderr,"WARNING:noClipPath=%d\n",noClipPath);
  fprintf(stderr,"WARNING:noMiterLimit=%d\n",noMiterLimit);
  fprintf(stderr,"WARNING:printerDriver=%s\n",printerDriver);
  fprintf(stderr,"WARNING:printerModel=%s\n",printerModel);
  fprintf(stderr,"WARNING:jobInfo=%s\n",jobInfo);
  fprintf(stderr,"WARNING:docInfo=%s\n",docInfo);
  fprintf(stderr,"WARNING:pageInfo=%s\n",pageInfo);
  fprintf(stderr,"WARNING:paperWidth=%d\n",paperWidth);
  fprintf(stderr,"WARNING:paperHeight=%d\n",paperHeight);
  fprintf(stderr,"WARNING:rotate=%d\n",rotate);
#ifdef OPVP_SKIP_CHECK
  fprintf(stderr,"WARNING:skipCheck=%d\n",skipCheck);
#endif
  fprintf(stderr,"WARNING:noBitmapChar=%d\n",noBitmapChar);
  fprintf(stderr,"WARNING:bitmapCharThreshold=%s\n",bitmapCharThreshold);
  fprintf(stderr,"WARNING:maxClipPathLength=%s\n",maxClipPathLength);
  fprintf(stderr,"WARNING:maxFillPathLength=%s\n",maxFillPathLength);
exit(0);
#endif

#else
  ok = parseArgs(argDesc, &argc, argv);
  if (mono && gray) {
    ok = gFalse;
  }
  if (!ok || printVersion || printHelp || argc < 2) {
        fprintf(stderr, "xpdf version %s\n", xpdfVersion);
        fprintf(stderr, "pdftoopvp version %s\n", pdftoopvpVersion);
        fprintf(stderr,"%s\n", xpdfCopyright);
        fprintf(stderr,"%s\n", pdftoopvpCopyright);
        if (!printVersion) {
            printUsage("pdftoopvp", "<PDF-file>", argDesc);
        }
        return 2;
  }
#endif
  if (oldLipsDriver) {
    optionKeys[nOptions] = "OPVP_OLDLIPSDRIVER";
    optionVals[nOptions] = "1";
    nOptions++;
    clipPathNotSaved = gTrue;
    noShearImage = gTrue;
  }
  if (HPDriver) {
    noClipPath = gTrue;
    noLineStyle = gTrue;
    noShearImage = gTrue;
  }
  if (NECDriver) {
    noMiterLimit = gTrue;
    strcpy(maxClipPathLength,"6");
    noShearImage = gTrue;
  }
  if (clipPathNotSaved) {
    optionKeys[nOptions] = "OPVP_CLIPPATHNOTSAVED";
    optionVals[nOptions] = "1";
    nOptions++;
  }
  if (noShearImage) {
    optionKeys[nOptions] = "OPVP_NOSHEARIMAGE";
    optionVals[nOptions] = "1";
    nOptions++;
  }
  if (noLineStyle) {
    optionKeys[nOptions] = "OPVP_NOLINESTYLE";
    optionVals[nOptions] = "1";
    nOptions++;
  }
  if (noImageMask) {
    optionKeys[nOptions] = "OPVP_NOIMAGEMASK";
    optionVals[nOptions] = "1";
    nOptions++;
  }
  if (noClipPath) {
    optionKeys[nOptions] = "OPVP_NOCLIPPATH";
    optionVals[nOptions] = "1";
    nOptions++;
  }
  if (noMiterLimit) {
    optionKeys[nOptions] = "OPVP_NOMITERLIMIT";
    optionVals[nOptions] = "1";
    nOptions++;
  }
  if (noBitmapChar) {
    optionKeys[nOptions] = "OPVP_NOBITMAPCHAR";
    optionVals[nOptions] = "1";
    nOptions++;
  }
  if (ignoreMiterLimit) {
    optionKeys[nOptions] = "OPVP_IGNOREMITERLIMIT";
    optionVals[nOptions] = "1";
    nOptions++;
  }
  optionKeys[nOptions] = "OPVP_BITMAPCHARTHRESHOLD";
  optionVals[nOptions] = bitmapCharThreshold;
  nOptions++;
  optionKeys[nOptions] = "OPVP_MAXCLIPPATHLENGTH";
  optionVals[nOptions] = maxClipPathLength;
  nOptions++;
  optionKeys[nOptions] = "OPVP_MAXFILLPATHLENGTH";
  optionVals[nOptions] = maxFillPathLength;
  nOptions++;
  if (hResolution == 0) hResolution = resolution;
  if (hResolution == 0) hResolution = resolution;
  if (vResolution == 0) vResolution = resolution;
  if (strcasecmp(outputOrderStr,"reverse") == 0) {
    reverse = gTrue;
  }

#ifdef HAVE_CUPSLIB
  if (argc > 6) {
    fileName.append(argv[6]);
#ifdef NOPDFTOPDF
    copies = atoi(argv[4]);
#endif
  } else {
    fileName.append("-");
  }
#else
  fileName.append(argv[1]);
#endif

  // read config file
  globalParams = new GlobalParams(cfgFileName);
  if (enableT1libStr[0]) {
    if (!globalParams->setEnableT1lib(enableT1libStr)) {
      error(-1,"Bad '-t1lib' value on command line");
      ok = gFalse;
    }
  }
  if (enableFreeTypeStr[0]) {
    if (!globalParams->setEnableFreeType(enableFreeTypeStr)) {
      error(-1,"Bad '-freetype' value on command line");
      ok = gFalse;
    }
  }
  globalParams->setAntialias("no");
  if (quiet) {
    globalParams->setErrQuiet(quiet);
  }
  if (!ok) {
    exitCode = 2;
    goto err0;
  }

  if (fileName.cmp("-") == 0) {
    /* stdin */
    char *s;
    GooString name;
    int fd;
    Object obj;
    BaseStream *str;
    FILE *fp;
    char buf[4096];
    int n;

    /* create a tmp file */
    if ((s = getenv("TMPDIR")) != 0) {
      name.append(s);
    } else {
      name.append("/tmp");
    }
    name.append("/XXXXXX");
    fd = mkstemp(name.getCString());
    /* remove name */
    unlink(name.getCString());
    if (fd < 0) {
      error(-1,"Can't create temporary file");
      exitCode = 2;
      goto err0;
    }

    /* check JCL */
    while (fgets(buf,sizeof(buf)-1,stdin) != NULL
        && strncmp(buf,"%PDF",4) != 0) {
      if (strncmp(buf,"pdftoopvp jobInfo:",18) == 0) {
	/* JCL jobInfo exists, override jobInfo */
	strncpy(jobInfo,buf+18,sizeof(jobInfo)-1);
	for (i = sizeof(jobInfo)-2;i >= 0
	  && (jobInfo[i] == 0 || jobInfo[i] == '\n' || jobInfo[i] == ';')
	  ;i--);
	jobInfo[i+1] = 0;
      }
    }
    if (strncmp(buf,"%PDF",4) != 0) {
      error(-1,"Can't find PDF header");
      exitCode = 2;
      goto err0;
    }
    /* copy PDF header */
    n = strlen(buf);
    if (write(fd,buf,n) != n) {
      error(-1,"Can't copy stdin to temporary file");
      close(fd);
      exitCode = 2;
      goto err0;
    }
    /* copy rest stdin to the tmp file */
    while ((n = fread(buf,1,sizeof(buf),stdin)) > 0) {
      if (write(fd,buf,n) != n) {
	error(-1,"Can't copy stdin to temporary file");
	close(fd);
	exitCode = 2;
	goto err0;
      }
    }
    if (lseek(fd,0,SEEK_SET) < 0) {
	error(-1,"Can't rewind temporary file");
	close(fd);
	exitCode = 2;
	goto err0;
    }

    if ((fp = fdopen(fd,"rb")) == 0) {
	error(-1,"Can't fdopen temporary file");
	close(fd);
	exitCode = 2;
	goto err0;
    }

    obj.initNull();
    str = new FileStream(fp,0,gFalse,0,&obj);
    doc = new PDFDoc(str);
  } else {
    /* no jcl check */
    doc = new PDFDoc(fileName.copy());
  }
  if (!doc->isOk()) {
    error(-1," Parsing PDF failed: error code %d",
      doc->getErrorCode());
    exitCode = 2;
    goto err05;
  }

  if (doc->isEncrypted() && !doc->okToPrint()) {
    error(-1,"Print Permission Denied");
    exitCode = 2;
    goto err05;
  }

  // get page range
  if (firstPage < 1)
    firstPage = 1;
  if (lastPage < 1 || lastPage > doc->getNumPages())
    lastPage = doc->getNumPages();

  if (pageWidth >= 0) {
	paperWidth = (int)(pageWidth*hResolution/72+0.5);
  } else {
	paperWidth = (int)(doc->getPageMediaWidth(1)*hResolution/72+0.5);
  }
  if (pageLength >= 0) {
	paperHeight = (int)(pageLength*vResolution/72+0.5);
  } else {
	paperHeight = (int)(doc->getPageMediaHeight(1)*vResolution/72+0.5);
  }
  if (pw == 0) {
    printableWidth = paperWidth;
  } else {
    printableWidth = (int)(pw*hResolution/72+0.5);
  }
  if (ph == 0) {
    printableHeight = paperHeight;
  } else {
    printableHeight = (int)(ph*vResolution/72+0.5);
  }
  /* paperColor is white */
  paperColor[0] = 255;
  paperColor[1] = 255;
  paperColor[2] = 255;
  opvpOut = new OPVPOutputDev();
  if (opvpOut->init(mono ? splashModeMono1 :
				    gray ? splashModeMono8 :
				             splashModeRGB8,
				  gFalse, paperColor, rasterMode,
                                 printerDriver,1,printerModel,
				 paperWidth, paperHeight,
				 nOptions,optionKeys,optionVals) < 0) {
      error(-1,"OPVPOutputDev Initialize fail");
      exitCode = 2;
      goto err1;
  }

  rasterMode = opvpOut->getRasterMode();

  opvpOut->startDoc(doc->getXRef());

#if 0
fprintf(stderr,"JobInfo=%s\n",jobInfo);
#endif
  if (opvpOut->OPVPStartJob(jobInfo) < 0) {
      error(-1,"Start job failed");
      exitCode = 2;
      goto err1;
  }
  for (c = 0;c < copies;c++) {
    if (opvpOut->OPVPStartDoc(docInfo) < 0) {
	error(-1,"Start Document failed");
	exitCode = 2;
	goto err2;
    }
    if (reverse) {
      for (pg = lastPage; pg >= firstPage; --pg) {
	if ((exitCode = outOnePage(doc,opvpOut,pg)) != 0) break;
      }
    } else {
      for (pg = firstPage; pg <= lastPage; ++pg) {
	if ((exitCode = outOnePage(doc,opvpOut,pg)) != 0) break;
      }
    }
    if (opvpOut->OPVPEndDoc() < 0) {
	error(-1,"End Document failed");
	exitCode = 2;
    }
    if (exitCode != 0) break;
  }
err2:
  if (opvpOut->OPVPEndJob() < 0) {
      error(-1,"End job failed");
      exitCode = 2;
  }

  // clean up
 err1:
  delete opvpOut;
 err05:
  delete doc;
 err0:
  delete globalParams;

  // check for memory leaks
  Object::memCheck(stderr);
  gMemReport(stderr);

}
/* muntrace(); */
  return exitCode;
}

/* for memory debug */
void *operator new(size_t size)
{
    void *p = malloc(size);
    return p;
}

void operator delete(void *p)
{
    free(p);
}
