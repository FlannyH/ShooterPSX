#define CGLM_FORCE_LEFT_HANDED
#include <cglm/cam.h>

#include <GL/gl3w.h>

#include <GLFW/glfw3.h>
#include <cglm/affine.h>
#include <cglm/types.h>
#include <cglm/vec3.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "../texture_pool.h"
#include "../common.h"
#include "debug_layer.h"
#include "renderer.h"
#include "memory.h"
#include "input.h"
#include "file.h"
#include "math/vec3.h"
#include "lut.h"

#define PI 3.14159265358979f
#define RESOLUTION_SCALING 4
GLFWwindow* window;
mat4 perspective_matrix;
mat4 view_matrix;
mat4 view_matrix_topdown;
mat4 view_matrix_third_person;
mat4 view_matrix_normal;
GLuint shader_gouraud;
GLuint shader_blit;
GLuint vao;
GLuint vbo;
clock_t dt_clock;
GLuint textures;
clock_t dt = 0;
float dt_ms_float = 0;
int dt_ms_int = 0;
uint32_t n_total_triangles = 0;
int render_w = 512;
int render_h = 240;
int prev_render_w = 0;
int prev_render_h = 0;
int window_w = 32;
int window_h = 32;
int prev_window_w = 0;
int prev_window_h = 0;
float prev_aspect = 0;
float aspect = 16.0f / 9.0f;
transform_t cam_transform;
vec3_t camera_pos;
vec3_t camera_dir;
int curr_depth_bias = 0;
int n_meshes_drawn = 0;

// todo(pc_renderer_globals): desc: reassess global variables
// Need to define these somewhere so it compiles, unused in Windows build
int is_pal = 0;
int vsync_enable = 0; // 0 = unlocked, 1 = 60 fps or 50 fps, 2 = 30 fps or 25 fps
int tex_entity_start = 0;
int tex_weapon_start = 0;
int tex_level_start = 0;
int tex_alloc_cursor = 0;
int res_x = 512; // Pretend it's the same as PSX

GLuint fbo;
GLuint fb_texture;
GLuint fb_depth;
GLuint picking_fb_texture;
int drawing_id = 255;
int drawing_what = 0; // 0 = nothing, 1 = entity, 2 = light

// debug
static int int_mode = 0;
static int edge_mode = 0;

// Textures
GLuint texture_metadata = 0;
texture_entry_t textures_level[MAX_TEXTURE_COUNT] = {0};
texture_entry_t textures_entity[MAX_TEXTURE_COUNT] = {0};
texture_entry_t textures_misc[MAX_TEXTURE_COUNT] = {0};
texture_entry_t textures_weapon[MAX_TEXTURE_COUNT] = {0};
texture_entry_t textures_persistent[MAX_TEXTURE_COUNT] = {0};
texture_entry_t* renderer_get_texture_entry(texture_category_t category, int texture_id) {
    if (texture_id >= 0) {
        switch (category) {
            case TEX_CAT_NONE: return NULL;
            case TEX_CAT_LEVEL: return &textures_level[texture_id];
            case TEX_CAT_ENTITY: return &textures_entity[texture_id];
            case TEX_CAT_WEAPON: return &textures_weapon[texture_id];
            case TEX_CAT_MISC: return &textures_misc[texture_id];
            case TEX_CAT_PERSISTENT: return &textures_persistent[texture_id];
            default: printf("[ERROR] Invalid texture category %i\n", (int)category); return NULL;
        }
    }
    return NULL;
}

struct {
	vec3 direction_position;
	float intensity;
	vec3 color;
	float type;
} converted_lights[MAX_LIGHT_COUNT];
GLuint light_buffer_gpu;

typedef enum { vertex, pixel, geometry, compute } ShaderType;

