#pragma once

#include "renderer_types.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

namespace xjar {

static void ProcessMesh(aiMesh *mesh, const aiScene *scene) {
    Mesh result;

    for (u32 i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;
        auto  &pos = mesh->mVertices[i];
        vertex.pos = glm::vec3(pos.x, pos.y, pos.z);

        auto &norm = mesh->mNormals[i];
        vertex.norm = glm::vec3(norm.x, norm.y, norm.z);
        if (mesh->mTextureCoords[0]) {
            vertex.texcoord = glm::vec2(mesh->mTextureCoords[0][i].x,
                                        mesh->mTextureCoords[0][i].y);
        } else {
            vertex.texcoord = glm::vec2(0.0f, 0.0f);
        }

        result.vertices.push_back(vertex);
    }
    
    for (u32 i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (u32 j = 0; j < face.mNumIndices; ++j) {
            result.indices.push_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex >= 0) {

    }

    return result;
}

static void ProcessEachNode(Model *model, aiNode *node, const aiScene *scene) {
    for (u32 i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        model->meshes.push_back(ProcessMesh(mesh, scene));
    }

    for (u32 i = 0; i < node->mNumChildren; i++) {
        ProcessEachNode(model, node->mChildren[i], scene);
    }
}

Model LoadModel(const std::string &path) {
    Model result;

    Assimp::Importer importer;
    const aiScene   *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        fprintf(stderr, "Assimp error %s\n", import.GetErrorString());
        exit(1);
    }

    ProcessEachNode(&model, scene->mRootNode, scene);

    return result;
}

}
