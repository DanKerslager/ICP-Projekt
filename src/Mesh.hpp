#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include "assets.hpp"

class Mesh {
public:
    Mesh() = delete;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices,
         GLenum primitiveType = GL_TRIANGLES)
        : vertices_(std::move(vertices)),
          indices_(std::move(indices)),
          primitiveType_(primitiveType)
    {
        glCreateVertexArrays(1, &vao_);
        glCreateBuffers(1, &vbo_);
        glNamedBufferData(vbo_,
                          static_cast<GLsizeiptr>(vertices_.size() * sizeof(Vertex)),
                          vertices_.data(), GL_STATIC_DRAW);

        glEnableVertexArrayAttrib(vao_, 0);
        glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(Vertex, position));
        glVertexArrayAttribBinding(vao_, 0, 0);

        glEnableVertexArrayAttrib(vao_, 1);
        glVertexArrayAttribFormat(vao_, 1, 3, GL_FLOAT, GL_FALSE,
                                  offsetof(Vertex, normal));
        glVertexArrayAttribBinding(vao_, 1, 0);

        glEnableVertexArrayAttrib(vao_, 2);
        glVertexArrayAttribFormat(vao_, 2, 2, GL_FLOAT, GL_FALSE,
                                  offsetof(Vertex, texCoords));
        glVertexArrayAttribBinding(vao_, 2, 0);

        glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, sizeof(Vertex));

        glCreateBuffers(1, &ebo_);
        glNamedBufferData(ebo_,
                          static_cast<GLsizeiptr>(indices_.size() * sizeof(GLuint)),
                          indices_.data(), GL_STATIC_DRAW);
        glVertexArrayElementBuffer(vao_, ebo_);
    }

    Mesh(Mesh&& other) noexcept
        : vertices_(std::move(other.vertices_)),
          indices_(std::move(other.indices_)),
          primitiveType_(other.primitiveType_),
          vao_(std::exchange(other.vao_, 0)),
          vbo_(std::exchange(other.vbo_, 0)),
          ebo_(std::exchange(other.ebo_, 0))
    {
    }

    Mesh& operator=(Mesh&& other) noexcept
    {
        if (this != &other) {
            release();
            vertices_ = std::move(other.vertices_);
            indices_ = std::move(other.indices_);
            primitiveType_ = other.primitiveType_;
            vao_ = std::exchange(other.vao_, 0);
            vbo_ = std::exchange(other.vbo_, 0);
            ebo_ = std::exchange(other.ebo_, 0);
        }
        return *this;
    }

    ~Mesh()
    {
        release();
    }

    void draw() const
    {
        glBindVertexArray(vao_);
        glDrawElements(primitiveType_, static_cast<GLsizei>(indices_.size()),
                       GL_UNSIGNED_INT, nullptr);
    }

private:
    void release()
    {
        glDeleteBuffers(1, &ebo_);
        glDeleteBuffers(1, &vbo_);
        glDeleteVertexArrays(1, &vao_);
    }

    std::vector<Vertex> vertices_;
    std::vector<GLuint> indices_;
    GLenum primitiveType_{GL_TRIANGLES};
    GLuint vao_{0};
    GLuint vbo_{0};
    GLuint ebo_{0};
};
