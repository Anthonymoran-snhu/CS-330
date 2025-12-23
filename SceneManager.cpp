///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
// 
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <iostream>
#include <cmath>
#include <GLFW/glfw3.h>

namespace
{
    const char* g_ModelName = "model";
    const char* g_ColorValueName = "objectColor";
    const char* g_TextureValueName = "objectTexture";
    const char* g_UseTextureName = "bUseTexture";
    const char* g_UseLightingName = "bUseLighting";
    const char* g_ViewPositionName = "viewPosition";
    const char* g_MaterialDiffuseName = "material.diffuseColor";
    const char* g_MaterialSpecularName = "material.specularColor";
    const char* g_MaterialShininessName = "material.shininess";
    const char* g_DirLightActive = "directionalLight.bActive";
    const char* g_DirLightDirection = "directionalLight.direction";
    const char* g_DirLightAmbient = "directionalLight.ambient";
    const char* g_DirLightDiffuse = "directionalLight.diffuse";
    const char* g_DirLightSpecular = "directionalLight.specular";
}

/***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();
    m_loadedTextures = 0;

    // Initialize texture IDs
    for (int i = 0; i < 16; i++)
        m_textureIDs[i].ID = 0;
}

/***********************************************************/
SceneManager::~SceneManager()
{
    DestroyGLTextures();
    if (m_basicMeshes) delete m_basicMeshes;
    m_basicMeshes = nullptr;
    m_pShaderManager = nullptr;
}

/***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data)
    {
        std::cout << "Failed to load texture: " << filename << std::endl;
        return false;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_textureIDs[m_loadedTextures].ID = textureID;
    m_textureIDs[m_loadedTextures].tag = tag;
    m_loadedTextures++;

    return true;
}

/***********************************************************/
void SceneManager::BindGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
    }
}

/***********************************************************/
void SceneManager::DestroyGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
        glDeleteTextures(1, &m_textureIDs[i].ID);
    m_loadedTextures = 0;
}

/***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
    for (int i = 0; i < m_loadedTextures; i++)
        if (m_textureIDs[i].tag == tag)
            return m_textureIDs[i].ID;
    return -1;
}

/***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
    for (int i = 0; i < m_loadedTextures; i++)
        if (m_textureIDs[i].tag == tag)
            return i;
    return -1;
}

/***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
    for (auto& m : m_objectMaterials)
    {
        if (m.tag == tag)
        {
            material = m;
            return true;
        }
    }
    return false;
}

/***********************************************************/
void SceneManager::SetTransformations(glm::vec3 scaleXYZ, float XrotationDegrees, float YrotationDegrees, float ZrotationDegrees, glm::vec3 positionXYZ)
{
    glm::mat4 model = glm::translate(positionXYZ)
        * glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f))
        * glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f))
        * glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f))
        * glm::scale(scaleXYZ);

    if (m_pShaderManager)
        m_pShaderManager->setMat4Value(g_ModelName, model);
}

/***********************************************************/
void SceneManager::SetShaderColor(float r, float g, float b, float a)
{
    if (m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, false);
        m_pShaderManager->setVec4Value(g_ColorValueName, glm::vec4(r, g, b, a));
    }
}

/***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
    if (m_pShaderManager)
    {
        int slot = FindTextureSlot(textureTag);
        if (slot >= 0)
        {
            m_pShaderManager->setIntValue(g_UseTextureName, true);
            m_pShaderManager->setSampler2DValue(g_TextureValueName, slot);
        }
    }
}

/***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
    if (m_pShaderManager)
        m_pShaderManager->setVec2Value("uvScale", glm::vec2(u, v));
}

/***********************************************************/
void SceneManager::SetShaderMaterial(std::string tag)
{
    OBJECT_MATERIAL mat;
    if (FindMaterial(tag, mat))
    {
        if (m_pShaderManager)
        {
            m_pShaderManager->setVec3Value(g_MaterialDiffuseName, mat.diffuseColor);
            m_pShaderManager->setVec3Value(g_MaterialSpecularName, mat.specularColor);
            m_pShaderManager->setFloatValue(g_MaterialShininessName, mat.shininess);
        }
    }
}

/***********************************************************/
void SceneManager::PrepareScene()
{
    // Load shapes
    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadBoxMesh();
    m_basicMeshes->LoadConeMesh();
    m_basicMeshes->LoadSphereMesh();
    m_basicMeshes->LoadTorusMesh();

    // Load textures
    CreateGLTexture("textures/desk_texture.jpg", "deskTexture");
    CreateGLTexture("textures/mug_texture.jpg", "mugTexture");
    CreateGLTexture("textures/monitor_texture.jpg", "monitorTexture");
    CreateGLTexture("textures/screen_texture.jpg", "screenTexture");
    CreateGLTexture("textures/keyboard_texture.jpg", "keyboardTexture");
    CreateGLTexture("textures/lamp_texture.jpg", "lampTexture");

    BindGLTextures();

    // Default material
    OBJECT_MATERIAL defaultMat;
    defaultMat.tag = "default";
    defaultMat.diffuseColor = glm::vec3(0.8f);
    defaultMat.specularColor = glm::vec3(1.0f);
    defaultMat.shininess = 64.0f;
    m_objectMaterials.push_back(defaultMat);
}

