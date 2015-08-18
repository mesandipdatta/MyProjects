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
 *  DISTORTION MESH
 *-----------------------------------------------------------------------------*/
class DistortionMesh {
    const static int ROWS = 40;
    const static int COLS = 40;
    
    GLuint mArrayBufferId = -1;
    GLuint mElementBufferId = -1;
    
    GLfloat vertexData[ROWS * COLS * 5];
    GLuint indexData[ROWS * COLS * 2];
    
    Distortion distortion;
    
public:
    DistortionMesh() {}
    
    DistortionMesh(Window* mWindow) {}
    
    DistortionMesh(int width, int height) {
        int vertexOffset = 0;
        float scale = 500.0f;
        
        float xEyeOffsetMScreen = width / 2.0f;
        float yEyeOffsetMScreen = height / 2.0f;
        
        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLS; col++) {
                float uTexture = col / (COLS - 1.0f);
                float vTexture = row / (ROWS - 1.0f);
                
                float xTexture = uTexture * width;
                float yTexture = vTexture * height;
                
                float xTextureEye = xTexture - xEyeOffsetMScreen;
                float yTextureEye = yTexture - yEyeOffsetMScreen;
                
                float rTexture = (float) sqrt(xTextureEye * xTextureEye + yTextureEye * yTextureEye);
                float textureToScreen = rTexture > 0.0F ? distortion.distortInverse(rTexture) / rTexture : 1.0F;
                
                float xScreen = xTextureEye * textureToScreen + xEyeOffsetMScreen;
                float yScreen = yTextureEye * textureToScreen + yEyeOffsetMScreen;
                float uScreen = xScreen / width;
                float vScreen = yScreen / height;
                
                vertexData[(vertexOffset + 0)] = (2.0f * uScreen - 1.0f) * scale;
                
                vertexData[(vertexOffset + 1)] = (2.0f * vScreen - 1.0f) * scale;
                
                vertexData[(vertexOffset + 2)] = 1.0f;
                vertexData[(vertexOffset + 3)] = uTexture;
                vertexData[(vertexOffset + 4)] = vTexture;
                
//                cout << "(" << vertexData[(vertexOffset + 0)] << ":" << vertexData[(vertexOffset + 1)] << "), (" << vertexData[(vertexOffset + 3)] << ":" << vertexData[(vertexOffset + 4)] << ")" << endl;
                
                vertexOffset += 5;
            }
        }
        
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
    }
    
    GLfloat* getVertexData() {
        return vertexData;
    }
    
    int getVertexDataLength() {
        return ROWS * COLS * 5;
    }
    
    GLuint* getIndexData() {
        return indexData;
    }
    
    int getIndexDataLength() {
        return ROWS * COLS * 2;
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
     *  WINDOW SIZE
     *-----------------------------------------------------------------------------*/
    int width = 1280, height = 960;
    
    /*-----------------------------------------------------------------------------
     *  DISTORTION PARAM
     *-----------------------------------------------------------------------------*/
    DistortionMesh mDistortionMesh;
    
    Shader * distortShader;
    //ID of Vertex Attribute
    GLuint distortPositionID, distortTextureCoordinateID, distortTextureSamplerID;
    
    //A buffer ID
    GLuint distortBufferID, distortElementID;                   //<-- add an elementID
    //An array ID
    GLuint distortArrayID;
    
    GLint mOriginalFramebufferId;
    GLuint mRenderbufferId = 0, mTextureId = 0, mFramebufferId = 0;
    
    
    /*-----------------------------------------------------------------------------
     *  CUSTOM PARAM
     *-----------------------------------------------------------------------------*/
    Shader * shader;
    
    //ID of Vertex Attribute
    GLuint positionID, colorID;

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
        /*-----------------------------------------------------------------------------
         *  Create Shader
         *-----------------------------------------------------------------------------*/
        distortShader = new Shader( distortVert, distortFrag );
        
        mDistortionMesh = DistortionMesh(width, height);
        
        GLfloat* vertexData = mDistortionMesh.getVertexData();
        int vertexDataLength = mDistortionMesh.getVertexDataLength();
        
        GLuint* indexData = mDistortionMesh.getIndexData();
        int indexDataLength = mDistortionMesh.getIndexDataLength();
        
        /*-----------------------------------------------------------------------------
         *  Get Attribute Locations
         *-----------------------------------------------------------------------------*/
        distortPositionID = glGetAttribLocation(distortShader -> id(), "position");
        checkGlError("initDistortion: glGetAttribLocation position");
        
        distortTextureCoordinateID = glGetAttribLocation( distortShader->id(), "textureCoordinate");
        checkGlError("initDistortion: glGetAttribLocation textureCoordinate");
        
        distortTextureSamplerID = glGetUniformLocation(distortShader->id(), "texture");
        checkGlError("initDistortion: glGetUniformLocation texture");
        
        /*-----------------------------------------------------------------------------
         *  Generate Vertex Array Object
         *-----------------------------------------------------------------------------*/
        GENVERTEXARRAYS(1,&distortArrayID);
        BINDVERTEXARRAY(distortArrayID);
        
        /*-----------------------------------------------------------------------------
         *  Generate Vertex Buffer Object
         *-----------------------------------------------------------------------------*/
        glGenBuffers(1, &distortBufferID);
        glBindBuffer( GL_ARRAY_BUFFER, distortBufferID);
        glBufferData( GL_ARRAY_BUFFER,  vertexDataLength * sizeof(GLfloat), vertexData, GL_STATIC_DRAW );
        
        /*-----------------------------------------------------------------------------
         *  CREATE THE ELEMENT ARRAY BUFFER OBJECT
         *-----------------------------------------------------------------------------*/
        glGenBuffers(1, &distortElementID);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, distortElementID);
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexDataLength * sizeof(GLuint), indexData, GL_STATIC_DRAW );
        
        /*-----------------------------------------------------------------------------
         *  Enable Vertex Attributes and Point to them
         *-----------------------------------------------------------------------------*/
        glEnableVertexAttribArray(distortPositionID);
        glEnableVertexAttribArray(distortTextureCoordinateID);
        glVertexAttribPointer( distortPositionID, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
        glVertexAttribPointer( distortTextureCoordinateID, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (void*) (sizeof(float)* 3) );
        
        setupRenderTextureAndRenderbuffer(width, height);
        
        /*-----------------------------------------------------------------------------
         *  Unbind Vertex Array Object and the Vertex Array Buffer
         *-----------------------------------------------------------------------------*/
        BINDVERTEXARRAY(0);
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        
        distortShader->unbind();
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
        
        shader->unbind();
    }
    
    void beforeDrawFrameDistortion() {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mOriginalFramebufferId);  // 36006
