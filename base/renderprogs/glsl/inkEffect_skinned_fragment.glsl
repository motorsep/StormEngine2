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
uniform sampler2D depthImage;

in vec4 vofi_TexCoord0;
in vec4 vofi_TexCoord1;
in vec4 vofi_TexCoord2;

out vec4 fo_Color;

float linearizeDepth (float inDepth ) {
	float
	zNear = 3.0 ,
	zFar = 3000.0 ;
	float
	zN = 2.0 * inDepth - 1.0 ;
	return 2.0 * zNear * zFar / ( zFar + zNear - zN * ( zFar - zNear ) ) ;
}
float GetDepth (sampler2D tex , vec2 uv ) {
	return tex2D ( tex , uv ). x ;
}
float GetLinearDepth (sampler2D tex , vec2 uv ) {
	return linearizeDepth ( GetDepth ( tex , uv ) ) ;
}
float InkTestSimple3 (float sample1 , float sample2 , float sample3 ) {
	return saturate ( ( sample3 - sample2 ) - ( sample2 - sample1 ) ) ;
}
float inkSimple (vec2 basePos , vec3 inkParms ) {
	vec3
	sampleDir = vec3 ( 1.0 , 0.0 , - 1.0 ) ;
	float
	baseSample = GetDepth ( depthImage , basePos ) ;
	vec2
	sampleSize = _fa_[0 /* rpWindowCoord */] . xy * inkParms. x ;
	float
	samples [ 8 ] ;
	samples [ 0 ] = GetDepth ( depthImage , sampleDir. xx * sampleSize + basePos ) ;
	samples [ 1 ] = GetDepth ( depthImage , sampleDir. xy * sampleSize + basePos ) ;
	samples [ 2 ] = GetDepth ( depthImage , sampleDir. xz * sampleSize + basePos ) ;
	samples [ 3 ] = GetDepth ( depthImage , sampleDir. yx * sampleSize + basePos ) ;
	samples [ 4 ] = GetDepth ( depthImage , sampleDir. yz * sampleSize + basePos ) ;
	samples [ 5 ] = GetDepth ( depthImage , sampleDir. zx * sampleSize + basePos ) ;
	samples [ 6 ] = GetDepth ( depthImage , sampleDir. zy * sampleSize + basePos ) ;
	samples [ 7 ] = GetDepth ( depthImage , sampleDir. zz * sampleSize + basePos ) ;
	float
	s1 = InkTestSimple3 ( samples [ 6 ] , baseSample , samples [ 1 ] ) ,
	s2 = InkTestSimple3 ( samples [ 3 ] , baseSample , samples [ 4 ] ) ,
	s3 = InkTestSimple3 ( samples [ 5 ] , baseSample , samples [ 2 ] ) ,
	s4 = InkTestSimple3 ( samples [ 7 ] , baseSample , samples [ 0 ] ) ;
	return 1.0 - saturate ( ( ( s1 + s2 + s3 + s4 ) * 0.25 ) * inkParms. y + inkParms. z ) ;
}
void main() {
	vec2
	basePos = vposToScreenPosTexCoord ( gl_FragCoord . xy ) ;
	float
	linDepth = GetLinearDepth ( depthImage , basePos ) ,
	parmLerp = saturate ( linDepth * vofi_TexCoord0 . y + vofi_TexCoord0 . z ) ;
	vec3
	iParms = mix ( vofi_TexCoord1 . xyz , vofi_TexCoord2 . xyz , parmLerp ) ;
	float
	inkValue = mix ( 1.0 , inkSimple ( basePos , iParms ) , vofi_TexCoord0 . x ) ;
	vec4
	outColor ;
	outColor. xyz = vec3 ( inkValue ) ;
	outColor. a = 1.0 ;
	fo_Color = outColor ;
}