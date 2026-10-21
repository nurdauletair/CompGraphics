// =====================================================================
//  СӨЖ (СРС) — TRAFFIC RACER 2D
//
//  Ойын: шексіз жол, трафик арасымен жүру. Төрт жолақ:
//    сол жақ екі жолақ — қарсы бетте келе жатқан машиналар,
//    оң жақ екі жолақ — өзіңмен бір бағыттағы (баяу) машиналар.
//
//  Басқару:
//    A / D  немесе  ← / →   — бұрылу
//    W / ↑                  — газ (жылдамдау)
//    S / ↓                  — тежегіш
//    R                      — соқтығысқаннан кейін қайта бастау
//    ESC                    — шығу
//
//  Ұпай: жүрген жол + басып озған әр машина (+10) + "near miss" бонусы
//        (машинаның жанынан өте жақын өтсең +25, қарсыдан келгенге +40).
//
//  Негіз: main.cpp (6-апта) — шейдер, Mesh (VAO+VBO+EBO), uOffset,
//  uAspect, delta time. Жаңасы: uScale енді vec2, uTint және uAlpha.
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;
const float ASPECT = (float)WIDTH / (float)HEIGHT;   // Y: -1..1, X: -ASPECT..ASPECT

// Жол
const float ROAD_HALF = 0.80f;                        // жолдың жарты ені
const float LANE_X[4] = { -0.6f, -0.2f, 0.2f, 0.6f };// жолақ ортасы. 0,1 — қарсы; 2,3 — бір бағыт

// Машина өлшемі (жергілікті: ені 0.22, ұзындығы 0.42, алды +Y жақта)
const float HIT_HW = 0.095f;     // соқтығысу қорабының жарты ені (сәл кіші — әділ болу үшін)
const float HIT_HH = 0.190f;     // жарты ұзындығы

// Ойыншы
const float PLAYER_Y = -0.55f;

// Жылдамдық (дүние бірлігі/сек)
const float V_MIN = 0.8f;
const float V_MAX = 2.6f;
const float KMH   = 70.0f;       // экранда көрсету үшін коэффициент

// Жолдағы үзік сызықтар мен ағаштар (қайталанатын тізбек)
const float DASH_P = 0.5f;
const float TREE_P = 0.9f;

// ---------------------------------------------------------------------
//  Шейдерлер
//  uScale — vec2 (x пен y бөлек масштабтау, теріс мән = бұру)
//  uTint  — түсті көбейткіш (ақ фигураны кез келген түске бояуға болады)
//  uAlpha — мөлдірлік (қараңғы қабат үшін)
// ---------------------------------------------------------------------
const char* vertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform vec2  uOffset;
uniform vec2  uScale;
uniform float uAspect;

out vec3 vColor;

void main() {
    vec2 p = aPos.xy * uScale + uOffset;
    gl_Position = vec4(p.x / uAspect, p.y, 0.0, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(#version 330 core
in vec3 vColor;
uniform vec3  uTint;
uniform float uAlpha;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor * uTint, uAlpha);
}
)";

unsigned int compileStage(GLenum type, const char* src, const char* name) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, 1024, nullptr, log);
        std::cerr << name << " шейдер қатесі:\n" << log << "\n";
    }
    return id;
}

unsigned int makeShader(const char* vsSrc, const char* fsSrc) {
    unsigned int vs = compileStage(GL_VERTEX_SHADER,   vsSrc, "VERTEX");
    unsigned int fs = compileStage(GL_FRAGMENT_SHADER, fsSrc, "FRAGMENT");

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    int ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "Линковка қатесі:\n" << log << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ---------------------------------------------------------------------
//  Mesh — VAO + VBO + EBO (5-апта)
// ---------------------------------------------------------------------
struct Mesh {
    unsigned int vao = 0, vbo = 0, ebo = 0;
    int count = 0;
};

Mesh makeMesh(const std::vector<float>& verts, const std::vector<unsigned int>& idx) {
    Mesh m;
    m.count = (int)idx.size();

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);   // EBO-ны 0-ге босатпаймыз!
    return m;
}

