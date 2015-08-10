/*
 * =====================================================================================
 *
 *       Filename:  distortion.cpp
 *
 *    Description:  Cardboard distortion render
 *
 *        Version:  1.0
 *        Created:  08/08/2015 18:43:34
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
#include <cmath>

using namespace playground;
using namespace std;

/*-----------------------------------------------------------------------------
 *  DISTORTION
 *-----------------------------------------------------------------------------*/
class Distortion {
    
private:
    float DEFAULT_COEFFICIENTS[2] = {250.0f, 50000.0f};
    float mCoefficients[2];
    
public:
    Distortion() {
        mCoefficients[0] = DEFAULT_COEFFICIENTS[0];
        mCoefficients[1] = DEFAULT_COEFFICIENTS[1];
    }
    
    Distortion(Distortion* other) {
        mCoefficients[0] = other->mCoefficients[0];
        mCoefficients[1] = other->mCoefficients[1];
    }
    
    void setCoefficients(float coefficients[]) {
        mCoefficients[0] = coefficients[0];
        mCoefficients[1] = coefficients[1];
    }
    
    float* getCoefficients() {
        return mCoefficients;
    }
    
    // distoration factor is the factor that decides the shape of a distorted image;
    // it is cloasely coupled with the lense
    float distortionFactor(float radius) {
        float rSq = radius * radius; // square of R
        //1 + a * R^2 + b * R^4
        return 1.0f + mCoefficients[0] * rSq + mCoefficients[1] * rSq * rSq;
    }
    
    float distort(float radius) {
        // R + a * R^3 + b * R^5
        return radius * distortionFactor(radius);
    }
    
    //try to guess the original radius from a distorted radius
    float distortInverse(float radius) {
        float r0 = radius / 0.9f;
        float r1 = radius * 0.9f;
        
        float dr0 = radius - distort(r0);
        
        double step = 0.0001;
        while (abs(r1 - r0) > step) {
            float dr1 = radius - distort(r1);
            //directive equation
            float r2 = r1 - dr1 * ((r1 - r0) / (dr1 - dr0));
            r0 = r1;
            r1 = r2;
            dr0 = dr1;
        }
        return r1;
    }
};

/*-----------------------------------------------------------------------------
 *  DISTORTION SHADER CODE
 *-----------------------------------------------------------------------------*/
const char * distortVert = GLSL(120,
                         
    attribute vec2 position;
    attribute float vignette;
    attribute vec2 textureCoordinate;                   //<-- Texture Coordinate Attribute
                         
    varying vec2 texCoord;                              //<-- To be passed to fragment shader
                         
    void main(void){
        texCoord = textureCoordinate;
                             
        gl_Position = vec4(position, 0, 1);
    }
                
);

const char * distortFrag = GLSL(120,
                         
    varying vec2 texCoord;                            //<-- coordinate passed in from vertex shader
                         
    uniform sampler2D texture;                        //<-- The texture itself
                         
    void main(void){
        gl_FragColor =  texture2D( texture, texCoord ); //<-- look up the coordinate
    }
                         
);

/*-----------------------------------------------------------------------------
 *  DISTORTION MESH
 *-----------------------------------------------------------------------------*/
class DistortionMesh {
    int ROWS = 40;
    int COLS = 40;
    
    int nIndices;
    GLuint mArrayBufferId = -1;
    GLuint mElementBufferId = -1;
    
    Distortion distortion;
    
public:
    DistortionMesh() {
    }
    