/***********************************************************/
void SceneManager::RenderScene()
{
    if (!m_pShaderManager) return;

    // Enable lighting
    m_pShaderManager->setIntValue(g_UseLightingName, true);

    // ----- Directional Light (main scene light) -----
    m_pShaderManager->setIntValue(g_DirLightActive, true);
    m_pShaderManager->setVec3Value(g_DirLightDirection, glm::vec3(-1.0f, -1.5f, -1.0f));
    m_pShaderManager->setVec3Value(g_DirLightAmbient, glm::vec3(0.35f, 0.45f, 0.8f));
    m_pShaderManager->setVec3Value(g_DirLightDiffuse, glm::vec3(0.9f));
    m_pShaderManager->setVec3Value(g_DirLightSpecular, glm::vec3(1.2f));

    // ----- Hovering Point Light (in front of desk) -----
    float time = (float)glfwGetTime();
    glm::vec3 hoverLightPos = glm::vec3(sin(time) * 3.0f, 4.0f, 5.0f);
    m_pShaderManager->setVec3Value("pointLights[0].position", hoverLightPos);
    m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.2f, 0.25f, 0.45f));
    m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.7f));
    m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(1.0f));

    // Default material
    SetShaderMaterial("default");

    glm::vec3 scale, pos;
    float deskY = 0.0f; // top of desk

    // ----- Desk -----
    float deskWidth = 10.0f;
    float deskDepth = 8.0f;
    float deskHeight = 1.0f;
    glm::vec3 deskPos = glm::vec3(0.0f, deskY, 0.0f);
    scale = glm::vec3(deskWidth, deskHeight, deskDepth);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, deskPos);
    SetShaderTexture("deskTexture");
    m_basicMeshes->DrawPlaneMesh();

    // ----- Coffee Mug -----
    float mugHeight = 1.0f;
    float mugOuterRadius = 0.37f;
    float mugInnerRadius = 0.28f;
    float holderHeight = 0.010f;

    glm::vec3 mugCenter = glm::vec3(2.0f, deskY + holderHeight + mugHeight * 0.5f, 1.0f);

    // Mug Holder
    glm::vec3 holderPos = glm::vec3(mugCenter.x, deskY + holderHeight * 0.0f, mugCenter.z);
    scale = glm::vec3(mugOuterRadius, holderHeight * 0.0f, mugOuterRadius);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, holderPos);
    SetShaderTexture("mugTexture");
    m_basicMeshes->DrawCylinderMesh();

    // Mug Body
    scale = glm::vec3(mugOuterRadius, mugHeight * -0.5f, mugOuterRadius);
    SetTransformations(scale, 0.0f, 0.5f, 0.0f, mugCenter);
    SetShaderTexture("mugTexture");
    m_basicMeshes->DrawCylinderMesh();

    // Mug Hollow
    scale = glm::vec3(mugInnerRadius, mugHeight * -0.0f, mugInnerRadius);
    glm::vec3 innerPos = mugCenter - glm::vec3(0.0f, 0.025f, 0.0f);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, innerPos);
    SetShaderTexture("mugTexture");
    m_basicMeshes->DrawCylinderMesh();

    // Mug Handle 
    float handleRadius = 0.15f;
    float handleThickness = 0.08f;
    glm::vec3 handlePos = mugCenter + glm::vec3(mugOuterRadius + handleRadius, -0.15f, 0.0f); // moved down
    scale = glm::vec3(handleRadius, handleRadius, handleThickness);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, handlePos);
    SetShaderTexture("mugTexture");
    m_basicMeshes->DrawTorusMesh();

    // Mug Liquid
    glm::vec3 liquidPos = mugCenter + glm::vec3(0.0f, mugHeight * -0.1f, 0.0f);
    scale = glm::vec3(mugInnerRadius, mugHeight * 0.15f, mugInnerRadius);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, liquidPos);
    SetShaderColor(0.55f, 0.35f, 0.1f, 1.0f);
    m_basicMeshes->DrawCylinderMesh();

    // ----- Monitor -----
    scale = glm::vec3(5.0f, 3.0f, 0.2f);
    pos = glm::vec3(0.0f, deskY + 2.0f, -3.5f);
    SetTransformations(scale, -5.0f, 0.0f, 0.0f, pos);
    SetShaderTexture("monitorTexture");
    m_basicMeshes->DrawBoxMesh();

    // Monitor Screen
    scale = glm::vec3(5.0f, 3.0f, 0.05f);
    pos = glm::vec3(0.0f, deskY + 2.0f, -3.4f);
    SetTransformations(scale, -5.0f, 0.0f, 0.0f, pos);
    SetShaderTexture("screenTexture");
    SetShaderMaterial("default");
    m_basicMeshes->DrawBoxMesh();

    // ----- Monitor Stand -----
    scale = glm::vec3(0.5f, 0.45f, 0.5f);
    pos = glm::vec3(0.0f, deskY + 0.175f, -3.5f);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, pos);
    SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
    m_basicMeshes->DrawCylinderMesh();

    // Monitor Stand Legs (Feet)
    float footHeight = 0.90f;
    glm::vec3 monitorCenter = pos;

    SetTransformations(glm::vec3(0.45f, footHeight, 0.15f), 0.0f, 0.5f, 0.0f,
        glm::vec3(monitorCenter.x - 0.6f, deskY + footHeight * 0.5f, monitorCenter.z));
    m_basicMeshes->DrawBoxMesh();

    SetTransformations(glm::vec3(0.45f, footHeight, 0.15f), 0.0f, 0.0f, 0.0f,
        glm::vec3(monitorCenter.x + 0.6f, deskY + footHeight * 0.5f, monitorCenter.z));
    m_basicMeshes->DrawBoxMesh();

    // ----- Keyboard -----
    scale = glm::vec3(4.0f, 0.3f, 2.0f);
    pos = glm::vec3(-0.60f, deskY + 0.15f, -1.2f);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, pos);
    SetShaderTexture("keyboardTexture");
    m_basicMeshes->DrawBoxMesh();

    // ----- Desk Lamp (Further Back and More to the Right) -----
    glm::vec3 lampBasePos = glm::vec3(mugCenter.x + 0.5f, deskY + 0.025f, mugCenter.z - 1.6f); // further back

    // Lamp Base
    scale = glm::vec3(0.5f, 0.05f, 0.5f);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, lampBasePos);
    SetShaderTexture("lampTexture");
    m_basicMeshes->DrawCylinderMesh();

    // Lamp Arms and Joints
    glm::mat4 lowerArmModel = glm::translate(glm::mat4(1.0f), lampBasePos + glm::vec3(0.0f, 0.05f, 0.0f));
    lowerArmModel = lowerArmModel * glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    lowerArmModel = lowerArmModel * glm::scale(glm::vec3(0.08f, 1.6f, 0.08f));
    m_pShaderManager->setMat4Value(g_ModelName, lowerArmModel);
    m_basicMeshes->DrawCylinderMesh();

    glm::mat4 lowerJointModel = glm::translate(lowerArmModel, glm::vec3(0.0f, 1.0f, 0.0f));
    lowerJointModel = lowerJointModel * glm::scale(glm::vec3(0.12f));
    m_pShaderManager->setMat4Value(g_ModelName, lowerJointModel);
    m_basicMeshes->DrawSphereMesh();

    glm::mat4 upperArmModel = glm::translate(lowerJointModel, glm::vec3(0.0f, 0.6f, 0.0f));
    upperArmModel = upperArmModel * glm::rotate(glm::mat4(1.0f), glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    upperArmModel = upperArmModel * glm::scale(glm::vec3(0.07f, 1.2f, 0.07f));
    m_pShaderManager->setMat4Value(g_ModelName, upperArmModel);
    m_basicMeshes->DrawCylinderMesh();

    glm::mat4 upperJointModel = glm::translate(upperArmModel, glm::vec3(0.0f, 0.6f, 0.0f));
    upperJointModel = upperJointModel * glm::scale(glm::vec3(0.1f));
    m_pShaderManager->setMat4Value(g_ModelName, upperJointModel);
    m_basicMeshes->DrawSphereMesh();

    // Lamp Head Cone 
    glm::vec3 headPos = glm::vec3(upperJointModel[3]) + glm::vec3(0.0f, 0.2f, 0.0f);
    scale = glm::vec3(0.3f, 0.5f, 0.3f);
    SetTransformations(scale, -80.0f, 0.0f, 0.0f, headPos); 
    SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
    m_basicMeshes->DrawConeMesh();

    // Bulb inside cone
    glm::vec3 bulbPos = headPos + glm::vec3(0.0f, -0.1f, 0.0f);
    scale = glm::vec3(0.15f);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, bulbPos);
    SetShaderColor(1.0f, 1.0f, 0.8f, 1.0f);
    m_basicMeshes->DrawSphereMesh();

    // Lamp Light
    m_pShaderManager->setVec3Value("pointLights[1].position", bulbPos);
    m_pShaderManager->setVec3Value("pointLights[1].ambient", glm::vec3(0.1f, 0.1f, 0.05f));
    m_pShaderManager->setVec3Value("pointLights[1].diffuse", glm::vec3(1.2f, 1.0f, 0.8f));
    m_pShaderManager->setVec3Value("pointLights[1].specular", glm::vec3(1.5f, 1.2f, 1.0f));

    // Optional glow halo
    SetShaderColor(1.0f, 0.9f, 0.7f, 0.3f);
    scale = glm::vec3(0.25f);
    SetTransformations(scale, 0.0f, 0.0f, 0.0f, bulbPos);
    m_basicMeshes->DrawSphereMesh();


}