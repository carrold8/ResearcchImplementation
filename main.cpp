/*

        Code submitted for Real-Time Rendering Assignment 5
        This code is an implementation of Line Rendering in Loose Style for 3D Models D. Chen, Y. Zhang and P. Xu
        
        David Carroll
        17330350

        April 2022

*/
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector> // STL dynamic memory.


// Assimp file loader
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include "maths_funcs.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb-master/stb_image.h"


/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define FENCE "Models/fence.dae"
#define BOX "Models/box.dae"
#define BALL "Models/ball.dae"
#define RABBIT "Models/rabbit.dae"


// Variables to control camera to move about scene
vec3 cameraPos = vec3(0.0f, 1.0f, 5.0f);
vec3 cameraTarget = vec3(0.0f, 0.0f, 0.0f);
vec3 cameraDirec = normalise(cameraPos - cameraTarget);
vec3 up = vec3(0.0f, 1.0f, 0.0f);
vec3 camRight = normalise(cross(up, cameraDirec));
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float fov = 45.0f;



// timing variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;


// mouse state
bool useMouse = false;
bool firstMouse = true;
float lastX = 800.0f / 2.0;
float lastY = 600.0f / 2.0;


// Struct for Models loaded in to project
#pragma region SimpleTypes
typedef struct ModelData
{
    size_t mPointCount = 0;
    std::vector<vec3> mVertices;
    std::vector<vec3> mNormals;
    std::vector<vec2> mTextureCoords;
    std::vector<vec3> mTangents;
    std::vector<vec3> mBitangents;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;


// Shader program declarations
GLuint shaderSimple;
GLuint shaderFinal;
GLuint shaderGeo;


// Model Data for imported models and their VAO's
ModelData box, fence, ball, torus, rabbit;
GLuint  fence_vao, box_vao, ball_vao, torus_vao, rabbit_vao;

// Locations for shader variables
GLuint loc1, loc2, loc3, loc4, loc5;

// This value is updated in the UpdateScene() function to slowly rotate models about the y-axis
GLfloat rotate_y = 0.0f;


// ------------------------------   FUNCTION DECLARATIONS    ----------------------------------------------------------


// FrameBuffer size callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// Mouse Callback and Key input
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void processInput(GLFWwindow* window);


//Loads Mesh Data using Assimp Library
ModelData load_mesh(const char* file_name);

// Shader Functions
char* readShaderSource(const char* shaderFile);                                             // Read shader file
static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType);    // Add shader file to Shader Program
GLuint CompileShaders(const char* vertex, const char* fragment, const char* geometry);      // Create Shader Program

// VBO functions
GLuint generateObjectBufferMesh(ModelData mesh_data, GLuint shader);                        // Return the VAO for a model

// Degreees and Radian Conversion
double radians(double degree);


// Display Function
void display();

// Load Texture - Used to load the preprocessed curvature texture
unsigned int loadTexture(char const* path);

// Init and Update Function
void init();
void updateScene();

// Multiply Vector by a float
vec3 vecXfloat(float f, vec3 v1);


// Curve Map Texture used to demonstrate Shader Program for Loose Line Rendering
unsigned int CurveMap;

// GLFW window width and height
const unsigned int screen_width = 1000;
const unsigned int screen_height = 750;

// Light Position for Lighting Model
vec3 light_position = vec3(5.0f, 20.0f, 15.0f);


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Loose Line Rendering", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Compile shaders, create textures and load mesh's for scene 
    init();
    

    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);

    while (!glfwWindowShouldClose(window))
    {

        // Listen for user keypress input
        processInput(window);

        //Display Loose Line Rendered Models
        display();

        updateScene();

        glfwSwapBuffers(window);
        glfwPollEvents();


    }

    glfwTerminate();
    return 0;
}




void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Here this function allows us to move our camera about the scene using keypresses
void processInput(GLFWwindow* window)
{
    float cameraSpeed = 10.5f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cameraPos += vecXfloat(cameraSpeed, cameraFront);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cameraPos -= vecXfloat(cameraSpeed, cameraFront);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cameraPos -= normalise(cross(cameraFront, cameraUp)) * cameraSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cameraPos += normalise(cross(cameraFront, cameraUp)) * cameraSpeed;
    }


}

