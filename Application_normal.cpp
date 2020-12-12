#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "stb_image/stb_image.h"
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <map>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static unsigned int CompileShader(unsigned int type, const std::string& source);
static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
unsigned int CreateTexture(const char* path);
void renderQuad();

// äåëàåì êàìåðó
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
bool firstMouse = true;
float lastX = 400, lastY = 300;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float fov = 45.0f;

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

const char* VertexShaderSource = "#version 330 core\n"
    "out vec3 Normal;\n"
    "out vec3 FragPos;\n"
    "out vec2 TexCoords;\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "layout (location = 2) in vec2 aTexCoords;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform mat3 normalMatrix;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0f);\n"
    "   Normal = normalMatrix * aNormal;\n"
    "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "   TexCoords = aTexCoords;\n"
    "}\0";

const char* FragmentShaderSource = "#version 330 core\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "in vec2 TexCoords;\n"
    "out vec4 FragColor;\n"
    "uniform vec3 viewPos;\n"

    "struct Material {\n"
    "   sampler2D diffuse;\n"
    "   sampler2D specular;\n"
    "   float shininess;\n"
    "};\n"
    "uniform Material material;\n"

    "struct Light {\n"
    "   vec3 direction;\n"
    "   vec3 ambient;\n"
    "   vec3 diffuse;\n"
    "   vec3 specular;\n"
    "};\n"
    "uniform Light light;\n"

    "void main()\n"
    "{\n"
    "   vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;\n"

    "   vec3 norm = normalize(Normal);\n"
    "   vec3 lightDir = normalize(-light.direction);\n"
    "   float diff = max(dot(norm, lightDir), 0.0);\n"
    "   vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;\n"
    
    "   vec3 viewDir = normalize(viewPos - FragPos);\n"
    "   vec3 reflectDir = reflect(-lightDir, norm);\n"// -lightDir, òàê êàê reflect äîëæåí ïðèíÿòü
    //                                 âåêòîð, èìåþùèé íàïðàâëåíèå îò èñòî÷íèêà ñâåòà ê îáúåêòó
    "   float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
    //  shininess - çíà÷åíèå áëåñêà (÷åì áîëüøå çíà÷åíèå, òåì ìåíüøå ðàññåèâàåòñÿ áëåñê)
    "   vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;\n"

    "   vec3 result = ambient + diffuse + specular;\n"
    "   FragColor = vec4(result, 1.0f);\n"
    "}\0";

const char* LightFragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0);\n"
    "}\0";

const char* BorderFragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.04, 0.28, 0.26, 1.0);\n"
    "}\0";

const char* BlendingFragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoords;\n"
    "uniform sampler2D texture1;\n"
    "void main()\n"
    "{\n"
    "    FragColor = texture(texture1, TexCoords);\n"
    "}\0";

const char* NormalVertexShaderSource = "#version 330 core\n"
    "out vec3 FragPos;\n"
    "out vec2 TexCoords;\n"
    "out vec3 TangentLightPos;\n"
    "out vec3 TangentViewPos;\n"
    "out vec3 TangentFragPos;\n"
    "out vec3 TangentLightDir;\n"

    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "layout (location = 2) in vec2 aTexCoords;\n"
    "layout (location = 3) in vec3 aTangent;\n"
    "layout (location = 4) in vec3 aBitangent;\n"

    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 lightDir;\n"
    "uniform vec3 viewPos;\n"

    "void main()\n"
    "{\n"\
    "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "   TexCoords = aTexCoords;\n"
    "   mat3 normalMatrix = transpose(inverse(mat3(model)));\n"
    "   vec3 T = normalize(normalMatrix * aTangent);\n"
    "   vec3 N = normalize(normalMatrix * aNormal);\n"
    "   T = normalize(T - dot(T, N) * N);\n"
    "   vec3 B = cross(N, T);\n"
    "   mat3 TBN = transpose(mat3(T, B, N));\n"
    "   TangentViewPos  = TBN * viewPos;\n"
    "   TangentFragPos  = TBN * FragPos;\n"
    "   TangentLightDir = TBN * lightDir;\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0f);\n"
    "}\0";

