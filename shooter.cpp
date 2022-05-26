// Include standard headers
#include <stdio.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// C++ libraries
#include <vector>
#include <iostream>
#include <random>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "common/shader.hpp"
#include "common/texture.hpp"
#include "common/objloader.hpp"

const float PI = 3.1416;

void createSphere(float radius, int sectorCount, int stackCount,
                  std::vector<glm::vec3>& out_vertices,
                  std::vector<glm::vec3>& out_normals,
                  std::vector<glm::vec2>& out_uvs,
                  glm::vec3 position = glm::vec3(0, 0, 0)) {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;

    glm::mat4 translation_mat = glm::mat4(1.0f);
    translation_mat = glm::translate(translation_mat, position);

    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
    float s, t;                                     // vertex uv

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i) {
        stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2 (vertical angle)
        xy = radius * cosf(stackAngle);          // r * cos(u)
        z = radius * sinf(stackAngle);           // r * sin(u)

        // the first and last vertices have same position and normal, but different tex coords
        for (int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi (horizontal angle)

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);          // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);          // r * cos(u) * sin(v)
            glm::vec4 vertex = translation_mat * glm::vec4(x, y, z, 1.0f);
            vertices.emplace_back(vertex);

            // normalized vertex normal (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            glm::vec4 normal = translation_mat * glm::vec4(nx, ny, nz, 0.0f);
            normals.emplace_back(normal);

            // vertex uv coord (s, t) range between [0, 1]
            s = (float) j / sectorCount;
            t = (float) i / stackCount;
            uvs.emplace_back(s, t);
        }
    }

    for (int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1);     // beginning of current stack
        int k2 = k1 + sectorCount + 1;      // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if (i != 0) {
                out_vertices.push_back(vertices[k1]);
                out_vertices.push_back(vertices[k2]);
                out_vertices.push_back(vertices[k1 + 1]);

                out_normals.push_back(normals[k1]);
                out_normals.push_back(normals[k2]);
                out_normals.push_back(normals[k1 + 1]);

                out_uvs.push_back(uvs[k1]);
                out_uvs.push_back(uvs[k2]);
                out_uvs.push_back(uvs[k1 + 1]);
            }

            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1)) {
                out_vertices.push_back(vertices[k1 + 1]);
                out_vertices.push_back(vertices[k2]);
                out_vertices.push_back(vertices[k2 + 1]);

                out_normals.push_back(normals[k1 + 1]);
                out_normals.push_back(normals[k2]);
                out_normals.push_back(normals[k2 + 1]);

                out_uvs.push_back(uvs[k1 + 1]);
                out_uvs.push_back(uvs[k2]);
                out_uvs.push_back(uvs[k2 + 1]);
            }
        }
    }
}

class Camera {
public:
    static constexpr double PI = 3.1416;

    explicit Camera(GLfloat horizontal_angle = 0.0f,
                    GLfloat vertical_angle = 0.0f,
                    GLfloat fov = 45.0f):
            horizontal_angle_(horizontal_angle),
            vertical_angle_(vertical_angle),
            fov_(fov) {}

    virtual ~Camera() = default;

    virtual glm::vec3 CameraDirection() {
        return {
                cos(vertical_angle_) * sin(horizontal_angle_),
                sin(vertical_angle_),
                cos(vertical_angle_) * cos(horizontal_angle_)
        };
    }

    virtual glm::vec3 CameraRight() {
        return {
                sin(horizontal_angle_ - Camera::PI / 2),
                0,
                cos(horizontal_angle_ - Camera::PI / 2)
        };
    }

    virtual glm::vec3 CameraUp() {
        return glm::cross(CameraRight(), CameraDirection());
    }

    virtual GLfloat FOV() {
        return fov_;
    }

protected:
    GLfloat horizontal_angle_;
    GLfloat vertical_angle_;
    GLfloat fov_;
};

class Model {
public:
    explicit Model(const std::string& texture_file,
                   const std::vector<glm::vec3>& vertices,
                   const std::vector<glm::vec2>& uvs) {
        texture_ = loadBMP_custom(texture_file.data());
        vertices_ = vertices;
        uvs_ = uvs;
    }

    explicit Model(const std::string& obj_file,
                   const std::string& texture_file) {
        texture_ = loadBMP_custom(texture_file.data());
        std::vector<glm::vec3> temp_normals;
        loadOBJ(obj_file.data(), vertices_, uvs_, temp_normals);
    }

    virtual ~Model() {
        glDeleteTextures(1, &texture_);
    }

    void Draw(GLuint texture_id, GLuint vertexbuffer, GLuint uvbuffer) {
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(glm::vec3), &vertices_[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, uvs_.size() * sizeof(glm::vec3), &uvs_[0], GL_STATIC_DRAW);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glUniform1i(texture_id, 0);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glDrawArrays(GL_TRIANGLES, 0, vertices_.size());

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }

protected:
    GLuint texture_;
    std::vector<glm::vec3> vertices_;
    std::vector<glm::vec2> uvs_;
};

