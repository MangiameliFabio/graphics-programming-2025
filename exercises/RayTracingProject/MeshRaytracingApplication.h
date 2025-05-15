#pragma once

#include <ituGL/application/Application.h>

#include <ituGL/renderer/Renderer.h>
#include <ituGL/camera/CameraController.h>
#include <ituGL/utils/DearImGui.h>

#include "glm/ext/matrix_transform.hpp"
#include "ituGL/geometry/ShaderStorageBufferObject.h"
#include "ituGL/scene/Scene.h"

class ModelLoader;

struct RaytracingMaterial {
    RaytracingMaterial(const unsigned int materialId, glm::vec4 albedo = glm::vec4(0.f), const float roughness = 0.f, const float metallic = 0.f,
        const float ior = 0.f, const glm::vec4 emissive = glm::vec4(0.f)) {
        m_materialId = materialId;
        m_albedo = albedo;
        m_roughness = roughness;
        m_metallic = metallic;
        m_ior = ior;
        m_emissive = emissive;
    }

    unsigned int m_materialId;
    float m_roughness = 0.f;
    float m_metallic = 0.f;
    float m_ior = 0.f;
    glm::vec4 m_albedo = glm::vec4(0.f);
    glm::vec4 m_emissive = glm::vec4(0.f);
};

class Material;
class Texture2DObject;
class FramebufferObject;

class MeshRaytracingApplication : public Application
{
public:
    MeshRaytracingApplication();

protected:
    void Initialize() override;
    void Update() override;
    void Render() override;
    void Cleanup() override;

private:
    void InitializeCamera();
    void InitializeMaterial();
    void InitializeFramebuffer();
    void InitializeRenderer();
    void InitializeModels();
    void InitializeSSBO();
    void InitializeTextureArray();

    std::shared_ptr<Material> CreateCopyMaterial();
    std::shared_ptr<Material> CreateRaytracingMaterial(const char* fragmentShaderPath);

    void InvalidateScene();

    void RenderGUI();
    void SendTexturesToShader(GLuint textures[20]);
    std::shared_ptr<Texture2DObject> LoadTexture(const char* path);
    void LoadModel(ModelLoader& loader, const char* path, unsigned int materialId = 0, glm::mat4 transform = glm::mat4(1.0f));

private:
    // Helper object for debug GUI
    DearImGui m_imGui;

    // Frame counter
    unsigned int m_frameCount;

    // World position for sphere center
    glm::vec3 m_sphereCenter;

    // World matrix for cube
    glm::mat4 m_boxMatrix;

    // World matrix for mesh
	glm::mat4 m_meshMatrix;

    // Camera controller
    CameraController m_cameraController;

    // Renderer
    Renderer m_renderer;

    // Materials
    std::shared_ptr<Material> m_material;

    std::shared_ptr<RaytracingMaterial> m_meshMaterial;

    // Framebuffer
    std::shared_ptr<Texture2DObject> m_sceneTexture;

    std::shared_ptr<FramebufferObject> m_sceneFramebuffer;

    // Default material
    std::shared_ptr<Material> m_defaultMaterial;

    ShaderStorageBufferObject m_ssboTriangles;
    ShaderStorageBufferObject m_ssboMaterials;
    ShaderStorageBufferObject m_ssboTransforms;

    std::vector<std::shared_ptr<Model>> m_models;
    std::vector<RaytracingMaterial> m_materials;
    std::vector<std::string> m_textureFiles;

    std::vector<glm::mat4> m_transforms;
    std::shared_ptr<ShaderProgram> m_shaderProgramPtr;

    // Global scene
    Scene m_scene;

    GLuint texture;
};
