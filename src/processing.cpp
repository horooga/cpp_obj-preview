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
#include "cpp_obj-preview/processing.h"

struct IndexLess {
    bool operator()(const tinyobj::index_t& a, const tinyobj::index_t& b) const {
        if (a.vertex_index != b.vertex_index)
            return a.vertex_index < b.vertex_index;
        if (a.normal_index != b.normal_index)
            return a.normal_index < b.normal_index;
        return a.texcoord_index < b.texcoord_index;
    }
};

Shader::Shader() {
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &VERTEX_CODE, nullptr);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, nullptr, infoLog);
        std::cerr << "Error: vertex shader compilation failed\n" << infoLog << std::endl;
    }
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &FRAGMENT_CODE, nullptr);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, nullptr, infoLog);
        std::cerr << "Error: fragment shader compilation failed\n" << infoLog << std::endl;
    }
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success); 
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}
 
void Shader::use() const {
    glUseProgram(ID);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::setMat4(const std::string &name, const glm::mat4 &mat) const {
    GLint location = glGetUniformLocation(ID, name.c_str());
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
    }
}

std::tuple<glm::vec3, glm::vec3> getBoundingBox(const tinyobj::attrib_t& attrib) {
    glm::vec3 bbox_min(FLT_MAX);
    glm::vec3 bbox_max(-FLT_MAX);

    for (size_t i = 0; i < attrib.vertices.size() / 3; i++) {
        float x = attrib.vertices[3 * i + 0];
        float y = attrib.vertices[3 * i + 1];
        float z = attrib.vertices[3 * i + 2];

        if (x < bbox_min.x) bbox_min.x = x;
        if (y < bbox_min.y) bbox_min.y = y;
        if (z < bbox_min.z) bbox_min.z = z;

        if (x > bbox_max.x) bbox_max.x = x;
        if (y > bbox_max.y) bbox_max.y = y;
        if (z > bbox_max.z) bbox_max.z = z;
    }
    return std::make_tuple(bbox_min, bbox_max);
}

void centerizeModel(tinyobj::attrib_t& attrib, glm::vec3& bbox_center) {
    for (size_t i = 0; i < attrib.vertices.size() / 3; i++) {
        attrib.vertices[3 * i + 0] -= bbox_center.x;
        attrib.vertices[3 * i + 1] -= bbox_center.y;
        attrib.vertices[3 * i + 2] -= bbox_center.z;
    }
}

void saveFrameAsPPM(int frame, const std::vector<unsigned char>& pixels, int width, int height) {
    std::ostringstream filename;
    filename << "frame_" << std::setw(3) << std::setfill('0') << frame << ".ppm";

    std::ofstream out(filename.str(), std::ios::binary);
    if (!out) {
        std::cerr << "Error: Could not open file " << filename.str() << " for writing.\n";
        return;
    }

    out << "P6\n" << width << " " << height << "\n255\n";

    for (int y = height - 1; y >= 0; --y) {
        const unsigned char* row = &pixels[y * width * 3];
        out.write(reinterpret_cast<const char*>(row), width * 3);
    }
    out.close();
}

void drawModel(const std::vector<MeshGL>& meshes, const std::vector<tinyobj::material_t>& materials, Shader& shader) {
    for (const auto& mesh : meshes) {
        glBindVertexArray(mesh.vao);
        if (mesh.materialId >= 0 && mesh.materialId < (int)materials.size()) {
            const auto& mat = materials[mesh.materialId];
            shader.setVec3("material.ambient", glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]));
            shader.setVec3("material.diffuse", glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]));
            shader.setVec3("material.specular", glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]));
            shader.setFloat("material.shininess", mat.shininess);
        } else {
            shader.setVec3("material.ambient", glm::vec3(0.5f));
            shader.setVec3("material.diffuse", glm::vec3(0.5f));
            shader.setVec3("material.specular", glm::vec3(0.5f));
            shader.setFloat("material.shininess", 0.5f);
        }
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

