#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

#include <epoxy/gl.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "vecgl.h"

#define PI (0x1.921fb54442d18469898cc51701b839a2p1)
#define FOV 45.0f

using namespace celestia;

static int offset = -1;
static int diffTex = -1;

constexpr int N = 9;
constexpr int kMaxLevel = 15;
/*
 * N            9      36
 * screensize level  level
 *   >= 2048    4      2
 *   >= 1024    3      1
 *   >=  256    2      0
 *   >=   64    1     -1
 *      else    0     -2
 */


GLuint shader = 0;

static const char VertexShader[] = R"glsl(
uniform vec2 offset;

const float PI = 3.1415927;
void main(void)
{
/*
    gl_TexCoord[0].st = vec2(1.0 - (gl_MultiTexCoord0.s + offset.x * 0.5), gl_MultiTexCoord0.t + offset.y);

    vec2 modelPos = gl_Vertex.xy + offset;

    float theta = (modelPos.x - 0.5) * PI; // longitude
    float phi   = (0.5 - modelPos.y) * PI; // latitude

    float sphi   = sin(phi);
    float cphi   = cos(phi);
    float stheta = sin(theta);
    float ctheta = cos(theta);

    vec4 pos = vec4(cphi * ctheta, sphi, cphi * stheta, 1.0);
    gl_Position = gl_ModelViewProjectionMatrix * pos;
*/
    gl_TexCoord[0].st = gl_MultiTexCoord0.st;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
)glsl";

static const char FragmentShader[] = R"glsl(
uniform sampler2D diffTex;

void main(void)
{
    vec4 diff = texture2D(diffTex, gl_TexCoord[0].st);
    gl_FragColor = diff;
}
)glsl";


Eigen::Vector4f
MapToSphere(double x, double y)
{
    //fmt::print("MapToSphere({}, {})\n", x, y);
    double theta = (x - 0.5) * double(PI); // longitude
    double phi   = (0.5 - y) * double(PI); // latitude

    double sphi, cphi, stheta, ctheta;
    sincos(phi, &sphi, &cphi);
    sincos(theta, &stheta, &ctheta);

    return Eigen::Vector4d(cphi * ctheta, sphi, cphi * stheta, 1.0).cast<float>();
}

float
CalculateScreenSize(const Eigen::Matrix4f &mvp, int w, int h)
{
    Eigen::Vector4f v1 = mvp * MapToSphere(0.5, 1.0);
    Eigen::Vector4f v0 = mvp * MapToSphere(0.5, 0.0);
    Eigen::Vector2f ndc1(v1.head(2) / v1.w());
    Eigen::Vector2f ndc0(v0.head(2) / v0.w());
    Eigen::Vector2f dev(static_cast<float>(w), static_cast<float>(h));
    // ac - bc <=> (a - b)c
    return 0.5f * (ndc1 - ndc0).cwiseProduct(dev).norm();
}

class QuadTree;

class Frustum
{
public:
    Frustum() = default;

    Frustum(const Eigen::Matrix4f &m)
    {
        Eigen::Vector4f n;

        // Left clipping plane
        n = m.col(3) + m.col(0);
        p[0] = Eigen::Hyperplane<float, 3>(n.head(3), n.w());

        // Right clipping plane
        n = m.col(3) - m.col(0);
        p[1] = Eigen::Hyperplane<float, 3>(n.head(3), n.w());

        // Bottom clipping plane
        n = m.col(3) + m.col(1);
        p[2] = Eigen::Hyperplane<float, 3>(n.head(3), n.w());

        // Top clipping plane
        n = m.col(3) - m.col(1);
        p[3] = Eigen::Hyperplane<float, 3>(n.head(3), n.w());

        // Near clipping plane
        n = m.col(3) + m.col(2);
        p[4] = Eigen::Hyperplane<float, 3>(n.head(3), n.w());

        // Far clipping plane
        n = m.col(3) - m.col(2);
        p[5] = Eigen::Hyperplane<float, 3>(n.head(3), n.w());

        for (auto &plane : p)
            plane.normalize();
    }

    bool isInside(const Eigen::Vector3f &v) const
    {
        return std::all_of(std::begin(p), std::end(p), [&v](const auto &plane) { return plane.signedDistance(v) >= 0; });
    }

private:
    std::array<Eigen::Hyperplane<float, 3>, 6> p;
};

class Patch
{
public:
    Patch(double width, double height);
    ~Patch() = default;

