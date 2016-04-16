/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#pragma hdrstop
#include "precompiled.h"

#include "DmapSimd_Generic.h"

idDmapSIMDProcessor	*	dmapProcessor = NULL;			// pointer to SIMD processor
idDmapSIMDProcessor *	dmapGenericSIMD = NULL;				// pointer to generic SIMD implementation
idDmapSIMDProcessor *	dmapSIMDProcessor = NULL;


/*
================
idDmapSIMD::Init
================
*/
void idDmapSIMD::Init(void) {
	dmapGenericSIMD = new idDmapSIMD_Generic;
	dmapGenericSIMD->cpuid = CPUID_GENERIC;
	dmapProcessor = NULL;
	dmapSIMDProcessor = dmapGenericSIMD;
}

/*
============
idDmapSIMD::InitProcessor
============
*/
void idDmapSIMD::InitProcessor(const char *module, bool forceGeneric) {
	int cpuid;
	idDmapSIMDProcessor *newProcessor;

	cpuid = idLib::sys->GetProcessorId();

	if (forceGeneric) {

		newProcessor = dmapGenericSIMD;

	}
	else {

		if (!dmapProcessor) {
			dmapProcessor = dmapGenericSIMD;
			dmapProcessor->cpuid = cpuid;
		}

		newProcessor = dmapProcessor;
	}

	if (newProcessor != dmapSIMDProcessor) {
		dmapSIMDProcessor = newProcessor;
		idLib::common->Printf("%s using %s for SIMD processing\n", module, dmapSIMDProcessor->GetName());
	}

	if (cpuid & CPUID_SSE) {
		idLib::sys->FPU_SetFTZ(true);
		idLib::sys->FPU_SetDAZ(true);
	}
}

/*
================
idDmapSIMD::Shutdown
================
*/
void idDmapSIMD::Shutdown(void) {
	if (dmapProcessor != dmapGenericSIMD) {
		delete dmapProcessor;
	}
	delete dmapGenericSIMD;
	dmapGenericSIMD = NULL;
	dmapProcessor = NULL;
	dmapSIMDProcessor = NULL;
}
