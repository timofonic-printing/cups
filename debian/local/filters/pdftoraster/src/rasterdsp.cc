#include <stdio.h>
#include <cups/raster.h>

static char *prefix = NULL;

static void printHeader(cups_page_header_t *hp)
{
    char *p;

    printf("MediaClass = %s\n",hp->MediaClass);

    printf("MediaColor = %s\n",hp->MediaColor);

    printf("MediaType = %s\n",hp->MediaType);

    printf("OutputType = %s\n",hp->OutputType);

    printf("AdvanceDistance = %d points\n",hp->AdvanceDistance);

    switch (hp->AdvanceMedia) {
    case CUPS_ADVANCE_NONE:
	p = "NONE";
	break;
    case CUPS_ADVANCE_FILE:
	p = "FILE";
	break;
    case CUPS_ADVANCE_JOB:
	p = "JOB";
	break;
    case CUPS_ADVANCE_SET:
	p = "SET";
	break;
    case CUPS_ADVANCE_PAGE:
	p = "PAGE";
	break;
    default:
	p = "Unknown";
	break;
    }
    printf("AdvanceMedia = %s\n",p);

    if (hp->Collate) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("Collate = %s\n",p);

    switch (hp->CutMedia) {
    case CUPS_CUT_NONE:
	p = "NONE";
	break;
    case CUPS_CUT_FILE:
	p = "FILE";
	break;
    case CUPS_CUT_JOB:
	p = "JOB";
	break;
    case CUPS_CUT_SET:
	p = "SET";
	break;
    case CUPS_CUT_PAGE:
	p = "PAGE";
	break;
    default:
	p = "Unknown";
	break;
    }
    printf("CutMedia = %s\n",p);

    if (hp->Duplex) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("Duplex = %s\n",p);

    printf("HWResolution %dx%d dpi\n",hp->HWResolution[0],hp->HWResolution[1]);

    printf("ImagingBoundingBox %d, %d, %d, %d\n",
      hp->ImagingBoundingBox[0],
      hp->ImagingBoundingBox[1],
      hp->ImagingBoundingBox[2],
      hp->ImagingBoundingBox[3]);

    if (hp->InsertSheet) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("InsertSheet = %s\n",p);

    switch (hp->Jog) {
    case CUPS_JOG_NONE:
	p = "NONE";
	break;
    case CUPS_JOG_FILE:
	p = "FILE";
	break;
    case CUPS_JOG_JOB:
	p = "JOB";
	break;
    case CUPS_JOG_SET:
	p = "SET";
	break;
    default:
	p = "Unknown";
	break;
    }
    printf("Jog = %s\n",p);

    switch (hp->LeadingEdge) {
    case CUPS_EDGE_TOP:
	p = "TOP";
	break;
    case CUPS_EDGE_RIGHT:
	p = "RIGHT";
	break;
    case CUPS_EDGE_BOTTOM:
	p = "BOTTOM";
	break;
    case CUPS_EDGE_LEFT:
	p = "LEFT";
	break;
    default:
	p = "Unknown";
	break;
    }
    printf("LeadingEdge = %s\n",p);

    printf("Margins %d, %d\n",hp->Margins[0],hp->Margins[1]);

    if (hp->ManualFeed) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("ManualFeed = %s\n",p);

    printf("MediaPosition %d\n",hp->MediaPosition);

    printf("MediaWeight %d g/m^2\n",hp->MediaWeight);

    if (hp->MirrorPrint) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("MirrorPrint = %s\n",p);

    if (hp->NegativePrint) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("NegativePrint = %s\n",p);

    printf("NumCopies %d\n",hp->NumCopies);

    switch (hp->Orientation) {
    case CUPS_ORIENT_0:
	p = "0";
	break;
    case CUPS_ORIENT_90:
	p = "90";
	break;
    case CUPS_ORIENT_180:
	p = "180";
	break;
    case CUPS_ORIENT_270:
	p = "270";
	break;
    default:
	p = "Unknown";
	break;
    }
    printf("Orientation = %s\n",p);

    if (hp->OutputFaceUp) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("OutputFaceUp = %s\n",p);

    printf("PageSize %dx%d points\n",hp->PageSize[0],hp->PageSize[1]);

    if (hp->Separations) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("Separations = %s\n",p);

    if (hp->TraySwitch) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("TraySwitch = %s\n",p);

    if (hp->Tumble) {
	p = "yes";
    } else {
	p = "no";
    }
    printf("Tumble = %s\n",p);

    printf("cupsWidth = %d pixels\n",hp->cupsWidth);

    printf("cupsHeight = %d pixels\n",hp->cupsHeight);

    printf("cupsMediaType = %d\n",hp->cupsMediaType);

    printf("cupsBitsPerColor = %d bits\n",hp->cupsBitsPerColor);

    printf("cupsBitsPerPixel = %d bits\n",hp->cupsBitsPerPixel);

    printf("cupsBytesPerLine = %d bytes\n",hp->cupsBytesPerLine);

    switch (hp->cupsColorOrder) {
    case CUPS_ORDER_CHUNKED:
	p = "CHUNKED";
	break;
    case CUPS_ORDER_BANDED:
	p = "BANDED";
	break;
    case CUPS_ORDER_PLANAR:
	p = "PLANAR";
	break;
    default:
	p = "Unknown";
	break;
    }
    printf("cupsColorOrder = %s\n",p);

    switch (hp->cupsColorSpace) {
    case CUPS_CSPACE_W:
	p = "W";
	break;
    case CUPS_CSPACE_RGB:
	p = "RGB";
	break;
    case CUPS_CSPACE_RGBA:
	p = "RGBA";
	break;
    case CUPS_CSPACE_K:
	p = "K";
	break;
    case CUPS_CSPACE_CMY:
	p = "CMY";
	break;
    case CUPS_CSPACE_YMC:
	p = "YMC";
	break;
    case CUPS_CSPACE_CMYK:
	p = "CMYK";
	break;
    case CUPS_CSPACE_YMCK:
	p = "YMCK";
	break;
    case CUPS_CSPACE_KCMY:
	p = "KCMY";
	break;
    case CUPS_CSPACE_KCMYcm:
	p = "KCMYcm";
	break;
    case CUPS_CSPACE_GMCK:
	p = "GMCK";
	break;
    case CUPS_CSPACE_GMCS:
	p = "GMCS";
	break;
    case CUPS_CSPACE_WHITE:
	p = "WHITE";
	break;
    case CUPS_CSPACE_GOLD:
	p = "GOLD";
	break;
    case CUPS_CSPACE_SILVER:
	p = "SILVER";
	break;
    case CUPS_CSPACE_CIEXYZ:
	p = "CIEXYZ";
	break;
    case CUPS_CSPACE_CIELab:
	p = "CIELab";
	break;
    case CUPS_CSPACE_RGBW:
	p = "RGBW";
	break;
    case CUPS_CSPACE_ICC1:
	p = "ICC1";
	break;
    case CUPS_CSPACE_ICC2:
	p = "ICC2";
	break;
    case CUPS_CSPACE_ICC3:
	p = "ICC3";
	break;
    case CUPS_CSPACE_ICC4:
	p = "ICC4";
	break;
    case CUPS_CSPACE_ICC5:
	p = "ICC5";
	break;
    case CUPS_CSPACE_ICC6:
	p = "ICC6";
	break;
    case CUPS_CSPACE_ICC7:
	p = "ICC7";
	break;
    case CUPS_CSPACE_ICC8:
	p = "ICC8";
	break;
    case CUPS_CSPACE_ICC9:
	p = "ICC9";
	break;
    case CUPS_CSPACE_ICCA:
	p = "ICCA";
	break;
    case CUPS_CSPACE_ICCB:
	p = "ICCB";
	break;
    case CUPS_CSPACE_ICCC:
	p = "ICCC";
	break;
    case CUPS_CSPACE_ICCD:
	p = "ICCD";
	break;
    case CUPS_CSPACE_ICCE:
	p = "ICCE";
	break;
    case CUPS_CSPACE_ICCF:
	p = "ICCF";
	break;
    default:
	p = "Unknown";
	break;
    }
    printf("cupsColorSpace = %s\n",p);

    printf("cupsCompression = %d\n",hp->cupsCompression);

    printf("cupsRowCount = %d rows per bands\n",hp->cupsRowCount);

    printf("cupsRowFeed = %d feed per bands\n",hp->cupsRowFeed);

    printf("cupsRowStep = %d\n",hp->cupsRowStep);
}

