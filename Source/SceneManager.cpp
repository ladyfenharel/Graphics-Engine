///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
// Addition
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <GLFW/glfw3.h>
#include <random>

// declaration of global variables and defines
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
	glm::vec3 flameColor;

}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	// create the shape meshes object
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// free the allocated objects
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}

	// free the allocated OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL, 
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename, 
		&width, 
		&height, 
		&colorChannels, 
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the 
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}
	
	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL &material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** The code in the methods BELOW is for preparing and     ***/
/*** rendering the 3D replicated scenes.                    ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/knit.jpg",
		"fabric");

	bReturn = CreateGLTexture(
		"textures/glass.jpg",
		"glass");

	bReturn = CreateGLTexture(
		"textures/rubber.jpg",
		"rubber");

	bReturn = CreateGLTexture(
		"textures/candle.jpg",
		"candle");

	bReturn = CreateGLTexture(
		"textures/stainless.jpg",
		"stainless");

	bReturn = CreateGLTexture(
		"textures/metal.jpg",
		"metal");

	bReturn = CreateGLTexture(
		"textures/pages.jpg",
		"pages");

	bReturn = CreateGLTexture(
		"textures/leather.png",
		"leather");

	bReturn = CreateGLTexture(
		"textures/wood.jpg",
		"wood");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.diffuseColor = glm::vec3(0.258824f, 0.258824f, 0.435294f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 0.3;
	backdropMaterial.tag = "backdrop";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec4(0.3f, 0.3f, 0.3f, 0.5f);
	glassMaterial.specularColor = glm::vec3(0.7f, 0.6f, 0.9f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	metalMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.6f);
	metalMaterial.shininess = 52.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.shininess = 0.1;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL flameMaterial;
	flameMaterial.diffuseColor = glm::vec3(1.0f, 0.5f, 0.0f);  // Bright orange
	flameMaterial.specularColor = glm::vec3(1.0f, 0.6f, 0.3f); // Highlight glow
	flameMaterial.shininess = 32.0f; // High shininess for glowing effect
	flameMaterial.emissiveColor = glm::vec3(1.0f, 0.4f, 0.0f);  // Bright orange glow
	flameMaterial.tag = "flame";
	// Simulate flame flickering by changing the emissive color
	float flickerIntensity = (sin(glfwGetTime() * 10.0f) * 0.2f) + 0.8f; // Flickering effect
	flameMaterial.emissiveColor *= flickerIntensity; // Apply flicker intensity

	m_objectMaterials.push_back(flameMaterial);

	OBJECT_MATERIAL liquidMaterial;
	liquidMaterial.diffuseColor = glm::vec4(0.396f, 0.694f, 0.996f, 0.9f);
	liquidMaterial.specularColor = glm::vec3(0.3f, 0.5f, 0.7f);
	liquidMaterial.shininess = 50.0f;
	liquidMaterial.tag = "liquid";

	m_objectMaterials.push_back(liquidMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light 
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting - to use the default rendered 
	// lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional light 1 for moonlight
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.05f, -0.3f, -0.1f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.08f, 0.08f, 0.1f); // Subtle moonlit shadow
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.3f, 0.3f, 0.5f); // Soft moonlight glow
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.6f, 0.6f, 0.7f); // Silvery highlights
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point light 1 - flickering flame light placed on top of flame mesh
	float flameTime = static_cast<float>(glfwGetTime()); // Get the current time

	// Flickering green channel
	float flickerFactor = sin(flameTime * 3.0f) * 0.3f + 0.5f;

	// Random variation
	static std::default_random_engine generator;
	static std::uniform_real_distribution<float> distribution(-0.03f, 0.03f);

	float red = 1.0f + distribution(generator);
	float green = 0.2f + flickerFactor * 0.2f + distribution(generator);
	float blue = 0.1f + distribution(generator) * 0.1f; // Small, subtle blue tint

	// Clamp values
	red = glm::clamp(red, 0.0f, 1.0f);
	green = glm::clamp(green, 0.0f, 1.0f);
	blue = glm::clamp(blue, 0.0f, 1.0f);

	// Update flameColor
	flameColor = glm::vec3(red, green, blue);

	// Update point light 1
	m_pShaderManager->setVec3Value("pointLights[3].position", -5.0f, 8.8f, 0.9f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", flameColor * 0.1f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", flameColor);
	m_pShaderManager->setVec3Value("pointLights[3].specular", flameColor * 0.8f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);

	// Point light 2 - cool bluish-purple magical light
	m_pShaderManager->setVec3Value("pointLights[1].position", -4.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.04f, 0.03f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.4f, 0.3f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.5f, 0.4f, 0.3f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Point light 3 - soft pinkish-purple magical light
	m_pShaderManager->setVec3Value("pointLights[2].position", 3.8f, 5.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.08f, 0.06f, 0.1f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.2f, 0.2f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.3f, 0.3f, 0.6f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
	
	// Spotlight for focus (moonbeam with a magical touch)
	m_pShaderManager->setVec3Value("spotLight.ambient", 0.1f, 0.1f, 0.15f);
	m_pShaderManager->setVec3Value("spotLight.diffuse", 0.6f, 0.6f, 0.9f);
	m_pShaderManager->setVec3Value("spotLight.specular", 0.9f, 0.9f, 1.2f);
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.032f);
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(35.0f)));
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(45.0f)));
	m_pShaderManager->setBoolValue("spotLight.bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh(); // not used yet
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderTable();
	RenderBackdrop();
	RenderPotionBottle();
	RenderCandle();
	RenderBottomBook();
	RenderTopBook();
	RenderCauldron();
}

/***********************************************************
 *  RenderTable()
 *
 *  This method is called to render the shapes for the table
 *  object.
 ***********************************************************/
