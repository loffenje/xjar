#version 460

layout(location = 0) in vec3 outColor;
layout(location = 0) out vec4 outFrag;

// layout(binding = 5) readonly buffer MatBO    { uint   data[]; } mat_bo;

void main()
{
	outFrag = vec4(outColor, 1.0);
}

