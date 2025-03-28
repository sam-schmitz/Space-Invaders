/* main.cpp */
/* By: Sam Schmitz */
/* Renders the window used for the game. */

#include <cstdio>
#include <cstdint>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void validate_shader(GLuint shader, const char* file = nullptr)
{
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success); // Check if the shader compiled successfully

	if (!success) {
		static const unsigned int BUFFER_SIZE = 512;
		char buffer[BUFFER_SIZE];
		GLsizei length = 0;
		glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer); // Retrieve error log

		printf("Shader %d (%s) compile error: %s\n", shader, (file ? file : "Unnamed Shader"), buffer);
	}
}

bool validate_program(GLuint program)
{
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		static const GLsizei BUFFER_SIZE = 512;
		GLchar buffer[BUFFER_SIZE];
		GLsizei length = 0;
		glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);
		printf("Program %d link error: %s\n", program, buffer);
		return false;
	}

	return true;
}

/* A chunk of memory on the CPU that stores the pixel values for the graphics. */
struct Buffer
{
	size_t width, height;
	uint32_t* data;
};

struct Sprite
{
	size_t width, height;
	uint8_t* data;
};

struct Alien
{
	size_t x, y;
	uint8_t type;
};

struct Player
{
	size_t x, y;
	size_t life;
};

struct Game
{
	size_t width, height;
	size_t num_aliens;
	Alien* aliens;
	Player player;
};

struct SpriteAnimation
{
	bool loop;
	size_t num_frames;
	size_t frame_duration;
	size_t time;
	Sprite** frames;
};

/* used to represent the colors of a pixel. Left most 8 bits are red, then green, then blue. With the last 8 bits set at 255. */
uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b)
{
	return (r << 24) | (g << 16) | (b << 8) | 255;
}

/* Iterates over all the pixels in buffer and sets them to the given color. */
void buffer_clear(Buffer* buffer, uint32_t color)
{
	for (size_t i = 0; i < buffer->width * buffer->height; ++i)
	{
		buffer->data[i] = color;
	}
}