void SceneManager::RenderTable()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/***********************/
	/*** FABRIC ON TABLE ***/
	/***********************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, .2f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.2f, -0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("fabric");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawBoxMesh();

	/********************/
	/*** ACTUAL TABLE ***/
	/********************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.5f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -1.2f, -0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.39f, 0.24f, 0.12f, 1.0f); // Very dark brown
	SetShaderTexture("wood");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawBoxMesh();
}

/***********************************************************
 *  RenderBackdrop()
 *
 *  This method is called to render the shapes for the scene
 *  backdrop object.
 ***********************************************************/
void SceneManager::RenderBackdrop()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 15.0f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("fabric");
	SetTextureUVScale(10.0, 10.0);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values - this plane is used for the backdrop
	m_basicMeshes->DrawPlaneMesh();
}

/***********************************************************
 *  RenderPotionBottle()
 *
 *  This method is called to render the shapes for the potion
 *  bottle object.
 ***********************************************************/
void SceneManager::RenderPotionBottle()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	float scaleFactor = 0.9f;

	/************************************************/
	/*** LIQUID IN BOTTLE - box						***/
	/************************************************/

	float liquidScaleFactor = 0.8f;

	// color for the liquid
	glm::vec4 liquidColor = glm::vec4(0.396f, 0.694f, 0.996f, 0.7f);

	// Liquid for the bottom section - Box shape
	scaleXYZ = glm::vec3(2.0f, 2.8f, 2.0f) * liquidScaleFactor;
	positionXYZ = glm::vec3(4.0f, 5.5f * liquidScaleFactor, -1.0f);

	SetTransformations(
		scaleXYZ,
		0.0f, 15.0f, 0.0f, // Same rotation as the bottle
		positionXYZ);

	SetShaderColor(liquidColor.r, liquidColor.g, liquidColor.b, liquidColor.a);
	SetShaderMaterial("liquid");

	m_basicMeshes->DrawBoxMesh();

	/************************************************/
	/*** BOTTOM OF POTION BOTTLE - Box shape	  ***/
	/************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 3.5f, 2.0f) * scaleFactor;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 4.75f * scaleFactor, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.196, 0.294, 0.796, 0.65);
	//SetShaderTexture("glass");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	// draw a box mesh minus the top
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::front);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::back);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::left);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::right);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::bottom);

	/************************************************/
	/*** MIDDLE OF POTION - Pyramid shape		  ***/
	/************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 1.5f, 2.0f) * scaleFactor;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 7.25f * scaleFactor, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.196, 0.294, 0.796, 0.65);
	//SetShaderTexture("glass");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();

	/************************************************/
	/*** NECK OF BOTTLE - Cylinder shape		  ***/
	/************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 2.2f, 0.35f) * scaleFactor;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 15.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 7.0f * scaleFactor, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.196, 0.294, 0.796, 0.65);
	//SetShaderTexture("glass");
	//SetTextureUVScale(2.0, 1.0);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/************************************************/
	/*** LIP OF BOTTLE - Torus shape			  ***/
	/************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.42f, 0.42f, 0.65f) * scaleFactor;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 9.2f * scaleFactor, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.196, 0.294, 0.796, 0.65);
	//SetShaderTexture("glass");
	//SetTextureUVScale(0.5, 1.0);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/************************************************/
	/*** BOTTLE CLOSURE - Tapered Cylinder shape  ***/
	/************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.45f, 0.69f, 0.45f) * scaleFactor;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 9.92f * scaleFactor, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.196, 0.294, 0.796, 0.8);
	SetShaderTexture("rubber");
	SetTextureUVScale(2.0, 2.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// disable transparency
	glDisable(GL_BLEND);
}

/***********************************************************
 *  RenderCandle()
 *
 *  This method is called to render the shapes for the wine
 *  bottle object.
 ***********************************************************/
