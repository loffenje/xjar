#include "types.h"

#include "renderer/renderer_types.h"
#include "material_descr.h"

namespace {
std::vector<xjar::Mesh>             g_meshes;
std::vector<xjar::MaterialDescr>    g_materials;
std::vector<std::string>            g_matFiles;

std::vector<u32>        g_indexData;
std::vector<f32>        g_vertexData;
u32                     g_indexOffset;
u32                     g_vertexOffset;
bool                    g_exportTexcoords = false;
bool                    g_exportNormals = false;
u32                     g_numElementsToStore = 3; // by default only vertex elements

constexpr char cmdExportTexcoords[] = "-t";
constexpr char cmdExportNormals[] = "-n";

}

inline int AddUnique(std::vector<std::string> &files, const std::string &file) {
    if (file.empty())
        return -1;

    auto i = std::find(std::begin(files), std::end(files), file);

    if (i == files.end()) {
        files.push_back(file);

        return (int)files.size() - 1;
    }

    return (int)std::distance(files.begin(), i);
}

xjar::MaterialDescr ConvertAIMaterial(const aiMaterial *mat, const char *dir) {
    xjar::MaterialDescr descr;

    aiColor4D color;
    if (aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &color) == AI_SUCCESS) {
        descr.emissiveColor = {color.r, color.g, color.b, color.a};
        if (descr.emissiveColor.w > 1.0f)
            descr.emissiveColor.w = 1.0f;
    }

    if (aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS) {
        descr.albedoColor = {color.r, color.g, color.b, color.a};
        if (descr.albedoColor.w > 1.0f)
            descr.albedoColor.w = 1.0f;
    }
    aiString         path;
    aiTextureMapping mapping;
    u32              uvIndex = 0;
    f32              blend = 1.0f;
    aiTextureOp      textureOp = aiTextureOp_Add;
    aiTextureMapMode textureMapMode[2] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
    u32              textureFlags = 0;

    if (aiGetMaterialTexture(mat, aiTextureType_DIFFUSE, 0, &path, &mapping, &uvIndex, &blend, &textureOp, textureMapMode, &textureFlags) == AI_SUCCESS) {
        std::string fullpath = dir;
        fullpath.append("/");
        fullpath.append(path.C_Str());

        descr.diffuseMap = AddUnique(g_matFiles, fullpath);
    }

    if (aiGetMaterialTexture(mat, aiTextureType_SPECULAR, 0, &path) == AI_SUCCESS) {
        std::string fullpath = dir;
        fullpath.append("/");
        fullpath.append(path.C_Str());

        descr.specularMap = AddUnique(g_matFiles, fullpath);
    }

    return descr;
}

xjar::Mesh ConvertAIMesh(const aiMesh *m) {
    const bool hasTexCoords = m->HasTextureCoords(0);

    const u32 numIndices = m->mNumFaces * 3;
    const u32 numElements = g_numElementsToStore;
    const u32 streamElementSize = static_cast<u32>(numElements * sizeof(f32));
    const u32 meshSize = static_cast<u32>(m->mNumVertices * streamElementSize + numIndices * sizeof(u32));


    for (size_t i = 0; i != m->mNumVertices; i++) {
        const aiVector3D &v = m->mVertices[i];
        const aiVector3D &n = m->mNormals[i];
        const aiVector3D &t = hasTexCoords ? m->mTextureCoords[0][i] : aiVector3D();
        g_vertexData.push_back(v.x);
        g_vertexData.push_back(v.y);
        g_vertexData.push_back(v.z);

        if (g_exportTexcoords) {
            g_vertexData.push_back(t.x);
            g_vertexData.push_back(t.y);
        }

        if (g_exportNormals) {
            g_vertexData.push_back(n.x);
            g_vertexData.push_back(n.y);
            g_vertexData.push_back(n.z);
        }

    }

    const xjar::Mesh result = {
        .lodNum = 1,
        .streamNum = 1,
        .materialID = 0,
        .meshSize = meshSize,
        .vertexCount = m->mNumVertices,
        .indexOffset = g_indexOffset,
        .vertexOffset = g_vertexOffset,
        .lodOffset = {g_indexOffset * sizeof(u32), (g_indexOffset + numIndices) * sizeof(u32)},
        .streamOffset = {g_vertexOffset * streamElementSize},
        .streamElementSize = streamElementSize};

    for (size_t i = 0; i != m->mNumFaces; i++) {
        const aiFace &face = m->mFaces[i];
        g_indexData.push_back(face.mIndices[0] + g_vertexOffset);
        g_indexData.push_back(face.mIndices[1] + g_vertexOffset);
        g_indexData.push_back(face.mIndices[2] + g_vertexOffset);
    }
    g_indexOffset += numIndices;
    g_vertexOffset += m->mNumVertices;

    return result;
}

