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
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
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
		glDeleteTextures(1, &m_textureIDs[i].ID);
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
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.54f, 0.27f, 0.07f);  // Brownish color for wood
	woodMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);  // Less reflective
	woodMaterial.shininess = 12.0f;  // Low shininess
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);  // Neutral color for glass
	glassMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);  // Less reflective
	glassMaterial.shininess = 32.0f;  // Reduced shininess for a glassy look without washing out
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	
	OBJECT_MATERIAL beerMaterial;
	beerMaterial.diffuseColor = glm::vec3(0.8f, 0.6f, 0.1f);  // Yellow color for beer
	beerMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);  // Less reflective
	beerMaterial.shininess = 0.5f;  // Further lower shininess for a less glossy look
	beerMaterial.tag = "beer";

	m_objectMaterials.push_back(beerMaterial);

	OBJECT_MATERIAL foamMaterial;
	foamMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);  // White color for foam
	foamMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);  // Slightly reflective
	foamMaterial.shininess = 0.25f;  // Further lower shininess for a soft look
	foamMaterial.tag = "foam";

	m_objectMaterials.push_back(foamMaterial);

	
	OBJECT_MATERIAL lemonMaterial;
	lemonMaterial.diffuseColor = glm::vec3(1.0f, 0.9f, 0.0f);  // Bright yellow color for lemon
	lemonMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);  // Slightly reflective
	lemonMaterial.shininess = 2.0f;  // Lower shininess for a dull look
	lemonMaterial.tag = "lemon";

	m_objectMaterials.push_back(lemonMaterial);

	OBJECT_MATERIAL innerLemonMaterial;
	innerLemonMaterial.diffuseColor = glm::vec3(1.0f, 0.85f, 0.0f);  // Bright yellow color for inner lemon
	innerLemonMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);  // No reflection
	innerLemonMaterial.shininess = 0.0f;  // No shininess
	innerLemonMaterial.tag = "innerLemon";


	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.1f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 0.0;
	backdropMaterial.tag = "backdrop";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL plateMaterial;
	plateMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	plateMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	plateMaterial.shininess = 30.0;
	plateMaterial.tag = "plate";

	m_objectMaterials.push_back(plateMaterial);

	OBJECT_MATERIAL steelMaterial;
	steelMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	steelMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	steelMaterial.shininess = 82.0;
	steelMaterial.tag = "metal";

	m_objectMaterials.push_back(steelMaterial);

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
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Define brightness modifier
	float brightnessModifier = 1.0f; // Adjust this value to increase or decrease brightness

	// Set global ambient color to a slightly reduced subtle gray for natural base lighting
	m_pShaderManager->setVec3Value("globalAmbientColor", 0.15f, 0.15f, 0.15f);

	// Light Source 0: Sunlight from above (warm light)
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 10.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.9f * brightnessModifier, 0.8f * brightnessModifier, 0.7f * brightnessModifier);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.9f * brightnessModifier, 0.8f * brightnessModifier, 0.7f * brightnessModifier);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 20.0f * brightnessModifier);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.5f * brightnessModifier);

	// Light Source 1: Fill light from the front-right (dim soft light)
	m_pShaderManager->setVec3Value("lightSources[1].position", 5.0f, 5.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f * brightnessModifier, 0.2f * brightnessModifier, 0.2f * brightnessModifier);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.2f * brightnessModifier, 0.2f * brightnessModifier, 0.2f * brightnessModifier);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 8.0f * brightnessModifier);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f * brightnessModifier);

	// Light Source 2: Fill light from the front-left (dim soft light)
	m_pShaderManager->setVec3Value("lightSources[2].position", -5.0f, 5.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.2f * brightnessModifier, 0.2f * brightnessModifier, 0.2f * brightnessModifier);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.2f * brightnessModifier, 0.2f * brightnessModifier, 0.2f * brightnessModifier);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 8.0f * brightnessModifier);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f * brightnessModifier);

	// Light Source 3: Low intensity fill light from the back (blue light)
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 3.0f, -5.0f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.1f * brightnessModifier, 0.1f * brightnessModifier, 1.0f * brightnessModifier); // More blue
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.1f * brightnessModifier, 0.1f * brightnessModifier, 1.0f * brightnessModifier); // More blue
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 20.0f * brightnessModifier);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.5f * brightnessModifier);
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
	//LoadSceneTextures();
	DefineObjectMaterials();
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


	// Load the wood texture
	bool bReturn = CreateGLTexture("textures/rusticwood.jpg", "table");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: rusticwood.jpg" << std::endl;
	}

	// Load the beer body texture
	bReturn = CreateGLTexture("textures/amber.jpg", "beerBody");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: amber.jpg" << std::endl;
	}

	bReturn = CreateGLTexture("textures/glass.jpg", "clearglass");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: glass.jpg" << std::endl;
	}

	bReturn = CreateGLTexture("textures/foam2.jpg", "beerFoam");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: foam.jpg" << std::endl;
	}

	bReturn = CreateGLTexture("textures/insideLemon.jpg", "inLemon");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: insideLemon.jpg" << std::endl;
	}

	bReturn = CreateGLTexture("textures/lemonSkin2.jpg", "outLemon");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: lemonSkin2.jpg" << std::endl;
	}


	bReturn = CreateGLTexture("textures/bubbles.png", "bubbles");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: bubbles.png" << std::endl;
	}

	bReturn = CreateGLTexture("textures/field.jpg", "backdrop");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: field.jpg" << std::endl;
	}

	bReturn = CreateGLTexture("textures/bottleglass.jpg", "bottleglass");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: bottleglass.jpg" << std::endl;
	}

	bReturn = CreateGLTexture("textures/stainless.jpg", "stainless");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: stainless.jpg" << std::endl;
	
	}

	bReturn = CreateGLTexture("textures/knife_handle.jpg", "knifeHandle");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: knife_handle.jpg" << std::endl;

	}

	bReturn = CreateGLTexture("textures/stainless_end.jpg", "metalScrew");
	if (!bReturn)
	{
		std::cerr << "Failed to load texture: stainless_end.jpg" << std::endl;

	}

	// Bind the textures
	BindGLTextures();
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
	RenderBeerGlass();
	RenderBeerBottle();
	RenderPlate();
	RenderLemon();
	RenderKnife();
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

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 2.0f, 30.0f); // updated y to 2.0

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.8f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Use wood texture instead of solid color
	SetShaderTexture("table");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");  // set shader for wood

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();  // This will be a box mesh in the final project
	glDisable(GL_BLEND);
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
	scaleXYZ = glm::vec3(40.0f, 2.0f, 25.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 5.0f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("backdrop");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("backdrop");

	// draw the mesh with transformation values - this plane is used for the backdrop
	m_basicMeshes->DrawPlaneMesh();
}


