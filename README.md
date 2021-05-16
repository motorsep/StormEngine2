# Storm Engine 2
brought to you by Kot in Action Creative Artel

Storm Engine 2 Readme - https://github.com/motorsep/StormEngine2

Note: It due time minimal set of assets and external content authoring tools might be made available to the public under suitable licenses.

DISCLAIMER
----------

This engine is provided as-is, without future support. Pull requests might or might not be accepted. Feel free to do with the engine anything you want, it's under GPL anyway. Please don't forget to credit us when using engine or parts of its code in your projects. IN ANY CASE, KOT IN ACTION CREATIVE ARTEL, AND ITS RESPECTIVE OFFICERS, REPRESENTATIVES, AGENTS, SUCCESSORS, AND ASSIGNS SHALL NOT BE LIABLE FOR LOSS OF DATA, LOSS OF PROFITS, LOST SAVINGS, SPECIAL, INCIDENTAL, CONSEQUENTIAL, INDIRECT OR PUNITIVE DAMAGES, OR ANY OTHER DAMAGES ARISING FROM ANY ALLEGED CLAIMS ARISING FROM USE OF THE ENGINE AND ITS SOURCE CODE EVEN IF KOT-IN-ACTION, OR THEIR RESPECTIVE AGENTS HAVE BEEN ADVISED OF THE POSSIBILITY OF ANY SUCH DAMAGES, OR EVEN IF SUCH DAMAGES ARE FORESEEABLE, OR LIABLE FOR ANY CLAIM BY ANY OTHER PARTY.

COPYRIGHT
---------

Steel Storm, Storm Engine, Phaeton, art assets, ascii assets and stormengine2.ico file copyright (C) 2008-2016 Kot in Action Creative Artel

stormengine2.ico file is not allowed to be used in commercial projects.

LICENSE
-------

Engine source code and shaders in base/renderprogs/ are licensed under GNU GPL v3. See GPL3-LICENSE.txt for the GNU GENERAL PUBLIC LICENSE

All assets in base/ folder (with the exception of renderprogs/ folder) are licensed under Creative Common Attribution NonCommercial NoDerivatives 4.0 International. See CC-BY-NC-ND-40-LICENSE.txt

ADDITIONAL TERMS:  The Storm Engine 2 GPL Source Code is also subject to certain additional terms, same as Doom 3 BFG Edition GPL Source code.

COMPILING ON WIN32/WIN64 WITH VISUAL C++ 2013/2015 COMMUNITY EDITION
--------------------------------------------------------------------

1. Download and install the Visual C++ 2013/2015 Community Edition.

2. Download and install the DirectX SDK (June 2010) here:
	http://www.microsoft.com/en-us/download/details.aspx?id=6812

3. Download and install MFC multibyte update https://www.microsoft.com/en-us/download/details.aspx?id=40770 (MSVC 2013 only)

4. Download and install the latest CMake.

5. Generate the MSVC projects using CMake by doubleclicking a matching configuration .bat file in the neo/ folder.

6. Open generated solution, choose solution config (Release, Debug, etc.) and build it

RUNNING ENGINE ON WIN32/WIN64
-----------------------------

1. Copy StormEngine2.exe into a FolderOfYourChoice/

2. Copy base/ folder into FolderOfYourChoice/ where you copied StormEngine2.exe previously

3. Run StormEngine2.exe

Note: legal screen is disabled in debug builds. Intro video can be skipped by pressing a key, a gamepad button or moving mouse in any build.

OVERALL CHANGES AND ADDITIONAL FEATURES 
--------------------------------------
(in addition to existing set of features of the original engine)

- Flexible build system using CMake

- Linux support (32 and 64 bit)

- Win64 support (including Win64 tools)

- OpenAL Soft sound backend primarily developed for Linux but works on Windows as well

- PNG screenshots support

- Soft shadows using PCF hardware shadow mapping

- True 64 bit HDR imaging

- Soft (feathered) particles

- Gloss maps support

- Oblique view frustum (for proper reflections in the water)

- High resolution RoQ videos playback

- Blurry real-time reflections

- Image based bloom ("glow")

- Image dithering (eliminates banding artifacts on gradients)

- Underwater view warping

- Material flag to force images to be kept in RGBA format (unocmpressed)

- Full set of built-in tools for Win32/Win64 platforms (with various fixes and improvements)

- envShot cmd to generate cubemaps

- Higher quality YCoCg "scaled" DXT5 compression for diffuse images (from GIMP DDS)

- YCoCg "scaled" DXT 5 compression for skyboxes and cubemaps (yields high quality skies with minimal artifacts and no banding)

- Demo recording and playback

- LOD for static and animated entities (although not for map brushes)

- Mods loading and packaging into .resources

- Wheeled Vehicles (player driven and self-driving)

- Automatic overwrite of outdated binary files when auto-authoring updated content

- Fixed inlining (embedding) of static meshes into .proc while retaining collisions

- Map loading progress indicator