const char* NormalFragmentShaderSource = "#version 330 core\n"
    "in vec3 FragPos;\n"
    "in vec2 TexCoords;\n"
    "in vec3 TangentViewPos;\n"
    "in vec3 TangentFragPos;\n"
    "in vec3 TangentLightDir;\n"
    "out vec4 FragColor;\n"

    "uniform vec3 viewPos;\n"

    "struct Material {\n"
    "   sampler2D diffuse;\n"
    "   sampler2D normalMap;\n"
    "   float shininess;\n"
    "};\n"
    "uniform Material material;\n"

    "struct Light {\n"
    "   vec3 ambient;\n"
    "   vec3 diffuse;\n"
    "   vec3 specular;\n"
    "};\n"
    "uniform Light light;\n"

    "void main()\n"
    "{\n"
    "   vec3 normal = texture(material.normalMap, TexCoords).rgb;\n"
    "   normal = normalize(normal * 2.0f - 1.0f);\n"

    "   vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;\n"
    
    "   vec3 lightDir = normalize(-TangentLightDir);\n"
    "   float diff = max(dot(normal, lightDir), 0.0);\n"
    "   vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;\n"

    "   vec3 viewDir = normalize(TangentViewPos - TangentFragPos);\n"
    "   vec3 reflectDir = reflect(-lightDir, normal);\n"// -lightDir, òàê êàê reflect äîëæåí ïðèíÿòü
    //                                 âåêòîð, èìåþùèé íàïðàâëåíèå îò èñòî÷íèêà ñâåòà ê îáúåêòó
    "   float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
    //  shininess - çíà÷åíèå áëåñêà (÷åì áîëüøå çíà÷åíèå, òåì ìåíüøå ðàññåèâàåòñÿ áëåñê)
    "   vec3 specular = light.specular * spec;\n"
    "   FragColor = vec4(ambient + diffuse + specular, 1.0f);\n"
    "}\0";

