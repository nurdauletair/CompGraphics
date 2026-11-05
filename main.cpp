// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add main.cpp && git commit -m "week10: mouse look, yaw/pitch, fov zoom"
//      git tag week10 && git push origin main --tags
//
//  Тег арқылы кез келген аптадағы күйге қайта оралуға болады.
//
//  Қазіргі күйі: 10-АПТА — тінтуірмен еркін камера, yaw/pitch, FOV зумы.
//                Сол бес куб, WASD, invert Y және камера күйін шығару.
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdio>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

// === 7-АПТА: терезенің ағымдағы өлшемі (resize кезінде жаңартылады) ===
int gWidth  = WIDTH;
int gHeight = HEIGHT;

bool  whiteBackground = false;

// === 4-АПТА: анимация жылдамдығы — енді Up/Down өзгертеді ===
float speed = 1.0f;

// === 9-АПТА: камера күйі — цикл сыртында сақталады ===
glm::vec3 camPos   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f, 0.0f);

// === 7-АПТА: проекция параметрлері ===
const float FOV_DEG    = 45.0f;     // бастапқы FOV; ағымдағысы төмендегі fov
const float NEAR_PLANE = 0.1f;      // 2-тапсырма: 0.0001f -> Z-fighting; 0.1f -> түзетілген
const float FAR_PLANE  = 100.0f;    // 2-тапсырма: 1000.0f (жасау кезінде)

// === 10-АПТА: тінтуір, бұрыштар және зум ===
float yaw = -90.0f;               // -Z бағыты; yaw = 0 болса, +X жағына қарайды
float pitch = 0.0f;
float lastX = WIDTH * 0.5f;
float lastY = HEIGHT * 0.5f;
bool firstMouse = true;
float fov = FOV_DEG;
bool invertY = false;
const float DEFAULT_SENSITIVITY = 0.1f;
float sensitivity = DEFAULT_SENSITIVITY; // 1/2/3: 0.03 / 0.1 / 0.5
// 1-тапсырма тәжірибесі: false қойып, pitch-ті консольден бақылау;
// тәжірибеден кейін міндетті түрде true күйіне қайтару.
const bool LIMIT_PITCH = true;

// === 8-АПТА: тапсырмалар үшін қосқыштар (эксперимент жасау үшін) ===
// 1-тапсырма: false қылып, скриншот түсір (тереңдік тесінсіз), сосын қайта true.
const bool   USE_DEPTH_TEST = true;
// 2-тапсырма: true қылсаң, 5 куб орнына бетпе-бет екі куб шығады (z айырмасы 0.001).
const bool   ZFIGHT_DEMO    = false;
// 3-тапсырма: GL_FRONT қылып диагноз қой (GL_BACK — қалыпты күй).
const GLenum CULL_MODE      = GL_BACK;

// === 7-АПТА: бір вершинаға 6 float: позиция (xyz) + түс (rgb) ===
// Куб: 6 қыр x 2 үшбұрыш x 3 вершина = 36 вершина.
// 8-АПТА: culling қосылғандықтан әр үшбұрыштың вершиналары сырттан
// қарағанда САҒАТ ТІЛІНЕ ҚАРСЫ (CCW) жазылуы керек.
float vertices[] = {
    // артқы қыр (қызыл) — сырттан (-z жағынан) қарағанда CCW
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,

    // алдыңғы қыр (жасыл)
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,

    // сол жақ қыр (көк)
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,

    // оң жақ қыр (сары) — сырттан (+x жағынан) қарағанда CCW
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

    // төменгі қыр (қызғылт)
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,

    // жоғарғы қыр (көгілдір) — сырттан (+y жағынан) қарағанда CCW
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f
};

// === 8-АПТА: бес куб — позициялар массиві ===
glm::vec3 positions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  0.5f, -2.0f),
    glm::vec3(-1.8f, -0.3f, -1.0f),
    glm::vec3( 1.2f, -0.8f, -3.5f),
    glm::vec3(-2.2f,  1.0f, -4.0f)
};

// === 7-АПТА: вершина шейдері — үш матрица бір-біріне көбейтіледі ===
// Назар: #version жолдың ЕҢ БІРІНШІ символынан басталуы керек.
const char* vertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* fragmentSrc = R"(#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
    // Тапсырма: түстерді төңкеру  ->  FragColor = vec4(1.0 - vColor, 1.0);
    // Қызыл тест:                 ->  FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