    void render(double offsetX, double offsetY) const;

private:
    struct Vertex
    {
        float pos[2];
        float texCoord[2];
    };

    double m_width;
    double m_height;

    GLuint m_vbo { 0 };
    static GLuint m_ibo;
    static int m_idxCount;
    std::vector<Vertex> vertices;

public:
    static int rendered;
};

int Patch::rendered = 0;
GLuint Patch::m_ibo = 0;
int Patch::m_idxCount = 0;

class Node
{
public:
    Node(QuadTree *qt, int level, double startX, double startY, double endX, double endY);
    ~Node() = default;

    const Patch*    getPatch()  const;
    bool            visible()   const;
    void            render(int level);

    void debug() const;

private:
    void split();
    bool splitable() const;
    float checkDirection() const;

    std::array<std::unique_ptr<Node>, 4> m_nodes;

    QuadTree *m_qt;

    double m_startX;
    double m_startY;
    double m_endX;
    double m_endY;

    std::array<Eigen::Vector4f, 4> m_mapped;

    int m_level;
};

class QuadTree
{
public:
    QuadTree(double startX, double startY, double endX, double endY);

//    const Node* getNode()  const { return m_node.get(); }

//    double getError() const { return m_error; }
//    void setError(double error) { m_error = error; }

    void setMVPMatrix(const Eigen::Matrix4f&);
    void setModelViewMatrix(const Eigen::Matrix4f& m) { m_modelViewMatrix = m; }
    void setFrustum(const Frustum&);
    const Eigen::Matrix4f& getMVPMatrix() const { return m_mvp; }
    const Frustum& getFrustum() const { return m_frustum; }
    void setCameraPosition(const Eigen::Vector3f& p) { m_cameraPosition = p; }
    Eigen::Vector3f getCameraPosition() const { return m_cameraPosition; }
    Eigen::Matrix3f getCameraMatrix() const { return m_modelViewMatrix.topLeftCorner(3,3); }

    void render(int level) const;

    void debug() const { /*m_node->debug();*/ }

private:
    const Patch* getPatch(int level) const;
    const Patch* createPatch(int level, double width, double height);

    Eigen::Matrix4f m_mvp;
    Eigen::Matrix4f m_modelViewMatrix;
    Eigen::Vector3f m_cameraPosition;
    Frustum         m_frustum;

    std::vector<std::unique_ptr<Patch>> m_patches;

    std::unique_ptr<Node> m_nodeFront;
    std::unique_ptr<Node> m_nodeRear;

    friend class Node;
};

QuadTree::QuadTree(double startX, double startY, double endX, double endY) :
    m_nodeFront(std::make_unique<Node>(this, 0, startX, startY, endX/2.0, endY)),
    m_nodeRear(std::make_unique<Node>(this, 0, endX/2.0, startY, endX, endY))
{
}

const Patch*
QuadTree::getPatch(int level) const
{
    if (m_patches.size() < level + 1)
        return nullptr;

    return m_patches[level].get();
}

const Patch*
QuadTree::createPatch(int level, double width, double height)
{
    fmt::print("Level {}: {}x{}\n", level, width, height);

    if (m_patches.size() < level + 1)
    {
        while (m_patches.size() < level)
            m_patches.emplace_back(nullptr);
        m_patches.emplace_back(std::make_unique<Patch>(width, height));
    }

    if (auto *patch = m_patches[level].get(); patch != nullptr)
        return patch;

    m_patches[level] = std::make_unique<Patch>(width, height);
    return m_patches[level].get();
}

void
QuadTree::render(int level) const
{
    m_nodeFront->render(level);
    m_nodeRear->render(level);
}

void
QuadTree::setMVPMatrix(const Eigen::Matrix4f &mvp)
{
    m_mvp = mvp;
}

void
QuadTree::setFrustum(const Frustum &frustum)
{
    m_frustum = frustum;
}

Node::Node(QuadTree *qt, int level, double startX, double startY, double endX, double endY) :
    m_qt(qt),
    m_level(level),
    m_startX(startX),
    m_startY(startY),
    m_endX(endX),
    m_endY(endY)
{
    //fmt::print("{} {}\n", m_startX,m_endX);
    m_mapped[0] = MapToSphere(m_startX, m_startY);
    m_mapped[1] = MapToSphere(m_startX, m_endY);
    m_mapped[2] = MapToSphere(m_endX, m_startY);
    m_mapped[3] = MapToSphere(m_endX, m_endY);
}

