#include <swa/swa.h>
#include <swa/key.h>
#include <dlg/dlg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include "shared.h"
#include "someutils.h"

#include <vulkan/vulkan.h>


static bool premult = false;

// Graphics data
///////////////////////////////////////////////////////////////////
struct Uniforms {
	uint32_t liCamPos;
	float iCamPos[2];
};
struct Uniforms uniforms = {
	0,
	0.0,0.0
};
// struct BVertex { // I didn't understand how to implement that
// 	float positions[2];
// 	float colors[3];
// 	//float padding[3];
// };
// struct BVertex *vertexes;
// float *vertexes;
struct {
	float *positions; // 2 floats postion 1 float color
// 	uint32_t posoff; // always 0 now
	float *colors;
	uint32_t coloff;
} vdata;
uint32_t vertex_count;
uint32_t BV_lposition = 0;
uint32_t BV_lcolor = 1;

//TODO create array of these
uint32_t vao = 0;
uint32_t vbo = 0;

struct WinPos {
	int x;
	int y;
	uint32_t width;
	uint32_t height;
};
struct WinPos win_dimensions = {
	0, 0, 640, 480
};

//TODO: generate shaders from graph
char *vert_shader_text =
"#version 400\n"
"precision highp float;\n"
"in vec2 aPos;\n"
"in vec3 aColor;\n"
"uniform vec2 iCamPos;\n"
"out vec3 vColor;\n"
"void main()\n"
"{\n"
"	gl_Position = vec4(aPos+iCamPos, 0.0f, 1.0f);\n"
"	vColor = aColor;\n"
"}\n";
char *frag_shader_text =
"#version 400\n"
"precision highp float;\n"
"in vec3 vColor;\n"
"uniform vec2 iCamPos;\n"
"out vec4 fColor;\n" // may be vec3, vec4 or float on different windowing systems
"void main()\n"
"{\n"
"	fColor = vec4(vColor, 1.0);\n"
"}\n";

uint32_t shader_program = 0;

// End graphics data
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////

void io_init() {
	inpfd = shm_open(inpname, O_RDWR, S_IRUSR | S_IWUSR);
	if (inpfd == -1) {
        dlg_error("can't open shm %s\n", inpname);
		exit(-1);
    }
	shm = (struct SharedMem*)mmap(0, sizeof(struct SharedMem), PROT_READ|PROT_WRITE, MAP_SHARED, inpfd, 0); // somehow it works without shmopen
	if (shm == MAP_FAILED) {
        dlg_error("mmap failed\n");
		exit(-1);
    }
	shm->pids.graphics = getpid();
	dlg_info("grpid %d", shm->pids.graphics);
}

void ui_init() {
	// allocate vertex buffer
	// background triangle
// 	vertexes = malloc(3*sizeof(struct BVertex));
	vertex_count += 3;
	vdata.positions = malloc(vertex_count*2*4);
	vdata.colors = malloc(vertex_count*3*4);
// 	for(uint8_t v = 0; v < 3; v++) {
// 		vertexes[v]
// 	}
// 	vertexes[0].colors[0] = 1.0;
// 	vertexes[0].colors[1] = 0.0;
// 	vertexes[0].colors[2] = 0.0;
// 	vertexes[0].positions[0] = 0.1;
// 	vertexes[0].positions[1] = 0.1;
// 	vertexes[1].colors[0] = 0.0;
// 	vertexes[1].colors[1] = 1.0;
// 	vertexes[1].colors[2] = 0.0;
// 	vertexes[1].positions[0] = 3.;
// 	vertexes[1].positions[1] = -1.;
// 	vertexes[2].colors[0] = 0.0;
// 	vertexes[2].colors[1] = 0.0;
// 	vertexes[2].colors[2] = 1.0;
// 	vertexes[2].positions[0] = -1.;
// 	vertexes[2].positions[1] = 3.;

	vdata.positions[0] = -2.;
	vdata.positions[1] = -2.;
	vdata.colors[0] = 1.1;
	vdata.colors[1] = 0.1;
	vdata.colors[2] = 0.1;

	vdata.positions[2] = 30.;
	vdata.positions[3] = -10.;
	vdata.colors[3] = 0.1;
	vdata.colors[4] = 1.0;
	vdata.colors[5] = 0.1;

	vdata.positions[4] = -10.;
	vdata.positions[5] = 30.;
	vdata.colors[6] = 0.1;
	vdata.colors[7] = 0.1;
	vdata.colors[8] = 1.1;
	// graph representation

	// copy db buffer data to vertex buffer
// 	for(int i = 0; i < shm->dv.statbuf.st_size; i++) {
// 		for(uint8_t v = 0; v < 3; v++) {

// 		}
// 	}
}

