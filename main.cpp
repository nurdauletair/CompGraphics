// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week07: координаттар кеңістіктері, MVP, проекция"
//      git tag week07 && git push origin main --tags
//
//  Тег арқылы кез келген аптадағы күйге қайта оралуға болады.
//
//  Қазіргі күйі: 7-АПТА — координаттар кеңістіктері, MVP, проекция.
//                Y осінен айналатын кубтар (үш куб).
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

// === 7-АПТА: терезенің ағымдағы өлшемі (resize кезінде жаңартылады) ===
// aspect осыдан есептеледі, тұрақты 800/600 емес.
int gWidth  = WIDTH;
int gHeight = HEIGHT;

bool  whiteBackground = false;

// === 4-АПТА: анимация жылдамдығы — W/S өзгертеді ===
float speed = 1.0f;

// === 7-АПТА, 1-ТАПСЫРМА: проекция параметрлері ===
// Эксперимент үшін тек осы үш санды өзгертіп, қайта құрастыр.
const float FOV_DEG    = 45.0f;     // көру бұрышы (градус): 30, 45, 90, 120
const float NEAR_PLANE = 0.1f;      // жақын жазықтық: 0.1, 1.0, 2.5, 4.0   (0 болмауы керек!)
const float FAR_PLANE  = 100.0f;    // алыс жазықтық: 100.0, 5.0, 2.0

// === 7-АПТА: бір вершинаға 6 float: позиция (xyz) + түс (rgb) ===
// Куб: 6 қыр x 2 үшбұрыш x 3 вершина = 36 вершина.
// 8 емес: бұрыш үш қырға ортақ, ал үш қырдың түсі әртүрлі, ал бір вершина
// үш түс ұстай алмайды. Әр қырдың өз түсі бар -> қырлардың шекарасы көрінеді.
float vertices[] = {
    // артқы қыр (қызыл)
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
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

    // оң жақ қыр (сары)
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

    // төменгі қыр (қызғылт)
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,

    // жоғарғы қыр (көгілдір)
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f
};

// === 7-АПТА, 3-ТАПСЫРМА: үш куб — әрқайсысы өз орнында, өз жылдамдығымен ===
glm::vec3 cubePositions[3] = {
    glm::vec3(-1.5f, 0.0f,  0.0f),
    glm::vec3( 0.0f, 0.0f, -1.0f),
    glm::vec3( 1.5f, 0.0f,  0.0f)
};
float cubeRates[3] = { 1.0f, 0.5f, 2.0f };   // айналу жылдамдығы (рад/сек)

// === 7-АПТА: вершина шейдері — үш матрица бір-біріне көбейтіледі ===
// Назар: #version жолдың ЕҢ БІРІНШІ символынан басталуы керек.
// Рет маңызды: оңнан солға оқылады — алдымен model, сосын view, сосын projection.
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

    // === 4-АПТА: W/S — жылдамдықты басқару ===
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        speed += 1.0f * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        speed -= 1.0f * dt;
        if (speed < 0.0f) speed = 0.0f;   // уақыт кері жүрмесін
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

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // Бастапқы өлшем (Retina экранда framebuffer терезеден үлкен болады)
    glfwGetFramebufferSize(window, &gWidth, &gHeight);
    glViewport(0, 0, gWidth, gHeight);

    // === 7-АПТА: куб — индекс жоқ, сондықтан EBO керек емес (VAO + VBO) ===
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

    // === 4-АПТА: уақыт және анимация күйі ===
    float  lastFrame = (float)glfwGetTime();   // НАЗАР: нөл емес!
    double simTime   = 0.0;                    // сахнаның ортақ уақыты (W/S жылдамдатады)

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

        // ================= СЫЗУ: күйді көрсету =================
        if (whiteBackground) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            glClearColor(0.12f, 0.14f, 0.20f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);

        // --- Матрицалар: цикл ІШІНДЕ құрылады ---
        // aspect — терезе өлшемінен, әр кадрда (resize кезінде куб созылмайды).
        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        // PROJECTION: view кеңістігі -> clip кеңістігі.
        // === 2-ТАПСЫРМА: O пернесін басып тұрсаң — ортографиялық проекция ===
        glm::mat4 projection;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, NEAR_PLANE, FAR_PLANE);
        } else {
            projection = glm::perspective(glm::radians(FOV_DEG), aspect, NEAR_PLANE, FAR_PLANE);
        }

        // VIEW: камера z = 3 нүктесінде, координат басына қарайды, "жоғары" = +Y.
        // Үш кубқа ортақ: бір рет есептеліп, бір рет жіберіледі.
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));

        // GL_FALSE: GLM матрицасын транспонирлеудің қажеті жоқ.
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        // Бір geometry (бір VAO), үш рет сызылады: әр кубтың өз MODEL матрицасы.
        glBindVertexArray(vao);
        for (int i = 0; i < 3; i++) {
            // Алдымен өз ортасынан Y осімен бұрамыз, сосын орнына апарамыз (T * R).
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
            model = glm::rotate(model, (float)simTime * cubeRates[i], glm::vec3(0.0f, 1.0f, 0.0f));

            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);      // индекс жоқ -> glDrawArrays, 36 вершина
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
//  2. uScale қосып, пульсация жасау — орындалды.
//  3. W/S-пен жылдамдықты басқару — орындалды (processInput ішінде, dt-мен).
// =====================================================================