void
Node::render(int level)
{
    if (!visible())
        return;

#if 0
    float dir = checkDirection();

    fmt::print("[{} {}] -> {}\n", m_startX, m_startY, dir);

    if (dir > 0.0f) // rear side of the sphere. 0.5 is cos(60deg)
        return;
#endif

    if (level == m_level)
    {
        auto patch = m_qt->getPatch(m_level);
        if (patch == nullptr)
            patch = m_qt->createPatch(m_level, m_endX - m_startX, m_endY - m_startY);
        patch->render(m_startX, m_startY);
        //glUseProgram(0);
        //glBegin(GL_LINE_LOOP);
        //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        //std::for_each(std::begin(m_mapped), std::end(m_mapped), [](const auto &v) { glVertex4fv(v.data()); });
        //glEnd();
        //glUseProgram(shader);
        return;
    }

#if 0
    if ((level == m_level + 1) && !visible())
        return;
#endif

    split();
    std::for_each(m_nodes.begin(), m_nodes.end(), [level](const auto &node) { node->render(level); });
}

void
Node::split()
{
    // endY     +---------+---------+
    //          |    2    |    3    |
    // halfY    +---------+---------+
    //          |    0    |    1    |
    // startY   +---------+---------+
    //        startX    halfX      endX
    double halfX = (m_startX + m_endX) * 0.5;
    double halfY = (m_startY + m_endY) * 0.5;
    int    level = m_level + 1;

    m_nodes[0] = std::make_unique<Node>(m_qt, level, m_startX, m_startY, halfX, halfY);
    m_nodes[1] = std::make_unique<Node>(m_qt, level, halfX, m_startY, m_endX, halfY);
    m_nodes[2] = std::make_unique<Node>(m_qt, level, m_startX, halfY, halfX, m_endY);
    m_nodes[3] = std::make_unique<Node>(m_qt, level, halfX, halfY, m_endX, m_endY);
}

bool
Node::splitable() const
{
    //return (m_endX - m_startX > m_qt->getError()) && (m_endY - m_startY > m_qt->getError());
    return true;
}

const Patch*
Node::getPatch() const
{
    return m_qt->getPatch(m_level);
}

bool check_visibility(const Eigen::Matrix4f &mvp, const Eigen::Vector4f &point)
{
    Eigen::Vector4f pos = mvp * point;
    //fmt::print("{}\n", pos.transpose());
    float w = 1.1f * pos.w();
    return (pos.x() >= -w && pos.x() <= w && pos.y() >= -w && pos.y() <= w);
}

bool
Node::visible() const // TODO: check patch normal direction
{
    if (m_level == 0)
        return true;

    const Eigen::Matrix4f &mvp = m_qt->getMVPMatrix();

    return check_visibility(mvp, m_mapped[0]) || check_visibility(mvp, m_mapped[1])
           || check_visibility(mvp, m_mapped[2]) || check_visibility(mvp, m_mapped[3]);
}

float
Node::checkDirection() const
{
    Eigen::Vector3f line1((m_mapped[0] - m_mapped[3]).head(3));
    Eigen::Vector3f line2((m_mapped[1] - m_mapped[2]).head(3));
    Eigen::Vector3f normal          = line1.cross(line2);
    Eigen::Vector3f cameraDirection = m_qt->getCameraPosition().normalized() * -1.0f;

    return (m_qt->getCameraMatrix() * normal).dot(cameraDirection);
}

void
Node::debug() const
{
    bool _split = splitable();
    //fmt::print("start=[{}, {}], end=[{}, {}], level={}, split={}.\n", m_startX, m_startY, m_endX, m_endY, m_level, _split);

    if (_split)
    {
        for (const auto &node : m_nodes)
            if (node != nullptr) node->debug();
    }
}

