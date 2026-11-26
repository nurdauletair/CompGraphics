// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  Бұл файл семестр бойы өседі. Әр аптада жаңа бөлік қосылады,
//  ескісі орнында қалады. Аптаның соңында:
//
//      git add main.cpp docs/week13/
//      git commit -m "week13: normals, Lambert, ambient + diffuse, normal matrix"
//      git tag week13 && git push origin main --tags
//
//  Тег арқылы кез келген аптадағы күйге қайта оралуға болады.
//
//  Қазіргі күйі: 13-АПТА — нормальдар, Lambert, ambient + diffuse.
//                Сол бес куб, еркін камера мен бұрынғы басқару сақталған.
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <cstdio>
#include <iostream>

#ifndef ASSET_DIR
#define ASSET_DIR "assets"         // CMake толық жолды өзі анықтайды
#endif

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

// === 7-АПТА: терезенің ағымдағы өлшемі (resize кезінде жаңартылады) ===
int gWidth  = WIDTH;
int gHeight = HEIGHT;

bool  whiteBackground = false;

// === 4-АПТА: анимация жылдамдығы — енді Page Up/Page Down өзгертеді ===
float speed = 1.0f;

// === 12-АПТА: екі текстураны араластыру ===
float blend = 0.0f;                 // Up/Down: секундына 0.5, шегі 0..1
enum BlendMode { MIX_MODE, MULTIPLY_MODE, ADD_MODE, CLAMP_ADD_MODE,
                 DECAL_MODE, GRADIENT_MODE, SMOOTHSTEP_MODE, BLEND_MODE_COUNT };
int blendMode = MIX_MODE;
const char* const BLEND_MODE_NAMES[] = {
    "mix", "multiply", "add", "clamped add", "alpha decal", "UV gradient", "smoothstep"
};
// 1/2-тапсырмадағы қателер тек F1/F2 арқылы әдейі қосылады; бастапқысы дұрыс.
bool brokenTextureUnits = false;
bool wrongSamplerId = false;
bool reportTextureState = true;

// === 13-АПТА: жарық эксперименттері (J/K — келесі/алдыңғы) ===
enum LightingMode { BASIC_LIGHTING, ORBIT_LIGHTING, NO_MAX_LIGHTING,
                    WRONG_NORMAL_LIGHTING, RIGHT_NORMAL_LIGHTING,
                    AMBIENT_LIGHTING, GOURAUD_LIGHTING, PHONG_LIGHTING,
                    LIGHTING_MODE_COUNT };
int lightingMode = ORBIT_LIGHTING;
const char* const LIGHTING_MODE_NAMES[] = {
    "basic Lambert", "orbiting light", "NO MAX (intentional)",
    "WRONG normal matrix", "inverse transpose", "ambient control",
    "Gouraud (per vertex)", "Phong shading (per fragment)"
};
float ambientStrength = 0.1f;
bool animateObjects = false;       // V: орбитадағы жарықты айналмайтын кубта байқау оңай
const glm::vec3 lightColor(1.0f);

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

// === 13-АПТА: 8 float: позиция (xyz) + UV (uv) + нормаль (xyz) ===
// Куб: 6 жақ x 2 үшбұрыш x 3 вершина = 36 вершина.
// Әр жақтың екі үшбұрышы сырттан қарағанда CCW, текстура толық жайылады:
// UV: (0,0) (1,0) (1,1), (0,0) (1,1) (0,1).
float vertices[] = {
    // артқы жақ (-Z)
     0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,

    // алдыңғы жақ (+Z)
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,

    // сол жақ (-X)
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  -1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,

    // оң жақ (+X)
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  1.0f, 0.0f, 0.0f,

    // төменгі жақ (-Y)
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, -1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f, -1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, -1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  0.0f, -1.0f, 0.0f,

    // жоғарғы жақ (+Y)
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f
};
static_assert(sizeof(vertices) / sizeof(float) == 36 * 8, "36 vertices: xyz + UV + normal");

