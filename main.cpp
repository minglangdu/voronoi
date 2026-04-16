#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <set>

const int POINTS = 4;
const float POINTSIZE = 15.0;
const float LINESIZE = 10.0;
// const int DELAY = 20;
const int DELAY = 150;
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
struct Arc;

struct Event {
    float x, y;
    int type; // 0 -> site; 1 -> circle (3) event; 2 -> two parabola intersection; 3 -> edge intersection
    Arc* a1, *a2, *a3;

    Event(float ax, float ay, int t, Arc* a1, Arc* a2, Arc* a3) {
        this->x = ax; this->y = ay;
        std::cout << ax << " " << ay << "\n";
        this->type = t;
        this->a1 = a1; this->a2 = a2; this->a3 = a3;
    }

    bool operator<(const Event& o) const {
        if (y == o.y) {
            return type > o.type;
        }
        return y > o.y;
    }
};

// set instead of priority queue in order to be able to invalidate obsolete intersection events
std::set<Event> q; 
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

class HalfEdge : public VAOH {
    /*
    two half-edges are created when a new site happens, facing in opposite directions
    directions are horizontal/vertical if they hit the screen's border
    or perpendicular to the line between the new arc's focus and the focus of the arc it intersects
    when two half edges meet, a new half edge is created from the intersection point with slope perpendicular to lines between focuses
    and the previous half edges are fixed in place. 

    To check for edge intersection events, we look at every pair of edges that is adjacent when sorted by x coordinate of point furthest from starting position
    When a new edge is placed between two previously adjacent edges, we remove the previous edge intersection event and add its intersections in. 

    The speed of the edge is not stored because it is variable and very precise. Instead, only the slope will be used. Each edge intersection location
    will be found and the one with the highest directrix will be chosen first. 

    Each arc is assigned two edges where the current positions of the moving end of the edge define b1 and b2. This causes each arc to move and shrink accordingly. 
    */
    public: 
        float sx, sy; // starting coordinates
        float cx, cy; // current location
        float dx, dy; // slope

        HalfEdge(float sx, float sy, float dx, float dy) : VAOH((([](float x, float y) {
                return std::vector<float> ({x, y, 0.0, 1.0, 0.0, 1.0, 1.0, x, y, 0.0, 1.0, 0.0, 1.0, 1.0});
            })(sx, sy)).data(), 14 * sizeof(float), 2, 
            GL_LINES, 
            GL_DYNAMIC_DRAW, 1) {
                this->sx = sx; this->sy = sy;
                this->cx = sx; this->cy = sy;
                this->dx = dx; this->dy = dy;
        }
        HalfEdge() : HalfEdge(0.0, 0.0, 1.0, 1.0) {}

        void update() {
            float narr[14] = {sx, sy, 0.0, 1.0, 0.0, 1.0, 1.0, cx, cy, 0.0, 1.0, 0.0, 1.0, 1.0};
            this->vertices = narr;
            VAOH::update();
        }
};

class Arc : public VAOH {
    public: 
        float fx, fy;
        float dir; // directrix
        float b1, b2; // two boundary x coordinates arc does not extend past

        // two boundary edges defining b1 and b2
        HalfEdge *h1, *h2; 

        Arc(float fx, float fy, HalfEdge* e1, HalfEdge* e2) : VAOH((([](float x, float y) {
            return std::vector<float> ({x, y, 0.0, 0.0, 0.4, 0.0, 1.0, x, 1.0, 0.0, 0.0, 0.4, 1.0});
        })(fx, fy)).data(), 7 * sizeof(float), 1, 
        GL_LINE_STRIP, 
        GL_DYNAMIC_DRAW, 1) {
            this->fx = fx; this->fy = fy;
            this->b1 = fx; this->b2 = fx;
            this->dir = fy;
            this->h1 = e1;
            this->h2 = e2;
        }
        
