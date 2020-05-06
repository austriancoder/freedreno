/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
 * Copyright (c) 2012 Jonathan Maw <jonathan.maw@codethink.co.uk>
 * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
 * Copyright (c) 2014 Ilia Mirkin <imirkin@alum.mit.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Code based on cube_textured test from lima driver project (Jonathan Maw)
 * adapted to the logging that I use..
 */

#include <GLES3/gl32.h>
#include "test-util-3d.h"

#include "cubetex.h"

static EGLint const config_attribute_list[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_DEPTH_SIZE, 8,
		EGL_NONE
};

static const EGLint context_attribute_list[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
};

static EGLDisplay display;
static EGLConfig config;
static EGLint num_config;
static EGLContext context;
static GLuint program;
const char *vertex_shader_source =
		"attribute vec3 point;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 UV;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(point, 1);\n"
		"  UV = texcoord;\n"
		"}\n";

const char *fragment_shader_source =
		"precision mediump float;\n"
		"varying vec2 UV;\n"
		"uniform sampler2D tex;\n"
		"\n"
		"void main()\n"
		"{\n"
		"  if (UV.y > 0.75 && UV.y < 0.8) {\n"
		"    discard;\n"
		"  }\n"
		"  gl_FragColor = texture2D(tex, UV).rgba;\n"
		"}\n";

static void test_draw(void)
{
	GLint width, height, ret;
	GLfloat vVertices[] = {
		 0.5f,  0.5f,  -1.0f,   1.0f, 1.0f,
		-0.5f,  0.5f,  -1.0f,   0.0f, 1.0f,
		-0.5f, -0.5f,  -1.0f,   0.0f, 0.0f,

		 0.7f,  0.9f,  -0.5f,   1.0f, 1.0f,
		-0.3f,  0.9f,  -0.5f,   0.0f, 1.0f,
		-0.3f, -0.1f,  -0.5f,   0.0f, 0.0f,
	}; //              `-- Z
	unsigned int indices[] = { 0, 1, 2, 3, 4, 5 };

	float pixels[] = {
		1.0f, 0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f
	};

	GLuint vbo, vao, tex, idx;

	EGLSurface surface;

	RD_START("draw-discard" ,"");

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));

	surface = make_window(display, config, 255, 255);

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	DEBUG_MSG("Buffer: %dx%d", width, height);

	/* connect the context to the surface */
	ECHK(eglMakeCurrent(display, surface, surface, context));

	printf("EGL Version %s\n", eglQueryString(display, EGL_VERSION));
	printf("EGL Vendor %s\n", eglQueryString(display, EGL_VENDOR));
	printf("EGL Extensions %s\n", eglQueryString(display, EGL_EXTENSIONS));
	printf("GL Version %s\n", glGetString(GL_VERSION));
	printf("GL extensions: %s\n", glGetString(GL_EXTENSIONS));

	program = get_program(vertex_shader_source, fragment_shader_source);

	link_program(program);

	GCHK(glGenVertexArrays(1, &vao));
	GCHK(glBindVertexArray(vao));

	GCHK(glGenBuffers(1, &vbo));
	GCHK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	GCHK(glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW));

	GCHK(glGenBuffers(1, &idx));
	GCHK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx));
	GCHK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

	GCHK(glVertexAttribPointer(glGetAttribLocation(program, "point"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0));
	GCHK(glVertexAttribPointer(glGetAttribLocation(program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float))));

	GCHK(glEnableVertexAttribArray(0));
	GCHK(glEnableVertexAttribArray(1));

	GCHK(glGenTextures(1, &tex));
	GCHK(glActiveTexture(GL_TEXTURE0));
	GCHK(glBindTexture(GL_TEXTURE_2D, tex));
	GCHK(glUniform1i(glGetUniformLocation(program, "tex"), 0));
	GCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_FLOAT, pixels));
 	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
 	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
 	GCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	//GCHK(glGenerateMipmap(GL_TEXTURE_2D));

	GCHK(glViewport(0, 0, width, height));

	GCHK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
	GCHK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)); 

	GCHK(glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void *)(3 * sizeof(unsigned int))));
	GCHK(glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void *)0));

	GCHK(glFlush());
	ECHK(eglSwapBuffers(display, surface));

	sleep(100000);

	ECHK(eglDestroySurface(display, surface));
	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	TEST_START();
	TEST(test_draw());
	TEST_END();

	return 0;
}

