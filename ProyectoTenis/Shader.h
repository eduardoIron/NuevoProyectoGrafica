// ============================================================================
//  Shader.h  -  Compilación y uso de programas GLSL (vertex + fragment)
//  Basado en la clase Shader de las prácticas del laboratorio, ampliado con
//  caché de uniformes y setters tipados.
// ============================================================================
#pragma once

#include <GL/glew.h>
#include "Utils.h"
#include <string>
#include <unordered_map>

class Shader
{
public:
    GLuint Program = 0;

    // Carga y compila los archivos. Devuelve false si falla (y escribe el log en consola).
    bool load(const std::string& vertexPath, const std::string& fragmentPath);

    void use() const { glUseProgram(Program); }

    GLint loc(const std::string& name) const;

    void setInt(const std::string& n, int v) const            { glUniform1i(loc(n), v); }
    void setBool(const std::string& n, bool v) const          { glUniform1i(loc(n), v ? 1 : 0); }
    void setFloat(const std::string& n, float v) const        { glUniform1f(loc(n), v); }
    void setVec2(const std::string& n, const glm::vec2& v) const { glUniform2fv(loc(n), 1, glm::value_ptr(v)); }
    void setVec3(const std::string& n, const glm::vec3& v) const { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
    void setVec4(const std::string& n, const glm::vec4& v) const { glUniform4fv(loc(n), 1, glm::value_ptr(v)); }
    void setMat3(const std::string& n, const glm::mat3& m) const { glUniformMatrix3fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }
    void setMat4(const std::string& n, const glm::mat4& m) const { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }

    // Elemento i de un arreglo de uniformes: "nombre[i]"
    void setVec2At(const std::string& n, int i, const glm::vec2& v) const { setVec2(n + "[" + std::to_string(i) + "]", v); }
    void setVec3At(const std::string& n, int i, const glm::vec3& v) const { setVec3(n + "[" + std::to_string(i) + "]", v); }

private:
    mutable std::unordered_map<std::string, GLint> cache_;
};