void destroyMesh(Mesh& m) {
    glDeleteVertexArrays(1, &m.vao);
    glDeleteBuffers(1, &m.vbo);
    glDeleteBuffers(1, &m.ebo);
}

// --- фигура құрастыру көмекшілері ---
void addVertex(std::vector<float>& v, float x, float y, float r, float g, float b) {
    v.push_back(x); v.push_back(y); v.push_back(0.0f);
    v.push_back(r); v.push_back(g); v.push_back(b);
}

// Төрт бұрыш сағат тіліне қарсы берілуі керек
void addQuad(std::vector<float>& v, std::vector<unsigned int>& idx,
             float x1, float y1, float x2, float y2,
             float x3, float y3, float x4, float y4,
             float r, float g, float b) {
    unsigned int base = (unsigned int)(v.size() / 6);
    addVertex(v, x1, y1, r, g, b);
    addVertex(v, x2, y2, r, g, b);
    addVertex(v, x3, y3, r, g, b);
    addVertex(v, x4, y4, r, g, b);
    idx.insert(idx.end(), { base, base + 1, base + 2,   base, base + 2, base + 3 });
}

void addRect(std::vector<float>& v, std::vector<unsigned int>& idx,
             float x0, float y0, float x1, float y1,
             float r, float g, float b) {
    addQuad(v, idx, x0, y0,  x1, y0,  x1, y1,  x0, y1,  r, g, b);
}

void addCircle(std::vector<float>& v, std::vector<unsigned int>& idx,
               float cx, float cy, float rad,
               float cr, float cg, float cb,
               float er, float eg, float eb) {
    const int N = 32;
    unsigned int base = (unsigned int)(v.size() / 6);
    addVertex(v, cx, cy, cr, cg, cb);
    for (int i = 0; i < N; i++) {
        float a = 2.0f * 3.14159265f * (float)i / (float)N;
        addVertex(v, cx + std::cos(a) * rad, cy + std::sin(a) * rad, er, eg, eb);
    }
    for (int i = 0; i < N; i++) {
        unsigned int a = base + 1 + i;
        unsigned int b = base + 1 + (i + 1) % N;
        idx.insert(idx.end(), { base, a, b });
    }
}

// ---------------------------------------------------------------------
//  Фигуралар
// ---------------------------------------------------------------------