///////////////////////////////////////////////////////////////////

void exit_cleanup();

VkShaderModule create_shader(uint32_t *code, uint32_t codeSize, VkDevice device) {
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = codeSize,
		.pCode = code
	};
	VkShaderModule shaderModule;
	VkResult r = vkCreateShaderModule(device, &shaderModuleCreateInfo, NULL,
		&shaderModule);
	if (r) {
		fprintf(stderr, "[ERROR] vkCreateShaderModule: %d", r);
		return 0;
	}

	return shaderModule;
}



void graphics_init() {

	uint32_t frag, vert;
	int status;

	frag = create_shader(frag_shader_text, GL_FRAGMENT_SHADER);
	vert = create_shader(vert_shader_text, GL_VERTEX_SHADER);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, frag);
	glAttachShader(shader_program, vert);

	glLinkProgram(shader_program);

	glGetProgramiv(shader_program, GL_LINK_STATUS, &status);
	if (!status) {
		char *log = malloc(10000);
		GLsizei len;
		glGetProgramInfoLog(shader_program, 10000, &len, log);
		dlg_error("Error: linking:\n%*s\n", len, log);
		exit(3);
	}

	glUseProgram(shader_program);

	glDeleteShader(vert);
	glDeleteShader(frag);

	glViewport(win_dimensions.x, win_dimensions.y, win_dimensions.width, win_dimensions.height);
	glGenVertexArrays(1, &vao);
	write(1,"1\n",2);
	glBindVertexArray(vao);
	write(1,"1\n",2);

	glGenBuffers(1, &vbo);
	write(1,"1\n",2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	write(1,"1\n",2);

// 	glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, offsetof(struct BVertex, positions));
// 	glVertexAttribBinding(0, 0);
// 	glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, offsetof(struct BVertex, colors));
// 	glVertexAttribBinding(1, 0);

// 	glBindVertexBuffer(0, vbo, 0, sizeof(struct BVertex));
// 	glBufferSubData(vbo, 0, vertex_count, vertexes);

// 	glBindVertexBuffer(0, vbo, 0, 3*2*32);
// 	write(1,"1\n",2);
	write(1,"1\n",2);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);
	write(1,"1\n",2);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)((intptr_t)(vertex_count*2*4)));
	glEnableVertexAttribArray(1);
	write(1,"1\n",2);
	glBufferData(GL_ARRAY_BUFFER, vertex_count*2*4+vertex_count*3*4, 0, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count*2*4, vdata.positions);
	vdata.coloff = vertex_count*2*4;
	glBufferSubData(GL_ARRAY_BUFFER, vdata.coloff, vertex_count*3*4, vdata.colors);
	write(1,"1\n",2);
// 	glBindAttribLocation(shader_program, 0, "aPos");
// 	glBindAttribLocation(shader_program, 1, "aColor");
// 	glEnable(GL_FRAMEBUFFER_SRGB);
	uniforms.liCamPos = glGetUniformLocation(shader_program, "iCamPos");
	glUniform2f(uniforms.liCamPos, uniforms.iCamPos[0], uniforms.iCamPos[1]);
	//glBlendFunc(GL_ONE,GL_ZERO);
	glDisable(GL_BLEND);
	//glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	//glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	//glEnable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glDepthFunc(GL_NEVER);
	float alpha = 0.1f;
	float mult = premult ? alpha : 1.f;
	glClearColor(mult * 0.9f, mult * 0.01f, mult * 1.0f, alpha);
	glClear(GL_COLOR_BUFFER_BIT);
}

static void window_draw(struct swa_window* win) {

	float alpha = 0.1f;
	float mult = premult ? alpha : 1.f;

	glClearColor(mult * 0.8f, mult * 0.6f, mult * 0.3f, alpha);
	glClear(GL_COLOR_BUFFER_BIT);
	glUniform2f(uniforms.liCamPos, uniforms.iCamPos[0], uniforms.iCamPos[1]);
	glBufferData(GL_ARRAY_BUFFER, vertex_count*2*4+vertex_count*3*4, 0, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count*2*4, vdata.positions);
// 	vdata.coloff = vertex_count*2*4;
	glBufferSubData(GL_ARRAY_BUFFER, vdata.coloff, vertex_count*3*4, vdata.colors);
	glDrawArrays(GL_TRIANGLES, 0, vertex_count);
	if(!swa_window_gl_swap_buffers(win)) {
		dlg_error("swa_window_gl_swap_buffers failed");
	}
	// for continous redrawing:
	swa_window_refresh(win);
}

