#include "MeshRaytracingApplication.h"

#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/camera/Camera.h>
#include <ituGL/scene/SceneCamera.h>
#include <ituGL/scene/Transform.h>
#include <ituGL/lighting/DirectionalLight.h>
#include <ituGL/shader/Material.h>
#include <ituGL/texture/Texture2DObject.h>
#include <ituGL/texture/FramebufferObject.h>
#include <ituGL/renderer/PostFXRenderPass.h>
#include <ituGL/scene/RendererSceneVisitor.h>
#include <imgui.h>
#include <iostream>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "stb_image.h"
#include "ituGL/asset/ModelLoader.h"
#include "ituGL/geometry/ShaderStorageBufferObject.h"
#include "ituGL/scene/SceneModel.h"

MeshRaytracingApplication::MeshRaytracingApplication()
    : Application(1024, 1024, "Ray-tracing demo")
    , m_renderer(GetDevice())
    , m_frameCount(0)
    , m_sphereCenter(0, 4, 4)
    , m_boxMatrix(glm::translate(glm::vec3(3, 0, 0)))
    , m_meshMatrix(glm::translate(glm::vec3(0, 0, 0)))
{
}

std::shared_ptr<Texture2DObject> MeshRaytracingApplication::LoadTexture(const char* path)
{
    std::shared_ptr<Texture2DObject> texture = std::make_shared<Texture2DObject>();

    int width = 0;
    int height = 0;
    int components = 0;

    // Load the texture data here
    unsigned char* data = stbi_load(path, &width, &height, &components, 4);
    stbi_set_flip_vertically_on_load(true);

    texture->Bind();
    texture->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormatRGBA, std::span<const unsigned char>(data, width * height * 4));

    // Generate mipmaps
    texture->GenerateMipmap();

    // Release texture data
    stbi_image_free(data);

    return texture;
}

void MeshRaytracingApplication::Initialize()
{
    Application::Initialize();

    // Initialize DearImGUI
    m_imGui.Initialize(GetMainWindow());

    InitializeCamera();
    InitializeMaterial();
    InitializeFramebuffer();
    InitializeRenderer();
    InitializeModels();
    InitializeSSBO();
    //InitializeTextureArray();
}

void MeshRaytracingApplication::Update()
{
    Application::Update();

    // Update camera controller
    m_cameraController.Update(GetMainWindow(), GetDeltaTime());

    // Invalidate accumulation when moving the camera
    if (m_cameraController.IsEnabled())
    {
        InvalidateScene();
    }

    // Set renderer camera
    const Camera& camera = *m_cameraController.GetCamera()->GetCamera();
    m_renderer.SetCurrentCamera(camera);

    // Update the material properties
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    m_material->SetUniformValue("ViewMatrix", viewMatrix);
    m_material->SetUniformValue("ProjMatrix", camera.GetProjectionMatrix());
    m_material->SetUniformValue("InvProjMatrix", glm::inverse(camera.GetProjectionMatrix()));
    m_material->SetUniformValue("SphereCenter", glm::vec3(viewMatrix * glm::vec4(m_sphereCenter, 1.0f)));
    m_material->SetUniformValue("BoxMatrix", viewMatrix * m_boxMatrix);
    m_material->SetUniformValue("MeshMatrix", viewMatrix * m_meshMatrix);
    m_material->SetUniformValue("FrameCount", ++m_frameCount);
}

void MeshRaytracingApplication::Render()
{
    Application::Render();

    GetDevice().Clear(true, Color(0.0f, 0.0f, 0.0f, 1.0f), true, 1.0f);

    // Render the scene
    m_renderer.Render();

    // Render the debug user interface
    RenderGUI();
}

void MeshRaytracingApplication::Cleanup()
{
    // Cleanup DearImGUI
    m_imGui.Cleanup();

    Application::Cleanup();
}

void MeshRaytracingApplication::InvalidateScene()
{
    m_frameCount = 0;
}

void MeshRaytracingApplication::InitializeCamera()
{
    // Create the main camera
    std::shared_ptr<Camera> camera = std::make_shared<Camera>();
    camera->SetViewMatrix(glm::vec3(0, 2.0f, 0), glm::vec3(0.0f, 2.3, -7), glm::vec3(0.0f, 1.0f, 0.0));
    float fov = 1.57f;
    camera->SetPerspectiveProjectionMatrix(fov, GetMainWindow().GetAspectRatio(), 0.1f, 100.0f);

    // Create a scene node for the camera
    std::shared_ptr<SceneCamera> sceneCamera = std::make_shared<SceneCamera>("camera", camera);

    // Set the camera scene node to be controlled by the camera controller
    m_cameraController.SetCamera(sceneCamera);
}