// ---------------------------------------------------------------------
//  === 3-АПТА: makeShader() — компиляция + қате тексеру ===
// ---------------------------------------------------------------------
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
//  Терезе өлшемі өзгергенде шақырылады.
//  7-апта: gWidth/gHeight те жаңартылады — aspect осыдан есептеледі.
// ---------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    gWidth  = width;
    gHeight = height;
    glViewport(0, 0, width, height);
}

// === 10-АПТА: градуспен берілген yaw/pitch-тен бірлік бағыт алу ===
void updateCameraFront() {
    const float yawRad = glm::radians(yaw);
    const float pitchRad = glm::radians(pitch);
    glm::vec3 dir;
    dir.x = std::cos(yawRad) * std::cos(pitchRad);
    dir.y = std::sin(pitchRad);
    dir.z = std::sin(yawRad) * std::cos(pitchRad);
    camFront = glm::normalize(dir);
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
        return;                     // алғашқы оқиға камераны секіртпейді
    }

    const float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); // экран Y бағытына кері
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
    if (invertY) yoffset = -yoffset;

    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;
    // Қосымша: yaw шексіз өспесін, бастапқы -90 мәні де сақталады.
    yaw = std::fmod(yaw, 360.0f);
    if (LIMIT_PITCH) {
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    } else {
        std::cout << "pitch = " << pitch << "\n";
    }
    updateCameraFront();
}

void scroll_callback(GLFWwindow*, double, double yoffset) {
    fov -= static_cast<float>(yoffset);
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 90.0f) fov = 90.0f;  // 5-тапсырма: жоғарғы шек 45-тен 90-ға өсті
}

// Терезеге қайта оралғанда бұрынғы курсор орнымен айырманы қолданбау.
void window_focus_callback(GLFWwindow*, int) {
    firstMouse = true;
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру. Әр кадрда шақырылады.
//  9-апта: камераның қозғалысы FPS-ке тәуелсіз болуы үшін dt қолданылады.
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        whiteBackground = true;
    }

    // === 10-АПТА: I бір басылғанда ғана invert Y ауысады ===
    static bool iPressedLast = false;
    const bool iNow = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
    if (iNow && !iPressedLast) {
        invertY = !invertY;
        std::cout << "invert Y: " << (invertY ? "ON" : "OFF") << "\n";
    }
    iPressedLast = iNow;

    // 2-тапсырма: үш sensitivity мәнін қайта құрастырмай салыстыру.
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) sensitivity = 0.03f;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) sensitivity = DEFAULT_SENSITIVITY;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) sensitivity = 0.5f;

    // === 4-АПТА: анимация жылдамдығын басқару (W/S енді камераға тиесілі) ===
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        speed += 1.0f * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        speed -= 1.0f * dt;
        if (speed < 0.0f) speed = 0.0f;   // уақыт кері жүрмесін
    }

    // === 9-АПТА: WASD + Space/Left Shift + Left Ctrl ===
    float cameraSpeed = 2.5f * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        cameraSpeed *= 2.0f;
    }

    const glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += cameraSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= cameraSpeed * camFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= cameraSpeed * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += cameraSpeed * right;

    // Әлемнің Y осін таңдадық: жоғары/төмен бағыты камера еңкейсе де өзгермейді.
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) camPos += cameraSpeed * worldUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camPos -= cameraSpeed * worldUp;

    // Қосымша: адасып кетсең, R бастапқы күйге қайтарады.
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camPos   = glm::vec3(0.0f, 0.0f, 3.0f);
        camUp    = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        fov = FOV_DEG;
        firstMouse = true;
        updateCameraFront();
    }

    // 4-тапсырма: P басылғанда толық күйді бір рет шығару.
    static bool pWasPressed = false;
    const bool pPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (pPressed && !pWasPressed) {
        char state[512];
        std::snprintf(state, sizeof(state),
                      "camPos = (%.2f, %.2f, %.2f)\n"
                      "camFront = (%.2f, %.2f, %.2f)\n"
                      "yaw = %.1f pitch = %.1f fov = %.1f\n"
                      "length = %.6f sensitivity = %.2f invertY = %s\n",
                      camPos.x, camPos.y, camPos.z,
                      camFront.x, camFront.y, camFront.z,
                      yaw, pitch, fov, glm::length(camFront), sensitivity,
                      invertY ? "ON" : "OFF");
        std::cout << state;
    }
    pWasPressed = pPressed;
}