// === 8-АПТА: бес куб — позициялар массиві ===
glm::vec3 positions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  0.5f, -2.0f),
    glm::vec3(-1.8f, -0.3f, -1.0f),
    glm::vec3( 1.2f, -0.8f, -3.5f),
    glm::vec3(-2.2f,  1.0f, -4.0f)
};

glm::vec3 sceneLightPosition(double time) {
    if (lightingMode == ORBIT_LIGHTING)
        return glm::vec3(1.5f * std::sin(static_cast<float>(time)), 1.0f,
                         1.5f * std::cos(static_cast<float>(time)));
    if (lightingMode == NO_MAX_LIGHTING) return glm::vec3(0.0f, 0.0f, -3.0f);
    if (lightingMode == GOURAUD_LIGHTING || lightingMode == PHONG_LIGHTING)
        return glm::vec3(0.0f, 0.0f, 1.2f);
    return glm::vec3(1.2f, 1.0f, 2.0f);
}

glm::mat4 sceneModel(int index, float angle) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), positions[index]);
    if (index == 0 && (lightingMode == WRONG_NORMAL_LIGHTING ||
                       lightingMode == RIGHT_NORMAL_LIGHTING)) {
        // GLM: S * R — алдымен Z бойынша 45 градус, содан кейін Y бойынша созу.
        model = glm::scale(model, glm::vec3(1.0f, 2.5f, 1.0f));
        return glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    if (lightingMode == GOURAUD_LIGHTING || lightingMode == PHONG_LIGHTING ||
        lightingMode == NO_MAX_LIGHTING) return model;
    const float rotation = lightingMode == ORBIT_LIGHTING && animateObjects ? angle : 0.7f;
    return glm::rotate(model, rotation * (0.5f + index * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));
}

glm::mat3 sceneNormalMatrix(const glm::mat4& model, int index) {
    if (index == 0 && lightingMode == WRONG_NORMAL_LIGHTING)
        return glm::mat3(model);   // тек 3-тапсырманың қате нұсқасында
    return glm::transpose(glm::inverse(glm::mat3(model)));
}

void selectLightingMode(int mode) {
    lightingMode = (mode + LIGHTING_MODE_COUNT) % LIGHTING_MODE_COUNT;
    ambientStrength = lightingMode == NO_MAX_LIGHTING ? 0.4f : 0.1f;
    // Одинаковая камера для каждой пары сравнения; растянутый куб помещается целиком.
    if (lightingMode == WRONG_NORMAL_LIGHTING || lightingMode == RIGHT_NORMAL_LIGHTING ||
        lightingMode == GOURAUD_LIGHTING || lightingMode == PHONG_LIGHTING) {
        const bool stretched = lightingMode == WRONG_NORMAL_LIGHTING || lightingMode == RIGHT_NORMAL_LIGHTING;
        camPos = stretched ? glm::vec3(3.5f, 2.0f, 6.0f) : glm::vec3(0.0f, 0.0f, 3.0f);
        camFront = glm::normalize(-camPos);
        yaw = glm::degrees(std::atan2(camFront.z, camFront.x));
        pitch = glm::degrees(std::asin(camFront.y));
        firstMouse = true;
    }
    std::cout << "Lighting " << lightingMode + 1 << "/" << LIGHTING_MODE_COUNT
              << ": " << LIGHTING_MODE_NAMES[lightingMode]
              << " | ambient = " << ambientStrength << "\n";
}

// === 7-АПТА: вершина шейдері — үш матрица бір-біріне көбейтіледі ===
// Назар: #version жолдың ЕҢ БІРІНШІ символынан басталуы керек.
const char* vertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec3 LightResult;
uniform mat3 normalMatrix;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform bool clampDiffuse;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = normalMatrix * aNormal;
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
    // Гуро нұсқасы: жарық төбелерде есептеліп, фрагментке интерполяцияланады.
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuse = dot(norm, lightDir);
    if (clampDiffuse) diffuse = max(diffuse, 0.0);
    LightResult = (ambientStrength + diffuse) * lightColor;
}
)";

