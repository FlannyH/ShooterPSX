#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "memory.h"
#include "renderer.h"
#include "win/debug_layer.h"
#include <GL/gl3w.h>
#include <cglm/affine.h>
#include <cglm/cam.h>
#include <GLFW/glfw3.h>


#include "file.h"
#include "texture.h"
#include "GL/gl3w.h"
#include "GL/glcorearb.h"
#include <cglm/types.h>
#include <cglm/vec3.h>
#include <cglm/affine.h>

#include "input.h"
#include "vec3.h"
#include "lut.h"
#define PI 3.14159265358979f

int n_sections;
int sections[N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE];

#define RESOLUTION_SCALING 4
GLFWwindow *window;
mat4 perspective_matrix;
mat4 view_matrix;
mat4 view_matrix_topdown;
mat4 view_matrix_third_person;
mat4 view_matrix_normal;
GLuint shader;
GLuint vao;
GLuint vbo;
clock_t dt_clock;
//GLuint textures[256];
GLuint textures;
float tex_res[512];
clock_t dt = 0;
uint32_t n_total_triangles = 0;
int w, h;
int prev_w = 0;
int prev_h = 0;
transform_t* cam_transform;
vec3_t camera_pos;
vec3_t camera_dir;
int tex_id_start;

typedef enum { vertex, pixel, geometry, compute } ShaderType;

static void DebugCallbackFunc(GLenum source, GLenum type, GLuint id,
															GLenum severity, GLsizei length,
															const GLchar *message, const GLvoid *userParam) {
	// Skip some less useful info
	if (id ==
			131218) // http://stackoverflow.com/questions/12004396/opengl-debug-context-performance-warning
		return;

	char *sourceString;
	char *typeString;
	char *severityString;

	// The AMD variant of this extension provides a less detailed classification
	// of the error, which is why some arguments might be "Unknown".
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

	printf("GL Debug Callback:\n source: %i:%s \n type: %i:%s \n id: %i \n "
				 "severity: %i:%s \n	message: %s",
				 source, sourceString, type, typeString, id, severity, severityString,
				 message);
}

void update_delta_time_ms(void) {
	clock_t new_dt;
	do {
		new_dt = clock();
		dt = ((new_dt - dt_clock) * 1000) / CLOCKS_PER_SEC;
	} while  (dt < 4);
	dt_clock = new_dt; 
}

bool load_shader_part(char *path, const ShaderType type, const GLuint *program) {
	const int shader_types[4] = {
			GL_VERTEX_SHADER,
			GL_FRAGMENT_SHADER,
			GL_GEOMETRY_SHADER,
			GL_COMPUTE_SHADER,
	};

	// Read shader source file
	size_t shader_size = 0;
	uint32_t *shader_data = NULL;

	if (file_read(path, &shader_data, &shader_size, 1, STACK_TEMP) == 0) {
		// Log error
		printf("[ERROR] Shader %s not found!\n", path);

		// Clean up
		mem_free(shader_data);
		return false;
	}

	// Create shader on GPU
	const GLuint type_to_create = shader_types[(int)type];
	const GLuint shader = glCreateShader(type_to_create);

	// Compile shader source
	const GLint shader_size_gl = (GLint)shader_size;
	glShaderSource(shader, 1, (const GLchar *const *)&shader_data, &shader_size_gl);
	glCompileShader(shader);

	// Error checking
	GLint result = GL_FALSE;
	int log_length;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
	char *frag_shader_error = malloc(log_length);
	glGetShaderInfoLog(shader, log_length, NULL, &frag_shader_error[0]);
	if (log_length > 0) {
		// Log error
		printf("[ERROR] File '%s':\n\n%s\n", path, &frag_shader_error[0]);
		return false;
	}

	// Attach to program
	glAttachShader(*program, shader);

	return true;
}

