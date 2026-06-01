#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>

const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastMouseX = SCR_WIDTH / 2.0f;
float lastMouseY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool filterEnabled = false;
bool fKeyWasPressed = false;
bool cursorWasCaptured = false;

class Shader
{
public:
    unsigned int ID;

    Shader(const char* vertexSource, const char* fragmentSource)
    {
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        checkShader(vertexShader, "VERTEX");

        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        checkShader(fragmentShader, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        checkProgram(ID);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void use() const
    {
        glUseProgram(ID);
    }

    void setBool(const char* name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name), (int)value);
    }

    void setInt(const char* name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name), value);
    }

    void setVec3(const char* name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(value));
    }

    void setVec4(const char* name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(value));
    }

    void setMat4(const char* name, const glm::mat4& value) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(value));
    }

    void Delete() const
    {
        glDeleteProgram(ID);
    }

private:
    void checkShader(unsigned int shader, const char* type)
    {
        int success;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "Blad kompilacji shadera " << type << ":\n" << infoLog << std::endl;
        }
    }

    void checkProgram(unsigned int program)
    {
        int success;
        char infoLog[512];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            std::cout << "Blad linkowania programu shaderow:\n" << infoLog << std::endl;
        }
    }
};

class Camera
{
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    float Yaw;
    float Pitch;
    float Speed;
    float Sensitivity;

    Camera(glm::vec3 position)
        : Position(position),
          Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
          Yaw(-90.0f),
          Pitch(0.0f),
          Speed(3.0f),
          Sensitivity(0.12f)
    {
        updateVectors();
    }

    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void processKeyboard(GLFWwindow* window, float dt)
    {
        float velocity = Speed * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            velocity *= 2.5f;
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            Position += Front * velocity;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            Position -= Front * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            Position -= Right * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            Position += Right * velocity;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            Position += WorldUp * velocity;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            Position -= WorldUp * velocity;
    }

    void processMouse(float xoffset, float yoffset)
    {
        xoffset *= Sensitivity;
        yoffset *= Sensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;

        updateVectors();
    }

private:
    void updateVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

Camera camera(glm::vec3(0.0f, 1.2f, 6.0f));

struct Mesh
{
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    int indexCount;
};

const char* objectVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* objectFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform bool useTexture;
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    vec3 baseColor = objectColor;
    if (useTexture)
    {
        baseColor = texture(texture1, TexCoord).rgb;
    }

    float ambientStrength = 0.20;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.50;
    vec3 viewDirection = normalize(viewPos - FragPos);
    vec3 reflectDirection = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * baseColor;
    FragColor = vec4(result, 1.0);
}
)";

const char* lightVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* lightFragmentShader = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 lightColor;

void main()
{
    FragColor = lightColor;
}
)";

const char* screenVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
)";

const char* screenFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D screenTexture;
uniform bool useFilter;

void main()
{
    if (!useFilter)
    {
        FragColor = texture(screenTexture, TexCoord);
        return;
    }

    vec2 texel = 1.0 / vec2(textureSize(screenTexture, 0));
    vec2 offsets[9] = vec2[](
        vec2(-texel.x,  texel.y), vec2(0.0,  texel.y), vec2(texel.x,  texel.y),
        vec2(-texel.x,  0.0),     vec2(0.0,  0.0),     vec2(texel.x,  0.0),
        vec2(-texel.x, -texel.y), vec2(0.0, -texel.y), vec2(texel.x, -texel.y)
    );

    float kernel[9] = float[](
         1.0,  1.0,  1.0,
         1.0, -8.0,  1.0,
         1.0,  1.0,  1.0
    );

    vec3 edgeColor = vec3(0.0);
    for (int i = 0; i < 9; i++)
    {
        edgeColor += vec3(texture(screenTexture, TexCoord + offsets[i])) * kernel[i];
    }

    edgeColor = vec3(length(edgeColor));
    FragColor = vec4(edgeColor, 1.0);
}
)";

float cubeVertices[] = {
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,

    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,

     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,

    -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f
};

