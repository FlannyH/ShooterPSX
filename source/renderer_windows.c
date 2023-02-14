#include <stdio.h>
#include <string.h>
#include <time.h>

#include "renderer.h"
#include <glfw/glfw3.h>
#include <GL/gl3w.h>
#include <cglm/cam.h>
#include <cglm/affine.h>

#include "file.h"
#include "texture.h"
#define PI 3.14159265358979f

#define RESOLUTION_SCALING 4
GLFWwindow* window;
mat4 perspective_matrix;
mat4 view_matrix;
GLuint shader;
GLuint vao;
GLuint vbo;
clock_t dt_clock;
GLuint textures[256];
float tex_res[512];

typedef enum _shader_type{
    vertex,
    pixel,
    geometry,
    compute
} ShaderType;

static void DebugCallbackFunc(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const GLvoid* userParam)
{
    // Skip some less useful info
    if (id == 131218)	// http://stackoverflow.com/questions/12004396/opengl-debug-context-performance-warning
        return;
    
    char* sourceString;
    char* typeString;
    char* severityString;

    // The AMD variant of this extension provides a less detailed classification of the error,
    // which is why some arguments might be "Unknown".
    switch (source) {
    case GL_DEBUG_SOURCE_API: {
        sourceString = "API";
        break;
    }
    case GL_DEBUG_SOURCE_APPLICATION: {
        sourceString = "Application";
        break;
    }
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: {
        sourceString = "Window System";
        break;
    }
    case GL_DEBUG_SOURCE_SHADER_COMPILER: {
        sourceString = "Shader Compiler";
        break;
    }
    case GL_DEBUG_SOURCE_THIRD_PARTY: {
        sourceString = "Third Party";
        break;
    }
    case GL_DEBUG_SOURCE_OTHER: {
        sourceString = "Other";
        break;
    }
    default: {
        sourceString = "Unknown";
        break;
    }
    }

    switch (type) {
    case GL_DEBUG_TYPE_ERROR: {
        typeString = "Error";
        break;
    }
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {
        typeString = "Deprecated Behavior";
        break;
    }
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: {
        typeString = "Undefined Behavior";
        break;
    }
    case GL_DEBUG_TYPE_PORTABILITY_ARB: {
        typeString = "Portability";
        break;
    }
    case GL_DEBUG_TYPE_PERFORMANCE: {
        typeString = "Performance";
        break;
    }
    case GL_DEBUG_TYPE_OTHER: {
        typeString = "Other";
        break;
    }
    default: {
        typeString = "Unknown";
        break;
    }
    }

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: {
        severityString = "High";
        break;
    }
    case GL_DEBUG_SEVERITY_MEDIUM: {
        severityString = "Medium";
        break;
    }
    case GL_DEBUG_SEVERITY_LOW: {
        severityString = "Low";
        break;
    }
    default: {
        severityString = "Unknown";
        return;
    }
    }

    printf("GL Debug Callback:\n source: %i:%s \n type: %i:%s \n id: %i \n severity: &i:%s \n  message: &s", source, sourceString, type, typeString, id, severity, severityString, message);
}


bool load_shader_part(char* path, ShaderType type, GLuint* program)
{
    const int shader_types[4] = 
    {
        GL_VERTEX_SHADER,
        GL_FRAGMENT_SHADER,
        GL_GEOMETRY_SHADER,
        GL_COMPUTE_SHADER,
    };

    //Read shader source file
    size_t shader_size = 0;
    char* shader_data = NULL;

    if (file_read(path, &shader_data, &shader_size) == 0)
    {
        // Log error
        printf("[ERROR] Shader %s not found!\n", path);
        
        // Clean up
        free(shader_data);
        return false;
    }

    //Create shader on GPU
    const GLuint type_to_create = shader_types[(int)type];
    const GLuint shader = glCreateShader(type_to_create);

    //Compile shader source
    glShaderSource(shader, 1, &shader_data, &shader_size);
    glCompileShader(shader);

    //Error checking
    GLint result = GL_FALSE;
    int log_length;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    char* frag_shader_error = malloc(log_length);
    glGetShaderInfoLog(shader, log_length, NULL, &frag_shader_error[0]);
    if (log_length > 0)
    {
        // Log error
        printf("[ERROR] File '%s':\n\n%s\n", path, &frag_shader_error[0]);

        // Clean up
        free(shader_data);
        return false;
    }

    //Attach to program
    glAttachShader(*program, shader);

    //Clean up
    free(shader_data);

    return true;
}

GLuint shader_from_file(char* vert_path, char* frag_path) {
    // Create program
    const GLuint shader_gpu = glCreateProgram();

    // Load shader parts
    const bool vert_loaded = load_shader_part(vert_path, vertex, &shader_gpu);
    const bool frag_loaded = load_shader_part(frag_path, pixel, &shader_gpu);

    // Make sure it worked
    if (vert_loaded == false)
        printf("[ERROR] Failed to load shader '%s'! Either the shader files do not exist, or a compilation error occurred.\n", vert_path);
    if (frag_loaded == false)
        printf("[ERROR] Failed to load shader '%s'! Either the shader files do not exist, or a compilation error occurred.\n", frag_path);

    // Link
    glLinkProgram(shader_gpu);

    return shader_gpu;
}