static void DebugCallbackFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
							  const GLchar *message, const GLvoid *userParam) {
	(void)length;
	(void)userParam;

	// Skip some less useful info
	// http://stackoverflow.com/questions/12004396/opengl-debug-context-performance-warning
	if (id == 131218) return;

	char *sourceString;
	char *typeString;
	char *severityString;

	// The AMD variant of this extension provides a less detailed classification
	// of the error, which is why some arguments might be "Unknown".
	switch (source) {
	case GL_DEBUG_SOURCE_API:				sourceString = "API";				break;
	case GL_DEBUG_SOURCE_APPLICATION:		sourceString = "Application";		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		sourceString = "Window System";		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:	sourceString = "Shader Compiler";	break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:		sourceString = "Third Party";		break;
	case GL_DEBUG_SOURCE_OTHER:				sourceString = "Other";				break;
	default:								sourceString = "Unknown";			break;
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR: 				typeString = "Error";				break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeString = "Deprecated Behavior";	break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: 	typeString = "Undefined Behavior";	break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB: 	typeString = "Portability";			break;
	case GL_DEBUG_TYPE_PERFORMANCE: 		typeString = "Performance";			break;
	case GL_DEBUG_TYPE_OTHER: 				typeString = "Other";				break;
	default: 								typeString = "Unknown";				break;
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH: 	severityString = "High"; 	break;
	case GL_DEBUG_SEVERITY_MEDIUM: 	severityString = "Medium"; 	break;
	case GL_DEBUG_SEVERITY_LOW: 	severityString = "Low"; 	break;
	default: 						severityString = "Unknown"; return;
	}

	printf("GL Debug Callback:\n\tsource: %i:%s\n\ttype: %i:%s\n\tid: %i\n\tseverity: %i:%s\n\tmessage: %s",
			source, sourceString, type, typeString, id, severity, severityString, message);
	return; // this is just here so you can put a breakpoint
}

void update_delta_time_ms(void) {
	clock_t new_dt;
	do {
		new_dt = clock();
		dt = new_dt - dt_clock;
		dt_clock = new_dt;
		dt_ms_float += ((float)dt * 1000.0f) / (float)CLOCKS_PER_SEC;
	} while (dt_ms_float < 1);

	dt_ms_int = (int)dt_ms_float;
	dt_ms_float -= (float)dt_ms_int;
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
		if (result == GL_FALSE) {
			// Log error
			printf("[ERROR] File '%s':\n\n%s\n", path, &frag_shader_error[0]);
			return false;
		}

		printf("[WARN]  File '%s':\n\n%s\n", path, &frag_shader_error[0]);
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
	GLint result = GL_FALSE;
	int log_length;
	glGetProgramiv(shader_gpu, GL_LINK_STATUS, &result);
	glGetProgramiv(shader_gpu, GL_INFO_LOG_LENGTH, &log_length);
	char *frag_shader_error = malloc(log_length);
	glGetProgramInfoLog(shader_gpu, log_length, NULL, &frag_shader_error[0]);
	if (log_length > 0) {
		if (result == GL_FALSE) {
			// Log error
			printf("[ERROR] Linking shader program:\n\n%s\n", &frag_shader_error[0]);
			return false;
		}

		printf("[WARN]  Linking shader program:\n\n%s\n", &frag_shader_error[0]);
	}

	return shader_gpu;
}

int renderer_width(void) {
    return render_w;
}

int renderer_height(void) {
    return render_h;
}

void renderer_init(void) {
	// Create OpenGL window
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	window = glfwCreateWindow(320 * RESOLUTION_SCALING, 240 * RESOLUTION_SCALING, "Sub Nivis", NULL, NULL);
	if (window == NULL) {
		printf("[ERROR] Could not open OpenGL window! Aborting.");
		glfwTerminate();
		exit(-1);
	}
	glfwMakeContextCurrent(window);
	gl3wInit();
	// todo(pc_gl_debug): desc: only enable opengl debug layer this if GL 4.3 core profile is available
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(DebugCallbackFunc, NULL);
	glfwSwapInterval(0);
	glFrontFace(GL_CCW);

	// Set viewport
	glViewport(0, 0, 320 * RESOLUTION_SCALING, 240 * RESOLUTION_SCALING);

	// Set perspective matrix
	glm_perspective(glm_rad(90.0f), 4.0f / 3.0f, 0.1f, 100000.f, perspective_matrix);

	// Load shaders
	shader_gouraud = shader_from_file("GOURAUD.VSH", "GOURAUD.FSH");
	shader_blit = shader_from_file("BLIT.VSH", "BLIT.FSH");

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

	// Initialize delta time clock
	dt_clock = clock();

	// Initialize ImGui
	debug_layer_init(window);

	// Zero init textures
	// todo(pc_texture_atlas_resolution): desc: unhardcode texture atlas resolutions
    uint32_t* zero_data = mem_alloc(2048 * 2048 * 4, MEM_CAT_TEXTURE);
	for (size_t i = 0; i < 2048 * 2048; ++i) {
		zero_data[i] = 0u;
	}

	glGenTextures(1, &textures);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2048, 2048, 0, GL_RGBA, GL_UNSIGNED_BYTE, zero_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    mem_free(zero_data);

	// Create pool info buffer
	const size_t texture_data_size = MAX_TEXTURE_COUNT * N_TEX_CATS * sizeof(uint16_t) * 4;
    uint16_t* zero_metadata = mem_alloc(texture_data_size, MEM_CAT_TEXTURE);
	for (size_t i = 0; i < texture_data_size / 2; ++i) {
		zero_metadata[i] = 0;
	}
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture_metadata);
	glBindTexture(GL_TEXTURE_2D, texture_metadata);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16I, MAX_TEXTURE_COUNT, (GLsizei)N_TEX_CATS, 0, GL_RGBA_INTEGER, GL_SHORT, zero_metadata);
    glBindTexture(GL_TEXTURE_2D, 0);
    mem_free(zero_metadata);

	// Create gpu light buffer
	glGenBuffers(1, &light_buffer_gpu);

	// Create fbo
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Create color attachment
	glGenTextures(1, &fb_texture);
	glBindTexture(GL_TEXTURE_2D, fb_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 320 * RESOLUTION_SCALING, 240 * RESOLUTION_SCALING, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb_texture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

	// Create depth attachment
	glGenTextures(1, &fb_depth);
	glBindTexture(GL_TEXTURE_2D, fb_depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 320 * RESOLUTION_SCALING, 240 * RESOLUTION_SCALING, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb_depth, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fb_depth, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

	// Create object picking framebuffer data
	glGenTextures(1, &picking_fb_texture);
	glBindTexture(GL_TEXTURE_2D, picking_fb_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, 320 * RESOLUTION_SCALING, 240 * RESOLUTION_SCALING, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, picking_fb_texture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

	// Check if ok
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		printf("[ERROR] FBO incomplete: 0x%X\n", status);
	}

	GLenum draw_buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, draw_buffers);

	glfwGetWindowSize(window, &window_w, &window_h);

	texture_pool_init(0, 0, 0, 2048);
}
double lasttime = 0.0;
void renderer_begin_frame(const transform_t *camera_transform) {
	curr_depth_bias = 0;
    cam_transform = *camera_transform;
	lasttime = glfwGetTime();
	// Set up viewport
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, render_w, render_h);