// Бірлік төртбұрыш (0,0)-(1,1), ақ. uScale/uOffset/uTint арқылы кез келген
// өлшем мен түсте сызылады (HUD, цифрлар, қабаттар).
Mesh makeUnitRect() {
    std::vector<float> v; std::vector<unsigned int> idx;
    addRect(v, idx, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    return makeMesh(v, idx);
}

// Машина (үстінен көрініс). Орталығы (0,0), алды +Y жақта.
Mesh makeCarMesh(float r, float g, float b, bool stripe) {
    std::vector<float> v; std::vector<unsigned int> idx;

    // Дөңгелектер (кузовтың артында, шетінен сәл шығып тұрады)
    const float wc = 0.08f;
    addRect(v, idx, -0.125f,  0.08f, -0.090f,  0.18f, wc, wc, wc);
    addRect(v, idx,  0.090f,  0.08f,  0.125f,  0.18f, wc, wc, wc);
    addRect(v, idx, -0.125f, -0.18f, -0.090f, -0.08f, wc, wc, wc);
    addRect(v, idx,  0.090f, -0.18f,  0.125f, -0.08f, wc, wc, wc);

    // Кузов (алды сәл тар)
    addQuad(v, idx, -0.11f, -0.21f,  0.11f, -0.21f,  0.10f, 0.21f,  -0.10f, 0.21f, r, g, b);

    // Төбесі
    addRect(v, idx, -0.08f, -0.09f, 0.08f, 0.04f, r * 0.75f, g * 0.75f, b * 0.75f);

    // Жолақ (тек ойыншыда)
    if (stripe) addRect(v, idx, -0.014f, -0.21f, 0.014f, 0.21f, 1.0f, 1.0f, 1.0f);

    // Алдыңғы әйнек және артқы әйнек
    addQuad(v, idx, -0.085f, 0.04f,  0.085f, 0.04f,  0.070f, 0.12f,  -0.070f, 0.12f, 0.60f, 0.85f, 1.00f);
    addRect(v, idx, -0.070f, -0.14f, 0.070f, -0.09f, 0.35f, 0.55f, 0.75f);

    // Фаралар (алды) және артқы шамдар
    addRect(v, idx, -0.090f,  0.19f, -0.050f,  0.21f, 1.0f, 0.95f, 0.35f);
    addRect(v, idx,  0.050f,  0.19f,  0.090f,  0.21f, 1.0f, 0.95f, 0.35f);
    addRect(v, idx, -0.095f, -0.21f, -0.050f, -0.19f, 0.9f, 0.1f, 0.1f);
    addRect(v, idx,  0.050f, -0.21f,  0.095f, -0.19f, 0.9f, 0.1f, 0.1f);

    return makeMesh(v, idx);
}

// Жол: асфальт, жиек сызықтары, ортасындағы қос сары сызық (қозғалмайды)
Mesh makeRoadStatic() {
    std::vector<float> v; std::vector<unsigned int> idx;
    addRect(v, idx, -ROAD_HALF, -1.0f, ROAD_HALF, 1.0f, 0.22f, 0.22f, 0.25f);                 // асфальт
    addRect(v, idx, -0.795f, -1.0f, -0.775f, 1.0f, 0.95f, 0.95f, 0.95f);                      // сол жиек
    addRect(v, idx,  0.775f, -1.0f,  0.795f, 1.0f, 0.95f, 0.95f, 0.95f);                      // оң жиек
    addRect(v, idx, -0.014f, -1.0f, -0.006f, 1.0f, 0.95f, 0.80f, 0.10f);                      // қос сары
    addRect(v, idx,  0.006f, -1.0f,  0.014f, 1.0f, 0.95f, 0.80f, 0.10f);
    return makeMesh(v, idx);
}

// Үзік сызықтар: жолақтарды бөледі (x = ±0.4). Төмен қарай жылжиды.
Mesh makeDashes() {
    std::vector<float> v; std::vector<unsigned int> idx;
    for (float y = -1.0f; y < 1.0f + DASH_P; y += DASH_P) {
        addRect(v, idx, -0.406f, y, -0.394f, y + 0.25f, 0.95f, 0.95f, 0.95f);
        addRect(v, idx,  0.394f, y,  0.406f, y + 0.25f, 0.95f, 0.95f, 0.95f);
    }
    return makeMesh(v, idx);
}

// Жол жиегіндегі ағаштар (үстінен: дөңгелек бұта)
Mesh makeTrees() {
    std::vector<float> v; std::vector<unsigned int> idx;
    const float px[4] = { -1.10f, -1.50f,  1.15f,  1.55f };
    const float py[4] = {  0.00f,  0.45f,  0.30f,  0.75f };
    for (float y = -1.2f; y < 1.2f + TREE_P; y += TREE_P) {
        for (int i = 0; i < 4; i++) {
            addCircle(v, idx, px[i], y + py[i], 0.11f, 0.18f, 0.52f, 0.22f, 0.08f, 0.35f, 0.12f);
        }
    }
    return makeMesh(v, idx);
}

Mesh makeExplosion() {
    std::vector<float> v; std::vector<unsigned int> idx;
    addCircle(v, idx, 0.0f, 0.0f, 0.15f, 1.0f, 0.95f, 0.3f, 1.0f, 0.4f, 0.05f);
    return makeMesh(v, idx);
}

// ---------------------------------------------------------------------
//  Кездейсоқ сандар
// ---------------------------------------------------------------------
std::mt19937 rng(std::random_device{}());
float rndf(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); }
int   rndi(int a, int b)     { return std::uniform_int_distribution<int>(a, b)(rng); }

