#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <random>

const int POINTS = 5;
const float POINTSIZE = 10.0;

const char* vsh = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 vcol;

out vec4 ccol;
void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    ccol = vcol;
}
)";

const char* fsh = R"(
#version 330 core
in vec4 ccol;  
out vec4 col;
void main() {
    col = ccol;
}
)";

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Voronoi", NULL, NULL);
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int x, int y) {glViewport(0, 0, x, y);});

    // init shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vsh, NULL);
    glCompileShader(vertexShader);
    unsigned int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fsh, NULL);
    glCompileShader(fragShader);
    unsigned int sprogram = glCreateProgram();
    glAttachShader(sprogram, vertexShader); glAttachShader(sprogram, fragShader);
    glLinkProgram(sprogram); 
    glDeleteShader(vertexShader); glDeleteShader(fragShader); 

    glPointSize(POINTSIZE);

    std::random_device rd; 
    std::mt19937 mt (rd()); 
    std::uniform_real_distribution<float> dist(-1, 1);
    float points[7 * POINTS];
    for (int i = 0; i < POINTS; i ++) {
        points[7 * i] = dist(mt);
        points[7 * i + 1] = dist(mt);
        points[7 * i + 2] = 0.0;
        points[7 * i + 3] = 0.0;
        points[7 * i + 4] = 0.0;
        points[7 * i + 5] = 1.0;
        points[7 * i + 6] = 1.0;
    }
    // vertex arrays
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    unsigned int VBO;
    glGenBuffers(1, &VBO); 
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    while (!glfwWindowShouldClose(window)) {

        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(sprogram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, POINTS);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}