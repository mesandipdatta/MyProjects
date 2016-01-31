/*
 * =====================================================================================
 *
 *       Filename:  drawing.cpp
 *
 *    Description:  Test for OpenGL multithreading
 *
 *        Version:  1.0
 *        Created:  08/22/2015 01:25:52
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
//#include <thread>
#include <pthread.h>

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
        gl_FragColor = vec4(triangleColor, 1.0);
    }
);

Shader * shader;
GLuint arrayID;

void *drawTriangle(void *) {

//    shader1->bind();
    cout << "drawTriangle" << endl;
}

/*-----------------------------------------------------------------------------
 *  OUR APP
 *-----------------------------------------------------------------------------*/
struct MyApp : public App{

    MyApp() { init(); }

    void init(){
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
        glUniform3f(uniColor, 1.0f, 0.0f, 0.0f);
        
//        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        
        BINDVERTEXARRAY(0);
    }
    
    /*-----------------------------------------------------------------------------
     *  Start the Draw Loop
     *-----------------------------------------------------------------------------*/
    void start(){
        printf("starting app\n");
        
        pthread_t t1, t2;
        
        pthread_create(&t1, NULL, drawTriangle, NULL);
//        pthread_create(&t2, NULL, drawTriangle, NULL);
        
        pthread_join(t1, NULL);
//        pthread_join(t2, NULL);

//        while ( !mWindow.shouldClose() ){
//        
//            mWindow.setViewport();
//            
//            glClearColor(.2,.2,.2,1);
//            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//            
//            onDraw();
//            
//            mWindow.swapBuffers();      //<-- SWAP BUFFERS
//            glfwPollEvents();           //<-- LISTEN FOR WINDOW EVENTS
//            
//        }
    }
};


int main(int argc, const char ** argv){
 
    MyApp app;
    app.start();
       
    return 0;
}



