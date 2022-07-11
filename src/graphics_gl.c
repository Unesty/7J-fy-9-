#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include <sys/signal.h>

#include <math.h>

#include <swa/swa.h>
#include <swa/key.h>
#include <dlg/dlg.h>

#include "shared.h"

// this is important as a msvc workaround: their gl.h header is
// broken windows.h has to be included first (which is pulled by stdlib.h)
// Thanks Bill!
#ifdef _WIN32
  #include <windows.h>
#endif

#ifdef __ANDROID__
  #include <GLES2/gl2.h>
#else
  #define GL_GLEXT_PROTOTYPES 1
  #include <GL/gl.h>
  #include <GL/glext.h>
#endif

#define GL_FRAMEBUFFER_SRGB 0x8DB9

static bool premult = false;

void exit_cleanup();

///////////////////////////////////////////////////////////////////
// Graphics data
struct Uniforms {
	uint32_t liCamPos;
	float iCamPos[3];
};
struct Uniforms uniforms = {
	0,
	0.0,0.0,1.0
};
// struct BVertex { // I didn't understand how to implement that
// 	float positions[2];
// 	float colors[3];
// 	//float padding[3];
// };
// struct BVertex *vertexes;
// float *vertexes;
uint32_t vertex_count=0;
struct {
	float *positions; // 2 floats postion 1 float color until we find out why more than 1 vertex array cause crash
// 	uint32_t posoff; // always 0 now
	uint32_t poslen;
	float *colors;
	uint32_t coloff;
	uint32_t collen;
} vdata = {
	.positions = NULL,
	.poslen = 3,
	.colors = NULL,
	.coloff = 0, // sizeof(positions)
	.collen = 3,
};
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
"in vec3 aPos;\n"
// "in vec3 aColor;\n"
"uniform vec3 iCamPos;\n"
// "out vec3 vColor;\n"
"out float vColor;\n"
"void main()\n"
"{\n"
"	gl_Position = vec4((aPos[0]+iCamPos[0])/iCamPos[2], (aPos[1]+iCamPos[1])/iCamPos[2], 0.0, 1.0);\n"
// "	vColor = aPos[2].xxx;\n"
"	vColor = aPos[2];\n"
"}\n";
char *frag_shader_text =
"#version 400\n"
"precision highp float;\n"
// "in vec3 vColor;\n"
"in float vColor;\n"
"uniform vec3 iCamPos;\n"
"out vec4 fColor;\n" // may be vec3, vec4 or float on different windowing systems
"void main()\n"
"{\n"
// "	fColor = vec4(vColor, 1.0);\n"
"	fColor = vec4(vColor, vColor, vColor, 0.7);\n"
"}\n";

GLuint shader_program = 0;

// End graphics data
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// Init and IPC

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

	// open db file
	int res = stat(shm->db_name, &dv.statbuf);
    if(res == -1) {
        dlg_error("failed to stat %s", shm->db_name); // we need a graph of errors, so we need to save parsing history in another graph
        return;
    }
    db_fds[0] = open(shm->db_name, O_RDWR);
    if(db_fds[0] == 0) {
        dlg_error("failed to open file %s", shm->db_name);
        return;
    }
    db_sz = dv.statbuf.st_size;
    if(db_sz < 32) {
        dlg_error("file size is less than minimal");
    }
	buf = mmap(NULL, db_sz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, db_fds[0], 0);

    if(buf == MAP_FAILED) {
        dlg_error("db mmap failed");
        return;
    }
    idshifts = &buf[shm->gri.idshifts_off];
}

void ui_init() {
	// allocate vertex buffer
	// background triangle
// 	vertexes = malloc(3*sizeof(struct BVertex));
	vertex_count += 3+3*shm->gri.vertexcnt;
	vdata.positions = malloc(vertex_count*vdata.poslen*4);
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

	vdata.positions[0] = -3.;
	vdata.positions[1] = -3.;

	vdata.positions[2] = 0.; // black

// 	vdata.colors[0] = 1.1;
// 	vdata.colors[1] = 0.1;
// 	vdata.colors[2] = 0.1;

	vdata.positions[3] = 6.;
	vdata.positions[4] = -3.;

	vdata.positions[5] = 4.; // white

// 	vdata.colors[3] = 0.1;
// 	vdata.colors[4] = 1.0;
// 	vdata.colors[5] = 0.1;

	vdata.positions[6] = -3.;
	vdata.positions[7] = 6.;

	vdata.positions[8] = -2.; // also black, but with bigger value in interpolation

// 	vdata.colors[6] = 0.1;
// 	vdata.colors[7] = 0.1;
// 	vdata.colors[8] = 1.1;
	uint32_t p = 9; // last vdata id of last triangle + 1
	// graph representation

	// draw triangles representing graph data in vertex buffer
	float gap = 1.5;
	float nwidth = 0.5;
	float ewidth = 0.005;
	// draw nodes and out edges
	dlg_debug("shm->gri.vertexcnt = %d", shm->gri.vertexcnt);
	for(uint32_t i = 0; i < shm->gri.vertexcnt; i++) {
// 		for(uint8_t v = 0; v < 3; v++) {
// 		}
		vdata.positions[p+i*9] = i*gap;
		vdata.positions[p+i*9+1] = i*nwidth*0.01;
		vdata.positions[p+i*9+2] = 0.6;
		vdata.positions[p+i*9+1*3] = i*gap-nwidth*0.5;
		vdata.positions[p+i*9+1*3+1] = i*nwidth*0.01+sqrtf(nwidth*nwidth*0.75);
		vdata.positions[p+i*9+1*3+2] = 1;
		vdata.positions[p+i*9+2*3] = i*gap+nwidth*0.5;
		vdata.positions[p+i*9+2*3+1] = i*nwidth*0.01+sqrtf(nwidth*nwidth*0.75);
		vdata.positions[p+i*9+2*3+2] = 1;
	}
}