// =====================================================================
//  MAIN
// =====================================================================
int main() {

    // -----------------------------------------------------------------
    //  1. GLFW-ны іске қосу
    // -----------------------------------------------------------------
    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    // Қандай OpenGL нұсқасы керек екенін айтамыз.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    // -----------------------------------------------------------------
    //  2. Терезе жасау
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Компьютерлік графика",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);              // осы терезенің контексі белсенді
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(0);                         // VSync

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу
    //     Контекст белсенді болғаннан КЕЙІН ғана. Ретін бұзсаң — бәрі құлайды.
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    // === 10-АПТА: callbacks тіркеу және курсорды ұстау ===
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetWindowFocusCallback(window, window_focus_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";
    std::cout << "Mouse: look | Wheel: zoom | I: invert Y | 1/2/3: sensitivity\n"
                 "WASD: move | Space/Left Shift: up/down | Left Ctrl: 2x speed\n"
                 "R: reset camera | P: full state | Up/Down: animation speed\n"
                 "B: white background | O: orthographic (hold) | Esc: exit\n";

    // Бастапқы өлшем (Retina экранда framebuffer терезеден үлкен болады)
    glfwGetFramebufferSize(window, &gWidth, &gHeight);
    glViewport(0, 0, gWidth, gHeight);

    // === 7-АПТА: куб — индекс жоқ (VAO + VBO) ===
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // === 3-АПТА: шейдерлер makeShader() арқылы ===
    unsigned int shader = makeShader(vertexSrc, fragmentSrc);

    // === 7-АПТА: үш uniform-ның орнын цикл алдында БІР РЕТ табамыз ===
    int locModel = glGetUniformLocation(shader, "uModel");
    int locView  = glGetUniformLocation(shader, "uView");
    int locProj  = glGetUniformLocation(shader, "uProjection");

    // === 8-АПТА: тереңдік тесті мен culling — цикл алдында, БІР РЕТ ===
    if (USE_DEPTH_TEST) {
        glEnable(GL_DEPTH_TEST);      // жақын фрагмент алыс фрагментті жабады
    }
    glEnable(GL_CULL_FACE);           // артқа қараған үшбұрыштарды сызбау
    glCullFace(CULL_MODE);            // GL_BACK: артқы жағын лақтыру
    glFrontFace(GL_CCW);              // "алдыңғы" = сағат тіліне қарсы жазылған

    // === 4-АПТА: уақыт және анимация күйі ===
    float  lastFrame = (float)glfwGetTime();   // НАЗАР: нөл емес!
    double simTime   = 0.0;                    // сахнаның ортақ уақыты (Up/Down жылдамдатады)

    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    double fpsTimer = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {

        // --- уақыт (4-апта: delta time) ---
        float now = (float)glfwGetTime();
        float dt  = now - lastFrame;
        lastFrame = now;

        // --- FPS (1-апта) ---
        frameCount++;
        if (now - fpsTimer >= 1.0) {
            std::cout << "FPS: " << frameCount
                      << "  speed: " << speed << "x\n";
            frameCount = 0;
            fpsTimer = now;
        }

        // ================= ЖАҢАРТУ: күйді есептеу =================
        processInput(window, dt);
        simTime += (double)(speed * dt);
        float angle = (float)simTime;

        // Қосымша: бұрыштар мен FOV терезенің тақырыбында көрінеді.
        // Жиілігі 10 Гц: native терезе тақырыбын әр кадрда өзгертпеу.
        static double titleTimer = -1.0;
        if (now - titleTimer >= 0.1) {
            char title[192];
            std::snprintf(title, sizeof(title),
                          "Week 10 | yaw=%.1f pitch=%.1f fov=%.1f | sens=%.2f | invertY=%s",
                          yaw, pitch, fov, sensitivity, invertY ? "ON" : "OFF");
            glfwSetWindowTitle(window, title);
            titleTimer = now;
        }

        // ================= СЫЗУ: күйді көрсету =================
        if (whiteBackground) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            glClearColor(0.12f, 0.14f, 0.20f, 1.0f);
        }
        // === 8-АПТА: түс буфері МЕН тереңдік буферін тазалау — әр кадрда ===
        // GL_DEPTH_BUFFER_BIT ұмытылса, бірінші кадрдан кейін экран қатып қалады.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // --- Матрицалар: цикл ІШІНДЕ құрылады ---
        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        // PROJECTION: O пернесін басып тұрсаң — ортографиялық (7-апта, 2-тапсырма)
        glm::mat4 projection;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, NEAR_PLANE, FAR_PLANE);
        } else {
            projection = glm::perspective(glm::radians(fov), aspect, NEAR_PLANE, FAR_PLANE);
        }

        // === 9-АПТА: мақсат нүктесі камерамен бірге жылжиды ===
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        // projection мен view — бір рет, үш куб үшін ортақ.
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(vao);

        if (ZFIGHT_DEMO) {
            // === 2-ТАПСЫРМА: екі куб бетпе-бет, z айырмасы 0.001 ===
            // A кубының алдыңғы беті (жасыл) z = 0.5.
            // B кубы 180° бұрылған: оның артқы беті (қызыл) z = 0.501 -> A-мен дерлік беттеседі.
            glm::mat4 modelA = glm::mat4(1.0f);
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelA));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glm::mat4 modelB = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.001f));
            modelB = glm::rotate(modelB, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelB));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        } else {
            // === 8-АПТА: бес куб. model — цикл ІШІНДЕ жарияланады,
            // әйтпесе түрлендірулер үстемеленіп, кубтар ұшып кетеді.
            for (int i = 0; i < 5; i++) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, positions[i]);
                model = glm::rotate(model, angle * (0.5f + i * 0.3f),
                                    glm::vec3(0.3f, 1.0f, 0.0f));
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        glfwSwapBuffers(window);   // дайын кадрды экранға шығару
        glfwPollEvents();          // пернетақта/тінтуір оқиғаларын өңдеу
    }

    // -----------------------------------------------------------------
    //  5. Тазалау
    // -----------------------------------------------------------------
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}


