#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <iostream>
#include <sstream>

unsigned int shaderProgram{};
void GLAPIENTRY DebugMessageCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    [[maybe_unused]] GLsizei length,
    const GLchar *message,
    [[maybe_unused]] const void *userParam);
glm::mat4 rotationMatrix = glm::mat4(1.0f);

constexpr auto vertexShaderSource = R"(
    #version 330 core
    
    layout (location = 0) in vec3 aPos;
    uniform mat4 rotationMatrix;

    void main()
    {
        gl_Position = rotationMatrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
)";

constexpr auto fragmentShaderSource = R"(
    #version 330 core

    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
)";

// vertices duplicated for GL_LINES
constexpr auto squareVertices = std::array{
    -0.5f, 0.5f, 0.0f,  // Top-left
    0.5f, 0.5f, 0.0f,   // Top-right
    0.5f, 0.5f, 0.0f,   // Top-right
    0.5f, -0.5f, 0.0f,  // Bottom-right
    0.5f, -0.5f, 0.0f,  // Bottom-right
    -0.5f, -0.5f, 0.0f, // Bottom-left
    -0.5f, -0.5f, 0.0f, // Bottom-left
    -0.5f, 0.5f, 0.0f   // Top-left
};

void framebufferSizeChanged(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window, float deltaTime);
void render(GLFWwindow *window);

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    const GLFWvidmode *VideoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    const auto ScreenHeight = VideoMode->height;

    GLFWwindow *window = glfwCreateWindow(ScreenHeight / 2, ScreenHeight / 2,
                                          "OpenGL + GLFW", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

#ifndef __APPLE__
    // glDebugMessageCallback only available in OpenGL 4.3 and later, but macOS is 4.1
    glDebugMessageCallback(DebugMessageCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "Vertex Shader Compilation Failed:" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cout << "Fragment Shader Compilation Failed:" << infoLog << std::endl;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "Shader Program Linking Failed: %s" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int VBO, VAO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 squareVertices.size() * sizeof(float),
                 squareVertices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glfwSetFramebufferSizeCallback(window, framebufferSizeChanged);

    float lastFrameStartTime = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrameStartTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameStartTime - lastFrameStartTime;
        lastFrameStartTime = currentFrameStartTime;

        processInput(window, deltaTime);
        render(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void framebufferSizeChanged(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    render(window);
}

void processInput(GLFWwindow *window, float deltaTime)
{
    const float rotationSpeedDegreesPerSecond = 90.0f;
    const float rotationAnglePerFrame = rotationSpeedDegreesPerSecond * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        rotationMatrix = glm::rotate(rotationMatrix,
                                     glm::radians(rotationAnglePerFrame),
                                     glm::vec3(0.0f, 0.0f, 1.0f));
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        rotationMatrix = glm::rotate(rotationMatrix,
                                     glm::radians(-rotationAnglePerFrame),
                                     glm::vec3(0.0f, 0.0f, 1.0f));
    }
}

void render(GLFWwindow *window)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    auto location = glGetUniformLocation(shaderProgram, "rotationMatrix");
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(rotationMatrix));

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(squareVertices.size() / 3));

    glfwSwapBuffers(window);
}

void GLAPIENTRY DebugMessageCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    [[maybe_unused]] GLsizei length,
    const GLchar *message,
    [[maybe_unused]] const void *userParam)
{
    // Ignore certain verbose info messages (particularly ones on Nvidia).
    if (id == 131169 ||
        id == 131185 || // NV: Buffer will use video memory
        id == 131218 ||
        id == 131204 || // Texture cannot be used for texture mapping
        id == 131222 ||
        id == 131154 || // NV: pixel transfer is synchronized with 3D rendering
        id == 0         // gl{Push, Pop}DebugGroup
    )
        return;

    std::stringstream debugMessageStream;
    debugMessageStream << message << '\n';

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        debugMessageStream << "Source: API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        debugMessageStream << "Source: Window Manager";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        debugMessageStream << "Source: Shader Compiler";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        debugMessageStream << "Source: Third Party";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        debugMessageStream << "Source: Application";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        debugMessageStream << "Source: Other";
        break;
    }

    debugMessageStream << '\n';

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        debugMessageStream << "Type: Error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        debugMessageStream << "Type: Deprecated Behaviour";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        debugMessageStream << "Type: Undefined Behaviour";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        debugMessageStream << "Type: Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        debugMessageStream << "Type: Performance";
        break;
    case GL_DEBUG_TYPE_MARKER:
        debugMessageStream << "Type: Marker";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        debugMessageStream << "Type: Push Group";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        debugMessageStream << "Type: Pop Group";
        break;
    case GL_DEBUG_TYPE_OTHER:
        debugMessageStream << "Type: Other";
        break;
    }

    debugMessageStream << '\n';

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        debugMessageStream << "Severity: high";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        debugMessageStream << "Severity: medium";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        debugMessageStream << "Severity: low";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        debugMessageStream << "Severity: notification";
        break;
    }

    std::cerr << "OpenGL Message: " << type << " " << debugMessageStream.str() << "\n";
}