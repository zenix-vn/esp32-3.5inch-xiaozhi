/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: buffers.c,v 1.1 2005/02/26 01:47:34 jrecker Exp $ 
 *   
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  

/**************************************************************************************
 * Fixed-point HE-AAC decoder
 * Jon Recker (jrecker@real.com)
 * February 2005
 *
 * buffers.c - allocation and deallocation of internal AAC decoder buffers
 **************************************************************************************/

#if defined(USE_DEFAULT_STDLIB) || defined(ARDUINO)
#include <stdlib.h>
#else
#include "hlxclib/stdlib.h"
#endif

#include "coder.h"
#if defined(ESP32)
#include <esp_heap_caps.h>
#endif

/**************************************************************************************
 * Function:    ClearBuffer
 *
 * Description: fill buffer with 0's
 *
 * Inputs:      pointer to buffer
 *              number of bytes to fill with 0
 *
 * Outputs:     cleared buffer
 *
 * Return:      none
 *
 * Notes:       slow, platform-independent equivalent to memset(buf, 0, nBytes)
 **************************************************************************************/
#include <string.h>
 void ClearBuffer(void *buf, int nBytes)
{
/*	int i;
	unsigned char *cbuf = (unsigned char *)buf;

	for (i = 0; i < nBytes; i++)
		cbuf[i] = 0;

	return;
	*/
	/* Zin100: word stores when possible - the coef/overlap blocks may live in IRAM, which takes 32-bit access only */
	if ((((uintptr_t)buf) & 3) == 0 && (nBytes & 3) == 0) {
		volatile int *w = (volatile int *)buf;
		int i;
		for (i = 0; i < nBytes / 4; i++)
			w[i] = 0;
		return;
	}
	memset(buf, 0, nBytes);
}

/* Zin100: coef[] and overlap[] (16 KB, ints only) go to IRAM when the ESP32 has some free: ordinary heap is scarce
   next to Wi-Fi, the UI and a TLS session, while ~29 KB of IRAM heap sits unused. Falls back to normal heap. */
static void *AllocInts(int nBytes)
{
	void *p = 0;
#if defined(ESP32)
	p = heap_caps_malloc(nBytes, MALLOC_CAP_EXEC | MALLOC_CAP_32BIT);
#endif
	if (!p)
		p = malloc(nBytes);
	return p;
}

/**************************************************************************************
 * Function:    AllocateBuffers
 *
 * Description: allocate all the memory needed for the AAC decoder
 *
 * Inputs:      none
 *
 * Outputs:     none
 *
 * Return:      pointer to AACDecInfo structure, cleared to all 0's (except for
 *                pointer to platform-specific data structure)
 *
 * Notes:       if one or more mallocs fail, function frees any buffers already
 *                allocated before returning
 **************************************************************************************/
AACDecInfo *AllocateBuffers(void)
{
	AACDecInfo *aacDecInfo;

	aacDecInfo = (AACDecInfo *)malloc(sizeof(AACDecInfo));
	if (!aacDecInfo)
		return 0;
	ClearBuffer(aacDecInfo, sizeof(AACDecInfo));

	aacDecInfo->psInfoBase = malloc(sizeof(PSInfoBase));
	if (!aacDecInfo->psInfoBase) {
		FreeBuffers(aacDecInfo);
		return 0;
	}
	ClearBuffer(aacDecInfo->psInfoBase, sizeof(PSInfoBase));
	{
		PSInfoBase *psi = (PSInfoBase *)aacDecInfo->psInfoBase;
		psi->coef = AllocInts(MAX_NCHANS_ELEM * AAC_MAX_NSAMPS * sizeof(int));
		psi->overlap = AllocInts(AAC_MAX_NCHANS * AAC_MAX_NSAMPS * sizeof(int));
		if (!psi->coef || !psi->overlap) {
			FreeBuffers(aacDecInfo);
			return 0;
		}
		ClearBuffer(psi->coef, MAX_NCHANS_ELEM * AAC_MAX_NSAMPS * sizeof(int));
		ClearBuffer(psi->overlap, AAC_MAX_NCHANS * AAC_MAX_NSAMPS * sizeof(int));
	}

	return aacDecInfo;
}

AACDecInfo *AllocateBuffersPre(void **ptr, int *sz)
{
        AACDecInfo *aacDecInfo;

        char *p = (char*)*ptr;
        aacDecInfo = (AACDecInfo *)p;
        p += (sizeof(AACDecInfo) +7 ) & ~7;
        *sz -= (sizeof(AACDecInfo) +7 ) & ~7;
        if (*sz < 0)
                return 0;
        ClearBuffer(aacDecInfo, sizeof(AACDecInfo));

        aacDecInfo->psInfoBase = (PSInfoBase*)p;
        p += (sizeof(PSInfoBase) + 7) & ~7;
        *sz -= (sizeof(PSInfoBase) + 7) & ~7;
        if (*sz <0) {
                return 0;
        }
        ClearBuffer(aacDecInfo->psInfoBase, sizeof(PSInfoBase));
        {	/* Zin100: coef[] and overlap[] are separate blocks now; carve them from the same space */
                PSInfoBase *psi = (PSInfoBase *)aacDecInfo->psInfoBase;
                int n = (MAX_NCHANS_ELEM + AAC_MAX_NCHANS) * AAC_MAX_NSAMPS * sizeof(int);
                *sz -= n;
                if (*sz < 0)
                        return 0;
                psi->coef = (int (*)[AAC_MAX_NSAMPS])p;
                psi->overlap = (int (*)[AAC_MAX_NSAMPS])(p + MAX_NCHANS_ELEM * AAC_MAX_NSAMPS * sizeof(int));
                ClearBuffer(p, n);
                p += n;
        }

	*ptr = p;

        return aacDecInfo;
}

#ifndef SAFE_FREE
#define SAFE_FREE(x)	{if (x)	free(x);	(x) = 0;}	/* helper macro */
#endif

/**************************************************************************************
 * Function:    FreeBuffers
 *
 * Description: frees all the memory used by the AAC decoder
 *
 * Inputs:      pointer to initialized AACDecInfo structure
 *
 * Outputs:     none
 *
 * Return:      none
 *
 * Notes:       safe to call even if some buffers were not allocated (uses SAFE_FREE)
 **************************************************************************************/
void FreeBuffers(AACDecInfo *aacDecInfo)
{
	if (!aacDecInfo)
		return;

	if (aacDecInfo->psInfoBase) {
		PSInfoBase *psi = (PSInfoBase *)aacDecInfo->psInfoBase;
		SAFE_FREE(psi->coef);
		SAFE_FREE(psi->overlap);
	}
	SAFE_FREE(aacDecInfo->psInfoBase);
	SAFE_FREE(aacDecInfo);
}
