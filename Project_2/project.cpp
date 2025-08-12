// texxtures cames from Nasa website : https://planetpixelemporium.com/mercury.html
// sphre came from blender default sphre with a triangulate modifier aplied before export

// rocket from : https://sketchfab.com/3d-models/space-craft-f7026b90bf9b44c99c15f7afc87bcdd3

// textures from :https://ambientcg.com/

// https://sketchfab.com/3d-models/grass-6367d6fa23ca4db3baffd69eecbbfda5
// https://sketchfab.com/3d-models/chunky-knight-f1722ab650ad4d8dbe6fc4bf44e33d38
// https://sketchfab.com/3d-models/stylized-ww1-plane-c4edeb0e410f46e8a4db320879f0a1db

//
//    COMP 371 Labs Framework
//
//    Created by Nicolas Bergeron on 20/06/2019.
//

// compile steps
// export LIBGL_ALWAYS_SOFTWARE=1
// g++ -o main project.cpp -I../include -L/usr/lib -lGLEW -lGL -lglfw -lm
// ./ main
// main.exe

#include <iostream>
#include <list>
#include <algorithm>
#include "OBJloader.h"
#include "OBJloaderV2.h"
#include "graph.h"
#include <fstream> // for std::ifstream
#include <sstream> // for std::stringstream
#include <iostream>
#include <algorithm>
#include <array>

#define GLEW_STATIC 1 // This allows linking with Static Library on Windows, without DLL
#include <GL/glew.h>  // Include GLEW - OpenGL Extension Wrangler

#include <GLFW/glfw3.h> // GLFW provides a cross-platform interface for creating a graphical context,
                        // initializing OpenGL and binding inputs

#include <glm/glm.hpp>                  // GLM is an optimized math library with syntax to similar to OpenGL Shading Language
#include <glm/gtc/matrix_transform.hpp> // include this to create transformation matrices
#include <glm/common.hpp>
#include <cmath> // for std::isfinite

// Face target on XZ without extra GLM extensions
static inline glm::mat4 chunkyFaceTargetYaw(const glm::vec3 &chunkyPos,
                                            const glm::vec3 &targetPosXZ,
                                            float modelForwardOffsetDeg = 0.0f)
{
    glm::vec3 dir(targetPosXZ.x - chunkyPos.x, 0.0f, targetPosXZ.z - chunkyPos.z);

    // Normalize safely
    float len2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    if (!std::isfinite(dir.x) || !std::isfinite(dir.y) || !std::isfinite(dir.z) || len2 < 1e-8f)
        return glm::mat4(1.0f);

    dir /= std::sqrt(len2);

    float yaw = std::atan2(dir.x, dir.z);
    return glm::rotate(glm::mat4(1.0f),
                       yaw + glm::radians(modelForwardOffsetDeg),
                       glm::vec3(0, 1, 0));
}

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace glm;
using namespace std;

// Reads OBJ like your loadOBJ2 but outputs unified arrays + single index buffer
bool loadOBJ_unified(const char *path,
                     std::vector<glm::vec3> &outPos,
                     std::vector<glm::vec2> &outUV,
                     std::vector<glm::vec3> &outNrm,
                     std::vector<unsigned int> &outIdx)
{
    std::vector<glm::vec3> V;
    std::vector<glm::vec2> T;
    std::vector<glm::vec3> N;
    std::vector<int> vI, tI, nI;

    // --- parse (like your loadOBJ2) ---
    FILE *f = fopen(path, "r");
    if (!f)
        return false;
    char hdr[128];
    while (true)
    {
        int r = fscanf(f, "%127s", hdr);
        if (r == EOF)
            break;
        if (!strcmp(hdr, "v"))
        {
            glm::vec3 v;
            fscanf(f, "%f %f %f", &v.x, &v.y, &v.z);
            V.push_back(v);
        }
        else if (!strcmp(hdr, "vt"))
        {
            glm::vec2 uv;
            fscanf(f, "%f %f", &uv.x, &uv.y); /* uv.y = -uv.y; */
            T.push_back(uv);
        }
        else if (!strcmp(hdr, "vn"))
        {
            glm::vec3 n;
            fscanf(f, "%f %f %f", &n.x, &n.y, &n.z);
            N.push_back(n);
        }
        else if (!strcmp(hdr, "f"))
        {
            char line[256];
            fgets(line, sizeof(line), f);
            int vi[3] = {0}, ti[3] = {0}, ni[3] = {0};
            int m = sscanf(line, "%d/%d/%d %d/%d/%d %d/%d/%d", &vi[0], &ti[0], &ni[0], &vi[1], &ti[1], &ni[1], &vi[2], &ti[2], &ni[2]);
            if (m != 9)
            { /* handle other face formats if needed */
                continue;
            }
            for (int k = 0; k < 3; k++)
            {
                vI.push_back(vi[k] - 1);
                tI.push_back(ti[k] - 1);
                nI.push_back(ni[k] - 1);
            }
        }
        else
        {
            fgets(hdr, sizeof(hdr), f);
        } // skip rest of line
    }
    fclose(f);

    // --- unify ---
    std::unordered_map<Key, unsigned int, KeyHash> map;
    outPos.clear();
    outUV.clear();
    outNrm.clear();
    outIdx.clear();
    outPos.reserve(vI.size());
    outUV.reserve(vI.size());
    outNrm.reserve(vI.size());
    outIdx.reserve(vI.size());

    for (size_t i = 0; i < vI.size(); ++i)
    {
        Key k{vI[i], tI[i], nI[i]};
        auto it = map.find(k);
        if (it == map.end())
        {
            unsigned int id = (unsigned)outPos.size();
            map.emplace(k, id);
            outPos.push_back(V[k.v]);
            outUV.push_back((k.t >= 0 && k.t < (int)T.size()) ? T[k.t] : glm::vec2(0));
            outNrm.push_back((k.n >= 0 && k.n < (int)N.size()) ? N[k.n] : glm::vec3(0, 0, 1));
            outIdx.push_back(id);
        }
        else
        {
            outIdx.push_back(it->second);
        }
    }
    return true;
}

