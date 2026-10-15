// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add . && git commit -m "week06" && git tag week06 && git push --tags
//
//  Тег арқылы кез келген аптадағы күйге қайта оралуға болады.
//
//  Қазіргі күйі: 6-АПТА — 2D сахна: жол, жүріп бара жатқан машина,
//                секіретін доп (нақты гравитациямен)
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

bool  whiteBackground = false;

// === 4-АПТА: анимация жылдамдығы — W/S өзгертеді ===
// 6-аптада бұл бүкіл сахнаның уақыт жылдамдығы (1.0 = қалыпты).
// Машина да, доп та ОДАН ТҰРАТЫН ортақ уақытқа тәуелді, сондықтан
// W/S басқанда да олар бір-біріне ешқашан тимейді.
float speed = 1.0f;

// === 6-АПТА: дүние координаталары ===
// Y: -1 ... +1 (төменнен жоғары). X: -ASPECT ... +ASPECT.
// Вершина шейдері x-ты ASPECT-ке бөледі, сондықтан шеңбер дөңгелек болып қалады.
const float ASPECT = (float)WIDTH / (float)HEIGHT;   // 16/9

// Жер деңгейі (дөңгелектер мен доптың төменгі нүктесі осы жерде)
const float GROUND = -0.9f;

// Машина
const float CAR_HALF_W   = 0.40f;   // машина ені / 2
const float CAR_HEIGHT   = 0.34f;   // машинаның жалпы биіктігі (төбесіне дейін)
const float CAR_START_X  = ASPECT + CAR_HALF_W;      // оң жақтан экраннан тыс
const float CAR_END_X    = -(ASPECT + CAR_HALF_W);   // сол жақта экраннан тыс
const float ROAD_LENGTH  = CAR_START_X - CAR_END_X;

// Доп
const float BALL_R  = 0.15f;                // машинадан сәл кіші
const float BALL_X  = -ASPECT * 0.5f;       // сол жарты жолдың ортасы
const float BALL_T  = 2.0f;                 // бір секіру ұзақтығы (секунд)
const float BALL_H  = 0.90f;                // допның ең жоғарғы көтерілуі (төменгі нүктесі)

// Машина циклі = 2 доп циклі -> әр өткенде доп бірдей фазада болады.
const float CAR_PERIOD = 2.0f * BALL_T;
const float CAR_SPEED  = ROAD_LENGTH / CAR_PERIOD;

// === 3-АПТА: вершина шейдері түсті де қабылдайды ===
// Назар: #version жолдың ЕҢ БІРІНШІ символынан басталуы керек.
const char* vertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

// === 4-АПТА: CPU-дан келетін мәндер ===
uniform vec2  uOffset;
uniform float uScale;
// === 6-АПТА: терезенің ен/биіктік қатынасы (шеңбер эллипс болмауы үшін) ===
uniform float uAspect;

out vec3 vColor;

void main() {
    vec2 p = aPos.xy * uScale + uOffset;
    gl_Position = vec4(p.x / uAspect, p.y, aPos.z, 1.0);
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
//  === 6-АПТА: Mesh — VAO + VBO + EBO бір құрылымда (5-аптадағы әдіс) ===
//  Вершина: 6 float (xyz + rgb). Индекстер — сағат тіліне қарсы.
// ---------------------------------------------------------------------
struct Mesh {
    unsigned int vao = 0, vbo = 0, ebo = 0;
    int count = 0;                 // ИНДЕКС саны (glDrawElements үшін)
};

Mesh makeMesh(const std::vector<float>& verts, const std::vector<unsigned int>& idx) {
    Mesh m;
    m.count = (int)idx.size();

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);

    glBindVertexArray(m.vao);                     // VAO бірінші

    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo); // EBO — VAO байланып тұрғанда
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int),
                 idx.data(), GL_STATIC_DRAW);

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

// Төртбұрыш: төрт бұрыш сағат тіліне қарсы берілуі керек
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