// ---------------------------------------------------------------------
//  Ойын күйі
// ---------------------------------------------------------------------
struct Car {
    float x, y;
    float speed;        // өз жылдамдығы
    bool  oncoming;     // қарсы бетте ме
    int   color;
    int   lane;
    bool  passed;       // ойыншыдан өтіп кетті ме
};

struct Game {
    float px, py, vx;           // ойыншы позициясы және бүйір жылдамдығы
    float v;                    // жол жылдамдығы
    float score;
    bool  over;
    float crashX, crashY, crashT;
    float scroll;               // жолдың жылжуы (үзік сызықтар, ағаштар үшін)
    float spawnDist;            // келесі машинаға дейінгі жол
    std::vector<Car> cars;
    float popupT, popupX, popupY;
    int   popupVal;
};

void resetGame(Game& g) {
    g.px = LANE_X[2]; g.py = PLAYER_Y; g.vx = 0.0f;
    g.v = 1.0f;
    g.score = 0.0f;
    g.over = false;
    g.crashX = g.crashY = 0.0f; g.crashT = 0.0f;
    g.scroll = 0.0f;
    g.spawnDist = 0.8f;
    g.cars.clear();
    g.popupT = 0.0f; g.popupX = g.popupY = 0.0f; g.popupVal = 0;
}

void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

bool keyDown(GLFWwindow* w, int a, int b) {
    return glfwGetKey(w, a) == GLFW_PRESS || glfwGetKey(w, b) == GLFW_PRESS;
}