static void skipPagePixels(cups_raster_t *raster, cups_page_header_t *hp)
{
    unsigned int size = hp->cupsBytesPerLine*hp->cupsHeight;
    unsigned char buf[1024];

    while (size > 0) {
	int n, r;

	n = size > sizeof(buf) ? sizeof(buf) : size;
	if ((r = cupsRasterReadPixels(raster,buf,n)) < n) {
	    break;
	}
	size -= r;
    }
}

static void readPagePixels(cups_raster_t *raster, cups_page_header_t *hp,
  int page)
{
    unsigned char *line;
    unsigned int i;
    FILE *fp;
    char fname[1024];
    char *ext;

    if (prefix == NULL) {
	skipPagePixels(raster,hp);
	return;
    }
    switch (hp->cupsColorSpace) {
    case CUPS_CSPACE_RGB:
	/* out ppm file */
	if (hp->cupsBitsPerColor != 8 || hp->cupsBitsPerPixel != 24) {
	    fprintf(stderr,"Not supported BitsPerColor or BitsPerPixel, output no image file\n");
	    skipPagePixels(raster,hp);
	    return;
	}
	ext = "ppm";
	break;
    case CUPS_CSPACE_K:
	/* out pbm file */
	if (hp->cupsBitsPerColor != 1 || hp->cupsBitsPerPixel != 1) {
	    fprintf(stderr,"Not supported BitsPerColor or BitsPerPixel, output no image file\n");
	    skipPagePixels(raster,hp);
	    return;
	}
	ext = "pbm";
	break;
    default:
	fprintf(stderr,"Not supported color space, output no image file\n");
	skipPagePixels(raster,hp);
	return;
	break;
    }
    if (hp->cupsColorOrder != CUPS_ORDER_CHUNKED) {
	fprintf(stderr,"Not supported color order, output no image file\n");
	skipPagePixels(raster,hp);
	return;
    }
    if (hp->cupsCompression > 1) {
	fprintf(stderr,"Not supported compression, output no image file\n");
	skipPagePixels(raster,hp);
	return;
    }
    sprintf(fname,"%s%d.%s",prefix,page,ext);
    if ((fp = fopen(fname,"wb")) == NULL) {
	fprintf(stderr,"Can't open file %s\n",fname);
	exit(2);
    }

    if ((line = new unsigned char[hp->cupsBytesPerLine]) == NULL) {
	fprintf(stderr,"Can't allocate line pixels buffer\n");
	exit(2);
    }

    if (hp->cupsColorSpace == CUPS_CSPACE_K) {
	/* black & white */
	fprintf(fp,"P4 %d %d\n",hp->cupsWidth,hp->cupsHeight);
	for (i = 0;i < hp->cupsHeight;i++) {
	    unsigned int n;

	    if (cupsRasterReadPixels(raster,line,hp->cupsBytesPerLine)
	          < hp->cupsBytesPerLine) {
		break;
	    }
	    n = (hp->cupsWidth+7)/8;
	    if (fwrite(line, 1, n,fp) != n) {
		fprintf(stderr,"image file write error\n");
		exit(2);
	    }
	}
    } else {
	/* rgb */
	fprintf(fp,"P6 %d %d 255\n",hp->cupsWidth,hp->cupsHeight);
	for (i = 0;i < hp->cupsHeight;i++) {
	    unsigned int n;

	    if (cupsRasterReadPixels(raster,line,hp->cupsBytesPerLine)
	          < hp->cupsBytesPerLine) {
		break;
	    }
	    n = hp->cupsWidth*3;
	    if (fwrite(line, 1, n,fp) != n) {
		fprintf(stderr,"image file write error\n");
		exit(2);
	    }
	}
    }

    delete[] line;
    fclose(fp);
}

int main(int argc, char **argv)
{
    cups_raster_t *raster;
    cups_page_header_t header;
    int page;

    if (argc > 1) {
	prefix = argv[1];
    }
    if ((raster = cupsRasterOpen(0,CUPS_RASTER_READ)) == 0) {
	fprintf(stderr,"Can't open raster\n");
	exit(2);
    }
    page = 1;
    while (cupsRasterReadHeader(raster,&header)) {
	printf("-------------- page %d --------------\n",page);
	printHeader(&header);
	readPagePixels(raster,&header,page);
	printf("-------------------------------------\n");
	page++;
    }
    cupsRasterClose(raster);
    return 0;
}