// =====================================================================
//  1-АПТА ТАПСЫРМАЛАРЫ — орындалды
//  1. Терезе 1280x720.   2. Фон жылдамдығы.   3. B = ақ фон.   4. FPS.
// =====================================================================

// =====================================================================
//  3-АПТА ТАПСЫРМАЛАРЫ (семинар 3: түрлі-түсті үшбұрыш)
//  1. Түстерді төңкер: fragment шейдерде  vec4(1.0 - vColor, 1.0)
//  2. Үш вершинаға бірдей түс бер -> градиент жоғалады (интерполяция).
//  3. Шейдерге әдейі қате жаз -> консольде makeShader() қатесін оқы.
// =====================================================================

// =====================================================================
//  4-АПТА ТАПСЫРМАЛАРЫ (семинар 4: шеңбер бойымен қозғалу) — орындалды
//  (dt, uScale пульсациясы, W/S жылдамдығы)
// =====================================================================

// =====================================================================
//  5-АПТА ТАПСЫРМАЛАРЫ (семинар 5: индекс буфері) — орындалды
//  6-АПТА (2D сахна: машина мен секіретін доп) — орындалды, тег: week06
// =====================================================================

// =====================================================================
//  7-АПТА ТАПСЫРМАЛАРЫ (MVP, проекция) — орындалды, тег: week07
//  FOV_DEG / NEAR_PLANE / FAR_PLANE константалары, O пернесі = ортография.
//  Бақылаулар: near = 4.0 және far = 2.0 болғанда куб толық жоғалады;
//  near = 2.5 болғанда куб жартылай кесіледі.
// =====================================================================

