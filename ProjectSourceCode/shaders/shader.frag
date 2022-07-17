#version 450

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(set = 0, binding = 0) uniform globalUniformBufferObject {
	mat4 view;
	mat4 proj;
	vec3 lightpos;
	vec3 eyePos;
	vec4 paramDecay;
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
	const float alpha = ubo.color.a;
	vec3  diffColor = texture(texSampler, fragTexCoord).rgb * ubo.color.rgb;

	/*
	if(ubo.selected > 0){
		const float whiteWeight = 0.8f;
		diffColor = (diffColor + whiteWeight * vec3(1.0f, 1.0f, 1.0f)) / (1+whiteWeight);
	}
	*/

	vec3 lightColor = vec3(0.4f, 0.4f, 0.4f);
	vec3 lightColor_Spot = vec3(0.9f, 0.9f, 0.9f);
	vec3 lightPos_Spot = vec3(0.0f, 8.0f, 0.0f);
	const vec3  specColor = vec3(1.0f, 0.8f, 0.8f);
	const float specPower = 64.0f;
	vec3 lDGeneral = vec3(0.5f, 0.5f, 0.5f); //light direction
	//vec3 lC = vec3(1.0f, 1.0f, 1.0f);

	vec3 lD = lDGeneral;

	float x = gubo.paramDecay.x / length(gubo.lightpos - fragViewDir);
	//vec3 lC = (pow(x, gubo.paramDecay.y))*lightColor;

	if(ubo.selected > 0) {
	vec3 y = lightColor_Spot*(pow(x, gubo.paramDecay.y));

	vec3 light_direction = (lightPos_Spot - fragViewDir)/(length(lightPos_Spot - fragViewDir));
	lD = light_direction;
	float z = clamp((dot(light_direction, lDGeneral) - gubo.paramDecay.w)/(gubo.paramDecay.z - gubo.paramDecay.w), 0, 1);
	lightColor = y*z;
	}	

	vec3 N = normalize(fragNorm);
	vec3 R = -reflect(lD, N);
	vec3 V = normalize(fragViewDir);
	vec3 EyeDir = normalize(gubo.eyePos.xyz - fragViewDir);
	
	// Lambert diffuse
	vec3 diffuse  = diffColor * max(dot(N,lD), 0.0f);
	// Phong specular
	vec3 specular = specColor * pow(max(dot(EyeDir, R), 0.0f), specPower);
	// Hemispheric ambient
	//vec3 ambient  = (vec3(0.1f,0.1f, 0.1f) * (1.0f + N.y) + vec3(0.4f,0.4f, 0.4f) * (1.0f - N.y)) * diffColor;

	outColor = vec4(clamp((diffuse + specular)*lightColor, vec3(0.0f), vec3(1.0f)), alpha);
	//else
	//	outColor = vec4(clamp(diffuse + specular, vec3(0.0f), vec3(1.0f)), alpha);
}