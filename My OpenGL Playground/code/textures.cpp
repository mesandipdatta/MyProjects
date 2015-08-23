/*
 * =====================================================================================
 *
 *       Filename:  textures.cpp
 *
 *    Description:  https://open.gl/textures
 *
 *        Version:  1.0
 *        Created:  08/18/2015 23:30:28
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
const char * vert =
GLSL(120,
                         
     attribute vec2 position;
     attribute vec3 color;
     attribute vec2 texcoord;
     
     varying vec2 Texcoord;
     varying vec3 Color;
     
     uniform mat4 model;
     uniform mat4 view;
     uniform mat4 proj;

     void main(){
         gl_Position = proj * view * model * vec4(position.x, -position.y, 0.0, 1.0);
         Color = color;
         Texcoord = texcoord;
     }
);

const char * frag =
GLSL(120,
     
     varying vec2 Texcoord;
     varying vec3 Color;
     
     uniform sampler2D texFlower;
//     uniform sampler2D texBox;
     uniform float time;
                         
     void main(){
         float factor = (sin(time * 3.0) + 1.0) / 2.0;
//         vec4 colFlower = texture2D(texFlower, Texcoord);
//         vec4 colBox = texture2D(texBox, Texcoord);
//         if (Texcoord.y > 0.5) {
//             gl_FragColor = texture2D(texFlower, Texcoord);
//         } else {
//             gl_FragColor = texture2D(texFlower, vec2(Texcoord.x + sin(Texcoord.y * 60.0 + time) / 30.0, 1.0 - Texcoord.y));
//         }
         gl_FragColor = texture2D(texFlower, Texcoord);
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
            //  Position      Color             Texcoords
            -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, // Top-left
             0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f, // Top-right
             0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f, // Bottom-right
            -0.5f, -0.5f,  1.0f,  1.0f,  1.0f,  0.0f,  1.0f  // Bottom-left
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
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), 0);
        glEnableVertexAttribArray(posAttrib);
        
        GLint colAttrib = glGetAttribLocation(shader->id(), "color");
        glEnableVertexAttribArray(colAttrib);
        glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(2*sizeof(float)));
        
        GLint texAttrib = glGetAttribLocation(shader->id(), "texcoord");
        glEnableVertexAttribArray(texAttrib);
        glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(5*sizeof(float)));
        
        GLuint textures[2];
        glGenTextures(2, textures);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        Bitmap img("resources/flower.bmp");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.pixels.data());
        // specifying the texture unit
        glUniform1i(glGetUniformLocation(shader->id(), "texFlower"), 0);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        
//        glm::mat4 trans;
//        trans = glm::rotate(trans, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//        GLint uniTrans = glGetUniformLocation(shader->id(), "trans");
//        glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(trans));
        
//        glm::vec4 result = trans * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
//        printf("%f, %f, %f\n", result.x, result.y, result.z);
        
        glm::mat4 view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        GLint uniView = glGetUniformLocation(shader->id(), "view");
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
        
//        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 10.0f);
//        GLint uniProj = glGetUniformLocation(shader->id(), "proj");
//        glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
        
//        glActiveTexture(GL_TEXTURE1);
//        glBindTexture(GL_TEXTURE_2D, textures[1]);
//        Bitmap img2("resources/box.bmp");
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img2.width, img2.height, 0, GL_RGB, GL_UNSIGNED_BYTE, img2.pixels.data());
//        glUniform1i(glGetUniformLocation(shader->id(), "texBox"), 1);
//        
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//        glGenerateMipmap(GL_TEXTURE_2D);
        
        BINDVERTEXARRAY(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        shader->unbind();
    }
    
    void onDraw(){
        static float time = 0.0;
        time += .01;
        
        shader->bind();
        BINDVERTEXARRAY(arrayID);
        
        glm::mat4 model;
        model = glm::rotate(model, glm::radians(180.0f) * time, glm::vec3(1.0f, 0.0f, 0.0f));
        GLint uniTrans = glGetUniformLocation(shader->id(), "model");
        glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));
        
        glm::mat4 proj = glm::perspective(glm::radians(45.0f) * time, 800.0f / 600.0f, 0.1f, 10.0f);
        GLint uniProj = glGetUniformLocation(shader->id(), "proj");
        glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
        
        GLint uniTime = glGetUniformLocation(shader->id(), "time");
        glUniform1f(uniTime, time);
        
        glDrawArrays( GL_QUADS, 0, 4);
        
        BINDVERTEXARRAY(0);
    }
};


int main(int argc, const char ** argv){
    
    MyApp app;
    app.start();
    
    return 0;
}



