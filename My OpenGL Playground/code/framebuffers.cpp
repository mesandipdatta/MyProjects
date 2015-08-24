/*
 * =====================================================================================
 *
 *       Filename:  framebuffers.cpp
 *
 *    Description:  https://open.gl/framebuffers
 *
 *        Version:  1.0
 *        Created:  08/23/2015 22:45:42
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
#include "gl_data.hpp"                                //<-- bitmap loader

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
const char * vert3d =
GLSL(120,
                         
     attribute vec3 position;
     attribute vec3 color;
     attribute vec2 texcoord;
     
     varying vec2 Texcoord;
     varying vec3 Color;
     
     uniform mat4 model;
     uniform mat4 view;
     uniform mat4 proj;

     void main(){
         gl_Position = proj * view * model * vec4(position, 1.0);
         Color = color;
         Texcoord = texcoord;
     }
);

const char * frag3d =
GLSL(120,
     
     varying vec2 Texcoord;
     varying vec3 Color;
     
     uniform sampler2D texFlower;
                         
     void main(){
         gl_FragColor = texture2D(texFlower, Texcoord);
     }
);

const char * vert2d =
GLSL(120,
     
     attribute vec2 position;
     attribute vec2 texcoord;
     
     varying vec2 Texcoord;
     
     void main(){
         Texcoord = texcoord;
         gl_Position = vec4(position, 0.0, 1.0);
     }
);

const char * frag2d =
GLSL(120,
     
     varying vec2 Texcoord;
     
     uniform sampler2D texFramebuffer;
     
     void main(){
         gl_FragColor = texture2D(texFramebuffer, Texcoord);
     }
);

// Cube vertices
GLfloat vertices3d[] = {
    -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    
    -0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    
    -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    
    0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    
    -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    
    -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f
};

// Quad vertices
GLfloat vertices2d[] = {
    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f,  1.0f,  1.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
    
     1.0f, -1.0f,  1.0f, 0.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
    -1.0f,  1.0f,  0.0f, 1.0f
};

/*-----------------------------------------------------------------------------
 *  OUR APP
 *-----------------------------------------------------------------------------*/
struct MyApp : public App{
    
    Shader * shader3d, * shader2d;
    
    GLuint vao3d, vao2d;
    
    GLuint texture3d;
    
    MyApp() { init(); }
    
    void init() {
        // create shader program
        shader3d = new Shader(vert3d, frag3d);
        
        // create vertex array
        glGenVertexArrays(1, &vao3d);
        BINDVERTEXARRAY(vao3d);
        
        // create buffer and send data
        GLuint vbo3d;
        glGenBuffers(1, &vbo3d);
        glBindBuffer(GL_ARRAY_BUFFER, vbo3d);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices3d), vertices3d, GL_STATIC_DRAW);
        
        // tell GPU how to use data
        GLint posAttrib = glGetAttribLocation(shader3d->id(), "position");
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(posAttrib);
        
        GLint colAttrib = glGetAttribLocation(shader3d->id(), "color");
        glEnableVertexAttribArray(colAttrib);
        glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
        
        GLint texAttrib = glGetAttribLocation(shader3d->id(), "texcoord");
        glEnableVertexAttribArray(texAttrib);
        glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
        
        // load texture
        glGenTextures(1, &texture3d);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture3d);
        Bitmap img("resources/flower.bmp");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.pixels.data());
        // specifying the texture unit
        glUniform1i(glGetUniformLocation(shader3d->id(), "texFlower"), 0);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // set view and proj matrix
        glm::mat4 view = glm::lookAt(glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        GLint uniView = glGetUniformLocation(shader3d->id(), "view");
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
        
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 1.0f, 10.0f);
        GLint uniProj = glGetUniformLocation(shader3d->id(), "proj");
        glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
        
        BINDVERTEXARRAY(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
    }
    
    void onDraw() {
        static float time = 0.0;
        time += .01;
        
        shader3d->bind();
        glm::mat4 model;
        model = glm::rotate(model, glm::radians(180.0f) * time, glm::vec3(0.0f, 0.0f, 1.0f));
        GLint uniModel = glGetUniformLocation(shader3d->id(), "model");
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        BINDVERTEXARRAY(0);
        glUseProgram(0);
    }
};


int main(int argc, const char ** argv){
    
    MyApp app;
    app.start();
    
    return 0;
}



