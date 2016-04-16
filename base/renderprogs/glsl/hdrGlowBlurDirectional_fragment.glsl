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

uniform sampler2D samp0;

in vec4 vofi_TexCoord0;
in vec4 vofi_TexCoord1;
in vec4 vofi_TexCoord2;
in vec4 vofi_TexCoord3;
in vec4 vofi_TexCoord4;
in vec4 vofi_TexCoord5;
in vec4 vofi_TexCoord6;
in vec4 vofi_TexCoord7;

out vec4 fo_Color;

void main() {
	vec4 tCoords0 = vofi_TexCoord0 ;
	vec4 tCoords1 = vofi_TexCoord1 ;
	vec4 tCoords2 = vofi_TexCoord2 ;
	vec4 tCoords3 = vofi_TexCoord3 ;
	vec4 tCoords4 = vofi_TexCoord4 ;
	vec4 tCoords5 = vofi_TexCoord5 ;
	vec4 tCoords6 = vofi_TexCoord6 ;
	vec4 tCoords7 = vofi_TexCoord7 ;
	fo_Color = ( ( tex2D ( samp0 , tCoords0. xy ) + tex2D ( samp0 , tCoords0. zw ) ) * 1.6
	+ ( tex2D ( samp0 , tCoords1. xy ) + tex2D ( samp0 , tCoords1. zw ) ) * 1.1
	+ ( tex2D ( samp0 , tCoords2. xy ) + tex2D ( samp0 , tCoords2. zw ) ) * 0.8
	+ ( tex2D ( samp0 , tCoords3. xy ) + tex2D ( samp0 , tCoords3. zw ) ) * 0.6
	+ ( tex2D ( samp0 , tCoords4. xy ) + tex2D ( samp0 , tCoords4. zw ) ) * 0.5
	+ ( tex2D ( samp0 , tCoords5. xy ) + tex2D ( samp0 , tCoords5. zw ) ) * 0.3
	+ ( tex2D ( samp0 , tCoords6. xy ) + tex2D ( samp0 , tCoords6. zw ) ) * 0.2
	+ ( tex2D ( samp0 , tCoords7. xy ) + tex2D ( samp0 , tCoords7. zw ) ) * 0.1 ) * 0.17857 ;
}