GLuint setupUnifiedVAO(const char *path, int &indexCount)
{
    std::vector<glm::vec3> pos, nrm;
    std::vector<glm::vec2> uv;
    std::vector<unsigned int> idx;
    if (!loadOBJ_unified(path, pos, uv, nrm, idx))
        return 0;

    GLuint vao, vboP, vboT, vboN, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vboP);
    glBindBuffer(GL_ARRAY_BUFFER, vboP);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(glm::vec3), pos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboT);
    glBindBuffer(GL_ARRAY_BUFFER, vboT);
    glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(glm::vec2), uv.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &vboN);
    glBindBuffer(GL_ARRAY_BUFFER, vboN);
    glBufferData(GL_ARRAY_BUFFER, nrm.size() * sizeof(glm::vec3), nrm.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    indexCount = (int)idx.size();
    return vao;
}

GLuint loadTexture(const char *filename);

std::string loadShaderSource(const std::string &filepath)
{
    std::ifstream file(filepath);
    std::stringstream buffer;
    if (!file)
    {
        std::cerr << "Failed to load shader: " << filepath << std::endl;
        return "";
    }
    buffer << file.rdbuf();
    return buffer.str();
}

const char *getVertexShaderSource();

const char *getFragmentShaderSource();

int compileAndLinkShaders(const char *vertexShaderSource, const char *fragmentShaderSource);

void setUVScale(int shaderProgram, glm::vec2 scale)
{
    glUseProgram(shaderProgram);
    GLuint uvScaleLocation = glGetUniformLocation(shaderProgram, "uvScale");
    if (uvScaleLocation != -1)
        glUniform2fv(uvScaleLocation, 1, &scale[0]);
}

struct TexturedColoredVertex
{
    TexturedColoredVertex(vec3 _position, vec3 _color, vec2 _uv)
        : position(_position), color(_color), uv(_uv) {}

    vec3 position;
    vec3 color;
    vec2 uv;
};

GLuint setupModelVBO(string path, int &vertexCount)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> UVs;

    // read the vertex data from the model's OBJ file
    loadOBJ(path.c_str(), vertices, normals, UVs);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO); // Becomes active VAO
    // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).

    // Vertex VBO setup
    GLuint vertices_VBO;
    glGenBuffers(1, &vertices_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Normals VBO setup
    // UVs → location 2
    GLuint uvs_VBO;
    glGenBuffers(1, &uvs_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvs_VBO);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(glm::vec2), &UVs.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    // Normals → location 3
    GLuint normals_VBO;
    glGenBuffers(1, &normals_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, normals_VBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(3);

    glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs, as we are using multiple VAOs)
    vertexCount = vertices.size();
    return VAO;
}
// Sets up a model using an Element Buffer Object to refer to vertex data
/// changed the order in which the info align to match the shaders
GLuint setupModelEBO(string path, int &vertexCount)
{
    vector<int> vertexIndices; // The contiguous sets of three indices of vertices, normals and UVs, used to make a triangle
    vector<glm::vec3> vertices;
    vector<glm::vec3> normals;
    vector<glm::vec2> UVs;

    // read the vertices from the cube.obj file
    // We won't be needing the normals or UVs for this program
    loadOBJ2(path.c_str(), vertexIndices, vertices, normals, UVs);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO); // Becomes active VAO
    // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).

    /*
    // Vertex VBO setup
    GLuint vertices_VBO;
    glGenBuffers(1, &vertices_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Normals VBO setup
    GLuint normals_VBO;
    glGenBuffers(1, &normals_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, normals_VBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(1);

    // UVs VBO setup
    GLuint uvs_VBO;
    glGenBuffers(1, &uvs_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvs_VBO);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(glm::vec2), &UVs.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(2);
    */

    // Vertex VBO setup (location = 0)
    GLuint vertices_VBO;
    glGenBuffers(1, &vertices_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Normals VBO setup (location = 3)
    GLuint normals_VBO;
    glGenBuffers(1, &normals_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, normals_VBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
    glEnableVertexAttribArray(3);

    // UVs VBO setup (location = 2)
    GLuint uvs_VBO;
    glGenBuffers(1, &uvs_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvs_VBO);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(glm::vec2), &UVs.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
    glEnableVertexAttribArray(2);

    // EBO setup
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(int), &vertexIndices.front(), GL_STATIC_DRAW);

    glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO
    vertexCount = vertexIndices.size();
    return VAO;
}

void setProjectionMatrix(int shaderProgram, mat4 projectionMatrix)
{
    glUseProgram(shaderProgram);
    GLuint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
}

void setViewMatrix(int shaderProgram, mat4 viewMatrix)
{
    glUseProgram(shaderProgram);
    GLuint viewMatrixLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
}

void setWorldMatrix(int shaderProgram, mat4 worldMatrix)
{
    glUseProgram(shaderProgram);
    GLuint worldMatrixLocation = glGetUniformLocation(shaderProgram, "worldMatrix");
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &worldMatrix[0][0]);
}

struct SceneAssets
{
    // Textures
    GLuint grass, brick, cement, rockTex, space, brick2, chunky_knight_tex, tankTex, treeTex, cliffTex;
    GLuint earth, jupiter, mars, mercury, neptune, pluto, saturn, sun, uranus, venus, moon, rocketTexture, saturnRing;
    GLuint planetTextures[11];
    GLuint textureLocation;

    // VAOs and vertex counts
    GLuint cubeVAO,
        planetVAO, rocketVAO, saturnRingVao, chunky_knightVao, tankVAO, treeVAO;

    // Mesh paths
    std::string cubePath, planetPath, rocketPath, saturnRingPath, chunky_knightPath, tankPath, treePath;

    int cubeVertices;
    int planet1Vertices;
    int rocketVertices;
    int saturnRingVertices;
    int chunky_knightVertices;
    int tankVerticies;
    int treeVerticies;

    int colorShaderProgram;
    int texturedShaderProgram;
};

