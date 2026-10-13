// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week05" && git tag week05 && git push --tags
//
//  Тег арқылы кез келген аптадағы күйге қайта оралуға болады.
//
//  Қазіргі күйі: 5-АПТА — индекс буфері (EBO), төртбұрыш
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

bool  whiteBackground = false;

// === 4-АПТА: анимация жылдамдығы (радиан/секунд) — W/S өзгертеді ===
float speed = 1.5f;

// === 3-АПТА: вершина шейдері енді түсті де қабылдайды ===
// Назар: #version жолдың ЕҢ БІРІНШІ символынан басталуы керек.
const char* vertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

// === 4-АПТА: CPU-дан келетін мәндер ===
uniform vec2  uOffset;
uniform float uScale;

out vec3 vColor;

void main() {
    gl_Position = vec4(aPos.xy * uScale + uOffset, aPos.z, 1.0);
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
//  Терезе өлшемі өзгергенде шақырылады
// ---------------------------------------------------------------------
void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру. Әр кадрда шақырылады.
//  4-апта: dt қосылды — W/S жеделдету болғандықтан dt МІНДЕТТІ.
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        whiteBackground = true;
    }

    // === 4-АПТА: W/S — жылдамдықты басқару (секундына 2 рад/сек өзгереді) ===
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        speed += 2.0f * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        speed -= 2.0f * dt;
    }
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
    glfwSetFramebufferSizeCallback(window, onResize);
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

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";


    // === 5-АПТА: үшбұрыштың орнына төртбұрыш — 4 вершина ===
    // Әр вершина бұрынғыдай 6 float: позиция (xyz) + түс (rgb).
    // 0.3f (0.5f емес): орбита радиусы 0.4, 0.3 + 0.4 = 0.7 — NDC шекарасына жетпейді.
    float vertices[] = {
        // позиция          // түс
         0.3f,  0.3f, 0.0f,  1.0f, 0.0f, 0.0f,   // 0 — оң жоғарғы
         0.3f, -0.3f, 0.0f,  0.0f, 1.0f, 0.0f,   // 1 — оң төменгі
        -0.3f, -0.3f, 0.0f,  0.0f, 0.0f, 1.0f,   // 2 — сол төменгі
        -0.3f,  0.3f, 0.0f,  1.0f, 1.0f, 0.0f    // 3 — сол жоғарғы
    };

    // === 5-АПТА: индекстер — вершина НӨМІРЛЕРІ, сағат тіліне қарсы ===
    unsigned int indices[] = {
        0, 1, 3,    // бірінші үшбұрыш
        1, 2, 3     // екінші үшбұрыш
    };

    unsigned int vao, vbo, ebo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);                        // === 5-АПТА ===

    glBindVertexArray(vao);                       // VAO бірінші

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
                 vertices, GL_STATIC_DRAW);

    // === 5-АПТА: EBO — VAO байланып тұрғанда жасалады ===
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),
                 indices, GL_STATIC_DRAW);

    const int STRIDE = 6 * sizeof(float);

    // location = 0: позиция
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);

    // location = 1: түс. Offset = 3 float
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // НАЗАР: GL_ELEMENT_ARRAY_BUFFER-ді 0-ге босатпаймыз!
    // EBO — VAO күйінің бөлігі, босатсаң VAO-дан да жоғалады.
    glBindVertexArray(0);

    // === 5-АПТА, 2-ТАПСЫРМА: бесбұрыш — 5 вершина, 3 үшбұрыш, 9 индекс ===
    // Вершиналар радиусы 0.25 шеңбердегі 5 нүкте, 90° -тан сағат тіліне қарсы.
    float pentVertices[] = {
        // позиция              // түс
         0.0000f,  0.2500f, 0.0f,  1.0f, 0.0f, 0.0f,   // 0 — жоғарғы
        -0.2378f,  0.0773f, 0.0f,  1.0f, 1.0f, 0.0f,   // 1 — сол жоғарғы
        -0.1469f, -0.2023f, 0.0f,  0.0f, 1.0f, 0.0f,   // 2 — сол төменгі
         0.1469f, -0.2023f, 0.0f,  0.0f, 1.0f, 1.0f,   // 3 — оң төменгі
         0.2378f,  0.0773f, 0.0f,  0.0f, 0.0f, 1.0f    // 4 — оң жоғарғы
    };

    // Желпуіш: үш үшбұрыштың үшеуі де 0-ші вершинадан шығады, барлығы сағат тіліне қарсы.
    unsigned int pentIndices[] = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 4
    };

    unsigned int pentVao, pentVbo, pentEbo;
    glGenVertexArrays(1, &pentVao);
    glGenBuffers(1, &pentVbo);
    glGenBuffers(1, &pentEbo);

    glBindVertexArray(pentVao);

    glBindBuffer(GL_ARRAY_BUFFER, pentVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pentVertices), pentVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pentEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pentIndices), pentIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // === 3-АПТА: шейдерлер енді makeShader() арқылы (16 жол -> 1 жол) ===
    unsigned int shader = makeShader(vertexSrc, fragmentSrc);

    // === 4-АПТА: uniform орнын цикл алдында БІР РЕТ табамыз ===
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale  = glGetUniformLocation(shader, "uScale");

    // === 4-АПТА: уақыт және анимация күйі ===
    float lastFrame = (float)glfwGetTime();   // НАЗАР: нөл емес!
    float angle = 0.0f;                       // шеңбердегі орын (радиан)

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
                      << "  speed: " << speed << " рад/с\n";
            frameCount = 0;
            fpsTimer = now;
        }

        // ================= ЖАҢАРТУ: күйді есептеу =================
        processInput(window, dt);
        angle += speed * dt;

        float offsetX = std::cos(angle) * 0.4f;   // радиус 0.4
        float offsetY = std::sin(angle) * 0.4f;
        float scale   = 0.75f + 0.25f * std::sin(now * 3.0f);  // пульсация

        // ================= СЫЗУ: күйді көрсету =================
        // --- Экранды тазалау (1-апта) ---
        float r = (std::sin(now * 2.0f) + 1.0f) * 0.5f * 0.3f;
        float g = (std::sin(now * 1.5f) + 1.0f) * 0.5f * 0.3f;

        if (whiteBackground) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            glClearColor(r, g, 0.35f, 1.0f);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        // === 5-АПТА: glDrawArrays орнына glDrawElements ===
        glUseProgram(shader);                       // 1. АЛДЫМЕН осы!
        glUniform2f(locOffset, offsetX, offsetY);   // 3. мән жіберу
        glUniform1f(locScale, scale);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);   // 6 = ИНДЕКС саны

        // === 5-АПТА, 3-ТАПСЫРМА: сол төртбұрыш екінші рет, қарама-қарсы орында ===
        // Бір glBindVertexArray, екінші glUniform2f + glDrawElements.
        glUniform2f(locOffset, -offsetX, -offsetY);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // === 5-АПТА, 2-ТАПСЫРМА: бесбұрыш (9 индекс), сол төменгі бұрышта тұрады ===
        glUniform2f(locOffset, -0.7f, -0.6f);
        glUniform1f(locScale, 1.0f);
        glBindVertexArray(pentVao);
        glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);   // дайын кадрды экранға шығару
        glfwPollEvents();          // пернетақта/тінтуір оқиғаларын өңдеу
    }

    // -----------------------------------------------------------------
    //  5. Тазалау
    // -----------------------------------------------------------------
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);      // === 5-АПТА: ЖАҢА ===
    glDeleteVertexArrays(1, &pentVao);
    glDeleteBuffers(1, &pentVbo);
    glDeleteBuffers(1, &pentEbo);
    glDeleteProgram(shader);

    glfwTerminate();
    return 0;
}