/***********************************************************
 *  RenderBeerGlass()
 *
 *  This method is called to render the shapes for the beer
 *  glass including the base, body, head, and lemon slices.
 ***********************************************************/
void SceneManager::RenderBeerGlass()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render Beer Glass Base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 0.5625f, 1.5f); // Scaled down by 0.75x

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.25f, 0.0f); // Scaled down by 0.75x

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// set the color for the glass
	SetShaderColor(0.8f, 0.9f, 1.0f, 0.5f); // Glass color (blue)
	SetShaderMaterial("glass"); // set shader for glass

	// draw the base cylinder mesh
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Render Beer Glass Body
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 5.625f, 1.5f); // Scaled down by 0.75x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;  // rotate so bottom portion is smaller than top portion

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 6.25f, 0.0f); // Scaled down by 0.75x

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Use beer body texture
	SetShaderTexture("beerBody");
	SetTextureUVScale(1.0f, 1.0f);

	// draw the body tapered cylinder mesh
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Overlay bubbles texture on top of beer body
	SetShaderTexture("bubbles");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("beer"); // set shader for middle body of beer

	// draw the body tapered cylinder mesh again with bubbles texture
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Render Beer Head
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 1.2f, 1.5f); // Scaled down by 0.75x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 6.25f, 0.0f); // Scaled down by 0.75x

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Use beer foam texture
	SetShaderTexture("beerFoam");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("foam"); // set shader for beer foam

	// draw the head cylinder mesh
	m_basicMeshes->DrawCylinderMesh();

	// Render Inner Lemon
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 0.15f, 0.75f); // Scaled down by 0.75x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;  // Rotate so the slice is vertical (flat end out, peel down)

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5f, 7.375f, 0.0f); // Scaled down by 0.75x

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Use inside lemon texture
	SetShaderTexture("inLemon");
	//SetTextureUVScale(1.0f, 1.0f);
	//SetShaderMaterial("lemon"); // set lemon shader

	// draw the lemon cylinder mesh
	m_basicMeshes->DrawCylinderMesh();

	// Render Outer Lemon
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8625f, 0.14925f, 0.8625f); // Scaled down by 0.75x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;  // Rotate so the slice is vertical (flat end out, peel down)

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5f, 7.375f, 0.0f); // Scaled down by 0.75x

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Use lemon skin texture
	SetShaderTexture("outLemon");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("lemon"); // set lemon shader

	// draw the lemon cylinder mesh
	m_basicMeshes->DrawCylinderMesh();

	// Disable blending after drawing
	glDisable(GL_BLEND);
}


