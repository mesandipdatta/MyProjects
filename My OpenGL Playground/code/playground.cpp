/*
 * =====================================================================================
 *
 *       Filename:  playground.cpp
 *
 *    Description:  My playgound for OpenGL
 *
 *        Version:  1.0
 *        Created:  08/05/2015 18:43:34
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
                         
    attribute vec4 position;
                         
    void main(){
        gl_Position = position;
    }
);

const char * frag = GLSL(120,
                         
    void main(){
        gl_FragColor = vec4(1.0,1.0,1.0,1.0);
    }
);

/*-----------------------------------------------------------------------------
 *  OUR APP
 *-----------------------------------------------------------------------------*/
struct MyApp : public App{
    
    Shader * shader;
    
    //ID of Vertex Attribute
    GLuint positionID;
    
    //A buffer ID
    GLuint vertexBufferID, indexBufferID;
    
    //An array ID
    GLuint arrayID;

    //Constructor (initialize application)
    MyApp() { init(); }

    void init(){
        
        GLfloat verts[] =
        {
            +0.0f, +0.0f,   // 0
            +1.0, +0.0, +0.0,
            +1.0f, +1.0f,   // 1
            +1.0, +0.0, +0.0,
            -1.0f, +1.0f,   // 2
            +1.0, +0.0, +0.0,
//            +0.0f, +0.0f,
            -1.0f, -1.0f,   // 3
            +1.0, +0.0, +0.0,
            +1.0f, -1.0f,   // 4
            +1.0, +0.0, +0.0,
        };
        
        /*-----------------------------------------------------------------------------
         *  CREATE THE SHADER
         *-----------------------------------------------------------------------------*/
        shader = new Shader( vert, frag );
        // Get attribute locations
        positionID = glGetAttribLocation(shader -> id(), "position");
        
        /*-----------------------------------------------------------------------------
         *  CREATE THE VERTEX ARRAY OBJECT
         *-----------------------------------------------------------------------------*/
        GENVERTEXARRAYS(1, &arrayID);
        BINDVERTEXARRAY(arrayID);
        
        /*-----------------------------------------------------------------------------
         *  CREATE THE VERTEX BUFFER OBJECT
         *-----------------------------------------------------------------------------*/
        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        
        /*-----------------------------------------------------------------------------
         *  ENABLE VERTEX ATTRIBUTES
         *-----------------------------------------------------------------------------*/
        glEnableVertexAttribArray(positionID);
        // Tell OpenGL how to handle the buffer of data that is already on the GPU
        // stride, 0 means pack together
        glVertexAttribPointer(positionID, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (char*)(sizeof(float) * 2));
        
        GLushort indices[] = {0, 1, 2, 0, 3, 4};
        /*-----------------------------------------------------------------------------
         *  CREATE THE ELEMENT ARRAY BUFFER OBJECT
         *-----------------------------------------------------------------------------*/
        glGenBuffers(1, &indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW );
        
        // Unbind Everything (NOTE: unbind the vertex array object first)
        BINDVERTEXARRAY(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        shader->unbind();
    }

    void onDraw(){
        shader->bind();
        BINDVERTEXARRAY(arrayID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        BINDVERTEXARRAY(0);
    }

};


int main(int argc, const char ** argv){
 
    MyApp app;
    app.start();
       
    return 0;
}



