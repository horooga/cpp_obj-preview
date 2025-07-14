#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>

struct MeshGL {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    int indexCount;
    int materialId;
};

class Shader {
public:
    unsigned int ID;
    Shader();
    void use() const;
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
};

const int WIDTH = 800;
const int HEIGHT = 600;
const int FRAMES = 360;
constexpr const char* VERTEX_CODE = R"(
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";
constexpr const char* FRAGMENT_CODE = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    vec3 ambient = light.ambient * material.ambient;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);
    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}
)";

std::string rgbToHex(float red, float green, float blue);
int render(std::vector<MeshGL>& meshes, tinyobj::attrib_t& attrib, const std::vector<tinyobj::material_t>& materials, GLFWwindow* window);
std::vector<MeshGL> setupMeshes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);