Patch::Patch(double width, double height) :
    m_width(width),
    m_height(height)
{
    //std::vector<Vertex> vertices;
    vertices.reserve(N*N);

    double stepX = m_width / static_cast<double>(N);
    double stepY = m_height / static_cast<double>(N);

    int i = 0;
    for (double y = 0.0; y < m_height + stepY * 0.1; y += stepY)
    {
        for (double x = 0.0; x < m_width + stepX * 0.1; x += stepX)
        {
            vertices.push_back({{(float) x, (float) y}, {float(0.5 * x), float(y)}});
        }
    }

    glGenBuffers(1, &m_vbo);
    //glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    //glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);

    if (m_ibo == 0)
    {
        std::vector<unsigned short> indices;
        indices.reserve(2*N*(N+1)+N);
        for (int i = 0; i < N; i++)
        {
            int baseVertex = i * (N+1);

            if (i > 0)
                indices.push_back(baseVertex);
            for (int j = 0; j <= N; j++)
            {
                int v = baseVertex + j;
                indices.push_back(v);
                indices.push_back(v + N + 1);
            }
            if (i < N-1)
                indices.push_back(indices.back());
        }
        glGenBuffers(1, &m_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
        m_idxCount = indices.size();
    }
}

void
Patch::render(double offsetX, double offsetY) const
{
#if 1
    struct MappedVtx
    {
        Eigen::Vector4f pos;
        Eigen::Vector2f texCoord;
    };

    std::vector<MappedVtx> mapped;
    mapped.reserve(vertices.size());

    for (const auto &v : vertices)
    {
        Eigen::Vector4f pos = MapToSphere(v.pos[0] + offsetX, v.pos[1] + offsetY);
        Eigen::Vector2f texCoord(1.0f - (v.texCoord[0] + offsetX * 0.5f), v.texCoord[1] + offsetY);
        mapped.push_back({pos, texCoord});
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, mapped.size()*sizeof(mapped[0]), mapped.data(), GL_STREAM_DRAW);
#endif

    //glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

    //float v[2] = { (float) offsetX, (float) offsetY };
    //glUniform2fv(offset, 1, v);
    glUniform1i(diffTex, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    const int stride = sizeof(MappedVtx);
    glVertexPointer(4, GL_FLOAT, stride, (void*)offsetof(MappedVtx, pos));
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, stride, (void*) offsetof(MappedVtx, texCoord));
    glDrawElements(GL_TRIANGLE_STRIP, m_idxCount, GL_UNSIGNED_SHORT, nullptr);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    rendered++;
}

template<typename T> inline constexpr T degToRad(T d)
{
    return d / static_cast<T>(180) * (T)PI;
}

/*! Return an perspective projection matrix
 */
template<class T> Eigen::Matrix<T, 4, 4>
Perspective(T fovy, T aspect, T nearZ, T farZ)
{
    using std::cos, std::sin;

    if (aspect == static_cast<T>(0))
        return Eigen::Matrix<T, 4, 4>::Identity();

    T deltaZ = farZ - nearZ;
    if (deltaZ == static_cast<T>(0))
        return Eigen::Matrix<T, 4, 4>::Identity();

    T angle = degToRad(fovy / static_cast<T>(2));
    T sine = sin(angle);
    if (sine == static_cast<T>(0))
        return Eigen::Matrix<T, 4, 4>::Identity();
    T ctg = cos(angle) / sine;

    Eigen::Matrix<T, 4, 4> m = Eigen::Matrix<T, 4, 4>::Identity();
    m(0, 0) = ctg / aspect;
    m(1, 1) = ctg;
    m(2, 2) = -(farZ + nearZ) / deltaZ;
    m(2, 3) = static_cast<T>(-2) * nearZ * farZ / deltaZ;
    m(3, 2) = static_cast<T>(-1);
    m(3, 3) = static_cast<T>(0);
    return m;
}

class SDL_Application
{
 public:
    SDL_Application() = delete;
    SDL_Application(std::string_view name, int w, int h) :
        m_appName       { name },
        m_windowWidth   { w },
        m_windowHeight  { h }
    {
            m_qt = std::make_unique<QuadTree>(0.0, 0.0, 2.0, 1.0);
    }
    ~SDL_Application();

    static std::shared_ptr<SDL_Application> init(std::string_view, int, int);

    bool createOpenGLWindow();

    bool initCelestiaCore();
    void run();
    std::string_view getError() const;

 private:
    void display();
    float getAspectRatio() const;
    void updateGL();

    // handlers
    void handleTextInputEvent(const SDL_TextInputEvent &event);
    void handleKeyPressEvent(const SDL_KeyboardEvent &event);
    void handleKeyReleaseEvent(const SDL_KeyboardEvent &event);
    void handleMousePressEvent(const SDL_MouseButtonEvent &event);
    void handleMouseReleaseEvent(const SDL_MouseButtonEvent &event);
    void handleMouseWheelEvent(const SDL_MouseWheelEvent &event);
    void handleMouseMotionEvent(const SDL_MouseMotionEvent &event);
    void handleWindowEvent(const SDL_WindowEvent &event);

    // aux functions
    void toggleFullscreen();
    void copyURL();
    void pasteURL();
    void configure() const;

    // state variables
    std::string m_appName;
    int m_windowWidth;
    int m_windowHeight;

    int m_lastX                 { 0 };
    int m_lastY                 { 0 };
    bool m_cursorVisible        { true };
    bool m_fullscreen           { false };

    int m_level                 { 0 };
    bool m_distChanged          { false };

    SDL_Window   *m_mainWindow  { nullptr };
    SDL_GLContext m_glContext   { nullptr };

    GLenum m_mode               { GL_FILL };

    std::unique_ptr<QuadTree> m_qt;

    Eigen::Matrix4f m_projection { Eigen::Matrix4f::Identity() };
    Eigen::Matrix4f m_modelView  { Eigen::Matrix4f::Identity() };

    Eigen::Vector3f m_cameraPos{ 0.0f, 0.0f, -3.0f };
};

std::shared_ptr<SDL_Application>
SDL_Application::init(std::string_view name, int w, int h)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return nullptr;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifdef GL_ES
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
    return std::make_shared<SDL_Application>(name, w, h);
}