class SceneObject : public Model {
public:
    explicit SceneObject(const glm::vec3& position,
                         const glm::vec3& direction,
                         GLfloat speed,
                         GLfloat collider_radius,
                         const std::string& texture_file,
                         const std::vector<glm::vec3>& vertices,
                         const std::vector<glm::vec2>& uvs):
            position_(position),
            direction_(direction),
            speed_(speed),
            collider_radius_(collider_radius),
            Model(texture_file, vertices, uvs) {}

    explicit SceneObject(const glm::vec3& position,
                         const glm::vec3& direction,
                         GLfloat speed,
                         GLfloat collider_radius,
                         const std::string& obj_file,
                         const std::string& texture_file):
            position_(position),
            direction_(direction),
            speed_(speed),
            collider_radius_(collider_radius),
            Model(obj_file, texture_file) {}

    bool IsIntersected(SceneObject* other) {
        return glm::length(position_ - other->GetPosition()) < (collider_radius_ +
                                                                other->GetColliderRadius());
    }

    glm::vec3 GetPosition() const {
        return position_;
    }

    GLfloat GetColliderRadius() const {
        return collider_radius_;
    }

    glm::vec3 GetDirection() const {
        return direction_;
    };

    float GetSpeed() const {
        return speed_;
    }

    virtual bool IsSnowBall() const {
        return false;
    }

    void Shift(const glm::vec3& step) {
        position_ += step;
    }

protected:
    glm::vec3 position_;
    glm::vec3 direction_;
    GLfloat speed_;
    GLfloat collider_radius_;
};

class CubeEnemy : public SceneObject {
public:
    explicit CubeEnemy(const glm::vec3& position,
                       glm::vec3 rotation = glm::vec3(1, 0, 0),
                       float angle = 0.0,
                       float scale_coef = 1.0f):
            SceneObject(position,
                        glm::vec3(0.0f),
                        0.0,
                        2,
                        "cube.obj",
                        "enemy_texture.bmp") {
        glm::mat4 transform_mat = glm::mat4(1.0f);
        glm::vec3 scale_vec = glm::vec3(scale_coef);

        transform_mat = glm::scale(transform_mat, scale_vec);
        transform_mat = glm::rotate(transform_mat, angle, rotation);
        // transform_mat = glm::translate(transform_mat, position);

        for (auto& vertex: vertices_) {
            vertex = glm::vec3(transform_mat * glm::vec4(vertex, 1.0f));
        }

        collider_radius_ *= scale_coef;
    }
};

class SnowBall : public SceneObject {
public:
    explicit SnowBall(const glm::vec3& position,
                      const glm::vec3& direction,
                      GLfloat exclusion_radius = 0.75f,
                      int sectorCount = 15,
                      int stackCount = 15,
                      GLfloat speed = 13.0f):
            SceneObject(position, direction, speed, exclusion_radius, "ice_texture.bmp", {}, {}) {
        std::vector<glm::vec3> temp_normals;
        createSphere(exclusion_radius, sectorCount, stackCount, vertices_, temp_normals, uvs_,
                     position);
    }

    bool IsSnowBall() const override {
        return true;
    }
};

class Player : public Camera {
public:
    explicit Player(const glm::vec3& position = glm::vec3(0.0f),
                    GLfloat collider_radius = 1.0f,
                    GLfloat mouse_speed = 0.005f,
                    GLfloat timedelay = 0.2f):
            position_(position),
            collider_radius_(collider_radius),
            mouse_speed_(mouse_speed),
            timedelay_(timedelay) {}

    glm::vec3 GetPosition() const {
        return position_;
    }

    GLfloat GetColliderRadius() const {
        return collider_radius_;
    }

    SnowBall* CreateSnowBall() {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        glfwSetCursorPos(window, 1024 / 2, 768 / 2);

        horizontal_angle_ += mouse_speed_ * GLfloat(1024 / 2 - xpos);
        vertical_angle_ += mouse_speed_ * GLfloat(768 / 2 - ypos);

        glm::vec3 camera_direction = CameraDirection();

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (glfwGetTime() > next_creation_time_) {
                next_creation_time_ = glfwGetTime() + timedelay_;
                auto* snowball = new SnowBall(position_ + camera_direction * 1.5f,
                                              camera_direction);
                return snowball;
            }
        }

        return nullptr;
    }

protected:
    glm::vec3 position_;
    GLfloat collider_radius_;
    GLfloat mouse_speed_;
    GLfloat timedelay_;
    GLfloat next_creation_time_ = glfwGetTime();
};

