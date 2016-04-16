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

vec2 vposToScreenPosTexCoord (vec2 vpos ) {return vpos. xy * _fa_[0 /* rpWindowCoord */] . xy + _fa_[0 /* rpWindowCoord */] . zw ; }
uniform sampler2D samp0;
uniform sampler2D samp1;

in vec2 vofi_TexCoord0;
in vec4 vofi_TexCoord1;
in vec4 vofi_TexCoord2;
in vec4 vofi_TexCoord3;
in vec4 vofi_Color;

out vec4 fo_Color;

float LinearizeDepth (float inDepth ) {
	float
	zNear = 5.0 ,
	zFar = 3000.0 ;
	float
	zN = 2.0 * inDepth - 1.0 ;
	return 2.0 * zNear * zFar / ( zFar + zNear - zN * ( zFar - zNear ) ) ;
}
float GetDepth (sampler2D tex , vec2 uv ) {
	return tex2D ( tex , uv ). x ;
}
float GetLinearDepth (sampler2D tex , vec2 uv ) {
	return LinearizeDepth ( GetDepth ( tex , uv ) ) ;
}
float GetDistanceFalloff (float baseDist , float start , float range ) {
	return saturate ( ( baseDist - start ) / range ) ;
}
float ZFeather (sampler2D depthSampler , vec2 depthSamplePos , float fragmentDepth , float featherAmount ) {
	float
	d = GetLinearDepth ( depthSampler , depthSamplePos ) ,
	d2 = LinearizeDepth ( fragmentDepth ) ;
	return saturate ( ( d - d2 ) * featherAmount ) ;
}
void main() {
	vec4
	texSample = tex2D ( samp1 , vofi_TexCoord0 ) ;
	vec4
	outColor ;
	outColor = texSample * vofi_Color ;
	vec2
	screenPos = vposToScreenPosTexCoord ( gl_FragCoord . xy ) ;
	float
	fragDepth = gl_FragCoord . z / gl_FragCoord . w ;
	outColor. rgb *= ZFeather ( samp0 , screenPos , gl_FragCoord . z , vofi_TexCoord1 . x ) ;
	outColor. rgb *= GetDistanceFalloff ( fragDepth , vofi_TexCoord2 . x , vofi_TexCoord2 . y ) ;
	outColor. rgb *= saturate ( 1.0 - ( fragDepth * vofi_TexCoord3 . a ) ) ;
	fo_Color = outColor ;
}