//        cout << "beforeDrawFrameDistortion: mOriginalFramebufferId->" << mOriginalFramebufferId[0] << endl;
        glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferId);
//        cout << "beforeDrawFrameDistortion: mFramebufferId->" << mFramebufferId << endl;
    }
    
    void renderDistortionMesh() {
        distortShader->bind();
        
        glBindBuffer(GL_ARRAY_BUFFER, distortArrayID);
        
        glVertexAttribPointer( distortPositionID, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
        glEnableVertexAttribArray(distortPositionID);
        
        glVertexAttribPointer( distortTextureCoordinateID, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (void*) (sizeof(float)* 3) );
        glEnableVertexAttribArray(distortTextureCoordinateID);
        
//        glActiveTexture(GL_TEXTURE0);
        //        cout << "renderDistortionMesh: mTextureId->" << mTextureId << endl;
        glBindTexture(GL_TEXTURE_2D, mTextureId);
        
//        BINDVERTEXARRAY(distortArrayID);
        
        glUniform1i(distortTextureSamplerID, 0);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, distortElementID);
        
        glDrawElements(GL_TRIANGLE_STRIP, mDistortionMesh.getIndexDataLength(), GL_UNSIGNED_INT, 0);
        
        distortShader->unbind();
    }
    
    void afterDrawFrameDistortion() {
        glBindFramebuffer(GL_FRAMEBUFFER, mOriginalFramebufferId);
        glViewport(0, 0, width, height);
        
//        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        renderDistortionMesh();
        
        //4) unbind resources
        //4.1) unbind attributes
        glDisableVertexAttribArray(distortPositionID);
        glDisableVertexAttribArray(distortTextureCoordinateID);
        //4.2) unbind programs and buffers
        glUseProgram(GL_FALSE);
        glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE); //GL_FALSE?
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_FALSE);
    }
    
    GLuint setupRenderTextureAndRenderbuffer(int width, int height) {
        if (mTextureId != 0) {
            cout << "setupRenderTextureAndRenderbuffer: error mTextureId" << endl;
        }
        if (mRenderbufferId != 0) {
            cout << "setupRenderTextureAndRenderbuffer: error mRenderbufferId" << endl;
        }
        if (mFramebufferId != 0) {
            cout << "setupRenderTextureAndRenderbuffer: error mFramebufferId" << endl;
        }
        
        mTextureId = createTexture(width, height);
        checkGlError("setupRenderTextureAndRenderbuffer: create texture");
        
        // The depth buffer
        glGenRenderbuffers(1, &mRenderbufferId);
        glBindRenderbuffer(GL_RENDERBUFFER, mRenderbufferId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        
        checkGlError("setupRenderTextureAndRenderbuffer: create renderbuffer");
        cout << "setupRenderTextureAndRenderbuffer: mRenderbufferId->" << mRenderbufferId << endl;
        
        glGenFramebuffers(1, &mFramebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferId);
        cout << "setupRenderTextureAndRenderbuffer: mFramebufferId->" << mFramebufferId << endl;
        
        // Set "renderedTexture" as our colour attachement #0
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextureId, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRenderbufferId);
        
        // Set the list of draw buffers.
//        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
//        glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
        
        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            cout << "Framebuffer is not complete: " << status << endl;
        }
        
        // unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        return mFramebufferId;
    }
    
    GLuint createTexture(int width, int height) {
        glGenTextures(1, &mTextureId);
        
        glBindTexture(GL_TEXTURE_2D, mTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
        
        //Mipmaps are good -- the regenerate the texture at various scales
        // and are necessary to avoid black screen if texParameters below are not set
        glGenerateMipmap(GL_TEXTURE_2D);
        
        /*-----------------------------------------------------------------------------
         *  Unbind texture
         *-----------------------------------------------------------------------------*/
        glBindTexture(GL_TEXTURE_2D, 0);
        
        return mTextureId;
    }
    
    void onDrawFrameCustom() {
        shader->bind();
        
        static float time = 0.0;
        time += .01;
        
        BINDVERTEXARRAY(arrayID);
        
        glm::mat4 view = glm::lookAt( glm::vec3(0,0,5), glm::vec3(0,0,0), glm::vec3(0,1,0) );
        glm::mat4 proj = glm::perspective( PI / 3.0f, (float)window().width()/window().height(), 0.1f,-10.f);
        
        glUniformMatrix4fv( viewID, 1, GL_FALSE, glm::value_ptr(view) );
        glUniformMatrix4fv( projectionID, 1, GL_FALSE, glm::value_ptr(proj) );
        
        glm::mat4 model = glm::rotate( glm::mat4(), time, glm::vec3(0,1,0) );
        
        glUniformMatrix4fv( modelID, 1, GL_FALSE, glm::value_ptr(model) );
        
        glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, 0);
        
        BINDVERTEXARRAY(0);
        
        shader->unbind();
    }

    void onDraw() {
//        beforeDrawFrameDistortion();
        onDrawFrameCustom();
//        afterDrawFrameDistortion();
    }
};


int main(int argc, const char ** argv){
 
    MyApp app;
    app.start();
    
    return 0;
}