std::string rgbToHex(float red, float green, float blue) {
    int r = static_cast<int>(std::round(red * 255.0f));
    int g = static_cast<int>(std::round(green * 255.0f));
    int b = static_cast<int>(std::round(blue * 255.0f));

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    ss << std::setw(2) << r;
    ss << std::setw(2) << g;
    ss << std::setw(2) << b;
    return ss.str();
}

int render(std::vector<MeshGL>& meshes, tinyobj::attrib_t& attrib, const std::vector<tinyobj::material_t>& materials, GLFWwindow* window) {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    Shader shader;
    shader.use();

    auto [bbox_min, bbox_max] = getBoundingBox(attrib);
    glm::vec3 bbox_center =(bbox_max + bbox_min) / 2.0f;
    centerizeModel(attrib, bbox_center);

    shader.setVec3("light.position", glm::vec3(1.2f, 1.0f, 2.0f));
    shader.setVec3("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    shader.setVec3("light.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    shader.setVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));

    shader.setMat4("projection", glm::perspective(glm::radians(45.0f), WIDTH / (float)HEIGHT, 0.1f, 100.0f));
    shader.setVec3("viewPos", glm::vec3(0.0f, 0.0f, 3.0f));

    float fovY = glm::radians(45.0f);
    float bbox_height = (bbox_max - bbox_min).y;
    float bbox_width = (bbox_max - bbox_min).x;

    float distanceY = (bbox_height * 0.5f) / tan(fovY * 0.5f);

    float aspect = WIDTH / (float)HEIGHT;
    float fovX = 2.0f * atan(tan(fovY * 0.5f) * aspect);
    float distanceX = (bbox_width * 0.5f) / tan(fovX * 0.5f);

    float camera_distance = glm::max(distanceX, distanceY);
    shader.setMat4("view", glm::lookAt(
        glm::vec3(0.0f, 0.0f, bbox_center.z + camera_distance + (bbox_max - bbox_min).z * 0.5f),
        bbox_center,
        glm::vec3(0, 1, 0)
    ));
 
    for (int frame = 0; frame < 360; ++frame) {
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = glm::radians((float)frame);

        shader.setMat4("model", glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0,1,0)));

        drawModel(meshes, materials, shader);

        std::vector<unsigned char> pixels(WIDTH * HEIGHT * 3);
        glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        saveFrameAsPPM(frame, pixels, WIDTH, HEIGHT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}

std::vector<MeshGL> setupMeshes(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes) {
    std::vector<MeshGL> meshes;
    for (const auto& shape : shapes) {
        std::vector<float> vertexData;
        std::vector<unsigned int> indices;

        std::map<tinyobj::index_t, unsigned int, IndexLess> indexMap;
        unsigned int nextIndex = 0;

        for (const auto& idx : shape.mesh.indices) {
            if (indexMap.count(idx) == 0) {
                if (idx.vertex_index < 0 || 3 * idx.vertex_index + 2 >= attrib.vertices.size()) {
                    std::cerr << "Error: Invalid vertex index\n";
                    continue;
                }

                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];

                float nx = 0, ny = 0, nz = 0;
                if (idx.normal_index >= 0 && 3 * idx.normal_index + 2 < attrib.normals.size()) {
                    nx = attrib.normals[3 * idx.normal_index + 0];
                    ny = attrib.normals[3 * idx.normal_index + 1];
                    nz = attrib.normals[3 * idx.normal_index + 2];
                }

                float tx = 0, ty = 0;
                if (idx.texcoord_index >= 0 && 2 * idx.texcoord_index + 1 < attrib.texcoords.size()) {
                    tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                }

                vertexData.push_back(vx);
                vertexData.push_back(vy);
                vertexData.push_back(vz);
                vertexData.push_back(nx);
                vertexData.push_back(ny);
                vertexData.push_back(nz);
                vertexData.push_back(tx);
                vertexData.push_back(ty);

                indexMap[idx] = nextIndex++;
            }
            indices.push_back(indexMap[idx]);
        }

        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        meshes.push_back({vao, vbo, ebo, (int)indices.size(), shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0]});
    }
    return meshes;
}