// =====================================================================
//  8-АПТА ТАПСЫРМАЛАРЫ (тереңдік буфері, culling, Z-fighting)
//
//  БАЗАЛЫҚ БӨЛІМ — орындалды:
//   1. glEnable(GL_DEPTH_TEST) — цикл алдында, бір рет.
//   2. glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) — әр кадрда.
//   3. glEnable(GL_CULL_FACE), glCullFace(GL_BACK), glFrontFace(GL_CCW).
//   4. Бес куб: positions[] массиві.
//   5. projection мен view — бір рет; model — цикл ішінде, әр кубқа бөлек.
//
//  ЕСКЕРТУ: 7-аптадағы массивте үш қырдың (артқы, оң жақ, жоғарғы)
//  вершиналары сағат тілімен жазылған екен. Culling қосылғанда олар
//  жоғалып кетер еді, сондықтан осы үш қырдың реті CCW болып түзетілді.
//
//  1-ТАПСЫРМА. USE_DEPTH_TEST = false қылып, екі скриншот түсір.
//   Тереңдік тесінсіз кубтар сызылу ретімен бірінің үстіне бірі жазылады:
//   соңғы сызылған (i = 4, ең алыстағы куб, z = -4) бәрінің үстінде
//   көрінеді. Себебі тест жоқ, әр жаңа фрагмент алдыңғысын "кім жақын"
//   деп салыстырмай-ақ үстінен жазып кетеді. Сызу ретін өзгертсең,
//   нәтиже өзгереді (кім соңғы салынса, сол үстінде болады).
//   Тесімен — қашықтыққа қарай дұрыс жабылады, ретке тәуелді емес.
//
//  2-ТАПСЫРМА. Z-fighting.
//   Жасау: ZFIGHT_DEMO = true, NEAR_PLANE = 0.0001f, FAR_PLANE = 1000.0f.
//   Екі куб бетпе-бет (z айырмасы 0.001): жасыл және қызыл беттер жыпылықтайды,
//   камера теңселгенде өрнек өзгереді.
//   Жою: NEAR_PLANE = 0.1f — жыпылықтау жоғалады.
//   Неге: 24-биттік тереңдік буферінің көбі near жанында жұмсалады
//   (жартысы near мен 2 x near арасында). near = 0.0001 болса, қашықтығы 2.5
//   жерде тереңдік қадамы ~0.004 шамасында — 0.001 айырмасын ажырата алмайды.
//   near = 0.1 болғанда қадам ~0.000004 — ажыратады.
//   far 1000 -> 100 жыпылықтауды азайтпайды: z_ndc ~ 1 - 2*near/z (far >> near
//   кезінде far формулаға дерлік кірмейді), тереңдік дәлдігін near анықтайды.
//
//  3-ТАПСЫРМА. Culling.
//   GL_BACK кезінде алты қыр да көрінеді. CULL_MODE = GL_FRONT қылсаң,
//   бетінен қараған қырлар жоғалып, куб "ішінен" көрінеді (диагноз сол).
//   Бір кубта 12 үшбұрыш бар. Бір сәтте көбіне 3 қыр (6 үшбұрыш) бізге
//   қарайды, қалған 3 қыр (6 үшбұрыш) расталмай қалады, яғни жартысы.
//   Куб айналғанда "артқы" қыр ауысады, бірақ код ӨЗГЕРМЕЙДІ: GPU әр кадрда
//   әр үшбұрыштың экрандағы айналу бағытына (CCW/CW) қарап өзі шешеді.
// =====================================================================

// =====================================================================
//  9-АПТА ТАПСЫРМАЛАРЫ (камера, LookAt, пернетақтамен жылжу) — орындалды
//
//  БАЗАЛЫҚ БӨЛІМ:
//   camPos, camFront, camUp — жаһандық айнымалылар.
//   WASD қозғалысы processInput(window, dt) ішінде, dt есептелгеннен кейін.
//   A/D бағыты = normalize(cross(camFront, camUp)).
//   view әр кадрда lookAt(camPos, camPos + camFront, camUp) арқылы құрылады.
//   Бес кубтың model матрицалары камера қозғалысына тәуелді емес.
//
//  1-ТАПСЫРМА. Екі LookAt нұсқасының айырмашылығы:
//   lookAt(camPos, glm::vec3(0.0f), camUp) болса, камера әрқашан координат
//   басына қарайды. W/S жақындатады/алыстатады, A/D кезінде көрініс бағыты
//   өзгеріп, камера сахнаға қарай бұрылады. Бұл нысанды жан-жағынан қарау
//   үшін пайдалы (мысалы, 3D модельді қарау, редактордағы орбиталық камера).
//   Түзетілген нұсқада мақсат = camPos + camFront: бағыт тұрақты қалады.
//   A камераны солға, бастапқы бағытта әлемнің -X осі бойымен жылжытады;
//   камера координат басына өздігінен бұрылмайды.
//
//  2-ТАПСЫРМА. Space — жоғары, Left Shift — төмен.
//   Әлемнің Y осі таңдалды: адам басқарылатын ойында жоғары бағыты
//   тұрақты болуы керек. Ұшақ симуляторында камерамен бірге бұрылатын
//   жергілікті up векторы пайдалы. Қазір camUp = worldUp, нәтижелері бірдей.
//
//  3-ТАПСЫРМА. Left Ctrl қозғалыс жылдамдығын 2 есе арттырады.
//   Қалыпты жылдамдық 2.5 бірлік/с, жүгіру 5.0 бірлік/с; екеуі де dt-мен.
//   2.0-ге dt-ден бұрын не кейін көбейту математикалық нәтижені өзгертпейді:
//   (2.5 * dt) * 2.0 = (2.5 * 2.0) * dt (float дөңгелектеуі ғана мүмкін).
//
//  ҚОСЫМША: R — камераны бастапқы күйге қайтару, P — координаттарды шығару.
//  Бұрынғы басқару: анимация жылдамдығы Up/Down, ақ фон B пернесіне көшті.
// =====================================================================

