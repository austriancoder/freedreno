/*
 * Copyright (c) 2011-2012 Luc Verhaegen <libv@codethink.co.uk>
 * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
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

/* Code copied from triangle_quad test from lima driver project adapted to the
 * logging that I use..
 */

#include "test-util-3d.h"

#ifndef EGL_BUFFER_AGE_EXT
#define EGL_BUFFER_AGE_EXT    0x313D
#endif

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
static EGLSurface surface;
static GLuint program;
static GLint width, height;
const char *vertex_shader_source =
	"attribute vec3 aPosition;                  \n"
	"                                           \n"
	"void main()                                \n"
	"{                                          \n"
	"    gl_Position = vec4(aPosition, 1);      \n"
	"}                                          \n";
const char *fragment_shader_source =
	"void main()                                \n"
	"{                                          \n"
	"    gl_FragColor = vec4(1.0, 0.0, 0.0, 1); \n"
	"}                                          \n";

#define TARGET_SIZE 256

/* Run through multiple variants to detect clear color, quad color (frag
 * shader param), and vertices
 */
void test_partial_update(void)
{
	RD_START("partial-update", "");
	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));

	surface = make_window(display, config, TARGET_SIZE, TARGET_SIZE);

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	DEBUG_MSG("Buffer: %dx%d", width, height);

	/* connect the context to the surface */
	ECHK(eglMakeCurrent(display, surface, surface, context));

	program = get_program(vertex_shader_source, fragment_shader_source);

	GCHK(glBindAttribLocation(program, 0, "aPosition"));

	link_program(program);

	GCHK(glViewport(0, 0, width, height));

	/* clear the color buffer */
	GCHK(glClearColor(0, 0, 0, 0));
	GCHK(glClear(GL_COLOR_BUFFER_BIT));

	GLfloat vertex[][9] = {
		{
			-1, -1, 0,
			-1, 1, 0,
			1, 1, 0,
		},
		{
			-0.5, -0.5, 0,
			0.5, -0.5, 0,
			0.5, 0.5, 0,
		},
	};


	EGLint age;
	ECHK(eglQuerySurface(display, surface, EGL_BUFFER_AGE_EXT, &age));
	printf("buffer 0 age %d\n", age);

	GLint position = glGetAttribLocation(program, "aPosition");
	GCHK(glEnableVertexAttribArray(position));
	GCHK(glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, vertex[0]));

	GCHK(glDrawArrays(GL_TRIANGLES, 0, 3));

	//ECHK(eglSwapBuffers(display, surface));
	//GCHK(glFlush());

	ECHK(eglQuerySurface(display, surface, EGL_BUFFER_AGE_EXT, &age));
	printf("buffer 1 age %d\n", age);

	static const EGLint rect[] = {
		TARGET_SIZE/4, TARGET_SIZE/4,
		TARGET_SIZE/2, TARGET_SIZE/2,
	};
	ECHK(eglSetDamageRegionKHR(display, surface, rect, 1));

	GCHK(glEnableVertexAttribArray(position));
	GCHK(glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, vertex[1]));

	GCHK(glDrawArrays(GL_TRIANGLES, 0, 3));

	ECHK(eglSwapBuffers(display, surface));
	GCHK(glFlush());

	eglTerminate(display);

	RD_END();
}

int main(int argc, char *argv[])
{
	TEST_START();
	TEST(test_partial_update());
	TEST_END();

	return 0;
}

