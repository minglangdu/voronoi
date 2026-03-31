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
const float LINESIZE = 6.0;
const int DELAY = 20;
const int WIDTH = 700, HEIGHT = 700;
const int PAR_STEP = 3; // pixels per part of parabola

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
        int shader, drawtype, managetype;

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
            this->managetype = managetype;
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
            glBufferData(GL_ARRAY_BUFFER, sz, vertices, managetype);
        }
    protected:
        unsigned int arr, buf; // array object, buffer object
};

class Arc : public VAOH {
    public: 
        float fx, fy;
        float dir; // directrix
        float b1, b2; // two boundary x coordinates arc does not extend past

        Arc(float fx, float fy) : VAOH((([](float x, float y) {
            return std::vector<float> ({x, y, 0.0, 0.0, 0.4, 0.0, 1.0});
        })(fx, fy)).data(), 7 * sizeof(float), 1, 
        GL_LINES, 
        GL_DYNAMIC_DRAW, 1) {
            this->fx = fx; this->fy = fy;
            this->b1 = fx; this->b2 = fx;
            this->dir = fy;
        }
        
        void update() {
            if (dir >= fy) {
                return;
            }
            std::vector<float> cur (0);
            float p = (fy - dir) / 2;
            // placeholder
            b1 = -1.0; b2 = 1.0;
            int camt = 0;
            for (float x = b1; x <= b2; x += std::max((float)0.01, ((float)PAR_STEP / WIDTH))) { 
                // (x - x0)^2 = 4py - 4py0
                // y = 1/4p(x - x0)^2 + y0
                float cy = ((((x - fx) * (x - fx)) / (4 * p)) + (fy - p));
                if (cy > 1.0) {
                    continue;
                }
                camt ++;
                cur.push_back(x); cur.push_back(cy); cur.push_back(0.0);
                cur.push_back(0.0); cur.push_back(0.4); cur.push_back(0.0); cur.push_back(1.0);
            }
            float nvbo[cur.size()];
            std::copy(cur.begin(), cur.end(), nvbo);
            this->vertices = nvbo;
            this->vamt = camt;
            this->sz = cur.size() * sizeof(float);
            glBindBuffer(GL_ARRAY_BUFFER, buf);
            glBufferData(GL_ARRAY_BUFFER, sz, vertices, managetype);
        }
};

class HalfEdge : public VAOH {
    // two half-edges are created when a new site happens, facing in opposite directions
    // directions are horizontal/vertical if they hit the screen's border
    // or perpendicular to the line between the new arc's focus and the focus of the arc it intersects
    // when two half edges meet, if they are parallel a new half edge is created from the intersection point with slope perpendicular to lines between focuses
    // if they aren't parallel they close off a cell. typically at the same time a half edge is created with the other site the intersected half edge is between
    
};

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Voronoi", NULL, NULL); 
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, 2 * WIDTH, 2 * HEIGHT); // full width of window is actually twice the given height and width
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
    glLineWidth(LINESIZE);
    
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

    std::vector<Arc*> arcs; arcs.reserve(POINTS);
    std::vector<HalfEdge*> edges;
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
            if (cur.second.first == 0) {
                // site
                arcs.push_back(new Arc(cur.second.second, cur.first));
            } else {
                // intersection

            }
        }
        tick ++; tick %= DELAY;

        for (Arc* arc : arcs) {
            arc->dir = sweep;
            arc->update();
            arc->draw();
        }
        for (HalfEdge* edge : edges) {
            // placeholder
            edge->update();
            edge->draw();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}