// =====================================================================
//  1-АПТА ТАПСЫРМАЛАРЫ — орындалды
//  1. Терезе 1280x720.   2. Фон жылдамдығы.   3. Пробел = ақ фон.   4. FPS.
// =====================================================================

// =====================================================================
//  3-АПТА ТАПСЫРМАЛАРЫ (семинар 3: түрлі-түсті үшбұрыш)
//  1. Түстерді төңкер: fragment шейдерде  vec4(1.0 - vColor, 1.0)
//     (бастапқы жолдың астында комментарийде жазылған — ауыстырып көр).
//  2. Үш вершинаға бірдей түс бер (vertices ішінде) -> градиент жоғалады,
//     себебі интерполяцияланатын үш мән бірдей.
//  3. Шейдерге әдейі қате жаз (мысалы, `float x = 1;` немесе `;` алып таста)
//     -> консольде makeShader() жариялаған қате хабарламасын оқы.
//     Жол нөмірі #version жолынан саналады.
// =====================================================================

// =====================================================================
//  4-АПТА ТАПСЫРМАЛАРЫ (семинар 4: шеңбер бойымен қозғалу)
//  1. dt-ны алып тастап (angle += speed * 0.02f;), көршімен салыстыр:
//     FPS әртүрлі -> жылдамдық әртүрлі.
//  2. uScale қосып, пульсация жасау — орындалды (scale, sin(now * 3.0)).
//  3. W/S-пен жылдамдықты басқару — орындалды (processInput ішінде, dt-мен).
// =====================================================================

// =====================================================================
//  5-АПТА ТАПСЫРМАЛАРЫ (семинар 5: индекс буфері)
//  1. Индекстерді {0, 1, 2} ет. Не көрінеді? Неге?
//     -> тек бір үшбұрыш (0, 1, 2 вершиналары) көрінеді: екінші үшбұрыш
//        {1, 2, 3} жоқ, сондықтан төртбұрыштың жартысы ғана қалады.
//        Тексеру үшін indices массивін {0, 1, 2} етіп, glDrawElements-те
//        6-ны 3-ке ауыстыр, содан кейін бәрін қайтадан бастапқы күйге қой.
//  2. Бесбұрыш жасалды — pentVertices (5), pentIndices (9), бөлек VAO/VBO/EBO.
//  3. Төртбұрыш екі рет сызылады — бір glBindVertexArray, екі glUniform2f +
//     glDrawElements, екінші рет қарама-қарсы орында (-offsetX, -offsetY).
// =====================================================================