void SceneManager::RenderCandle()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	
	/*** Set needed transformations before drawing the basic mesh ***/
	
	/****************************************************************/
	/*** Set needed transformations  and draw the basic mesh of   ***/
	/*** CANDLE - Torus shape touching the floor			      ***/
	/****************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.7f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 0.25f, 0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("metal");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Set needed transformations  and draw the basic mesh of   ***/
	/*** CANDLE - Tapered cylinder for Bell shape bottom	      ***/
	/****************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 2.2f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 0.3f, 0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("metal");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	/*** Set needed transformations  and draw the basic mesh of   ***/
	/*** CANDLE - Middle cylinder							      ***/
	/****************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 2.5f, 0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("metal");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Set needed transformations  and draw the basic mesh of   ***/
	/*** CANDLE - Middle tapered upside down cylinder		      ***/
	/****************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.58f, 1.4f, 0.58f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 3.0f, 0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("metal");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	/*** Set needed transformations  and draw the basic mesh of   ***/
	/*** CANDLE - Top tapered upside down cylinder		      ***/
	/****************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.62f, 1.9f, 0.62f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 5.8f, 0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("metal");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/****************************************************************/
	/*** Set needed transformations  and draw the basic mesh of   ***/
	/*** CANDLE - Torus shape for the lip of the candleholder     ***/
	/****************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.7f, 0.7f, 0.45f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 5.85f, 0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("metal");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Set needed transformations  and draw the basic mesh of   ***/
	/*** CANDLE - Cylinder for actual candle				      ***/
	/****************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 2.4f, 0.35f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 5.8f, 0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1.0, 1.0, 1.0, 1.0);
	SetShaderTexture("candle");
	SetTextureUVScale(2.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/*** Set needed transformations  and draw the basic mesh of   ***/
	/*** CANDLE - Cone shape for wick						      ***/
	/****************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.04f, 0.6f, 0.04f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 8.0f, 0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1.0);
	SetShaderTexture("rubber");
	//SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	/****************************************************************/
	/*** Set needed transformations  and draw the basic mesh of   ***/
	/*** CANDLE - Flame on  wick							      ***/
	/****************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.8f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 8.0f, 0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// flickering transparency
	float alpha = sin(static_cast<float>(glfwGetTime()) * 2.5f) * 0.2f + 0.6f;
	alpha = glm::clamp(alpha, 0.5f, 0.8f);

	// set color of flame with same color from the light source + the flickering transparency
	SetShaderColor(flameColor.r, flameColor.g, flameColor.b, alpha); // Set object color with transparency

	SetShaderMaterial("flame");  // Set the flame material properties (defined earlier)

	// Enable blending for transparency (flames are semi-transparent)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	glDisable(GL_BLEND); // Disable blending after drawing the flame
}

/***********************************************************
 *  RenderBottomBook()
 *
 *  This method is called to render the shapes for the book
 *  object on the bottom of pile.
 ***********************************************************/
void SceneManager::RenderBottomBook()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/******************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** BOTTOM BOOK - Box shape, pages					***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 1.4f, 4.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.75f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.659, 0.576, 0.439, 1.0);
	SetShaderTexture("pages");
	SetTextureUVScale(2.0, 0.5);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values, only sides visible
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::front);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::back);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::left);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::right);

	/******************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** BOTTOM BOOK - Box shape, bottom cover						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.2f, 0.2f, 4.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.1f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.36, 0.25, 0.20, 1.0);
	SetShaderTexture("leather");
	SetTextureUVScale(0.5, 6.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** BOTTOM BOOK - Box shape, top cover							***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.2f, 0.2f, 4.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 1.5f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.36, 0.25, 0.20, 1.0);
	SetShaderTexture("leather");
	SetTextureUVScale(0.5, 6.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** BOTTOM BOOK - Box shape, binding							***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.6f, 0.2f, 4.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 30.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.8f, 0.8f, -2.05f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.36, 0.25, 0.20, 1.0);
	SetShaderTexture("leather");
	SetTextureUVScale(0.5, 6.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}

/***********************************************************
 *  RenderTopBook()
 *
 *  This method is called to render the shapes for the book
 *  on the top of book pile.
 ***********************************************************/