GLuint shader_from_file(char *vert_path, char *frag_path) {
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
    glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(DebugCallbackFunc, NULL);
	glfwSwapInterval(1);

	// Set viewport
	glViewport(0, 0, 320 * RESOLUTION_SCALING, 240 * RESOLUTION_SCALING);

	// Set perspective matrix
	glm_perspective(glm_rad(90.0f), 4.0f / 3.0f, 0.1f, 100000.f, perspective_matrix);

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
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, sizeof(vertex_3d_t), (const void *)offsetof(vertex_3d_t, x));
	glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_3d_t), (const void *)offsetof(vertex_3d_t, r));
	glVertexAttribPointer(2, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vertex_3d_t), (const void *)offsetof(vertex_3d_t, u));
	glVertexAttribPointer(3, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vertex_3d_t), (const void *)offsetof(vertex_3d_t, tex_id));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Initialize delta time clock
	dt_clock = clock();

	// Initialize ImGui
	debug_layer_init(window);

	// Zero init textures
	//memset(textures, 0, sizeof(textures));
    void* random_data = mem_alloc(2048 * 512 * 4, MEM_CAT_TEXTURE);
	glGenTextures(1, &textures);
	glBindTexture(GL_TEXTURE_2D, textures);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2048, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, random_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    mem_free(random_data);
}

void renderer_begin_frame(const transform_t *camera_transform) {
    cam_transform = camera_transform;
	// Set up viewport
	glfwGetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);

	if (w != prev_w || h != prev_h) {
		glm_perspective(glm_rad(90.0f), (float)w / (float)h, 0.1f, 1000000.f,
										perspective_matrix);
	}

	// Convert from PS1 to GLM
    vec3 position = {
        -(float)(camera_transform->position.x >> 12),
        -(float)(camera_transform->position.y >> 12),
        -(float)(camera_transform->position.z >> 12)
    };
	const vec3 rotation = {
			(float)camera_transform->rotation.x * (2 * PI / 131072.0f),
			-(float)camera_transform->rotation.y * (2 * PI / 131072.0f) + PI,
			(float)camera_transform->rotation.z * (2 * PI / 131072.0f) + PI
	};

	// Set view matrix
	glm_mat4_identity(view_matrix_normal);

	// Apply rotation
	glm_rotate_x(view_matrix_normal, rotation[0], view_matrix_normal);
	glm_rotate_y(view_matrix_normal, rotation[1], view_matrix_normal);
	glm_rotate_z(view_matrix_normal, rotation[2], view_matrix_normal);

    //printf("position:\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n", position[0], position[1], position[2], rotation[0], rotation[1], rotation[2]);

	// Apply translation
	glm_translate(view_matrix_normal, position);

    // Set view matrix for top down debug
    vec3 top_down_position = { position[0], 3000, position[2] };
    glm_mat4_identity(view_matrix_topdown);
    glm_rotate_x(view_matrix_topdown, PI / 2.0f, view_matrix_topdown);
    glm_rotate_z(view_matrix_topdown, PI, view_matrix_topdown);
    glm_translate(view_matrix_topdown, top_down_position);

    glm_mat4_identity(view_matrix_third_person);
    vec3 distance = { 0, 0, -400 };
    glm_translate(view_matrix_third_person, distance);

    // Apply rotation
    glm_rotate_x(view_matrix_third_person, rotation[0], view_matrix_third_person);
    glm_rotate_y(view_matrix_third_person, rotation[1], view_matrix_third_person);
    glm_rotate_z(view_matrix_third_person, rotation[2], view_matrix_third_person);

    //printf("position:\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n", position[0], position[1], position[2], rotation[0], rotation[1], rotation[2]);

    // Apply translation
    glm_translate(view_matrix_third_person, position);

	// Clear screen
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (input_held(PAD_SQUARE, 0)) {
        memcpy(view_matrix, view_matrix_topdown, sizeof(view_matrix_topdown));
    }
    else if (input_held(PAD_TRIANGLE, 0)) {
        memcpy(view_matrix, view_matrix_third_person, sizeof(view_matrix_third_person));
    }
    else
    {
        memcpy(view_matrix, view_matrix_normal, sizeof(view_matrix_normal));
    }

	camera_dir.x = -view_matrix_normal[2][0] * 4096.f;
	camera_dir.y = -view_matrix_normal[2][1] * 4096.f;
	camera_dir.z = -view_matrix_normal[2][2] * 4096.f;
	memcpy(&camera_pos, &camera_transform->position, sizeof(camera_pos));
	//vec3_debug(camera_dir);

	n_total_triangles = 0;
}

