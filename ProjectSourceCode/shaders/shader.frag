#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(set = 0, binding = 0) uniform globalUniformBufferObject {
	mat4 view;
	mat4 proj;
	vec3 lightpos;
	vec2 paramDecay;
} gubo;

layout(set = 1, binding = 0) uniform UniformBufferObject {
	mat4 model;
	vec4 color;
	float selected;
} ubo;


layout(location = 0) in vec3 fragViewDir;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	vec3  diffColor = texture(texSampler, fragTexCoord).rgb * ubo.color.rgb;
	if(ubo.selected > 0){
		const float whiteWeight = 0.8f;
		diffColor = (diffColor + whiteWeight * vec3(1.0f, 1.0f, 1.0f)) / (1+whiteWeight);
	}

	vec3 lightColor = vec3(1.0f, 0.9f, 0.9f);
	const vec3  specColor = vec3(1.0f, 0.8f, 0.8f);
	const float specPower = 100.0f;
	vec3 lD = vec3(-0.4830f, 0.8365f, -0.2588f); //light direction
	vec3 lC = vec3(1.0f, 1.0f, 1.0f);

	if(ubo.selected > 0) {
	//Point Light Direction
	lD = (gubo.lightpos - fragViewDir) / length(gubo.lightpos - fragViewDir);

	//Point Light color
	float x = gubo.paramDecay.x / length(gubo.lightpos - fragViewDir);
	lC = (pow(x, gubo.paramDecay.y))*lightColor;
	
	}

	vec3 N = normalize(fragNorm);
	vec3 R = -reflect(lD, N);
	vec3 V = normalize(fragViewDir);
	
	// Lambert diffuse
	vec3 diffuse  = diffColor * max(dot(N,lD), 0.0f);
	// Phong specular
	vec3 specular = specColor * pow(max(dot(R,V), 0.0f), specPower);
	// Hemispheric ambient
	//vec3 ambient  = (vec3(0.1f,0.1f, 0.1f) * (1.0f + N.y) + vec3(0.0f,0.0f, 0.1f) * (1.0f - N.y)) * diffColor;
	vec3 ambient  = (vec3(0.1f,0.1f, 0.1f) * (1.0f + N.y) + vec3(0.4f,0.4f, 0.4f) * (1.0f - N.y)) * diffColor;


	if(ubo.selected > 0)
		outColor = vec4(clamp((ambient + diffuse + specular)*lC, vec3(0.0f), vec3(1.0f)), 1.0f);
	else
		outColor = vec4(clamp(ambient + diffuse + specular, vec3(0.0f), vec3(1.0f)), 1.0f);
}