SDL_Application::~SDL_Application()
{
    //m_qt->debug();
    if (m_glContext != nullptr)
        SDL_GL_DeleteContext(m_glContext);
    if (m_mainWindow != nullptr)
        SDL_DestroyWindow(m_mainWindow);
    SDL_Quit();
}

bool
SDL_Application::createOpenGLWindow()
{
    m_mainWindow = SDL_CreateWindow(m_appName.c_str(),
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    m_windowWidth,
                                    m_windowHeight,
                                    SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    if (m_mainWindow == nullptr)
        return false;

    m_glContext = SDL_GL_CreateContext(m_mainWindow);
    if (m_glContext == nullptr)
        return false;

    // First try to enable adaptive sync and then vsync
    if (SDL_GL_SetSwapInterval(-1) == -1)
        SDL_GL_SetSwapInterval(1);
    return true;
}

void
SDL_Application::run()
{
    updateGL();

    SDL_StartTextInput();

    bool quit = false;
    while (!quit)
    {
        m_distChanged = false;
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0)
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_TEXTINPUT:
                handleTextInputEvent(event.text);
                break;
            case SDL_KEYDOWN:
                handleKeyPressEvent(event.key);
                break;
            case SDL_KEYUP:
                handleKeyReleaseEvent(event.key);
                break;
            case SDL_MOUSEBUTTONDOWN:
                handleMousePressEvent(event.button);
                break;
            case SDL_MOUSEBUTTONUP:
                handleMouseReleaseEvent(event.button);
                break;
            case SDL_MOUSEWHEEL:
                handleMouseWheelEvent(event.wheel);
                break;
            case SDL_MOUSEMOTION:
                handleMouseMotionEvent(event.motion);
                break;
            case SDL_WINDOWEVENT:
                handleWindowEvent(event.window);
                break;
            default:
                break;
            }
        }
        display();
    }
}

float
SDL_Application::getAspectRatio() const
{
    return static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
}

void
SDL_Application::updateGL()
{
    m_projection = Perspective(FOV, getAspectRatio(), 0.0f, 1.0f);
    m_modelView = Eigen::Matrix4f::Identity() * vecgl::translate(m_cameraPos);
    m_qt->setCameraPosition(m_cameraPos);
    m_qt->setModelViewMatrix(Eigen::Matrix4f::Identity());
    Eigen::Matrix4f mvp = m_projection * m_modelView;
    m_qt->setMVPMatrix(mvp);
    m_qt->setFrustum(Frustum(m_projection * Eigen::Matrix4f::Identity().transpose()));

    //fmt::print("{}\n", CalculateScreenSize(mvp, m_windowWidth, m_windowHeight));

    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m_projection.data());
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m_modelView.data());
}

void
SDL_Application::display()
{
    int oldPatches = Patch::rendered;
    Patch::rendered = 0;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_qt->render(m_level);
    SDL_GL_SwapWindow(m_mainWindow);

    if (oldPatches != Patch::rendered || m_distChanged)
    {
        fmt::print(
            "Rendered {} patches at level {}, distance {}, screensize {}\n",
            Patch::rendered,
            m_level,
            m_cameraPos.norm(),
            CalculateScreenSize(m_projection * m_modelView, m_windowWidth, m_windowHeight));
    }
}

