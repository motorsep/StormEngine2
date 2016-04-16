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


uniform vec4 _fa_[28];

float dot3 (vec3 a , vec3 b ) {return dot ( a , b ) ; }
float dot3 (vec3 a , vec4 b ) {return dot ( a , b. xyz ) ; }
float dot3 (vec4 a , vec3 b ) {return dot ( a. xyz , b ) ; }
float dot3 (vec4 a , vec4 b ) {return dot ( a. xyz , b. xyz ) ; }
float dot4 (vec4 a , vec4 b ) {return dot ( a , b ) ; }
float dot4 (vec2 a , vec4 b ) {return dot ( vec4 ( a , 0 , 1 ) , b ) ; }
const vec4 matrixCoCg1YtoRGB1X = vec4( 1.0, -1.0, 0.0, 1.0 );
const vec4 matrixCoCg1YtoRGB1Y = vec4( 0.0, 1.0, -0.50196078, 1.0 );
const vec4 matrixCoCg1YtoRGB1Z = vec4( -1.0, -1.0, 1.00392156, 1.0 );
vec3 ConvertYCoCgToRGB (vec4 YCoCg ) {
	vec3 rgbColor ;
	YCoCg. z = ( YCoCg. z * 31.875 ) + 1.0 ;
	YCoCg. z = 1.0 / YCoCg. z ;
	YCoCg. xy *= YCoCg. z ;
	rgbColor. x = dot4 ( YCoCg , matrixCoCg1YtoRGB1X ) ;
	rgbColor. y = dot4 ( YCoCg , matrixCoCg1YtoRGB1Y ) ;
	rgbColor. z = dot4 ( YCoCg , matrixCoCg1YtoRGB1Z ) ;
	return rgbColor ;
}
vec4 idtex2Dproj (sampler2D samp , vec4 texCoords ) {return tex2Dproj ( samp , texCoords. xyw ) ; }
const float diffuseScale = 5.2;
const float diffuseBias = -0.5;
const float softScale = 0.55;
const float softBias = 0.3;
uniform sampler2D samp0;
uniform sampler2D samp1;
uniform sampler2D samp2;
uniform sampler2D samp3;
uniform sampler2D samp4;
uniform sampler2DArrayShadow samp5;
uniform sampler2D samp6;
uniform sampler2D samp7;

in vec4 vofi_TexCoord0;
in vec4 vofi_TexCoord1;
in vec4 vofi_TexCoord2;
in vec4 vofi_TexCoord3;
in vec4 vofi_TexCoord4;
in vec4 vofi_TexCoord5;
in vec4 vofi_TexCoord6;
in vec4 vofi_TexCoord7;
in vec4 vofi_TexCoord8;
in vec4 vofi_TexCoord9;
in vec4 vofi_TexCoord10;
in vec4 vofi_Color;

out vec4 fo_Color;

