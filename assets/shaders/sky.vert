#version 330 core

layout (location = 0) in vec3 aPos;

// A posição do vértice do cubo também é a direção usada para ler o cubemap.
out vec3 texDir;

uniform mat4 camMatrix;

void main()
{
	texDir = aPos;

	vec4 pos = camMatrix * vec4(aPos, 1.0f);
	// Com z igual a w, a profundidade final é a máxima: o céu fica sempre atrás de tudo.
	gl_Position = pos.xyww;
}