void MeshRaytracingApplication::InitializeMaterial()
{
    m_material = CreateRaytracingMaterial("shaders/intersection_checks.glsl");

    // Initialize material uniforms
    m_material->SetUniformValue("SphereCenter", m_sphereCenter);
    m_material->SetUniformValue("SphereRadius", 1.25f);
    m_material->SetUniformValue("SphereColor", glm::vec3(1.f));
    m_material->SetUniformValue("SphereRoughness", 0.f);
    m_material->SetUniformValue("SphereMetalness", 1.f);

    m_material->SetUniformValue("BoxMatrix", m_boxMatrix);
    m_material->SetUniformValue("BoxSize", glm::vec3(1, 1, 1));
    m_material->SetUniformValue("BoxColor", glm::vec3(1, 0, 0));
    m_material->SetUniformValue("BoxRoughness", 1.f);
    m_material->SetUniformValue("BoxMetalness", 0.5f);

    m_material->SetUniformValue("MeshMatrix", m_meshMatrix);
    m_material->SetUniformValue("MeshColor", glm::vec3(0, 1, 0));
    m_material->SetUniformValue("MeshRoughness", 0.f);
    m_material->SetUniformValue("MeshMetalness", 1.f);

    m_material->SetUniformValue("LightColor", glm::vec3(1.0f));
    m_material->SetUniformValue("LightIntensity", 4.0f);
    m_material->SetUniformValue("LightSize", glm::vec2(3.0f));

    m_materials.emplace_back(0, glm::vec4(glm::vec4(1, 0, 0,0)), 0.5f,0.f,1.35f);

    m_material->SetUniformValue("WallTexture", LoadTexture("models/Wall.jpg"));
    m_materials.emplace_back(1, glm::vec4(1.f), 1.f);

    m_material->SetUniformValue("FloorTexture", LoadTexture("models/Floor.jpg"));
    m_materials.emplace_back(2, glm::vec4(1.f), 0.0f, 1.f);

    m_material->SetUniformValue("MonaTexture", LoadTexture("models/Mona.jpg"));
    m_materials.emplace_back(3, glm::vec4(1.f), 1.f, 0.f);

    //m_material->SetBlendEquation(Material::BlendEquation::None);

    // Enable blending and set the blending parameters to alpha blending
    m_material->SetBlendEquation(Material::BlendEquation::Add);
    m_material->SetBlendParams(Material::BlendParam::SourceAlpha, Material::BlendParam::OneMinusSourceAlpha);
}

void MeshRaytracingApplication::InitializeFramebuffer()
{
    int width, height;
    GetMainWindow().GetDimensions(width, height);

    // Scene Texture
    m_sceneTexture = std::make_shared<Texture2DObject>();
    m_sceneTexture->Bind();
    m_sceneTexture->SetImage(0, width, height, TextureObject::FormatRGBA, TextureObject::InternalFormat::InternalFormatRGBA16F);
    m_sceneTexture->SetParameter(TextureObject::ParameterEnum::MinFilter, GL_LINEAR);
    m_sceneTexture->SetParameter(TextureObject::ParameterEnum::MagFilter, GL_LINEAR);
    Texture2DObject::Unbind();

    // Scene framebuffer
    m_sceneFramebuffer = std::make_shared<FramebufferObject>();
    m_sceneFramebuffer->Bind();
    m_sceneFramebuffer->SetTexture(FramebufferObject::Target::Draw, FramebufferObject::Attachment::Color0, *m_sceneTexture);
    m_sceneFramebuffer->SetDrawBuffers(std::array<FramebufferObject::Attachment, 1>({ FramebufferObject::Attachment::Color0 }));
    FramebufferObject::Unbind();
}

void MeshRaytracingApplication::InitializeRenderer()
{
    m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(m_material, m_sceneFramebuffer));

    std::shared_ptr<Material> copyMaterial = CreateCopyMaterial();
    copyMaterial->SetUniformValue("SourceTexture", m_sceneTexture);
    m_renderer.AddRenderPass(std::make_unique<PostFXRenderPass>(copyMaterial, m_renderer.GetDefaultFramebuffer()));
}

void MeshRaytracingApplication::InitializeModels()
{
    // Configure loader
    ModelLoader loader(m_material);

    //LoadModel(loader, "models/Box.obj", 0, glm::translate(glm::vec3(1.0f, 2.3, -3)) * glm::scale(glm::vec3(0.75f)));
    LoadModel(loader, "models/Wall_East.obj", 1);
    LoadModel(loader, "models/Wall_West.obj", 1);
    LoadModel(loader, "models/Wall_South.obj", 1);
    LoadModel(loader, "models/Wall_North.obj", 1);
    LoadModel(loader, "models/Ceiling.obj", 1);
    LoadModel(loader, "models/Floor.obj", 2);
    LoadModel(loader, "models/Mona.obj", 3);
}