ModelData load_mesh(const char* file_name) {

    ModelData modelData;

    /* Use assimp to read the model file, forcing it to be read as    */
    /* triangles. The second flag (aiProcess_PreTransformVertices) is */
    /* relevant if there are multiple meshes in the model file that   */
    /* are offset from the origin. This is pre-transform them so      */
    /* they're in the right position.                                 */
    const aiScene* scene = aiImportFile(
        file_name,
        aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace
    );

    if (!scene) {
        fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
        return modelData;
    }

    printf("  %i materials\n", scene->mNumMaterials);
    printf("  %i meshes\n", scene->mNumMeshes);
    printf("  %i textures\n", scene->mNumTextures);

    for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
        const aiMesh* mesh = scene->mMeshes[m_i];
        printf("    %i vertices in mesh\n", mesh->mNumVertices);
        modelData.mPointCount += mesh->mNumVertices;
        for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
            if (mesh->HasPositions()) {
                const aiVector3D* vp = &(mesh->mVertices[v_i]);
                modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
            }
            if (mesh->HasNormals()) {
                const aiVector3D* vn = &(mesh->mNormals[v_i]);
                modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
            }
            if (mesh->HasTextureCoords(0)) {
                const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
                modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));

            }
            if (mesh->HasTangentsAndBitangents()) {
                /* You can extract tangents and bitangents here              */
                /* Note that you might need to make Assimp generate this     */
                /* data for you. Take a look at the flags that aiImportFile  */
                /* can take.                                                 */
                const aiVector3D* vtan = &(mesh->mTangents[v_i]);
                modelData.mTangents.push_back(vec3(vtan->x, vtan->y, vtan->z));

                const aiVector3D* vbt = &(mesh->mBitangents[v_i]);
                modelData.mBitangents.push_back(vec3(vbt->x, vbt->y, vbt->z));
            }
        }
    }

    aiReleaseImport(scene);
    return modelData;
}




char* readShaderSource(const char* shaderFile) {
    FILE* fp;
    fopen_s(&fp, shaderFile, "rb");

    if (fp == NULL) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';

    fclose(fp);

    return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    // create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        std::cerr << "Error creating shader..." << std::endl;
        std::cerr << "Press enter/return to exit..." << std::endl;
        std::cin.get();
        exit(1);
    }
    const char* pShaderSource = readShaderSource(pShaderText);


    // Bind the source code to the shader, this happens before compilation
    glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
    // compile the shader and check for errors


    glCompileShader(ShaderObj);
    GLint success;
    // check for shader related errors using glGetShaderiv

    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024] = { '\0' };
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        std::cerr << "Error compiling "
            << (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
            << " shader program: " << InfoLog << std::endl;
        std::cerr << "Press enter/return to exit..." << std::endl;
        std::cin.get();
        exit(1);
    }
    // Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}



GLuint CompileShaders(const char* vertex, const char* fragment, const char* geometry)
{
    //Start the process of setting up our shaders by creating a program ID
    //Note: we will link all the shaders together into this ID
    GLuint shaderProgram = glCreateProgram();
    if (shaderProgram == 0) {
        std::cerr << "Error creating shader program..." << std::endl;
        std::cerr << "Press enter/return to exit..." << std::endl;
        std::cin.get();
        exit(1);
    }


    // Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgram, vertex, GL_VERTEX_SHADER);
    AddShader(shaderProgram, fragment, GL_FRAGMENT_SHADER);

    if (geometry != nullptr) {
        AddShader(shaderProgram, geometry, GL_GEOMETRY_SHADER);
    }


    GLint Success = 0;
    GLchar ErrorLog[1024] = { '\0' };
    // After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgram);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(shaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
        std::cerr << "Press enter/return to exit..." << std::endl;
        std::cin.get();
        exit(1);
    }

    // program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgram);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
        std::cerr << "Press enter/return to exit..." << std::endl;
        std::cin.get();
        exit(1);
    }
    // Finally, use the linked shader program
    // Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgram);
    return shaderProgram;
}



GLuint generateObjectBufferMesh(ModelData mesh_data, GLuint shader) {
    /*----------------------------------------------------------------------------
    LOAD MESH HERE AND COPY INTO BUFFERS
    ----------------------------------------------------------------------------*/

    unsigned int vp_vbo = 0;
    loc1 = glGetAttribLocation(shader, "vertex_position");
    loc2 = glGetAttribLocation(shader, "vertex_normal");
    loc3 = glGetAttribLocation(shader, "vertex_texture");
    loc4 = glGetAttribLocation(shader, "vertex_tangent");
    loc5 = glGetAttribLocation(shader, "vertex_bitangent");

    glGenBuffers(1, &vp_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mVertices[0], GL_STATIC_DRAW);
    unsigned int vn_vbo = 0;
    glGenBuffers(1, &vn_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mNormals[0], GL_STATIC_DRAW);

    unsigned int vt_vbo = 0;
    glGenBuffers(1, &vt_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec2), &mesh_data.mTextureCoords[0], GL_STATIC_DRAW);

    unsigned int vtan_vbo = 0;
    glGenBuffers(1, &vtan_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vtan_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mTangents[0], GL_STATIC_DRAW);

    unsigned int vbt_vbo = 0;
    glGenBuffers(1, &vbt_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbt_vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mBitangents[0], GL_STATIC_DRAW);


    unsigned int vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glEnableVertexAttribArray(loc1);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
    glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(loc2);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
    glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(loc3);
    glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
    glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(loc4);
    glBindBuffer(GL_ARRAY_BUFFER, vtan_vbo);
    glVertexAttribPointer(loc4, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(loc5);
    glBindBuffer(GL_ARRAY_BUFFER, vbt_vbo);
    glVertexAttribPointer(loc5, 3, GL_FLOAT, GL_FALSE, 0, NULL);


    return vao;

}



