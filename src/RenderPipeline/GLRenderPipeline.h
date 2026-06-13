#pragma once
#include <glad/gl.h>
#include <stdio.h>

// WARN these are here temporarily. idk how to store and handle these yet
const GLchar *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const GLchar *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n";

void CreateVertexArray(GLuint *vertexArrayObject)
{
    glGenVertexArrays(1, vertexArrayObject);
    glBindVertexArray(*vertexArrayObject);
}

void CreateVertexBuffer(float vertices[], GLuint *vertexBufferObject)
{
    glGenBuffers(1, vertexBufferObject);
    // copy vertices into buffer
    glBindBuffer(GL_ARRAY_BUFFER, *vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(*vertices), vertices, GL_STATIC_DRAW);
    // set vertex attributes
    // 0 means layout location 0 in the shader program. I.E our vertex data
    // 3 is vector 3, as our vertecies only have 3 points.
    // GL_FALSE specifies we do not want out data to be normalized. check documentation for setting this to true
    // 3 * sizeof(float) is the stride amount. its how far away in memory the next vertex lives. I.E the 2nd vertex lives 3 floats away as each vertx is a float 3
    // 0 cast to (void*) is the offset of where the position data begins in the buffer.
    // # NOTE this function uses the currently bound VertexBufferObject to GL_ARRAY_BUFFER.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
}

int CompileShaderFromSource(const GLchar *shaderSource, GLenum ShaderType, GLuint *compiledShader)
{
    *compiledShader = glCreateShader(ShaderType);
    glShaderSource(*compiledShader, 1, &shaderSource, NULL);
    glCompileShader(*compiledShader);

    int success;
    char infoLog[512];
    glGetShaderiv(*compiledShader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(*compiledShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::%u::COMPILATION_FAILED\n%s", ShaderType, infoLog);
    }

    return success;
}

int CreateShaderProgram(GLuint *vertexShader, GLuint *fragmentShader, GLuint *shaderProgram)
{
    *shaderProgram = glCreateProgram();
    glAttachShader(*shaderProgram, *vertexShader);
    glAttachShader(*shaderProgram, *fragmentShader);
    glLinkProgram(*shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(*shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(*shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::CREATION_FAILED\n%s", infoLog);

        return success;
    }
    return success;
}

int CompileShaderProgram(const GLchar *vertexProgramSource, const GLchar *fragmentProgramSource, GLuint *shaderProgram)
{
    int success;
    GLuint vertexProgram;
    success = CompileShaderFromSource(vertexProgramSource, GL_VERTEX_SHADER, &vertexProgram);
    if (!success) {
        return success;
    }

    GLuint fragmentProgram;
    success = CompileShaderFromSource(fragmentProgramSource, GL_FRAGMENT_SHADER, &fragmentProgram);
    if (!success) {
        return success;
    }

    success = CreateShaderProgram(&vertexProgram, &fragmentProgram, shaderProgram);

    glDeleteShader(vertexProgram);
    glDeleteShader(fragmentProgram);
    return success;
}

void RenderMesh(GLuint *shaderProgram, GLuint *vertexArray)
{
    glUseProgram(*shaderProgram);
    glBindVertexArray(*vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // printf("draw the mesh now\n");
}
