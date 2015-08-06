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
    GLuint bufferID;
    
    //An array ID
    GLuint arrayID;

    //Constructor (initialize application)
    MyApp() { init(); }

    void init(){
        
        GLfloat verts[] =
        {
            +0.0f, +0.0f,
            +1.0f, +1.0f,
            -1.0f, +1.0f,
            
            +0.0f, +0.0f,
            -1.0f, -1.0f,
            +1.0f, -1.0f,
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
        glGenBuffers(1, &bufferID);
        glBindBuffer(GL_ARRAY_BUFFER, bufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        
        /*-----------------------------------------------------------------------------
         *  ENABLE VERTEX ATTRIBUTES
         *-----------------------------------------------------------------------------*/
        glEnableVertexAttribArray(positionID);
        // Tell OpenGL how to handle the buffer of data that is already on the GPU
        glVertexAttribPointer(positionID, 2, GL_FLOAT, GL_FALSE, 0, 0);
        
        // Unbind Everything (NOTE: unbind the vertex array object first)
        BINDVERTEXARRAY(0);
        glBindBuffer( GL_ARRAY_BUFFER, 0);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0);
        shader->unbind();
    }

    void onDraw(){
        shader->bind();
        BINDVERTEXARRAY(arrayID);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        BINDVERTEXARRAY(0);
    }

};


int main(int argc, const char ** argv){
 
    MyApp app;
    app.start();
       
    return 0;
}