// Шеңбер: ортасы + 40 нүкте (желпуіш). Ортасы мен жиегінің түсі әртүрлі -> градиент.
void addCircle(std::vector<float>& v, std::vector<unsigned int>& idx,
               float cx, float cy, float rad,
               float cr, float cg, float cb,
               float er, float eg, float eb) {
    const int N = 40;
    unsigned int base = (unsigned int)(v.size() / 6);
    addVertex(v, cx, cy, cr, cg, cb);                       // 0 — орталығы
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
//  Жол: қара-сұр жолақ + ақ үзік сызықтар
// ---------------------------------------------------------------------
Mesh makeRoad() {
    std::vector<float> v;
    std::vector<unsigned int> idx;

    // Жол: x бойынша экраннан сәл кең
    addRect(v, idx, -ASPECT - 0.1f, -1.0f, ASPECT + 0.1f, -0.7f, 0.25f, 0.25f, 0.28f);

    // Үзік сызықтар (дөңгелектің астында, жолдың төменгі бөлігінде)
    for (float x = -ASPECT; x < ASPECT; x += 0.6f) {
        addRect(v, idx, x, -0.975f, x + 0.3f, -0.950f, 0.95f, 0.95f, 0.95f);
    }
    return makeMesh(v, idx);
}

// ---------------------------------------------------------------------
//  Машина. Жергілікті координата: (0,0) — төменгі орта нүкте (жермен жанасу).
//  Алдыңғы жағы СОЛ жақта (машина солға жүреді).
// ---------------------------------------------------------------------
Mesh makeCar() {
    std::vector<float> v;
    std::vector<unsigned int> idx;

    // Кузов (қызыл)
    addRect(v, idx, -0.40f, 0.07f, 0.40f, 0.20f, 0.85f, 0.10f, 0.10f);

    // Кабина (трапеция, сәл күңгірт қызыл)
    addQuad(v, idx, -0.28f, 0.20f,   0.22f, 0.20f,   0.12f, CAR_HEIGHT,  -0.14f, CAR_HEIGHT,
            0.70f, 0.08f, 0.08f);

    // Терезе (ашық көк)
    addQuad(v, idx, -0.22f, 0.22f,   0.16f, 0.22f,   0.09f, 0.32f,  -0.11f, 0.32f,
            0.65f, 0.88f, 1.00f);

    // Фара (сары) — алдыңғы жағында
    addRect(v, idx, -0.40f, 0.12f, -0.35f, 0.17f, 1.0f, 0.95f, 0.30f);

    // Дөңгелектер: қара шина + сұр диск. Кузовтан кейін салынады -> алдында көрінеді
    const float wx[2] = { -0.25f, 0.25f };
    for (int i = 0; i < 2; i++) {
        addCircle(v, idx, wx[i], 0.07f, 0.07f,   0.10f, 0.10f, 0.10f,   0.10f, 0.10f, 0.10f);
        addCircle(v, idx, wx[i], 0.07f, 0.03f,   0.75f, 0.75f, 0.78f,   0.75f, 0.75f, 0.78f);
    }
    return makeMesh(v, idx);
}

// ---------------------------------------------------------------------
//  Доп. Жергілікті координата: (0,0) — доптың орталығы.
// ---------------------------------------------------------------------
Mesh makeBall() {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    addCircle(v, idx, 0.0f, 0.0f, BALL_R,   1.0f, 0.65f, 0.35f,   0.90f, 0.35f, 0.05f);
    return makeMesh(v, idx);
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

    // Бастапқы viewport (Retina экранда framebuffer терезеден үлкен болады)
    int fbw, fbh;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);

    // === 6-АПТА: үш фигура ===
    Mesh road = makeRoad();
    Mesh car  = makeCar();
    Mesh ball = makeBall();

    // === 3-АПТА: шейдерлер makeShader() арқылы ===
    unsigned int shader = makeShader(vertexSrc, fragmentSrc);

    // === 4-АПТА: uniform орнын цикл алдында БІР РЕТ табамыз ===
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale  = glGetUniformLocation(shader, "uScale");
    int locAspect = glGetUniformLocation(shader, "uAspect");

    // Өзгермейтін uniform-дар бір рет жіберіледі
    glUseProgram(shader);
    glUniform1f(locAspect, ASPECT);
    glUniform1f(locScale, 1.0f);

    // === 6-АПТА: доптың фазасын есептейміз ===
    // Машина доптың x-ына жеткен сәтте доп ЕҢ ЖОҒАРЫ нүктеде болуы керек.
    // Сонда машина доптың астынан өтеді, тимейді.
    float tPass = (CAR_START_X - BALL_X) / CAR_SPEED;    // машина доптың x-ына жететін уақыт
    float ballPhase = BALL_T * 0.5f - tPass;
    ballPhase = std::fmod(std::fmod(ballPhase, BALL_T) + BALL_T, BALL_T);

    // === 4-АПТА: уақыт және анимация күйі ===
    float  lastFrame = (float)glfwGetTime();   // НАЗАР: нөл емес!
    double simTime   = 0.0;                    // сахнаның ортақ уақыты

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

        // --- Машина: оңнан солға, сосын қайтадан оңнан (шексіз) ---
        float carT = (float)std::fmod(simTime, (double)CAR_PERIOD);
        float carX = CAR_START_X - CAR_SPEED * carT;

        // --- Доп: тұрақты гравитация -> парабола ---
        // h = H * (1 - s^2), s = -1...+1. Төменде жылдам, жоғарыда баяу:
        // көтерілгенде бірте-бірте баяулайды, құлағанда бірте-бірте жылдамдайды.
        float bt = (float)std::fmod(simTime + (double)ballPhase, (double)BALL_T);
        float s  = 2.0f * (bt / BALL_T) - 1.0f;
        float h  = BALL_H * (1.0f - s * s);
        float ballY = GROUND + BALL_R + h;

        // ================= СЫЗУ: күйді көрсету =================
        if (whiteBackground) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            glClearColor(0.53f, 0.81f, 0.92f, 1.0f);   // аспан
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);

        // Жол
        glUniform2f(locOffset, 0.0f, 0.0f);
        glBindVertexArray(road.vao);
        glDrawElements(GL_TRIANGLES, road.count, GL_UNSIGNED_INT, 0);

        // Машина
        glUniform2f(locOffset, carX, GROUND);
        glBindVertexArray(car.vao);
        glDrawElements(GL_TRIANGLES, car.count, GL_UNSIGNED_INT, 0);

        // Доп (орнында секіреді: x тұрақты, y өзгереді)
        glUniform2f(locOffset, BALL_X, ballY);
        glBindVertexArray(ball.vao);
        glDrawElements(GL_TRIANGLES, ball.count, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);   // дайын кадрды экранға шығару
        glfwPollEvents();          // пернетақта/тінтуір оқиғаларын өңдеу
    }

    // -----------------------------------------------------------------
    //  5. Тазалау
    // -----------------------------------------------------------------
    destroyMesh(road);
    destroyMesh(car);
    destroyMesh(ball);
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
//  (6-аптада сахна ауыстырылды, EBO әдісі makeMesh() ішінде қалды.)
// =====================================================================

// =====================================================================
//  6-АПТА ТАПСЫРМАСЫ (2D сахна)
//  1. Машина фигурасы оңғы төменнен солға қарай жүреді, сол жақтан
//     шыққанда оң жақтан қайта пайда болады (шексіз) — орындалды.
//  2. Доп машинадан сәл кіші (радиус 0.15, машина биіктігі 0.34) —
//     орындалды.
//  3. Доп сол жарты жолдың ортасында тұрып, орнында секіреді — орындалды.
//  4. Секіру нақты физикамен: h = H*(1 - s^2). Жоғары көтерілгенде баяулайды,
//     құлағанда жылдамдайды — орындалды.
//  5. Машина мен доп ешқашан тимейді: машина доптың x-ынан өткенде доп ең
//     жоғарғы нүктеде болады (ballPhase), ал машина циклі доп циклінен
//     дәл 2 есе ұзақ (CAR_PERIOD = 2 * BALL_T) — орындалды.
// =====================================================================