double radians(double degree) {
    double pi = 3.14159265359;
    return (degree * (pi / 180));
}



void display() {




    // tell GL to only draw onto a pixel if the shape is closer to the viewer
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // -----------      CREATE TEXTURED MODELS AND CULL BACK FACES

    glUseProgram(shaderSimple);
    glCullFace(GL_BACK);

    //Declare uniform variables that will be used in shader
    int matrix_location = glGetUniformLocation(shaderSimple, "model");
    int view_mat_location = glGetUniformLocation(shaderSimple, "view");
    int proj_mat_location = glGetUniformLocation(shaderSimple, "proj");
    int sampler_location = glGetUniformLocation(shaderSimple, "texture1");


    // CREATE MATRICES 
    mat4 view = identity_mat4();
    mat4 persp_proj = perspective(fov, (float)screen_width / (float)screen_height, 0.1f, 1000.0f);
   
    mat4 fence_model = identity_mat4();
    mat4 box_model = identity_mat4();
    mat4 ball_model = identity_mat4();
    mat4 rabbit_model = identity_mat4();

    view = look_at(cameraPos, cameraPos + cameraFront, cameraUp);

    // update uniforms 
    glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
    glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
    glUniform1i(sampler_location, 0);


    // Fence model with curvature mesh
    fence_model = rotate_y_deg(fence_model, rotate_y);
    glUniformMatrix4fv(matrix_location, 1, GL_FALSE, fence_model.m);
    glBindVertexArray(fence_vao);


    // Box Model with curvature mesh
    box_model = rotate_y_deg(box_model, rotate_y);
    box_model = translate(box_model, vec3(-7.5f, 0.0f, 0.0f));
    glUniformMatrix4fv(matrix_location, 1, GL_FALSE, box_model.m);
    glBindVertexArray(box_vao);


    // Ball Model with curvature mesh
    ball_model = rotate_y_deg(ball_model, rotate_y);
    ball_model = translate(ball_model, vec3(7.5f, 0.0f, 0.0f));
    glUniformMatrix4fv(matrix_location, 1, GL_FALSE, ball_model.m);
    glBindVertexArray(ball_vao);

    // Rabbit Model with curvature mesh
    rabbit_model = rotate_y_deg(rabbit_model, rotate_y);
    rabbit_model = translate(rabbit_model, vec3(15.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(matrix_location, 1, GL_FALSE, rabbit_model.m);
    glBindVertexArray(rabbit_vao);



    // ------------------------ DRAW SILHOUETTE (CULL FRONT FACES) ----------------

    glCullFace(GL_FRONT);
    glDepthFunc(GL_LEQUAL);
       
    // Use geometry shader
    glUseProgram(shaderGeo);

    // Set uniforms
    int matrix_location1 = glGetUniformLocation(shaderGeo, "model");
    int view_mat_location1 = glGetUniformLocation(shaderGeo, "view");
    int proj_mat_location1 = glGetUniformLocation(shaderGeo, "proj");
    int sampler_location1 = glGetUniformLocation(shaderGeo, "curveMap");
    int light_location1 = glGetUniformLocation(shaderGeo, "light_pos");

    glUniformMatrix4fv(proj_mat_location1, 1, GL_FALSE, persp_proj.m);
    glUniformMatrix4fv(view_mat_location1, 1, GL_FALSE, view.m);
    glUniform3fv(light_location1, 1, light_position.v);
    glUniform1i(sampler_location1, 0);

    // Fence outline
    glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, fence_model.m);
    glBindVertexArray(fence_vao);
    glDrawArrays(GL_TRIANGLES, 0, fence.mPointCount);

    // Box Outline
    glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, box_model.m);
    glBindVertexArray(box_vao);
    glDrawArrays(GL_TRIANGLES, 0, box.mPointCount);

    // Ball outline
    glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, ball_model.m);
    glBindVertexArray(ball_vao);
    glDrawArrays(GL_TRIANGLES, 0, ball.mPointCount);


    // Rabbit Outline
    glUniformMatrix4fv(matrix_location1, 1, GL_FALSE, rabbit_model.m);
    glBindVertexArray(rabbit_vao);
    glDrawArrays(GL_TRIANGLES, 0, rabbit.mPointCount);


    //     ---------------------     DRAW MODELS IN WHITE INSIDE THE OUTLINES (CULL BACKFACES)
    glUseProgram(shaderFinal);

    glCullFace(GL_BACK);

    //Declare uniforms
    int matrix_location2 = glGetUniformLocation(shaderFinal, "model");
    int view_mat_location2 = glGetUniformLocation(shaderFinal, "view");
    int proj_mat_location2 = glGetUniformLocation(shaderFinal, "proj");

    glUniformMatrix4fv(proj_mat_location2, 1, GL_FALSE, persp_proj.m);
    glUniformMatrix4fv(view_mat_location2, 1, GL_FALSE, view.m);

    // Draw Fence
    glUniformMatrix4fv(matrix_location2, 1, GL_FALSE, fence_model.m);
    glBindVertexArray(fence_vao);
    glDrawArrays(GL_TRIANGLES, 0, fence.mPointCount);

    //Draw Box
    glUniformMatrix4fv(matrix_location2, 1, GL_FALSE, box_model.m);
    glBindVertexArray(box_vao);
    glDrawArrays(GL_TRIANGLES, 0, box.mPointCount);

    // Draw Ball
    glUniformMatrix4fv(matrix_location2, 1, GL_FALSE, ball_model.m);
    glBindVertexArray(ball_vao);
    glDrawArrays(GL_TRIANGLES, 0, ball.mPointCount);

    // Draw Rabbit
    glUniformMatrix4fv(matrix_location2, 1, GL_FALSE, rabbit_model.m);
    glBindVertexArray(rabbit_vao);
    glDrawArrays(GL_TRIANGLES, 0, rabbit.mPointCount);

}