    DistortionMesh(Window mWindow) {
        cout << "create the DistortionMesh" << endl;
        
        GLfloat vertexData[ROWS * COLS * 5];
        int vertexOffset = 0;
        float scale = 500.0f;
        
        float xEyeOffsetMScreen = mWindow.width() / 2.0f;
        float yEyeOffsetMScreen = mWindow.height() / 2.0f;
        
        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLS; col++) {
                float uTexture = col / (COLS - 1.0f);
                float vTexture = row / (ROWS - 1.0f);
                
                float xTexture = uTexture * mWindow.width();
                float yTexture = vTexture * mWindow.height();
                
                float xTextureEye = xTexture - xEyeOffsetMScreen;
                float yTextureEye = yTexture - yEyeOffsetMScreen;
                
                float rTexture = (float) sqrt(xTextureEye * xTextureEye + yTextureEye * yTextureEye);
                float textureToScreen = rTexture > 0.0F ? distortion.distortInverse(rTexture) / rTexture : 1.0F;
                
                float xScreen = xTextureEye * textureToScreen + xEyeOffsetMScreen;
                float yScreen = yTextureEye * textureToScreen + yEyeOffsetMScreen;
                float uScreen = xScreen / mWindow.width();
                float vScreen = yScreen / mWindow.height();
                
                vertexData[(vertexOffset + 0)] = (2.0f * uScreen - 1.0f) * scale;
                
                vertexData[(vertexOffset + 1)] = (2.0f * vScreen - 1.0f) * scale;
                
                vertexData[(vertexOffset + 2)] = 1.0f;
                vertexData[(vertexOffset + 3)] = uTexture;
                vertexData[(vertexOffset + 4)] = vTexture;
                
                cout << "(" << vertexData[(vertexOffset + 0)] << ":" << vertexData[(vertexOffset + 1)] << "), (" << vertexData[(vertexOffset + 3)] << ":" << vertexData[(vertexOffset + 4)] << ")" << endl;
                
                vertexOffset += 5;
            }
        }
        
        nIndices = ROWS * COLS * 2;
        GLuint indexData[nIndices];
        int indexOffset = 0;
        vertexOffset = 0;
        for (int row = 0; row < (ROWS - 1); row++) {
            if (row > 0) {
                indexData[indexOffset] = indexData[(indexOffset - 1)];
                indexOffset++;
            }
            for (int col = 0; col < COLS; col++) {
                if (col > 0) {
                    if (row % 2 == 0) {
                        vertexOffset++;
                    } else {
                        vertexOffset--;
                    }
                }
                indexData[(indexOffset++)] = vertexOffset;
                indexData[(indexOffset++)] = (vertexOffset + COLS);
            }
            vertexOffset += ROWS;
        }
        
        GLuint bufferIds[2];
        glGenBuffers(2, bufferIds);
        mArrayBufferId = bufferIds[0];
        mElementBufferId = bufferIds[1];
        
        glBindBuffer(GL_ARRAY_BUFFER, mArrayBufferId);
        glBufferData(GL_ARRAY_BUFFER, ROWS * COLS * 5 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBufferId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, nIndices * sizeof(GLuint), indexData, GL_STATIC_DRAW);
        
        // unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
};

/*-----------------------------------------------------------------------------
 *  SHADER CODE
 *-----------------------------------------------------------------------------*/
const char * vert = GLSL(120, 
  
  attribute vec4 position;
  attribute vec4 color;
  attribute vec2 textureCoord;

  varying vec4 dstColor;
  varying vec2 dstTextureCoord;

  uniform mat4 model;
  uniform mat4 view;                 //<-- 4x4 Transformation Matrices
  uniform mat4 projection;
  uniform float textureCoordScale;

  void main(){
    dstColor = color;
    gl_Position = projection * view * model * position;   //<-- Apply transformation
  }

);

const char * frag = GLSL(120,
  
  varying vec4 dstColor;
  varying vec2 dstTextureCoord;
                         
  uniform sampler2D textureSampler;
   
  void main(){
      gl_FragColor = dstColor;
  }

);


/*-----------------------------------------------------------------------------
 *  CREATE A VERTEX OBJECT
 *-----------------------------------------------------------------------------*/
struct Vertex{
  glm::vec3 position;
  glm::vec4 color;
};


/*-----------------------------------------------------------------------------
 *  OUR APP
 *-----------------------------------------------------------------------------*/
struct MyApp : public App{
    
