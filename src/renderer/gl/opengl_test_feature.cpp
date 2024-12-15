#include "opengl_test_feature.h"
#include "opengl_mesh.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "world.h"

namespace xjar {

static const char *shaderCodeVertex = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
)";

static const char *shaderCodeFragment = R"(

#version 460 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D textureSampler;

void main()
{
	FragColor = texture(textureSampler, TexCoord);
}

)";
void OpenGL_TestFeature::Init(void *device, void *renderPass) {
    CreatePipeline();
}

OpenGL_TestFeature::~OpenGL_TestFeature() {
    glDeleteProgram(m_defaultShader.program);
}

void OpenGL_TestFeature::DrawEntities(void *cmdbuf, const GPU_SceneData &sceneData, std::initializer_list<Entity *> entities) {
    m_defaultShader.Bind();

    u32 viewLoc = glGetUniformLocation(m_defaultShader.program, "view");
    u32 projLoc = glGetUniformLocation(m_defaultShader.program, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &sceneData.viewMat[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(sceneData.projMat));

    GLint samplerLoc = glGetUniformLocation(m_defaultShader.program, "textureSampler");
    glUniform1i(samplerLoc, 0);
    
    for (Entity *ent : entities) {

        u32 modelLoc = glGetUniformLocation(m_defaultShader.program, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(ent->model.localTransform));

        OpenGL_Mesh *glmesh = (OpenGL_Mesh *)ent->model.handle;
        glmesh->Draw();

    }
        
    m_defaultShader.Unbind();

}

void OpenGL_TestFeature::CreatePipeline() {
    m_defaultShader.CreateShader(shaderCodeVertex, shaderCodeFragment);
}

}
