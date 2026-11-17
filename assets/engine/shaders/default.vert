#version 330 core

// Posição do vértice no modelo.
layout (location = 0) in vec3 aPos;
// Cor associada ao vértice.
layout (location = 1) in vec3 aColor;
// Coordenadas para amostrar as texturas.
layout (location = 2) in vec2 aTex;
// Normal usada nos cálculos de iluminação.
layout (location = 3) in vec3 aNormal;


// Atributos enviados ao shader de fragmentos.
out vec3 color;
out vec2 texCoord;
out vec3 Normal;
out vec3 crntPos;

// Matrizes enviadas pela aplicação.
uniform mat4 camMatrix;
uniform mat4 model;


void main()
{
	// Transforma a posição do vértice para o espaço do mundo.
	crntPos = vec3(model * vec4(aPos, 1.0f));
	// Projeta o vértice na tela.
	gl_Position = camMatrix * vec4(crntPos, 1.0);

	color = aColor;
	texCoord = aTex;
	// Corrige a normal para que acompanhe a transformação do modelo.
	Normal = mat3(transpose(inverse(model))) * aNormal;
}