float
step(float z)
{
    if (z < -5)
        return 1.0f;
    if (z < -3)
        return 0.1f;
    if (z < -2)
        return 0.02f;
    if (z < -1.5)
        return 0.01f;
    if (z < -1.3)
        return 0.005f;
    if (z < -1.2)
        return 0.002f;
    if (z < -1.1)
        return 0.001f;
    return 0.0005f;

}

void
SDL_Application::handleKeyPressEvent(const SDL_KeyboardEvent &event)
{
    switch (event.keysym.sym)
    {
    case SDLK_w:
        m_mode = (m_mode == GL_FILL ? GL_LINE : GL_FILL);
        glPolygonMode(GL_FRONT_AND_BACK, m_mode);
        break;
    case SDLK_KP_PLUS:
        m_level = std::min(kMaxLevel, m_level+1);
        break;
    case SDLK_KP_MINUS:
        m_level = std::max(0, m_level-1);
        break;
    case SDLK_UP:
        m_cameraPos.z() += step(m_cameraPos.z());
        m_distChanged = true;
        updateGL();
        break;
    case SDLK_DOWN:
        m_cameraPos.z() -= step(m_cameraPos.z());
        m_distChanged = true;
        updateGL();
        break;
    case SDLK_LEFT:
        m_cameraPos.x() -= 0.1f;
        m_distChanged = true;
        updateGL();
        break;
    case SDLK_RIGHT:
        m_cameraPos.x() += 0.1f;
        m_distChanged = true;
        updateGL();
        break;
    case SDLK_PAGEDOWN:
        m_cameraPos.y() -= 0.1f;
        m_distChanged = true;
        updateGL();
        break;
    case SDLK_PAGEUP:
        m_cameraPos.y() += 0.1f;
        m_distChanged = true;
        updateGL();
        break;
    case SDLK_LEFTBRACKET:
        break;
    case SDLK_RIGHTBRACKET:
        break;
    default:
        break;
    }
#if 0
    switch (event.keysym.sym)
    {
    case SDLK_TAB:
    case SDLK_BACKSPACE:
    case SDLK_DELETE:
    case SDLK_ESCAPE:
        m_appCore->charEntered(event.keysym.sym, 0);
    case SDLK_RETURN:
        return;
    }

    int key = toCelestiaKey(event.keysym.sym);
    if (key == -1)
        return;

    int mod = 0;
    if ((event.keysym.mod & KMOD_CTRL) != 0)
    {
        int k = std::tolower(key);
        if (k >= 'a' && k <= 'z')
        {
            switch (k)
            {
            case 'c':
                copyURL();
                break;
            case 'v':
                pasteURL();
                break;
            default:
                key = k + 1 - 'a';
                m_appCore->charEntered(key, mod);
            }
            return;
        }
        mod |= CelestiaCore::ControlKey;
    }
    if ((event.keysym.mod & KMOD_SHIFT) != 0)
        mod |= CelestiaCore::ShiftKey;

    m_appCore->keyDown(key, mod);
#endif
}

void
SDL_Application::handleKeyReleaseEvent(const SDL_KeyboardEvent &event)
{
#if 0
    if (event.keysym.sym == SDLK_RETURN)
    {
        if ((event.keysym.mod & KMOD_ALT) != 0)
            toggleFullscreen();
        else
            m_appCore->charEntered(SDLK_RETURN, 0);
    }
    else
    {
        int key = toCelestiaKey(event.keysym.sym);
        if (key == -1)
            return;
        int mod = 0;
        if ((event.keysym.mod & KMOD_CTRL) != 0)
        {
            mod |= CelestiaCore::ControlKey;
            if (key >= '0' && key <= '9')
                m_appCore->charEntered(key, mod);
        }
        if ((event.keysym.mod & KMOD_SHIFT) != 0)
            mod |= CelestiaCore::ShiftKey;

        m_appCore->keyUp(key, 0);
    }
#endif
}

void
SDL_Application::handleTextInputEvent(const SDL_TextInputEvent &event)
{
    //m_appCore->charEntered(event.text, 0);
}

#if 0
static int
toCelestiaButton(int button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT:
        return CelestiaCore::LeftButton;
    case SDL_BUTTON_MIDDLE:
        return CelestiaCore::MiddleButton;
    case SDL_BUTTON_RIGHT:
        return CelestiaCore::RightButton;
    default:
        return -1;
    }
}
#endif

