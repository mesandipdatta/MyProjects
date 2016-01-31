/*
 * =====================================================================================
 *
 *       Filename:  distortionTextureLoader.cpp
 *
 *    Description: load a texture from file. 
 *
 *        Version:  1.0
 *        Created:  08/09/2015 11:06:36
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Yu Sun
 *
 * =====================================================================================
 */

#include "glfw_app.hpp"
#include "gl_shader.hpp"
#include "gl_data.hpp"                                //<-- bitmap loader
#include "gl_macros.hpp"

#include <cmath>

using namespace playground;
using namespace std;


const char * vert = GLSL(120,

    attribute vec2 position;
    attribute float vignette;
    attribute vec2 textureCoordinate;                   //<-- Texture Coordinate Attribute

    varying vec2 texCoord;                              //<-- To be passed to fragment shader

    void main(void){
        texCoord = textureCoordinate;

        gl_Position = vec4(position, 0, 1);
    }

);

const char * frag = GLSL(120,

    varying vec2 texCoord;                            //<-- coordinate passed in from vertex shader
                         
    uniform sampler2D texture;                        //<-- The texture itself

    void main(void){
        gl_FragColor =  texture2D( texture, texCoord ); //<-- look up the coordinate
    }

);


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

struct MyApp : App {

    Shader * shader;

    GLuint tID;
    GLuint arrayID;
    GLuint bufferID;
    GLuint elementID;
    GLuint positionID;
    GLuint textureCoordinateID;
    GLuint samplerID;
    
    int ROWS = 40;
    int COLS = 40;
 

    MyApp() : App() { init(); }


    void init(){
        Distortion distortion;

        Bitmap img("resources/flower.bmp");
      
        /*-----------------------------------------------------------------------------
         *  Distortion Mesh
         *-----------------------------------------------------------------------------*/
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
      
        int nIndices = ROWS * COLS * 2;
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
      
        /*-----------------------------------------------------------------------------
         *  Make some rgba data (can also load a file here)
         *-----------------------------------------------------------------------------*/
        int tw = img.width;
        int th = img.height;

        /*-----------------------------------------------------------------------------
         *  Create Shader
         *-----------------------------------------------------------------------------*/
        shader = new Shader(vert,frag);

        /*-----------------------------------------------------------------------------
         *  Get Attribute Locations
         *-----------------------------------------------------------------------------*/
        positionID = glGetAttribLocation( shader->id(), "position" );
        textureCoordinateID = glGetAttribLocation( shader->id(), "textureCoordinate");
      
        /*-----------------------------------------------------------------------------
         *  Generate Vertex Array Object
         *-----------------------------------------------------------------------------*/
        GENVERTEXARRAYS(1,&arrayID);
        BINDVERTEXARRAY(arrayID);

        /*-----------------------------------------------------------------------------
         *  Generate Vertex Buffer Object
         *-----------------------------------------------------------------------------*/
        glGenBuffers(1, &bufferID);
        glBindBuffer( GL_ARRAY_BUFFER, bufferID);
        glBufferData( GL_ARRAY_BUFFER,  ROWS * COLS * 5 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW );
      
        /*-----------------------------------------------------------------------------
         *  CREATE THE ELEMENT ARRAY BUFFER OBJECT
         *-----------------------------------------------------------------------------*/
        glGenBuffers(1, &elementID);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, elementID);
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, nIndices * sizeof(GLuint), indexData, GL_STATIC_DRAW );

        /*-----------------------------------------------------------------------------
         *  Enable Vertex Attributes and Point to them
         *-----------------------------------------------------------------------------*/
        glEnableVertexAttribArray(positionID);
        glEnableVertexAttribArray(textureCoordinateID);
        glVertexAttribPointer( positionID, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
        glVertexAttribPointer( textureCoordinateID, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (void*) (sizeof(float)* 3) );

        /*-----------------------------------------------------------------------------
         *  Unbind Vertex Array Object and the Vertex Array Buffer
         *-----------------------------------------------------------------------------*/
        BINDVERTEXARRAY(0);
        glBindBuffer( GL_ARRAY_BUFFER, 0 );

        /*-----------------------------------------------------------------------------
         *  Generate Texture and Bind it
         *-----------------------------------------------------------------------------*/
        glGenTextures(1, &tID);
        glBindTexture(GL_TEXTURE_2D, tID);

        /*-----------------------------------------------------------------------------
         *  Allocate Memory on the GPU
         *-----------------------------------------------------------------------------*/
        // target | lod | internal_format | width | height | border | format | type | data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
         
        /*-----------------------------------------------------------------------------
         *  Load data onto GPU (bitmaps flip RGB order)
         *-----------------------------------------------------------------------------*/
        // target | lod | xoffset | yoffset | width | height | format | type | data
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw, th, GL_BGR,GL_UNSIGNED_BYTE,img.pixels.data() );

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        /*-----------------------------------------------------------------------------
         *  Unbind texture
         *-----------------------------------------------------------------------------*/
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void onDraw(){
//        cout << mWindow.width() << ":" << mWindow.height() << endl;
        glUseProgram( shader->id() );          //<-- 1. Bind Shader
        glBindTexture( GL_TEXTURE_2D, tID );   //<-- 2. Bind Texture

        BINDVERTEXARRAY(arrayID);            //<-- 3. Bind VAO

        glDrawElements(GL_TRIANGLE_STRIP, ROWS * COLS * 2, GL_UNSIGNED_INT, 0);
        BINDVERTEXARRAY(0);                  //<-- 5. Unbind the VAO

        glBindTexture( GL_TEXTURE_2D, 0);      //<-- 6. Unbind the texture
        glUseProgram( 0 );                     //<-- 7. Unbind the shader

    }
};


int main(int argc, const char * argv[]){

    MyApp app;
    app.start();

    return 0;
}