void renderer_end_frame() {
	update_delta_time_ms();
	debug_layer_begin();
#ifdef _LEVEL_EDITOR
	debug_layer_update_editor();
#else
	debug_layer_update_gameplay();
#endif
	debug_layer_end();
	// Flip buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void renderer_draw_model_shaded(const model_t* model, const transform_t* model_transform, visfield_t* vislist, int tex_id_offset) {
    glViewport(0, 0, w, h);
    if (vislist == NULL || n_sections == 0) {
        for (size_t i = 0; i < model->n_meshes; ++i) {
            //renderer_debug_draw_aabb(&model->meshes[i].bounds, red, &id_transform);
            renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
        }
    }
    else {
        // Determine which meshes to render
        visfield_t combined = { 0, 0, 0, 0 };

        // Get all the vislist bitfields and combine them together
        for (size_t i = 0; i < n_sections; ++i) {
            combined.sections_0_31 |= vislist[sections[i]].sections_0_31;
            combined.sections_32_63 |= vislist[sections[i]].sections_32_63;
            combined.sections_64_95 |= vislist[sections[i]].sections_64_95;
            combined.sections_96_127 |= vislist[sections[i]].sections_96_127;
        }

        // Render only the meshes that are visible
		printf("rendering meshes: ");
        for (size_t i = 0; i < model->n_meshes; ++i) {
            if ((i < 32) && (combined.sections_0_31 & (1 << i))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
            else if ((i >= 32) && (i < 64) && (combined.sections_32_63 & (1 << (i - 32)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
            else if ((i >= 64) && (i < 96) && (combined.sections_64_95 & (1 << (i - 64)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
            else if ((i >= 96) && (i < 128) && (combined.sections_96_127 & (1 << (i - 96)))) renderer_draw_mesh_shaded(&model->meshes[i], model_transform);
        }
		printf("\n");
    }
}

int32_t max_dot_value = 0;
void renderer_draw_mesh_shaded(const mesh_t *mesh, transform_t *model_transform) {
	// Calculate model matrix
	mat4 model_matrix;
	glm_mat4_identity(model_matrix);

	// Apply rotation
	// Apply translation
	// Apply scale
    vec3 position = {
            (float)model_transform->position.x,
            (float)model_transform->position.y,
            (float)model_transform->position.z,
    };
    vec3 scale = {
            (float)model_transform->scale.x / 4096.0f,
            (float)model_transform->scale.y / 4096.0f,
            (float)model_transform->scale.z / 4096.0f,
    };
	glm_translate(model_matrix, position);
    glm_scale(model_matrix, scale);
	glm_rotate_x(model_matrix, (float)model_transform->rotation.x * 2 * PI / 131072.0f, model_matrix);
	glm_rotate_y(model_matrix, (float)model_transform->rotation.y * 2 * PI / 131072.0f, model_matrix);
	glm_rotate_z(model_matrix, (float)model_transform->rotation.z * 2 * PI / 131072.0f, model_matrix);

	// Bind shader
	glUseProgram(shader);

	// Bind texture
	glBindTexture(GL_TEXTURE_2D, textures);

	// Bind vertex buffers
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Set matrices
	glUniformMatrix4fv(glGetUniformLocation(shader, "proj_matrix"), 1, GL_FALSE, &perspective_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader, "view_matrix"), 1, GL_FALSE, &view_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader, "model_matrix"), 1, GL_FALSE, &model_matrix[0][0]);

    glUniform1i(glGetUniformLocation(shader, "texture_bound"), mesh->vertices[0].tex_id != 255);
    glUniform1i(glGetUniformLocation(shader, "texture_offset"), tex_id_start);
	glUniform1i(glGetUniformLocation(shader, "texture_is_page"), 0);
	glUniform1f(glGetUniformLocation(shader, "alpha"), 1.0f);
    
	// Copy data into it
	glBufferData(GL_ARRAY_BUFFER, ((mesh->n_triangles * 3) + (mesh->n_quads * 4)) * sizeof(vertex_3d_t), mesh->vertices, GL_STATIC_DRAW);

	// Enable depth and draw
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glDrawArrays(GL_TRIANGLES, 0, mesh->n_triangles * 3);
    glDrawArrays(GL_QUADS, mesh->n_triangles * 3, mesh->n_quads * 4);

	n_total_triangles += mesh->n_triangles;
    tex_id_start = 0;
}

void renderer_draw_triangles_shaded_2d(const vertex_2d_t *vertex_buffer, uint16_t n_verts, int16_t x, int16_t y) {

}

void renderer_debug_draw_line(vec3_t v0, vec3_t v1, pixel32_t color, const transform_t* model_transform) {
    // Calculate model matrix
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);

    // Apply rotation
    // Apply translation
    // Apply scale
    vec3 position = {
            (float)model_transform->position.x,
            (float)model_transform->position.y,
            (float)model_transform->position.z,
    };
    vec3 scale = {
            (float)model_transform->scale.x / (float)COL_SCALE,
            (float)model_transform->scale.y / (float)COL_SCALE,
            (float)model_transform->scale.z / (float)COL_SCALE,
    };
    glm_translate(model_matrix, position);
    glm_scale(model_matrix, scale);
    glm_rotate_x(model_matrix, (float)model_transform->rotation.x * 2 * PI / 131072.0f, model_matrix);
    glm_rotate_y(model_matrix, (float)model_transform->rotation.y * 2 * PI / 131072.0f, model_matrix);
    glm_rotate_z(model_matrix, (float)model_transform->rotation.z * 2 * PI / 131072.0f, model_matrix);

    // Bind shader
    glUseProgram(shader);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, 0);
    glUniform1i(glGetUniformLocation(shader, "texture_bound"), 0);

    // Bind vertex buffers
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(shader, "proj_matrix"), 1, GL_FALSE, &perspective_matrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view_matrix"), 1, GL_FALSE, &view_matrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model_matrix"), 1, GL_FALSE, &model_matrix[0][0]);
	glUniform1i(glGetUniformLocation(shader, "texture_is_page"), 0);
	glUniform1f(glGetUniformLocation(shader, "alpha"), 1.0f);

    // Copy data into it
    line_3d_t line;
    line.v0.x = (int16_t)(v0.x >> 12);
    line.v0.y = (int16_t)(v0.y >> 12);
    line.v0.z = (int16_t)(v0.z >> 12);
    line.v1.x = (int16_t)(v1.x >> 12);
    line.v1.y = (int16_t)(v1.y >> 12);
    line.v1.z = (int16_t)(v1.z >> 12);
    line.v0.r = color.r;
    line.v0.g = color.g;
    line.v0.b = color.b;
    line.v1.r = color.r;
    line.v1.g = color.g;
    line.v1.b = color.b;
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_3d_t),
        &line, GL_STATIC_DRAW);

    // Enable depth and draw
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDrawArrays(GL_LINES, 0, 2);
}