void MeshRaytracingApplication::LoadModel(ModelLoader &loader, const char* path, unsigned int materialId, glm::mat4 transform)
{
    std::shared_ptr<Model> model = loader.LoadShared(path);
    m_transforms.push_back(transform);
    int transformId = m_transforms.size() - 1;

    model->GetMesh().SetTriangleMaterialID(materialId);
    model->GetMesh().SetTriangleTransformID(transformId);

    m_models.push_back(model);
}

void MeshRaytracingApplication::InitializeSSBO()
{
    std::vector<Triangle> collectedTriangleData;
	for (const auto& model : m_models)
	{
        const std::vector<Triangle>& meshData = model->GetMesh().GetTriangleData();
        collectedTriangleData.insert(collectedTriangleData.end(), meshData.begin(), meshData.end());
	}

    m_ssboTriangles.Bind();
    m_ssboTriangles.AllocateData(std::span(collectedTriangleData), BufferObject::Usage::StaticDraw);
    m_ssboTriangles.BindSSBO(1);

    ShaderStorageBufferObject::Unbind();

    m_ssboTransforms.Bind();
    m_ssboTransforms.AllocateData(std::span(m_transforms), BufferObject::Usage::StaticDraw);
    m_ssboTransforms.BindSSBO(2);

    ShaderStorageBufferObject::Unbind();

    m_ssboMaterials.Bind();
    m_ssboMaterials.AllocateData(sizeof(RaytracingMaterial) * m_materials.size(), m_materials.data(), BufferObject::Usage::StaticDraw);
    m_ssboMaterials.BindSSBO(3);

    ShaderStorageBufferObject::Unbind();
}