// =====================================================================
//  10-АПТА (mouse look, yaw/pitch, FOV) — код бөлігі орындалды
//
//  БАЗАЛЫҚ БӨЛІМ:
//   yaw = -90, pitch = 0; firstMouse алғашқы оқиғаның секіруін болдырмайды.
//   Курсор ұсталған, mouse/scroll callbacks тіркелген. Бұрыштар градуспен,
//   sin/cos алдында radians қолданылады, camFront normalize арқылы алынады.
//   pitch [-89, 89], fov [1, 90]; perspective ағымдағы fov-ты қолданады.
//
//  1-ТАПСЫРМА. LIMIT_PITCH = false — шектеусіз тәжірибе және pitch журналы.
//   Математикалық сингулярлық: pitch = +/-90 кезінде cos(pitch) = 0,
//   camFront = (0, +/-1, 0), ол camUp-пен параллель. cross(front, up) = 0;
//   normalize(0) нөлге бөледі, NaN lookAt матрицасына тарауы мүмкін.
//   float-та cos(radians(90)) дәл нөл болмауы мүмкін: экранның ағаруы
//   міндетті емес, 90-нан өткенде көрініс төңкерілуі де мүмкін.
//   Нақты бұзылған бұрышты қолданушы тәжірибеде өлшеуі керек; мұнда
//   өлшенбеген сан нәтиже ретінде жазылмады. Соңғы кодта шектеу қосулы.
//
//  2-ТАПСЫРМА. Әдепкі sensitivity = 0.1: 0.03-тен жылдамырақ, 0.5-тен
//   баяуырақ, сондықтан бастапқы орташа баптау ретінде таңдалды.
//   1/2/3 арқылы үш мәнді салыстыруға болады (0.03 / 0.1 / 0.5).
//   360 градусқа теориялық курсор жолы: 12000 / 3600 / 720 бірлік.
//   Тінтуірді неше рет көтеру керегі DPI мен жұмыс орнына тәуелді.
//   const мәнді кездейсоқ өзгертуден қорғайды. Бізде әдепкі мән const,
//   ал sensitivity пернемен өзгертілетін жаһандық айнымалы.
//
//  3-ТАПСЫРМА. I — invert Y, тек жаңа басылу кезінде ауысады.
//   Алдыңғы күй сақталмаса, ұсталған перне әр кадрда режимді ауыстырады.
//   WASD үшін әр кадрда dt-мен қозғалу керісінше қажет.
//
//  4-ТАПСЫРМА. P — camPos, camFront, yaw, pitch, fov, length шығару.
//   Float дөңгелектеуінен length 1-ден сәл өзгеше болуы мүмкін.
//   Бағыт формуласының теориялық ұзындығы normalize болмаса да 1:
//   cos^2(pitch)*(cos^2(yaw)+sin^2(yaw))+sin^2(pitch)=1.
//   normalize сандық қателікті азайтып, бірлік бағытты сақтайды.
//
//  5-ТАПСЫРМА. Зумның жоғарғы шегі 90 градус.
//   Үлкен FOV-та кең бұрыш көбірек кеңістікті қамтиды, ал шеттегі нысандар
//   жазық экранға перспективалық проекцияланғанда созылып көрінеді.
//   Бұл қалыпты проекция қасиеті; камера орны зум кезінде өзгермейді.
//
//  ҚОСЫМША: тақырыпта бұрыштар/FOV, R — орын/бұрыштар/FOV қалпына келтіру,
//   yaw fmod арқылы (-360, 360) аралығында. Жүгіру бұрынғы Left Ctrl-де,
//   Left Shift төмен қозғалу үшін сақталған.
// =====================================================================