void renderer_debug_draw_bvh_triangles(const bvh_t* box, const pixel32_t color, const transform_t* model_transform) {
    for (size_t i = 0; i < box->n_primitives; ++i) {
        const collision_triangle_3d_t* tri = &box->primitives[i];
        renderer_debug_draw_line(tri->v0, tri->v1, color, model_transform);
        renderer_debug_draw_line(tri->v1, tri->v2, color, model_transform);
        renderer_debug_draw_line(tri->v2, tri->v0, color, model_transform);
    }
}

void renderer_debug_draw_aabb(const aabb_t* box, const pixel32_t color, const transform_t* model_transform) {
    // Create 8 vertices
    const vec3_t vertex000 = {box->min.x, box->min.y, box->min.z};
    const vec3_t vertex001 = {box->min.x, box->min.y, box->max.z};
    const vec3_t vertex010 = {box->min.x, box->max.y, box->min.z};
    const vec3_t vertex011 = {box->min.x, box->max.y, box->max.z};
    const vec3_t vertex100 = {box->max.x, box->min.y, box->min.z};
    const vec3_t vertex101 = {box->max.x, box->min.y, box->max.z};
    const vec3_t vertex110 = {box->max.x, box->max.y, box->min.z};
    const vec3_t vertex111 = {box->max.x, box->max.y, box->max.z};

    // Draw the lines
    renderer_debug_draw_line(vertex000, vertex100, color, model_transform);
    renderer_debug_draw_line(vertex100, vertex101, color, model_transform);
    renderer_debug_draw_line(vertex101, vertex001, color, model_transform);
    renderer_debug_draw_line(vertex001, vertex000, color, model_transform);
    renderer_debug_draw_line(vertex010, vertex110, color, model_transform);
    renderer_debug_draw_line(vertex110, vertex111, color, model_transform);
    renderer_debug_draw_line(vertex111, vertex011, color, model_transform);
    renderer_debug_draw_line(vertex011, vertex010, color, model_transform);
    renderer_debug_draw_line(vertex000, vertex010, color, model_transform);
    renderer_debug_draw_line(vertex100, vertex110, color, model_transform);
    renderer_debug_draw_line(vertex101, vertex111, color, model_transform);
    renderer_debug_draw_line(vertex001, vertex011, color, model_transform);
}

