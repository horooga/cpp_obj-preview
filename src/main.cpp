#include <tiny_obj_loader.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Cpp_obj-preview/processing.h"

std::vector<std::string> ReadObjComments(const std::string& filename) {
    std::vector<std::string> comments;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file for reading comments: " << filename << std::endl;
        return comments;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] == '#') {
            comments.push_back(line.substr(1));
        }
    }
    return comments;
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


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: exe [file.obj]\n";
        return 1;
    }

    const std::string filename = argv[1];

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
    if (!warn.empty()) std::cout << "Error: " << warn << std::endl;
    if (!err.empty()) std::cerr << "Error: " << err << std::endl;
    if (!ret) {
        std::cerr << "Error: Failed to load/parse OBJ file: " << filename << std::endl;
        return 1;
    }

    if (!glfwInit()) {
        std::cerr << "Error: Failed to init GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "360 Rotation Capture", nullptr, nullptr);
    if (!window) {
        std::cerr << "Error: Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Error: Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::vector<MeshGL> meshes = setupMeshes(attrib, shapes);
 
    int render_ret = render(meshes, attrib, materials, window);
    if (render_ret != 0) {
        std::cerr << "Error: Failed to render the OBJ file: " << filename << std::endl;
        return 1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    std::ofstream report("obj-preview.md");
    if (!report.is_open()) {
        std::cerr << "Error: Failed to create report file obj-preview.md\n";
        return 1;
    }

    report << "# OBJ file preview\n\n";
    report << "File: `" << filename << "`\n\n";

    report << "## File Comments\n\n";
    std::vector<std::string> comments = ReadObjComments(filename);
    if (comments.empty()) {
        report << "_No comments found in the OBJ file._\n";
    } else {
        for (const auto& c : comments) {
            report << "- " << c << "\n";
        }
    }
    report << "\n";

    report << "## Summary\n";
    report << "- Number of vertices: " << (attrib.vertices.size() / 3) << "\n";
    report << "- Number of normals: " << (attrib.normals.size() / 3) << "\n";
    report << "- Number of texture coordinates: " << (attrib.texcoords.size() / 2) << "\n";
    report << "- Number of shapes: " << shapes.size() << "\n";
    report << "- Number of materials: " << materials.size() << "\n\n";

    if (!materials.empty()) {
        report << "## Materials\n\n";
        for (size_t i = 0; i < materials.size(); i++) {
            std::string ambient_hex = rgbToHex(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
            std::string diffuse_hex = rgbToHex(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
            std::string specular_hex = rgbToHex(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
            std::stringstream ambient_string;
            ambient_string << "- Ambient color: ![" << ambient_hex << "](https://placehold.co/15x15/" << ambient_hex << "/" << ambient_hex << ".png)\n";
            std::stringstream diffuse_string;
            diffuse_string << "- Diffuse color: ![" << diffuse_hex << "](https://placehold.co/15x15/" << diffuse_hex << "/" << diffuse_hex << ".png)\n";
            std::stringstream specular_string;
            specular_string << "- Specular color: ![" << specular_hex << "](https://placehold.co/15x15/" << specular_hex << "/" << specular_hex << ".png)\n";

            report << "Material `" << materials[i].name << "`\n\n";
            report << ambient_string.str();
            report << diffuse_string.str();
            report << specular_string.str();
            report << "- Specular exponent: " << materials[i].shininess << "\n\n";
        }
        report << "\n";
    }

    report << "## Shapes\n";
    for (size_t i = 0; i < shapes.size(); i++) {
        size_t face_count = shapes[i].mesh.num_face_vertices.size();
        report << "- Shape `" << shapes[i].name << "` has " << face_count << " face(s)\n";
    }
    report << "\n";

    report << "## Overview\n";
    report << "![Overview gif](obj-overview.gif)\n";

    report.close();

    std::cout << "OBJ report generated: obj-overview.md\n";

    return 0;
}

