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

#include <GLES3/gl3.h>
#include "test-util-3d.h"

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
		"#version 300 es              \n"
		"\n"
		"in float inValue;            \n"
		"out float outValue;          \n"
		"\n"
		"void main()                  \n"
		"{                            \n"
		"    outValue = sqrt(inValue);\n"
		"} \n";

const char *fragment_shader_source =
		"#version 300 es              \n"
		"precision highp float;       \n"
		"out vec4 FragColor;          \n"
		"void main() {                \n"
		"   FragColor = vec4(1, 0, 0, 0);\n"
		"} \n";

static void test_transform_feedback_compute()
{
	EGLSurface surface;
	GLint width, height;

	const char *varyings[] = {
		"outValue"
	};

	RD_START("transform-feedback-compute", "");

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

	GCHK(glTransformFeedbackVaryings(program, 1, varyings, GL_SEPARATE_ATTRIBS));

	link_program(program);

	// Create VAO
	GLuint vao;
	GCHK(glGenVertexArrays(1, &vao));
	GCHK(glBindVertexArray(vao));

	// Create input VBO and vertex format
	GLfloat data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 16.0f };

	GLuint vbo;
	GCHK(glGenBuffers(1, &vbo));
	GCHK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
	GCHK(glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW));

	GLint inputAttrib = glGetAttribLocation(program, "inValue");
	GCHK(glEnableVertexAttribArray(inputAttrib));
	GCHK(glVertexAttribPointer(inputAttrib, 1, GL_FLOAT, GL_FALSE, 0, 0));

	// Create transform feedback buffer
	GLuint tbo;
	GCHK(glGenBuffers(1, &tbo));
	GCHK(glBindBuffer(GL_ARRAY_BUFFER, tbo));
	GCHK(glBufferData(GL_ARRAY_BUFFER, sizeof(data), NULL, GL_STATIC_READ));

	// Perform feedback transform
	GCHK(glEnable(GL_RASTERIZER_DISCARD));

	GCHK(glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo));

	GCHK(glBeginTransformFeedback(GL_POINTS));
		GCHK(glDrawArrays(GL_POINTS, 0, 5));
	GCHK(glEndTransformFeedback());

	GCHK(glDisable(GL_RASTERIZER_DISCARD));

	GCHK(glFlush());

	// Fetch and print results
	float *buffer = glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(float) * 5, GL_MAP_READ_BIT);

	if (!buffer) {
		printf("%f %f %f %f %f\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
		GCHK(glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER));
	}

	GCHK(glDeleteProgram(program));

	GCHK(glDeleteBuffers(1, &tbo));
	GCHK(glDeleteBuffers(1, &vbo));

	GCHK(glDeleteVertexArrays(1, &vao));

	ECHK(eglDestroySurface(display, surface));

	ECHK(eglTerminate(display));

	RD_END();
}

int main(int argc, char *argv[])
{
	TEST_START();
	TEST(test_transform_feedback_compute());
	TEST_END();

	return 0;
}