SceneAssets loadSceneAssets()
{
    SceneAssets assets;

    // Textures
    assets.grass = loadTexture("./Textures/Grass001_1K-JPG_Color.jpg");
    assets.brick = loadTexture("./Textures/Bricks060_1K-JPG_Color.jpg");
    assets.cement = loadTexture("./Textures/cement.jpg");
    assets.rockTex = loadTexture("./Textures/Rock058_1K-JPG_Color.jpg");
    assets.space = loadTexture("./Textures/skybox_to_equirect_2.png");
    assets.brick2 = loadTexture("./Textures/brick.jpg");
    assets.chunky_knight_tex = loadTexture("./Textures/chunky_knight_tex.png");
    assets.tankTex = loadTexture("./Textures/Tank.png");
    assets.treeTex = loadTexture("./Textures/tree.png");
    assets.cliffTex = loadTexture("./Textures/cliff_side_diff_1k.jpg");

    assets.sun = loadTexture("./Textures/sunmap.jpg");
    assets.earth = loadTexture("./Textures/earthmap1k.jpg");
    assets.jupiter = loadTexture("./Textures/jupitermap.jpg");
    assets.mars = loadTexture("./Textures/mars_1k_color.jpg");
    assets.mercury = loadTexture("./Textures/mercurymap.jpg");
    assets.neptune = loadTexture("./Textures/neptunemap.jpg");
    assets.pluto = loadTexture("./Textures/plutomap1k.jpg");
    assets.saturn = loadTexture("./Textures/saturnmap.jpg");
    assets.uranus = loadTexture("./Textures/uranusmap.jpg");
    assets.venus = loadTexture("./Textures/venusmap.jpg");
    assets.saturnRing = loadTexture("./Textures/saturnringcolor.png");
    assets.moon = loadTexture("./Textures/moonmap1k.png");
    assets.rocketTexture = loadTexture("./Textures/Plane.png");

    assets.planetTextures[0] = assets.sun;
    assets.planetTextures[1] = assets.earth;
    assets.planetTextures[2] = assets.jupiter;
    assets.planetTextures[3] = assets.mars;
    assets.planetTextures[4] = assets.mercury;
    assets.planetTextures[5] = assets.neptune;
    assets.planetTextures[6] = assets.pluto;
    assets.planetTextures[7] = assets.saturn;
    assets.planetTextures[8] = assets.uranus;
    assets.planetTextures[9] = assets.venus;
    assets.planetTextures[10] = assets.moon;

    // Mesh paths
    assets.cubePath = "./Models/cube.obj";
    assets.planetPath = "./Models/Planet.obj";
    assets.rocketPath = "./Models/Plane.obj";
    assets.saturnRingPath = "./Models/saturn_ring.obj";
    assets.chunky_knightPath = "./Models/chunky_knight.obj";
    assets.tankPath = "./Models/tank.obj";
    assets.treePath = "./Models/tree.obj";

    // Mesh loading
    {
        assets.cubeVAO = setupUnifiedVAO(assets.cubePath.c_str(), assets.cubeVertices);
        assets.planetVAO = setupUnifiedVAO(assets.planetPath.c_str(), assets.planet1Vertices);

        assets.rocketVAO = setupModelEBO(assets.rocketPath.c_str(), assets.rocketVertices);

        assets.saturnRingVao = setupUnifiedVAO(assets.saturnRingPath.c_str(), assets.saturnRingVertices);

        assets.chunky_knightVao = setupModelEBO(assets.chunky_knightPath.c_str(), assets.chunky_knightVertices);
        assets.tankVAO = setupModelEBO(assets.tankPath.c_str(), assets.tankVerticies);

        assets.treeVAO = setupUnifiedVAO(assets.treePath.c_str(), assets.treeVerticies);
    }

    // Compile and link shaders here ...
    assets.colorShaderProgram = compileAndLinkShaders(getVertexShaderSource(), getFragmentShaderSource());
    assets.texturedShaderProgram = compileAndLinkShaders(getVertexShaderSource(), getFragmentShaderSource());
    assets.textureLocation = glGetUniformLocation(assets.texturedShaderProgram, "textureSampler");

    return assets;
}

void drawTank(const SceneAssets &assets)
{
    setUVScale(assets.texturedShaderProgram, vec2(1.0f));

    vec3 position = vec3(20.0f, 0.8f, 0.0f);

    mat4 model = mat4(1.0f);
    model = translate(model, position);

    float scaleFactor = 1.0f;
    model = scale(model, vec3(1.0f) * scaleFactor);

    model = translate(model, position);

    model *= rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    model *= rotate(mat4(1.0f), radians(0.0f), vec3(0.0f, 1.0f, 0.0f));
    model *= rotate(mat4(1.0f), radians(0.0f), vec3(1.0f, 0.0f, 0.0f));

    setWorldMatrix(assets.texturedShaderProgram, model);

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    GLuint normalLoc = glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalLoc, 1, GL_FALSE, &normalMatrix[0][0]);

    glBindTexture(GL_TEXTURE_2D, assets.tankTex);

    glBindVertexArray(assets.tankVAO);
    glDrawElements(GL_TRIANGLES, assets.tankVerticies, GL_UNSIGNED_INT, 0);
}

class Projectile
{
public:
    Projectile(vec3 position, vec3 velocity, int shaderProgram) : mPosition(position), mVelocity(velocity)
    {
        mWorldMatrixLocation = glGetUniformLocation(shaderProgram, "worldMatrix");
    }

    void Update(float dt)
    {
        mPosition += mVelocity * dt;
    }

    void Draw(const SceneAssets &assets)
    {
        // Match UV scale
        setUVScale(assets.texturedShaderProgram, vec2(1));

        // Bind texture
        glBindTexture(GL_TEXTURE_2D, assets.rockTex);

        // World transform
        mat4 worldMatrix = translate(mat4(1.0f), mPosition) * rotate(mat4(1.0f), radians(180.0f), vec3(0.0f, 1.0f, 0.0f)) * scale(mat4(1.0f), vec3(0.2f));
        glUniformMatrix4fv(mWorldMatrixLocation, 1, GL_FALSE, &worldMatrix[0][0]);

        // Normal matrix
        mat3 normalMatrix = transpose(inverse(mat3(worldMatrix)));
        GLuint normalLoc = glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix");
        glUniformMatrix3fv(normalLoc, 1, GL_FALSE, &normalMatrix[0][0]);

        // Bind VAO and draw
        glBindVertexArray(assets.cubeVAO);
        glDrawElements(GL_TRIANGLES, assets.cubeVertices, GL_UNSIGNED_INT, 0);
    }