int main(void)
{
    GLFWwindow* window;
    if (!glfwInit()) // èíèöèàëèçàöèÿ GLFW
        return -1;
    window = glfwCreateWindow(800, 600, "Window", NULL, NULL);
    if (!window) {
        std::cout << "ERROR: failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK)
        std::cout << "ERROR: failed to initialize GLEW" << std::endl;
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE); // äëÿ óâåëè÷åíèÿ ðàçìåðà ÷àñòèö

    unsigned int shaderProgram = CreateShader(VertexShaderSource, FragmentShaderSource);
    //unsigned int lightingShaderProgram = CreateShader(VertexShaderSource, LightFragmentShaderSource);
    unsigned int borderShaderProgram = CreateShader(VertexShaderSource, BorderFragmentShaderSource);
    unsigned int blendingShaderProgram = CreateShader(VertexShaderSource, BlendingFragmentShaderSource);
    unsigned int normalShaderProgram = CreateShader(NormalVertexShaderSource, NormalFragmentShaderSource);

    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };
    float planeVertices[] = {
        // positions                        // texture Coords
         5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

         5.0f, -0.5f,  5.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        -5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
         5.0f, -0.5f, -5.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f
    };
    float palmVertices[] = {
        // positions                        // texture Coords (swapped y coordinates because texture is flipped upside down)
        0.0f,  1.5f,  0.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f,
        0.0f, -0.5f,  0.0f, 0.0f,  0.0f, -1.0f, 0.0f,  1.0f,
        1.0f, -0.5f,  0.0f, 0.0f,  0.0f, -1.0f, 1.0f,  1.0f,

        0.0f,  1.5f,  0.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f,
        1.0f, -0.5f,  0.0f, 0.0f,  0.0f, -1.0f, 1.0f,  1.0f,
        1.0f,  1.5f,  0.0f, 0.0f,  0.0f, -1.0f, 1.0f,  0.0f
    };   
    // êóáèê
    unsigned int cubeVBO, cubeVAO;
    glGenBuffers(1, &cubeVBO);
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // ïëîñêîñòü
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    //ïàëüìû
    unsigned int palmVAO, palmVBO;
    glGenVertexArrays(1, &palmVAO);
    glGenBuffers(1, &palmVBO);
    glBindVertexArray(palmVAO);
    glBindBuffer(GL_ARRAY_BUFFER, palmVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(palmVertices), &palmVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    std::string diffuseCube = "./src/marble.jpg"; // êóá
    unsigned int diffuseMapCube = CreateTexture(diffuseCube.c_str());
    std::string specularCube = "./src/white.jpg";
    unsigned int specularMapCube = CreateTexture(specularCube.c_str());
    std::string diffusePlane = "./src/sand.jpg";
    unsigned int diffuseMapPlane = CreateTexture(diffusePlane.c_str()); // ïëîñêîñòü
    std::string specularPlane = "./src/sand_specular.jpg";
    unsigned int specularMapPlane = CreateTexture(specularPlane.c_str());
    std::string palm = "./src/palm.png"; // ïàëüìà
    unsigned int palmTexture = CreateTexture(palm.c_str());
    std::string half_palm1Path = "./src/half_palm1.png";
    unsigned int halfpalmTexture1 = CreateTexture(half_palm1Path.c_str());
    std::string half_palm2Path = "./src/half_palm2.png";
    unsigned int halfpalmTexture2 = CreateTexture(half_palm2Path.c_str());
    std::string diffuseStone = "./src/stoneDiffuse.jpeg"; // êàìåíü
    unsigned int diffuseMapStone = CreateTexture(diffuseStone.c_str());
    std::string normalStone = "./src/stoneNormal.png";
    unsigned int normalMapStone = CreateTexture(normalStone.c_str());

    std::vector<glm::vec3> palmsPositions
    {
        glm::vec3(-1.5f, 0.0f, -0.48f),
        glm::vec3(1.5f, 0.0f, 0.51f),
        glm::vec3(0.20001f, 0.0f, -2.3001f), // 1 (2 ïîëîâèíà äåðåâà)
        glm::vec3(0.2f, 0.0f, -3.3f), // 2 (1 ïîëîâèíà äåðåâà)
        glm::vec3(-0.8f, 0.0f, -2.3f), // 3 (1 ïîëîâèíà äåðåâà)
        glm::vec3(0.2f, 0.0f, -1.3f), // 4 (2 ïîëîâèíà äåðåâà)
    };

    while (!glfwWindowShouldClose(window)) // öèêë äëÿ ðèñîâàíèÿ èçîáðàæåíèé (ïîêà ïîëüçîâàòåëü íå çàêðîåò)
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window); // ÷òîáû çàêðûòü îêíî ñ ïîìîùüþ êíîïêè

        // ñîðòèðóåì îêíà, ÷òîáû ïîòîì âûâåñòè â îáðàòíîì ïîðÿäêå
        std::map<float, glm::vec3> sorted; 
        for (unsigned int i = 0; i < palmsPositions.size(); i++)
        {
            float distance = glm::length(cameraPos - palmsPositions[i]);
            sorted[distance] = palmsPositions[i];
        }

        glClearColor(0.560784f, 0.905882f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(shaderProgram);

        unsigned int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);

        glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), 64.0f);

        // ñâîéñòâà ñâåòîâîãî èñòî÷íèêà
        glUniform3f(glGetUniformLocation(shaderProgram, "light.direction"), 
            -0.2f, -1.0f, -0.3f); // ìèíóñ, ò.ê. íàïðàâëåíèå ÎÒ èñòî÷íèêà ñâåòà
        glUniform3f(glGetUniformLocation(shaderProgram, "light.ambient"),
            0.2f, 0.2f, 0.2f);
        glUniform3f(glGetUniformLocation(shaderProgram, "light.diffuse"),
            0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(shaderProgram, "light.specular"),
            1.0f, 1.0f, 1.0f);

        glm::mat4 view = glm::mat4(1.0f);
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 100.0f);
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        glUniform1i(glGetUniformLocation(shaderProgram, "material.diffuse"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.specular"), 1);

        glUseProgram(borderShaderProgram);
        viewLoc = glGetUniformLocation(borderShaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        projectionLoc = glGetUniformLocation(borderShaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glUseProgram(shaderProgram);

        // ðèñóåì ïîë
        glStencilMask(0x00);
        glBindVertexArray(planeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapPlane);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMapPlane);
        glm::mat4 model = glm::mat4(1.0f);
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
        glm::mat3 normal = glm::mat3(glm::transpose(glm::inverse(model)));
        unsigned int normalLoc = glGetUniformLocation(shaderProgram, "normalMatrix");
        glUniformMatrix3fv(normalLoc, 1, GL_FALSE, glm::value_ptr(normal));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // ðèñóåì ñàìè êóáèêè
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapCube);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMapCube);
        model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
        //modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // óâåëè÷èâàåì êóáèêè, äëÿ òîãî ÷òîáû íàðèñîâàòü îáâîäêó
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(borderShaderProgram);
        float scale = 1.1;
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapCube);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(scale, scale, scale));
        modelLoc = glGetUniformLocation(borderShaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(scale, scale, scale));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glEnable(GL_DEPTH_TEST);

        //ðèñóåì êàìåíü
        glUseProgram(normalShaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapStone);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMapStone);
        glUniform3f(glGetUniformLocation(normalShaderProgram, "viewPos"), 
            cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform1f(glGetUniformLocation(normalShaderProgram, "material.shininess"), 256.0f);
        glUniform3f(glGetUniformLocation(normalShaderProgram, "lightDir"),
            -0.2f, -1.0f, -0.3f); // ìèíóñ, ò.ê. íàïðàâëåíèå ÎÒ èñòî÷íèêà ñâåòà
        glUniform3f(glGetUniformLocation(normalShaderProgram, "light.ambient"),
            0.2f, 0.2f, 0.2f);
        glUniform3f(glGetUniformLocation(normalShaderProgram, "light.diffuse"),
            0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(normalShaderProgram, "light.specular"),
            1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(normalShaderProgram, "view"), 
            1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(normalShaderProgram, "projection"), 
            1, GL_FALSE, glm::value_ptr(projection));
        glUniform1i(glGetUniformLocation(normalShaderProgram, "material.diffuse"), 0);
        glUniform1i(glGetUniformLocation(normalShaderProgram, "material.normalMap"), 1);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-3.0f, -0.49f, -3.0f));
        glUniformMatrix4fv(glGetUniformLocation(normalShaderProgram, "model"),
            1, GL_FALSE, glm::value_ptr(model));
        renderQuad();

        // ðèñóåì ïàëüìû
        glUseProgram(blendingShaderProgram);
        glUniform1i(glGetUniformLocation(blendingShaderProgram, "texture1"), 0);
        glBindVertexArray(palmVAO);
        glActiveTexture(GL_TEXTURE0);
        viewLoc = glGetUniformLocation(blendingShaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        projectionLoc = glGetUniformLocation(blendingShaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        // ðèñóåì îò äàëüíåãî ê áëèæíåìó
        for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            if (it->second == glm::vec3(0.2f, 0.0f, -3.3f)) {
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            }
            if (it->second == glm::vec3(0.2f, 0.0f, -1.3f)) {
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            }
            if (it->second == glm::vec3(-1.5f, 0.0f, -0.48f) || it->second == glm::vec3(1.5f, 0.0f, 0.51f)) {
                glBindTexture(GL_TEXTURE_2D, palmTexture);
            }
            if (it->second == glm::vec3(0.2f, 0.0f, -3.3f) || it->second == glm::vec3(-0.8f, 0.0f, -2.3f) || it->second == glm::vec3(0.2f, 0.0f, -1.3f)) {
                glBindTexture(GL_TEXTURE_2D, halfpalmTexture1);
            }
            if (it->second == glm::vec3(0.20001f, 0.0f, -2.3001f)) {
                glBindTexture(GL_TEXTURE_2D, halfpalmTexture2);
            }
            modelLoc = glGetUniformLocation(blendingShaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glfwSwapBuffers(window);
        glfwPollEvents(); // ïðîâåðÿåò íàëè÷èå äåéñòâèé ñ êëàâèàòóðû èëè ìûøè
    }
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    const float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id;
    if (type == GL_VERTEX_SHADER)
        id = glCreateShader(GL_VERTEX_SHADER);
    else
        id = glCreateShader(GL_FRAGMENT_SHADER);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)malloc(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "ERROR: failed to compile " <<
            (type == GL_VERTEX_SHADER ? "vertex" : "fragment") <<
            " shader!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    int result;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)malloc(length * sizeof(char));
        glGetProgramInfoLog(program, length, &length, message);
        std::cout << "ERROR: failed to link shader program!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

unsigned int CreateTexture(const char* path) 
{
    unsigned int texture;
    glGenTextures(1, &texture);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        std::cout << "ERROR: failed to load texture!" << std::endl;
    }
    stbi_image_free(data);
    return texture;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        // positions
        glm::vec3 pos1(-1.0f, 0.0f, 1.0f);
        glm::vec3 pos2(-1.0f, 0.0f, -1.0f);
        glm::vec3 pos3(1.0f, 0.0f, -1.0f);
        glm::vec3 pos4(1.0f, 0.0f, 1.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 1.0f, 0.0f);

        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);


        float quadVertices[] = {
            // positions            // normal         // texcoords  // tangent                          // bitangent
            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
            pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
            pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
