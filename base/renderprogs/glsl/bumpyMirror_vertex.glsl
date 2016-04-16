#version 150
#define PC

float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }
vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }
vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }
vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }
vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }


uniform vec4 _va_[9];

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

out vec4 vofi_TexCoord0;
out vec4 vofi_TexCoord1;
out vec4 vofi_TexCoord2;
out vec4 vofi_TexCoord3;
out vec4 vofi_TexCoord4;
out vec4 vofi_TexCoord5;
out vec4 vofi_TexCoord6;
out vec4 vofi_TexCoord7;

void main() {
	vec4 modelPosition = in_Position ; if ( _va_[5 /* rpEnableSkinning */] . x > 0.0 ) {
		float w0 = in_Color2 . x ;
		float w1 = in_Color2 . y ;
		float w2 = in_Color2 . z ;
		float w3 = in_Color2 . w ;
		vec4 matX , matY , matZ ;
		float joint = in_Color . x * 255.1 * 3 ;
		matX = matrices [ int ( joint + 0 ) ] * w0 ;
		matY = matrices [ int ( joint + 1 ) ] * w0 ;
		matZ = matrices [ int ( joint + 2 ) ] * w0 ;
		joint = in_Color . y * 255.1 * 3 ;
		matX += matrices [ int ( joint + 0 ) ] * w1 ;
		matY += matrices [ int ( joint + 1 ) ] * w1 ;
		matZ += matrices [ int ( joint + 2 ) ] * w1 ;
		joint = in_Color . z * 255.1 * 3 ;
		matX += matrices [ int ( joint + 0 ) ] * w2 ;
		matY += matrices [ int ( joint + 1 ) ] * w2 ;
		matZ += matrices [ int ( joint + 2 ) ] * w2 ;
		joint = in_Color . w * 255.1 * 3 ;
		matX += matrices [ int ( joint + 0 ) ] * w3 ;
		matY += matrices [ int ( joint + 1 ) ] * w3 ;
		matZ += matrices [ int ( joint + 2 ) ] * w3 ;
		modelPosition. x = dot4 ( matX , in_Position ) ;
		modelPosition. y = dot4 ( matY , in_Position ) ;
		modelPosition. z = dot4 ( matZ , in_Position ) ;
		modelPosition. w = 1.0 ;
	}
	gl_Position . x = dot4 ( modelPosition , _va_[1 /* rpMVPmatrixX */] ) ;
	gl_Position . y = dot4 ( modelPosition , _va_[2 /* rpMVPmatrixY */] ) ;
	gl_Position . z = dot4 ( modelPosition , _va_[3 /* rpMVPmatrixZ */] ) ;
	gl_Position . w = dot4 ( modelPosition , _va_[4 /* rpMVPmatrixW */] ) ;
	vofi_TexCoord0 = vec4 ( in_TexCoord . xy , 0 , 0 ) ;
	vofi_TexCoord1 = _va_[6 /* rpUser0 */] ;
	vofi_TexCoord1 . w = dot4 ( in_Position , _va_[4 /* rpMVPmatrixW */] ) ;
	vofi_TexCoord2 = _va_[7 /* rpUser1 */] ;
	vofi_TexCoord3 = _va_[8 /* rpUser2 */] ;
	vec4 normal = in_Normal * 2.0 - 1.0 ;
	vec4 tangent = in_Tangent * 2.0 - 1.0 ;
	vec3 binormal = cross ( normal. xyz , tangent. xyz ) * tangent. w ;
	vec4 viewVec = _va_[0 /* rpLocalViewOrigin */] - in_Position ;
	vofi_TexCoord4 . x = dot3 ( viewVec , tangent ) ;
	vofi_TexCoord4 . y = dot3 ( viewVec , binormal ) ;
	vofi_TexCoord4 . z = dot3 ( viewVec , normal ) ;
	vofi_TexCoord5 . x = dot3 ( tangent , _va_[1 /* rpMVPmatrixX */] ) ;
	vofi_TexCoord5 . y = dot3 ( binormal , _va_[1 /* rpMVPmatrixX */] ) ;
	vofi_TexCoord5 . z = dot3 ( normal , _va_[1 /* rpMVPmatrixX */] ) ;
	vofi_TexCoord6 . x = dot3 ( tangent , _va_[2 /* rpMVPmatrixY */] ) ;
	vofi_TexCoord6 . y = dot3 ( binormal , _va_[2 /* rpMVPmatrixY */] ) ;
	vofi_TexCoord6 . z = dot3 ( normal , _va_[2 /* rpMVPmatrixY */] ) ;
	vofi_TexCoord7 . x = dot3 ( tangent , _va_[3 /* rpMVPmatrixZ */] ) ;
	vofi_TexCoord7 . y = dot3 ( binormal , _va_[3 /* rpMVPmatrixZ */] ) ;
	vofi_TexCoord7 . z = dot3 ( normal , _va_[3 /* rpMVPmatrixZ */] ) ;
}