void updateScene() {

    // Get the change in time 
    static double last_time = 0;
    double curr_time = glfwGetTime();
    if (last_time == 0)
        last_time = curr_time;
    float delta = (curr_time - last_time);
    last_time = curr_time;

    float currentFrame = glfwGetTime();

    deltaTime = curr_time - lastFrame;
    lastFrame = currentFrame;

    // Rotate the model slowly around the y axis at 20 degrees per second
    rotate_y += 20.5f * deltaTime;
    rotate_y = fmodf(rotate_y, 360.0f);


}


// Load textures from image files ( Taken from the online book LearnOpenGL by Joey de Vries )
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


void init()
{
    // Set up the shaders
    shaderFinal = CompileShaders("Shader_Files/Simple_Vertex.txt", "Shader_Files/Simple_Fragment.txt", NULL);
    shaderSimple = CompileShaders("Shader_Files/Tex_Vertex.txt", "Shader_Files/Tex_Fragment.txt", NULL);
    shaderGeo = CompileShaders("Shader_Files/Outline_Vert.txt", "Shader_Files/Outline_Frag.txt", "Shader_Files/Outline_Geo.txt");

    // Load Models
    fence = load_mesh(FENCE);
    fence_vao = generateObjectBufferMesh(fence, shaderSimple);

    box = load_mesh(BOX);
    box_vao = generateObjectBufferMesh(box, shaderSimple);

    ball = load_mesh(BALL);
    ball_vao = generateObjectBufferMesh(ball, shaderSimple);

    rabbit = load_mesh(RABBIT);
    rabbit_vao = generateObjectBufferMesh(rabbit, shaderSimple);

    // Set up Curvemap texture ( uncomment the one you wish to use)
    CurveMap = loadTexture("Textures/chequered.jpg");
 //   CurveMap = loadTexture("Textures/Black_White.jpg");


}

// function to multiple a vector by a float
vec3 vecXfloat(float f, vec3 v1) {

    vec3 result = vec3((v1.v[0] * f), (v1.v[1] * f), (v1.v[2] * f));
    return result;
}


// Mouse Callback function, used to control direction of camera
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {


    float x = static_cast<float>(xposIn);
    float y = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    float xoffset = x - lastX;
    float yoffset = lastY - y;
    lastX = x;
    lastY = y;

    float sensitivity = 0.5f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    vec3 front;
    front.v[0] = cos(radians(yaw)) * cos(radians(pitch));
    front.v[1] = sin(radians(pitch));
    front.v[2] = sin(radians(yaw)) * cos(radians(pitch));

    cameraFront = normalise(front);
    
     
}