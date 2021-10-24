/**
* NAME: Guilherme Pereira
* DATE: 10/13/2021
* DESC: This program simulates free movement in 3D space around multiple 3D objects. It also includes a simulating of perspective and orthgraphic views,
* lighting generated from certain points in space, and textures to give objcts details on their surface.
**/

#include <iostream>             // cout, cerr
#include <cstdlib>              // EXIT_FAILURE
#include <GL/glew.h>            // GLEW library
#include <GLFW/glfw3.h>         // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>          // Image loading Utility functions


// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = " Guilherme Pereira - Final Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint nVertices;   // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    // Triangle mesh data
    GLMesh planeMesh;
    GLMesh pyramidMesh;
    GLMesh cubeMesh;
    GLMesh rectPrismMesh;
    GLMesh cylinderMesh;

    // Textures
    GLuint texTorchHandleId;
    GLuint texTorchLightId;
    GLuint texShinyBlueId;
    GLuint texBirchId;
    GLuint texPlasticId;

    GLint gTexWrapMode = GL_REPEAT;

    // Shader program
    GLuint gProgramId;
    GLuint gLightProgramId;

    // Camera
    Camera gCamera(glm::vec3(0.0f, 1.5f, 7.0f)); // Default camera position
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    bool gPerspectiveView = true; // Global view variable set to perspective by default

    // Timing
    float gDeltaTime = 0.0f; // Time between current frame and last frame
    float gLastFrame = 0.0f;

    // Object color
    glm::vec3 gObjectColor(1.0f, 1.0f, 1.0f); // All objects will have their full colors

    // Light properties
    glm::vec3 sunPosition(0.0f, 8.0f, 0.0f); // Position of global light
    glm::vec3 sunColor(1.0f, 1.0f, 1.0f); // Global color
    glm::vec3 torchLightColor(1.0f, 0.7f, 0.3f); // Orange-yellow color
    glm::vec3 torchLightPosition(1.4f, 3.3f, -0.15f); // Should be placed where torch light head is
    glm::vec3 gLightScale(0.2f);
}


// Function prototypes
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void processView(GLFWwindow* window); // Toggle between views
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void createPlaneMesh(GLMesh& mesh);
void createPyramidMesh(GLMesh& mesh);
void createCubeMesh(GLMesh& mesh);
void createRectPrismMesh(GLMesh& mesh);
void createCylinderMesh(GLMesh& mesh);
void drawScene(); // Functiont that draws all the shapes at once
void drawPlane(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle); // Will draw a plane with passed values
void drawPyramid(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle); // Will draw a pyramid with passed values
void drawCube(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle); // Will draw a cube with passed values
void drawRectPrism(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle); // Will draw a rectangular prism with passed values
void drawCylinder(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle); // Will draw a cylinder with passed values
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


// Vertex shader source code
const GLchar* vertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


// Fragment shader source code
const GLchar* fragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 torchColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    // Ambient calculation
    float ambientStrength = 0.3f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    // Diffuse calculation
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    // Specular calculation
    float specularIntensity = 1.0f; // Set specular light strength
    float highlightSize = 10.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector

                                                     // Specualr component calculation
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);



// Light shader source code
const GLchar* lightVertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

    // Uniform / Global variables for transform matrix
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Trasnform vertices into clip coordinates
}
);



// Light fragment shader source code
const GLchar* lightFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing light color

void main()
{
    fragmentColor = vec4(1.0f); // Color set to white
}
);



// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
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


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the meshes
    createPlaneMesh(planeMesh);
    createPyramidMesh(pyramidMesh);
    createCubeMesh(cubeMesh);
    createRectPrismMesh(rectPrismMesh);
    createCylinderMesh(cylinderMesh);

    // Create the shader programs
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;


    if (!UCreateShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, gLightProgramId))
        return EXIT_FAILURE;


    // Load textures
    const char* filenameTorchHandle = "../../resources/textures/Torch_Stick.png";
    const char* filenameTorchLight = "../../resources/textures/Torch_Light.png";
    const char* filenameShinyBlue = "../../resources/textures/Dark_Blue.jpg";
    const char* filenameBirch = "../../resources/textures/Birch.jpg";
    const char* filenamePlastic = "../../resources/textures/White_Plastic.jpg";

    // Check if textures loaded
    if (!UCreateTexture(filenameTorchHandle, texTorchHandleId)) {
        cout << "Failed to load texture: " << filenameTorchHandle << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(filenameTorchLight, texTorchLightId)) {
        cout << "Failed to load texture: " << filenameTorchLight << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(filenameShinyBlue, texShinyBlueId)) {
        cout << "Failed to load texture: " << filenameShinyBlue << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(filenameBirch, texBirchId)) {
        cout << "Failed to load texture: " << filenameBirch << endl;
        return EXIT_FAILURE;
    }

    if (!UCreateTexture(filenamePlastic, texPlasticId)) {
        cout << "Failed to load texture: " << filenamePlastic << endl;
        return EXIT_FAILURE;
    }

    // Tell OpenGL for each sampler to which texture unit it belongs to. (Only needs to be done once).
    glUseProgram(gProgramId);

    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Render loop
    while (!glfwWindowShouldClose(gWindow))
    {
        // Frame timing
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // Inputs
        UProcessInput(gWindow);
        processView(gWindow);

        // Render current frame
        drawScene();
        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(planeMesh);
    UDestroyMesh(pyramidMesh);
    UDestroyMesh(cubeMesh);
    UDestroyMesh(rectPrismMesh);

    // Release texture
    UDestroyTexture(texTorchHandleId);
    UDestroyTexture(texTorchLightId);
    UDestroyTexture(texShinyBlueId);
    UDestroyTexture(texBirchId);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLightProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
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


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 3.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);

    // Wrapping mode
    glBindTexture(GL_TEXTURE_2D, texTorchHandleId);
    glBindTexture(GL_TEXTURE_2D, texTorchLightId);
    glBindTexture(GL_TEXTURE_2D, texShinyBlueId);
    glBindTexture(GL_TEXTURE_2D, texBirchId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Speed is changed within the header file of Camera.
    gCamera.ProcessMouseScroll(yoffset);
}


void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Changes the view
void processView(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        //std::cout << "P was pressed! Switching to ORTHO mode." << std::endl;
        gPerspectiveView = false;
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        //std::cout << "O was pressed! Switching to PERSPECTIVE mode." << std::endl;
        gPerspectiveView = true;
    }
}


// Update camera
void updateCamera(glm::mat4 model) {
    glm::mat4 view = gCamera.GetViewMatrix();
    glm::mat4 projection; // Initialize projection

    // Determine whether the projection is perspective or ortho.
    if (gPerspectiveView) {
        // Creates perspective projection
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        // Creates ortho projection
        projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
    }

    // Shader selection
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from object shader program for color and position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the object Shader program's corresponding uniforms
    // Pass data to sun light
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, sunColor.r, sunColor.g, sunColor.b);
    glUniform3f(lightPositionLoc, sunPosition.x, sunPosition.y, sunPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Pass data to torch light
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, torchLightColor.r, torchLightColor.g, torchLightColor.b);
    glUniform3f(lightPositionLoc, torchLightPosition.x, torchLightPosition.y, torchLightPosition.z);
}


// Renders
void drawPlane(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId); // Shader to be used

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Update camera transformation
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texBirchId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, planeMesh.nVertices);






}


void drawCube(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId); // Shader to be used

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Updates the camera and selects shader
    glm::mat4 view = gCamera.GetViewMatrix();
    glm::mat4 projection; // Initialize projection


    // Determine whether the projection is perspective or ortho.
    if (gPerspectiveView) {
        // Creates perspective projection
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        // Creates ortho projection
        projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 100.0f);
    }

    // Update camera
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(cubeMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texTorchLightId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, cubeMesh.nVertices);

    // Draw the light source
    GLint modelLoc;
    GLint viewLoc;
    GLint projLoc;

    // Select shader program
    glUseProgram(gLightProgramId);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(torchLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Light Shader program
    modelLoc = glGetUniformLocation(gLightProgramId, "model");
    viewLoc = glGetUniformLocation(gLightProgramId, "view");
    projLoc = glGetUniformLocation(gLightProgramId, "projection");

    // Pass matrix data to the Light Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, cubeMesh.nVertices);
}