void ui_update() {
	// clear color isn't used, so backend follows camera to clear background in 1 pass
	vdata.positions[0] = (-3*uniforms.iCamPos[2]-uniforms.iCamPos[0]);
	vdata.positions[1] = (-3*uniforms.iCamPos[2]-uniforms.iCamPos[1]);
	vdata.positions[2] = 0.1+2/(uniforms.iCamPos[2]);

	vdata.positions[3] = (6.-uniforms.iCamPos[0])*uniforms.iCamPos[2];
	vdata.positions[4] = (-3-uniforms.iCamPos[1])*uniforms.iCamPos[2];
	vdata.positions[5] = 0.5-0.0000001*(uniforms.iCamPos[2]);

	vdata.positions[6] = (-3*uniforms.iCamPos[2]-uniforms.iCamPos[0]);
	vdata.positions[7] = (6.*uniforms.iCamPos[2]-uniforms.iCamPos[1]);
	vdata.positions[8] = 0.5-0.0000001*(uniforms.iCamPos[2]);
}

uint8_t reopened_buf = 0;
void sig_handler(int signum) {
	// kill child processes if any
	if(signum == SIGTERM || signum == SIGINT) {
		shm_unlink(inpname);
		close(inpfd);
		exit_cleanup();
		// can wait for children here
		printf("kill -%d signal handled\n", signum);
		exit(signum);
	}
	if(signum == SIGBUS || signum == SIGUSR1) {
		close(db_fds[0]);
		dlg_debug("reopening db %d time", reopened_buf);
		if(stat(shm->db_name, &dv.statbuf) == -1) {
			dlg_error("failed to stat %s", shm->db_name); // we need a graph of errors, so we need to save parsing history in another graph
			return;
		}
		db_fds[0] = open(shm->db_name, O_RDWR);
		if(db_fds[0] == 0) {
			dlg_error("failed to open file %s", shm->db_name);
			return;
		}
		db_sz = dv.statbuf.st_size;
		if(db_sz < 32) {
			dlg_error("file size is less than minimal");
		}
		buf = mmap(NULL, db_sz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, db_fds[0], 0);

		if(buf == MAP_FAILED) {
			dlg_error("reopen db mmap failed");
			return;
		}
// 		idshifts = &buf[shm->gri.idshifts_off];
		reopened_buf++;
	}
	dlg_debug("kill -%d signal handled\n", signum);
}

// End Init and IPC
///////////////////////////////////////////////////////////////////

GLuint create_shader(const char *source, GLenum shader_type)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(shader_type);
	if(!shader) {
		dlg_error("glCreateShader failed");
		exit(1);
	}

	glShaderSource(shader, 1, (const char **) &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		dlg_error("Error: compiling %s: %*s\n",
			shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
			len, log);
		exit(2);
	}

	return shader;
}

void create_ui_shaders() {
	GLuint frag, vert;
	GLint status;

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
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}