static void window_resize(struct swa_window* win, uint32_t w, uint32_t h) {
	bool ret = swa_window_gl_make_current(win);
	if(!ret) {
		dlg_error("swa_window_gl_make_current failed\n");
		return;
	}
	win_dimensions.width = w;
	win_dimensions.height = h;
	glViewport(win_dimensions.x, win_dimensions.y, win_dimensions.width, win_dimensions.height);
}

static void window_close(struct swa_window* win) {
	shm->run = false;
}

static void window_mouse_button(struct swa_window* win,
		const struct swa_mouse_button_event* ev) {
	if(ev->pressed && ev->button == swa_mouse_button_left) {
		//dlg_debug("begin resize");
		swa_window_begin_resize(win, swa_edge_bottom_right);
	}
}

static void mouse_move(struct swa_window* win,
		const struct swa_mouse_move_event* ev) {
// 	dlg_info("mouse moved to (%d, %d)", ev->x, ev->y);
	uniforms.iCamPos[0] = (float)ev->x/win_dimensions.width;
	uniforms.iCamPos[1] = -(float)ev->y/win_dimensions.height;
}

static void window_key(struct swa_window* win, const struct swa_key_event* ev) {
	//dlg_trace("key: %d %d, utf8: %s", ev->keycode, ev->pressed, ev->utf8 ? ev->utf8 : "<null>");
	if(ev->pressed && ev->keycode == swa_key_escape) {
		dlg_info("Escape pressed, exiting");
		shm->run = false;
	}
	if(ev->keycode == swa_key_a) {
		if(ev->pressed)
			shm->input = inp_left;
		else
			shm->input = inp_none;
	}
	if(ev->keycode == swa_key_d) {
		if(ev->pressed)
			shm->input = inp_right;
		else
			shm->input = inp_none;
	}
	if(ev->keycode == swa_key_r) {
		if(ev->pressed) {
			exit_cleanup();
			exit(0);
		}
	}
}

static void window_surface_created(struct swa_window* win) {
	bool ret = swa_window_gl_make_current(win);
	dlg_assert(ret);
}

static const struct swa_window_listener window_listener = {
	.draw = window_draw,
	.resize = window_resize,
	.mouse_button = window_mouse_button,
	.close = window_close,
	.key = window_key,
	.mouse_move = mouse_move,
	.surface_created = window_surface_created
};
struct swa_display* dpy;
struct swa_window* win;
int main(int argc, char** argv, char** envp) {
	if(argc < 2) {
		printf("usage: graphics [SHM NAME]\n");
		return 0;
	}
	dpy = swa_display_autocreate("swa example-gl");
	if(!dpy) {
		dlg_fatal("No swa backend available");
		return EXIT_FAILURE;
	}

	struct swa_cursor cursor;
	cursor.type = swa_cursor_right_pointer;

	struct swa_window_settings settings;
	swa_window_settings_default(&settings);
	settings.title = "swa-example-window";
	settings.listener = &window_listener;
	settings.cursor = cursor;
	settings.transparent = true;
	settings.surface = swa_surface_gl;
#ifdef __ANDROID__
	settings.surface_settings.gl.major = 2;
	settings.surface_settings.gl.minor = 0;
	settings.surface_settings.gl.api = swa_api_gles;
#else
	settings.surface_settings.gl.major = 4;
	settings.surface_settings.gl.minor = 0;
	settings.surface_settings.gl.api = swa_api_gl;
#endif
	// settings.surface_settings.gl.srgb = true;
	// settings.surface_settings.gl.debug = true;
	// settings.surface_settings.gl.compatibility = false;
	win = swa_display_create_window(dpy, &settings);
	if(!win) {
		//dlg_fatal("Failed to create window");
		swa_display_destroy(dpy);
		return EXIT_FAILURE;
	}

	swa_window_set_userdata(win, dpy);

	if(!swa_window_gl_make_current(win)) {
		dlg_fatal("Could not make gl window current");
		return EXIT_FAILURE;
	}
	inpname = argv[1];

	glEnable(GL_MULTISAMPLE);

	io_init();
	ui_init();
	graphics_init();



	while(shm->run) {
		if(!swa_display_dispatch(dpy, true)) {
			break;
		}
	}

	exit_cleanup();
}

void exit_cleanup() {
	dlg_info("exiting");
	close(inpfd);
	swa_window_destroy(win);
	swa_display_destroy(dpy);
}
