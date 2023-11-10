#version 400 core
out vec4 FragColor;
in vec2 texCoord;
in vec3 normal;
in vec3 crntPos;

uniform sampler2D tex0;
uniform sampler2DArray shadowMap;

uniform vec4 lightColor;
uniform vec3 lightDir;
uniform float ambient;
uniform float specularStrength;
uniform vec3 camPos;
uniform float shininess;
uniform mat4 view;

uniform mat4 lightProjection[4];
uniform float cascade0range;
uniform float cascade1range;
uniform float cascade2range;
uniform float cascade3range;

void main(){
	float depthValue = abs((view*vec4(crntPos,1.0f)).z);
	int layer = -1;
	if(depthValue<=cascade0range){
		layer=0;
	}
	else if(depthValue<=cascade1range){
		layer=1;
	}
	else if(depthValue<=cascade2range){
		layer=2;
	}
	else if(depthValue<=cascade3range){
		layer=3;
	}

	vec4 fragPosLight=vec4(crntPos,1.0f);
	if(layer!=-1){
		fragPosLight=lightProjection[layer]*fragPosLight;
	}

	vec3 lightDirection = normalize(lightDir);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	float specular=0;
	if(diffuse!=0){
		vec3 viewDirection = normalize(camPos - crntPos);
		vec3 reflectionDirection = reflect(-lightDirection, normal);
		float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), shininess);
		specular = specAmount * specularStrength;
	}

	float shadow = 0.0f;
	if(layer!=-1){
		vec3 lightCoords = fragPosLight.xyz / fragPosLight.w;
		if(lightCoords.z <= 1.0f)
		{
			lightCoords = (lightCoords + 1.0f) / 2.0f;
			float currentDepth = lightCoords.z;

			float bias = max(0.05 * (1.0f - dot(normal, lightDirection)), 0.005);
			const float biasModifier = 0.5f;
			if(layer==0){
				bias *= 1 / (cascade0range * biasModifier);
			}
			else if(layer==1){
				bias *= 1 / (cascade1range * biasModifier);
			}
			else if(layer==2){
				bias *= 1 / (cascade2range * biasModifier);
			}
			else if(layer==3){
				bias *= 1 / (cascade3range * biasModifier);
			}

			int sampleRadius = 5;
			vec2 pixelSize = 1.0 / vec2(textureSize(shadowMap, 0));
			for(int y = -sampleRadius; y <= sampleRadius; y++)
			{
				for(int x = -sampleRadius; x <= sampleRadius; x++)
				{
					float closestDepth = texture(shadowMap, vec3(lightCoords.xy + vec2(x, y) * pixelSize,layer)).r;
					if (currentDepth > closestDepth + bias)
						shadow += 1.0f;     
				}    
			}
			shadow /= pow((sampleRadius * 2 + 1), 2);
		}
	}

	diffuse = diffuse * (1.0f - shadow);
	specular = specular * (1.0f - shadow);
	FragColor = texture(tex0, texCoord) * lightColor * (diffuse + ambient + specular);

	FragColor.a = 1.0f;
}