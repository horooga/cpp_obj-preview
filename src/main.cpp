#include <tiny_obj_loader.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <unordered_map>
#include "cpp_obj-preview/processing.h"

std::vector<std::string> readObjComments(const std::string& filename) {
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

std::unordered_map<std::string, std::string> readConfig(std::string filename) {
    std::unordered_map<std::string, std::string> config; 

    std::ifstream file(filename);
    if (!file) {
        printf("Failed to open config file %s\n", filename.c_str());
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            config[key] = value;
        }
    }
    return config;
}

void viewCmd(const std::string& open_cmd, std::string save_dir) {
    std::string command = open_cmd + " " + save_dir + "obj-preview.md";
    int ret = std::system(command.c_str());
    if (ret != 0) {
        std::cerr << "Error: Failed to open " << save_dir << "obj-preview.md in the config viewer\n";
    }
}

void clean(std::string save_dir) {
    std::remove((save_dir + "obj-preview.md").c_str());
    std::remove((save_dir + "obj-overview.gif").c_str());
}

int runGifGenCmd(std::string save_dir, std::string overwrite_flag) {
    std::string ffmpegCmd = "ffmpeg -framerate 20 -i frame_%03d.ppm -filter_complex "
                             "\"palettegen=stats_mode=full[p];[0][p]paletteuse=dither=sierra2_4a\" "
                             "-fps_mode passthrough " + overwrite_flag + save_dir + "obj-overview.gif";

    int ret = std::system(ffmpegCmd.c_str());
    if (ret != 0) {
        std::cerr << "Error: Failed to run: " << ffmpegCmd << std::endl;
    }

    std::string rmCmd = "rm frame_*.ppm";

    ret = std::system(rmCmd.c_str());
    if (ret != 0) {
        std::cerr << "Error: Failed to run: rm frame_*.ppm" << std::endl;
    }

    return 0;
}

std::string extendHome(std::string path) {
    if (!path.empty() && path[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            path = std::string(home) + path.substr(1);
        }
    }
    return path;
}

int generateOverview(tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes, std::vector<tinyobj::material_t>& materials, std::string filename, std::string save_dir) {
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

    return 0;
}

int generateReport(tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes, std::vector<tinyobj::material_t>& materials, std::string filename, std::string save_dir) { 
    std::ofstream report((save_dir + "obj-preview.md").c_str());
    if (!report.is_open()) {
        std::cerr << "Error: Failed to create report file obj-preview.md\n";
        return 1;
    }
    report << "# OBJ file preview\n\n";
    report << "File: `" << filename << "`\n\n";

    report << "## File Comments\n\n";
    std::vector<std::string> comments = readObjComments(filename);
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

    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: exe [file.obj | mode]\n\nmodes:\n\n    - clean - cleans saved preview.md and overview.gif";
        return 1;
    } 

    const std::unordered_map<std::string, std::string> config = readConfig(extendHome("~/.config/cpp_obj-preview.conf"));
    std::string save_dir = "./";
    if (config.find("save-dir") != config.end()) {
        save_dir = extendHome(config.at("save-dir"));
        if (!save_dir.empty() && save_dir.back() != '/' && save_dir.back() != '\\') {
            save_dir += '/';
        }
    }

    if (std::string(argv[1]) == "clean") {
        std::remove((save_dir + "obj-preview.md").c_str());
        std::remove((save_dir + "obj-overview.gif").c_str());
        return 0;
    }

    std::string filename = extendHome(argv[1]); 

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    int ret;

    ret = generateOverview(attrib, shapes, materials, filename, save_dir);
    if (ret != 0) {
        return ret;
    }
    
    std::string overwrite_flag = "";
    if (config.find("overwrite-flag") != config.end()) {
        config.at("overwrite-flag") == "true" || config.at("overwrite-flag") == "1" ? overwrite_flag = "-y " : overwrite_flag = "";
    }

    ret = runGifGenCmd(save_dir, overwrite_flag);
    if (ret != 0) {
        return ret;
    }

    ret = generateReport(attrib, shapes, materials, filename, save_dir);
    if (ret != 0) {
        return ret;
    }

    if (config.find("view-cmd") != config.end()) {
        viewCmd(config.at("view-cmd"), save_dir);
    }

    return 0;
}