void graphics_init() {
	// During init, enable OpenGL debug output
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	dlg_info("OpenGL version: %s", glGetString(GL_VERSION));

	create_ui_shaders();

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
// 	write(1,"1\n",2);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, vdata.poslen, GL_FLOAT, GL_FALSE, 0, (void*)0);
	write(1,"1\n",2);
// 	glEnableVertexAttribArray(1); // cause crash for some reason
// 	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)((intptr_t)(vertex_count*2*4)));
// 	write(1,"1\n",2);
	glBufferData(GL_ARRAY_BUFFER, vertex_count*vdata.poslen*4, vdata.positions, GL_DYNAMIC_DRAW);
// 	glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count*vdata.poslen*4, vdata.positions);
// 	vdata.coloff = vertex_count*2*4;
// 	glBufferSubData(GL_ARRAY_BUFFER, vdata.coloff, vertex_count*vdata.collen*4, vdata.colors);
	write(1,"1\n",2);
// 	glBindAttribLocation(shader_program, 0, "aPos");
// 	glBindAttribLocation(shader_program, 1, "aColor");
// 	glEnable(GL_FRAMEBUFFER_SRGB);
	uniforms.liCamPos = glGetUniformLocation(shader_program, "iCamPos");
	glUniform3f(uniforms.liCamPos, uniforms.iCamPos[0], uniforms.iCamPos[1], uniforms.iCamPos[2]);
	//glBlendFunc(GL_ONE,GL_ZERO);
	glDisable(GL_BLEND);
	//glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	//glEnable(GL_CULL_FACE);
// 	glDisable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_TEST);
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
	glUniform3f(uniforms.liCamPos, uniforms.iCamPos[0], uniforms.iCamPos[1], uniforms.iCamPos[2]);
	glBufferData(GL_ARRAY_BUFFER, vertex_count*vdata.poslen*4, vdata.positions, GL_DYNAMIC_DRAW);
// 	glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count*vdata.poslen*4, vdata.positions);
// 	vdata.coloff = vertex_count*2*4;
// 	glBufferSubData(GL_ARRAY_BUFFER, vdata.coloff, vertex_count*vdata.collen*4, vdata.colors);
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
	if(ev->button == swa_mouse_button_left) {
		if(ev->pressed) {
			shm->keys.lmb = true;
			//dlg_debug("begin resize");
			//swa_window_begin_resize(win, swa_edge_bottom_right);
			// detect intersection with UI elements, currently just node triangles
		} else
			shm->keys.lmb = false;
	}
}

static void window_mouse_move(struct swa_window* win,
		const struct swa_mouse_move_event* ev) {
// 	dlg_info("mouse moved to (%d, %d)", ev->x, ev->y);
	if(shm->keys.lmb) {
		// detect intersection with UI elements, currently just node triangles
		uniforms.iCamPos[0] += (float)ev->dx/win_dimensions.width*uniforms.iCamPos[2];
		uniforms.iCamPos[1] += -(float)ev->dy/win_dimensions.height*uniforms.iCamPos[2];
		ui_update();
	}
}

static void window_mouse_wheel(struct swa_window* win,
		float dx, float dy) {
// 	uniforms.iCamPos[0] += dx+0.00000000000000001;
	uniforms.iCamPos[2] -= dy+0.00000000000000001;
	ui_update();
}

static void window_key(struct swa_window* win, const struct swa_key_event* ev) {
	//dlg_trace("key: %d %d, utf8: %s", ev->keycode, ev->pressed, ev->utf8 ? ev->utf8 : "<null>");
	if(ev->pressed && ev->keycode == swa_key_escape) {
		dlg_info("Escape pressed, exiting");
		shm->run = false;
	}
// 	if(ev->keycode == swa_key_a) {
// 		if(ev->pressed)
// 			shm->input = inp_left;
// 		else
// 			shm->input = inp_none;
// 	}
// 	if(ev->keycode == swa_key_d) {
// 		if(ev->pressed)
// 			shm->input = inp_right;
// 		else
// 			shm->input = inp_none;
// 	}
	if(ev->keycode == swa_key_r) {
		if(ev->pressed) {
			exit_cleanup();
			exit(0);
		}
	}
	if(ev->keycode == swa_key_p) {
		if(ev->pressed & !ev->repeated) {
			dlg_info("dv.statbuf.st_size = %ld", dv.statbuf.st_size);
			for(size_t k = 0; k < db_sz; k++) {
				printf("%d ",buf[k]);
			}
			printf("\n");
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
	.mouse_move = window_mouse_move,
	.mouse_wheel = window_mouse_wheel,
	.close = window_close,
	.key = window_key,
	.surface_created = window_surface_created
};
struct swa_display* dpy;
struct swa_window* win;
int main(int argc, char** argv, char** envp) {
	signal(SIGINT, sig_handler); // Register signal handler
    signal(SIGQUIT, sig_handler); // Register signal handler
    signal(SIGBUS, sig_handler); // Register signal handler
    signal(SIGUSR1, sig_handler); // Register signal handler
    signal(SIGUSR2, sig_handler); // Register signal handler
//     signal(SIGSEGV, sig_handler); // Register signal handler
    signal(SIGTERM, sig_handler); // Register signal handler
	if(argc < 2) {
		printf("usage: graphics [SHM NAME]\n");
		return 0;
	}
	dpy = swa_display_autocreate("swa example-gl");
	if(!dpy) {
		dlg_fatal("No swa backend available");
		return EXIT_FAILURE;
	}

	glEnable(GL_MULTISAMPLE);
	glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);

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

	io_init();
	ui_init();
	graphics_init();

	dlg_debug("most likely when %ld", dv.statbuf.st_size);
	for(size_t k = 0; k < db_sz; k++) {
		printf("%d ",buf[k]);
	}
	printf("\n");

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