void renderer_debug_draw_sphere(const sphere_t sphere) {
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_from_int32s(sphere.radius, 0, 0)), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_from_int32s(-sphere.radius, 0, 0)), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_from_int32s(0, 0, sphere.radius)), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_from_int32s(0, 0, -sphere.radius)), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_mul(vec3_from_int32s(+sphere.radius, 0, +sphere.radius), vec3_from_scalar(2896))), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_mul(vec3_from_int32s(+sphere.radius, 0, -sphere.radius), vec3_from_scalar(2896))), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_mul(vec3_from_int32s(-sphere.radius, 0, +sphere.radius), vec3_from_scalar(2896))), white, &id_transform);
    renderer_debug_draw_line(sphere.center, vec3_add(sphere.center, vec3_mul(vec3_from_int32s(-sphere.radius, 0, -sphere.radius), vec3_from_scalar(2896))), white, &id_transform);
}

void renderer_upload_texture(const texture_cpu_t *texture, const uint8_t index) {
	// This is where all the pixels will be stored
	pixel32_t *pixels = mem_alloc((size_t)texture->width * (size_t)texture->height * 4, MEM_CAT_TEXTURE);

	// The texture is stored in 4bpp format, so each byte in the texture is 2
	// pixels horizontally - Convert to 32-bit color
	for (size_t i = 0; i < (texture->width * texture->height / 2); ++i) {
		// Get indices from texture
		const uint8_t color_index_left = (texture->data[i] >> 0) & 0x0F;
		const uint8_t color_index_right = (texture->data[i] >> 4) & 0x0F;

		// Get 16-bit color values from palette
		const pixel16_t pixel_left = texture->palette[color_index_left];
		const pixel16_t pixel_right = texture->palette[color_index_right];

		// Expand to 32-bit color
		pixels[i * 2 + 0].r = pixel_left.r << 3;
		pixels[i * 2 + 0].g = pixel_left.g << 3;
		pixels[i * 2 + 0].b = pixel_left.b << 3;
		pixels[i * 2 + 0].a = pixel_left.a * 255;

		// And on the right side as well
		pixels[i * 2 + 1].r = pixel_right.r << 3;
		pixels[i * 2 + 1].g = pixel_right.g << 3;
		pixels[i * 2 + 1].b = pixel_right.b << 3;
		pixels[i * 2 + 1].a = pixel_right.a * 255;
	}

	// Upload texture
	glBindTexture(GL_TEXTURE_2D, textures);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 
	((int16_t)index / 4) * 64, 
	(((int16_t)index) % 4) * 64, 
	texture->width, 
	texture->height, 
	GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Store texture resolution
	tex_res[(size_t)index * 2 + 0] = (float)texture->width;
	tex_res[(size_t)index * 2 + 1] = (float)texture->height;

	printf("texture %d has avg color: %d, %d, %d\n", index, texture->avg_color.r,
				 texture->avg_color.g, texture->avg_color.b);

	// Clean up after we're done
	mem_free(pixels);
}