const char* fragmentSrc = R"(#version 330 core
in vec2 vTexCoord;
in vec3 FragPos;
in vec3 Normal;
in vec3 LightResult;
out vec4 FragColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform bool clampDiffuse;
uniform bool useGouraud;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float blend;
uniform int blendMode;

void main() {
    vec4 a = texture(texture1, vTexCoord);
    vec4 b = texture(texture2, vTexCoord);
    vec4 surface;
    if (blendMode == 1) {
        surface = a * b;
    } else if (blendMode == 2) {
        surface = a + b;
    } else if (blendMode == 3) {
        surface = clamp(a + b, 0.0, 1.0);
    } else if (blendMode == 4) {
        surface = mix(a, b, b.a);
    } else if (blendMode == 5) {
        surface = mix(a, b, vTexCoord.y);
    } else if (blendMode == 6) {
        float t = smoothstep(0.3, 0.7, vTexCoord.y);
        surface = mix(a, b, t);
    } else {
        surface = mix(a, b, blend);
    }
    vec3 lighting = LightResult;
    if (!useGouraud) {
        // Lambert: нормаль мен жарық бағыты екеуі де әлем координаттарында.
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diffuse = dot(norm, lightDir);
        if (clampDiffuse) diffuse = max(diffuse, 0.0);
        lighting = (ambientStrength + diffuse) * lightColor;
    }
    FragColor = vec4(lighting * surface.rgb, surface.a);
}
)";

// Қосымша Қ1: жарық көзін сол VAO-мен кішкентай ақ куб ретінде көрсету.
const char* lampVertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
)";
const char* lampFragmentSrc = R"(#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(1.0); }
)";

// === 11-АПТА: суретті CPU-ға оқу, GPU-ға көшіру, mipmap құру ===
unsigned int loadTexture(const char* path) {
    stbi_set_flip_vertically_on_load(true); // суреттің жоғарғы жағын UV-ге сәйкестендіру
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (!data) {
        std::cerr << "Сурет жүктелмеді: " << path << "\n"
                  << "stb_image: " << stbi_failure_reason() << "\n";
        return 0;
    }

    // JPG/RGB PNG -> RGB, альфасы бар PNG -> RGBA; сұр суреттер де қолдау табады.
    const GLenum formats[] = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
    const GLenum fmt = formats[n - 1];
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (n < 3) {
        const GLint swizzle[] = {GL_RED, GL_RED, GL_RED, n == 2 ? GL_GREEN : GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    }

    // RGB сурет ені 4-ке бөлінбесе де, жолдар дұрыс оқылуы керек.
    GLint previousAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
    glGenerateMipmap(GL_TEXTURE_2D);  // тек негізгі деңгей жүктелгеннен кейін

    stbi_image_free(data);           // CPU көшірмесі енді қажет емес
    glBindTexture(GL_TEXTURE_2D, 0);
    std::cout << path << " " << w << "x" << h << " арна: " << n << "\n";
    return id;
}

// === 12-АПТА: әр кадрдағы екі unit пен sampler байланысы ===
void bindSceneTextures(unsigned int tex1, unsigned int tex2, unsigned int logo,
                       int locTexture2) {
    const unsigned int secondTexture = blendMode == DECAL_MODE ? logo : tex2;
    if (brokenTextureUnits) {
        // 1-тапсырма: екі bind арасында unit ауыспаса не болады?
        // Детерминді бастапқы күй: unit 1 бос, ағымдағы unit 0.
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glBindTexture(GL_TEXTURE_2D, secondTexture); // tex1-ді ығыстырады
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, secondTexture);
    }
    // 2-тапсырма: F2 қосылғанда ғана ID қасақана unit ретінде беріледі.
    glUniform1i(locTexture2, wrongSamplerId ? static_cast<int>(secondTexture) : 1);
    if (reportTextureState) {
        GLint bound0, bound1;
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound0);
        glActiveTexture(GL_TEXTURE1);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound1);
        std::cout << "unit 0 -> ID " << bound0 << "; unit 1 -> ID " << bound1
                  << "; texture1 sampler = 0; texture2 sampler = "
                  << (wrongSamplerId ? secondTexture : 1)
                  << "; blend = " << blend << "; mode = " << BLEND_MODE_NAMES[blendMode] << "\n";
        reportTextureState = false;
    }
}

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