    // in Projectile class (public:)
    const glm::vec3 &position() const { return mPosition; }

private:
    GLuint mWorldMatrixLocation;
    vec3 mPosition;
    vec3 mVelocity;
};

void drawRocket(const SceneAssets &assets, std::list<Projectile> &projectileList)
{
    setUVScale(assets.texturedShaderProgram, vec2(1.0f));

    float time = glfwGetTime();
    float orbitSpeed = 15.0f;
    float orbitRadius = 60.0f;
    vec3 orbitCenter = vec3(0.0f, 15.0f, 0.0f);

    float angleNow = radians(time * orbitSpeed);
    float angleAhead = radians((time + 1.0f) * orbitSpeed);

    vec3 posNow = orbitCenter + vec3(cos(angleNow), 0.0f, sin(angleNow)) * orbitRadius;
    vec3 posAhead = orbitCenter + vec3(cos(angleAhead), 0.0f, sin(angleAhead)) * orbitRadius;

    vec3 forward = normalize(posAhead - posNow);
    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(up, forward));
    vec3 correctedUp = cross(forward, right); // rocket's local +Y
    vec3 downDir = -normalize(correctedUp);   // launch direction (DOWN)

    mat4 rotationMatrix = mat4(
        vec4(right, 0.0f),
        vec4(correctedUp, 0.0f),
        vec4(forward, 0.0f),
        vec4(0.0f, 0.0f, 0.0f, 1.0f));

    mat4 model = mat4(1.0f);
    model = translate(model, posNow);
    model *= rotationMatrix;

    // Pre-rotate rocket from Y-up to Z-forward (if your mesh needs it)
    model *= rotate(mat4(1.0f), radians(-90.0f), vec3(1, 0, 0));
    model *= scale(mat4(1.0f), vec3(1));

    setWorldMatrix(assets.texturedShaderProgram, model);

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    glUniformMatrix3fv(glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix"),
                       1, GL_FALSE, &normalMatrix[0][0]);

    glBindTexture(GL_TEXTURE_2D, assets.rocketTexture);
    glBindVertexArray(assets.rocketVAO);
    glDrawElements(GL_TRIANGLES, assets.rocketVertices, GL_UNSIGNED_INT, 0);

    // ------------ Fire projectiles along DOWN vector ------------
    static double lastShot = 0.0;
    const double fireInterval = 1;       // seconds between shots
    const float projectileSpeed = 10.0f; // units/sec

    double now = glfwGetTime();
    if (now - lastShot >= fireInterval)
    {
        projectileList.emplace_back(
            posNow,                    // spawn at rocket position
            downDir * projectileSpeed, // velocity along down
            assets.texturedShaderProgram);
        lastShot = now;
    }
}

void drawPlanets(SceneAssets assets)
{
    setUVScale(assets.texturedShaderProgram, vec2(1));

    for (int i = 0; i < 11; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, assets.planetTextures[i]);

        float time = glfwGetTime() + i * 3;
        float angle = radians(time * 10.0f + i * 36.0f);
        float spin = radians(time * 50.0f + i * 20.0f);

        float orbitRadius = (i == 0) ? 0 : 50.0f + i * 3.0f;
        vec3 orbitCenter = vec3(0.0f, 16.0f, -10.0f);

        mat4 model = mat4(1.0f);
        model = translate(model, orbitCenter);
        model = rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));
        model = translate(model, vec3(orbitRadius, 0.0f, 0.0f));
        model = rotate(model, spin, vec3(0.0f, 1.0f, 0.0f));
        model = scale(model, vec3(6.0f));

        setWorldMatrix(assets.texturedShaderProgram, model);
        mat3 normalMatrix = transpose(inverse(mat3(model)));
        GLuint normalLoc = glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix");
        glUniformMatrix3fv(normalLoc, 1, GL_FALSE, &normalMatrix[0][0]);

        glBindVertexArray(assets.planetVAO);
        glDrawElements(GL_TRIANGLES, assets.planet1Vertices, GL_UNSIGNED_INT, 0);

        if (i == 1)
        {
            float moonOrbitRadius = 2.0f;
            float moonOrbitSpeed = 25.0f;
            float moonSpinSpeed = 60.0f;
            float moonScale = 0.1f;
            float moonOrbitAngle = radians(time * moonOrbitSpeed);

            mat4 moonModel = model *
                             rotate(mat4(1.0f), moonOrbitAngle, vec3(0.0f, 1.0f, 0.0f)) *
                             translate(mat4(1.0f), vec3(moonOrbitRadius, 0.0f, 0.0f)) *
                             rotate(mat4(1.0f), radians(time * moonSpinSpeed), vec3(0.0f, 1.0f, 0.0f)) *
                             scale(mat4(1.0f), vec3(moonScale));

            setWorldMatrix(assets.texturedShaderProgram, moonModel);
            mat3 moonNormalMatrix = transpose(inverse(mat3(moonModel)));
            GLuint moonNormalLoc = glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix");
            glUniformMatrix3fv(moonNormalLoc, 1, GL_FALSE, &moonNormalMatrix[0][0]);

            glBindTexture(GL_TEXTURE_2D, assets.moon);
            glBindVertexArray(assets.planetVAO);
            glDrawElements(GL_TRIANGLES, assets.planet1Vertices, GL_UNSIGNED_INT, 0);
        }

        if (i == 7) // saturn rings
        {
            mat4 ringModel = model *
                             rotate(mat4(1.0f), radians(90.0f), vec3(1.0f, 0.0f, 0.0f)) *
                             scale(mat4(1.0f), vec3(1)); // Optional scale tweak

            setWorldMatrix(assets.texturedShaderProgram, ringModel);
            mat3 ringNormalMatrix = transpose(inverse(mat3(ringModel)));
            GLuint ringNormalLoc = glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix");
            glUniformMatrix3fv(ringNormalLoc, 1, GL_FALSE, &ringNormalMatrix[0][0]);

            glBindTexture(GL_TEXTURE_2D, assets.saturnRing);
            glBindVertexArray(assets.saturnRingVao);
            glDrawElements(GL_TRIANGLES, assets.saturnRingVertices, GL_UNSIGNED_INT, 0);
        }
    }
}

