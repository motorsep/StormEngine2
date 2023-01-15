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
#include "../Game_local.h"

const static int NUM_SYSTEM_OPTIONS_OPTIONS = 8;

extern idCVar r_multiSamples;
extern idCVar r_motionBlur;
extern idCVar r_swapInterval;
extern idCVar s_volume_dB;
extern idCVar r_lightScale;
extern idCVar r_screenshot_png;
extern idCVar r_glowEnable;
extern idCVar r_useHightQualitySky;

/*
========================
idMenuScreen_Shell_SystemOptions::Initialize
========================
*/
void idMenuScreen_Shell_SystemOptions::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );
	
	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}
	
	SetSpritePath( "menuSystemOptions" );
	
	options = new( TAG_SWF ) idMenuWidget_ControlList();
	options->SetNumVisibleOptions( NUM_SYSTEM_OPTIONS_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	while (options->GetChildren().Num() < NUM_SYSTEM_OPTIONS_OPTIONS)
	{
		idMenuWidget_ControlButton* buttonWidget = new idMenuWidget_ControlButton;
		options->AddChild(buttonWidget);
	}
	options->Initialize(data);
	options->AddTextSlider("#str_02154", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN, DEFAULT_REPEAT_TIME);
	options->AddTextSlider("#str_swf_framerate", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE, DEFAULT_REPEAT_TIME);
	options->AddTextSlider("#str_swf_vsync", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_VSYNC, DEFAULT_REPEAT_TIME);
	options->AddTextSlider("#str_swf_aa", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_ANTIALIASING, DEFAULT_REPEAT_TIME);
	options->AddTextSlider("#str_swf_motionblur", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_MOTIONBLUR, DEFAULT_REPEAT_TIME);
	options->AddSliderBar("#str_swf_lodbias", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_LODBIAS, DEFAULT_REPEAT_TIME);
	options->AddSliderBar("#str_swf_brightness", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_BRIGHTNESS, 2);
	options->AddSliderBar("#str_swf_volume", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_VOLUME, 2);
	options->AddSliderToggle("#str_swf_shadowmap_enable", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_SHADOWMAPPING, DEFAULT_REPEAT_TIME);
	options->AddTextSlider("#str_swf_shadowmap_samples", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_SHADOWMAPSAMPLES, DEFAULT_REPEAT_TIME);
	//options->AddTextSlider("#str_swf_shadowmap_imagesize", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_SHADOWMAPIMAGESIZE, DEFAULT_REPEAT_TIME);
	options->AddTextSlider("#str_swf_shadowmap_quality", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_SHADOWMAPQUALITY, DEFAULT_REPEAT_TIME);
	options->AddSliderBar("#str_swf_shadowmap_distance", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_SHADOWMAPDISTANCE, 2);
	options->AddSliderToggle("#str_swf_png_enable", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_PNG, DEFAULT_REPEAT_TIME);
	options->AddTextSlider("#str_swf_png_quality", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_PNG_QUALITY, DEFAULT_REPEAT_TIME);
	options->AddSliderToggle("#str_swf_glow_enable", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_GLOW_ENABLE, DEFAULT_REPEAT_TIME);
	options->AddSliderToggle("#str_swf_hdr_enable", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_HDR_ENABLE, DEFAULT_REPEAT_TIME);
	options->AddSliderToggle("#str_swf_hq_sky_enable", &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_HIGH_QUALITY_SKY, DEFAULT_REPEAT_TIME);
	

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_settings" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	
	AddChild( options );
	AddChild( btnBack );
	
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
}

/*
========================
idMenuScreen_Shell_SystemOptions::Update
========================
*/
void idMenuScreen_Shell_SystemOptions::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_swf_back";
			}
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );
			
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}
	}
	
	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_swf_header_system_options" );	// was #str_00183
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}
		
		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if( gradient != NULL && heading != NULL )
		{
			gradient->SetXPos( heading->GetTextLength() );
		}
	}
	
	if( btnBack != NULL )
	{
		btnBack->BindSprite( root );
	}
	
	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_SystemOptions::ShowScreen