- Texure compression progress indicator

- Collision fix for large coordinates


KNOWN ISSUES
------------

- Game crashes when saving/loading game on maps with vehicles

AUTHORING OF BITMAP FONTS
-------------------------

Generation of new bitmap fonts is somewhat convoluted process, no thanks to Doom 3 BFG engine. To be able to generate engine compliant bitmap fonts, please follow instructions below. The process used to work smoothly back in the days and hasn't been tested in 2021.

Download BFGFont app (based on https://github.com/Zbyl/BFGFontTool by Zbyl): https://drive.google.com/file/d/1P-o0gyZocH5oK_GIpT7TAr-Zzrs6w4Dl/view?usp=sharing

1. Install your favourite font (licensed or open source licensed; beware of "free" fonts found on the Net) into the system OS.
 
2. Generate bitmap font and image atlas using BMFont app (Bitmap Font Generator; v1.13 was tested) using settings file located in utils/BFGFont/bmfont_settings/ (don't forget to choose font)
 
3. Save bitmap font as "48", rename 48_0.tga into 48.tga and edit 48.fnt to adjust image name.
 
4. If there are 48_1, etc. image atlases present, adjust image resolution in BMFont settings - there can only be one atlas.
 
5. Run BFGFontTool 1.2 from utils/BFGFont/ and generate BFG font using .fnt file we just created 
 
6. Copy 48.dat, 48.tga, fontimage_12, _24, _48 into base/newfonts/fontname/ (fontname can contain smallletters, numbers and _ )
 
7. Rename fontimage_12, _24, _48 into old_12, _24, _48
 
8. Edit base/strings/english.lang and add definition for new fonts at the beginning of the file, after "#font_" line. For example, "#font_ubu"	"ubuntu". That will remap
font definition in .gui UI from [font "fonts/ubu"] to a new font "ubuntu". Note that "#font_XXXXX" seems to be currently limited to 5 symbols after "_".

CODE LICENSE EXCEPTIONS - The parts that are not covered by the GPL:
--------------------------------------------------------------------

EXCLUDED CODE:  The code described below and contained in the Doom 3 BFG Edition GPL Source Code release
is not part of the Program covered by the GPL and is expressly excluded from its terms. 
You are solely responsible for obtaining from the copyright holder a license for such code and complying with the applicable license terms.


JPEG library
-----------------------------------------------------------------------------
neo/libs/jpeg-6/*

Copyright (C) 1991-1995, Thomas G. Lane

Permission is hereby granted to use, copy, modify, and distribute this
software (or portions thereof) for any purpose, without fee, subject to these
conditions:
(1) If any part of the source code for this software is distributed, then this
README file must be included, with this copyright and no-warranty notice
unaltered; and any additions, deletions, or changes to the original files
must be clearly indicated in accompanying documentation.
(2) If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the work of
the Independent JPEG Group".
(3) Permission for use of this software is granted only if the user accepts
full responsibility for any undesirable consequences; the authors accept
NO LIABILITY for damages of any kind.

These conditions apply to any software derived from or based on the IJG code,
not just to the unmodified library.  If you use our work, you ought to
acknowledge us.

NOTE: unfortunately the README that came with our copy of the library has
been lost, so the one from release 6b is included instead. There are a few
'glue type' modifications to the library to make it easier to use from
the engine, but otherwise the dependency can be easily cleaned up to a
better release of the library.

zlib library
---------------------------------------------------------------------------
neo/libs/zlib/*

Copyright (C) 1995-2012 Jean-loup Gailly and Mark Adler

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Base64 implementation
---------------------------------------------------------------------------
neo/idlib/Base64.cpp

Copyright (c) 1996 Lars Wirzenius.  All rights reserved.

June 14 2003: TTimo <ttimo@idsoftware.com>
	modified + endian bug fixes
	http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=197039

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

IO for (un)compress .zip files using zlib
---------------------------------------------------------------------------
neo/libs/zlib/minizip/*

Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )

Modifications of Unzip for Zip64
Copyright (C) 2007-2008 Even Rouault

Modifications for Zip64 support
Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

MD4 Message-Digest Algorithm
-----------------------------------------------------------------------------
neo/idlib/hashing/MD4.cpp
Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD4 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD4 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.

MD5 Message-Digest Algorithm
-----------------------------------------------------------------------------
neo/idlib/hashing/MD5.cpp
This code implements the MD5 message-digest algorithm.
The algorithm is due to Ron Rivest.  This code was
written by Colin Plumb in 1993, no copyright is claimed.
This code is in the public domain; do with it what you wish.

CRC32 Checksum
-----------------------------------------------------------------------------
neo/idlib/hashing/CRC32.cpp
Copyright (C) 1995-1998 Mark Adler

OpenGL headers
---------------------------------------------------------------------------
neo/renderer/OpenGL/glext.h
neo/renderer/OpenGL/wglext.h

Copyright (c) 2007-2012 The Khronos Group Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Materials.

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
