#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <queue>

const int POINTS = 10;
const float POINTSIZE = 15.0;
const int DELAY = 80;

unsigned int sprogram1;

const char* vsh1 = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 vcol;

out vec4 ccol;
void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    ccol = vcol;
}
)";

const char* fsh1 = R"(
#version 330 core
in vec4 ccol;  
out vec4 col;
void main() {
    col = ccol;
}
)";

struct comp {
    bool operator() (std::pair<float, std::pair<int, float>> a, std::pair<float, std::pair<int, float>> b) {
        if (a.first == b.first) {
            return a.second.first == 0;
        }
        return a.first < b.first;
    }
};

std::priority_queue<std::pair<float, std::pair<int, float>>, std::vector<std::pair<float, std::pair<int, float>>>, comp> q; // {y coordinate, {type, x coordinate}}
// type - 0 -> site event, 1 -> intersection event

float sweep = 1.0;

class VAOH {
    public: 
        float* vertices;
        int sz, vamt;
        int shader, drawtype;

        VAOH(float vertices[], int sz, int vamt, int drawtype, int managetype, int shader=1) {
            /*
            vertices -> float only for now, could be converted to a template to change
            sz -> size of vertices array (sizeof() doesn't work)
            vamt -> amount of vertices
            drawtype -> GL_POINTS, GL_LINES, etc. 
            managetype -> GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW
            shader -> what shader program to use. currently only one (sprogram1)
            */
            glGenVertexArrays(1, &arr); glBindVertexArray(arr);
            glGenBuffers(1, &buf); 
            glBindBuffer(GL_ARRAY_BUFFER, buf);
            glBufferData(GL_ARRAY_BUFFER, sz, vertices, managetype);
            switch (shader) {
                case 1:
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(0));
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
                    glEnableVertexAttribArray(1);
                    break;
                default:
                    throw std::invalid_argument("Shader not found");
                    break;
            }
            this->vertices = vertices;
            this->shader = shader;
            this->drawtype = drawtype;
            this->sz = sz;
            this->vamt = vamt;
        }
        ~VAOH() {
            delete vertices;
            glDeleteBuffers(1, &buf);
            glDeleteVertexArrays(1, &arr);
        }

        void draw() {
            switch (shader) {
                case 1:
                    glUseProgram(sprogram1);
                    break;
                default:
                    throw std::invalid_argument("Shader not found");
                    break;
            }
            glBindVertexArray(arr);
            glDrawArrays(drawtype, 0, vamt);
        }
        void update() {
            // meant for updating VBO after manually changing vertices variable
            glBindBuffer(GL_ARRAY_BUFFER, buf);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sz, vertices);
        }
    private:
        unsigned int arr, buf; // array object, buffer object
};

class Arc : public VAOH {
    float fx, fy;
    float dir; // directrix
    float b1, b2; // two boundary x coordinates arc does not extend past
    Arc(float fx, float fy) : VAOH((([](float x, float y) {
        return std::vector<float> ({x, y, 0.0, 0.0, 1.0, 0.0, 1.0});
    })(fx, fy)).data(), 7 * sizeof(float), 1, GL_LINE_STRIP, GL_DYNAMIC_DRAW, 1) {
        this->fx = fx; this->fy = fy;
        this->b1 = fx; this->b2 = fx;
        this->dir = fy;
    }
};

class Edge : public VAOH {

};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(700, 700, "Voronoi", NULL, NULL);
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    int wi, he; glfwGetWindowSize(window, &wi, &he);
    glViewport(0, 0, 2 * wi, 2 * he); // full width of window is actually twice the given height and width
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int x, int y) {glViewport(0, 0, x, y);});

    // shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vsh1, NULL);
    glCompileShader(vertexShader);
    unsigned int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fsh1, NULL);
    glCompileShader(fragShader);
    sprogram1 = glCreateProgram();
    glAttachShader(sprogram1, vertexShader); glAttachShader(sprogram1, fragShader);
    glLinkProgram(sprogram1); 
    glDeleteShader(vertexShader); glDeleteShader(fragShader); 

    // misc configurations
    glPointSize(POINTSIZE);

    
    std::random_device rd; 
    std::mt19937 mt (rd()); 
    std::uniform_real_distribution<float> dist(-1.0, 1.0);
    float points[7 * POINTS];
    for (int i = 0; i < POINTS; i ++) {
        float x = dist(mt), y = dist(mt);
        q.push({y, {0, x}});
        points[7 * i] = x;
        points[7 * i + 1] = y;
        points[7 * i + 2] = 0.0;
        points[7 * i + 3] = 0.0; 
        points[7 * i + 4] = 0.0;
        points[7 * i + 5] = 1.0; 
        points[7 * i + 6] = 1.0;
    } 
    VAOH* sites = new VAOH(points, sizeof(points), POINTS, GL_POINTS, GL_STATIC_DRAW, 1);

    float lvert[7 * 2] = {-1.0, sweep, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, sweep, 0.0, 1.0, 0.0, 0.0, 1.0}; // red vertical line
    VAOH* sline = new VAOH(lvert, sizeof(lvert), 2, GL_LINES, GL_DYNAMIC_DRAW, 1);

    int tick = 0; 
    while (!glfwWindowShouldClose(window)) {

        glClearColor(0.9, 0.9, 0.9, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        sites->draw();

        sline->draw();
        
        if (tick == 0 && !q.empty()) {
            auto cur = q.top(); q.pop();
            sweep = cur.first;
            // update VBO
            lvert[1] = sweep; lvert[8] = sweep;
            sline->vertices = lvert;
            sline->update();
        }
        tick ++; tick %= DELAY;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    delete sites; delete sline;
    glfwTerminate();
    return 0;
}