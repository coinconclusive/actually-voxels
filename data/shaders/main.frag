#version 460 core

out vec4 oColor;

in vec3 sPosition;
in vec3 sNormal;
in vec2 sTexCoord;

void main() {
	oColor = vec4(sTexCoord * 0.5 + 0.5, 1.0, 1.0);
}