void LoadFile(const char *filename, const char *materialDir) {
    const u32 flags = aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals 
        | aiProcess_PreTransformVertices | aiProcess_RemoveRedundantMaterials | aiProcess_FindDegenerates 
        | aiProcess_FindInvalidData | aiProcess_FindInstances | aiProcess_OptimizeMeshes;

    Assimp::Importer importer;
    const aiScene   *scene = importer.ReadFile(filename, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        fprintf(stderr, "Assimp error %s\n", importer.GetErrorString());
        exit(1);
    }

    g_meshes.reserve(scene->mNumMeshes);
    for (size_t i = 0; i != scene->mNumMeshes; i++) {
        g_meshes.push_back(ConvertAIMesh(scene->mMeshes[i]));
    }

    g_materials.reserve(scene->mNumMaterials);
    for (size_t i = 0; i != scene->mNumMaterials; i++) {
        g_materials.push_back(ConvertAIMaterial(scene->mMaterials[i], materialDir));
    }
}

void SaveStringList(FILE *f, const std::vector<std::string> &lines) {
    u32 size = (u32)lines.size();
    fwrite(&size, sizeof(u32), 1, f);

    for (const auto &s : lines) {
        size = (u32)s.length();
        fwrite(&size, sizeof(u32), 1, f);
        fwrite(s.c_str(), size + 1, 1, f);
    }
}
void MeshConvert(const char *inputFile,
    const char *outputMeshFile,
    const char *outputInstanceDataFile,
    const char *outputMaterialFile,
    const char *materialDir,
    bool exportTexcoords,
    bool exportNormals) {

    if (exportTexcoords) {
        g_numElementsToStore += 2;
        g_exportTexcoords = true;
    }
    if (exportNormals) {
        g_numElementsToStore += 3;
        g_exportNormals = true;
    }

    LoadFile(inputFile, materialDir);

    FILE *outputMesh = fopen(outputMeshFile, "wb");
    xjar::MeshHdr hdr = {
        .magicValue = 0xdeadbeef,
        .meshNum = (u32)g_meshes.size(),
        .dataStartOffset = (u32)(sizeof(xjar::MeshHdr) + g_meshes.size() * sizeof(xjar::Mesh)),
        .indexDataSize = (u32)(g_indexData.size() * sizeof(u32)),
        .vertexDataSize = (u32)(g_vertexData.size() * sizeof(f32))};

    fwrite(&hdr, 1, sizeof(hdr), outputMesh);
    fwrite(g_meshes.data(), hdr.meshNum, sizeof(xjar::Mesh), outputMesh);
    fwrite(g_indexData.data(), 1, hdr.indexDataSize, outputMesh);
    fwrite(g_vertexData.data(), 1, hdr.vertexDataSize, outputMesh);
    fclose(outputMesh);

    FILE *outputInstanceData = fopen(outputInstanceDataFile, "wb");

    std::vector<xjar::InstanceData> instanceData;

    g_vertexOffset = 0;
    for (u32 i = 0; i < g_meshes.size(); i++) {
        instanceData.push_back(xjar::InstanceData {
            .meshIndex = (u32) i,
            .materialIndex = 0,
            .LOD = 0,
            .indexOffset = g_meshes[i].indexOffset,
            .vertexOffset = g_vertexOffset,
            .transformIndex = 0
        });

        g_vertexOffset += g_meshes[i].vertexCount;
    }

    fwrite(instanceData.data(), instanceData.size(), sizeof(xjar::InstanceData), outputInstanceData);
    fclose(outputInstanceData);

    FILE *outputMaterial = fopen(outputMaterialFile, "wb");

    u32 matSize = (u32)g_materials.size();
    fwrite(&matSize, sizeof(u32), 1, outputMaterial);
    fwrite(g_materials.data(), sizeof(xjar::MaterialDescr), matSize, outputMaterial);
    SaveStringList(outputMaterial, g_matFiles);
    fclose(outputMaterial);

}
