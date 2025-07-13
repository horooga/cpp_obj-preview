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

int render(std::vector<MeshGL>& meshes, tinyobj::attrib_t& attrib, const std::vector<tinyobj::material_t>& materials, GLFWwindow* window);
std::vector<MeshGL> setupMeshes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);