#ifndef _LEVEL_EDITOR
	glfwGetWindowSize(window, &window_w, &window_h);
	aspect = (float)window_w / (float)window_h;
#endif
	if (aspect != prev_aspect) {
		// Recreate projection matrix
		widescreen = (aspect >= 16.0f/10.0f);
		glm_perspective(glm_rad(90.0f), aspect, 0.1f, 100000.f, perspective_matrix);
		prev_window_w = window_w;
		prev_window_h = window_h;
		prev_aspect = aspect;
	}

	if (render_w != prev_render_w || render_h != prev_render_h) {
		// Resize color attachment
		glBindTexture(GL_TEXTURE_2D, fb_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, render_w, render_h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb_texture, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Resize depth attachment
		glBindTexture(GL_TEXTURE_2D, fb_depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, render_w, render_h, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb_depth, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fb_depth, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Resize object picking attachment
		glBindTexture(GL_TEXTURE_2D, picking_fb_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, render_w, render_h, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, picking_fb_texture, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Check if ok
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			printf("[ERROR] FBO incomplete: 0x%X\n", status);
		}
	}

	// Convert from PS1 to GLM
    vec3 position = {
        -(float)camera_transform->position.x / ONE,
        -(float)camera_transform->position.y / ONE,
        -(float)camera_transform->position.z / ONE
    };
	const vec3 rotation = {
		-((float)camera_transform->rotation.x * (2 * PI / ONE)),
		-((float)camera_transform->rotation.y * (2 * PI / ONE)),
		-((float)camera_transform->rotation.z * (2 * PI / ONE))
	};

	// Set view matrix
	glm_mat4_identity(view_matrix_normal);

	// Apply rotation
	glm_rotate_x(view_matrix_normal, rotation[0], view_matrix_normal);
	glm_rotate_y(view_matrix_normal, rotation[1], view_matrix_normal);
	glm_rotate_z(view_matrix_normal, rotation[2], view_matrix_normal);

	// Apply translation
	glm_translate(view_matrix_normal, position);

    // Set view matrix for top down debug
    vec3 top_down_position = { position[0], position[1] + 12000, position[2] };
    glm_mat4_identity(view_matrix_topdown);
    glm_rotate_x(view_matrix_topdown, PI / 2.0f, view_matrix_topdown);
    glm_rotate_z(view_matrix_topdown, PI, view_matrix_topdown);
    glm_translate(view_matrix_topdown, top_down_position);

    glm_mat4_identity(view_matrix_third_person);
    vec3 distance = { 0, 0, 400 };
    glm_translate(view_matrix_third_person, distance);

    // Apply rotation
    glm_rotate_x(view_matrix_third_person, rotation[0], view_matrix_third_person);
    glm_rotate_y(view_matrix_third_person, rotation[1], view_matrix_third_person);
    glm_rotate_z(view_matrix_third_person, rotation[2], view_matrix_third_person);

    // Apply translation
    glm_translate(view_matrix_third_person, position);

	// Clear screen
	glStencilMask(0xFF);
	float clear_color0[] = { 0.1f, 0.1f, 0.2f, 1.0f}; glClearBufferfv(GL_COLOR, 0, clear_color0);
	float clear_color1[] = { 0.0f, 0.0f, 0.0f, 0.0f}; glClearBufferfv(GL_COLOR, 1, clear_color1);
	glClearDepth(1.0); glClear(GL_DEPTH_BUFFER_BIT);

	memcpy(view_matrix, view_matrix_normal, sizeof(view_matrix_normal));

	camera_dir.x = view_matrix_normal[2][0] * ONE;
	camera_dir.y = view_matrix_normal[2][1] * ONE;
	camera_dir.z = view_matrix_normal[2][2] * ONE;
	memcpy(&camera_pos, &camera_transform->position, sizeof(camera_pos));

	n_total_triangles = 0;

	if (input_held(PAD_SELECT, 0) && input_pressed(PAD_UP, 0)) {
		++int_mode;
		printf("int_mode: %i\n", int_mode);
	}
	if (input_held(PAD_SELECT, 0) && input_pressed(PAD_DOWN, 0)) {
		--int_mode;
		printf("int_mode: %i\n", int_mode);
	}
	if (input_held(PAD_SELECT, 0) && input_pressed(PAD_LEFT, 0)) {
		--edge_mode;
		printf("edge_mode: %i\n", edge_mode);
	}
	if (input_held(PAD_SELECT, 0) && input_pressed(PAD_RIGHT, 0)) {
		++edge_mode;
		printf("edge_mode: %i\n", edge_mode);
	}
}

void renderer_end_frame(void) {
    renderer_tick_fade();

	glfwGetWindowSize(window, &window_w, &window_h);

#ifndef _LEVEL_EDITOR
	// Blit framebuffer to window
	glUseProgram(shader_blit);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glViewport(0, 0, window_w, window_h);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fb_texture);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glUseProgram(0);
#endif

	// Flip buffers
	glfwSwapInterval(vsync_enable);
	glfwSwapBuffers(window);
	glfwPollEvents();
}