void drawCubeField(const SceneAssets &assets)
{
    setUVScale(assets.texturedShaderProgram, vec2(1));

    auto drawCubes = [&](GLuint texture, float radius, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            glBindTexture(GL_TEXTURE_2D, texture);

            float time = glfwGetTime();
            float angle = radians(time * 5.0f + i * 18.0f);
            float spin = radians(time * 10.0f);
            vec3 orbitCenter = vec3(0.0f, 0.0f, -10.0f);

            mat4 model = mat4(1.0f);
            model = translate(model, orbitCenter);
            model = rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));
            model = translate(model, vec3(radius, 0.0f, 0.0f));
            model = rotate(model, spin, vec3(0.0f, 1.0f, 0.0f));
            model = scale(model, vec3(0.5f));

            setWorldMatrix(assets.texturedShaderProgram, model);
            mat3 normalMatrix = transpose(inverse(mat3(model)));
            GLuint normalLoc = glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix");
            glUniformMatrix3fv(normalLoc, 1, GL_FALSE, &normalMatrix[0][0]);

            glBindVertexArray(assets.cubeVAO);
            glDrawElements(GL_TRIANGLES, assets.cubeVertices, GL_UNSIGNED_INT, 0);
        }
    };

    drawCubes(assets.brick, 30.0f, 40);
    drawCubes(assets.cement, 20.0f, 20);
    drawCubes(assets.rockTex, 10.0f, 20);
}

void HealthBar(const SceneAssets &assets,
               const glm::vec3 &pos,
               float health, // range: 0–100
               glm::vec3 &targetPos,
               float modelForwardOffsetDeg = 0.0f)
{
    glUseProgram(assets.texturedShaderProgram);

    // Map health to color (red -> green)
    float t = glm::clamp(health / 100.0f, 0.0f, 1.0f);
    glm::vec3 color = glm::mix(glm::vec3(1.0f, 0.0f, 0.0f), // Red
                               glm::vec3(0.0f, 1.0f, 0.0f), // Green
                               t);

    // Solid color mode
    glUniform1i(glGetUniformLocation(assets.texturedShaderProgram, "useTexture"), GL_FALSE);
    glUniform3fv(glGetUniformLocation(assets.texturedShaderProgram, "solidColor"), 1, &color[0]);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Orientation toward target
    glm::mat4 R_yaw = chunkyFaceTargetYaw(pos, targetPos, modelForwardOffsetDeg);

    // Model matrix: translate, rotate toward target, then scale width by health
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0, 10, 0)) *
                      R_yaw *
                      glm::scale(glm::mat4(1.0f), glm::vec3(1.0f * t, 0.1f, 0.1f));
    setWorldMatrix(assets.texturedShaderProgram, model);

    // Normal matrix
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix3fv(glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix"),
                       1, GL_FALSE, &normalMatrix[0][0]);

    glBindVertexArray(assets.cubeVAO);
    glDrawElements(GL_TRIANGLES, assets.cubeVertices, GL_UNSIGNED_INT, 0);

    // Restore texture mode
    glUniform1i(glGetUniformLocation(assets.texturedShaderProgram, "useTexture"), GL_TRUE);
}

struct CameraState
{
    // Position and orientation
    vec3 position; // Camera position
    vec3 lookAt;   // Camera forward direction
    vec3 up = vec3(0.0f, 1.0f, 0.0f);

    // Angles for rotation
    float horizontalAngle = 0.0f;
    float verticalAngle = 0.0f;

    // Movement speed
    float fastSpeed = 40.0f;
    float normalSpeed = 15.0f;

    // Camera mode
    bool firstPerson = true;
    bool goUp = false;
    bool goDown = false;

    // Mouse tracking
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    int lastMouseLeftState = GLFW_RELEASE;

    // Timing
    float dt = 0.0f;
    float lastFrameTime = 0.0f;

    // Visual effect (e.g. rotating cube)
    void updateDeltaTime(float currentTime)
    {
        dt = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
    }
};
void cameraHealthBar(const SceneAssets &assets,
                     const CameraState &camera,     // camera.position, camera.lookAt, camera.up
                     float health,                  // 0–100
                     const glm::vec3 &screenOffset, // X=right, Y=up, Z=forward (camera space)
                     float distance = 3.0f)         // how far in front of camera
{
    glUseProgram(assets.texturedShaderProgram);

    // Map health to color (red -> green)
    const float t = glm::clamp(health / 100.0f, 0.0f, 1.0f);
    const glm::vec3 color = glm::mix(glm::vec3(1.0f, 0.0f, 0.0f), // Red
                                     glm::vec3(0.0f, 1.0f, 0.0f), // Green
                                     t);

    // Solid color mode
    glUniform1i(glGetUniformLocation(assets.texturedShaderProgram, "useTexture"), GL_FALSE);
    glUniform3fv(glGetUniformLocation(assets.texturedShaderProgram, "solidColor"), 1, &color[0]);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Camera-aligned basis
    const glm::vec3 fwd = glm::normalize(camera.lookAt);
    const glm::vec3 right = glm::normalize(glm::cross(fwd, camera.up));
    const glm::vec3 up = glm::normalize(glm::cross(right, fwd));

    // Anchor: in front of camera
    glm::vec3 anchor = camera.position + fwd * distance;

    // Extra translation in camera space for screen positioning
    anchor += right * screenOffset.x; // move left/right
    anchor += up * screenOffset.y;    // move up/down
    anchor += fwd * screenOffset.z;   // move closer/farther

    // Rotation from camera basis
    glm::mat4 R(1.0f);
    R[0] = glm::vec4(right, 0.0f);
    R[1] = glm::vec4(up, 0.0f);
    R[2] = glm::vec4(-fwd, 0.0f);

    // Model: translate to anchor, face camera, scale width by health
    glm::mat4 model = glm::translate(glm::mat4(1.0f), anchor) *
                      R *
                      glm::scale(glm::mat4(1.0f), glm::vec3(t / 5.0f, 0.01f, 0.001f));
    setWorldMatrix(assets.texturedShaderProgram, model);

    // Normal matrix
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    glUniformMatrix3fv(glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix"),
                       1, GL_FALSE, &normalMatrix[0][0]);

    glBindVertexArray(assets.cubeVAO);
    glDrawElements(GL_TRIANGLES, assets.cubeVertices, GL_UNSIGNED_INT, 0);

    // Restore texture mode
    glUniform1i(glGetUniformLocation(assets.texturedShaderProgram, "useTexture"), GL_TRUE);
}

