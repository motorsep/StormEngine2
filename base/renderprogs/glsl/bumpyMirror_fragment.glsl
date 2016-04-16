#version 150
#define PC

void clip( float v ) { if ( v < 0.0 ) { discard; } }
void clip( vec2 v ) { if ( any( lessThan( v, vec2( 0.0 ) ) ) ) { discard; } }
void clip( vec3 v ) { if ( any( lessThan( v, vec3( 0.0 ) ) ) ) { discard; } }
void clip( vec4 v ) { if ( any( lessThan( v, vec4( 0.0 ) ) ) ) { discard; } }

float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }
vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }
vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }
vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }

vec4 tex2D( sampler2D sampler, vec2 texcoord ) { return texture( sampler, texcoord.xy ); }
vec4 tex2D( sampler2DShadow sampler, vec3 texcoord ) { return vec4( texture( sampler, texcoord.xyz ) ); }

vec4 tex2D( sampler2D sampler, vec2 texcoord, vec2 dx, vec2 dy ) { return textureGrad( sampler, texcoord.xy, dx, dy ); }
vec4 tex2D( sampler2DShadow sampler, vec3 texcoord, vec2 dx, vec2 dy ) { return vec4( textureGrad( sampler, texcoord.xyz, dx, dy ) ); }

vec4 texCUBE( samplerCube sampler, vec3 texcoord ) { return texture( sampler, texcoord.xyz ); }
vec4 texCUBE( samplerCubeShadow sampler, vec4 texcoord ) { return vec4( texture( sampler, texcoord.xyzw ) ); }

vec4 tex1Dproj( sampler1D sampler, vec2 texcoord ) { return textureProj( sampler, texcoord ); }
vec4 tex2Dproj( sampler2D sampler, vec3 texcoord ) { return textureProj( sampler, texcoord ); }
vec4 tex3Dproj( sampler3D sampler, vec4 texcoord ) { return textureProj( sampler, texcoord ); }

vec4 tex1Dbias( sampler1D sampler, vec4 texcoord ) { return texture( sampler, texcoord.x, texcoord.w ); }
vec4 tex2Dbias( sampler2D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xy, texcoord.w ); }
vec4 tex3Dbias( sampler3D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }
vec4 texCUBEbias( samplerCube sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }

vec4 tex1Dlod( sampler1D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.x, texcoord.w ); }
vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }
vec4 tex3Dlod( sampler3D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }
vec4 texCUBElod( samplerCube sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }


uniform vec4 _fa_[1];

float dot3 (vec3 a , vec3 b ) {return dot ( a , b ) ; }
float dot3 (vec3 a , vec4 b ) {return dot ( a , b. xyz ) ; }
float dot3 (vec4 a , vec3 b ) {return dot ( a. xyz , b ) ; }
float dot3 (vec4 a , vec4 b ) {return dot ( a. xyz , b. xyz ) ; }
vec2 vposToScreenPosTexCoord (vec2 vpos ) {return vpos. xy * _fa_[0 /* rpWindowCoord */] . xy + _fa_[0 /* rpWindowCoord */] . zw ; }
uniform sampler2D samp0;
uniform sampler2D samp1;
uniform sampler2D samp2;

in vec4 vofi_TexCoord0;
in vec4 vofi_TexCoord1;
in vec4 vofi_TexCoord2;
in vec4 vofi_TexCoord3;
in vec4 vofi_TexCoord4;
in vec4 vofi_TexCoord5;
in vec4 vofi_TexCoord6;
in vec4 vofi_TexCoord7;

out vec4 fo_Color;

const vec2 poisson8 [8] = vec2[8]( vec2( 0.100000, 0.300000 ), vec2( 0.527837, -0.085868 ), vec2( -0.040088, 0.536087 ), vec2( -0.170445, -0.179949 ), vec2( -0.419418, -0.616039 ), vec2( 0.440453, -0.639399 ), vec2( -0.757088, 0.349334 ), vec2( 0.574619, 0.685879 ) );
void main() {
	vec4 mask = tex2D ( samp2 , vofi_TexCoord0 . xy ) ;
	vec4 normalVec = tex2D ( samp1 , vofi_TexCoord0 . xy ) * 2.0 - 1.0 ;
	normalVec. xyz = normalize ( normalVec. wyz ) ;
	float Fresnel = 1.0 - saturate ( dot3 ( normalVec. xyz , - normalize ( vofi_TexCoord4 . xyz ) ) ) ;
	Fresnel = pow ( Fresnel , vofi_TexCoord2 . x ) * vofi_TexCoord2 . y + vofi_TexCoord2 . z ;
	vec2 viewNormal ;
	viewNormal. x = dot3 ( normalVec. xyz , vofi_TexCoord5 ) ;
	viewNormal. y = dot3 ( normalVec. xyz , vofi_TexCoord6 ) ;
	vec2 scale = min ( vofi_TexCoord1 . xy / vofi_TexCoord1 . ww , vec2 ( 0.05 , 0.01 ) ) ;
	vec2 mins = vec2 ( _fa_[0 /* rpWindowCoord */] . xy * 4 ) ;
	vec2 maxs = 1 - mins ;
	vec2 screenTC = vposToScreenPosTexCoord ( gl_FragCoord . xy ) ;
	screenTC += ( viewNormal. xy * scale. xx ) ;
	screenTC = max ( min ( screenTC , maxs ) , mins ) ;
	vec4 color = tex2D ( samp0 , screenTC ) ;
	if ( vofi_TexCoord1 . z > 0 ) {
		for ( int i = 0 ; i < 8 ; i ++ ) {
			vec2 uv = screenTC + poisson8 [ i ]. xy * scale. yy ;
			color += tex2D ( samp0 , max ( min ( uv , maxs ) , mins ) ) ;
		}
		color = color / 9 ;
	}
	vec3 tint = dot3 ( color. xyz , vec3 ( 0.212671 , 0.715160 , 0.072169 ) ) * vofi_TexCoord3 . xyz ;
	color. xyz = mix ( color. xyz , tint. xyz , vofi_TexCoord3 . w ) * mask. xyz * Fresnel ;
	fo_Color = color ;
}