int32_t max_dot_value = 0;
void renderer_draw_mesh_shaded(mesh_t* mesh, const transform_t *model_transform, int local, int facing_camera) {
	++n_meshes_drawn;

	// Calculate model matrix
	mat4 model_matrix;
	glm_mat4_identity(model_matrix);
    glViewport(0, 0, render_w, render_h);

	// Apply rotation
	// Apply translation
	// Apply scale
    vec3 position = {
		(float)model_transform->position.x / ONE,
		(float)model_transform->position.y / ONE,
		(float)model_transform->position.z / ONE,
    };
    vec3 scale = {
		(float)model_transform->scale.x / ONE,
		(float)model_transform->scale.y / ONE,
		(float)model_transform->scale.z / ONE,
    };
	glm_translate(model_matrix, position);
    glm_scale(model_matrix, scale);
	if (facing_camera) {
		vec3 camera_pos_float = {
			(float)camera_pos.x / ONE,
			(float)camera_pos.y / ONE,
			(float)camera_pos.z / ONE,
		};
		vec3 up = {  0.0f, 1.0f, 0.0f };
		vec3 forward;
		vec3 right;
		glm_vec3_sub(camera_pos_float, position, forward);
		glm_vec3_normalize(forward);
		glm_vec3_cross(up, forward, right);
		glm_vec3_normalize(right);
		model_matrix[0][0] = right[0]; 		model_matrix[0][1] = right[1];  	model_matrix[0][2] = right[2];
		model_matrix[1][0] = up[0]; 		model_matrix[1][1] = up[1];     	model_matrix[1][2] = up[2];
		model_matrix[2][0] = forward[0]; 	model_matrix[2][1] = forward[1];	model_matrix[2][2] = forward[2];
	}
	else {
		glm_rotate_x(model_matrix, (float)model_transform->rotation.x * 2 * PI / ONE, model_matrix);
		glm_rotate_y(model_matrix, (float)model_transform->rotation.y * 2 * PI / ONE, model_matrix);
		glm_rotate_z(model_matrix, (float)model_transform->rotation.z * 2 * PI / ONE, model_matrix);
	}

	glUseProgram(shader_gouraud);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, light_buffer_gpu);
	unsigned int lights_index = glGetUniformBlockIndex(shader_gouraud, "Lights");
	glUniformBlockBinding(shader_gouraud, lights_index, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_metadata);
	glUniform1i(glGetUniformLocation(shader_gouraud, "tex"), 0);
	glUniform1i(glGetUniformLocation(shader_gouraud, "tex_meta"), 1);
	glBindVertexArray(mesh->vao);

	if (mesh->vbo_vertices == 0) {
		glGenVertexArrays(1, &mesh->vao);
		glBindVertexArray(mesh->vao);
		glGenBuffers(1, &mesh->vbo_vertices);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_vertices);
		glBufferData(GL_ARRAY_BUFFER, ((mesh->n_triangles * 3) + (mesh->n_quads * 4)) * sizeof(vertex_3d_t), mesh->vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, sizeof(vertex_3d_t), (const void*)offsetof(vertex_3d_t, x));
		glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_3d_t), (const void*)offsetof(vertex_3d_t, r));
		glVertexAttribPointer(2, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vertex_3d_t), (const void*)offsetof(vertex_3d_t, u));
		glVertexAttribPointer(3, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vertex_3d_t), (const void*)offsetof(vertex_3d_t, tex_id));

		if (mesh->vbo_normals == 0 && mesh->normals) {
			glGenBuffers(1, &mesh->vbo_normals);
			glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_normals);
			glBufferData(GL_ARRAY_BUFFER, ((mesh->n_triangles * 3) + (mesh->n_quads * 4)) * sizeof(normal_t), mesh->normals, GL_STATIC_DRAW);
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_BYTE, GL_TRUE, sizeof(normal_t), 0);
		}
	}

	// Set matrices
	glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "proj_matrix"), 1, GL_FALSE, &perspective_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "model_matrix"), 1, GL_FALSE, &model_matrix[0][0]);
	if (local) {
		static const mat4 id_matrix =   {{1.0f, 0.0f, 0.0f, 0.0f},                    \
                                 		 {0.0f, 1.0f, 0.0f, 0.0f},                    \
                                 		 {0.0f, 0.0f, 1.0f, 0.0f},                    \
                                 		 {0.0f, 0.0f, 0.0f, 1.0f}};
		glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "view_matrix"), 1, GL_FALSE, &id_matrix[0][0]);
	}
	else {
		glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "view_matrix"), 1, GL_FALSE, &view_matrix[0][0]);
	}

    glUniform1i(glGetUniformLocation(shader_gouraud, "texture_bound"), mesh->vertices[0].tex_id != 255);
    glUniform1i(glGetUniformLocation(shader_gouraud, "texture_offset"), 0);
	glUniform1i(glGetUniformLocation(shader_gouraud, "curr_depth_bias"), curr_depth_bias);
	glUniform1i(glGetUniformLocation(shader_gouraud, "interpolation_mode"), int_mode);
	glUniform1i(glGetUniformLocation(shader_gouraud, "edge_behavior"), edge_mode);
	glUniform1i(glGetUniformLocation(shader_gouraud, "tex_category"), (GLint)mesh->tex_category);
	glUniform1i(glGetUniformLocation(shader_gouraud, "drawing_id"), drawing_id);
	glUniform1i(glGetUniformLocation(shader_gouraud, "drawing_what"), drawing_what);
	glUniform1i(glGetUniformLocation(shader_gouraud, "vertex_lighting"), 0);
	glUniform1f(glGetUniformLocation(shader_gouraud, "alpha"), 1.0f);

	// Enable depth, culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Draw
	if (mesh->n_triangles) glDrawArrays(GL_TRIANGLES, 0, mesh->n_triangles * 3);
	// todo(pc_quad_deprecated): desc: get rid of quads on pc (they're legacy feature and aren't guaranteed to work)
	if (mesh->n_quads) glDrawArrays(GL_QUADS, mesh->n_triangles * 3, mesh->n_quads * 4);

	n_total_triangles += mesh->n_triangles;