class EnemyCreator {
public:
    explicit EnemyCreator(GLfloat timedelay = 3.0f,
                          GLfloat min_radius = 5.0f,
                          GLfloat max_radius = 50.0f,
                          GLfloat min_size = 0.5f,
                          GLfloat max_size = 4.0f):
            timedelay_(timedelay),
            rng_(std::random_device()()),
            angle_(0, 2*PI),
            radius_(min_radius, max_radius),
            size_(min_size, max_size) {}

    SceneObject* CreateEnemy(const glm::vec3& position) {
        if (glfwGetTime() <= next_creation_time_) {
            return nullptr;
        }

        next_creation_time_ = glfwGetTime() + timedelay_;

        GLfloat angle_rotation = angle_(rng_);
        GLfloat phi = angle_(rng_);
        GLfloat theta = angle_(rng_);

        glm::vec3 rotation_axis(
                cos(phi) * sin(theta),
                sin(phi),
                cos(phi) * cos(theta)
        );

        GLfloat angle_position = angle_(rng_);
        glm::vec3 direction(
                sin(angle_position),
                0.0f,
                cos(angle_position)
        );

        GLfloat radius = radius_(rng_);
        glm::vec3 new_position = position + radius * direction;

        GLfloat size = size_(rng_);
        SceneObject* new_obj = new CubeEnemy(new_position,
                                             rotation_axis,
                                             angle_rotation,
                                             size);

        return new_obj;
    }

private:
    GLfloat timedelay_;
    GLfloat next_creation_time_ = glfwGetTime();
    std::mt19937 rng_;
    std::uniform_real_distribution<> angle_;
    std::uniform_real_distribution<> radius_;
    std::uniform_real_distribution<> size_;
};

int main() {
    // Initialise GLFW
    if (!glfwInit()) {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "Shooter", NULL, NULL);
    if (window == NULL) {
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Dark black background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("SimpleVertexShader.vertexshader",
                                   "SimpleFragmentShader.fragmentshader");

    GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
    GLuint ProjectionID = glGetUniformLocation(programID, "Projection");
    GLuint ViewID = glGetUniformLocation(programID, "View");
    GLuint ModelID = glGetUniformLocation(programID, "Model");

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);

    std::vector<SceneObject*> objects;
    auto player = new Player();

    GLfloat prev_time = glfwGetTime();

    EnemyCreator enemy_creator;

    do {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLfloat current_time = glfwGetTime();
        GLfloat timediff = current_time - prev_time;

        for (SceneObject* obj : objects) {
            obj->Shift(obj->GetDirection() * obj->GetSpeed() * timediff);
        }

        prev_time = current_time;

        std::vector<bool> remains(objects.size(), true);

        for (size_t i = 0; i < objects.size(); ++i) {
            if (remains[i] == false and objects[i]->IsSnowBall() == false) {
                continue;
            }

            for (size_t j = i + 1; j < objects.size(); ++j) {
                if (objects[i]->IsIntersected(objects[j])) {
                    if (objects[i]->IsSnowBall() xor objects[j]->IsSnowBall()) {
                        remains[i] = remains[j] = false;
                    }
                }
            }
        }

        std::vector<SceneObject*> alive_objects;

        for (size_t i = 0; i < objects.size(); ++i) {
            if (remains[i]) {
                alive_objects.push_back(objects[i]);
            } else {
                delete objects[i];
            }
        }

        objects = alive_objects;

        SnowBall* new_snowball = player->CreateSnowBall();
        if (new_snowball != nullptr) {
            objects.push_back(new_snowball);
        }

        SceneObject* new_enemy = enemy_creator.CreateEnemy(player->GetPosition());
        if (new_enemy != nullptr) {
            objects.push_back(new_enemy);
        }

        glm::mat4 Projection = glm::perspective(glm::radians(player->FOV()),
                                                4.0f / 3.0f,
                                                player->GetColliderRadius(),
                                                300.0f);
        glm::mat4 View = glm::lookAt(
                player->GetPosition(),
                player->GetPosition() + player->CameraDirection(),
                player->CameraUp()
        );

        glUseProgram(programID);

        glUniformMatrix4fv(ProjectionID, 1, GL_FALSE, &Projection[0][0]);
        glUniformMatrix4fv(ViewID, 1, GL_FALSE, &View[0][0]);

        for (SceneObject* obj : objects) {
            glm::mat4 Translate = glm::translate(glm::mat4(), obj->GetPosition());

            glm::mat4 Model = Translate;
            glUniformMatrix4fv(ModelID, 1, GL_FALSE, &Model[0][0]);

            obj->Draw(TextureID, vertexbuffer, uvbuffer);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(window) == 0);

    for (SceneObject* obj : objects) {
        delete obj;
    }

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VertexArrayID);

    glfwTerminate();

    return 0;
}
