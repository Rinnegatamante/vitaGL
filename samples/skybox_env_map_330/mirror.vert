#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 modelMatrix;
uniform mat4 inverseModelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 wNormal;
out vec3 wPosition;

void main() {
	wPosition = (modelMatrix * vec4(position, 1.0)).xyz;
	wNormal = mat3(transpose(inverseModelMatrix)) * normal;
	gl_Position = projectionMatrix * viewMatrix * vec4(wPosition, 1.0);
}
