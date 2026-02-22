#include <cstddef>
#include <glad/glad.h>
//#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Vertex shader
const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec2 aPos;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// Fragment shader
const char* fragmentShaderSource = R"(
#version 460 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 0.5); // red
}
)";

unsigned int createShader(GLenum type, const char* shaderSource) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);

	int result;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE) {
		 int length;
		 glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		 char message[length];
		 glGetShaderInfoLog(shader, length, &length, message);
		 std::cout << "Could not compile shader: " << message << "\n";
		 glDeleteShader(shader);
		 return 0;
	}

	return shader;
}

void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

	glfwSetErrorCallback(error_callback);

    // Request OpenGL 3.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);

    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Universim 2", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

	int major, minor, rev;
	glfwGetVersion(&major, &minor, &rev);
	std::cout << "GLFW version: " << major << "." << minor << "/" << rev << "\n";

	glfwMakeContextCurrent(window);
	std::cout << "Made context\n";

	//const GLubyte* version = glGetString(GL_VERSION);
	//std::cout << "OpenGL version: " << version << std::endl;

	// GLEW does not work :(
	// Needs valid OpenGL context
	//GLenum rc = glewInit();
	//if (rc != GLEW_OK) {
	//	const unsigned char* err = glewGetErrorString(rc);
	//	std::cerr << "Failed to initialize GLEW: " << err << "\n";
	//	return -1;
	//}
	// Initialize GLAD AFTER creating context
	//int gladVersion = gladLoadGL();
	//if (gladVersion == 0) {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //// --- Compile vertex shader ---
    //GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    //glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    //glCompileShader(vertexShader);

    //// --- Compile fragment shader ---
    //GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    //glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    //glCompileShader(fragmentShader);
	GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSource);
	GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // --- Link shader program ---
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Triangle vertices (centered)
    float vertices[] = {
         0.0f,  0.5f,
        -0.5f, -0.5f,
         0.5f, -0.5f,
        -0.25f, 0.25f,
        -0.75f, -0.75f,
         0.25f, -0.75f
    };

    // Upload vertex data
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
		glDrawArrays(GL_TRIANGLES, 3, 3);

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