void main() {
	vec4 bumpMap = tex2D ( samp0 , vofi_TexCoord1 . xy ) ;
	vec4 lightFalloff = idtex2Dproj ( samp1 , vofi_TexCoord2 ) ;
	vec4 lightProj = idtex2Dproj ( samp2 , vofi_TexCoord3 ) ;
	vec4 YCoCG = tex2D ( samp3 , vofi_TexCoord4 . xy ) ;
	vec4 specMap = tex2D ( samp4 , vofi_TexCoord5 . xy ) ;
	vec4 glossMap = tex2D ( samp7 , vofi_TexCoord10 . xy ) ;
	vec3 lightVector = normalize ( vofi_TexCoord0 . xyz ) ;
	vec3 diffuseMap = ConvertYCoCgToRGB ( YCoCG ) ;
	vec3 localNormal ;
	localNormal. xy = bumpMap. wy - 0.5 ;
	localNormal. z = sqrt ( abs ( dot ( localNormal. xy , localNormal. xy ) - 0.25 ) ) ;
	localNormal = normalize ( localNormal ) ;
	float specularPower = glossMap. x * 32 ;
	float hDotN = saturate ( dot3 ( normalize ( vofi_TexCoord6 . xyz ) , localNormal ) ) ;
	vec3 specularContribution = vec3 ( pow ( hDotN , specularPower ) ) ;
	vec3 diffuseColor = diffuseMap. xyz * _fa_[1 /* rpDiffuseModifier */] . xyz ;
	vec3 specularColor = specMap. xyz * specularContribution * _fa_[2 /* rpSpecularModifier */] . xyz ;
	float mainShade = saturate ( dot3 ( lightVector , localNormal ) ) ;
	float softShade = saturate ( mainShade * softScale + softBias ) ;
	float finalShade = saturate ( mainShade * diffuseScale + diffuseBias ) * softShade ;
	vec3 lightColor = finalShade * lightProj. xyz * lightFalloff. xyz ;
	float bias = 0.000035 ;
	vec4 shadowMatrixX ;
	vec4 shadowMatrixY ;
	vec4 shadowMatrixZ ;
	vec4 shadowMatrixW ;
	vec4 shadowTexcoord ;
	vec4 modelPosition = vec4 ( vofi_TexCoord7 . xyz , 1.0 ) ;
	shadowTexcoord. xyz =
	vec3 ( dot4 ( modelPosition , _fa_[4 /* rpShadowMatrix0X */] ) ,
	dot4 ( modelPosition , _fa_[5 /* rpShadowMatrix0Y */] ) ,
	dot4 ( modelPosition , _fa_[6 /* rpShadowMatrix0Z */] ) ) /
	dot4 ( modelPosition , _fa_[7 /* rpShadowMatrix0W */] ) ;
	shadowTexcoord. w = 0.0 ;
	vec3 toLightGlobal = normalize ( vofi_TexCoord8 . xyz ) ;
	float axis [ 6 ] ;
	axis [ 0 ] = - toLightGlobal. x ;
	axis [ 1 ] = toLightGlobal. x ;
	axis [ 2 ] = - toLightGlobal. y ;
	axis [ 3 ] = toLightGlobal. y ;
	axis [ 4 ] = - toLightGlobal. z ;
	axis [ 5 ] = toLightGlobal. z ;
	float best = axis [ 0 ] ;
	if ( best < axis [ 1 ] )
	{
		best = axis [ 1 ] ;
		shadowTexcoord. xyz =
		vec3 ( dot4 ( modelPosition , _fa_[8 /* rpShadowMatrix1X */] ) ,
		dot4 ( modelPosition , _fa_[9 /* rpShadowMatrix1Y */] ) ,
		dot4 ( modelPosition , _fa_[10 /* rpShadowMatrix1Z */] ) ) /
		dot4 ( modelPosition , _fa_[11 /* rpShadowMatrix1W */] ) ;
		shadowTexcoord. w = 1.0 ;
	}
	if ( best < axis [ 2 ] )
	{
		best = axis [ 2 ] ;
		shadowTexcoord. xyz =
		vec3 ( dot4 ( modelPosition , _fa_[12 /* rpShadowMatrix2X */] ) ,
		dot4 ( modelPosition , _fa_[13 /* rpShadowMatrix2Y */] ) ,
		dot4 ( modelPosition , _fa_[14 /* rpShadowMatrix2Z */] ) ) /
		dot4 ( modelPosition , _fa_[15 /* rpShadowMatrix2W */] ) ;
		shadowTexcoord. w = 2.0 ;
	}
	if ( best < axis [ 3 ] )
	{
		best = axis [ 3 ] ;
		shadowTexcoord. xyz =
		vec3 ( dot4 ( modelPosition , _fa_[16 /* rpShadowMatrix3X */] ) ,
		dot4 ( modelPosition , _fa_[17 /* rpShadowMatrix3Y */] ) ,
		dot4 ( modelPosition , _fa_[18 /* rpShadowMatrix3Z */] ) ) /
		dot4 ( modelPosition , _fa_[19 /* rpShadowMatrix3W */] ) ;
		shadowTexcoord. w = 3.0 ;
	}
	if ( best < axis [ 4 ] )
	{
		best = axis [ 4 ] ;
		shadowTexcoord. xyz =
		vec3 ( dot4 ( modelPosition , _fa_[20 /* rpShadowMatrix4X */] ) ,
		dot4 ( modelPosition , _fa_[21 /* rpShadowMatrix4Y */] ) ,
		dot4 ( modelPosition , _fa_[22 /* rpShadowMatrix4Z */] ) ) /
		dot4 ( modelPosition , _fa_[23 /* rpShadowMatrix4W */] ) ;
		shadowTexcoord. w = 4.0 ;
	}
	if ( best < axis [ 5 ] )
	{
		best = axis [ 5 ] ;
		shadowTexcoord. xyz =
		vec3 ( dot4 ( modelPosition , _fa_[24 /* rpShadowMatrix5X */] ) ,
		dot4 ( modelPosition , _fa_[25 /* rpShadowMatrix5Y */] ) ,
		dot4 ( modelPosition , _fa_[26 /* rpShadowMatrix5Z */] ) ) /
		dot4 ( modelPosition , _fa_[27 /* rpShadowMatrix5W */] ) ;
		shadowTexcoord. w = 5.0 ;
	}
	shadowTexcoord. z -= bias ;
	float shadow = 0.0 ;
	float shadowTexelSize = _fa_[0 /* rpScreenCorrectionFactor */] . z * _fa_[3 /* rpJitterTexScale */] . x ;
	vec2 poissonDisk [ 12 ] = vec2 [ ] (
	vec2 ( 0.6111618 , 0.1050905 ) ,
	vec2 ( 0.1088336 , 0.1127091 ) ,
	vec2 ( 0.3030421 , - 0.6292974 ) ,
	vec2 ( 0.4090526 , 0.6716492 ) ,
	vec2 ( - 0.1608387 , - 0.3867823 ) ,
	vec2 ( 0.7685862 , - 0.6118501 ) ,
	vec2 ( - 0.1935026 , - 0.856501 ) ,
	vec2 ( - 0.4028573 , 0.07754025 ) ,
	vec2 ( - 0.6411021 , - 0.4748057 ) ,
	vec2 ( - 0.1314865 , 0.8404058 ) ,
	vec2 ( - 0.7005203 , 0.4596822 ) ,
	vec2 ( - 0.9713828 , - 0.06329931 ) ) ;
	float numSamples = 12 ;
	float stepSize = 1.0 / numSamples ;
	vec4 jitterTC = ( gl_FragCoord * _fa_[0 /* rpScreenCorrectionFactor */] ) ;
	vec4 random = tex2D ( samp6 , jitterTC. xy * 1.0 ) * 3.14159265358979323846 ;
	vec2 rot ;
	rot. x = cos ( random. x ) ;
	rot. y = sin ( random. x ) ;
	for ( int i = 0 ; i < 12 ; i ++ )
	{
		vec2 jitter = poissonDisk [ i ] ;
		vec2 jitterRotated ;
		jitterRotated. x = jitter. x * rot. x - jitter. y * rot. y ;
		jitterRotated. y = jitter. x * rot. y + jitter. y * rot. x ;
		vec4 shadowTexcoordJittered = vec4 ( shadowTexcoord. xy + jitterRotated * shadowTexelSize , shadowTexcoord. z , shadowTexcoord. w ) ;
		shadow += texture ( samp5 , shadowTexcoordJittered. xywz * 1.0 ) ;
	}
	shadow *= stepSize ;
	fo_Color . xyz = ( diffuseColor + specularColor ) * lightColor * vofi_Color . xyz * shadow ;
	fo_Color . w = 1.0 ;
}