void renderer_init() {
    // Create OpenGL window
    glfwInit();
    window = glfwCreateWindow(320 * RESOLUTION_SCALING, 240 * RESOLUTION_SCALING, "ShooterPSX", NULL, NULL);
    if (window == NULL) {
        printf("[ERROR] Could not open OpenGL window! Aborting.");
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);
    gl3wInit();
    glDebugMessageCallback(DebugCallbackFunc, NULL);
    glfwSwapInterval(1);

    // Set viewport
    glViewport(0, 0, 320 * RESOLUTION_SCALING, 240 * RESOLUTION_SCALING);

    // Set perspective matrix
    glm_perspective(glm_rad(90.0f), 4.0f / 3.0f, 0.1f, 10000.f, perspective_matrix);

    // Load shaders
    shader = shader_from_file("\\ASSETS\\GOURAUD.VSH;1", "\\ASSETS\\GOURAUD.FSH;1");

    // Set up VAO and VBO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, sizeof(Vertex3D), offsetof(Vertex3D, x));
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex3D), offsetof(Vertex3D, r));
    glVertexAttribPointer(2, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex3D), offsetof(Vertex3D, u));
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Initialize delta time clock
    dt_clock = clock();

    // Zero init textures
    memset(textures, 0, sizeof(textures));
}

void renderer_begin_frame(Transform* camera_transform) {
    // Convert from PS1 to GLM
    vec3 position = {
        (float)(camera_transform->position.vx >> 12),
        (float)(camera_transform->position.vy >> 12),
        (float)(camera_transform->position.vz >> 12)
    };
    vec3 rotation = {
        (float)camera_transform->rotation.vx * (2*PI / 131072.0f),
        -(float)camera_transform->rotation.vy * (2*PI / 131072.0f) + PI,
        (float)camera_transform->rotation.vz * (2*PI / 131072.0f) + PI
    };

    // Set view matrix
    glm_mat4_identity(view_matrix);

    // Apply rotation
    glm_rotate_x(view_matrix, rotation[0], view_matrix);
    glm_rotate_y(view_matrix, rotation[1], view_matrix);
    glm_rotate_z(view_matrix, rotation[2], view_matrix);

    // Apply translation
    glm_translate(view_matrix, position);

    // Clear screen
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer_end_frame() {
    // Flip buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void renderer_draw_model_shaded(Model* model, Transform* model_transform) {
    for (size_t i = 0; i < model->n_meshes; ++i) {
        renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
    }
}

void renderer_draw_mesh_shaded(Mesh* mesh, Transform* model_transform) {
    // Calculate model matrix
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);

    // todo: add scale

    // Apply rotation
    // Apply translation
    vec3 position = {
        (float)model_transform->position.vx,
        (float)model_transform->position.vy,
        (float)model_transform->position.vz,
    };
    glm_translate(model_matrix, position);
    glm_rotate_x(model_matrix, (float)model_transform->rotation.vx * 2 * PI / 131072.0f, model_matrix);
    glm_rotate_y(model_matrix, (float)model_transform->rotation.vy * 2 * PI / 131072.0f, model_matrix);
    glm_rotate_z(model_matrix, (float)model_transform->rotation.vz * 2 * PI / 131072.0f, model_matrix);

    // Bind shader
    glUseProgram(shader);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, textures[mesh->vertices[0].tex_id]);

    // Bind vertex buffers
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(shader, "proj_matrix"), 1, GL_FALSE, &perspective_matrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view_matrix"), 1, GL_FALSE, &view_matrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model_matrix"), 1, GL_FALSE, &model_matrix[0][0]);

    // Copy data into it
    glBufferData(GL_ARRAY_BUFFER, mesh->n_vertices * sizeof(Vertex3D), mesh->vertices, GL_STATIC_DRAW);

    // Enable depth and draw
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDrawArrays(GL_TRIANGLES, 0, mesh->n_vertices);
}

void renderer_draw_triangles_shaded_2d(Vertex2D* vertex_buffer, uint16_t n_verts, int16_t x, int16_t y) {

}

void renderer_upload_texture(const TextureCPU* texture, const uint8_t index) {
    // This is where all the pixels will be stored
    Pixel32* pixels = malloc((size_t)texture->width * (size_t)texture->height * 4);
    
    // The texture is stored in 4bpp format, so each byte in the texture is 2 pixels horizontally - Convert to 32-bit color
    for (size_t i = 0; i < (texture->width * texture->height / 2); ++i) {
        // Get indices from texture
        const uint8_t color_index_left = (texture->data[i] >> 4) & 0x0F;
        const uint8_t color_index_right = (texture->data[i] >> 0) & 0x0F;

        // Get 16-bit color values from palette
        const Pixel16 pixel_left = texture->palette[color_index_left];
        const Pixel16 pixel_right = texture->palette[color_index_right];
        
        // Expand to 32-bit color
        pixels[i * 2 + 0].r = pixel_left.r << 3;
        pixels[i * 2 + 0].g = pixel_left.g << 3;
        pixels[i * 2 + 0].b = pixel_left.b << 3;
        pixels[i * 2 + 0].a = pixel_left.a << 7;
        
        // And on the right side as well
        pixels[i * 2 + 1].r = pixel_right.r << 3;
        pixels[i * 2 + 1].g = pixel_right.g << 3;
        pixels[i * 2 + 1].b = pixel_right.b << 3;
        pixels[i * 2 + 1].a = pixel_right.a << 7;
    }

    // Generate texture if necessary
    if (textures[index] == 0)
        glGenTextures(1, &textures[index]);

    // Upload texture
    glBindTexture(GL_TEXTURE_2D, textures[index]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Store texture resolution
    tex_res[(size_t)index * 2 + 0] = (float)texture->width;
    tex_res[(size_t)index * 2 + 1] = (float)texture->height;

    // Clean up after we're done
    free(pixels);
}

int renderer_get_delta_time_raw() {
    return 0;
}

int renderer_get_delta_time_ms() {
    int new_dt = clock();
    int dt = (new_dt - dt_clock) * 1000 / CLOCKS_PER_SEC;
    dt_clock = new_dt;
    return dt;
}

int renderer_get_n_rendered_triangles() {
    return 0;
}

int renderer_get_n_total_triangles() {
    return 0;
}