void SceneManager::RenderTopBook()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	/******************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** TOP BOOK - Box shape, pages								***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 1.5f, 4.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 2.35f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.659, 0.576, 0.439, 1.0);
	SetShaderTexture("pages");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::front);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::back);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::left);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::right);

	/******************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** TOP BOOK - Box shape, bottom cover							***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.2f, 0.2f, 4.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 1.7f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.36, 0.25, 0.20, 1.0);
	SetShaderTexture("leather");
	// SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** TOP BOOK - Box shape, top cover							***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.2f, 0.2f, 4.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 3.1f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.36, 0.25, 0.20, 1.0);
	SetShaderTexture("leather");
	// SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** TOP BOOK - Box shape, binding								***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.6f, 0.2f, 4.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 2.4f, -2.735f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.36, 0.25, 0.20, 1.0);
	SetShaderTexture("leather");
	SetTextureUVScale(0.5, 6.0);
	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/
}

/***********************************************************
 *  RenderCauldron()
 *
 *  This method is called to render the shapes for the book
 *  on the top of book pile.
 ***********************************************************/
void SceneManager::RenderCauldron()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	/*** Set transformations and draw the main body of the cauldron ***/
	/****************************************************************/
	// Set the XYZ scale for the cauldron body (half sphere for cauldron shape)
	scaleXYZ = glm::vec3(3.2f, 4.2f, 3.2f);

	// Set the XYZ position for the cauldron body
	positionXYZ = glm::vec3(-1.5f, 4.6f, -2.5f);

	// Apply transformations and properties for the cauldron body
	SetTransformations(
		scaleXYZ,
		180.0f,
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal");

	// Draw the main body as a half sphere
	m_basicMeshes->DrawHalfSphereMesh();

	/****************************************************************/
	/*** Set transformations and draw the rim of the cauldron      ***/
	/****************************************************************/
	// Set the XYZ scale for the rim (torus)
	scaleXYZ = glm::vec3(3.0f, 3.0f, 0.6f);

	// Set the XYZ position for the rim
	positionXYZ = glm::vec3(-1.5f, 4.5f, -2.5f); 

	// Apply transformations for the rim
	SetTransformations(
		scaleXYZ,
		90.0f,  // Rotate 90 degrees around the X-axis to lay the torus parallel to the half-sphere
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal");

	// Draw the rim as a torus mesh (correct orientation)
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/
	/*** Set transformations and draw the legs of the cauldron     ***/
	/****************************************************************/
	// Define the scale for each leg
	glm::vec3 legScale = glm::vec3(0.6f, 1.9f, 0.3f);

	// Define the positions for three evenly spaced legs under the cauldron
	glm::vec3 legPositions[] = {
		glm::vec3(-2.8f, 2.0f, -3.0f),
		glm::vec3(-0.2f, 2.0f, -3.0f),
		glm::vec3(-1.25f, 2.0f, -1.0f)
	};

	for (const auto& legPos : legPositions) {
		// Set transformations for each leg
		SetTransformations(
			legScale,
			0.0f,
			0.0f,
			180.0f,
			legPos);

		SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
		SetShaderTexture("metal");
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderMaterial("metal");

		// Draw each leg as a cylinder mesh
		m_basicMeshes->DrawTaperedCylinderMesh();
	}

	/********************************************************************/
	/*** Set transformations and draw the liquid inside the cauldron  ***/
	/********************************************************************/
	// Enable blending for transparency (liquid is semi-transparent)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Set the scale for the liquid inside the cauldron (slightly smaller than the body)
	scaleXYZ = glm::vec3(2.8f, 1.5f, 2.8f);

	// Set the position for the liquid inside (slightly below the rim)
	positionXYZ = glm::vec3(-1.5f, 2.75f, -2.5f);

	// Apply transformations for the liquid
	SetTransformations(
		scaleXYZ,
		0.0f,   // No rotation for the liquid
		0.0f,
		0.0f,
		positionXYZ);

	SetShaderColor(0.3f, 0.8f, 1.0f, 0.65f);
	SetShaderMaterial("liquid");

	// Draw the liquid as a cylinder mesh (inside the cauldron)
	m_basicMeshes->DrawCylinderMesh();
	glDisable(GL_BLEND); // Disable blending after drawing the liquid
}