void drawRectPrism(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId); // Shader to be used

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Updates the camera and selects shader
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(rectPrismMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texTorchHandleId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, rectPrismMesh.nVertices);
}


void drawPyramid(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId); // Shader to be used

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // Apply adjusted angle
    glm::mat4 adjustment = glm::rotate(1.58f, glm::vec3(-1.0f, 0.0f, 0.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * (rotation * adjustment) * scale;

    // Update the camera's position
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(pyramidMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texShinyBlueId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, pyramidMesh.nVertices);
}


void drawCylinder(float xScale, float yScale, float zScale, float xPos, float yPos, float zPos, float angle) {
    glUseProgram(gProgramId); // Shader to be used

    // Apply scale
    glm::mat4 scale = glm::scale(glm::vec3(xScale, yScale, zScale));
    // Apply Rotation
    glm::mat4 rotation = glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // Apply Translation
    glm::mat4 translation = glm::translate(glm::vec3(xPos, yPos, zPos));
    // Apply model matrix
    glm::mat4 model = translation * rotation * scale;

    // Updates the camera and selects shader
    updateCamera(model);

    // Scale the texture proportional to the object
    glm::vec2 gUVScale(1.0f, 1.0f);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(cylinderMesh.vao);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texPlasticId);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, cylinderMesh.nVertices);
}





// Function to draw all the shapes
void drawScene() {
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Desk
    drawPlane(12.5, 1.0, 10.0, 0.0, 0.0, 0.0, 0.0);

    // Blue pyramid
    drawPyramid(1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0);

    // Minecraft Torch light
    drawRectPrism(0.8, 3.2, 0.8, 1.4, 0.0, -0.15, 5.0); // Torch handle
    drawCube(0.8, 0.8, 0.8, 1.4, 3.2, -0.15, 5.0); // Torch head

    // Water bottle
    drawCylinder(0.6, 2.8, 0.6, 0.0, 0.0, -2.0, 0.0);
    drawCylinder(0.55, 0.3, 0.55, 0.0, 2.8, -2.0, 0.0);
    drawCylinder(0.2, 0.2, 0.2, 0.0, 3.1, -2.0, 0.0);

    // Deactviate VAO
    glBindVertexArray(0);
    glUseProgram(0);

    // Refresh the screen
    glfwSwapBuffers(gWindow);
}