// Бір реттік әрекеттер үшін: ұсталған перне режимді әр кадрда ауыстырмайды.
bool keyPressedOnce(GLFWwindow* window, int key) {
    static bool previous[GLFW_KEY_LAST + 1] = {};
    const bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
    const bool justPressed = pressed && !previous[key];
    previous[key] = pressed;
    return justPressed;
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

    // === 13-АПТА: Mac-та F пернелерін қажет етпейтін басқару ===
    if (keyPressedOnce(window, GLFW_KEY_J)) selectLightingMode(lightingMode + 1);
    if (keyPressedOnce(window, GLFW_KEY_K)) selectLightingMode(lightingMode - 1);
    if (keyPressedOnce(window, GLFW_KEY_H)) {
        selectLightingMode(ORBIT_LIGHTING);
        blend = 0.0f;
        blendMode = MIX_MODE;
        brokenTextureUnits = false;
        wrongSamplerId = false;
        animateObjects = false;
        reportTextureState = true;
    }
    if (keyPressedOnce(window, GLFW_KEY_V)) animateObjects = !animateObjects;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) ambientStrength += 0.3f * dt;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) ambientStrength -= 0.3f * dt;
    if (ambientStrength < 0.0f) ambientStrength = 0.0f;
    if (ambientStrength > 0.5f) ambientStrength = 0.5f;

    // === 12-АПТА: 4..0 — жеті шейдер нұсқасын қайта құрастырмай салыстыру ===
    const int modeKeys[] = {GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7,
                           GLFW_KEY_8, GLFW_KEY_9, GLFW_KEY_0};
    for (int mode = 0; mode < BLEND_MODE_COUNT; ++mode) {
        if (keyPressedOnce(window, modeKeys[mode])) {
            blendMode = mode;
            std::cout << "Mode: " << BLEND_MODE_NAMES[mode] << "\n";
            reportTextureState = true;
        }
    }
    if (keyPressedOnce(window, GLFW_KEY_F1)) {
        brokenTextureUnits = !brokenTextureUnits;
        wrongSamplerId = false;
        blendMode = MIX_MODE;
        reportTextureState = true;
    }
    if (keyPressedOnce(window, GLFW_KEY_F2)) {
        wrongSamplerId = !wrongSamplerId;
        brokenTextureUnits = false;
        blendMode = MIX_MODE;
        reportTextureState = true;
    }
    if (keyPressedOnce(window, GLFW_KEY_F3)) {
        blend = 0.5f;
        blendMode = MIX_MODE;
        brokenTextureUnits = false;
        wrongSamplerId = false;
        reportTextureState = true;
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) blend += 0.5f * dt;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) blend -= 0.5f * dt;
    if (blend > 1.0f) blend = 1.0f;
    if (blend < 0.0f) blend = 0.0f;

    // === 4-АПТА: анимация жылдамдығын басқару (W/S енді камераға тиесілі) ===
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
        speed += 1.0f * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
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
        std::cout << "Lighting: " << LIGHTING_MODE_NAMES[lightingMode]
                  << " | ambient = " << ambientStrength << " | animate cubes = " << animateObjects << "\n";
        std::cout << "blend = " << blend << " mode = " << BLEND_MODE_NAMES[blendMode]
                  << " brokenUnits = " << brokenTextureUnits
                  << " wrongSamplerId = " << wrongSamplerId << "\n";
        reportTextureState = true;
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
                 "R: reset camera | P: full state | Page Up/Down: animation speed\n"
                 "Up/Down: blend | 4: mix | 5: multiply | 6: add | 7: clamped add\n"
                 "8: alpha logo | 9: UV gradient | 0: smoothstep\n"
                 "F1: broken units | F2: wrong sampler ID | F3: restore blending\n"
                 "J/K: next/previous lighting experiment | O/L: ambient up/down\n"
                 "H: restore lighting | V: animate cubes in orbit mode\n"
                 "B: white background | Q: orthographic (hold) | Esc: exit\n";

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

    const int STRIDE = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // === 3-АПТА: шейдерлер makeShader() арқылы ===
    unsigned int shader = makeShader(vertexSrc, fragmentSrc);

    // === 12-АПТА: екі sampler, екінші unit үшін pattern не logo таңдалады ===
    unsigned int tex1 = loadTexture(ASSET_DIR "/container.png");
    unsigned int tex2 = loadTexture(ASSET_DIR "/pattern.png");
    unsigned int logo = loadTexture(ASSET_DIR "/logo.png");
    if (tex1 == 0 || tex2 == 0 || logo == 0) {
        glDeleteTextures(1, &tex1);
        glDeleteTextures(1, &tex2);
        glDeleteTextures(1, &logo);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteProgram(shader);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    std::cout << "tex1 = " << tex1 << " tex2 = " << tex2 << " logo = " << logo << "\n";
    glUseProgram(shader);
    int locTexture1 = glGetUniformLocation(shader, "texture1");
    int locTexture2 = glGetUniformLocation(shader, "texture2");
    int blendLoc = glGetUniformLocation(shader, "blend");
    int modeLoc = glGetUniformLocation(shader, "blendMode");
    glUniform1i(locTexture1, 0);     // unit нөмірлері, текстура объектілерінің ID-і емес
    glUniform1i(locTexture2, 1);

    int locNormal = glGetUniformLocation(shader, "normalMatrix");
    int locLightPos = glGetUniformLocation(shader, "lightPos");
    int locLightColor = glGetUniformLocation(shader, "lightColor");
    int locAmbient = glGetUniformLocation(shader, "ambientStrength");
    int locClamp = glGetUniformLocation(shader, "clampDiffuse");
    int locGouraud = glGetUniformLocation(shader, "useGouraud");
    unsigned int lampShader = makeShader(lampVertexSrc, lampFragmentSrc);
    int lampLocModel = glGetUniformLocation(lampShader, "uModel");
    int lampLocView = glGetUniformLocation(lampShader, "uView");
    int lampLocProj = glGetUniformLocation(lampShader, "uProjection");

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
            char title[320];
            std::snprintf(title, sizeof(title),
                          "Week 13 | %d/%d %s | ambient=%.2f | %s blend=%.2f%s%s",
                          lightingMode + 1, LIGHTING_MODE_COUNT, LIGHTING_MODE_NAMES[lightingMode],
                          ambientStrength, BLEND_MODE_NAMES[blendMode], blend,
                          brokenTextureUnits ? " | BROKEN UNITS (F1)" : "",
                          wrongSamplerId ? " | WRONG ID (F2)" : "");
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
        bindSceneTextures(tex1, tex2, logo, locTexture2);
        glUniform1f(blendLoc, blend);
        glUniform1i(modeLoc, blendMode);
        const glm::vec3 lightPos = sceneLightPosition(glfwGetTime());
        glUniform3fv(locLightPos, 1, glm::value_ptr(lightPos));
        glUniform3fv(locLightColor, 1, glm::value_ptr(lightColor));
        glUniform1f(locAmbient, ambientStrength);
        glUniform1i(locClamp, lightingMode != NO_MAX_LIGHTING);
        glUniform1i(locGouraud, lightingMode == GOURAUD_LIGHTING);

        // --- Матрицалар: цикл ІШІНДЕ құрылады ---
        float aspect = (gHeight > 0) ? (float)gWidth / (float)gHeight : 1.0f;

        // PROJECTION: Q пернесін басып тұрсаң — ортографиялық (7-апта, 2-тапсырма)
        glm::mat4 projection;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, NEAR_PLANE, FAR_PLANE);
        } else {
            projection = glm::perspective(glm::radians(fov), aspect, NEAR_PLANE, FAR_PLANE);
        }

        // === 9-АПТА: мақсат нүктесі камерамен бірге жылжиды ===
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        // projection мен view — бір рет, бес куб үшін ортақ.
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(vao);

        if (ZFIGHT_DEMO) {
            // === 2-ТАПСЫРМА: екі куб бетпе-бет, z айырмасы 0.001 ===
            // A кубының алдыңғы беті z = 0.5.
            // B кубы 180° бұрылған: оның артқы беті z = 0.501 -> A-мен дерлік беттеседі.
            // 11-аптадан бастап бұрынғы жасыл/қызыл түстің орнына текстура бар.
            glm::mat4 modelA = glm::mat4(1.0f);
            const glm::mat3 normalA = glm::transpose(glm::inverse(glm::mat3(modelA)));
            glUniformMatrix3fv(locNormal, 1, GL_FALSE, glm::value_ptr(normalA));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelA));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glm::mat4 modelB = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.001f));
            modelB = glm::rotate(modelB, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat3 normalB = glm::transpose(glm::inverse(glm::mat3(modelB)));
            glUniformMatrix3fv(locNormal, 1, GL_FALSE, glm::value_ptr(normalB));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelB));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        } else {
            // === 8-АПТА: бес куб. model — цикл ІШІНДЕ жарияланады,
            // әйтпесе түрлендірулер үстемеленіп, кубтар ұшып кетеді.
            for (int i = 0; i < 5; i++) {
                const glm::mat4 model = sceneModel(i, angle);
                const glm::mat3 normalMatrix = sceneNormalMatrix(model, i);
                glUniformMatrix3fv(locNormal, 1, GL_FALSE, glm::value_ptr(normalMatrix));
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // У Гуро/Фонга лампа скрыта, чтобы не закрывать центр сравниваемой грани.
        if (lightingMode != GOURAUD_LIGHTING && lightingMode != PHONG_LIGHTING) {
            glUseProgram(lampShader);
            glm::mat4 lampModel = glm::translate(glm::mat4(1.0f), lightPos);
            lampModel = glm::scale(lampModel, glm::vec3(0.1f));
            glUniformMatrix4fv(lampLocModel, 1, GL_FALSE, glm::value_ptr(lampModel));
            glUniformMatrix4fv(lampLocView, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(lampLocProj, 1, GL_FALSE, glm::value_ptr(projection));
            glDrawArrays(GL_TRIANGLES, 0, 36);
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
    glDeleteProgram(lampShader);
    glDeleteTextures(1, &tex1);
    glDeleteTextures(1, &tex2);
    glDeleteTextures(1, &logo);

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

// =====================================================================
//  11-АПТА (текстура, UV, stb_image, mipmap) — орындалды
//
//  1. STB_IMAGE_IMPLEMENTATION тек осы .cpp ішінде анықталған.
//  2. 36 вершина сақталған: бұрын xyz+rgb (6 float), енді xyz+uv (5 float).
//     Атрибут 0: 3 компонент, атрибут 1: 2 компонент, offset = 3 float.
//     Әр жақта UV 0..1 аралығында; сыртқы CCW орау реті сақталған.
//  3. loadTexture(ASSET_DIR "/container.png") суретті бір рет жүктейді.
//     Тест суреті: 512x512 RGB. Өз PNG суретіңмен ауыстыруға болады;
//     өлшем мен арна саны файлдан алынады және консольге жазылады.
//  4. WRAP_S/T = GL_REPEAT; MIN = GL_LINEAR_MIPMAP_LINEAR; MAG = GL_LINEAR.
//     MAG үшін mipmap сүзгісін қолдануға болмайды.
//  5. glTexImage2D CPU дерегін GPU-ға көшіреді, содан кейін ғана
//     glGenerateMipmap шақырылады. CPU жады stbi_image_free арқылы босайды.
//  6. Вершина шейдері UV-ді өткізеді; фрагмент шейдері
//     texture(ourTexture, vTexCoord) арқылы интерполяцияланған UV-ден оқиды.
//  7. ourTexture = 0 — текстура объектісінің id-і емес, текстуралық бірлік.
//     Render циклінде GL_TEXTURE0 және glBindTexture анық көрсетілген.
//  8. RGB жолдары үшін GL_UNPACK_ALIGNMENT = 1, бұрынғы мәні қайтарылады.
//     Жүктеу қатесі толық жолымен шығады, бағдарлама ресурстарын босатады.
//  9. Соңында GPU текстурасы glDeleteTextures арқылы жойылады.
//     Камера, MVP, тереңдік тесті, culling және бес кубтың анимациясы сақталды.
// =====================================================================

// =====================================================================
//  12-АПТА (екі текстура, texture units, араластыру) — орындалды
//
//  Базалық: texture1 = 0, texture2 = 1, glUniform1i glUseProgram-нан кейін.
//   Әр кадрда glActiveTexture -> glBindTexture екі unit үшін де орындалады.
//   Up/Down blend-ті 0.5 * dt-мен өзгертеді, шегі 0..1. Uniform әр кадрда.
//   Анимация жылдамдығы Page Up/Page Down пернелеріне көшірілді.
//  1-тапсырма: F1 — unit 0-ге екі bind, unit 1 бос; қайта F1 — түзету.
//   blend = 0: pattern, blend = 1: бос sampler-ден қара түс (GPU-да тексерілді).
//  2-тапсырма: F2 — texture2 sampler-іне unit орнына ID беру; қайта F2 — түзету.
//   ID-лер және нақты unit байланыстары консольге шығады; ID != unit.
//  3-тапсырма: 4/5/6/7 — mix, multiply, add, clamped add.
//   RGBA8 framebuffer add нәтижесін де шектейді, сондықтан 6/7 бірдей көрінуі мүмкін.
//  4-тапсырма: 8 — unit 1-ге RGBA logo, mix(base, decal, decal.a).
//  5-тапсырма: 9 — vTexCoord.y, 0 — smoothstep(0.3, 0.7, vTexCoord.y).
//  F3 — дұрыс байланыстар, mix және blend = 0.5; P — толық күй.
//  docs/week12/README.md: қорғауға жауаптар, GPU тексеру журналы және 12 кадр.
// =====================================================================

// =====================================================================
//  13-АПТА — нормальдар, Lambert, ambient + diffuse, normal matrix
//  36 вершина: xyz + UV + normal, stride = 8; атрибут 2: offset = 5 float.
//  FragPos және жарық әлем координаттарында; нормаль әр фрагментте normalize.
//  Әр куб үшін normalMatrix = transpose(inverse(mat3(model))) бөлек беріледі.
//  J/K: basic, orbit, no max, wrong normal, right normal, ambient, Gouraud, Phong.
//  O/L: ambient 0..0.5, 0.3 * dt; Q: бұрынғы ортография; H: жарықты қалпына келтіру.
//  Қате max/normal тек арнайы экспериментте; бастапқы режимде екеуі де дұрыс.
//  Қосымша: жарық орнында шағын ақ куб. V орбита режимінде кубтарды айналдырады.
//  docs/week13/README.md — жауаптар, есептеулер, тексеру және GPU кадрлары.
// =====================================================================