========================
*/
void idMenuScreen_Shell_SystemOptions::ShowScreen( const mainMenuTransition_t transitionType )
{

	systemData.LoadData();
	
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_SystemOptions::HideScreen
========================
*/
void idMenuScreen_Shell_SystemOptions::HideScreen( const mainMenuTransition_t transitionType )
{

	if( systemData.IsRestartRequired() )
	{
		class idSWFScriptFunction_Restart : public idSWFScriptFunction_RefCounted
		{
		public:
			idSWFScriptFunction_Restart( gameDialogMessages_t _msg, bool _restart )
			{
				msg = _msg;
				restart = _restart;
			}
			idSWFScriptVar Call( idSWFScriptObject* thisObject, const idSWFParmList& parms )
			{
				common->Dialog().ClearDialog( msg );
				if( restart )
				{
					// DG: Sys_ReLaunch() doesn't need any options anymore
					//     (the old way would have been unnecessarily painful on POSIX systems)
					//Sys_ReLaunch();
					// DG end

					// motorsep 12-28-2014; reverted back to the original Sys_ReLaunch; guys from RBDoom 3 BFG team made it impossible to pass any cmds on restart

					idStr cmdLine = Sys_GetCmdLine();
					if (cmdLine.Find("com_skipIntroVideos") < 0) {
						cmdLine.Append(" +set com_skipIntroVideos 1");
					}
					Sys_ReLaunch( (void*)cmdLine.c_str(), cmdLine.Length() );
				}
				return idSWFScriptVar();
			}
		private:
			gameDialogMessages_t msg;
			bool restart;
		};
		idStaticList<idSWFScriptFunction*, 4> callbacks;
		idStaticList<idStrId, 4> optionText;
		callbacks.Append( new idSWFScriptFunction_Restart( GDM_GAME_RESTART_REQUIRED, false ) );
		callbacks.Append( new idSWFScriptFunction_Restart( GDM_GAME_RESTART_REQUIRED, true ) );
		optionText.Append( idStrId( "#str_swf_system_options_continue" ) ); // Continue
		optionText.Append( idStrId( "#str_swf_system_options_restart" ) ); // Restart Now
		common->Dialog().AddDynamicDialog( GDM_GAME_RESTART_REQUIRED, callbacks, optionText, true, idStr() );
	}
	
	if( systemData.IsDataChanged() )
	{
		systemData.CommitData();
	}
	
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_SystemOptions::HandleAction h
========================
*/
bool idMenuScreen_Shell_SystemOptions::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}
	
	if( menuData->ActiveScreen() != SHELL_AREA_SYSTEM_OPTIONS )
	{
		return false;
	}
	
	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();
	
	switch( actionType )
	{

		case WIDGET_ACTION_GO_BACK:
		{
			if( menuData != NULL )
			{
				menuData->SetNextScreen( SHELL_AREA_SETTINGS, MENU_TRANSITION_SIMPLE );
			}
			return true;
		}
		case WIDGET_ACTION_ADJUST_FIELD:
			if( widget->GetDataSourceFieldIndex() == idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN )
			{
				menuData->SetNextScreen( SHELL_AREA_RESOLUTION, MENU_TRANSITION_SIMPLE );
				return true;
			}
			break;
		case WIDGET_ACTION_COMMAND:
		{
		
			if( options == NULL )
			{
				return true;
			}
			
			int selectionIndex = options->GetFocusIndex();
			if( parms.Num() > 0 )
			{
				selectionIndex = parms[0].ToInteger();
			}
			
			switch( parms[0].ToInteger() )
			{
				case idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN:
				{
					menuData->SetNextScreen( SHELL_AREA_RESOLUTION, MENU_TRANSITION_SIMPLE );
					return true;
				}
				default:
				{
					systemData.AdjustField( parms[0].ToInteger(), 1 );
					options->Update();
				}
			}
			
			return true;
		}
		case WIDGET_ACTION_START_REPEATER:
		{

			if (options == NULL)
			{
				return true;
			}
			break;
		}
	}
	
	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/////////////////////////////////
// SCREEN SETTINGS
/////////////////////////////////

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::idMenuDataSource_SystemSettings
========================
*/
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::idMenuDataSource_SystemSettings()
{
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::LoadData
========================
*/
void idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::LoadData()
{
	originalFramerate = com_engineHz.GetInteger();
	originalAntialias = r_multiSamples.GetInteger();
	originalMotionBlur = r_motionBlur.GetInteger();
	originalVsync = r_swapInterval.GetInteger();
	originalBrightness = r_lightScale.GetFloat();
	originalVolume = s_volume_dB.GetFloat();
	
	originalShadowMapEnable = r_useShadowMapping.GetBool();
	//originalShadowMapImageSize = r_shadowMapImageSize.GetInteger();
	//originalShadowMapImageSize = r_shadowMapLodScale.GetInteger(); // currently sets resolution for shadow map image
	originalShadowMapQuality = r_shadowMapQuality.GetInteger();
	originalShadowMapSampleCount = r_shadowMapSamples.GetInteger();
	originalShadowMapDistance = r_shadowMapMaxDistance.GetFloat();

	originalUsePNG = r_screenshot_png.GetBool();
	originalPNGQuality = r_screenshot_png_quality.GetInteger();
	originalGlowEnable = r_glowEnable.GetBool();
	originalHDREnable = r_useHDR.GetBool();
	originalHQSky = r_useHightQualitySky.GetBool();

	const int fullscreen = r_fullscreen.GetInteger();
	if( fullscreen > 0 )
	{
		R_GetModeListForDisplay( fullscreen - 1, modeList );
	}
	else
	{
		modeList.Clear();
	}
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsRestartRequired
========================
*/
bool idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsRestartRequired() const
{
	if( originalAntialias != r_multiSamples.GetInteger() )
	{
		return true;
	}
	if( originalFramerate != com_engineHz.GetInteger() )
	{
		return true;
	}
	/* These Shadow Map Options Require Restart.*/
	//if (originalShadowMapImageSize != r_shadowMapImageSize.GetInteger())
	//{
	//	return true;
	//}
	if (originalShadowMapQuality != r_shadowMapQuality.GetInteger())
	{
		return true;
	}
	if (originalHQSky != r_useHightQualitySky.GetBool())
	{
		return true;
	}
	return false;
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::CommitData
========================
*/
void idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::CommitData()
{
	cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
}

/*
========================
AdjustOption
Given a current value in an array of possible values, returns the next n value
========================
*/
int AdjustOption( int currentValue, const int values[], int numValues, int adjustment )
{
	int index = 0;
	for( int i = 0; i < numValues; i++ )
	{
		if( currentValue == values[i] )
		{
			index = i;
			break;
		}
	}
	index += adjustment;
	while( index < 0 )
	{
		index += numValues;
	}
	index %= numValues;
	return values[index];
}

/*
========================
LinearAdjust
Linearly converts a float from one scale to another
========================
*/
float LinearAdjust( float input, float currentMin, float currentMax, float desiredMin, float desiredMax )
{
	return ( ( input - currentMin ) / ( currentMax - currentMin ) ) * ( desiredMax - desiredMin ) + desiredMin;
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::AdjustField
========================
*/
void idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::AdjustField( const int fieldIndex, const int adjustAmount )
{
	switch( fieldIndex )
	{
		case SYSTEM_FIELD_FRAMERATE:
		{
			static const int numValues = 2;
			// motorsep 01-14-2023: potential fix for high refresh rate monitors
			static const int values[numValues] = { 60, 240 };
			//static const int values[numValues] = { 60, 120 };
			com_engineHz.SetInteger( AdjustOption( com_engineHz.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_VSYNC:
		{
			static const int numValues = 3;
			static const int values[numValues] = { 0, 1, 2 };
			r_swapInterval.SetInteger( AdjustOption( r_swapInterval.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_ANTIALIASING:
		{
			static const int numValues = 5;
			static const int values[numValues] = { 0, 2, 4, 8, 16 };
			r_multiSamples.SetInteger( AdjustOption( r_multiSamples.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_MOTIONBLUR:
		{
			static const int numValues = 5;
			static const int values[numValues] = { 0, 2, 3, 4, 5 };
			r_motionBlur.SetInteger( AdjustOption( r_motionBlur.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}
		case SYSTEM_FIELD_LODBIAS:
		{
			const float percent = LinearAdjust( r_lodBias.GetFloat(), -1.0f, 1.0f, 0.0f, 100.0f );
			const float adjusted = percent + ( float )adjustAmount * 5.0f;
			const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );
			r_lodBias.SetFloat( LinearAdjust( clamped, 0.0f, 100.0f, -1.0f, 1.0f ) );
			break;
		}
		case SYSTEM_FIELD_BRIGHTNESS:
		{
			const float percent = LinearAdjust( r_lightScale.GetFloat(), 2.0f, 4.0f, 0.0f, 100.0f );
			const float adjusted = percent + ( float )adjustAmount;
			const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );
			r_lightScale.SetFloat( LinearAdjust( clamped, 0.0f, 100.0f, 2.0f, 4.0f ) );
			break;
		}
		case SYSTEM_FIELD_VOLUME:
		{
			const float percent = 100.0f * Square( 1.0f - ( s_volume_dB.GetFloat() / DB_SILENCE ) );
			const float adjusted = percent + ( float )adjustAmount;
			const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );
			s_volume_dB.SetFloat( DB_SILENCE - ( idMath::Sqrt( clamped / 100.0f ) * DB_SILENCE ) );
			break;
		}
		case SYSTEM_FIELD_SHADOWMAPDISTANCE:
		{
			const float percent = LinearAdjust(r_shadowMapMaxDistance.GetFloat(), 512.0f, 16384.0f, 0.0f, 100.0f);
			const float adjusted = percent + (float)adjustAmount * 5.0f;
			const float clamped = idMath::ClampFloat(0.0f, 100.0f, adjusted);
			r_shadowMapMaxDistance.SetFloat(LinearAdjust(clamped, 0.0f, 100.0f, 512.0f, 16384.0f));
			break;
		}
		case SYSTEM_FIELD_SHADOWMAPPING:
		{
			r_useShadowMapping.SetBool(!r_useShadowMapping.GetBool());
			break;
		}
		/*case SYSTEM_FIELD_SHADOWMAPIMAGESIZE:
		{

			static const int numValues = 2;
			static const int values[numValues] = { 0, 1 }; //  0 is LOW (??) and 1 is HIGH (2048)
			r_shadowMapLodScale.SetInteger( AdjustOption( r_shadowMapLodScale.GetInteger(), values, numValues, adjustAmount ) );
			break;
		}*/
		case SYSTEM_FIELD_SHADOWMAPQUALITY:
		{
			static const int numValues = 5;
			static const int values[numValues] = { 0, 1, 2, 3, 4 };
			r_shadowMapQuality.SetInteger(AdjustOption(r_shadowMapQuality.GetInteger(), values, numValues, adjustAmount));
			break;
		}
		case SYSTEM_FIELD_SHADOWMAPSAMPLES:
		{
			static const int numValues = 4;
			// Why is this 0, 1, 4, or 16. I would expect this to be 0, 1, 2, 4, 8, or 16.
			static const int values[numValues] = { 0, 1, 4, 16 };
			r_shadowMapSamples.SetInteger(AdjustOption(r_shadowMapSamples.GetInteger(), values, numValues, adjustAmount));
			break;
		}
		case SYSTEM_FIELD_HIGH_QUALITY_SKY:
		{
			r_useHightQualitySky.SetBool(!r_useHightQualitySky.GetBool());
			break;
		}
		case SYSTEM_FIELD_HDR_ENABLE:
		{
			r_useHDR.SetBool(!r_useHDR.GetBool());
			break;
		}
		case SYSTEM_FIELD_GLOW_ENABLE:
		{
			r_glowEnable.SetBool(!r_glowEnable.GetBool());
			break;
		}
		case SYSTEM_FIELD_PNG:
		{
			r_screenshot_png.SetBool(!r_screenshot_png.GetBool());
			break;
		}
		case SYSTEM_FIELD_PNG_QUALITY:
		{

			static const int numValues = 5;
			// Why is this 0, 1, 4, or 16. I would expect this to be 0, 1, 2, 4, 8, or 16.
			static const int values[numValues] = { 0, 3, 5, 7, 8 };
			r_screenshot_png_quality.SetInteger(AdjustOption(r_screenshot_png_quality.GetInteger(), values, numValues, adjustAmount));
			break;
			
			break;
		}

	}
	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );
}

const char* valueToQuality(int qualityIndex)
{
	switch (qualityIndex)
	{
	case 0:
		return "#str_low_quality"; /* Low */
	case 1:
		return "#str_medium_quality"; // medium
	case 2:
		return "#str_high_quality"; // high
	case 3:
		return "#str_ultra_quality";
	case 4:
		return "#str_absurd_quality";
	}
	return "";
}

/*const char* imageSize_valueToQuality( int qualityIndex ) {
	switch( qualityIndex ) {
	case 0:
		return "#str_low_resolution";
	case 1:
		return "#str_high_resolution";
	}
	return "";
} */

const char* png_valueToQuality(int qualityIndex)
{
	switch (qualityIndex)
	{
	case 0:
	case 1:
		return "#str_low_quality"; /* Low */
	case 2:
	case 3:
		return "#str_medium_quality"; // medium
	case 4:
	case 5:
		return "#str_high_quality"; // high
	case 6:
	case 7:
		return "#str_ultra_quality";
	case 8:
		return "#str_absurd_quality";
	}
	return "";
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::GetField
========================
*/
idSWFScriptVar idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::GetField( const int fieldIndex ) const
{
	switch( fieldIndex )
	{
		case SYSTEM_FIELD_FULLSCREEN:
		{
			const int fullscreen = r_fullscreen.GetInteger();
			const int vidmode = r_vidMode.GetInteger();
			if( fullscreen == 0 )
			{
				return "#str_swf_disabled";
			}
			if( fullscreen < 0 || vidmode < 0 || vidmode >= modeList.Num() )
			{
				return "???";
			}
			if( modeList[vidmode].displayHz == 60 )
			{
				return va( "%4i x %4i", modeList[vidmode].width, modeList[vidmode].height );
			}
			else
			{
				return va( "%4i x %4i @ %dhz", modeList[vidmode].width, modeList[vidmode].height, modeList[vidmode].displayHz );
			}
		}
		case SYSTEM_FIELD_FRAMERATE:
			return va( "%d FPS", com_engineHz.GetInteger() );
		case SYSTEM_FIELD_VSYNC:
			if( r_swapInterval.GetInteger() == 1 )
			{
				return "#str_swf_smart";
			}
			else if( r_swapInterval.GetInteger() == 2 )
			{
				return "#str_swf_enabled";
			}
			else
			{
				return "#str_swf_disabled";
			}
		case SYSTEM_FIELD_ANTIALIASING:
			if( r_multiSamples.GetInteger() == 0 )
			{
				return "#str_swf_disabled";
			}
			return va( "%dx", r_multiSamples.GetInteger() );
		case SYSTEM_FIELD_MOTIONBLUR:
			if( r_motionBlur.GetInteger() == 0 )
			{
				return "#str_swf_disabled";
			}
			return va( "%dx", idMath::IPow( 2, r_motionBlur.GetInteger() ) );
		case SYSTEM_FIELD_LODBIAS:
			return LinearAdjust( r_lodBias.GetFloat(), -1.0f, 1.0f, 0.0f, 100.0f );
		case SYSTEM_FIELD_BRIGHTNESS:
			return LinearAdjust(r_lightScale.GetFloat(), 2.0f, 4.0f, 0.0f, 100.0f);
		case SYSTEM_FIELD_VOLUME:
		{
			return 100.0f * Square( 1.0f - ( s_volume_dB.GetFloat() / DB_SILENCE ) );
		}
		case SYSTEM_FIELD_SHADOWMAPPING:
			return r_useShadowMapping.GetBool();
		case SYSTEM_FIELD_SHADOWMAPQUALITY:
		{
			int shadowmapQuality = r_shadowMapQuality.GetInteger();
			return valueToQuality(shadowmapQuality);
		}
		case SYSTEM_FIELD_SHADOWMAPDISTANCE:
			return LinearAdjust(r_shadowMapMaxDistance.GetFloat(), 512.0f, 16384.0f, 0.0f, 100.0f);
		case SYSTEM_FIELD_HIGH_QUALITY_SKY:
			return r_useHightQualitySky.GetBool();
		case SYSTEM_FIELD_HDR_ENABLE:
			return r_useHDR.GetBool();
		case SYSTEM_FIELD_GLOW_ENABLE:
			return r_glowEnable.GetBool();
		case SYSTEM_FIELD_PNG:
			return r_screenshot_png.GetBool();
		case SYSTEM_FIELD_PNG_QUALITY:
			return png_valueToQuality(r_screenshot_png_quality.GetInteger());
		/*case SYSTEM_FIELD_SHADOWMAPIMAGESIZE:
		{
			int shadowmapImageSizeRes = r_shadowMapLodScale.GetInteger();
			return imageSize_valueToQuality( shadowmapImageSizeRes );
		}*/
			//return va( "%4i x %4i", r_shadowMapLodScale.GetInteger(), r_shadowMapLodScale.GetInteger() );
		case SYSTEM_FIELD_SHADOWMAPSAMPLES:
			if (r_shadowMapSamples.GetInteger() == 0)
			{
				return "#str_swf_disabled";
			}
			return va("%dx", r_shadowMapSamples.GetInteger());
	}
	return false;
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsDataChanged
========================
*/
bool idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsDataChanged() const
{
	if( originalFramerate != com_engineHz.GetInteger() )
	{
		return true;
	}
	if( originalAntialias != r_multiSamples.GetInteger() )
	{
		return true;
	}
	if( originalMotionBlur != r_motionBlur.GetInteger() )
	{
		return true;
	}
	if( originalVsync != r_swapInterval.GetInteger() )
	{
		return true;
	}
	if( originalBrightness != r_lightScale.GetFloat() )
	{
		return true;
	}
	if( originalVolume != s_volume_dB.GetFloat() )
	{
		return true;
	}
	if (originalShadowMapEnable != r_useShadowMapping.GetBool())
	{
		return true;
	}
	/*if( originalShadowMapImageSize != r_shadowMapLodScale.GetInteger() )
	{
		return true;
	}*/
	if (originalShadowMapQuality != r_shadowMapQuality.GetInteger())
	{
		return true;
	}
	if (originalShadowMapSampleCount != r_shadowMapSamples.GetInteger())
	{
		return true;
	}
	if (originalShadowMapDistance != r_shadowMapMaxDistance.GetFloat())
	{
		return true;
	}
	if (originalUsePNG != r_screenshot_png.GetBool())
	{
		return true;
	}
	if (originalPNGQuality != r_screenshot_png_quality.GetInteger())
	{
		return true;
	}
	if (r_glowEnable.GetBool() != originalGlowEnable)
	{
		return true;
	}
	if (originalHDREnable != r_useHDR.GetBool())
		return true;
	if (originalHQSky != r_useHightQualitySky.GetBool())
		return true;
	return false;
}