std::shared_ptr<Material> MeshRaytracingApplication::CreateRaytracingMaterial(const char* fragmentShaderPath)
{
    // We could keep this vertex shader and reuse it, but it looks simpler this way
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/renderer/fullscreen.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths; 
	fragmentShaderPaths.push_back("shaders/version330.glsl");
	fragmentShaderPaths.push_back("shaders/utils.glsl");
	fragmentShaderPaths.push_back("shaders/transform.glsl");
	fragmentShaderPaths.push_back("shaders/raytracer.glsl");
	fragmentShaderPaths.push_back("shaders/raylibrary.glsl");
	fragmentShaderPaths.push_back(fragmentShaderPath);
	fragmentShaderPaths.push_back("shaders/raytracing.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    m_shaderProgramPtr = std::make_shared<ShaderProgram>();
    m_shaderProgramPtr->Build(vertexShader, fragmentShader);

    // Create material
    std::shared_ptr<Material> material = std::make_shared<Material>(m_shaderProgramPtr);
    return material;
}

std::shared_ptr<Material> MeshRaytracingApplication::CreateCopyMaterial()
{
    std::vector<const char*> vertexShaderPaths;
    vertexShaderPaths.push_back("shaders/version330.glsl");
    vertexShaderPaths.push_back("shaders/renderer/fullscreen.vert");
    Shader vertexShader = ShaderLoader(Shader::VertexShader).Load(vertexShaderPaths);

    std::vector<const char*> fragmentShaderPaths;
    fragmentShaderPaths.push_back("shaders/version330.glsl");
    fragmentShaderPaths.push_back("shaders/utils.glsl");
    fragmentShaderPaths.push_back("shaders/renderer/copy.frag");
    Shader fragmentShader = ShaderLoader(Shader::FragmentShader).Load(fragmentShaderPaths);

    std::shared_ptr<ShaderProgram> shaderProgramPtr = std::make_shared<ShaderProgram>();
    shaderProgramPtr->Build(vertexShader, fragmentShader);

    // Create material
    std::shared_ptr<Material> material = std::make_shared<Material>(shaderProgramPtr);

    return material;
}

void MeshRaytracingApplication::RenderGUI()
{
    //m_imGui.BeginFrame();

    //bool changed = false;

    //if (auto window = m_imGui.UseWindow("Scene parameters"))
    //{
    //    if (ImGui::TreeNodeEx("Sphere", ImGuiTreeNodeFlags_DefaultOpen))
    //    {
    //        changed |= ImGui::DragFloat3("Center", &m_sphereCenter[0], 0.1f);
    //        changed |= ImGui::DragFloat("Radius", m_material->GetDataUniformPointer<float>("SphereRadius"), 0.1f);
    //        //changed |= ImGui::ColorEdit3("Color", m_material->GetDataUniformPointer<float>("SphereColor"));
    //        //changed |= ImGui::DragFloat("Metalness", m_material->GetDataUniformPointer<float>("SphereMetalness"), 0.01, 0.f, 1.f);
    //        //changed |= ImGui::DragFloat("Roughness", m_material->GetDataUniformPointer<float>("SphereRoughness"), 0.01, 0.f, 1.f);
    //        
    //        ImGui::TreePop();
    //    }
    //    /*if (ImGui::TreeNodeEx("Box", ImGuiTreeNodeFlags_DefaultOpen))
    //    {
    //        static glm::vec3 translation(3, 0, 0);
    //        static glm::vec3 rotation(0.0f);

    //        changed |= ImGui::DragFloat3("Translation", &translation[0], 0.1f);
    //        changed |= ImGui::DragFloat3("Rotation", &rotation[0], 0.1f);
    //        changed |= ImGui::ColorEdit3("Color", m_material->GetDataUniformPointer<float>("BoxColor"));
    //        changed |= ImGui::DragFloat("Metalness", m_material->GetDataUniformPointer<float>("BoxMetalness"), 0.01, 0.f, 1.f);
    //        changed |= ImGui::DragFloat("Roughness", m_material->GetDataUniformPointer<float>("BoxRoughness"), 0.01, 0.f, 1.f);
    //        m_boxMatrix = glm::translate(translation) * glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z);

    //        ImGui::TreePop();
    //    }*/
    //    //if (ImGui::TreeNodeEx("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    //    //{
    //    //    static glm::vec3 translation(3, -3, 0);
    //    //    static glm::vec3 rotation(0.0f);

    //    //    //changed |= ImGui::DragFloat3("Translation", &translation[0], 0.1f);
    //    //    //changed |= ImGui::DragFloat3("Rotation", &rotation[0], 0.1f);
    //    //    //changed |= ImGui::ColorEdit3("Color", m_material->GetDataUniformPointer<float>("MeshColor"));
    //    //    //changed |= ImGui::DragFloat("Metalness", m_material->GetDataUniformPointer<float>("MeshMetalness"), 0.01, 0.f, 1.f);
    //    //    //changed |= ImGui::DragFloat("Roughness", m_material->GetDataUniformPointer<float>("MeshRoughness"), 0.01, 0.f, 1.f);
    //    //    m_meshMatrix = glm::translate(translation) * glm::eulerAngleXYZ(rotation.x, rotation.y, rotation.z);

    //    //    ImGui::TreePop();
    //    //}
    //    if (ImGui::TreeNodeEx("Light", ImGuiTreeNodeFlags_DefaultOpen))
    //    {
    //        //changed |= ImGui::DragFloat2("Size", m_material->GetDataUniformPointer<float>("LightSize"), 0.1f);
    //        changed |= ImGui::DragFloat("Intensity", m_material->GetDataUniformPointer<float>("LightIntensity"), 0.1f);
    //        changed |= ImGui::ColorEdit3("Color", m_material->GetDataUniformPointer<float>("LightColor"));

    //        ImGui::TreePop();
    //    }
    //}

    //if (changed)
    //{
    //    InvalidateScene();
    //}

    //m_imGui.EndFrame();
}

//void MeshRaytracingApplication::InitializeTextureArray() {
//    if (m_textureFiles.empty()) {
//        std::cerr << "No file paths provided for texture array.\n";
//        return;
//    }
//
//    int width, height, channels;
//    stbi_set_flip_vertically_on_load(true); // Optional, depending on your texture orientation
//
//    // Load the first image to get dimensions and format
//    unsigned char* data = stbi_load(m_textureFiles[0].c_str(), &width, &height, &channels, STBI_rgb_alpha);
//    if (!data) {
//        std::cerr << "Failed to load texture: " << m_textureFiles[0] << "\n";
//        return;
//    }
//    stbi_image_free(data); // We'll reload each image in the loop below
//
//    glGenTextures(1, &texture);
//    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
//
//    GLsizei layerCount = static_cast<GLsizei>(m_textureFiles.size());
//    glTexImage3D(texture, 0, GL_RGBA8, width, height, layerCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
//
//    for (GLsizei i = 0; i < layerCount; ++i) {
//        data = stbi_load(m_textureFiles[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
//        if (!data) {
//            std::cerr << "Failed to load texture: " << m_textureFiles[i] << "\n";
//            glDeleteTextures(1, &texture);
//            return;
//        }
//
//        glTexSubImage3D(
//            texture,
//            0,                // mipmap level
//            0, 0, i,          // x, y, z offset
//            width, height, 1, // width, height, depth (one layer)
//            GL_RGBA,
//            GL_UNSIGNED_BYTE,
//            data
//        );
//        stbi_image_free(data);
//    }
//
//    // Set texture parameters
//    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//
//    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
//}