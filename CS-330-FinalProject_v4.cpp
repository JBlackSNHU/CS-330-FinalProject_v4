#include <iostream>
#include <cstdlib>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"
#include "shader.h"



using namespace std;

namespace
{
	const char* const WINDOW_TITLE = "JBLACK - Final Project - GUNDAM";
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    struct GLMesh {
        GLuint vao;
        GLuint vbo;
        GLuint nVertices;
    };

    GLFWwindow* gWindow = nullptr;

	// Lighting
	// setting the light pretty far away, but I'm not using attenuation so it shouldn't matter
	glm::vec3 lightPosition(-15.0f, 20.0f, 15.0f);	

    // Camera
    Camera camera(glm::vec3(0.0f, 4.0f, 20.0f));
    float gLastX = WINDOW_WIDTH / 2.0F;
    float gLastY = WINDOW_HEIGHT / 2.0F;
    bool gFirstMouse = true;

    // Timing
    float gDeltaTime = 0.0f;
    float gLastFrame = 0.0f;
    
    // Meshes
	GLMesh mPlane;
	GLMesh mLight;
	GLMesh mFrontHedge;
	GLMesh mLeftFoot;
	GLMesh mRightFoot;
	GLMesh mLeftLeg;
	GLMesh mRightLeg;
	GLMesh mTorso;
	GLMesh mLeftArm;
	GLMesh mRightArm;
	GLMesh mHead;
	GLMesh mLeftHedge;
	GLMesh mTree;
	GLMesh mTrailer;

	// Ortho boolean
	bool orthographic = false;
}

// ---------------------------------------------------------------------
// Function prototypes
// ---------------------------------------------------------------------

// MESH CONSTRUCTORS
void MeshConstructor();
void CreatePlane(GLMesh& mesh);
void UCreateLight(GLMesh& mesh);
void CreateFrontHedge(GLMesh& mesh);
void CreateLeftFoot(GLMesh& mesh);
void CreateRightFoot(GLMesh& mesh);
void CreateLeftLeg(GLMesh& mesh);
void CreateRightLeg(GLMesh& mesh);
void CreateTorso(GLMesh& mesh);
void CreateLeftArm(GLMesh& mesh);
void CreateRightArm(GLMesh& mesh);
void CreateHead(GLMesh& mesh);
void CreateLeftHedge(GLMesh& mesh);
void CreateTrailer(GLMesh& mesh);
// STANDARD FUNCTIONS
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
// CLEAN UP FUNCTIONS
void UDestroyTexture(GLuint textureId);
void UDestroyMesh(GLMesh& mesh);
// INPUT FUNCTIONS
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

// Images load Y axis going down, OpenGL goes up. This flips the image.
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; j++)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;
		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}

// Borrowed from LearnOpenGL
// utility function for loading a 2D texture from file
// ---------------------------------------------------------------------
unsigned int loadTexture(char const* path)
{
	unsigned int textureId;
	glGenTextures(1, &textureId);
	int width, height, channels;
	unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
	if (data)
	{
		GLenum format;
		if (channels == 1)
			format = GL_RED;
		else if (channels == 3)
			format = GL_RGB;
		else if (channels == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to loat at path: " << path << std::endl;
		stbi_image_free(data);
	}
	return textureId;
}

int main(int argc, char* argv[])
{
    // Initialize the Window
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the Shader Program
	Shader objectShader("objectVertexShader.vs", "objectFragmentShader.fs");
	Shader lightShader("lampVertexShader.vs", "lampFragmentShader.fs");

	const char* imgPavement = "Images/pavement.jpg";
	const char* imgSteel = "Images/steel.jpg";
	const char* imgHedge = "Images/OpenfootageNETgreen.jpg";
	const char* imgGray = "Images/plastic.jpg";
	
	unsigned int texPavement = loadTexture(imgPavement);
	unsigned int texSteel = loadTexture(imgSteel);
	unsigned int texHedge = loadTexture(imgHedge);
	unsigned int texGray = loadTexture(imgGray);


	objectShader.use();
	objectShader.setInt("material.diffuse", 0);		// setting the int that the diffuse map will bind the texture to
	objectShader.setInt("material.specular", 1);	// setting the int that the specular map will bind the texture to

	// Method to instantiate all meshes in one area for readability.
	// Prevents clutter in the main function
	MeshConstructor();

	while (!glfwWindowShouldClose(gWindow))
	{
		// per-frame timing
		// -----------------
		double currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// Input
		// -----
		UProcessInput(gWindow);

		// Rendering
		// Enable depth-test
		glEnable(GL_DEPTH_TEST);

		// Clear the frame and z buffers
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Shader configuration
		objectShader.use();
		objectShader.setVec3("light.position", lightPosition);
		
		// Set the camera view position
		objectShader.setVec3("viewPos", camera.Position);

		// Light Properties
		objectShader.setVec3("light.ambient", 0.1f, 0.1f, 0.1f);
		objectShader.setVec3("light.diffuse", 0.7f, 0.7f, 0.7f);
		objectShader.setVec3("light.specular", 0.8f, 0.8f, 0.8f);

		// Material Properties
		// Pavement is not shiny, set pretty low.
		objectShader.setFloat("material.shininess", 1.0f);

		// view/projection transformations
		glm::mat4 projection;
		if (orthographic)
		{
			float scale = 50;
			projection = glm::ortho(-((float)WINDOW_WIDTH / scale), ((float)WINDOW_WIDTH / scale), -((float)WINDOW_HEIGHT / scale), ((float)WINDOW_HEIGHT / scale), 20.0f, -20.0f);
		}
		else
		{
			projection = glm::perspective(glm::radians(camera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1F, 100.0F);
		}
		glm::mat4 view = camera.GetViewMatrix();
		objectShader.setMat4("projection", projection);
		objectShader.setMat4("view", view);

		glm::mat4 model = glm::mat4(1.0f);
		objectShader.setMat4("model", model);
	
		// --------------------
		// PAVEMENT PLANE
		// --------------------
		// bind diffuse map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texPavement);
		// bind specular map
		/* JBLACK - 
		 * Not particularly needed as I'm using the same image, for
		 * both the specular and diffuse maps, but I didn't want to mess
		 * with my shaders any further. */
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texPavement);
		
		glBindVertexArray(mPlane.vao);
		glDrawArrays(GL_TRIANGLES, 0, mPlane.nVertices);

		// --------------------
		// FRONT HEDGE
		// --------------------
		// Bind Hedge Texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texHedge);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texHedge);

		// Front Hedge
		glBindVertexArray(mFrontHedge.vao);
		glDrawArrays(GL_TRIANGLES, 0, mFrontHedge.nVertices);

		//Left Hedge
		glBindVertexArray(mLeftHedge.vao);
		glDrawArrays(GL_TRIANGLES, 0, mLeftHedge.nVertices);

		//Draw the Trailer
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texGray);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texGray);

		glBindVertexArray(mTrailer.vao);
		glDrawArrays(GL_TRIANGLES, 0, mTrailer.nVertices);


		// --------------------
		// GUNDAM PARTS
		// --------------------

		// shiny gundam
		objectShader.setFloat("material.shininess", 64.0f);

		// Set texture to steel texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texSteel);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texSteel);

		// LEFT FOOT
		glBindVertexArray(mLeftFoot.vao);
		glDrawArrays(GL_TRIANGLES, 0, mLeftFoot.nVertices);

		// RIGHT FOOT
		glBindVertexArray(mRightFoot.vao);
		glDrawArrays(GL_TRIANGLES, 0, mRightFoot.nVertices);

		// LEFT LEG
		glBindVertexArray(mLeftLeg.vao);
		glDrawArrays(GL_TRIANGLES, 0, mLeftLeg.nVertices);

		// RIGHT LEG
		glBindVertexArray(mRightLeg.vao);
		glDrawArrays(GL_TRIANGLES, 0, mRightLeg.nVertices);

		// TORSO
		glBindVertexArray(mTorso.vao);
		glDrawArrays(GL_TRIANGLES, 0, mTorso.nVertices);
		
		// LEFT ARM
		glBindVertexArray(mLeftArm.vao);
		glDrawArrays(GL_TRIANGLES, 0, mLeftArm.nVertices);
		
		// RIGHT ARM
		glBindVertexArray(mRightArm.vao);
		glDrawArrays(GL_TRIANGLES, 0, mRightArm.nVertices);

		// HEAD
		glBindVertexArray(mHead.vao);
		glDrawArrays(GL_TRIANGLES, 0, mHead.nVertices);

		// --------------------
		// LIGHT OBJECT
		// --------------------
		// Draw light object
		lightShader.use();
		lightShader.setMat4("projection", projection);
		lightShader.setMat4("view", view);
		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPosition);
		model = glm::scale(model, glm::vec3(1.2f));
		lightShader.setMat4("model", model);

		glBindVertexArray(mLight.vao);
		glDrawArrays(GL_TRIANGLES, 0, mLight.nVertices);

		// Swap buffer
		glfwSwapBuffers(gWindow);

		// Poll IO events
		glfwPollEvents();
	}

}