/***********************************************************
 *  RenderBeerBottle()
 *
 *  This method is called to render the shapes for the beer
 *  bottle object.
 ***********************************************************/
void SceneManager::RenderBeerBottle()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the bottom half-sphere ***/

	// set the XYZ scale for the mesh (scaled up by 1.25x)
	scaleXYZ = glm::vec3(1.125f, 0.5625f, 1.125f); // Scaled up by 1.25x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 0.2f, -2.0f); // Adjusted for scale

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("bottleglass");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this half-sphere is used for the bottom of the bottle
	m_basicMeshes->DrawHalfSphereMesh();

	/*** Set needed transformations before drawing the main cylinder body ***/

	// set the XYZ scale for the mesh (scaled up by 1.25x and lengthened a bit)
	scaleXYZ = glm::vec3(1.125f, 4.375f, 1.125f); // Scaled up by 1.25x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 0.2f, -2.0f); // Adjusted for scale

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("bottleglass");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this cylinder is used for the main body of the bottle
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/*** Set needed transformations before drawing the top half-sphere ***/

	// set the XYZ scale for the mesh (scaled up by 1.25x)
	scaleXYZ = glm::vec3(1.1375f, 1.125f, 1.1375f); // Scaled up by 1.25x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -6.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 4.5375f, -2.0f); // Adjusted for scale and moved down by 0.625f

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("bottleglass");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this half-sphere is used for the top of the bottle
	m_basicMeshes->DrawHalfSphereMesh();

	/*** Set needed transformations before drawing the neck cylinder ***/

	// set the XYZ scale for the mesh (scaled up by 1.25x and lengthened)
	scaleXYZ = glm::vec3(0.5625f, 3.75f, 0.5625f); // Scaled up by 1.25x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 5.3375f, -2.0f); // Adjusted for scale and moved down by 0.625f

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("bottleglass");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this cylinder is used for the neck
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	/*** Set needed transformations before drawing the brass bottle cap ***/

	// set the XYZ scale for the mesh (scaled up by 1.25x)
	scaleXYZ = glm::vec3(0.6f, 0.225f, 0.6f); // Scaled up by 1.25x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 8.875f, -2.0f); // Adjusted for scale and moved down by 0.625f

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8f, 0.5f, 0.2f, 1.0f); // Brass color for bottle cap
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this cylinder is used for the bottle cap
	m_basicMeshes->DrawCylinderMesh();

	/*** Set needed transformations before drawing the torus at the neck ***/

	// set the XYZ scale for the mesh (scaled up by 1.25x)
	scaleXYZ = glm::vec3(0.525f, 0.525f, 0.75f); // Scaled up by 1.25x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.5f, 8.7125f, -2.0f); // Adjusted for scale and moved down by 0.625f

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("bottleglass");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this torus is used for the neck ring
	m_basicMeshes->DrawTorusMesh();
}


