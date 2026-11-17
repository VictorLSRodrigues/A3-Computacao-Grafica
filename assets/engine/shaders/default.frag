#version 330 core

// Cor final do fragmento, em RGBA.
out vec4 FragColor;


// Dados interpolados recebidos do shader de vértices.
in vec3 color;
in vec2 texCoord;
in vec3 Normal;
in vec3 crntPos;

// Texturas e parâmetros enviados pela aplicação.
uniform sampler2D tex0;
uniform sampler2D specular0;
uniform vec4 baseColor; // cor do material, multiplica a textura
uniform int lightType;
uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 lightDir; // direção em que a luz viaja, com comprimento 1
uniform vec3 camPos;


vec4 pointLight()
{
	// Combina as componentes ambiente, difusa e especular; a luz perde força com a distância.
	// Vetor da superfície até a luz, reutilizado nos cálculos seguintes.
	vec3 lightVec = lightPos - crntPos;

	float dist = length(lightVec);
	float a = 3.0;
	float b = 0.7;
	float inten = 1.0f / (a * dist * dist + b * dist + 1.0f);

	// A iluminação ambiente mantém uma contribuição mínima.
	float ambient = 0.20f;

	// A difusa depende do ângulo entre a normal e a luz.
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(lightVec);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// A especular destaca superfícies cujo reflexo aponta para a câmera.
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	return (texture(tex0, texCoord) * baseColor * (diffuse * inten + ambient) + texture(specular0, texCoord).r * specular * inten) * lightColor;
}

vec4 direcLight()
{
	// A direção da luz é constante em toda a cena.
	float ambient = 0.20f;

	vec3 normal = normalize(Normal);
	// A luz vem do lado oposto ao da direção em que ela viaja.
	vec3 lightDirection = normalize(-lightDir);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	return (texture(tex0, texCoord) * baseColor * (diffuse + ambient) + texture(specular0, texCoord).r * specular) * lightColor;
}

vec4 spotLight()
{
	// O cone interno é totalmente iluminado; a borda entre os cones é suavizada.
	float outerCone = 0.90f;
	float innerCone = 0.95f;

	float ambient = 0.20f;

	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(lightPos - crntPos);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	// Suaviza a intensidade conforme a superfície se aproxima da borda do cone.
	// O eixo do holofote é a direção da luz.
	float angle = dot(lightDir, -lightDirection);
	float inten = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);

	return (texture(tex0, texCoord) * baseColor * (diffuse * inten + ambient) + texture(specular0, texCoord).r * specular * inten) * lightColor;
}


void main()
{
	vec4 resultado;
	if (lightType == 0)
		resultado = pointLight();
	else if (lightType == 1)
		resultado = direcLight();
	else
		resultado = spotLight();

    FragColor = vec4(resultado.rgb, 1.0);
}