// =====================================================================
//  MAIN
// =====================================================================
int main() {

    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Traffic Racer 2D", nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(1);   // VSync қосулы — ойын тегіс жүрсін

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";
    std::cout << "A/D - бұрылу, W - газ, S - тежегіш, R - қайта бастау, ESC - шығу\n";

    int fbw, fbh;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);

    // Мөлдірлік (қараңғы қабат пен жарылыс үшін)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- фигуралар ---
    Mesh unitRect  = makeUnitRect();
    Mesh roadMesh  = makeRoadStatic();
    Mesh dashMesh  = makeDashes();
    Mesh treeMesh  = makeTrees();
    Mesh boomMesh  = makeExplosion();
    Mesh playerMesh = makeCarMesh(0.15f, 0.35f, 0.90f, true);

    const int NCOL = 6;
    const float palette[NCOL][3] = {
        { 0.95f, 0.80f, 0.10f },   // сары
        { 0.20f, 0.70f, 0.30f },   // жасыл
        { 0.90f, 0.90f, 0.92f },   // ақ
        { 0.95f, 0.50f, 0.10f },   // қызғылт сары
        { 0.60f, 0.30f, 0.80f },   // күлгін
        { 0.50f, 0.50f, 0.55f }    // сұр
    };
    Mesh trafficMesh[NCOL];
    for (int i = 0; i < NCOL; i++) {
        trafficMesh[i] = makeCarMesh(palette[i][0], palette[i][1], palette[i][2], false);
    }

    unsigned int shader = makeShader(vertexSrc, fragmentSrc);
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale  = glGetUniformLocation(shader, "uScale");
    int locAspect = glGetUniformLocation(shader, "uAspect");
    int locTint   = glGetUniformLocation(shader, "uTint");
    int locAlpha  = glGetUniformLocation(shader, "uAlpha");

    glUseProgram(shader);
    glUniform1f(locAspect, ASPECT);

    // --- сызу көмекшілері ---
    auto setTint = [&](float r, float g, float b, float a = 1.0f) {
        glUniform3f(locTint, r, g, b);
        glUniform1f(locAlpha, a);
    };
    auto drawMesh = [&](const Mesh& m, float x, float y, float sx, float sy) {
        glUniform2f(locOffset, x, y);
        glUniform2f(locScale, sx, sy);
        glBindVertexArray(m.vao);
        glDrawElements(GL_TRIANGLES, m.count, GL_UNSIGNED_INT, 0);
    };
    auto rect = [&](float x, float y, float w, float h) {
        drawMesh(unitRect, x, y, w, h);
    };

    // 7 сегментті цифрлар: биттер a b c d e f g = 1 2 4 8 16 32 64
    const int SEG[10] = { 63, 6, 91, 79, 102, 109, 125, 7, 127, 111 };

    auto drawDigit = [&](int d, float x, float y, float h) {
        float w = h * 0.55f, t = h * 0.13f;
        int m = SEG[d];
        if (m & 1)  rect(x + t,     y + h - t,       w - 2 * t, t);           // a
        if (m & 2)  rect(x + w - t, y + h / 2,       t, h / 2 - t);           // b
        if (m & 4)  rect(x + w - t, y + t,           t, h / 2 - t);           // c
        if (m & 8)  rect(x + t,     y,               w - 2 * t, t);           // d
        if (m & 16) rect(x,         y + t,           t, h / 2 - t);           // e
        if (m & 32) rect(x,         y + h / 2,       t, h / 2 - t);           // f
        if (m & 64) rect(x + t,     y + h / 2 - t / 2, w - 2 * t, t);         // g
    };
    auto digitCount = [](int val) {
        int n = 1;
        while (val >= 10) { val /= 10; n++; }
        return n;
    };
    auto numberWidth = [&](int val, float h) {
        float adv = h * 0.55f + h * 0.2f;
        return digitCount(val) * adv - h * 0.2f;
    };
    auto drawNumber = [&](int val, float x, float y, float h) {
        if (val < 0) val = 0;
        int n = digitCount(val);
        float adv = h * 0.55f + h * 0.2f;
        for (int i = 0; i < n; i++) {
            int p = 1;
            for (int k = 0; k < n - 1 - i; k++) p *= 10;
            drawDigit((val / p) % 10, x + i * adv, y, h);
        }
    };

    // "R" әрпі (блоктық), қайта бастау белгісі үшін
    auto drawLetterR = [&](float x, float y, float h) {
        float w = h * 0.6f, t = h * 0.16f;
        rect(x, y, t, h);                              // сабағы
        rect(x, y + h - t, w, t);                      // үсті
        rect(x, y + h / 2 - t / 2, w, t);              // ортасы
        rect(x + w - t, y + h / 2, t, h / 2 - t);      // оң жоғарғы
        rect(x + w - t, y, t, h / 2 - t / 2);          // аяғы
    };

    // --- ойын ---
    Game g;
    resetGame(g);
    int best = 0;

    float lastFrame = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {

        float now = (float)glfwGetTime();
        float dt  = now - lastFrame;
        lastFrame = now;
        if (dt > 0.05f) dt = 0.05f;      // үлкен секірулерді болдырмау (тоқтап қалса)

        // ================= БАСҚАРУ =================
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        if (g.over) {
            g.crashT += dt;
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                resetGame(g);
                glfwSetWindowTitle(window, "Traffic Racer 2D");
            }
        } else {
            // ---------- ЖАҢАРТУ ----------
            float steer = 0.0f;
            if (keyDown(window, GLFW_KEY_A, GLFW_KEY_LEFT))  steer -= 1.0f;
            if (keyDown(window, GLFW_KEY_D, GLFW_KEY_RIGHT)) steer += 1.0f;

            // Бүйір қозғалыс: тегіс (инерциямен)
            g.vx += (steer * 1.4f - g.vx) * std::min(1.0f, 10.0f * dt);
            g.px += g.vx * dt;
            float limit = ROAD_HALF - 0.125f - 0.01f;
            if (g.px < -limit) { g.px = -limit; g.vx = 0.0f; }
            if (g.px >  limit) { g.px =  limit; g.vx = 0.0f; }

            // Газ / тежегіш (ештеңе баспаса — жылдамдық сақталады)
            if (keyDown(window, GLFW_KEY_W, GLFW_KEY_UP))   g.v += 1.0f * dt;
            if (keyDown(window, GLFW_KEY_S, GLFW_KEY_DOWN)) g.v -= 2.0f * dt;
            g.v = std::max(V_MIN, std::min(V_MAX, g.v));

            g.scroll += g.v * dt;
            g.score  += g.v * dt * 8.0f;

            // Машиналарды жылжыту
            // Қарсы бет: экранда (v + өз жылдамдығы) жылдамдықпен төмен.
            // Бір бағыт:  экранда (v - өз жылдамдығы) жылдамдықпен төмен.
            for (Car& c : g.cars) {
                if (c.oncoming) c.y -= (g.v + c.speed) * dt;
                else            c.y -= (g.v - c.speed) * dt;
            }

            // Соқтығысу (AABB)
            for (const Car& c : g.cars) {
                if (std::fabs(c.x - g.px) < HIT_HW * 2.0f &&
                    std::fabs(c.y - g.py) < HIT_HH * 2.0f) {
                    g.over = true;
                    g.crashX = (g.px + c.x) * 0.5f;
                    g.crashY = (g.py + c.y) * 0.5f;
                    g.crashT = 0.0f;
                    break;
                }
            }

            if (g.over) {
                best = std::max(best, (int)g.score);
                std::string title = "Traffic Racer 2D - GAME OVER | Upai: " +
                                    std::to_string((int)g.score) +
                                    " | Best: " + std::to_string(best) +
                                    " | R - qaita bastau";
                glfwSetWindowTitle(window, title.c_str());
            } else {
                // Басып озу және near miss
                for (Car& c : g.cars) {
                    if (!c.passed && c.y < g.py) {
                        c.passed = true;
                        float dx = std::fabs(c.x - g.px);
                        int bonus = 10;
                        if (dx < 0.34f) bonus += c.oncoming ? 40 : 25;   // өте жақын өтті
                        g.score += (float)bonus;
                        if (bonus > 10) {
                            g.popupVal = bonus;
                            g.popupT = 0.8f;
                            g.popupX = g.px;
                            g.popupY = g.py + 0.30f;
                        }
                    }
                }

                // Экраннан кеткендерді өшіру
                g.cars.erase(std::remove_if(g.cars.begin(), g.cars.end(),
                             [](const Car& c) { return c.y < -1.4f; }), g.cars.end());

                // Жаңа машина шығару
                g.spawnDist -= g.v * dt;
                if (g.spawnDist <= 0.0f) {
                    int lane = rndi(0, 3);
                    bool laneBusy[4] = { false, false, false, false };
                    bool ok = true;
                    for (const Car& c : g.cars) {
                        if (c.y > 0.5f) laneBusy[c.lane] = true;
                        if (c.lane == lane && c.y > 0.3f) ok = false;   // бір жолақта тым жақын
                    }
                    int busy = 0;
                    for (int i = 0; i < 4; i++) if (laneBusy[i]) busy++;
                    if (busy >= 2) ok = false;       // ең болмағанда 2 жолақ бос қалсын

                    if (ok) {
                        Car c;
                        c.lane = lane;
                        c.x = LANE_X[lane];
                        c.y = 1.30f;
                        c.oncoming = (lane < 2);
                        c.speed = c.oncoming ? rndf(0.5f, 0.9f) : rndf(0.35f, 0.65f);
                        c.color = rndi(0, NCOL - 1);
                        c.passed = false;
                        g.cars.push_back(c);
                        g.spawnDist = rndf(0.8f, 1.5f);
                    } else {
                        g.spawnDist = 0.05f;         // жақын арада қайта тексереміз
                    }
                }

                if (g.popupT > 0.0f) g.popupT -= dt;
            }
        }

        // ================= СЫЗУ =================
        glClearColor(0.30f, 0.62f, 0.28f, 1.0f);     // шөп
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
        setTint(1.0f, 1.0f, 1.0f);

        // Ағаштар (төмен қарай жылжиды)
        drawMesh(treeMesh, 0.0f, -std::fmod(g.scroll, TREE_P), 1.0f, 1.0f);

        // Жол
        drawMesh(roadMesh, 0.0f, 0.0f, 1.0f, 1.0f);
        drawMesh(dashMesh, 0.0f, -std::fmod(g.scroll, DASH_P), 1.0f, 1.0f);

        // Трафик (қарсы бетке келе жатқандар 180° бұрылған: scale = -1,-1)
        for (const Car& c : g.cars) {
            float s = c.oncoming ? -1.0f : 1.0f;
            drawMesh(trafficMesh[c.color], c.x, c.y, s, s);
        }

        // Ойыншы
        drawMesh(playerMesh, g.px, g.py, 1.0f, 1.0f);

        // Жарылыс
        if (g.over) {
            float s = 0.6f + std::min(g.crashT, 1.0f) * 2.5f;
            float a = std::max(0.0f, 1.0f - g.crashT * 0.6f);
            setTint(1.0f, 1.0f, 1.0f, a);
            drawMesh(boomMesh, g.crashX, g.crashY, s, s);
            setTint(1.0f, 1.0f, 1.0f, 1.0f);
        }

        // Near miss бонусы (ойыншының үстінде көтеріліп жоғалады)
        if (!g.over && g.popupT > 0.0f) {
            float rise = (0.8f - g.popupT) * 0.2f;
            float h = 0.08f;
            setTint(1.0f, 0.92f, 0.2f, std::min(1.0f, g.popupT * 3.0f));
            drawNumber(g.popupVal, g.popupX - numberWidth(g.popupVal, h) * 0.5f,
                       g.popupY + rise, h);
            setTint(1.0f, 1.0f, 1.0f, 1.0f);
        }

        // ---------- HUD ----------
        // Ұпай (сол жоғарыда, ақ)
        setTint(1.0f, 1.0f, 1.0f);
        drawNumber((int)g.score, -ASPECT + 0.08f, 0.82f, 0.12f);

        // Ең жақсы нәтиже (оң жоғарыда, алтын)
        {
            float h = 0.08f;
            setTint(1.0f, 0.82f, 0.2f);
            drawNumber(best, ASPECT - 0.08f - numberWidth(best, h), 0.88f, h);
        }

        // Жылдамдық: км/сағ + индикатор жолағы (төменгі оң жақта)
        {
            int kmh = (int)(g.v * KMH);
            float x0 = ASPECT - 0.55f;
            setTint(0.9f, 0.9f, 0.9f);
            drawNumber(kmh, x0, -0.86f, 0.09f);

            float frac = (g.v - V_MIN) / (V_MAX - V_MIN);
            frac = std::max(0.0f, std::min(1.0f, frac));
            setTint(0.05f, 0.05f, 0.05f, 0.75f);
            rect(x0 - 0.01f, -0.975f, 0.47f, 0.06f);
            setTint(frac, 1.0f - frac * 0.8f, 0.1f);
            rect(x0, -0.965f, 0.45f * frac, 0.04f);
            setTint(1.0f, 1.0f, 1.0f);
        }

        // Game Over: қараңғы қабат + үлкен ұпай + R пернесі
        if (g.over && g.crashT > 0.4f) {
            setTint(0.0f, 0.0f, 0.0f, 0.55f);
            rect(-ASPECT, -1.0f, 2.0f * ASPECT, 2.0f);

            int sc = (int)g.score;
            float h = 0.35f;
            setTint(1.0f, 1.0f, 1.0f);
            drawNumber(sc, -numberWidth(sc, h) * 0.5f, 0.05f, h);

            setTint(0.2f, 0.8f, 0.3f);
            rect(-0.15f, -0.50f, 0.30f, 0.30f);          // пернетақта батырмасы
            setTint(0.05f, 0.2f, 0.08f);
            drawLetterR(-0.065f, -0.43f, 0.16f);
            setTint(1.0f, 1.0f, 1.0f);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---------------- тазалау ----------------
    destroyMesh(unitRect);
    destroyMesh(roadMesh);
    destroyMesh(dashMesh);
    destroyMesh(treeMesh);
    destroyMesh(boomMesh);
    destroyMesh(playerMesh);
    for (int i = 0; i < NCOL; i++) destroyMesh(trafficMesh[i]);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}