int renderer_get_delta_time_raw() { return 0; }

int renderer_get_delta_time_ms() { return dt; }

uint32_t renderer_get_n_rendered_triangles() { return 0; }

uint32_t renderer_get_n_total_triangles() { return n_total_triangles; }

int renderer_should_close() { return glfwWindowShouldClose(window); }

vec3_t renderer_get_forward_vector() {
    return vec3_from_floats(
        -view_matrix_normal[0][2],
        -view_matrix_normal[1][2],
        -view_matrix_normal[2][2]
    );
}

int renderer_get_level_section_from_position(const model_t* model, vec3_t position) {
    position.x = -position.x / COL_SCALE;
    position.y = -position.y / COL_SCALE;
    position.z = -position.z / COL_SCALE;
    n_sections = 0;
    for (size_t i = 0; i < model->n_meshes; ++i) {
        if (n_sections == N_SECTIONS_PLAYER_CAN_BE_IN_AT_ONCE) break;
        if (point_aabb_intersect(&model->meshes[i].bounds, position)) {
            renderer_debug_draw_aabb(&model->meshes[i].bounds, green, &id_transform);
            sections[n_sections] = i;
            n_sections += 1;
        }
        else {
            renderer_debug_draw_aabb(&model->meshes[i].bounds, red, &id_transform);
        }
    }
    return n_sections; // -1 means no section
}

// todo
void renderer_draw_2d_quad_axis_aligned(vec2_t center, vec2_t size, vec2_t uv_tl, vec2_t uv_br, pixel32_t color, int depth, int texture_id, int is_page) {
	const vec2_t tl = { center.x - size.x / 2, center.y - size.y / 2 };
	const vec2_t tr = { center.x + size.x / 2, center.y - size.y / 2 };
	const vec2_t bl = { center.x - size.x / 2, center.y + size.y / 2 };
	const vec2_t br = { center.x + size.x / 2, center.y + size.y / 2 };
	renderer_draw_2d_quad(tl, tr, bl, br, uv_tl, uv_br, color, depth, texture_id, is_page);
}

void renderer_draw_2d_quad(vec2_t tl, vec2_t tr, vec2_t bl, vec2_t br, vec2_t uv_tl, vec2_t uv_br, pixel32_t color,
    int depth, int texture_id, int is_page) {
	vertex_3d_t verts[4];
	// Top left
	verts[0].x = tl.x / ONE;
	verts[0].y = tl.y / ONE;
	verts[0].u = uv_tl.x / ONE;
	verts[0].v = uv_tl.y / ONE;

	// Top right
	verts[1].x = br.x / ONE;
	verts[1].y = tl.y / ONE;
	verts[1].u = uv_br.x / ONE;
	verts[1].v = uv_tl.y / ONE;

	// Bottom right
	verts[2].x = br.x / ONE;
	verts[2].y = br.y / ONE;
	verts[2].u = uv_br.x / ONE;
	verts[2].v = uv_br.y / ONE;

	// Bottom left
	verts[3].x = tl.x / ONE;
	verts[3].y = br.y / ONE;
	verts[3].u = uv_tl.x / ONE;
	verts[3].v = uv_br.y / ONE;

	for (size_t i = 0; i < 4; ++i) {
		verts[i].x -= 512 / 2;
		verts[i].y -= 272 / 2;
		verts[i].r = color.r;
		verts[i].g = color.g;
		verts[i].b = color.b;
		verts[i].tex_id = texture_id;
		verts[i].z = depth;
		if (!is_page) {
			verts[i].u /= 4;
			verts[i].v /= 4;
		}
	}

	vertex_3d_t triangulated[6] = {
		verts[0], verts[1], verts[2],
		verts[0], verts[2], verts[3]
	};

	// Bind shader
	glUseProgram(shader);

	// Bind texture
	glBindTexture(GL_TEXTURE_2D, textures);

	// Bind vertex buffers
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Set matrices
	mat4 id_matrix;
	glm_mat4_identity(id_matrix);
	mat4 screen_matrix;
	glm_mat4_identity(screen_matrix);
	screen_matrix[0][0] = 1.0f / 256.0f;
	screen_matrix[1][1] = -2.0f / 240.0f;
	screen_matrix[2][2] = 1.0f / 256.0f;
	glUniformMatrix4fv(glGetUniformLocation(shader, "proj_matrix"), 1, GL_FALSE, &id_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader, "view_matrix"), 1, GL_FALSE, &screen_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader, "model_matrix"), 1, GL_FALSE, &id_matrix[0][0]);

	glUniform1i(glGetUniformLocation(shader, "texture_bound"), texture_id != 255);
	glUniform1i(glGetUniformLocation(shader, "texture_offset"), 0);
	glUniform1i(glGetUniformLocation(shader, "texture_is_page"), is_page);
	glUniform1f(glGetUniformLocation(shader, "alpha"), ((float)color.a) / 255.0f);

	// Copy data into it
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vertex_3d_t), triangulated, GL_STATIC_DRAW);

	// Enable depth and draw
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_BLEND);
}

