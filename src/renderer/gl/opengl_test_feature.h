#pragma once

#include <initializer_list>
#include "renderer/test_feature.h"
#include "renderer/camera.h"
#include "opengl_pipeline.h"

namespace xjar {

class OpenGL_TestFeature final : public TestFeature {
public:
    OpenGL_TestFeature() = default;
    ~OpenGL_TestFeature();
    void Init(void *device, void *renderPass) override;
    void DrawEntities(void *cmdbuf, const GPU_SceneData &sceneData, std::initializer_list<Entity *> entities) override;

private:
    void CreatePipeline();
    
    OpenGL_Shader m_defaultShader;

};

}
