#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in float tid;
layout (location = 3) in float mid;
layout (location = 4) in vec4 color;

layout(set = 0,binding = 0) uniform UniformBufferObject
{
	mat4 projView;
} ubo;

layout (location = 0) out DATA
{
	vec3 position;
	vec2 uv;
	float tid;
	float mid;
	vec4 color;
} vs_out;

void main()
{
	gl_Position = ubo.projView * vec4(position,1.0);
	vs_out.position = position;
	vs_out.uv = uv;
	vs_out.tid = tid;
	vs_out.mid = mid;
	vs_out.color = color;
}