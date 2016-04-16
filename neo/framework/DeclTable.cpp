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
#include "precompiled.h"
#pragma hdrstop

/*
=================
idDeclTable::TableLookup
=================
*/
float idDeclTable::TableLookup( float index ) const
{
	int iIndex;
	float iFrac;
	
	int domain = values.Num() - 1;
	
	if( domain <= 1 )
	{
		return 1.0f;
	}
	
	if( clamp )
	{
		index *= ( domain - 1 );
		if( index >= domain - 1 )
		{
			return values[domain - 1];
		}
		else if( index <= 0 )
		{
			return values[0];
		}
		iIndex = idMath::Ftoi( index );
		iFrac = index - iIndex;
	}
	else
	{
		index *= domain;
		
		if( index < 0 )
		{
			index += domain * idMath::Ceil( -index / domain );
		}
		
		iIndex = idMath::Ftoi( idMath::Floor( index ) );
		iFrac = index - iIndex;
		iIndex = iIndex % domain;
	}
	
	if( !snap )
	{
		// we duplicated the 0 index at the end at creation time, so we
		// don't need to worry about wrapping the filter
		return values[iIndex] * ( 1.0f - iFrac ) + values[iIndex + 1] * iFrac;
	}
	
	return values[iIndex];
}

/*
=================
idDeclTable::Size
=================
*/
size_t idDeclTable::Size() const
{
	return sizeof( idDeclTable ) + values.Allocated();
}

/*
=================
idDeclTable::FreeData
=================
*/
void idDeclTable::FreeData()
{
	snap = false;
	clamp = false;
	values.Clear();
}

/*
=================
idDeclTable::DefaultDefinition
=================
*/
const char* idDeclTable::DefaultDefinition() const
{
	return "{ { 0 } }";
}

/*
=================
idDeclTable::Parse
=================
*/
bool idDeclTable::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	idToken token;
	float v;
	
	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );
	
	snap = false;
	clamp = false;
	values.Clear();
	
	while( 1 )
	{
		if( !src.ReadToken( &token ) )
		{
			break;
		}
		
		if( token == "}" )
		{
			break;
		}
		
		if( token.Icmp( "snap" ) == 0 )
		{
			snap = true;
		}
		else if( token.Icmp( "clamp" ) == 0 )
		{
			clamp = true;
		}
		else if( token.Icmp( "{" ) == 0 )
		{
		
			while( 1 )
			{
				bool errorFlag;
				
				v = src.ParseFloat( &errorFlag );
				if( errorFlag )
				{
					// we got something non-numeric
					MakeDefault();
					return false;
				}
				
				values.Append( v );
				
				src.ReadToken( &token );
				if( token == "}" )
				{
					break;
				}
				if( token == "," )
				{
					continue;
				}
				src.Warning( "expected comma or brace" );
				MakeDefault();
				return false;
			}
			
		}
		else
		{
			src.Warning( "unknown token '%s'", token.c_str() );
			MakeDefault();
			return false;
		}
	}
	
	// copy the 0 element to the end, so lerping doesn't
	// need to worry about the wrap case
	float val = values[0];		// template bug requires this to not be in the Append()?
	values.Append( val );
	
	return true;
}

idDeclTable2d::idDeclTable2d()
{
	values.Clear();
}

/*
=================
idDeclTable2d::MinInput
=================
*/
float idDeclTable2d::MinInput() const
{
	if ( values.Size() > 0 )
		return values.First().mInput;
	return 0.0f;
}

/*
=================
idDeclTable2d::MinOutput
=================
*/
float idDeclTable2d::MinOutput() const
{
	if ( values.Size() > 0 )
		return values.First().mOutput;
	return 0.0f;
}

/*
=================
idDeclTable2d::MaxInput
=================
*/
float idDeclTable2d::MaxInput() const
{
	if ( values.Size() > 0 )
		return values.Last().mInput;
	return 0.0f;
}

/*
=================
idDeclTable2d::MaxOutput
=================
*/
float idDeclTable2d::MaxOutput() const
{
	if ( values.Size() > 0 )
		return values.Last().mOutput;
	return 0.0f;
}

/*
=================
idDeclTable2d::TableLookup
=================
*/
float idDeclTable2d::TableLookup( float value ) const
{
	if ( values.Size() > 0 )
	{
		if ( value < values.First().mInput )
		{
			return values.First().mOutput;
		}
		else if ( value > values.Last().mInput )
		{
			return values.Last().mOutput;
		}
		else
		{
			// look for where this value falls in the table
			for ( int i = 1; i < values.Num(); ++i )
			{
				if ( value <= values[ i ].mInput )
				{
					const float range = values[ i ].mInput - values[ i - 1 ].mInput;
					const float diff = value - values[ i - 1 ].mInput;
					const float frac = diff / range;
					return Lerp( values[ i - 1 ].mOutput, values[ i ].mOutput, frac );
				}
			}
		}
	}
	return 0.0f;
}

/*
=================
idDeclTable::Size
=================
*/
size_t idDeclTable2d::Size() const
{
	return sizeof( idDeclTable2d ) + values.Allocated();
}

/*
=================
idDeclTable::FreeData
=================
*/
void idDeclTable2d::FreeData()
{
	values.Clear();
}

/*
=================
idDeclTable2d::DefaultDefinition
=================
*/
const char* idDeclTable2d::DefaultDefinition() const
{
	return "{ [ 0, 0 ] }";
}

/*
=================
idDeclTable2d::Parse
=================
*/
bool idDeclTable2d::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	idLexer src;
	idToken token;
	
	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	values.Clear();

	while ( 1 )
	{
		if ( !src.ReadToken( &token ) )
		{
			break;
		}

		if ( token == "}" )
		{
			break;
		}

		if ( token.Icmp( "[" ) == 0 )
		{
			tableEntry entry;

			bool errorFlag;
			entry.mInput = src.ParseFloat( &errorFlag );
			if ( errorFlag )
			{
				// we got something non-numeric
				MakeDefault();
				return false;
			}

			src.ReadToken( &token );
			if ( token != "," )
			{
				// we got something non-numeric
				MakeDefault();
				return false;
			}

			entry.mOutput = src.ParseFloat( &errorFlag );
			if ( errorFlag )
			{
				// we got something non-numeric
				MakeDefault();
				return false;
			}

			src.ReadToken( &token );
			if ( token != "]" )
			{
				// we got something non-numeric
				MakeDefault();
				return false;
			}

			values.Append( entry );
		}
		else
		{
			src.Warning( "unknown token '%s'", token.c_str() );
			MakeDefault();
			return false;
		}
	}

	return true;
}
