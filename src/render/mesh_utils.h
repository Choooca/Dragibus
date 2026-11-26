#pragma once

#include <vector>

namespace Render {
    struct Primitive;
    struct Vertex;
}

namespace Render::MeshUtils {
    std::vector<Render::Vertex> Unindex(const Render::Primitive& primitive);
    void Reindex(Primitive& primitive, const std::vector<Render::Vertex> &corners);
    void GenerateFlatNormals(Render::Primitive& primitive);
}