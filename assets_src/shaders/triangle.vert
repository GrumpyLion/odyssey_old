#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;


layout(set = 0, binding = 0) uniform CameraBufferUniform
{
	mat4 myView;
	mat4 myProjection;
	mat4 myViewProjection;
} CameraData;


layout( push_constant ) uniform Constants
{
	vec4 myData;
	mat4 myRenderMatrix;
} PushConstants;

void main()
{
	mat4 transformMatrix = (CameraData.myViewProjection * PushConstants.myRenderMatrix);
	gl_Position = transformMatrix * vec4(vPosition, 1.0f);
	outColor = vColor;
}