#ifdef _LEVEL_EDITOR
	drawing_id = 0;
	drawing_what = 0;
#endif
}

#ifdef _LEVEL_EDITOR
void renderer_set_drawing_id(int id, int what) {
	drawing_id = id;
	drawing_what = what;
}

void renderer_update_window_res(int width, int height) {
	window_w = width;
	window_h = height;
	render_w = width;
	render_h = height;
	aspect = (float)width / (float)height;
}

void renderer_update_lights(const light_t* const lights) {
	size_t i = 0;
	while (i < MAX_LIGHT_COUNT) {
		if (lights[i].type != LIGHT_NONE) {
			converted_lights[i].direction_position[0] = (float)(lights[i].direction_position.x) / ONE;
			converted_lights[i].direction_position[1] = (float)(lights[i].direction_position.y) / ONE;
			converted_lights[i].direction_position[2] = (float)(lights[i].direction_position.z) / ONE;
			converted_lights[i].intensity = (float)(lights[i].intensity) / 256.0f;
			converted_lights[i].color[0] = (float)(lights[i].color_r) / 255.0f;
			converted_lights[i].color[1] = (float)(lights[i].color_g) / 255.0f;
			converted_lights[i].color[2] = (float)(lights[i].color_b) / 255.0f;
			converted_lights[i].type = (float)lights[i].type;
			++i;
		}
		else {
			converted_lights[i].type = 0.0f;
			++i;
		}
	}

	glBindBuffer(GL_UNIFORM_BUFFER, light_buffer_gpu);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(converted_lights), converted_lights, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
#endif

void renderer_debug_draw_line(vec3_t v0, vec3_t v1, pixel32_t color, const transform_t* model_transform) {
    // Calculate model matrix
    mat4 model_matrix;
    glm_mat4_identity(model_matrix);

    // Apply rotation
    // Apply translation
    // Apply scale
    vec3 position = {
		(float)model_transform->position.x / (float)ONE,
		(float)model_transform->position.y / (float)ONE,
		(float)model_transform->position.z / (float)ONE,
    };
    vec3 scale = {
		(float)model_transform->scale.x / (float)ONE,
		(float)model_transform->scale.y / (float)ONE,
		(float)model_transform->scale.z / (float)ONE,
    };
    glm_translate(model_matrix, position);
    glm_scale(model_matrix, scale);
    glm_rotate_x(model_matrix, (float)model_transform->rotation.x * 2 * PI / ONE, model_matrix);
    glm_rotate_y(model_matrix, (float)model_transform->rotation.y * 2 * PI / ONE, model_matrix);
    glm_rotate_z(model_matrix, (float)model_transform->rotation.z * 2 * PI / ONE, model_matrix);

    // Bind shader
    glUseProgram(shader_gouraud);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, 0);
    glUniform1i(glGetUniformLocation(shader_gouraud, "texture_bound"), 0);
	glUniform1i(glGetUniformLocation(shader_gouraud, "tex"), 0);
	glUniform1i(glGetUniformLocation(shader_gouraud, "tex_meta"), 1);

    // Bind vertex buffers
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "proj_matrix"), 1, GL_FALSE, &perspective_matrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "view_matrix"), 1, GL_FALSE, &view_matrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "model_matrix"), 1, GL_FALSE, &model_matrix[0][0]);
    glUniform1i(glGetUniformLocation(shader_gouraud, "curr_depth_bias"), curr_depth_bias - 16);
	glUniform1i(glGetUniformLocation(shader_gouraud, "interpolation_mode"), 0);
	glUniform1i(glGetUniformLocation(shader_gouraud, "edge_behavior"), 0);
    glUniform1f(glGetUniformLocation(shader_gouraud, "alpha"), 1.0f);

    // Copy data into it
    line_3d_t line;
    line.v0.x = (int16_t)int_from_scalar(v0.x);
    line.v0.y = (int16_t)int_from_scalar(v0.y);
    line.v0.z = (int16_t)int_from_scalar(v0.z);
    line.v1.x = (int16_t)int_from_scalar(v1.x);
    line.v1.y = (int16_t)int_from_scalar(v1.y);
    line.v1.z = (int16_t)int_from_scalar(v1.z);
    line.v0.r = color.r;
    line.v0.g = color.g;
    line.v0.b = color.b;
    line.v1.r = color.r;
    line.v1.g = color.g;
    line.v1.b = color.b;
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_3d_t),
        &line, GL_STATIC_DRAW);

    // Enable depth and draw
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_LINES, 0, 2);
}

