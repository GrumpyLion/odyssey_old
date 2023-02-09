#version 450

layout (location = 0) out vec4 outFragColor;
layout (location = 0) in vec3 inColor;

layout(set = 0, binding = 1) uniform SceneDataUniform 
{
    vec4 myFogColor; // w is for exponent
	vec4 myFogDistances; //x for min, y for max, zw unused.
	vec4 myAmbientColor;
	vec4 mySunlightDirection; //w for sun power
	vec4 mySunlightColor;
} SceneData;

void main()
{
	outFragColor = vec4(inColor + SceneData.myAmbientColor.xyz, 1.0f);
}