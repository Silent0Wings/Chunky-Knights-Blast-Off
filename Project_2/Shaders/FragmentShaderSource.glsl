#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 vertexUV;

uniform sampler2D textureSampler;
uniform vec3 lightPos1;
uniform vec3 lightPos2;
uniform vec3 viewPos;

uniform bool useTexture;
uniform vec3 solidColor;

out vec4 FragColor;

void main() {
    vec3 norm = normalize(Normal);
    vec3 baseColor = useTexture 
        ? texture(textureSampler, vertexUV).rgb 
        : solidColor;

    vec3 lightDir1 = normalize(lightPos1 - FragPos);
    vec3 lightDir2 = normalize(lightPos2 - FragPos);

    float dist1 = length(lightPos1 - FragPos);
    float dist2 = length(lightPos2 - FragPos);
    float atten1 = 100.0 / (dist1 * dist1);
    float atten2 = 100.0 / (dist2 * dist2);

    float diff1 = max(dot(norm, lightDir1), 0.0) * atten1;
    float diff2 = max(dot(norm, lightDir2), 0.0) * atten2;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir1 = reflect(-lightDir1, norm);
    vec3 reflectDir2 = reflect(-lightDir2, norm);
    float spec1 = pow(max(dot(viewDir, reflectDir1), 0.0), 32.0) * atten1;
    float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), 32.0) * atten2;

    vec3 ambient = 0.6 * baseColor;
    vec3 diffuse = baseColor * (diff1 + diff2);
    vec3 specular = vec3(1.0) * (spec1 + spec2);

    vec3 lighting = ambient + diffuse + specular;
    FragColor = vec4(lighting, 1.0);
}