void drawGraph(const SceneAssets &assets)
{
    // Build graph and fetch cube positions
    graph G(10, 10);
    const std::vector<std::array<int, 3>> Loc = G.getLocation();

    setUVScale(assets.texturedShaderProgram, vec2(1));

    const float tile = 20.0f; // GRID->WORLD scale; tweak as needed
    const float cubeScale = 0.5f;

    GLuint program = assets.texturedShaderProgram;
    GLint normalLoc = glGetUniformLocation(program, "normalMatrix");

    glBindVertexArray(assets.cubeVAO);

    for (const auto &loc : Loc)
    {
        // Choose texture by type (loc[2]); fallback to brick
        GLuint tex =
            (loc[2] == 1) ? assets.cement : (loc[2] == 2) ? assets.rockTex
                                                          : assets.brick;

        glBindTexture(GL_TEXTURE_2D, tex);

        vec3 pos = vec3(loc[0] * tile, loc[1] * tile, loc[2] * tile);
        std::cout << pos.r << " " << pos.g << " " << pos.b << std::endl;

        mat4 model(1.0f);
        model = translate(model, pos);
        model = scale(model, vec3(cubeScale));

        setWorldMatrix(program, model);

        mat3 normalMatrix = transpose(inverse(mat3(model)));
        glUniformMatrix3fv(normalLoc, 1, GL_FALSE, &normalMatrix[0][0]);

        glDrawElements(GL_TRIANGLES, assets.cubeVertices, GL_UNSIGNED_INT, 0);
    }
}