/***********************************************************
 *  RenderPlate()
 *
 *  This method is called to render the shapes for the plate object.
 ***********************************************************/
void SceneManager::RenderPlate()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                        ***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.92f, 0.16f, 0.92f); // Increased by 2x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 0.2f, 1.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1.0);
	SetShaderMaterial("plate");

	// draw the mesh with transformation values - this cylinder is used for the base of the plate
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.                        ***/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.12f, 0.2f, 2.12f); // Increased by 2x

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.7f, 0.55f, 1.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1.0);
	SetShaderMaterial("plate");

	// draw the mesh with transformation values - this half-sphere is used for the top of the plate
	m_basicMeshes->DrawHalfSphereMesh();
}

/***********************************************************
 *  RenderLemon()
 *
 *  This method is called to render the shapes for the lemon object.
 ***********************************************************/
void SceneManager::RenderLemon()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Render the first lemon
	scaleXYZ = glm::vec3(0.95f, 0.75f, 0.95f);
	positionXYZ = glm::vec3(-3.7f, 1.1f, 1.3f); // Adjusted position to sit on the plate
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("outLemon");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("lemon");
	m_basicMeshes->DrawSphereMesh();

	// Render the second lemon
	scaleXYZ = glm::vec3(0.85f, 0.75f, 0.75f);
	positionXYZ = glm::vec3(-1.9f, 1.1f, 1.4f); // Adjusted position to sit next to the first lemon
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawSphereMesh();
	
	
	// Render the first lemon slice (inner and outer)
	scaleXYZ = glm::vec3(0.70f, 0.15f, 0.70f);
	XrotationDegrees = 0.0f; // Rotate to lay flat on the plate
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.0f, 0.5f, 2.9f); // Slightly in front of the first lemon
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("inLemon");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("lemon");
	m_basicMeshes->DrawCylinderMesh();
	scaleXYZ = glm::vec3(0.8f, 0.14925f, 0.8f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("outLemon");
	m_basicMeshes->DrawCylinderMesh();

	// Render the second lemon slice (inner and outer)
	scaleXYZ = glm::vec3(0.72f, 0.15f, 0.72f);
	positionXYZ = glm::vec3(-3.2f, 0.65f, 2.9f); // Slightly in front and above the first slice
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("inLemon");
	m_basicMeshes->DrawCylinderMesh();
	scaleXYZ = glm::vec3(0.8f, 0.14925f, 0.8f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("outLemon");
	m_basicMeshes->DrawCylinderMesh();

	// Render the third lemon slice (inner and outer)
	scaleXYZ = glm::vec3(0.70f, 0.15f, 0.70f);
	positionXYZ = glm::vec3(-2.8f, 0.8f, 2.8f); // Slightly in front and above the second slice
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("inLemon");
	m_basicMeshes->DrawCylinderMesh();
	scaleXYZ = glm::vec3(0.8f, 0.14925f, 0.8f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("outLemon");
	m_basicMeshes->DrawCylinderMesh();

}

/***********************************************************
 *  RenderKnife()
 *
 *  This method is called to render the shapes for the knife object.
 ***********************************************************/
void SceneManager::RenderKnife()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Render the knife handle
	scaleXYZ = glm::vec3(1.0f, 0.18f, 0.20f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 20.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.19f, 2.8f); // Adjusted position forward and to the right
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("knifeHandle");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();

	// Render the knife blade
	scaleXYZ = glm::vec3(0.3f, 2.0f, 0.02f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 110.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.5f, 0.30f, 2.25f); // Adjusted position forward and to the right
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("stainless");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metal");
	m_basicMeshes->DrawPyramid4Mesh();


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.186f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 0.2f, 2.625f);
	
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("metalScrew");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawCylinderMesh(true, true, false);
	/******************************************************************/
}