void renderer_draw_text(vec2_t pos, const char* text, const int text_type, const int centered, const pixel32_t color) {
	int font_x;
	int font_y;
	int font_src_width;
	int font_src_height;
	int font_dst_width;
	int font_dst_height;
	int chars_per_row;

	if (text_type == 0) {
		font_x = 0;
		font_y = 60;
		font_src_width = 7;
		font_src_height = 9;
		font_dst_width = 7;
		font_dst_height = 9;
		chars_per_row = 36;
	}
	else if (text_type == 1) {
		font_x = 0;
		font_y = 80;
		font_src_width = 16;
		font_src_height = 16;
		font_dst_width = 16;
		font_dst_height = 16;
		chars_per_row = 16;
	}
	else if (text_type == 2) {
		font_x = 0;
		font_y = 60;
		font_src_width = 7;
		font_src_height = 9;
		font_dst_width = 14;
		font_dst_height = 18;
		chars_per_row = 16;
	}

	if (centered) {
		pos.x -= ((strlen(text)) * font_dst_width - (font_dst_width / 2)) * ONE / 2;
	}

	vec2_t start_pos = pos;

	// Draw each character
	while (*text) {
		// Handle special cases
		if (*text == '\n') {
			pos.x = start_pos.x;
			pos.y += font_dst_height * ONE;
			goto end;
		}

		if (*text == '\r') {
			pos.x = start_pos.x;
			goto end;
		}

		if (*text == '\t') {
			// Get X coordinate relative to start, and round the position up to the nearest multiple of 4
			scalar_t rel_x = pos.x - start_pos.x;
			int n_spaces = 4 - ((rel_x / font_dst_width) % 4);
			pos.x += n_spaces * font_dst_width * ONE;
			goto end;
		}

		// Get index in bitmap
		int index_to_draw = (int)lut_font_letters[(size_t)*text];

		// Get UV coordinates
		vec2_t top_left;
		top_left.x = (font_x + (font_src_width * (index_to_draw % chars_per_row))) * ONE;
		top_left.y = (font_y + ((index_to_draw / chars_per_row) * font_src_height)) * ONE;
		vec2_t bottom_right = vec2_add(top_left, (vec2_t) { (font_src_width - 1)* ONE, (font_src_height - 1)* ONE });

		renderer_draw_2d_quad_axis_aligned(pos, (vec2_t) { (font_dst_width - 1)* ONE, (font_dst_height - 1)* ONE }, top_left, bottom_right, (pixel32_t) { color.r / 2, color.g / 2, color.b / 2, 255 }, 1, 5, 1);
		pos.x += font_dst_width * ONE;
	end:
		++text;
	}

}
void renderer_apply_fade(int fade_level) {
	renderer_draw_2d_quad_axis_aligned(
		(vec2_t){ 256 * ONE, 128 * ONE}, 
		(vec2_t){ 512 * ONE, 256 * ONE }, 
		(vec2_t){ 0, 0 }, 
		(vec2_t){ 0, 0 }, 
		(pixel32_t){ 0, 0, 0, fade_level }, 
		0, 255, 0
	);
}