void drawGraphTree(const SceneAssets &assets)
{
    // Build graph and fetch positions
    graph G(8, 8);
    const std::vector<std::array<int, 3>> Loc = G.getLocation();

    setUVScale(assets.texturedShaderProgram, vec2(1.0f));

    const float tile = 80.0f; // grid -> world
    const float treeScale = 1.0f;

    GLuint program = assets.texturedShaderProgram;
    GLint normalLoc = glGetUniformLocation(program, "normalMatrix");

    glBindVertexArray(assets.treeVAO);
    glBindTexture(GL_TEXTURE_2D, assets.treeTex);

    for (const auto &loc : Loc)
    {
        vec3 pos = vec3(loc[0] * tile, loc[1] * tile, loc[2] * tile) + vec3(-300, 0, -300);

        mat4 model(1.0f);
        model = translate(model, pos);
        model = scale(model, vec3(treeScale));
        model *= rotate(mat4(1.0f), radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
        model *= rotate(mat4(1.0f), radians(0.0f), vec3(0.0f, 1.0f, 0.0f));
        model *= rotate(mat4(1.0f), radians(0.0f), vec3(1.0f, 0.0f, 0.0f));
        setWorldMatrix(program, model);

        mat3 normalMatrix = transpose(inverse(mat3(model)));
        glUniformMatrix3fv(normalLoc, 1, GL_FALSE, &normalMatrix[0][0]);

        glDrawElements(GL_TRIANGLES, assets.treeVerticies, GL_UNSIGNED_INT, 0);
    }
}
struct EnemyHealth
{
    int health = 150;
    vec3 knightPosition = vec3(20.0f, 1.2f, 10.0f);
    int hitRadius = 20;
};

void GraphEnemy(vector<EnemyHealth> &eH)
{
    // Build graph and fetch positions
    graph G(8, 8);
    const std::vector<std::array<int, 3>> Loc = G.getLocation();
    const float tile = 100.0f; // grid -> world

    for (const auto &loc : Loc)
    {
        vec3 pos = vec3(loc[0] * tile, loc[1] * tile, loc[2] * tile) + vec3(-100, 0, -100);

        // cout << pos[0] << " | " << pos[1] << " | " << pos[2] << " | " << endl;

        eH.push_back(EnemyHealth());
        eH.back().knightPosition = pos;
    }
}

void drawSkybox(const SceneAssets &assets)
{
    glDisable(GL_CULL_FACE); // Show inside of the sphere
    glDepthMask(GL_FALSE);   // Prevent depth buffer overwrite

    glUseProgram(assets.texturedShaderProgram);
    setUVScale(assets.texturedShaderProgram, vec2(1.0f, 1.0f));

    glBindTexture(GL_TEXTURE_2D, assets.space);

    mat4 skyModel = translate(mat4(1.0f), vec3(0.0f, 16.0f, -10.0f)) *
                    scale(mat4(1.0f), vec3(-800.0f));

    setWorldMatrix(assets.texturedShaderProgram, skyModel);
    glBindVertexArray(assets.planetVAO);
    glDrawElements(GL_TRIANGLES, assets.planet1Vertices, GL_UNSIGNED_INT, 0);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

void drawGround(const SceneAssets &assets)
{
    glUseProgram(assets.texturedShaderProgram);
    setUVScale(assets.texturedShaderProgram, vec2(300.0f, 300.0f));
    glBindTexture(GL_TEXTURE_2D, assets.cliffTex);

    mat4 model = mat4(1.0f);
    model = translate(model, vec3(0.0f, -1.5f, 0.0f));
    model = scale(model, vec3(2000.0f, 0.02f, 2000.0f));

    setWorldMatrix(assets.texturedShaderProgram, model);

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    GLuint normalLoc = glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalLoc, 1, GL_FALSE, &normalMatrix[0][0]);

    glBindVertexArray(assets.cubeVAO);
    glDrawElements(GL_TRIANGLES, assets.cubeVertices, GL_UNSIGNED_INT, 0);
}

void handleInput(GLFWwindow *window, SceneAssets &assets, CameraState &camera,
                 std::list<Projectile> &projectileList)
{
    // 1) DT FIRST (once)
    double now = glfwGetTime();
    camera.dt = now - camera.lastFrameTime;
    camera.lastFrameTime = now;

    // 2) Keys/modes
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        camera.firstPerson = true;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        camera.firstPerson = false;

    camera.goUp = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    camera.goDown = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;

    bool fast = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    float speed = fast ? camera.fastSpeed : camera.normalSpeed;

    // 3) Mouse look
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    double dx = mouseX - camera.lastMouseX;
    double dy = mouseY - camera.lastMouseY;
    camera.lastMouseX = mouseX;
    camera.lastMouseY = mouseY;

    camera.horizontalAngle -= float(dx) * 60.0f * camera.dt;
    camera.verticalAngle -= float(dy) * 60.0f * camera.dt;
    camera.verticalAngle = std::clamp(camera.verticalAngle, -85.0f, 85.0f);

    float theta = radians(camera.horizontalAngle);
    float phi = radians(camera.verticalAngle);
    camera.lookAt = vec3(cosf(phi) * cosf(theta), sinf(phi), -cosf(phi) * sinf(theta));
    vec3 side = normalize(cross(camera.lookAt, camera.up));

    // 4) Move
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.position += camera.lookAt * camera.dt * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.position -= camera.lookAt * camera.dt * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.position += side * camera.dt * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.position -= side * camera.dt * speed;
    if (camera.goUp)
        camera.position.y += camera.dt * speed;
    if (camera.goDown)
        camera.position.y -= camera.dt * speed;

    // 5) View
    mat4 viewMatrix;
    if (camera.firstPerson)
    {
        viewMatrix = lookAt(camera.position, camera.position + camera.lookAt, camera.up);
    }
    else
    {
        float r = 5.0f;
        vec3 offset = vec3(r * cosf(phi) * cosf(theta), r * sinf(phi), -r * cosf(phi) * sinf(theta));
        viewMatrix = lookAt(camera.position - offset, camera.position, camera.up);
    }
    setViewMatrix(assets.colorShaderProgram, viewMatrix);
    setViewMatrix(assets.texturedShaderProgram, viewMatrix);

    // 6) Mouse PRESS edge -> spawn
    const int prev = camera.lastMouseLeftState;
    const int curr = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (prev == GLFW_RELEASE && curr == GLFW_PRESS)
    {
        const float projectileSpeed = 25.0f;
        projectileList.push_back(
            Projectile(camera.position, projectileSpeed * camera.lookAt,
                       assets.texturedShaderProgram));
        std::cout << "shoot1 !" << std::endl;
    }
    camera.lastMouseLeftState = curr;
}

struct PlayerHealth
{
    int health = 100;
};

void drawChunkyKnight(const SceneAssets &assets, CameraState &camera, PlayerHealth &pH, EnemyHealth &chunkyHP)
{
    if (chunkyHP.health <= 0)
    {
        return;
    }

    setUVScale(assets.texturedShaderProgram, vec2(1.0f));

    // HUD + debug cube
    // drawChunkyHealthBarHUD(chunkyHP);
    // drawCubeSingleColored(assets, chunkyHP.knightPosition, vec3(1.0f, 0.0f, 0.0f)); // normalized color
    // drawCubeSingleColoredOriented(assets, chunkyHP.knightPosition, vec3(1.0f, 0.0f, 0.0f), camera.position, 0);

    HealthBar(assets, chunkyHP.knightPosition, chunkyHP.health, camera.position, 0);
    // Early out if dead
    if (chunkyHP.health <= 0)
        return;

    // Damage when close
    const float hitRadius = 20.0f, dps = 15.0f;
    static float acc = 0.0f;
    float distToPlayer = length(camera.position - chunkyHP.knightPosition);
    if (distToPlayer < hitRadius)
    {
        acc += dps * camera.dt;
        int dmg = (int)acc;
        if (dmg > 0)
        {
            pH.health = std::max(0, pH.health - dmg);
            acc -= dmg;
        }
    }

    // Chase (unchanged)
    const vec3 target = vec3(camera.position.x, chunkyHP.knightPosition.y, camera.position.z);
    vec3 step = target - chunkyHP.knightPosition;
    float d = length(step);
    const float moveThreshold = 0.1f, moveSpeed = 10.0f;
    if (d > moveThreshold)
    {
        step /= d;
        chunkyHP.knightPosition += step * (moveSpeed * camera.dt);
    }

    // Orientation via helper
    const mat4 R_fix =
        rotate(mat4(1.0f), radians(-90.0f), vec3(1, 0, 0)) *
        rotate(mat4(1.0f), radians(0.0f), vec3(0, 1, 0)) *
        rotate(mat4(1.0f), radians(0.0f), vec3(0, 0, 1));

    const mat4 R_yaw = chunkyFaceTargetYaw(chunkyHP.knightPosition, target, 0.0f);

    mat4 model = translate(mat4(1.0f), chunkyHP.knightPosition) * R_yaw * R_fix * scale(mat4(1.0f), vec3(4.0f));
    model = scale(model, vec3(0.5f));

    setWorldMatrix(assets.texturedShaderProgram, model);

    const mat3 normalMatrix = transpose(inverse(mat3(model)));
    const GLuint normalLoc = glGetUniformLocation(assets.texturedShaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalLoc, 1, GL_FALSE, &normalMatrix[0][0]);

    glBindTexture(GL_TEXTURE_2D, assets.chunky_knight_tex);
    glBindVertexArray(assets.chunky_knightVao);
    glDrawElements(GL_TRIANGLES, assets.chunky_knightVertices, GL_UNSIGNED_INT, 0);
}

void updateLights(GLuint shader, const vec3 &lightPos1, const vec3 &lightPos2, const vec3 &viewPos)
{
    if (shader == 0)
    {
        std::cerr << "[updateLights] Warning: shader ID is 0. Skipping light update.\n";
        return;
    }
    glUseProgram(shader);
    glUniform3fv(glGetUniformLocation(shader, "lightPos1"), 1, &lightPos1[0]);
    glUniform3fv(glGetUniformLocation(shader, "lightPos2"), 1, &lightPos2[0]);
    glUniform3fv(glGetUniformLocation(shader, "viewPos"), 1, &viewPos[0]);
}

void drawProjectile(std::list<Projectile> &projectiles,
                    const CameraState &camera,
                    EnemyHealth &chunkyHP, const SceneAssets &assets)
{
    for (auto it = projectiles.begin(); it != projectiles.end();)
    {
        it->Update(camera.dt);

        if (chunkyHP.health > 0)
        {
            // Collision: distance from projectile to knight
            if (glm::length(it->position() - chunkyHP.knightPosition) <= chunkyHP.hitRadius)
            {
                chunkyHP.health = std::max(0, chunkyHP.health - 40);
                it = projectiles.erase(it); // remove projectile on hit
                continue;                   // don't Draw() a removed projectile
            }
        }

        it->Draw(assets);
        ++it;
    }
}

int main(int argc, char *argv[])
{

    // Initialize GLFW and OpenGL version
    glfwInit();
#if defined(PLATFORM_OSX)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    // Container for projectiles to be implemented in tutorial
    list<Projectile> projectileList;

    EnemyHealth eh;

    // Create Window and rendering context using GLFW, resolution is 800x600
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Comp371 - project 02", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to create GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Load Textures
    SceneAssets assets = loadSceneAssets();
    PlayerHealth playerH = PlayerHealth();

    // Black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClearColor(0.6f, 0.6f, 0.6f, 1.0f); // soft blue-gray

    // Spinning cube at camera position
    float spinningCubeAngle = 0.0f;

    // Set projection matrix for shader, this won't change
    mat4 projectionMatrix = glm::perspective(70.0f,           // field of view in degrees
                                             800.0f / 600.0f, // aspect ratio
                                             0.01f, 1000.0f); // near and far (near > 0)

    // setup camera input
    CameraState camera;

    // Set initial view matrix
    mat4 viewMatrix = lookAt(camera.position,                 // eye
                             camera.position + camera.lookAt, // center
                             camera.up);                      // up

    // Set View and Projection matrices on both shaders
    setViewMatrix(assets.colorShaderProgram, viewMatrix);
    setViewMatrix(assets.texturedShaderProgram, viewMatrix);

    setProjectionMatrix(assets.colorShaderProgram, projectionMatrix);
    setProjectionMatrix(assets.texturedShaderProgram, projectionMatrix);

    // Other OpenGL states to set once
    // Enable Backface culling
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // set shader uniforms that don’t change every frame once before the main render loop begins.
    glUseProgram(assets.texturedShaderProgram);
    glUniform1i(glGetUniformLocation(assets.texturedShaderProgram, "textureSampler"), 0);

    // For frame time
    glfwGetCursorPos(window, &camera.lastMouseX, &camera.lastMouseY);

    camera.position = vec3(0, 10, 0);

    vector<EnemyHealth> enemyVecotor;
    GraphEnemy(enemyVecotor);

    // Entering Main Loop
    while (!glfwWindowShouldClose(window))
    {
        // Frame time calculation
        camera.updateDeltaTime(glfwGetTime());

        // Each frame, reset color of each pixel to glClearColor
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // === LIGHTING ===
        float time = glfwGetTime();

        // Light 1 orbits the camera
        vec3 lightPos1 = camera.position + vec3(
                                               10.0f * cos(time), // bigger horizontal radius
                                               5.0f,              // higher vertical offset
                                               10.0f * sin(time)  // bigger horizontal radius
                                           );

        // Light 2 orbits the center of the scene at height 15
        vec3 lightPos2 = vec3(
            10.0f * cos(time * 0.5f),
            15.0f,
            10.0f * sin(time * 0.5f));

        updateLights(assets.texturedShaderProgram, lightPos1, lightPos2, camera.position);

        vec3 viewPos = camera.position; // assuming you track camera pos

        // Draw ground
        drawGround(assets);

        // draw grass
        drawGraphTree(assets);

        // draw graph
        // drawGraph(assets);

        // draw the rocket
        drawRocket(assets, projectileList);

        // draw the planets
        // drawPlanets(assets);

        // draw single cube
        // drawCubeSingle(assets, vec3(0, 0, 0));

        // draw the field of cubes
        // drawCubeField(assets);

        // draw tank
        drawTank(assets);

        handleInput(window, assets, camera, projectileList);

        glUseProgram(assets.texturedShaderProgram);

        // draw chunky
        for (EnemyHealth &enemy : enemyVecotor)
        {
            drawChunkyKnight(assets, camera, playerH, enemy);
            drawProjectile(projectileList, camera, enemy, assets);
        }

        enemyVecotor.erase(
            std::remove_if(enemyVecotor.begin(), enemyVecotor.end(),
                           [](const EnemyHealth &enemy)
                           {
                               return enemy.health < 0;
                           }),
            enemyVecotor.end());

        // draw health bar
        // drawHealthBar(assets, camera, playerH);

        cameraHealthBar(assets, camera, playerH.health, vec3(-1, -1, 0.1f));

        // draw sky
        drawSkybox(assets);

        if (playerH.health <= 0)
        {
            /* code */
            cerr << "YOU LOST";
            return 0;
        }

        // End Frame
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

const char *getVertexShaderSource()
{
    return strdup(loadShaderSource("Shaders/vertexShaderSource.glsl").c_str());
}

const char *getFragmentShaderSource()
{

    return strdup(loadShaderSource("Shaders/fragmentShaderSource.glsl").c_str());
}

int compileAndLinkShaders(const char *vertexShaderSource, const char *fragmentShaderSource)
{
    // compile and link shader program
    // return shader program id
    // ------------------------------------

    // vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // link shaders
    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint loadTexture(const char *filename)
{
    // Step1 Load Textures with dimension data
    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cerr << "Error::Texture could not load texture file:" << filename << std::endl;
        return 0;
    }

    // Step2 Create and bind textures
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    assert(textureId != 0);

    glBindTexture(GL_TEXTURE_2D, textureId);

    // Step3 Set filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Step4 Upload the texture to the PU
    GLenum format = 0;
    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height,
                 0, format, GL_UNSIGNED_BYTE, data);

    // Step5 Free resources
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureId;
}