unsigned int cubeIndices[] = {
     0,  1,  2,  2,  3,  0,
     4,  5,  6,  6,  7,  4,
     8,  9, 10, 10, 11,  8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20
};

float pyramidVertices[] = {
    -0.6f, 0.0f, -0.6f,    0.0f, -1.0f,  0.0f,               0.0f, 0.0f,
     0.6f, 0.0f, -0.6f,    0.0f, -1.0f,  0.0f,               1.0f, 0.0f,
     0.6f, 0.0f,  0.6f,    0.0f, -1.0f,  0.0f,               1.0f, 1.0f,
    -0.6f, 0.0f,  0.6f,    0.0f, -1.0f,  0.0f,               0.0f, 1.0f,

    -0.6f, 0.0f, -0.6f,    0.0f,  0.707f, -0.707f,           0.0f, 0.0f,
     0.6f, 0.0f, -0.6f,    0.0f,  0.707f, -0.707f,           1.0f, 0.0f,
     0.0f, 1.1f,  0.0f,    0.0f,  0.707f, -0.707f,           0.5f, 1.0f,

     0.6f, 0.0f, -0.6f,    0.707f, 0.707f,  0.0f,            0.0f, 0.0f,
     0.6f, 0.0f,  0.6f,    0.707f, 0.707f,  0.0f,            1.0f, 0.0f,
     0.0f, 1.1f,  0.0f,    0.707f, 0.707f,  0.0f,            0.5f, 1.0f,

     0.6f, 0.0f,  0.6f,    0.0f,  0.707f,  0.707f,           0.0f, 0.0f,
    -0.6f, 0.0f,  0.6f,    0.0f,  0.707f,  0.707f,           1.0f, 0.0f,
     0.0f, 1.1f,  0.0f,    0.0f,  0.707f,  0.707f,           0.5f, 1.0f,

    -0.6f, 0.0f,  0.6f,   -0.707f, 0.707f,  0.0f,            0.0f, 0.0f,
    -0.6f, 0.0f, -0.6f,   -0.707f, 0.707f,  0.0f,            1.0f, 0.0f,
     0.0f, 1.1f,  0.0f,   -0.707f, 0.707f,  0.0f,            0.5f, 1.0f
};

unsigned int pyramidIndices[] = {
     0,  1,  2,  2,  3,  0,
     4,  5,  6,
     7,  8,  9,
    10, 11, 12,
    13, 14, 15
};

float screenQuadVertices[] = {
    -1.0f,  1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f
};

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processMouseLook(GLFWwindow* window)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (!cursorWasCaptured)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            cursorWasCaptured = true;
            firstMouse = true;
        }

        double xpos;
        double ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (firstMouse)
        {
            lastMouseX = (float)xpos;
            lastMouseY = (float)ypos;
            firstMouse = false;
        }

        float xoffset = (float)xpos - lastMouseX;
        float yoffset = lastMouseY - (float)ypos;
        lastMouseX = (float)xpos;
        lastMouseY = (float)ypos;

        camera.processMouse(xoffset, yoffset);
    }
    else if (cursorWasCaptured)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        cursorWasCaptured = false;
        firstMouse = true;
    }
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    int fState = glfwGetKey(window, GLFW_KEY_F);
    if (fState == GLFW_PRESS && !fKeyWasPressed)
    {
        filterEnabled = !filterEnabled;
    }
    fKeyWasPressed = (fState == GLFW_PRESS);

    camera.processKeyboard(window, deltaTime);
    processMouseLook(window);
}

Mesh createMesh(float* vertices, int vertexBytes, unsigned int* indices, int indexBytes)
{
    Mesh mesh;
    mesh.indexCount = indexBytes / (int)sizeof(unsigned int);

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBytes, indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return mesh;
}

unsigned int createCheckerTexture()
{
    const int textureSize = 64;
    unsigned char pixels[textureSize * textureSize * 3];

    for (int y = 0; y < textureSize; y++)
    {
        for (int x = 0; x < textureSize; x++)
        {
            int index = (y * textureSize + x) * 3;
            bool bright = ((x / 8 + y / 8) % 2) == 0;
            pixels[index + 0] = bright ? 230 : 70;
            pixels[index + 1] = bright ? 190 : 90;
            pixels[index + 2] = bright ? 80 : 130;
        }
    }

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureSize, textureSize, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    return texture;
}