void renderer_debug_draw_bvh_triangles(const level_collision_t* box, const pixel32_t color, const transform_t* model_transform) {
    for (size_t i = 0; i < box->n_primitives; ++i) {
        const collision_triangle_3d_t* tri = &box->primitives[i];
        renderer_debug_draw_line(tri->v0, tri->v1, color, model_transform);
        renderer_debug_draw_line(tri->v1, tri->v2, color, model_transform);
        renderer_debug_draw_line(tri->v2, tri->v0, color, model_transform);
    }
}

static inline pixel32_t pixel16_to_32(const pixel16_t pixel) {
	return (pixel32_t) {
		pixel.r << 3,
		pixel.g << 3,
		pixel.b << 3,
		pixel.a * 255,
	};
}

void renderer_upload_texture(const texture_cpu_t* texture, int index, texture_category_t category) {
    assert(texture != NULL);

	// todo(pc_renderer_extend_transparent): desc: fill transparent pixels by extending from the opaque pixels for better alpha cutting

	// if texture resolution is 0, interpret it as 256
	uint32_t width = (uint32_t)texture->width;
	uint32_t height = (uint32_t)texture->height;
	if (width == 0) width = 256;
	if (height == 0) height = 256;
	const size_t pixel_count = width * height;

	pixel32_t* pixels = mem_alloc(pixel_count * sizeof(*pixels), MEM_CAT_TEXTURE);
	for (size_t pi = 0; pi < pixel_count; ++pi) {
		pixels[pi] = (pixel32_t){.r = 0x72, .g = 0x05, .b = 0xFF, .a = 0x00};
	}

	if (texture->bits_per_pixel == 4) {
		const uint8_t* pixel_bytes = (const uint8_t*)texture->data;
		const size_t pixels_per_unit = 2;
		const size_t byte_count = (pixel_count + 1) / pixels_per_unit;

		size_t dst_i = 0;
		for (size_t i = 0; i < byte_count; ++i) {
			// Get 2 indices from 1 byte of texture data
			const uint8_t color_index_left = (pixel_bytes[i] >> 0) & 0x0F;
			const uint8_t color_index_right = (pixel_bytes[i] >> 4) & 0x0F;

			// Get 16-bit color values from palette
			const pixel16_t pixel_left = texture->palette[(size_t)color_index_left];

			// Expand to 32-bit color
			pixels[dst_i++] = pixel16_to_32(pixel_left);
			if (dst_i < pixel_count) {
				const pixel16_t pixel_right = texture->palette[(size_t)color_index_right];
				pixels[dst_i++] = pixel16_to_32(pixel_right);
			}
		}
	}

	else if (texture->bits_per_pixel == 8) {
		const uint8_t* pixel_bytes = (const uint8_t*)texture->data;

		for (size_t i = 0; i < (width * height); ++i) {
			// Fetch and expand color
			const uint8_t color_index = pixel_bytes[i];
			const pixel16_t pixel = texture->palette[(size_t)color_index];
			pixels[i] = pixel16_to_32(pixel);
		}
	}

	else if (texture->bits_per_pixel == 16) {
		const pixel16_t* tex_pixels = (const pixel16_t*)texture->data;

		for (size_t i = 0; i < (width * height); ++i) {
			// Fetch and expand color
			const pixel16_t pixel = tex_pixels[i];
			pixels[i] = pixel16_to_32(pixel);
		}
	}

	else {
		printf("Invalid bits per pixel, got %i, expected 4, 8 or 16\n", texture->bits_per_pixel);
		mem_free(pixels);
		return;
	}

	const int pool_id = 0;
	int pool_entry = texture_pool_alloc(pool_id, width, height);
	if (pool_entry < 0) {
#ifdef _DEBUG
		static const char* texture_category_names[] = {
			"TEX_CAT_NONE",
			"TEX_CAT_LEVEL",
			"TEX_CAT_ENTITY",
			"TEX_CAT_WEAPON",
			"TEX_CAT_MISC",
			"N_TEX_CATS"
		};
		printf("[ERROR] Failed to allocate texture %i in category %s, ran out of texture pool space\n", index, texture_category_names[(size_t)category]);
#endif
		mem_free(pixels);
		return;
	}
	rect_t tex_rect = texture_pool_rect(pool_id, pool_entry);

	texture_entry_t tex_entry = {
		.offset_u = tex_rect.x,
		.offset_v = tex_rect.y,
		.width = texture->width,
		.height = texture->height,
		.allocated = 1,
		.average_color = texture->avg_color,
		.texture_pool_id = pool_id,
		.texture_entry_id = pool_entry,
		.palette_pool_id = 255,
		.palette_entry_id = 255,
	};

    switch (category) {
        case TEX_CAT_LEVEL: textures_level[index] = tex_entry; break;
        case TEX_CAT_ENTITY: textures_entity[index] = tex_entry; break;
        case TEX_CAT_WEAPON: textures_weapon[index] = tex_entry; break;
        case TEX_CAT_MISC: textures_misc[index] = tex_entry; break;
        default: break;
    }

	// Update meta info
	glBindTexture(GL_TEXTURE_2D, texture_metadata);
	glTexSubImage2D(GL_TEXTURE_2D, 0, (GLint)index, (GLint)category, 1, 1, GL_RGBA_INTEGER, GL_SHORT, &tex_rect);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Upload texture
	glBindTexture(GL_TEXTURE_2D, textures);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		tex_rect.x,
		tex_rect.y,
		tex_rect.w,
		tex_rect.h,
		GL_RGBA, GL_UNSIGNED_BYTE, pixels
	);
	// todo(pc_renderer_defer_mipmap_gen): desc: expensive, maybe mark texture as dirty and generate on begin frame?
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Clean up after we're done
	mem_free(pixels);
}

