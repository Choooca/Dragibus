#include "mikktspace_impl.h"

#include <spdlog/spdlog.h>

#include <render/primitives.h>
#include <render/mesh_utils.h>

#include <utils/debug_macro.h>

CalcTangents::CalcTangents() {
    iface.m_getNumFaces = GetNumFaces;
    iface.m_getNumVerticesOfFace = GetNumVerticesOfFace;

    iface.m_getNormal = GetNormal;
    iface.m_getPosition = GetPosition;
    iface.m_getTexCoord = GetTexCoords;
    iface.m_setTSpaceBasic = SetTspaceBasic;

    context.m_pInterface = &iface;
}

bool CalcTangents::Calc(Render::Primitive* primitive) {

    if (primitive->mode != Render::DRAW_MODE::TRIANGLES) {
        PRINT_INVALID("Only Triangle render mode is supported");
        return false;
    }

    Render::Primitive work_primitive;
    work_primitive.vertices = Render::MeshUtils::Unindex(*primitive);
    work_primitive.mode = primitive->mode;

    if (work_primitive.vertices.size() < 3 || work_primitive.vertices.size() % 3 != 0) {
        PRINT_INVALID("Invalid corner count {}", work_primitive.vertices.size());
        return false;
    }

    context.m_pUserData = &work_primitive;
    if (!genTangSpaceDefault(&this->context)) {
        PRINT_INVALID("genTangSpaceDefault failed");
        return false;
    }

    Render::MeshUtils::Reindex(*primitive, work_primitive.vertices);

    return true;
}

int CalcTangents::GetNumFaces(const SMikkTSpaceContext* context) {
    Render::Primitive* working_mesh = static_cast<Render::Primitive*> (context->m_pUserData);

    return static_cast<int>(working_mesh->vertices.size() / 3);
}

int CalcTangents::GetNumVerticesOfFace(const SMikkTSpaceContext* context,
    const int iFace) {
    return 3;
}

void CalcTangents::GetPosition(const SMikkTSpaceContext* context,
    float* outpos,
    const int iFace, const int iVert) {

    Render::Primitive* working_mesh = static_cast<Render::Primitive*> (context->m_pUserData);

    auto index = GetVertexIndex(context, iFace, iVert);
    auto &vertex = working_mesh->vertices[index];


    outpos[0] = vertex.position.x;
    outpos[1] = vertex.position.y;
    outpos[2] = vertex.position.z;
}

void CalcTangents::GetNormal(const SMikkTSpaceContext* context,
    float* outnormal,
    const int iFace, const int iVert) {
    Render::Primitive* working_mesh = static_cast<Render::Primitive*> (context->m_pUserData);

    auto index = GetVertexIndex(context, iFace, iVert);
    const auto &vertex = working_mesh->vertices[index];

    outnormal[0] = vertex.normal.x;
    outnormal[1] = vertex.normal.y;
    outnormal[2] = vertex.normal.z;
}

void CalcTangents::GetTexCoords(const SMikkTSpaceContext* context,
    float* outuv,
    const int iFace, const int iVert) {
    Render::Primitive* working_mesh = static_cast<Render::Primitive*> (context->m_pUserData);

    auto index = GetVertexIndex(context, iFace, iVert);
    const auto &vertex = working_mesh->vertices[index];

    outuv[0] = vertex.tex_coord0.x;
    outuv[1] = vertex.tex_coord0.y;
}

void CalcTangents::SetTspaceBasic(const SMikkTSpaceContext* context,
    const float* tangentu,
    const float f_sign, const int iFace, const int iVert) {
    Render::Primitive* working_mesh = static_cast<Render::Primitive*> (context->m_pUserData);

    auto index = GetVertexIndex(context, iFace, iVert);
    auto* vertex = &working_mesh->vertices[index];

    vertex->tangent.x = tangentu[0];
    vertex->tangent.y = tangentu[1];
    vertex->tangent.z = tangentu[2];
    vertex->tangent.w = f_sign;
}

int CalcTangents::GetVertexIndex(const SMikkTSpaceContext* context, int iFace, int iVert) {
    return iFace * 3 + iVert;
}