void drawObject(const Shader& shader, const Mesh& mesh, const glm::vec3& position,
                const glm::vec3& scale, const glm::vec3& color, bool useTexture,
                float rotationAngle)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, scale);

    shader.setMat4("model", model);
    shader.setVec3("objectColor", color);
    shader.setBool("useTexture", useTexture);

    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Projekt GK - scena OpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Nie udalo sie utworzyc okna GLFW." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Nie udalo sie zaladowac GLAD." << std::endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    Shader objectShader(objectVertexShader, objectFragmentShader);
    Shader lightShader(lightVertexShader, lightFragmentShader);
    Shader screenShader(screenVertexShader, screenFragmentShader);

    Mesh cubeMesh = createMesh(cubeVertices, sizeof(cubeVertices), cubeIndices, sizeof(cubeIndices));
    Mesh pyramidMesh = createMesh(pyramidVertices, sizeof(pyramidVertices), pyramidIndices, sizeof(pyramidIndices));

    unsigned int checkerTexture = createCheckerTexture();

    unsigned int FBO;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    unsigned int framebufferTexture;
    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

    unsigned int RBO;
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer nie jest kompletny." << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int quadVAO;
    unsigned int quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), screenQuadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    objectShader.use();
    objectShader.setInt("texture1", 0);
    screenShader.use();
    screenShader.setInt("screenTexture", 0);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glm::vec3 lightPos(
            sin(currentFrame) * 3.0f,
            2.0f,
            cos(currentFrame) * 3.0f
        );
        glm::vec4 lightColor(1.0f, 0.95f, 0.82f, 1.0f);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.07f, 0.10f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        objectShader.use();
        objectShader.setMat4("view", view);
        objectShader.setMat4("projection", projection);
        objectShader.setVec3("lightPos", lightPos);
        objectShader.setVec3("viewPos", camera.Position);
        objectShader.setVec3("lightColor", glm::vec3(lightColor));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, checkerTexture);

        drawObject(objectShader, cubeMesh, glm::vec3(-1.8f, 0.6f, 0.0f),
            glm::vec3(1.2f), glm::vec3(1.0f), true, currentFrame * 0.5f);

        drawObject(objectShader, pyramidMesh, glm::vec3(0.4f, 0.0f, -0.8f),
            glm::vec3(1.2f), glm::vec3(0.25f, 0.65f, 0.95f), false, 0.0f);

        drawObject(objectShader, cubeMesh, glm::vec3(2.0f, 0.35f, 0.7f),
            glm::vec3(0.8f, 0.7f, 1.8f), glm::vec3(0.65f, 0.45f, 0.90f), false, -0.35f);

        lightShader.use();
        lightShader.setMat4("view", view);
        lightShader.setMat4("projection", projection);
        lightShader.setVec4("lightColor", lightColor);
        glm::mat4 lightModel(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightModel = glm::scale(lightModel, glm::vec3(0.18f));
        lightShader.setMat4("model", lightModel);
        glBindVertexArray(cubeMesh.VAO);
        glDrawElements(GL_TRIANGLES, cubeMesh.indexCount, GL_UNSIGNED_INT, 0);

       
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        screenShader.use();
        screenShader.setBool("useFilter", filterEnabled);
        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeMesh.VAO);
    glDeleteBuffers(1, &cubeMesh.VBO);
    glDeleteBuffers(1, &cubeMesh.EBO);
    glDeleteVertexArrays(1, &pyramidMesh.VAO);
    glDeleteBuffers(1, &pyramidMesh.VBO);
    glDeleteBuffers(1, &pyramidMesh.EBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteTextures(1, &checkerTexture);
    glDeleteTextures(1, &framebufferTexture);
    glDeleteRenderbuffers(1, &RBO);
    glDeleteFramebuffers(1, &FBO);
    objectShader.Delete();
    lightShader.Delete();
    screenShader.Delete();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
