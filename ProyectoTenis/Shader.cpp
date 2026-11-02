#include "Shader.h"
#include <fstream>
#include <iostream>
#include <sstream>

static bool readFile(const std::string& path, std::string& out)
{
    std::ifstream f(path.c_str(), std::ios::in | std::ios::binary);
    if (!f.is_open()) return false;
    std::stringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    // quitar BOM UTF-8 si existe (algunos drivers no lo toleran)
    if (out.size() >= 3 && static_cast<unsigned char>(out[0]) == 0xEF && static_cast<unsigned char>(out[1]) == 0xBB && static_cast<unsigned char>(out[2]) == 0xBF)
        out.erase(0, 3);
    return true;
}

static GLuint compile(GLenum type, const std::string& src, const std::string& name)
{
    GLuint s = glCreateShader(type);
    const GLchar* code = src.c_str();
    glShaderSource(s, 1, &code, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLchar log[2048];
        glGetShaderInfoLog(s, 2048, NULL, log);
        std::cout << "ERROR::SHADER::COMPILACION (" << name << ")\n" << log << std::endl;
    }
    return ok ? s : 0;
}

bool Shader::load(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vs, fs;
    if (!readFile(vertexPath, vs))   { std::cout << "ERROR::SHADER: no se pudo abrir " << vertexPath << std::endl; return false; }
    if (!readFile(fragmentPath, fs)) { std::cout << "ERROR::SHADER: no se pudo abrir " << fragmentPath << std::endl; return false; }

    GLuint v = compile(GL_VERTEX_SHADER, vs, vertexPath);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs, fragmentPath);
    if (!v || !f) return false;

    Program = glCreateProgram();
    glAttachShader(Program, v);
    glAttachShader(Program, f);
    glLinkProgram(Program);
    GLint ok = 0;
    glGetProgramiv(Program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLchar log[2048];
        glGetProgramInfoLog(Program, 2048, NULL, log);
        std::cout << "ERROR::SHADER::LINK (" << vertexPath << ")\n" << log << std::endl;
        return false;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    cache_.clear();
    return true;
}

GLint Shader::loc(const std::string& name) const
{
    std::unordered_map<std::string, GLint>::const_iterator it = cache_.find(name);
    if (it != cache_.end()) return it->second;
    GLint l = glGetUniformLocation(Program, name.c_str());
    cache_[name] = l;
    return l;
}