    /*-----------------------------------------------------------------------------
     *  DISTORTION PARAM
     *-----------------------------------------------------------------------------*/
    GLint mOriginalFramebufferId[1];
    GLint mViewport[4];
    GLint mRenderbufferId = -1, mTextureId = -1, mFramebufferId = -1;
    bool mProjectionChanged = true;
    DistortionMesh mDistortionMesh;

    Shader * shader;
    
    //ID of Vertex Attribute
    GLuint positionID, normalID, colorID;

    //A buffer ID
    GLuint bufferID, elementID;                   //<-- add an elementID
    //An array ID
    GLuint arrayID;
  
    //ID of Uniforms
    GLuint modelID, viewID, projectionID;

    //Constructor (initialize application)
    MyApp() {
        initDistortion();
        init();
    }
    
    void checkGlError(string op) {
        int error;
        if ((error = glGetError()) != 0) {
            cout << op << ": glError " << error << endl;
        }
    }
    
    void initDistortion() {
        mDistortionMesh = DistortionMesh(mWindow);
    }

    void init() {
        //Specify the 8 VERTICES of A Cube
        Vertex cube[] = {
            {glm::vec3( 1, -1,  1), glm::vec4(1,0,0,1 )},
            {glm::vec3( 1,  1,  1), glm::vec4(0,1,0,1 )},
            {glm::vec3(-1,  1,  1), glm::vec4(0,0,1,1 )},
            {glm::vec3(-1, -1,  1), glm::vec4(1,0,0,1 )},
            
            {glm::vec3( 1, -1, -1), glm::vec4(0,1,0,1 )},
            {glm::vec3( 1,  1, -1), glm::vec4(0,0,1,1 )},
            {glm::vec3(-1,  1, -1), glm::vec4(1,0,0,1 )},
            {glm::vec3(-1, -1, -1), glm::vec4(0,1,0,1 )}
        };


             //6-------------/5
          //  .           // |
        //2--------------1   |
        //    .          |   |
        //    .          |   |
        //    .          |   |
        //    .          |   |
        //    7.......   |   /4
        //               | //
        //3--------------/0

        GLubyte indices[24] = {
            0,1,2,3, //front
            7,6,5,4, //back
            3,2,6,7, //left
            4,5,1,0, //right
            1,5,6,2, //top
            4,0,3,7 }; //bottom

        /*-----------------------------------------------------------------------------
         *  CREATE THE SHADER
         *-----------------------------------------------------------------------------*/
    
        shader = new Shader( vert, frag );

        // With Shader bound, get attribute and uniform locations:
 
        // Get attribute locations
        positionID = glGetAttribLocation(shader -> id(), "position");
        colorID = glGetAttribLocation(shader -> id(), "color");

        // Get uniform locations
        modelID = glGetUniformLocation(shader -> id(), "model");
        viewID = glGetUniformLocation(shader -> id(), "view");
        projectionID = glGetUniformLocation(shader -> id(), "projection");


        /*-----------------------------------------------------------------------------
         *  CREATE THE VERTEX ARRAY OBJECT
         *-----------------------------------------------------------------------------*/
        GENVERTEXARRAYS(1, &arrayID);
        BINDVERTEXARRAY(arrayID);

        /*-----------------------------------------------------------------------------
         *  CREATE THE VERTEX BUFFER OBJECT
         *-----------------------------------------------------------------------------*/
        // Generate one buffer
        glGenBuffers(1, &bufferID);
        glBindBuffer( GL_ARRAY_BUFFER, bufferID);
        glBufferData( GL_ARRAY_BUFFER, 8 * sizeof(Vertex), cube, GL_STATIC_DRAW );

        /*-----------------------------------------------------------------------------
         *  CREATE THE ELEMENT ARRAY BUFFER OBJECT
         *-----------------------------------------------------------------------------*/
        glGenBuffers(1, &elementID);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, elementID);
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, 24 * sizeof(GLubyte), indices, GL_STATIC_DRAW );

        /*-----------------------------------------------------------------------------
         *  ENABLE VERTEX ATTRIBUTES
         *-----------------------------------------------------------------------------*/
        glEnableVertexAttribArray(positionID);
        glEnableVertexAttribArray(colorID);

        // Tell OpenGL how to handle the buffer of data that is already on the GPU
        //                      attrib    num   type     normalize   stride     offset
        glVertexAttribPointer( positionID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0 );
        glVertexAttribPointer( colorID, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(glm::vec3) );

        // Unbind Everything (NOTE: unbind the vertex array object first)
        BINDVERTEXARRAY(0);
        glBindBuffer( GL_ARRAY_BUFFER, 0);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
    void beforeDrawFrameDistortion() {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, mOriginalFramebufferId);  // 36006
        glGetIntegerv(GL_FRAMEBUFFER, &mFramebufferId);
    }
    
    void afterDrawFrameDistortion() {
        glBindFramebuffer(GL_FRAMEBUFFER, mOriginalFramebufferId[0]);
        glViewport(0, 0, mWindow.width(), mWindow.height());
    }
    
    GLuint setupRenderTextureAndRenderbuffer(int width, int height) {
        mTextureId = createTexture(width, height);
        checkGlError("setupRenderTextureAndRenderbuffer: create texture");
        
        GLuint renderbufferIds[1];
        glGenRenderbuffers(1, renderbufferIds);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbufferIds[0]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
        
        mRenderbufferId = renderbufferIds[0];
        checkGlError("setupRenderTextureAndRenderbuffer: create renderbuffer");
        
        GLuint framebufferIds[1];
        glGenFramebuffers(1, framebufferIds);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferIds[0]);
        mFramebufferId = framebufferIds[0];
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureId, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbufferIds[0]);
        
        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            cout << "Framebuffer is not complete: " << status << endl;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        return framebufferIds[0];
    }
    
    GLuint createTexture(int width, int height) {
        GLuint textureIds[1];
        glGenTextures(1, textureIds);
        
        glBindTexture(GL_TEXTURE_2D, textureIds[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
        
        return textureIds[0];
    }
    
    void onDrawFrameCustom() {
        static float time = 0.0;
        time += .01;
        
        BINDVERTEXARRAY(arrayID);
        
        glm::mat4 view = glm::lookAt( glm::vec3(0,0,5), glm::vec3(0,0,0), glm::vec3(0,1,0) );
        glm::mat4 proj = glm::perspective( PI / 3.0f, (float)window().width()/window().height(), 0.1f,-10.f);
        
        glUniformMatrix4fv( viewID, 1, GL_FALSE, glm::value_ptr(view) );
        glUniformMatrix4fv( projectionID, 1, GL_FALSE, glm::value_ptr(proj) );
        
        glm::mat4 model = glm::rotate( glm::mat4(), time, glm::vec3(0,1,0) );
        // glm::mat4 model = glm::rotate( glm::mat4(), 0.0f, glm::vec3(0,1,0) );
        
        // glm::mat4 translation =  glm::translate( glm::mat4(), glm::vec3(0,0,0) );
        // glm::mat4 rotation =  glm::rotate( glm::mat4(), 0.0f, glm::vec3(0,1,0) );
        // glm::mat4 scale = glm::scale( glm::mat4(), glm::vec3(1) );
        // //ORDER MATTERS!
        // glm::mat4 model = translation * rotation * scale;
        
        glUniformMatrix4fv( modelID, 1, GL_FALSE, glm::value_ptr(model) );
        
        //glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, elementID);
        glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, 0);
        
        BINDVERTEXARRAY(0);

    }
    
    void onProjectionChanged() {
        float zNear = 0.1f, zFar = -10.f;
        
//        mDistortionMesh =
    }

    void onDraw() {
        if (mProjectionChanged) {
            
            mProjectionChanged = false;
        }
        
        beforeDrawFrameDistortion();

        onDrawFrameCustom();
        
        afterDrawFrameDistortion();
  
    }

};


int main(int argc, const char ** argv){
 
    MyApp app;
    app.start();
    
    return 0;
}