void render_upload_8bit_texture_page(const texture_cpu_t* texture, const uint8_t index) {
    // This is where all the pixels will be stored
    size_t width = texture->width == 0 ? 256 : texture->width;
    size_t height = texture->height == 0 ? 256 : texture->height;
    pixel32_t* pixels = mem_alloc((size_t)width * (size_t)height * 4, MEM_CAT_TEXTURE);
    // The texture is stored in 8bpp format
    // pixels horizontally - Convert to 32-bit color
    for (size_t i = 0; i < ((size_t)width * (size_t)height); ++i) {
        // Get indices from texture
        const uint8_t color_index = texture->data[i];

        // Get 16-bit color values from palette
        const pixel16_t pixel = texture->palette[color_index];

        // Expand to 32-bit color
        pixels[i].r = pixel.r << 3;
        pixels[i].g = pixel.g << 3;
        pixels[i].b = pixel.b << 3;
        pixels[i].a = 255 * ((pixel.r | pixel.g | pixel.b) != 0);
    }

    // Upload texture
    glBindTexture(GL_TEXTURE_2D, textures);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
        256 * (int)index,
        256,
        width,
        height,
        GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store texture resolution
    tex_res[(size_t)index * 2 + 0] = (float)width;
    tex_res[(size_t)index * 2 + 1] = (float)height;

    printf("texture %d has avg color: %d, %d, %d\n", index, texture->avg_color.r,
        texture->avg_color.g, texture->avg_color.b);

    // Clean up after we're done
    mem_free(pixels);
}

void renderer_set_video_mode(int is_pal) {}
void renderer_cycle_res_x(void) {}
void renderer_draw_mesh_shaded_offset(const mesh_t* mesh, const transform_t* model_transform, int tex_id_offset) {
    tex_id_start = tex_id_offset;
    renderer_draw_mesh_shaded(mesh, model_transform);
}

void renderer_draw_mesh_shaded_offset_local(const mesh_t* mesh, const transform_t* model_transform, int tex_id_offset) {
    tex_id_start = tex_id_offset;
    // Calculate model matrix
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);

    // Apply rotation
    // Apply translation
    // Apply scale
    vec3 position = {
            (float)model_transform->position.x,
            (float)-model_transform->position.y,
            (float)-model_transform->position.z,
    };
    vec3 scale = {
            (float)model_transform->scale.x / 4096.0f,
            (float)model_transform->scale.y / -4096.0f,
            (float)model_transform->scale.z / -4096.0f,
    };
    glm_translate(model_matrix, position);
    glm_scale(model_matrix, scale);
    glm_rotate_x(model_matrix, (float)model_transform->rotation.x * 2 * PI / 131072.0f, model_matrix);
    glm_rotate_y(model_matrix, (float)model_transform->rotation.y * 2 * PI / 131072.0f, model_matrix);
    glm_rotate_z(model_matrix, (float)model_transform->rotation.z * 2 * PI / 131072.0f, model_matrix);

    // Bind shader
    glUseProgram(shader);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, textures);

    // Bind vertex buffers
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Set matrices
    mat4 id_matrix;
    glm_mat4_identity(id_matrix);
    glUniformMatrix4fv(glGetUniformLocation(shader, "proj_matrix"), 1, GL_FALSE, &perspective_matrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "view_matrix"), 1, GL_FALSE, &id_matrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model_matrix"), 1, GL_FALSE, &model_matrix[0][0]);

    glUniform1i(glGetUniformLocation(shader, "texture_bound"), mesh->vertices[0].tex_id != 255);
    glUniform1i(glGetUniformLocation(shader, "texture_offset"), tex_id_start);
	glUniform1i(glGetUniformLocation(shader, "texture_is_page"), 0);
	glUniform1f(glGetUniformLocation(shader, "alpha"), 1.0f);

    // Copy data into it
    glBufferData(GL_ARRAY_BUFFER, ((mesh->n_triangles * 3) + (mesh->n_quads * 4)) * sizeof(vertex_3d_t), mesh->vertices, GL_STATIC_DRAW);

    // Enable depth and draw
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDrawArrays(GL_TRIANGLES, 0, mesh->n_triangles * 3);

    n_total_triangles += mesh->n_triangles;
    tex_id_start = 0;
}