// ---------------------------------------------------------
// MESH CONSTRUCTORS
// ---------------------------------------------------------
void MeshConstructor()
{
	CreatePlane(mPlane);
	UCreateLight(mLight);
	CreateFrontHedge(mFrontHedge);
	CreateLeftFoot(mLeftFoot);
	CreateRightFoot(mRightFoot);
	CreateLeftLeg(mLeftLeg);
	CreateRightLeg(mRightLeg);
	CreateTorso(mTorso);
	CreateLeftArm(mLeftArm);
	CreateRightArm(mRightArm);
	CreateHead(mHead);
	CreateLeftHedge(mLeftHedge);
	CreateTrailer(mTrailer);

}

void CreatePlane(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------
		 10.0f, 0.0f, -10.0f,	0.0f, 1.0f, 0.0f,	1.0f, 1.0f,
		 10.0f, 0.0f,  10.0f,	0.0f, 1.0f, 0.0f,	1.0f, 0.0f,
		-10.0f, 0.0f,  10.0f,	0.0f, 1.0f, 0.0f,	0.0f, 0.0f,
		 10.0f, 0.0f, -10.0f,   0.0f, 1.0f, 0.0f,	1.0f, 1.0f,
		-10.0f, 0.0f, -10.0f,	0.0f, 1.0f, 0.0f,	0.0f, 1.0f,
		-10.0f, 0.0f,  10.0f,	0.0f, 1.0f, 0.0f,	0.0f, 0.0f,
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	// Create VBO
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	// Strides between vertex coordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	// Create Vertex Attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void UCreateLight(GLMesh& mesh)
{
	// Position and Color data
	GLfloat verts[] = {
		//Positions          //Normals
		// ------------------------------------------------------
		//Back Face          //Negative Z Normal  Texture Coords.
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

	   //Front Face         //Positive Z Normal
	  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
	   0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
	   0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
	   0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
	  -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
	  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

	  //Left Face          //Negative X Normal
	 -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	 -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	 -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	 -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	 //Right Face         //Positive X Normal
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	 //Bottom Face        //Negative Y Normal
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

	//Top Face           //Positive Y Normal
   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
	0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
	0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
	0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
   -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateFrontHedge(GLMesh& mesh)
{

	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------		
		// Front Face
		 2.5f, 0.0f, 4.0f,		 0.0f,  0.0f,  1.0f,	1.0f, 0.0f,
		-4.5f, 0.0f, 4.0f,		 0.0f,  0.0f,  1.0f,	0.0f, 0.0f,
		-4.5f, 1.0f, 4.0f,		 0.0f,  0.0f,  1.0f,	0.0f, 0.4f,
		 2.5f, 0.0f, 4.0f,		 0.0f,  0.0f,  1.0f,	1.0f, 0.0f,
		 2.5f, 1.0f, 4.0f,		 0.0f,  0.0f,  1.0f,	1.0f, 0.4f,
		-4.5f, 1.0f, 4.0f,		 0.0f,  0.0f,  1.0f,	0.0f, 0.4f,

		// rear face
		 2.5f, 0.0f, 3.5f,		 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
		-4.5f, 0.0f, 3.5f,		 0.0f,  0.0f, -1.0f,	0.0f, 1.0f,
		-4.5f, 1.0f, 3.5f,		 0.0f,  0.0f, -1.0f,	0.0f, 0.6f,
		 2.5f, 0.0f, 3.5f,		 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
		 2.5f, 1.0f, 3.5f,		 0.0f,  0.0f, -1.0f,	1.0f, 0.6f,
		-4.5f, 1.0f, 3.5f,		 0.0f,  0.0f, -1.0f,	0.0f, 0.6f,

		// top face
		 2.5f, 1.0f, 4.0f,		 0.0f,  1.0f,  0.0f,	1.0f, 0.4f,
	    -4.5f, 1.0f, 4.0f,		 0.0f,  1.0f,  0.0f,    0.0f, 0.4f,
	    -4.5f, 1.0f, 3.5f,		 0.0f,  1.0f,  0.0f,    0.0f, 0.6f,
	 	 2.5f, 1.0f, 4.0f,		 0.0f,  1.0f,  0.0f,    1.0f, 0.4f,
		 2.5f, 1.0f, 3.5f,		 0.0f,  1.0f,  0.0f,	1.0f, 0.6f,
	    -4.5f, 1.0f, 3.5f,		 0.0f,  1.0f,  0.0f,	0.0f, 0.0f,

	    // bottom face
	     2.5f, 0.0f, 4.0f,		 0.0f, -1.0f,  0.0f,    0.0f, 0.0f,
	    -4.5f, 0.0f, 4.0f,		 0.0f, -1.0f,  0.0f,    1.0f, 0.0f,
	    -4.5f, 0.0f, 3.5f,		 0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
	     2.5f, 0.0f, 4.0f,		 0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
	     2.5f, 0.0f, 3.5f,		 0.0f, -1.0f,  0.0f,    0.0f, 1.0f,
	    -4.5f, 0.0f, 3.5f, 		 0.0f, -1.0f,  0.0f,    0.0f, 0.0f,

	    // left face
	     2.5f, 0.0f, 4.0f, 		-1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
	     2.5f, 1.0f, 4.0f, 		-1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
	     2.5f, 1.0f, 3.5f, 		-1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
	     2.5f, 0.0f, 4.0f, 		-1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
	     2.5f, 1.0f, 3.5f, 		-1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
	     2.5f, 0.0f, 3.5f, 		-1.0f,  0.0f,  0.0f,    1.0f, 0.0f,

	    // right face
	    -4.5f, 0.0f, 4.0f, 		 1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
	    -4.5f, 1.0f, 4.0f, 		 1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
	    -4.5f, 1.0f, 3.5f, 		 1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
	    -4.5f, 0.0f, 4.0f, 		 1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
	    -4.5f, 1.0f, 3.5f, 		 1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
	    -4.5f, 0.0f, 3.5f, 		 1.0f,  0.0f,  0.0f,   1.0f, 0.0f
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateLeftFoot(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------
		// Bottom Face
		2.5f,  0.0f,  1.75f,	0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		1.75f, 0.0f,  1.75f,	0.0f, -1.0f, 0.0f,   1.0f, 0.0f,
		1.75f, 0.0f, -1.45f,	0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
		2.5f,  0.0f,  1.75f,	0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		1.75f, 0.0f, -1.45f,	0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
		2.5f,  0.0f, -1.45f,	0.0f, -1.0f, 0.0f,   0.0f, 1.0f,

		// front face
		2.5f,  0.0f, 1.75f,		0.0f, 1.0f, 1.0f,    1.0f, 0.0f,
		1.75f, 0.0f, 1.75f,		0.0f, 1.0f, 1.0f,    0.0f, 0.0f,
		2.4f,  1.5f, 0.0f,		0.0f, 1.0f, 1.0f,    1.0f, 1.0f,
		1.75f, 0.0f, 1.75f,		0.0f, 1.0f, 1.0f,    0.0f, 0.0f,
		2.4f,  1.5f, 0.0f,		0.0f, 1.0f, 1.0f,    1.0f, 1.0f,
		1.75f, 1.5f, 0.0f,		0.0f, 1.0f, 1.0f,    0.0f, 1.0f,

		// rear face
		2.5f,  0.0f,  -1.45f,	0.0f, 1.0f, -1.0f,   1.0f, 0.0f,
		1.75f, 0.0f,  -1.45f,	0.0f, 1.0f, -1.0f,   0.0f, 0.0f,
		2.4f,  1.5f,   0.0f,	0.0f, 1.0f, -1.0f,   1.0f, 1.0f,
		1.75f, 0.0f,  -1.45f,	0.0f, 1.0f, -1.0f,   0.0f, 0.0f,
		2.4f,  1.5f,   0.0f, 	0.0f, 1.0f, -1.0f,   1.0f, 1.0f,
		1.75f, 1.5f,   0.0f,	0.0f, 1.0f, -1.0f,   0.0f, 1.0f,

		// left face
		1.75f,  0.0f,  1.75f,  -1.0f, 0.0f, 0.0f,    0.0f, 0.0f,
		1.75f,  0.0f, -1.45f,  -1.0f, 0.0f, 0.0f,    1.0f, 0.0f,
		1.75f,  1.5f,  0.0f,   -1.0f, 0.0f, 0.0f,    0.5f, 1.0f,

		// right face
		2.5f,   0.0f,  1.75f,   1.0f, 0.0f, 0.0f,    0.0f, 0.0f,
		2.5f,   0.0f, -1.45f,   1.0f, 0.0f, 0.0f,    1.0f, 0.0f,
		2.4f,   1.5f,  0.0f,    1.0f, 0.0f, 0.0f,    0.5f, 1.0f
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateRightFoot(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------
		-2.5f,  0.0f,  1.75f,	0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		-1.75f, 0.0f,  1.75f,	0.0f, -1.0f, 0.0f,   1.0f, 0.0f,
		-1.75f, 0.0f, -1.45f,	0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
		-2.5f,  0.0f,  1.75f,	0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		-1.75f, 0.0f, -1.45f,	0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
		-2.5f,  0.0f, -1.45f,	0.0f, -1.0f, 0.0f,   0.0f, 1.0f,

		// front face
		-2.5f,  0.0f, 1.75f,	0.0f, 1.0f, 1.0f,   0.0f, 0.0f,
		-1.75f, 0.0f, 1.75f,	0.0f, 1.0f, 1.0f,   1.0f, 0.0f,
		-2.4f,  1.5f, 0.0f, 	0.0f, 1.0f, 1.0f,   0.0f, 1.0f,
		-1.75f, 0.0f, 1.75f,	0.0f, 1.0f, 1.0f,   1.0f, 0.0f,
		-2.4f,  1.5f, 0.0f, 	0.0f, 1.0f, 1.0f,   0.0f, 1.0f,
		-1.75f, 1.5f, 0.0f, 	0.0f, 1.0f, -1.0f,   1.0f, 1.0f,

		// rear face
		-2.5f,  0.0f, -1.45f,	0.0f, 1.0f, -1.0f,  0.0f, 0.0f,
		-1.75f, 0.0f, -1.45f,	0.0f, 1.0f, -1.0f,  1.0f, 0.0f,
		-2.4f,  1.5f,  0.0f, 	0.0f, 1.0f, -1.0f,  0.0f, 1.0f,
		-1.75f, 0.0f, -1.45f,	0.0f, 1.0f, -1.0f,  1.0f, 0.0f,
		-2.4f,  1.5f,  0.0f, 	0.0f, 1.0f, -1.0f,  0.0f, 1.0f,
		-1.75f, 1.5f,  0.0f, 	0.0f, 1.0f, -1.0f,  1.0f, 1.0f,

		// left face
		-1.75f,  0.0f,  1.75f,	-1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
		-1.75f,  0.0f, -1.45f,	-1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
		-1.75f,  1.5f,  0.0f, 	-1.0f, 0.0f, 0.0f,  0.5f, 1.0f,

		// right face
		-2.5f,   0.0f,  1.75f,	1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
		-2.5f,   0.0f, -1.45f,	1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		-2.4f,   1.5f,  0.0f, 	1.0f, -1.0f, 0.0f,  0.5f, 1.0f
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateLeftLeg(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------
		// Bottom Face
		1.75f, 0.75f, 0.9f,		-0.41, -1.53, 0.0f,  0.0f, 0.0f,
		1.75f, 0.75f, -1.15f,	-0.41, -1.53, 0.0f,   1.0f, 0.0f,
		2.5f,  0.95f, 0.9f,		-0.41, -1.53, 0.0f,   0.0f, 1.0f,
		1.75f, 0.75f, -1.15f,	-0.41, -1.53, 0.0f,   1.0f, 0.0f,
		2.5f, 0.95f, 0.9f,		-0.41, -1.53, 0.0f,   0.0f, 1.0f,
		2.5f, 0.95f, -1.15f,	-0.41, -1.53, 0.0f,   1.0f, 1.0f,

		// top face
		1.0f, 5.0f, 0.7f,   	0.0, 1.0f, 0.0f,   0.0f, 0.0f,
		1.0f, 5.0f, -1.05f, 	0.0, 1.0f, 0.0f,   1.0f, 0.0f,
		1.75f, 5.1f, 0.7f,  	0.0, 1.0f, 0.0f,   0.0f, 1.0f,
		1.0f, 5.0f, -1.05f, 	0.0, 1.0f, 0.0f,   1.0f, 0.0f,
		1.75f, 5.1f, -1.05f,	0.0, 1.0f, 0.0f,   0.0f, 1.0f,
		1.75f, 5.1f, 0.7f,  	0.0, 1.0f, 0.0f,   1.0f, 1.0f,

		// front face
		1.75f, 0.75f, 0.9f, 	0.0, 0.0f, 1.0f,   0.0f, 0.0f,
		1.0f, 5.0f, 0.7f,   	0.0, 0.0f, 1.0f,    1.0f, 0.0f,
		2.5f, 0.95f, 0.9f,  	0.0, 0.0f, 1.0f,    0.0f, 1.0f,
		1.0f, 5.0f, 0.7f,   	0.0, 0.0f, 1.0f,    1.0f, 0.0f,
		2.5f, 0.95f, 0.9f,  	0.0, 0.0f, 1.0f,    0.0f, 1.0f,
		1.75f, 5.1f, 0.7f,  	0.0, 0.0f, 1.0f,    1.0f, 1.0f,

		//rear face
		1.75f, 0.75f, -1.15f, 	0.0, 0.0f, -1.0f,  0.0f, 0.0f,
		1.0f, 5.0f, -1.05f,   	0.0, 0.0f, -1.0f,  1.0f, 0.0f,
		2.5f, 0.95f, -1.15f,  	0.0, 0.0f, -1.0f,  0.0f, 1.0f,
		1.0f, 5.0f, -1.05f,  	0.0, 0.0f, -1.0f,  1.0f, 0.0f,
		2.5f, 0.95f, -1.15f, 	0.0, 0.0f, -1.0f,  0.0f, 1.0f,
		1.75f, 5.1f, -1.05f,  	0.0, 0.0f, -1.0f,  1.0f, 1.0f,

		// left face
		1.75f, 0.75f, 0.9f,   	-1.0, 0.0f, 0.0f,  0.0f, 0.0f,
		1.75f, 0.75f, -1.15f, 	-1.0, 0.0f, 0.0f,  1.0f, 0.0f,
		1.0f, 5.0f, 0.7f,     	-1.0, 0.0f, 0.0f,  0.0f, 1.0f,
		1.75f, 0.75f, -1.15f, 	-1.0, 0.0f, 0.0f,  1.0f, 0.0f,
		1.0f, 5.0f, 0.7f,     	-1.0, 0.0f, 0.0f,  0.0f, 1.0f,
		1.0f, 5.0f, -1.05f,   	-1.0, 0.0f, 0.0f,  1.0f, 1.0f,

		// right face
		2.5f, 0.95f, 0.9f,    	1.0, 0.0f, 0.0f,   0.0f, 0.0f,
		2.5f, 0.95f, -1.15f,  	1.0, 0.0f, 0.0f,   1.0f, 0.0f,
		1.75f, 5.1f, 0.7f,    	1.0, 0.0f, 0.0f,   0.0f, 1.0f,
		2.5f, 0.95f, -1.15f,  	1.0, 0.0f, 0.0f,   1.0f, 0.0f,
		1.75f, 5.1f, 0.7f,    	1.0, 0.0f, 0.0f,   0.0f, 1.0f,
		1.75f, 5.1f, -1.05f,  	1.0, 0.0f, 0.0f,   1.0f, 1.0f,
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateRightLeg(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------
		// Bottom Face
		-1.75f, 0.75f, 0.9f,	0.41, -1.53, 0.0f,   0.0f, 0.0f,
		-1.75f, 0.75f, -1.15f,	0.41, -1.53, 0.0f,   1.0f, 0.0f,
		-2.5f,  0.95f, 0.9f,  	0.41, -1.53, 0.0f,   0.0f, 1.0f,
		-1.75f, 0.75f, -1.15f,	0.41, -1.53, 0.0f,   1.0f, 0.0f,
		-2.5f, 0.95f, 0.9f,   	0.41, -1.53, 0.0f,   0.0f, 1.0f,
		-2.5f, 0.95f, -1.15f,	0.41, -1.53, 0.0f,   1.0f, 1.0f,

		// top face
		-1.0f, 5.0f, 0.7f,		0.0, 1.0f, 0.0f,    0.0f, 0.0f,
		-1.0f, 5.0f, -1.05f,	0.0, 1.0f, 0.0f,    1.0f, 0.0f,
		-1.75f, 5.1f, 0.7f, 	0.0, 1.0f, 0.0f,    0.0f, 1.0f,
		-1.0f, 5.0f, -1.05f,	0.0, 1.0f, 0.0f,    1.0f, 0.0f,
		-1.75f, 5.1f, -1.05f,	0.0, 1.0f, 0.0f,    0.0f, 1.0f,
		-1.75f, 5.1f, 0.7f,  	0.0, 1.0f, 0.0f,    1.0f, 1.0f,

		// front face
		-1.75f, 0.75f, 0.9f,	0.0, 0.0f, 1.0f,    0.0f, 0.0f,
		-1.0f, 5.0f, 0.7f,  	0.0, 0.0f, 1.0f,    0.0f, 1.0f,
		-2.5f, 0.95f, 0.9f, 	0.0, 0.0f, 1.0f,    1.0f, 0.0f,
		-1.0f, 5.0f, 0.7f,  	0.0, 0.0f, 1.0f,    0.0f, 1.0f,
		-2.5f, 0.95f, 0.9f,		0.0, 0.0f, 1.0f,     1.0f, 0.0f,
		-1.75f, 5.1f, 0.7f, 	0.0, 0.0f, 1.0f,    1.0f, 1.0f,

		//rear face
		-1.75f, 0.75f, -1.15f,	0.0, 0.0f, -1.0f,  0.0f, 0.0f,
		-1.0f, 5.0f, -1.05f,  	0.0, 0.0f, -1.0f,  0.0f, 1.0f,
		-2.5f, 0.95f, -1.15f,	0.0, 0.0f, -1.0f,   1.0f, 0.0f,
		-1.0f, 5.0f, -1.05f, 	0.0, 0.0f, -1.0f,   0.0f, 1.0f,
		-2.5f, 0.95f, -1.15f, 	0.0, 0.0f, -1.0f,  1.0f, 0.0f,
		-1.75f, 5.1f, -1.05f,	0.0, 0.0f, -1.0f,   1.0f, 1.0f,

		// left face
		-1.75f, 0.75f, 0.9f,  	-1.0, 0.0f, 0.0f,  0.0f, 0.0f,
		-1.75f, 0.75f, -1.15f,	-1.0, 0.0f, 0.0f,  0.0f, 1.0f,
		-1.0f, 5.0f, 0.7f,    	-1.0, 0.0f, 0.0f,  1.0f, 0.0f,
		-1.75f, 0.75f, -1.15f,	-1.0, 0.0f, 0.0f,  0.0f, 1.0f,
		-1.0f, 5.0f, 0.7f,    	-1.0, 0.0f, 0.0f,  1.0f, 0.0f,
		-1.0f, 5.0f, -1.05f,  	-1.0, 0.0f, 0.0f,  1.0f, 1.0f,

		// right face
		-2.5f, 0.95f, 0.9f,   	1.0, 0.0f, 0.0f,   0.0f, 0.0f,
		-2.5f, 0.95f, -1.15f, 	1.0, 0.0f, 0.0f,   0.0f, 1.0f,
		-1.75f, 5.1f, 0.7f,   	1.0, 0.0f, 0.0f,   1.0f, 0.0f,
		-2.5f, 0.95f, -1.15f, 	1.0, 0.0f, 0.0f,   0.0f, 1.0f,
		-1.75f, 5.1f, 0.7f,   	1.0, 0.0f, 0.0f,   1.0f, 0.0f,
		-1.75f, 5.1f, -1.05f, 	1.0, 0.0f, 0.0f,   1.0f, 1.0f,
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateTorso(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------
		// Bottom Face
		1.75f, 4.8f, 1.0f,		0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
	   -1.75f, 4.8f, 1.0f,  	0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
	   -1.75f, 4.8f, -1.25f,	0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
		1.75f, 4.8f, 1.0f,  	0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		1.75f, 4.8f, -1.25f,	0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
	   -1.75f, 4.8f, -1.25f,	0.0f, -1.0f, 0.0f,  1.0f, 1.0f,

	   // top face
	   1.75f, 9.5f, 1.0f,   	0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
	  -1.75f, 9.5f, 1.0f,   	0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
	  -1.75f, 9.5f, -1.25f, 	0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
	   1.75f, 9.5f, 1.0f,   	0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
	   1.75f, 9.5f, -1.25f, 	0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
	  -1.75f, 9.5f, -1.25f, 	0.0f, 1.0f, 0.0f,   1.0f, 1.0f,

	  // front face
	  1.75f, 4.8f, 1.0f,   		0.0f, 0.0f, 1.0f,    0.0f, 0.0f,
	  1.75f, 9.5f, 1.0f,   		0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
	  -1.75f,9.5f, 1.0f,   		0.0f, 0.0f, 1.0f,    0.0f, 1.0f,
	  1.75f, 4.8f, 1.0f,   		0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
	  -1.75f, 4.8f, 1.0f,  		0.0f, 0.0f, 1.0f,    0.0f, 1.0f,
	  -1.75f, 9.5f, 1.0f,  		0.0f, 0.0f, 1.0f,    1.0f, 1.0f,

	  // back face
	  1.75f, 4.8f, -1.25f,  	0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
	  1.75f, 9.5f, -1.25f,  	0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
	  -1.75f,9.5f, -1.25f,  	0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
	  1.75f, 4.8f, -1.25f,  	0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
	  -1.75f, 4.8f, -1.25f, 	0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
	  -1.75f, 9.5f, -1.25f, 	0.0f, 0.0f, -1.0f,   1.0f, 1.0f,

	  // left face
	  1.75f, 4.8f, 1.0f,   		-1.0f, 0.0f, 0.0f,    0.0f, 0.0f,
	  1.75f, 4.8f, -1.25f, 		-1.0f, 0.0f, 0.0f,    1.0f, 0.0f,
	  1.75f, 9.5f, -1.25f, 		-1.0f, 0.0f, 0.0f,    0.0f, 1.0f,
	  1.75f, 4.8f, 1.0f,   		-1.0f, 0.0f, 0.0f,    1.0f, 0.0f,
	  1.75f, 9.5f, 1.0f,   		-1.0f, 0.0f, 0.0f,    0.0f, 1.0f,
	  1.75f, 9.5f, -1.25f, 		-1.0f, 0.0f, 0.0f,    1.0f, 1.0f,

	  // right face
	  -1.75f, 4.8f, 1.0f,   	1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
	  -1.75f, 4.8f, -1.25f, 	1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
	  -1.75f, 9.5f, -1.25f, 	1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
	  -1.75f, 4.8f, 1.0f,   	1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
	  -1.75f, 9.5f, 1.0f,    	1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
	  -1.75f, 9.5f, -1.25f, 	1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateLeftArm(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------
		// Bottom Face			// neg y normal
		-3.0f,  5.0f, 0.7f,		0.0f, -1.0f, 0.0f,	0.0f, 0.0f,
		-1.75f, 5.0f, 0.7f,		0.0f, -1.0f, 0.0f,	1.0f, 0.0f,
		-1.75f, 5.2f, 1.9f,		0.0f, -1.0f, 0.0f,	1.0f, 1.0f,
		-1.75f, 5.2f, 1.9f,		0.0f, -1.0f, 0.0f,	1.0f, 1.0f,
		-3.0f,  5.2f, 1.9f,		0.0f, -1.0f, 0.0f,	0.0f, 1.0f,
		-3.0f,  5.0f, 0.7f,		0.0f, -1.0f, 0.0f,	0.0f, 0.0f,

		// Top Face				// pos y normal
		-1.75f, 9.8f,  0.7f,	0.0f,  1.0f, 0.0f,	0.0f, 0.0f,
		-1.75f, 9.6f, -0.7f,	0.0f,  1.0f, 0.0f,	1.0f, 0.0f,
		-3.0f, 9.6f,  -0.7f,	0.0f,  1.0f, 0.0f,	1.0f, 1.0f,
		-3.0f, 9.6f,  -0.7f,	0.0f,  1.0f, 0.0f,	1.0f, 1.0f,
		-3.0f, 9.8f,   0.7f,	0.0f,  1.0f, 0.0f,	0.0f, 1.0f,
		-1.75f, 9.8f,  0.7f,	0.0f,  1.0f, 0.0f,	0.0f, 0.0f,

		// Left Face		
		-3.0f, 5.0f,  0.7f,	    1.0f,  0.0f, 0.0f,	0.0f, 0.0f,
		-3.0f, 5.2f,  1.9f,		1.0f,  0.0f, 0.0f,	1.0f, 0.0f,
		-3.0f, 9.8f,  0.7f,		1.0f,  0.0f, 0.0f,	1.0f, 1.0f,
		-3.0f, 9.8f,  0.7f,		1.0f,  0.0f, 0.0f,	1.0f, 1.0f,
		-3.0f, 9.6f, -0.7f,		1.0f,  0.0f, 0.0f,	0.0f, 1.0f,
		-3.0f, 5.0f,  0.7f,	    1.0f,  0.0f, 0.0f,	0.0f, 0.0f,
		
		// Right Face
		-1.75f, 5.0f,  0.7f,	    1.0f,  0.0f, 0.0f,	0.0f, 0.0f,
		-1.75f, 5.2f,  1.9f,		1.0f,  0.0f, 0.0f,	1.0f, 0.0f,
		-1.75f, 9.8f,  0.7f,		1.0f,  0.0f, 0.0f,	1.0f, 1.0f,
		-1.75f, 9.8f,  0.7f,		1.0f,  0.0f, 0.0f,	1.0f, 1.0f,
		-1.75f, 9.6f, -0.7f,		1.0f,  0.0f, 0.0f,	0.0f, 1.0f,
		-1.75f, 5.0f,  0.7f,	    1.0f,  0.0f, 0.0f,	0.0f, 0.0f,

		// Front Face
		-1.75f, 5.2f, 1.9f,		0.0f, 0.0f, 1.0f,	0.0f, 0.0f,
		-3.0f, 5.2f, 1.9f,		0.0f, 0.0f, 1.0f,	1.0f, 0.0f,
		-3.0f, 9.8f, 0.7f,		0.0f, 0.0f, 1.0f,	1.0f, 1.0f,
		-3.0f, 9.8f, 0.7f,		0.0f, 0.0f, 1.0f,	1.0f, 1.0f,
		-1.75f, 9.8f, 0.7f,		0.0f, 0.0f, 1.0f,	0.0f, 1.0f,
		-1.75f, 5.2f, 1.9f, 	0.0f, 0.0f, 1.0f,	0.0f, 0.0f,

		// Back Face
		-1.75f, 5.0f,  0.7f,	0.0f, 0.0f, -1.0f,	0.0f, 0.0f,
		-3.0f,  5.0f,  0.7f,	0.0f, 0.0f, -1.0f,	1.0f, 0.0f,
		-3.0f,  9.6f, -0.7f,	0.0f, 0.0f, -1.0f,	1.0f, 1.0f,
		-3.0f,  9.6f, -0.7f,	0.0f, 0.0f, -1.0f,	1.0f, 1.0f,
		-1.75f, 9.6f, -0.7f,	0.0f, 0.0f, -1.0f,	0.0f, 1.0f,
		-1.75f, 5.0f,  0.7f, 	0.0f, 0.0f, -1.0f,	0.0f, 0.0f,
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateRightArm(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------
		// Bottom Face			// neg y normal
		3.0f,  5.0f, 0.7f,		0.0f, -1.0f, 0.0f,	0.0f, 0.0f,
		1.75f, 5.0f, 0.7f,		0.0f, -1.0f, 0.0f,	1.0f, 0.0f,
		1.75f, 5.2f, 1.9f,		0.0f, -1.0f, 0.0f,	1.0f, 1.0f,
		1.75f, 5.2f, 1.9f,		0.0f, -1.0f, 0.0f,	1.0f, 1.0f,
		3.0f,  5.2f, 1.9f,		0.0f, -1.0f, 0.0f,	0.0f, 1.0f,
		3.0f,  5.0f, 0.7f,		0.0f, -1.0f, 0.0f,	0.0f, 0.0f,

		// Top Face				// pos y normal
		1.75f, 9.8f,  0.7f,		0.0f,  1.0f, 0.0f,	0.0f, 0.0f,
		1.75f, 9.6f, -0.7f,		0.0f,  1.0f, 0.0f,	1.0f, 0.0f,
		3.0f, 9.6f,  -0.7f,		0.0f,  1.0f, 0.0f,	1.0f, 1.0f,
		3.0f, 9.6f,  -0.7f,		0.0f,  1.0f, 0.0f,	1.0f, 1.0f,
		3.0f, 9.8f,   0.7f,		0.0f,  1.0f, 0.0f,	0.0f, 1.0f,
		1.75f, 9.8f,  0.7f,		0.0f,  1.0f, 0.0f,	0.0f, 0.0f,

		// Left Face		
		3.0f, 5.0f,  0.7f,	   -1.0f,  0.0f, 0.0f,	0.0f, 0.0f,
		3.0f, 5.2f,  1.9f,	   -1.0f,  0.0f, 0.0f,	1.0f, 0.0f,
		3.0f, 9.8f,  0.7f,	   -1.0f,  0.0f, 0.0f,	1.0f, 1.0f,
		3.0f, 9.8f,  0.7f,	   -1.0f,  0.0f, 0.0f,	1.0f, 1.0f,
		3.0f, 9.6f, -0.7f,	   -1.0f,  0.0f, 0.0f,	0.0f, 1.0f,
		3.0f, 5.0f,  0.7f,	   -1.0f,  0.0f, 0.0f,	0.0f, 0.0f,

		// Right Face
		1.75f, 5.0f,  0.7f,	    1.0f,  0.0f, 0.0f,	0.0f, 0.0f,
		1.75f, 5.2f,  1.9f,		1.0f,  0.0f, 0.0f,	1.0f, 0.0f,
		1.75f, 9.8f,  0.7f,		1.0f,  0.0f, 0.0f,	1.0f, 1.0f,
		1.75f, 9.8f,  0.7f,		1.0f,  0.0f, 0.0f,	1.0f, 1.0f,
		1.75f, 9.6f, -0.7f,		1.0f,  0.0f, 0.0f,	0.0f, 1.0f,
		1.75f, 5.0f,  0.7f,	    1.0f,  0.0f, 0.0f,	0.0f, 0.0f,

		// Front Face
		1.75f, 5.2f, 1.9f,		0.0f, 0.0f, 1.0f,	0.0f, 0.0f,
		3.0f, 5.2f, 1.9f,		0.0f, 0.0f, 1.0f,	1.0f, 0.0f,
		3.0f, 9.8f, 0.7f,		0.0f, 0.0f, 1.0f,	1.0f, 1.0f,
		3.0f, 9.8f, 0.7f,		0.0f, 0.0f, 1.0f,	1.0f, 1.0f,
		1.75f, 9.8f, 0.7f,		0.0f, 0.0f, 1.0f,	0.0f, 1.0f,
		1.75f, 5.2f, 1.9f, 		0.0f, 0.0f, 1.0f,	0.0f, 0.0f,

		// Back Face
		1.75f, 5.0f,  0.7f,		0.0f, 0.0f, -1.0f,	0.0f, 0.0f,
		3.0f,  5.0f,  0.7f,		0.0f, 0.0f, -1.0f,	1.0f, 0.0f,
		3.0f,  9.6f, -0.7f,		0.0f, 0.0f, -1.0f,	1.0f, 1.0f,
		3.0f,  9.6f, -0.7f,		0.0f, 0.0f, -1.0f,	1.0f, 1.0f,
		1.75f, 9.6f, -0.7f,		0.0f, 0.0f, -1.0f,	0.0f, 1.0f,
		1.75f, 5.0f,  0.7f, 	0.0f, 0.0f, -1.0f,	0.0f, 0.0f,
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateHead(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------
		// Bottom Face
		-0.7f, 9.5f,  0.8f,		0.0f, -1.0f, 0.0f,	0.0f, 0.0f,
		 0.7f, 9.5f,  0.8f,		0.0f, -1.0f, 0.0f,	1.0f, 0.0f,
		 0.7f, 9.5f, -0.7f,		0.0f, -1.0f, 0.0f,	1.0f, 1.0f,
		 0.7f, 9.5f, -0.7f,		0.0f, -1.0f, 0.0f,	1.0f, 1.0f,
		-0.7f, 9.5f, -0.7f,		0.0f, -1.0f, 0.0f,	0.0f, 1.0f,
		-0.7f, 9.5f,  0.8f,		0.0f, -1.0f, 0.0f,	0.0f, 0.0f,

		// Top Face
		-0.7f, 10.5f,  0.8f,	0.0f, 1.0f, 0.0f,	0.0f, 0.0f,
		 0.7f, 10.5f,  0.8f,	0.0f, 1.0f, 0.0f,	1.0f, 0.0f,
		 0.7f, 10.5f, -0.7f,	0.0f, 1.0f, 0.0f,	1.0f, 1.0f,
		 0.7f, 10.5f, -0.7f,	0.0f, 1.0f, 0.0f,	1.0f, 1.0f,
		-0.7f, 10.5f, -0.7f,	0.0f, 1.0f, 0.0f,	0.0f, 1.0f,
		-0.7f, 10.5f,  0.8f,	0.0f, 1.0f, 0.0f,	0.0f, 0.0f,

		// Front Face
		-0.7f, 9.5f,  0.8f,		0.0f, 0.0f, 1.0f,	0.0f, 0.0f,
		 0.7f, 9.5f,  0.8f,		0.0f, 0.0f, 1.0f,	1.0f, 0.0f,
		 0.7f, 10.5f, 0.8f,		0.0f, 0.0f, 1.0f,	1.0f, 1.0f,
		 0.7f, 10.5f, 0.8f,		0.0f, 0.0f, 1.0f,	1.0f, 1.0f,
		-0.7f, 10.5f, 0.8f,		0.0f, 0.0f, 1.0f,	0.0f, 0.0f,
		-0.7f, 9.5f,  0.8f,		0.0f, 0.0f, 1.0f,	0.0f, 0.0f,

		// Back Face
		-0.7f, 9.5f,  -0.7f,	0.0f, 0.0f, -1.0f,	0.0f, 0.0f,
		 0.7f, 9.5f,  -0.7f,	0.0f, 0.0f, -1.0f,	1.0f, 0.0f,
		 0.7f, 10.5f, -0.7f,	0.0f, 0.0f, -1.0f,	1.0f, 1.0f,
		 0.7f, 10.5f, -0.7f,	0.0f, 0.0f, -1.0f,	1.0f, 1.0f,
		-0.7f, 10.5f, -0.7f,	0.0f, 0.0f, -1.0f,	0.0f, 0.0f,
		-0.7f, 9.5f,  -0.7f,	0.0f, 0.0f, -1.0f,	0.0f, 0.0f,

		// Left Face
		-0.7f, 9.5f,  0.8f,		-1.0f, 0.0f, 0.0f,	0.0f, 0.0f,
		-0.7f, 9.5f, -0.7f,		-1.0f, 0.0f, 0.0f,	1.0f, 0.0f,
		-0.7f, 10.5f, -0.7f,	-1.0f, 0.0f, 0.0f,	1.0f, 1.0f,
		-0.7f, 10.5f, -0.7f,	-1.0f, 0.0f, 0.0f,	1.0f, 1.0f,
		-0.7f, 10.5f, 0.8f,		-1.0f, 0.0f, 0.0f,	0.0f, 1.0f,
		-0.7f, 9.5f,  0.8f,		-1.0f, 0.0f, 0.0f,	0.0f, 0.0f,

		// Right Face
		0.7f, 9.5f,  0.8f,		1.0f, 0.0f, 0.0f,	0.0f, 0.0f,
		0.7f, 9.5f, -0.7f,		1.0f, 0.0f, 0.0f,	1.0f, 0.0f,
		0.7f, 10.5f, -0.7f,		1.0f, 0.0f, 0.0f,	1.0f, 1.0f,
		0.7f, 10.5f, -0.7f,		1.0f, 0.0f, 0.0f,	1.0f, 1.0f,
		0.7f, 10.5f, 0.8f,		1.0f, 0.0f, 0.0f,	0.0f, 1.0f,
		0.7f, 9.5f,  0.8f,		1.0f, 0.0f, 0.0f,	0.0f, 0.0f,
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateLeftHedge(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------		
		// Front Face
		-4.5f, 0.0f,  2.0f,		 -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		-4.5f, 0.0f, -4.0f,		 -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		-4.5f, 1.0f, -4.0f,		 -1.0f,  0.0f,  0.0f,	0.0f, 0.4f,
		-4.5f, 0.0f,  2.0f,		 -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		-4.5f, 1.0f,  2.0f,		 -1.0f,  0.0f,  0.0f,	1.0f, 0.4f,
		-4.5f, 1.0f, -4.0f,		 -1.0f,  0.0f,  0.0f,	0.0f, 0.4f,

		// rear face
		-4.0f, 0.0f,  2.0f,		 1.0f,  0.0f, 0.0f,		1.0f, 1.0f,
		-4.0f, 0.0f, -4.0f,		 1.0f,  0.0f, 0.0f,		0.0f, 1.0f,
		-4.0f, 1.0f, -4.0f,		 1.0f,  0.0f, 0.0f,		0.0f, 0.6f,
		-4.0f, 0.0f,  2.0f,		 1.0f,  0.0f, 0.0f,		1.0f, 1.0f,
		-4.0f, 1.0f,  2.0f,		 1.0f,  0.0f, 0.0f,		1.0f, 0.6f,
		-4.0f, 1.0f, -4.0f,		 1.0f,  0.0f, 0.0f,		0.0f, 0.6f,

		// top face
		-4.5f, 1.0f,  2.0f,		 0.0f,  1.0f,  0.0f,	1.0f, 0.4f,
		-4.5f, 1.0f, -4.0f,		 0.0f,  1.0f,  0.0f,    0.0f, 0.4f,
		-4.0f, 1.0f, -4.0f,		 0.0f,  1.0f,  0.0f,    0.0f, 0.6f,
		-4.5f, 1.0f,  2.0f,		 0.0f,  1.0f,  0.0f,    1.0f, 0.4f,
		-4.0f, 1.0f,  2.0f,		 0.0f,  1.0f,  0.0f,	1.0f, 0.6f,
		-4.0f, 1.0f, -4.0f,		 0.0f,  1.0f,  0.0f,	0.0f, 0.0f,

		// bottom face
		-4.5f, 0.0f,  2.0f,		 0.0f, -1.0f,  0.0f,    0.0f, 0.0f,
		-4.5f, 0.0f, -4.0f,		 0.0f, -1.0f,  0.0f,    1.0f, 0.0f,
		-4.0f, 0.0f, -4.0f,		 0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
		-4.5f, 0.0f,  2.0f,		 0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
		-4.0f, 0.0f,  2.0f,		 0.0f, -1.0f,  0.0f,    0.0f, 1.0f,
		-4.0f, 0.0f, -4.0f, 	 0.0f, -1.0f,  0.0f,    0.0f, 0.0f,

		// left face
		-4.5f, 0.0f, 2.0f, 		-1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
		-4.5f, 1.0f, 2.0f, 		-1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
		-4.0f, 1.0f, 2.0f, 		-1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
		-4.5f, 0.0f, 2.0f, 		-1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
		-4.0f, 1.0f, 2.0f, 		-1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
		-4.0f, 0.0f, 2.0f, 		-1.0f,  0.0f,  0.0f,    1.0f, 0.0f,

		 // right face
		 -4.5f, 0.0f, -4.0f, 	 1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
		 -4.5f, 1.0f, -4.0f, 	 1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
		 -4.0f, 1.0f, -4.0f, 	 1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
		 -4.5f, 0.0f, -4.0f, 	 1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
		 -4.0f, 1.0f, -4.0f, 	 1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
		 -4.0f, 0.0f, -4.0f, 	 1.0f,  0.0f,  0.0f,   1.0f, 0.0f
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}

void CreateTrailer(GLMesh& mesh)
{
	GLfloat verts[] = {
		//Vectors				// Normals			// Texture Coords
		//------------------------------------------------------------		
		// Front Face
		 5.5f, 0.0f, -2.0f,		 -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		 4.5f, 0.0f, -6.5f,		 -1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
		 4.5f, 3.0f, -6.5f,		 -1.0f,  0.0f,  0.0f,	0.0f, 0.4f,
		 5.5f, 0.0f, -2.0f,		 -1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
		 5.5f, 3.0f, -2.0f,		 -1.0f,  0.0f,  0.0f,	1.0f, 0.4f,
		 4.5f, 3.0f, -6.5f,		 -1.0f,  0.0f,  0.0f,	0.0f, 0.4f,

		// rear face
		6.75f, 3.0f, -7.0f,		 1.0f,  0.0f, 0.0f,		0.0f, 0.6f,
		6.75f, 0.0f, -7.0f,		 1.0f,  0.0f, 0.0f,		0.0f, 1.0f,
		7.75f, 3.0f, -2.5f,		 1.0f,  0.0f, 0.0f,		1.0f, 0.6f,
		7.75f, 3.0f, -2.5f,		 1.0f,  0.0f, 0.0f,		1.0f, 0.6f,
		7.75f, 0.0f, -2.5f,		 1.0f,  0.0f, 0.0f,		1.0f, 1.0f,
		6.75f, 0.0f, -7.0f,		 1.0f,  0.0f, 0.0f,		0.0f, 1.0f,

		// top face
		 4.5f, 3.0f, -6.5f,		 0.0f,  1.0f,  0.0f,	0.0f, 0.4f,
		6.75f, 3.0f, -7.0f,		 0.0f,  1.0f,  0.0f,    0.0f, 0.6f,
		 5.5f, 3.0f, -2.0f,		 0.0f,  1.0f,  0.0f,    1.0f, 0.4f,
		 5.5f, 3.0f, -2.0f,		 0.0f,  1.0f,  0.0f,    1.0f, 0.4f,
		7.75f, 3.0f, -2.5f,		 0.0f,  1.0f,  0.0f,	1.0f, 0.6f,
		6.75f, 3.0f, -7.0f,		 0.0f,  1.0f,  0.0f,	0.0f, 0.0f,

		// bottom face
		 4.5f, 0.0f, -6.5f,		 0.0f, -1.0f,  0.0f,    0.0f, 0.0f,
		6.75f, 0.0f, -7.0f,		 0.0f, -1.0f,  0.0f,    1.0f, 0.0f,
		 5.5f, 0.0f, -2.0f,		 0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
		 5.5f, 0.0f, -2.0f,		 0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
		7.75f, 0.0f, -2.5f,		 0.0f, -1.0f,  0.0f,    0.0f, 1.0f,
		6.75f, 0.0f, -7.0f,	 	 0.0f, -1.0f,  0.0f,    0.0f, 0.0f,

		// left face
		 4.5f, 0.0f, -6.5f, 	-1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
		 4.5f, 3.0f, -6.5f, 	-1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
		6.75f, 3.0f, -7.0f, 	-1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
		6.75f, 3.0f, -7.0f, 	-1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
		6.75f, 0.0f, -7.0f, 	-1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
		 4.5f, 0.0f, -6.5f, 	-1.0f,  0.0f,  0.0f,    0.0f, 0.0f,

		// right face
		 5.5f, 0.0f, -2.0f, 	-1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
		 5.5f, 3.0f, -2.0f, 	-1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
		7.75f, 3.0f, -2.5f, 	-1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
		7.75f, 3.0f, -2.5f, 	-1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
		7.75f, 0.0f, -2.5f, 	-1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
		 5.5f, 0.0f, -2.0f, 	-1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}


// ---------------------------------------------------------
// STANDARD FUNCTIONS - No changes made beyond this point
// ---------------------------------------------------------
bool UInitialize(int, char* [], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}

void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
// INPUT HANDLER
// ---------------------------------------------------------------------
void UProcessInput(GLFWwindow* window)
{
	static const float cameraSpeed = 2.5f;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraOffset = cameraSpeed * gDeltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		orthographic = !orthographic;
}

// ---------------------------------------------------------------------
// CLEANUP FUNCTIONS
// ---------------------------------------------------------------------
void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}
void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}
// ---------------------------------------------------------------------
// MOUSE CONTROL FUNCTIONS
// ---------------------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coords go bottom to top

    gLastX = xpos;
    gLastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset > 0.0f)
		if (camera.MovementSpeed < 7.5f)
			camera.MovementSpeed += 0.5f;
	if (yoffset < 0.0f)
		if (camera.MovementSpeed > 0.5f)
			camera.MovementSpeed -= 0.5f;
}
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // no mouse click events required. 
}