// Meshes
void createPlaneMesh(GLMesh& mesh) {
    // Vertex data
    GLfloat verts[] = {
        // Vertex Positions||  Normal vectors  || Texture coordinates
        // Base (Facing Y+)
       -0.5f,  0.0f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,    // A
        0.5f,  0.0f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,    // B
        0.5f,  0.0f, -0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,    // C
       -0.5f,  0.0f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,    // A
       -0.5f,  0.0f, -0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,    // D
        0.5f,  0.0f, -0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f     // C
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


void createPyramidMesh(GLMesh& mesh) {
    // Vertex Data
    GLfloat verts[] = {
        // Vertex Positions||  Normal vectors   || Texture coordinates
        // Base (Facing Y-)
       -0.5f, -0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,  // A
        0.5f, -0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,  // B
        0.5f,  0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,  // C
       -0.5f, -0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,  // A
       -0.5f,  0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,  // D
        0.5f,  0.5f, 0.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,  // C

        // Side 1 (Facing Z+)
       -0.5f, -0.5f, 0.0f,   0.0f, -1.0f,  1.0f,   0.0f, 0.0f,  // A
        0.5f, -0.5f, 0.0f,   0.0f, -1.0f,  1.0f,   1.0f, 0.0f,  // B
        0.0f, -0.0f, 1.0f,   0.0f, -1.0f,  1.0f,   0.5f, 1.0f,  // E

        // Side 2 (Facing X+)
        0.5f, -0.5f, 0.0f,   1.0f,  0.0f, 1.0f,    0.0f, 0.0f,  // B
        0.5f,  0.5f, 0.0f,   1.0f,  0.0f, 1.0f,    1.0f, 0.0f,  // C
        0.0f, -0.0f, 1.0f,   1.0f,  0.0f, 1.0f,    0.5f, 1.0f,  // E

        // Side 3 (Facing Z-)
        0.5f,  0.5f, 0.0f,   0.0f,  1.0f, 1.0f,    0.0f, 0.0f,  // C
       -0.5f,  0.5f, 0.0f,   0.0f,  1.0f, 1.0f,    1.0f, 0.0f,  // D
        0.0f, -0.0f, 1.0f,   0.0f,  1.0f, 1.0f,    0.5f, 1.0f,  // E

        // Side 4 (Facing X-)
       -0.5f,  0.5f, 0.0f,   -1.0f,  0.0f, 1.0f,   0.0f, 0.0f,  // D
       -0.5f, -0.5f, 0.0f,   -1.0f,  0.0f, 1.0f,   1.0f, 0.0f,  // A
        0.0f, -0.0f, 1.0f,   -1.0f,  0.0f, 1.0f,   0.5f, 1.0f   // E
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


void createCubeMesh(GLMesh& mesh) {
    // Vertex Data
    GLfloat verts[] = {
        // Normal vectors must be opposite for light to pass through
        // Vertex Positions||  Normal vectors   || Texture coordinates
        // Base (Facing Y-)
       -0.5f,  0.0f, -0.5f,   0.0f, 1.0f, 0.0f,    0.0f, 0.0f,    // A
        0.5f,  0.0f, -0.5f,   0.0f, 1.0f, 0.0f,    1.0f, 0.0f,    // B
        0.5f,  0.0f,  0.5f,   0.0f, 1.0f, 0.0f,    1.0f, 1.0f,    // C
       -0.5f,  0.0f, -0.5f,   0.0f, 1.0f, 0.0f,    0.0f, 0.0f,    // A
       -0.5f,  0.0f,  0.5f,   0.0f, 1.0f, 0.0f,    0.0f, 1.0f,    // D
        0.5f,  0.0f,  0.5f,   0.0f, 1.0f, 0.0f,    1.0f, 1.0f,    // C

        // Top (Facing Y+)
       -0.5f,  1.0f, -0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f,    // E
        0.5f,  1.0f, -0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f,    // F
        0.5f,  1.0f,  0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,    // G
       -0.5f,  1.0f, -0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f,    // E
       -0.5f,  1.0f,  0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 1.0f,    // H
        0.5f,  1.0f,  0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,    // G

        // Side #1 (Facing Z-)
       -0.5f,  0.0f, -0.5f,   0.0f,  0.0f, 1.0f,  0.0f, 0.0f,    // A
        0.5f,  0.0f, -0.5f,   0.0f,  0.0f, 1.0f,  1.0f, 0.0f,    // B
        0.5f,  1.0f, -0.5f,   0.0f,  0.0f, 1.0f,  1.0f, 1.0f,    // F
       -0.5f,  0.0f, -0.5f,   0.0f,  0.0f, 1.0f,  0.0f, 0.0f,    // A
       -0.5f,  1.0f, -0.5f,   0.0f,  0.0f, 1.0f,  0.0f, 1.0f,    // E
        0.5f,  1.0f, -0.5f,   0.0f,  0.0f, 1.0f,  1.0f, 1.0f,    // F

        // Side #2 (Facing X-)
       -0.5f,  1.0f, -0.5f,   1.0f,  0.0f, 0.0f,  0.0f, 0.0f,    // E
       -0.5f,  0.0f, -0.5f,   1.0f,  0.0f, 0.0f,  1.0f, 0.0f,    // A
       -0.5f,  0.0f,  0.5f,   1.0f,  0.0f, 0.0f,  1.0f, 1.0f,    // D
       -0.5f,  1.0f, -0.5f,   1.0f,  0.0f, 0.0f,  0.0f, 0.0f,    // E
       -0.5f,  1.0f,  0.5f,   1.0f,  0.0f, 0.0f,  0.0f, 1.0f,    // H
       -0.5f,  0.0f,  0.5f,   1.0f,  0.0f, 0.0f,  1.0f, 1.0f,    // D

        // Side #3 (Facing X+)
        0.5f,  0.0f, -0.5f,    -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,    // B
        0.5f,  1.0f, -0.5f,    -1.0f,  0.0f, 0.0f,  1.0f, 0.0f,    // F
        0.5f,  1.0f,  0.5f,    -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,    // G
        0.5f,  0.0f, -0.5f,    -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,    // B
        0.5f,  0.0f,  0.5f,    -1.0f,  0.0f, 0.0f,  0.0f, 1.0f,    // C
        0.5f,  1.0f,  0.5f,    -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,    // G

        // Side #4 (Facing Z+)
       -0.5f,  0.0f,  0.5f,    0.0f,  0.0f, -1.0f,  0.0f, 0.0f,    // D
        0.5f,  0.0f,  0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 0.0f,    // C
        0.5f,  1.0f,  0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 1.0f,    // G
       -0.5f,  0.0f,  0.5f,    0.0f,  0.0f, -1.0f,  0.0f, 0.0f,    // D
       -0.5f,  1.0f,  0.5f,    0.0f,  0.0f, -1.0f,  0.0f, 1.0f,    // H
        0.5f,  1.0f,  0.5f,    0.0f,  0.0f, -1.0f,  1.0f, 1.0f,    // G
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


void createRectPrismMesh(GLMesh& mesh) {
    // Vertex Data
    GLfloat verts[] = {
        // Vertex Positions||  Normal vectors   || Texture coordinates
        // Base (Facing Y-)
       -0.5f,  0.0f, -0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f,    // A
        0.5f,  0.0f, -0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f,    // B
        0.5f,  0.0f,  0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,    // C
       -0.5f,  0.0f, -0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f,    // A
       -0.5f,  0.0f,  0.5f,   0.0f, -1.0f, 0.0f,   0.0f, 1.0f,    // D
        0.5f,  0.0f,  0.5f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,    // C

        // Top (Facing Y+)
       -0.5f,  1.0f, -0.5f,   0.0f,  1.0f, 0.0f,   0.0f, 0.0f,    // E
        0.5f,  1.0f, -0.5f,   0.0f,  1.0f, 0.0f,   1.0f, 0.0f,    // F
        0.5f,  1.0f,  0.5f,   0.0f,  1.0f, 0.0f,   1.0f, 1.0f,    // G
       -0.5f,  1.0f, -0.5f,   0.0f,  1.0f, 0.0f,   0.0f, 0.0f,    // E
       -0.5f,  1.0f,  0.5f,   0.0f,  1.0f, 0.0f,   0.0f, 1.0f,    // H
        0.5f,  1.0f,  0.5f,   0.0f,  1.0f, 0.0f,   1.0f, 1.0f,    // G

        // Side #1 (Facing Z-)
       -0.5f,  0.0f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,    // A
        0.5f,  0.0f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f,    // B
        0.5f,  1.0f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,    // F
       -0.5f,  0.0f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,    // A
       -0.5f,  1.0f, -0.5f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f,    // E
        0.5f,  1.0f, -0.5f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,    // F

        // Side #2 (Facing X-)
       -0.5f,  1.0f, -0.5f,   -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,    // E
       -0.5f,  0.0f, -0.5f,   -1.0f,  0.0f, 0.0f,  0.0f, 1.0f,    // A
       -0.5f,  0.0f,  0.5f,   -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,    // D
       -0.5f,  1.0f, -0.5f,   -1.0f,  0.0f, 0.0f,  0.0f, 0.0f,    // E
       -0.5f,  1.0f,  0.5f,   -1.0f,  0.0f, 0.0f,  1.0f, 0.0f,    // H
       -0.5f,  0.0f,  0.5f,   -1.0f,  0.0f, 0.0f,  1.0f, 1.0f,    // D

        // Side #3 (Facing X+)
        0.5f,  0.0f, -0.5f,    1.0f,  0.0f, 0.0f,  1.0f, 1.0f,    // B
        0.5f,  1.0f, -0.5f,    1.0f,  0.0f, 0.0f,  1.0f, 0.0f,    // F
        0.5f,  1.0f,  0.5f,    1.0f,  0.0f, 0.0f,  0.0f, 0.0f,    // G
        0.5f,  0.0f, -0.5f,    1.0f,  0.0f, 0.0f,  1.0f, 1.0f,    // B
        0.5f,  0.0f,  0.5f,    1.0f,  0.0f, 0.0f,  0.0f, 1.0f,    // C
        0.5f,  1.0f,  0.5f,    1.0f,  0.0f, 0.0f,  0.0f, 0.0f,    // G

        // Side #4 (Facing Z+)
       -0.5f,  0.0f,  0.5f,    0.0f,  0.0f, 1.0f,  0.0f, 0.0f,    // D
        0.5f,  0.0f,  0.5f,    0.0f,  0.0f, 1.0f,  1.0f, 0.0f,    // C
        0.5f,  1.0f,  0.5f,    0.0f,  0.0f, 1.0f,  1.0f, 1.0f,    // G
       -0.5f,  0.0f,  0.5f,    0.0f,  0.0f, 1.0f,  0.0f, 0.0f,    // D
       -0.5f,  1.0f,  0.5f,    0.0f,  0.0f, 1.0f,  0.0f, 1.0f,    // H
        0.5f,  1.0f,  0.5f,    0.0f,  0.0f, 1.0f,  1.0f, 1.0f,    // G
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


void createCylinderMesh(GLMesh& mesh) {
    GLfloat verts[] = {
        // Vertex Positions  ||  Normal vectors   || Texture coordinates
        // Base (Facing Y-)
        
        -1.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // A
        -0.7f,  0.0f, -0.7f,    0.0f, 0.0f, 0.0f,    0.0f, 1.0f,     // B
         0.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    1.0f, 1.0f,     // O

        -0.7f,  0.0f, -0.7f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // B
         0.0f,  0.0f, -1.0f,    0.0f, 0.0f, 0.0f,    0.0f, 1.0f,     // C
         0.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    1.0f, 1.0f,     // O

         0.0f,  0.0f, -1.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // C
         0.7f,  0.0f, -0.7f,    0.0f, 0.0f, 0.0f,    0.0f, 1.0f,     // D
         0.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    1.0f, 1.0f,     // O

         0.7f,  0.0f, -0.7f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // D
         1.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 1.0f,     // E
         0.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    1.0f, 1.0f,     // O

         1.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // E
         0.7f,  0.0f,  0.7f,    0.0f, 0.0f, 0.0f,    0.0f, 1.0f,     // F
         0.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    1.0f, 1.0f,     // O

         0.7f,  0.0f,  0.7f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // F
         0.0f,  0.0f,  1.0f,    0.0f, 0.0f, 0.0f,    0.0f, 1.0f,     // G
         0.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    1.0f, 1.0f,     // O

         0.0f,  0.0f,  1.0f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // G
        -0.7f,  0.0f,  0.7f,    0.0f, 0.0f, 0.0f,    0.0f, 1.0f,     // H
         0.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    1.0f, 1.0f,     // O

        -0.7f,  0.0f,  0.7f,    0.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // H
        -1.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    0.0f, 1.0f,     // A
         0.0f,  0.0f,  0.0f,    0.0f, 0.0f, 0.0f,    1.0f, 1.0f,     // O
         

         // Side #1
        -1.0f,  0.0f,  0.0f,   -1.0f, 0.0f,  0.0f,    0.0f, 0.0f,     // A
        -1.0f,  1.0f,  0.0f,   -1.0f, 0.0f,  0.0f,    0.0f, 0.0f,     // A2
        -0.7f,  1.0f, -0.7f,   -1.0f, 0.0f,  0.0f,    0.0f, 0.0f,     // B2

        -1.0f,  0.0f,  0.0f,   -1.0f, 0.0f,  0.0f,    0.0f, 0.0f,     // A
        -0.7f,  0.0f, -0.7f,   -1.0f, 0.0f,  0.0f,    0.0f, 0.0f,     // B
        -0.7f,  1.0f, -0.7f,   -1.0f, 0.0f,  0.0f,    0.0f, 0.0f,     // B2

         // Side #2
        -0.7f,  0.0f, -0.7f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // B
        -0.7f,  1.0f, -0.7f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // B2
         0.0f,  1.0f, -1.0f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // C2

        -0.7f,  0.0f, -0.7f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // B
         0.0f,  0.0f, -1.0f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // C
         0.0f,  1.0f, -1.0f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // C2

          // Side #3
         0.0f,  0.0f, -1.0f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // C
         0.0f,  1.0f, -1.0f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // C2
         0.7f,  1.0f, -0.7f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // D2

         0.0f,  0.0f, -1.0f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // C
         0.7f,  0.0f, -0.7f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // D
         0.7f,  1.0f, -0.7f,    0.0f, 0.0f, -1.0f,    0.0f, 0.0f,     // D2

         // Side #4
         0.7f,  0.0f, -0.7f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // D
         0.7f,  1.0f, -0.7f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // D2
         1.0f,  1.0f,  0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // E2

         0.7f,  0.0f, -0.7f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // D
         1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // E
         1.0f,  1.0f,  0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // E2

          // Side #5
         1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // E
         1.0f,  1.0f,  0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // E2
         0.7f,  1.0f,  0.7f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // F2

         1.0f,  0.0f,  0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // E
         0.7f,  0.0f,  0.7f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // F
         0.7f,  1.0f,  0.7f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // F2

          // Side #6
         0.7f,  0.0f,  0.7f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // F
         0.7f,  1.0f,  0.7f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // F2
         0.0f,  1.0f,  1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // G2

         0.7f,  0.0f,  0.7f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // F
         0.0f,  0.0f,  1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // G
         0.0f,  1.0f,  1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // G2 

         // Side #7
         0.0f,  0.0f,  1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // G
         0.0f,  1.0f,  1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // G2
        -0.7f,  1.0f,  0.7f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // H2

         0.0f,  0.0f,  1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // G
        -0.7f,  0.0f,  0.7f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // H
        -0.7f,  1.0f,  0.7f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,     // H2

        // Side #8
        -0.7f,  0.0f,  0.7f,    -1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // H
        -0.7f,  1.0f,  0.7f,    -1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // H2 
        -1.0f,  1.0f,  0.0f,    -1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // A2

        -0.7f,  0.0f,  0.7f,    -1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // H
        -1.0f,  0.0f,  0.0f,    -1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // A
        -1.0f,  1.0f,  0.0f,    -1.0f, 0.0f, 0.0f,    0.0f, 0.0f,     // A2


         // Top (Facing Y+)
        -1.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     // A2
        -0.7f,  1.0f, -0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     // B2
         0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     // O2

        -0.7f,  1.0f, -0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     // B2
         0.0f,  1.0f, -1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     // C2
         0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     // O2
             
         0.0f,  1.0f, -1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     // C2
         0.7f,  1.0f, -0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     // D2
         0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     // O2

         0.7f,  1.0f, -0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     // D2
         1.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     // E2
         0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     // O2

         1.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     // E2
         0.7f,  1.0f,  0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     // F2
         0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     // O2

         0.7f,  1.0f,  0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     // F2
         0.0f,  1.0f,  1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     // G2
         0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     // O2

         0.0f,  1.0f,  1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     // G2
        -0.7f,  1.0f,  0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     // H2
         0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     // O2

        -0.7f,  1.0f,  0.7f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,     // H2
        -1.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,     // A2
         0.0f,  1.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,     // O2
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


// Generate and load texture
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

// Destroy mesh
void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}

// Destroy Texture
void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}

// End shader program
void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}