#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "out vec3 ourColor;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4 (aPos, 1.0);\n"
    "   ourColor = aColor;\n"
    "}\0";
const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 ourColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(ourColor, 1.0);\n"
    "}\0";

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id;
    if(type == GL_VERTEX_SHADER)
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main(void)
{
    GLFWwindow* window;
    if (!glfwInit()) // инициализация GLFW
        return -1;
    window = glfwCreateWindow(800, 600, "Window", NULL, NULL);
    if (!window)
    {
        std::cout << "ERROR: failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK)
        std::cout << "ERROR: failed to initialize GLEW" << std::endl;
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    unsigned int shaderProgram = CreateShader(vertexShaderSource, fragmentShaderSource);

    float vertices[] = {
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   
     0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f
    };
    float texCoords[] = { // координаты для текстуры
    0.0f, 0.0f,    
    1.0f, 0.0f,  
    0.5f, 1.0f   
    };
    /*unsigned int indices[] = {  // делали прямоугольник из двух треугольников
        0, 1, 3,  
        1, 2, 3  
    };
    unsigned int EBO;
    glGenBuffers(1, &EBO);*/
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    unsigned int array;
    glGenVertexArrays(1, &array);
    glBindVertexArray(array);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    /*glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);*/
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    while (!glfwWindowShouldClose(window)) // цикл для рисования изображений (пока пользователь не закроет)
    {
        processInput(window); // чтобы закрыть окно с помощью кнопки
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        /*float timeValue = glfwGetTime();
        float greenValue = sin(timeValue) / 2.0f + 0.5f;
        int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
        glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);*/

        glBindVertexArray(array);
        glDrawArrays(GL_TRIANGLES, 0, 3); 
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents(); // проверяет наличие действий с клавиатуры или мыши
    }

    glfwTerminate();
    return 0;
}