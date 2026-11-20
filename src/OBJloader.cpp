#include "OBJloader.hpp"

#include <array>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <glm/glm.hpp>

namespace {
struct FaceVertex {
    unsigned int position{0};
    unsigned int texCoord{0};
    unsigned int normal{0};
};

bool parse_face_vertex(const std::string& value, FaceVertex& result)
{
    std::istringstream stream(value);
    std::string token;

    if (!std::getline(stream, token, '/') || token.empty())
        return false;
    try {
        result.position = static_cast<unsigned int>(std::stoul(token));
    } catch (const std::exception&) {
        return false;
    }
    if (result.position == 0)
        return false;

    if (std::getline(stream, token, '/') && !token.empty()) {
        try {
            result.texCoord = static_cast<unsigned int>(std::stoul(token));
        } catch (const std::exception&) {
            return false;
        }
    }

    if (std::getline(stream, token, '/') && !token.empty()) {
        try {
            result.normal = static_cast<unsigned int>(std::stoul(token));
        } catch (const std::exception&) {
            return false;
        }
    }

    return true;
}

bool valid_index(unsigned int index, std::size_t size)
{
    return index > 0 && index <= size;
}
}

bool loadOBJ(const std::filesystem::path& filename,
             std::vector<Vertex>& vertices,
             std::vector<GLuint>& indices)
{
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Unable to open OBJ file: " << filename << '\n';
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    vertices.clear();
    indices.clear();

    std::string line;
    unsigned int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        std::istringstream stream(line);
        std::string header;
        stream >> header;
        if (header.empty() || header[0] == '#')
            continue;

        if (header == "v") {
            glm::vec3 position{};
            if (!(stream >> position.x >> position.y >> position.z)) {
                std::cerr << "Invalid vertex at line " << lineNumber << '\n';
                return false;
            }
            positions.push_back(position);
        } else if (header == "vt") {
            glm::vec2 texCoord{};
            if (!(stream >> texCoord.x >> texCoord.y)) {
                std::cerr << "Invalid texture coordinate at line " << lineNumber << '\n';
                return false;
            }
            texCoords.push_back(texCoord);
        } else if (header == "vn") {
            glm::vec3 normal{};
            if (!(stream >> normal.x >> normal.y >> normal.z)) {
                std::cerr << "Invalid normal at line " << lineNumber << '\n';
                return false;
            }
            normals.push_back(normal);
        } else if (header == "f") {
            std::vector<FaceVertex> face;
            std::string value;
            while (stream >> value) {
                FaceVertex faceVertex;
                if (!parse_face_vertex(value, faceVertex))
                    face.clear();
                else
                    face.push_back(faceVertex);
            }
            if (face.size() < 3) {
                std::cerr << "Invalid face at line " << lineNumber << '\n';
                return false;
            }

            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const std::array<FaceVertex, 3> triangle{face[0], face[i], face[i + 1]};
                if (!valid_index(triangle[0].position, positions.size())
                    || !valid_index(triangle[1].position, positions.size())
                    || !valid_index(triangle[2].position, positions.size())) {
                    std::cerr << "Invalid position index at line " << lineNumber << '\n';
                    return false;
                }

                const glm::vec3& point0 = positions[triangle[0].position - 1];
                const glm::vec3& point1 = positions[triangle[1].position - 1];
                const glm::vec3& point2 = positions[triangle[2].position - 1];
                const glm::vec3 crossProduct = glm::cross(point1 - point0, point2 - point0);
                const glm::vec3 fallbackNormal = glm::length(crossProduct) > 0.0f
                    ? glm::normalize(crossProduct)
                    : glm::vec3(0.0f, 0.0f, 1.0f);

                for (const FaceVertex& faceVertex : triangle) {
                    const glm::vec2 texCoord = valid_index(faceVertex.texCoord, texCoords.size())
                        ? texCoords[faceVertex.texCoord - 1]
                        : glm::vec2(0.0f);
                    const glm::vec3 normal = valid_index(faceVertex.normal, normals.size())
                        ? normals[faceVertex.normal - 1]
                        : fallbackNormal;
                    Vertex vertex{positions[faceVertex.position - 1], normal, texCoord};

                    auto existing = std::find(vertices.begin(), vertices.end(), vertex);
                    if (existing == vertices.end()) {
                        vertices.push_back(vertex);
                        indices.push_back(static_cast<GLuint>(vertices.size() - 1));
                    } else {
                        indices.push_back(static_cast<GLuint>(
                            std::distance(vertices.begin(), existing)));
                    }
                }
            }
        }
    }

    if (vertices.empty() || indices.empty()) {
        std::cerr << "OBJ file contains no drawable triangles: " << filename << '\n';
        return false;
    }

    std::cout << "Loaded OBJ " << filename << " (" << vertices.size()
              << " vertices, " << indices.size() << " indices)\n";
    return true;
}