void buffer_draw_sprite(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color)
{
	for (size_t xi = 0; xi < sprite.width; ++xi)
	{
		for (size_t yi = 0; yi < sprite.height; ++yi)
		{
			if (sprite.data[yi * sprite.width + xi] &&
				(sprite.height - 1 + y - yi) < buffer->height &&
				(x + xi) < buffer->width)
			{
				buffer->data[(sprite.height - 1 + y - yi) * buffer->width + (x + xi)] = color;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	const size_t buffer_width = 224;
	const size_t buffer_height = 256;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
	{
		return -1;
	}

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(buffer_width, buffer_height, "Space Invaders", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		fprintf(stderr, "Error initializing GLEW. \n");
		glfwTerminate();
		return -1; 
	}
	int glVersion[2] = { -1, 1 };
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

	printf("Using OpenGL: %d.%d\n", glVersion[0], glVersion[1]);
	printf("Renderer used: %s\n", glGetString(GL_RENDERER));
	printf("Shading Language: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	glClearColor(1.0, 0.0, 0.0, 1.0);

	Buffer buffer;
	buffer.width = buffer_width;
	buffer.height = buffer_height;
	buffer.data = new uint32_t[buffer.width * buffer.height];

	buffer_clear(&buffer, 0);

	GLuint buffer_texture;
	glGenTextures(1, &buffer_texture);

	glBindTexture(GL_TEXTURE_2D, buffer_texture);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RGB8,
		buffer.width, buffer.height, 0,
		GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLuint fullscreen_triangle_vao;
	glGenVertexArrays(1, &fullscreen_triangle_vao);

	static const char* vertex_shader =
		"\n"
		"#version 330\n"
		"\n"
		"noperspective out vec2 TexCoord;\n"
		"void main(void){\n"
		"\n"
		"	TexCoord.x = (gl_VertexID == 2)? 2.0: 0.0;\n"
		"	TexCoord.y = (gl_VertexID == 1)? 2.0: 0.0;\n"
		"	\n"
		"	gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
		"}\n";

	static const char* fragment_shader =
		"\n"
		"#version 330\n"
		"\n"
		"uniform sampler2D screenTexture;\n" // Renamed from 'buffer'
		"noperspective in vec2 TexCoord;\n"
		"\n"
		"out vec3 outColor;\n"
		"\n"
		"void main(void){\n"
		"   outColor = texture(screenTexture, TexCoord).rgb;\n"
		"}\n";

	GLuint shader_id = glCreateProgram();
	GLint success;

	// Create vertex shader
	GLuint shader_vp = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader_vp, 1, &vertex_shader, 0);
	glCompileShader(shader_vp);
	glGetShaderiv(shader_vp, GL_COMPILE_STATUS, &success);

	if (!success) {
		validate_shader(shader_vp, "Vertex Shader");
		glDeleteShader(shader_vp);
		return -1;
	}

	glAttachShader(shader_id, shader_vp);
	glDeleteShader(shader_vp);

	// Create fragment shader
	GLuint shader_fp = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader_fp, 1, &fragment_shader, 0);
	glCompileShader(shader_fp);
	glGetShaderiv(shader_fp, GL_COMPILE_STATUS, &success);

	if (!success) {
		validate_shader(shader_fp, "Fragment Shader");
		glDeleteShader(shader_fp);
		return -1;
	}

	glAttachShader(shader_id, shader_fp);
	glDeleteShader(shader_fp);

	glLinkProgram(shader_id);

	if (!validate_program(shader_id))
	{
		fprintf(stderr, "Error while validating shader.\n");
		glfwTerminate();
		glDeleteVertexArrays(1, &fullscreen_triangle_vao);
		delete[] buffer.data;
		return -1;
	}

	glUseProgram(shader_id);

	GLint location = glGetUniformLocation(shader_id, "screenTexture");
	glUniform1i(location, 0);

	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, buffer_texture);

	glBindVertexArray(fullscreen_triangle_vao);

	//Prepare the game
	Sprite alien_sprite0;
	alien_sprite0.width = 11;
	alien_sprite0.height = 8;
	alien_sprite0.data = new uint8_t[88]
	{
		0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
		0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
		0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
		0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
		1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
		0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
	};

	Sprite alien_sprite1;
	alien_sprite1.width = 11;
	alien_sprite1.height = 8;
	alien_sprite1.data = new uint8_t[88]
	{
		0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
	1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
	1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
	1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
	1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
	0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
	0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
	0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
	};

	Sprite player_sprite;
	player_sprite.width = 11;
	player_sprite.height = 7;
	player_sprite.data = new uint8_t[77]
	{
		0,0,0,0,0,1,0,0,0,0,0, // .....@.....
		0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
		0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
		0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
		1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
	};

	SpriteAnimation* alien_animation = new SpriteAnimation;

	alien_animation->loop = true;
	alien_animation->num_frames = 2;
	alien_animation->frame_duration = 10;
	alien_animation->time = 0;

	alien_animation->frames = new Sprite * [2];
	alien_animation->frames[0] = &alien_sprite0;
	alien_animation->frames[1] = &alien_sprite1;

	Game game;
	game.width = buffer_width;
	game.height = buffer_height;
	game.num_aliens = 55;
	game.aliens = new Alien[game.num_aliens];

	game.player.x = 112 - 5;
	game.player.y = 32;

	game.player.life = 3;

	for (size_t yi = 0; yi < 5; ++yi)
	{
		for (size_t xi = 0; xi < 11; ++xi)
		{
			game.aliens[yi * 11 + xi].x = 16 * xi + 20;
			game.aliens[yi * 11 + xi].y = 17 * yi + 128;
		}
	}	

	uint32_t clear_color = rgb_to_uint32(0, 128, 0);

	while (!glfwWindowShouldClose(window))
	{
		buffer_clear(&buffer, clear_color);

		for (size_t ai = 0; ai < game.num_aliens; ++ai)
		{
			const Alien& alien = game.aliens[ai];
			size_t current_frame = alien_animation->time / alien_animation->frame_duration;
			const Sprite& sprite = *alien_animation->frames[current_frame];
			buffer_draw_sprite(&buffer, sprite,
				alien.x, alien.y, rgb_to_uint32(128, 0, 0));
		}

		buffer_draw_sprite(&buffer, player_sprite, game.player.x, game.player.y, rgb_to_uint32(128, 0, 0));

		++alien_animation->time;
		if (alien_animation->time == alien_animation->num_frames * alien_animation->frame_duration)
		{
			if (alien_animation->loop) alien_animation->time = 0;
			else
			{
				delete alien_animation;
				alien_animation = nullptr;
			}
		}

		glTexSubImage2D(
			GL_TEXTURE_2D, 0, 0, 0,
			buffer.width, buffer.height,
			GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
			buffer.data
		);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();

	glDeleteVertexArrays(1, &fullscreen_triangle_vao);

	delete[] alien_sprite0.data;
	delete[] alien_sprite1.data;
	delete[] alien_animation->frames;
	delete[] buffer.data;
	delete[] game.aliens;

	delete alien_animation;

	return 0;
}

