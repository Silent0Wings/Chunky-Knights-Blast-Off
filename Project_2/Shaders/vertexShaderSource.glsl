#version 330 core
           layout (location = 0) in vec3 aPos;
           layout (location = 1) in vec3 aColor;
           layout (location = 2) in vec2 aUV;
           layout (location = 3) in vec3 aNormal;

           uniform mat4 worldMatrix;
           uniform mat4 viewMatrix;
           uniform mat4 projectionMatrix;
           uniform vec2 uvScale;
           uniform mat3 normalMatrix;

           out vec3 FragPos;
           out vec3 Normal;
           out vec3 vertexColor;
           out vec2 vertexUV;

           void main() {
               mat4 MVP = projectionMatrix * viewMatrix * worldMatrix;
               gl_Position = MVP * vec4(aPos, 1.0);
               FragPos = vec3(worldMatrix * vec4(aPos, 1.0));
               Normal = normalize(normalMatrix * aNormal);
               vertexColor = aColor;
               vertexUV = aUV * uvScale;
           }