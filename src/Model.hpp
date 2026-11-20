#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <utility>

#include "Mesh.hpp"
#include "OBJloader.hpp"

class Model {
public:
    Model() = delete;
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    explicit Model(const std::filesystem::path& filename)
    {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        if (!loadOBJ(filename, vertices, indices))
            throw std::runtime_error("Failed to load model: " + filename.string());
        mesh_ = std::make_unique<Mesh>(std::move(vertices), std::move(indices));
    }

    void draw() const
    {
        mesh_->draw();
    }

private:
    std::unique_ptr<Mesh> mesh_;
};