int renderer_delta_time_ms(dt_flags_t flags) {
	if (flags == DT_TICK) {
		update_delta_time_ms();
	}
	return dt_ms_int;
}

uint32_t renderer_get_n_total_triangles(void) { return n_total_triangles; }

int renderer_n_meshes_drawn(void) { return n_meshes_drawn; }

int renderer_should_close(void) { return glfwWindowShouldClose(window); }

vec3_t renderer_get_forward_vector(void) {
    return vec3_from_floats(
        -view_matrix_normal[0][2],
        -view_matrix_normal[1][2],
        -view_matrix_normal[2][2]
    );
}

void renderer_draw_2d_quad(vec2_t tl, vec2_t tr, vec2_t bl, vec2_t br, vec2_t uv_tl, vec2_t uv_br, pixel32_t color,
    int depth, int texture_id, texture_category_t category) {
	vertex_3d_t verts[4];
	// Top left
	verts[0].x = tl.x / ONE;
	verts[0].y = tl.y / ONE;
	verts[0].u = uv_tl.x / ONE;
	verts[0].v = uv_tl.y / ONE;

	// Top right
	verts[1].x = tr.x / ONE;
	verts[1].y = tr.y / ONE;
	verts[1].u = uv_br.x / ONE;
	verts[1].v = uv_tl.y / ONE;

	// Bottom right
	verts[2].x = br.x / ONE;
	verts[2].y = br.y / ONE;
	verts[2].u = uv_br.x / ONE;
	verts[2].v = uv_br.y / ONE;

	// Bottom left
	verts[3].x = bl.x / ONE;
	verts[3].y = bl.y / ONE;
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
	}

	const vertex_3d_t triangulated[6] = {
		verts[0], verts[1], verts[2],
		verts[0], verts[2], verts[3]
	};

	// Bind shader
	glUseProgram(shader_gouraud);

	// Bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_metadata);
	glUniform1i(glGetUniformLocation(shader_gouraud, "tex"), 0);
	glUniform1i(glGetUniformLocation(shader_gouraud, "tex_meta"), 1);

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
	glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "proj_matrix"), 1, GL_FALSE, &id_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "view_matrix"), 1, GL_FALSE, &screen_matrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader_gouraud, "model_matrix"), 1, GL_FALSE, &id_matrix[0][0]);

	glUniform1i(glGetUniformLocation(shader_gouraud, "texture_bound"), texture_id != 255);
	glUniform1i(glGetUniformLocation(shader_gouraud, "curr_depth_bias"), curr_depth_bias);
	glUniform1i(glGetUniformLocation(shader_gouraud, "interpolation_mode"), 0);
	glUniform1i(glGetUniformLocation(shader_gouraud, "edge_behavior"), 0);
	glUniform1i(glGetUniformLocation(shader_gouraud, "tex_category"), (GLint)category);
	glUniform1f(glGetUniformLocation(shader_gouraud, "alpha"), ((float)color.a) / 255.0f);

	// Copy data into it
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vertex_3d_t), triangulated, GL_STATIC_DRAW);

	// Enable depth and draw
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_BLEND);
}

void renderer_apply_fade(scalar_t fade_level) {
	const scalar_t fade_level_normalized = scalar_div(fade_level, MAX_FADE_LEVEL);
	const int fade_level_255 = (fade_level_normalized * 255) / ONE;

	renderer_draw_2d_quad_axis_aligned(
		(vec2_t){ 256 * ONE, 128 * ONE},
		(vec2_t){ 512 * ONE, 256 * ONE },
		(vec2_t){ 0, 0 },
		(vec2_t){ 0, 0 },
		(pixel32_t){ 0, 0, 0, fade_level_255 },
		0, 255, 0
	);
}

void renderer_set_video_mode(int is_pal) {
	(void)is_pal;
}

void renderer_set_depth_bias(int bias) {
	curr_depth_bias = bias;
}

float* renderer_debug_perspective_matrix(void) {
	return &perspective_matrix[0][0];
}

float* renderer_debug_view_matrix(void) {
	return &view_matrix[0][0];
}
