/*
 * =====================================================================================
 *
 *       Filename:  drawing.cpp
 *
 *    Description:  https://open.gl/drawing
 *
 *        Version:  1.0
 *        Created:  08/17/2015 23:30:28
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Yu Sun
 *
 * =====================================================================================
 */

#include "glfw_app.hpp"
#include "gl_shader.hpp"
#include "gl_macros.hpp"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <vector>

using namespace playground;
using namespace std;

/*-----------------------------------------------------------------------------
 *  SHADER CODE
 *-----------------------------------------------------------------------------*/
const char * vert = GLSL(120,
                         
    attribute vec2 position;

    void main(){
        gl_Position = vec4(position.x, -position.y, 0.0, 1.0);
    }
);

const char * frag = GLSL(120,
                         
    uniform vec3 triangleColor;

    void main(){
        gl_FragColor = vec4(1 - triangleColor.r, 1 - triangleColor.g, 1 - triangleColor.b, 1.0);
    }
);

/*-----------------------------------------------------------------------------
 *  OUR APP
 *-----------------------------------------------------------------------------*/
struct MyApp : public App{
    
    Shader * shader;
    
    GLuint arrayID;

    MyApp() { init(); }

    void init(){
        shader = new Shader( vert, frag );
        
        float vertices[] = {
            0.0f,  0.5f, // Vertex 1 (X, Y)
            0.5f, -0.5f, // Vertex 2 (X, Y)
            -0.5f, -0.5f  // Vertex 3 (X, Y)
        };
        
        GLuint elements[] = {
            0, 1, 2
        };
        
        GLuint vao;
        glGenVertexArrays(1, &vao);
        BINDVERTEXARRAY(vao);
        arrayID = vao;
        
        GLuint vbo;
        glGenBuffers(1, &vbo); // Generate 1 buffer
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        GLint posAttrib = glGetAttribLocation(shader->id(), "position");
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(posAttrib);
        
        GLuint ebo;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
        
//        GLint uniColor = glGetUniformLocation(shader->id(), "triangleColor");
//        glUniform3f(uniColor, 1.0f, 0.0f, 0.0f);
        
        BINDVERTEXARRAY(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        shader->unbind();
    }

    void onDraw(){
        static float time = 0.0;
        time += .01;
        
        shader->bind();
        BINDVERTEXARRAY(arrayID);
        
        GLint uniColor = glGetUniformLocation(shader->id(), "triangleColor");
        glUniform3f(uniColor, (sin(time * 4.0f) + 1.0f) / 2.0f, 0.0f, 0.0f);
        
//        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        
        BINDVERTEXARRAY(0);
    }
};


int main(int argc, const char ** argv){
 
    MyApp app;
    app.start();
       
    return 0;
}