void
SDL_Application::handleMousePressEvent(const SDL_MouseButtonEvent &event)
{
#if 0
    m_lastX = event.x;
    m_lastY = event.y;
    m_appCore->mouseButtonDown(event.x, event.y, button);
#endif
}

void
SDL_Application::handleMouseReleaseEvent(const SDL_MouseButtonEvent &event)
{
#if 0
    int button = toCelestiaButton(event.button);;
    if (button == -1)
        return;

    if (button & (CelestiaCore::LeftButton | CelestiaCore::RightButton))
    {
        if (!m_cursorVisible)
        {
            SDL_ShowCursor(SDL_ENABLE);
            m_cursorVisible = true;
            SDL_WarpMouseInWindow(m_mainWindow, m_lastX, m_lastY);
        }
    }
    m_lastX = event.x;
    m_lastY = event.y;
    m_appCore->mouseButtonUp(event.x, event.y, button);
#endif
}

void
SDL_Application::handleMouseWheelEvent(const SDL_MouseWheelEvent &event)
{
#if 0
    if (event.y > 0) // scroll up
        m_appCore->mouseWheel(-1.0f, 0);
    else if (event.y < 0) // scroll down
        m_appCore->mouseWheel(1.0f, 0);
#endif
}

void
SDL_Application::handleMouseMotionEvent(const SDL_MouseMotionEvent &event)
{
#if 0
    if (event.state & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK))
    {
        int buttons = 0;
        if (event.state & SDL_BUTTON_LMASK)
            buttons |= CelestiaCore::LeftButton;
        if (event.state & SDL_BUTTON_RMASK)
            buttons |= CelestiaCore::RightButton;

        int x = event.x - m_lastX;
        int y = event.y - m_lastY;
        if (m_cursorVisible)
        {
            SDL_ShowCursor(SDL_DISABLE);
            m_cursorVisible = false;
            m_lastX = event.x;
            m_lastY = event.y;
        }
        SDL_WarpMouseInWindow(m_mainWindow, m_lastX, m_lastY);
    }
#endif
}

void
SDL_Application::handleWindowEvent(const SDL_WindowEvent &event)
{
    switch (event.event)
    {
    case SDL_WINDOWEVENT_RESIZED:
        m_windowWidth  = event.data1;
        m_windowHeight = event.data2;
        updateGL();
        break;
    default:
        break;
    }
}


int main()
{
    auto app = SDL_Application::init("quadtree", 640, 480);

    if (!app->createOpenGLWindow())
    {
        return 1;
    }

    GLint status;

    GLuint vsid = glCreateShader(GL_VERTEX_SHADER);
    const char *v[] = { VertexShader };
    glShaderSource(vsid, 1, v, nullptr);
    glCompileShader(vsid);
    glGetShaderiv(vsid, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        fmt::print("Failed to compile vertex shader!\n");
        return 1;
    }

    GLuint fsid = glCreateShader(GL_FRAGMENT_SHADER);
    const char *f[] = { FragmentShader };
    glShaderSource(fsid, 1, f, nullptr);
    glCompileShader(fsid);
    glGetShaderiv(fsid, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        fmt::print("Failed to compile fragment shader!\n");
        return 1;
    }

    GLuint id = glCreateProgram();
    glAttachShader(id, vsid);
    glAttachShader(id, fsid);

    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        fmt::print("Failed to link program!\n");
        return 1;
    }

#if 0
    offset = glGetUniformLocation(id, "offset");
    if (offset == -1)
    {
        fmt::print("Failed to get `offset` uniform location!\n");
        return 1;
    }
#endif

    diffTex = glGetUniformLocation(id, "diffTex");
    if (diffTex == -1)
    {
        fmt::print("Failed to get `diffTex` uniform location!\n");
        return 1;
    }

    glUseProgram(id);
    shader = id;
    glFrontFace(GL_CW);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    const char texturePath[] = "../../textures/medres/earth.png";
    SDL_Surface *surface = IMG_Load(texturePath);
    if (surface == nullptr)
    {
        fmt::print("Failed to load image {}!\n", texturePath);
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
#if 1
    glGenerateMipmap(GL_TEXTURE_2D);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif
    if (glGetError() != 0)
    {
        fmt::print("Failed to create texture from {}!\n", texturePath);
    }
    SDL_FreeSurface(surface);

    app->run();
}