// =====================================================================
//  5-АПТА ТАПСЫРМАЛАРЫ (семинар 5: индекс буфері)
//  1. Индекстерді {0, 1, 2} ет -> тек бір үшбұрыш көрінеді.
//  2. Бесбұрыш (5 вершина, 3 үшбұрыш, 9 индекс) — орындалды.
//  3. Төртбұрышты екі рет сызу — орындалды.
// =====================================================================

// =====================================================================
//  6-АПТА ТАПСЫРМАСЫ (2D сахна: машина мен секіретін доп) — орындалды.
//  Тег: week06. (7-аптада сахна кубқа ауыстырылды.)
// =====================================================================

// =====================================================================
//  7-АПТА ТАПСЫРМАЛАРЫ (семинар 7: координаттар кеңістіктері, MVP)
//
//  БАЗАЛЫҚ БӨЛІМ — орындалды:
//   1. 36 вершина (әр қырдың өз түсі), 6 float/вершина.
//   2. Шейдерде үш бөлек uniform: uModel, uView, uProjection,
//      рет: uProjection * uView * uModel.
//   3. gWidth/gHeight — жаһандық, framebuffer_size_callback ішінде жаңарады.
//   4. Uniform орындары цикл алдында кэштелген.
//   5. Үш матрица цикл ішінде құрылады (aspect терезеден есептеледі).
//   6. glDrawArrays(GL_TRIANGLES, 0, 36), бұру осі (0, 1, 0).
//   Ескерту: тереңдік тесті әлі жоқ (8-апта), сондықтан артқы қырлар
//   алдыңғысын жауып тұрады — бұл қате емес.
//
//  1-ТАПСЫРМА. Проекция параметрлерін зерттеу (камера z=3, куб ортада,
//  қыры 1 -> алдыңғы беті 2.5, артқы беті 3.5; айналғанда бұрыштары
//  2.29 ... 3.71 аралығында болады).
//   fov:
//     30°  — куб үлкен көрінеді (камера "жақындатқандай").
//     45°  — қалыпты.
//     90°  — куб айтарлықтай кіші, перспектива күшті.
//     120° — куб өте кішкентай, шеттері қатты бұрмаланған.
//   near:
//     0.1, 1.0 — ештеңе өзгермейді (жақын жазықтық кубқа жетпейді).
//     2.5      — кубтың камераға жақын бұрыштары near-нен жақын болып қалады,
//                сондықтан кесіледі: куб "кесілген" көрінеді.
//     4.0      — near кубтың ең алыс нүктесінен (3.71) де алыс -> куб түгел
//                кесіледі, экран бос.
//   far:
//     100.0, 5.0 — куб көрінеді (кубтың ең алыс нүктесі 3.71 < 5).
//     2.0        — far кубтың ең жақын нүктесінен (2.29) де жақын -> куб
//                  түгел жоғалады.
//   Қорытынды: куб толық жоғалатын жағдайлар — near = 4.0 және far = 2.0.
//   (near = 2.5 жағдайында куб жартылай кесіледі — экранда тексер.)
//
//  2-ТАПСЫРМА. Ортография (O пернесін басып тұр, перспективамен салыстыр):
//   Куб айналады, бірақ көлем жоғалады: жақын және алыс қырлар бірдей
//   өлшемде. Ортографияда w = 1 әрдайым (бөлгіш өзгермейді), ал
//   перспективада w = камерадан қашықтық -> алыс нәрсе кішірейеді.
//
//  3-ТАПСЫРМА. Үш куб — орындалды (бір VAO, цикл ішінде үш рет
//  "model құру -> glUniformMatrix4fv -> glDrawArrays").
//   Ортадағы куб z = -1 жерде, яғни камерадан 4 бірлік (қалғандары 3),
//   сондықтан перспективада кішірек көрінеді.
//   view матрицасы әр кубқа бөлек есептелмейді — бір рет есептеліп,
//   үшеуіне ортақ жіберіледі.
// =====================================================================