#version 150
#define PC

float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }
vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }
vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }
vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }
vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }


uniform vec4 _va_[2];


in vec4 in_Position;
in vec2 in_TexCoord;
in vec4 in_Normal;
in vec4 in_Tangent;
in vec4 in_Color;

out vec4 vofi_TexCoord0;
out vec4 vofi_TexCoord1;
out vec4 vofi_TexCoord2;
out vec4 vofi_TexCoord3;
out vec4 vofi_TexCoord4;
out vec4 vofi_TexCoord5;
out vec4 vofi_TexCoord6;
out vec4 vofi_TexCoord7;

void main() {
	gl_Position = in_Position ;
	vec2 tCoords = in_TexCoord * _va_[0 /* rpScreenCorrectionFactor */] . xy + _va_[0 /* rpScreenCorrectionFactor */] . zw ;
	vec4 tCoords4 = tCoords. xyxy ;
	vofi_TexCoord0 = tCoords4 + _va_[1 /* rpDiffuseModifier */] * 1.0 ;
	vofi_TexCoord1 = tCoords4 + _va_[1 /* rpDiffuseModifier */] * 2.5 ;
	vofi_TexCoord2 = tCoords4 + _va_[1 /* rpDiffuseModifier */] * 4.5 ;
	vofi_TexCoord3 = tCoords4 + _va_[1 /* rpDiffuseModifier */] * 6.5 ;
	vofi_TexCoord4 = tCoords4 + _va_[1 /* rpDiffuseModifier */] * 8.5 ;
	vofi_TexCoord5 = tCoords4 + _va_[1 /* rpDiffuseModifier */] * 10.5 ;
	vofi_TexCoord6 = tCoords4 + _va_[1 /* rpDiffuseModifier */] * 12.5 ;
	vofi_TexCoord7 = tCoords4 + _va_[1 /* rpDiffuseModifier */] * 14.5 ;
}