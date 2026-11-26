#pragma once

#include <vector>

#include <mikktspace.h>

namespace Render {
    struct Primitive;
    struct Vertex;
}

class CalcTangents {
public:
    CalcTangents();
    bool Calc(Render::Primitive* primitive);

private:

    SMikkTSpaceInterface iface{};
    SMikkTSpaceContext context{};

    static int GetVertexIndex(const SMikkTSpaceContext* context, int iFace, int iVert);

    static int GetNumFaces(const SMikkTSpaceContext* context);
    static int GetNumVerticesOfFace(const SMikkTSpaceContext* context, int iFace);
    static void GetPosition(const SMikkTSpaceContext* context, float outpos[],
        int iFace, int iVert);

    static void GetNormal(const SMikkTSpaceContext* context, float outnormal[],
        int iFace, int iVert);

    static void GetTexCoords(const SMikkTSpaceContext* context, float outuv[],
        int iFace, int iVert);

    static void SetTspaceBasic(const SMikkTSpaceContext* context,
        const float tangentu[],
        float fSign, int iFace, int iVert);
};