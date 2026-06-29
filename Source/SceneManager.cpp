///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

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
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
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
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load the scene textures before drawing the scene.
	CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "wood");
	CreateGLTexture("../../Utilities/textures/backdrop.jpg", "screen");
	CreateGLTexture("../../Utilities/textures/stainless.jpg", "metal");
	BindGLTextures();

	OBJECT_MATERIAL deskMaterial;
	deskMaterial.ambientColor = glm::vec3(0.35f, 0.28f, 0.20f);
	deskMaterial.ambientStrength = 0.18f;
	deskMaterial.diffuseColor = glm::vec3(0.55f, 0.45f, 0.34f);
	deskMaterial.specularColor = glm::vec3(0.20f, 0.16f, 0.12f);
	deskMaterial.shininess = 8.0f;
	deskMaterial.tag = "desk";
	m_objectMaterials.push_back(deskMaterial);

	OBJECT_MATERIAL wallMaterial;
	wallMaterial.ambientColor = glm::vec3(0.72f, 0.70f, 0.66f);
	wallMaterial.ambientStrength = 0.22f;
	wallMaterial.diffuseColor = glm::vec3(0.86f, 0.84f, 0.78f);
	wallMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	wallMaterial.shininess = 4.0f;
	wallMaterial.tag = "wallMaterial";
	m_objectMaterials.push_back(wallMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.24f, 0.24f, 0.24f);
	metalMaterial.ambientStrength = 0.16f;
	metalMaterial.diffuseColor = glm::vec3(0.50f, 0.50f, 0.48f);
	metalMaterial.specularColor = glm::vec3(0.35f, 0.35f, 0.35f);
	metalMaterial.shininess = 20.0f;
	metalMaterial.tag = "metalMaterial";
	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL screenMaterial;
	screenMaterial.ambientColor = glm::vec3(0.22f, 0.22f, 0.24f);
	screenMaterial.ambientStrength = 0.22f;
	screenMaterial.diffuseColor = glm::vec3(0.58f, 0.58f, 0.60f);
	screenMaterial.specularColor = glm::vec3(0.12f, 0.12f, 0.14f);
	screenMaterial.shininess = 10.0f;
	screenMaterial.tag = "screenMaterial";
	m_objectMaterials.push_back(screenMaterial);

	OBJECT_MATERIAL darkMaterial;
	darkMaterial.ambientColor = glm::vec3(0.03f, 0.03f, 0.03f);
	darkMaterial.ambientStrength = 0.18f;
	darkMaterial.diffuseColor = glm::vec3(0.08f, 0.08f, 0.08f);
	darkMaterial.specularColor = glm::vec3(0.18f, 0.18f, 0.18f);
	darkMaterial.shininess = 12.0f;
	darkMaterial.tag = "darkMaterial";
	m_objectMaterials.push_back(darkMaterial);

	OBJECT_MATERIAL potMaterial;
	potMaterial.ambientColor = glm::vec3(0.18f, 0.08f, 0.04f);
	potMaterial.ambientStrength = 0.20f;
	potMaterial.diffuseColor = glm::vec3(0.40f, 0.18f, 0.10f);
	potMaterial.specularColor = glm::vec3(0.08f, 0.05f, 0.03f);
	potMaterial.shininess = 8.0f;
	potMaterial.tag = "potMaterial";
	m_objectMaterials.push_back(potMaterial);

	OBJECT_MATERIAL leafMaterial;
	leafMaterial.ambientColor = glm::vec3(0.04f, 0.16f, 0.04f);
	leafMaterial.ambientStrength = 0.22f;
	leafMaterial.diffuseColor = glm::vec3(0.12f, 0.42f, 0.12f);
	leafMaterial.specularColor = glm::vec3(0.06f, 0.10f, 0.06f);
	leafMaterial.shininess = 6.0f;
	leafMaterial.tag = "leafMaterial";
	m_objectMaterials.push_back(leafMaterial);

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Soft overhead light for the desktop scene.
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(0.0f, 6.0f, 5.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.08f, 0.08f, 0.08f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.70f, 0.70f, 0.68f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.35f, 0.35f, 0.34f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 24.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.08f);

	// Weak fill light keeps the monitor visible.
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-5.0f, 3.0f, 4.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.04f, 0.04f, 0.05f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.30f, 0.32f, 0.36f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.10f, 0.11f, 0.12f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 18.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.03f);

	// Unused light slots are disabled.
	m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(0.0f, -50.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(0.0f));
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.0f);

	m_pShaderManager->setVec3Value("lightSources[3].position", glm::vec3(0.0f, -50.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", glm::vec3(0.0f));
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", glm::vec3(0.0f));
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.0f);

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
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
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(16.0f, 1.0f, 8.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wood");
	SetTextureUVScale(4.0f, 4.0f);
	SetShaderMaterial("desk");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// wall
	scaleXYZ = glm::vec3(16.0f, 1.0f, 7.8f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 3.9f, -3.7f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.86f, 0.84f, 0.78f, 1.0f);
	SetShaderMaterial("wallMaterial");
	m_basicMeshes->DrawPlaneMesh();

	// monitor base
	scaleXYZ = glm::vec3(1.25f, 0.14f, 0.8f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.2f, 0.07f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("metal");
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderMaterial("metalMaterial");
	m_basicMeshes->DrawBoxMesh();

	// monitor stand
	scaleXYZ = glm::vec3(0.22f, 1.05f, 0.22f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.2f, 0.14f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("metal");
	SetTextureUVScale(3.0f, 3.0f);
	SetShaderMaterial("metalMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// monitor screen
	scaleXYZ = glm::vec3(9.2f, 3.0f, 0.25f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 2.35f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("screen");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("screenMaterial");
	m_basicMeshes->DrawBoxMesh();

	// keyboard
	scaleXYZ = glm::vec3(3.9f, 0.09f, 0.85f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.15f, 0.055f, 2.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.04f, 0.04f, 0.04f, 1.0f);
	SetShaderMaterial("darkMaterial");
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(3.45f, 0.04f, 0.13f);
	positionXYZ = glm::vec3(-0.15f, 0.12f, 2.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.12f, 0.12f, 0.12f, 1.0f);
	SetShaderMaterial("darkMaterial");
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-0.15f, 0.12f, 2.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-0.15f, 0.12f, 2.85f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	// mouse
	scaleXYZ = glm::vec3(0.55f, 0.17f, 0.78f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.9f, 0.16f, 2.65f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
	SetShaderMaterial("darkMaterial");
	m_basicMeshes->DrawSphereMesh();

	// plant pot
	scaleXYZ = glm::vec3(0.42f, 0.58f, 0.42f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-5.55f, 0.05f, 0.85f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.38f, 0.16f, 0.08f, 1.0f);
	SetShaderMaterial("potMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// plant leaves
	scaleXYZ = glm::vec3(0.95f, 1.25f, 0.95f);
	positionXYZ = glm::vec3(-5.55f, 0.58f, 0.85f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.10f, 0.38f, 0.10f, 1.0f);
	SetShaderMaterial("leafMaterial");
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
}
