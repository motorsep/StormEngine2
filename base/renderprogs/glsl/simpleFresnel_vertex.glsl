#version 150
#define PC

float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }
vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }
vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }
vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }
vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }


uniform vec4 _va_[8];

float dot3 (vec3 a , vec3 b ) {return dot ( a , b ) ; }
float dot3 (vec3 a , vec4 b ) {return dot ( a , b. xyz ) ; }
float dot3 (vec4 a , vec3 b ) {return dot ( a. xyz , b ) ; }
float dot3 (vec4 a , vec4 b ) {return dot ( a. xyz , b. xyz ) ; }
float dot4 (vec4 a , vec4 b ) {return dot ( a , b ) ; }
float dot4 (vec2 a , vec4 b ) {return dot ( vec4 ( a , 0 , 1 ) , b ) ; }
uniform matrices_ubo {vec4 matrices [ 408 ] ; } ;

in vec4 in_Position;
in vec2 in_TexCoord;
in vec4 in_Normal;
in vec4 in_Tangent;
in vec4 in_Color;
in vec4 in_Color2;

out vec2 vofi_TexCoord0;
out vec3 vofi_TexCoord1;
out vec4 vofi_TexCoord2;
out vec4 vofi_TexCoord3;
out vec4 vofi_TexCoord4;
out vec4 vofi_TexCoord5;
out vec4 vofi_TexCoord6;
out vec4 vofi_Color;

void main() {
	gl_Position . x = dot4 ( in_Position , _va_[2 /* rpMVPmatrixX */] ) ;
	gl_Position . y = dot4 ( in_Position , _va_[3 /* rpMVPmatrixY */] ) ;
	gl_Position . z = dot4 ( in_Position , _va_[4 /* rpMVPmatrixZ */] ) ;
	gl_Position . w = dot4 ( in_Position , _va_[5 /* rpMVPmatrixW */] ) ;
	vec4 normal = in_Normal * 2.0 - 1.0 ;
	vec4 tangent = in_Tangent * 2.0 - 1.0 ;
	vec3 binormal = cross ( normal. xyz , tangent. xyz ) * tangent. w ;
	vofi_TexCoord0 = in_TexCoord . xy ;
	vec4 toEye = _va_[0 /* rpLocalViewOrigin */] - in_Position ;
	vofi_TexCoord1 . x = dot3 ( tangent. xyz , toEye ) ;
	vofi_TexCoord1 . y = dot3 ( binormal , toEye ) ;
	vofi_TexCoord1 . z = dot3 ( normal , toEye ) ;
	vofi_TexCoord2 = _va_[6 /* rpUser0 */] ;
	vofi_TexCoord3 = _va_[7 /* rpUser1 */] ;
	vofi_Color = saturate ( _va_[1 /* rpColor */] ) ;
}