        void update() {
            if (dir == fy) {
                // straight vertical line
                float nvbo[] = {fx, fy, 0.0, 0.0, 0.4, 0.0, 1.0, fx, h1->sy, 0.0, 0.0, 0.4, 1.0}; // can use h2->sy too
                this->vertices = nvbo;
                this->vamt = 2; 
                this->sz = sizeof(nvbo);
                VAOH::update();
                return;
            }
            if (dir > fy) {
                return;
            }
            std::vector<float> cur (0);
            float p = (fy - dir) / 2;

            int camt = 0;
            // std::cout << "b1b2" << b1 << " " << b2 << "\n";
            for (float x = b1; x <= b2; x += std::max((float)0.01, ((float)PAR_STEP / WIDTH))) { 
                // (x - x0)^2 = 4py - 4py0
                // y = 1/4p(x - x0)^2 + y0 (y0 = fy - p, x0 = fx)
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
            VAOH::update();
        }

        std::pair<float, float> extend() {
            // find new b1 and b2 given current dir
            // b1 -> y = slope * (x - sx) + sy; 
            // (x - x0)^2 = 4py - 4py0
            // (1)(x^2) + (-2x0 - 4p*slope)(x) + (4py0 + 4p*slope*sx + x0^2 - 4p*sy) = 0
            // x < sx1
            // for b1, x = (-b - \sqrt{b^2 - 4c})/2
            // for b2, x = (-b + \sqrt{b^2 - 4c})/2 (different values for b, c)
            if (dir == fy) {
                b1 = fx; b2 = fx;
                h1->cx = fx; h1->cy = h1->sy + (h1->dy / h1->dx) * (b1 - h1->sx);
                h2->cx = fx; h2->cy = h2->sy + (h2->dy / h2->dx) * (b2 - h2->sx);
                return {b1, b2};
            }
            float p = (fy - dir) / 2;
            float y0 = (fy - p);

            float slope = h1->dy / h1->dx;
            float b = -2 * fx - 4 * p * slope; 
            float c = 4 * p * y0 + 4 * p * slope * h1->sx + fx * fx - 4 * p * h1->sy;
            b1 = (-b - std::sqrt(b * b - 4 * c)) / 2.0;

            slope = h2->dy / h2->dx;
            c = 4 * p * y0 + 4 * p * slope * h2->sx + fx * fx - 4 * p * h2->sy;
            b2 = (-b + std::sqrt(b * b - 4 * c)) / 2.0;

            h1->cx = b1;
            h1->cy = h1->sy + (h1->dy / h1->dx) * (b1 - h1->sx);

            h2->cx = b2;
            h2->cy = h2->sy + (h2->dy / h2->dx) * (b2 - h2->sx);
            if (h1->dy != 0 || h2->dy != 0) 
            std::cout << "extend (" << b1 << ", " << h1->cy << ") (" << b2 << ", " << h2->cy << ")\n";
            return {b1, b2};
        }

        void draw() {
            VAOH::draw();
            this->h1->update();
            this->h2->update();
            this->h1->draw();
            this->h2->draw();
        }

        bool operator<(const Arc* o) {
            return b1 < o->b1;
        }
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
        q.insert(Event(x, y, 0, NULL, NULL, NULL));
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
    int tick = 0; 
    while (!glfwWindowShouldClose(window)) {

        glClearColor(0.9, 0.9, 0.9, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        sites->draw();

        sline->draw();

        if (tick == 0 && !q.empty()) {
            auto cur = *q.begin(); q.erase(q.begin());
            sweep = cur.y;
            // TODO: use a red-black tree instead of an arc vector so that you only need to call extend() on log N sites not all of them 
            for (Arc* arc : arcs) {
                std::cout << "extend arc\n";
                arc->dir = sweep;
                arc->extend(); // not optimal
                arc->update();
            }
            sort(arcs.begin(), arcs.end());

            // update VBO
            lvert[1] = sweep; lvert[8] = sweep;
            sline->vertices = lvert;
            sline->update();
            if (cur.type == 0) {
                // site
                Arc* above = NULL; // the arc it is above
                Arc* na = new Arc(cur.x, cur.y, new HalfEdge(), new HalfEdge()); // placeholder edges
                auto nxt = std::lower_bound(arcs.begin(), arcs.end(), na);

                if (nxt != arcs.end()) std::cout << (*nxt)->b1 << " " << (*nxt)->b2 << " " << cur.x << "\n";

                if ((nxt != arcs.end()) && (*nxt)->b1 <= cur.x && (*nxt)->b2 >= cur.x) {
                    above = *nxt;
                } else if (nxt != arcs.begin()) {
                    nxt = prev(nxt);
                    if ((*nxt)->b1 <= cur.x && (*nxt)->b2 >= cur.x) {
                        above = *nxt;
                    }
                }
                if (above != NULL) {
                    // TODO: doesn't work if the line is vertical. 
                    float p = (above->fy - sweep) / 2;
                    float cy = ((((cur.x - above->fx) * (cur.x - above->fx)) / (4 * p)) + (above->fy - p));
                    float dx = above->fx - na->fx, dy = above->fy - na->fy;
                    std::swap(dx, dy); dx *= -1; // make perpendicular 
                    if (dx < 0) {
                        // dx *= -1; dy *= -1;
                    }
                    na->h1 = new HalfEdge(cur.x, cy, -dx, -dy);
                    na->h2 = new HalfEdge(cur.x, cy, dx, dy);
                    std::cout << cur.x << " " << cy << "\n";
                    std::cout << "dxdy" << dx << " " << dy << "\n";
                    // remove top arc and add two arcs that have new left and right half edges + old edges as borders
                    Arc* larc = new Arc(above->fx, above->fy, above->h1, na->h1);
                    Arc* rarc = new Arc(above->fx, above->fy, na->h2, above->h2);
                    std::cout << "larc, rarc done\n";
                    arcs.erase(remove(arcs.begin(), arcs.end(), above));
                    std::cout << "removed above arc\n";
                    arcs.push_back(larc); arcs.push_back(rarc);
                } else {
                    // two edges on top border
                    na->h1 = new HalfEdge(cur.x, 1.0, -1.0, 0.0);
                    na->h2 = new HalfEdge(cur.x, 1.0, 1.0, 0.0);
                    std::cout << cur.x << " " << 1.0 << "\n";
                }
                std::cout << "sorting\n";
                arcs.push_back(na);
                sort(arcs.begin(), arcs.end());
                auto npos = find(arcs.begin(), arcs.end(), na); // TODO: speed up with binary tree

                // check if added edges will invalidate previous events

                if (above != NULL && std::next(npos, -1) != arcs.begin() && std::next(npos, 2) != arcs.end()) { // potential undefined behavior
                    // current position: {?L, larc, na, rarc, ?R}
                    // checks if ?L and ?R exist
                    // if they do, then the circle event that closes off `above` arc should be invalidated

                } else if (above == NULL && npos != arcs.begin() && std::next(npos, 1) != arcs.end()) { // remove two site intersection event

                } else if (false) { // left edge screen invalidation

                } else if (false) { // right edge screen invalidation

                }

                // TODO: add new circle events and two site
                if (arcs.size() >= 3 && above != NULL) {
                    // two circle events: (site left of above, larc, na), (na, rarc, site right of above)
                    if (std::next(npos, -1) != arcs.begin()) { // left circle

                    } else if (npos != arcs.begin()) { // left two site

                    }
                    if (false) { // right circle

                    } else if (false) { // right two site

                    }
                }
                if (false) { // left edge screen

                } 
                if (false) { // right edge screen

                }
            } else {
                // intersection

            }
        }
        tick ++; tick %= DELAY;

        for (Arc* arc : arcs) {
            arc->dir = sweep;
            arc->extend();
            arc->update();
            arc->draw();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}