/*
 * =====================================================================================
 *
 *       Filename:  Texture.cpp
 *
 *    Description:  apply textures to a rectangular slab
 *
 *        Version:  1.0
 *        Created:  06/11/2014 18:41:42
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Pablo Colapinto (), gmail -> wolftype
 *
 * =====================================================================================
 */

#include "glfw_app.hpp"
#include "gl_shader.hpp"
#include "gl_macros.hpp"
#include "gl_data.hpp"              //<-- bitmap data loader
#include "gl_error.hpp"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <vector>
#include <cmath>

using namespace playground;
using namespace std;

bool enable_vignette = true;
bool enable_distortion = false;

const char * vert = GLSL(120,
                         attribute vec2 position;
                         attribute vec2 textureCoordinate;              //<-- Texture Coordinate Attribute
                         attribute float vigneet;
                         
                         varying vec2 texCoord;                         //<-- To be passed to fragment shader
                         varying float vVigneet;                        //<-- To be passed to fragment shader
                         
                         void main(void){
                             texCoord = textureCoordinate;
                             vVigneet = vigneet;
                             gl_Position = vec4(position, 0, 1.0);//position;
                         }
                         
                         );

const char * frag = GLSL(120,
                         
                         uniform sampler2D texture;                       //<-- The texture itself
                         
                         varying vec2 texCoord;                           //<-- coordinate passed in from vertex shader
                         varying float vVigneet;                          //<-- vignette
                         
                         void main(void){
                             gl_FragColor = vVigneet * texture2D( texture, texCoord ); //<-- look up the coordinate's value
                         }
                         
                         );

const char * cvert = GLSL(120,
                         
                         attribute vec4 position;
                         attribute vec4 color;
                         
                         varying vec4 dstColor;
                         
                         uniform mat4 model;
                         uniform mat4 view;                 //<-- 4x4 Transformation Matrices
                         uniform mat4 projection;
                         
                         void main(){
                             dstColor = color;
                             gl_Position = projection * view * model * position;   //<-- Apply transformation
                         }
                         
                         );

const char * cfrag = GLSL(120,
                         
                         varying vec4 dstColor;
                         
                         void main(){
                             gl_FragColor = dstColor;
                         }
                         
                         );

struct vec2 {
    vec2(float _x=0, float _y=0) : x(_x), y(_y) {}
    float x,y;
};

struct vec4 {
    vec4(float _x=0, float _y=0, float _z=0, float _w=0) : x(_x), y(_y), z(_z), w(_w){}
    float x,y,z,w;
};

struct Vertex{
    vec2 position;
    vec2 textureCoordinate;
    float vigneet;
};

struct Vertex3{
    glm::vec3 position;
    glm::vec4 color;
};

int screenWidth = 800;
int screenHeight = 600;
int viewportWidth = screenWidth/2;
int viewportHeight = screenHeight/2;

float pxPerInch = 220;

struct Param{
    float interLenseInch = 4;
    float lenseWidthInch = 3;
    float lenseHeightInch = 3;
    float lenseCenterRatio = 0.5;
    //float lenseCenterRatio = 0.5;
    
};

struct MyApp : App {
    
    int th, tw;
    
    Shader * shader;
    Shader * cShader;
    
    /*
        For distortion
     */
    GLuint arrayID1;
    GLuint arrayID2;
    
    GLuint positionID;
    GLuint textureCoordinateID;
    GLuint vigneetID;
    GLuint samplerID;
    /*
        For custom render
     */
    GLuint cPositionID;
    GLuint cColorID;
    GLuint cBufferID, cElementID;
    GLuint cArrayID;
    GLuint cModelID, cViewID, cProjectionID;
    
    /*
        For context switching
     */
    GLint screenBufferID;
    GLuint renderbufferID;
    GLuint framebufferID;
    GLuint textureID;
    
    //A Container for Vertices
    vector<Vertex> vertices1;
    //A Container for Vertices
    vector<Vertex> vertices2;
    //A Container for indices
    vector<unsigned short> indices;
    
    MyApp() : App(screenWidth, screenHeight) { init(); }
    
    float unit = 0.1;
    
    void onKeyDown(int key, int action){

        if(action == GLFW_PRESS) {
            switch (key) {
                case GLFW_KEY_UP:
                    moveZ -= unit;
                    cout << "up " << moveZ << endl;
                    break;
                case GLFW_KEY_DOWN:
                    moveZ += unit;
                    cout << "down " << moveZ <<endl;
                    break;
                case GLFW_KEY_LEFT:
                    moveX -= unit;
                    cout << "left " << moveX << endl;
                    break;
                case GLFW_KEY_RIGHT:
                    moveX += unit;
                    cout << "right " << moveX << endl;
                    break;
            }
        }
    
    }
    
    int COLS = 40;
    int ROWS = 40;
    
    float moveX = 0;
    float moveZ = 0;
    
    float a = 15;//250.0f;
    float b = 100;//50000.0f;
    
    float distortionFactor(float radius) {
        float rSq = radius * radius; // square of R
        //1 + a * R^2 + b * R^4
        return 1.0f + a * rSq + b * rSq * rSq;
    }
    
    float distort(float radius) {
        // R + a * R^3 + b * R^5
        return radius * distortionFactor(radius);
    }
    
    //try to guess the original radius from a distorted radius
    float distortInverse(float radius) {
        float r0 = radius / 0.9F;
        float r1 = radius * 0.9F;
        
        float dr0 = radius - distort(r0);
        
        while (abs(r1 - r0) > 0.0001f) {
            float dr1 = radius - distort(r1);
            //directive equation
            float r2 = r1 - dr1 * ((r1 - r0) / (dr1 - dr0));
            r0 = r1;
            r1 = r2;
            dr0 = dr1;
        }
        return r1;
    }

    inline float clamp(float val, float min, float max) {
        
        float mid = std::min(max, val);
        return std::max(min, mid);
    }
    
    float vigneetWidth = 0.1;
    float scale = 600;
    
    void initMesh(bool left, float viewportTextureWidth, float viewportTextureHeight) {
        
        printf("Construc Mesh : %s\n", left? "Left":"Right");
        /*-----------------------------------------------------------------------------
         *  Create vertices of a 2d grid mesh
         *-----------------------------------------------------------------------------*/
        //                  position      texture coord
        
//        float xEyeOffsetMScreen = 0.5 * viewportTextureWidth;
//        float yEyeOffsetMScreen = 0.5 * viewportTextureHeight;
        
        for (int i=0;i<ROWS;i++){
            for(int j=0;j<COLS;j++){
                float tv = i * 1.0f / (ROWS - 1);           //u, v in single eye space
                float tu = j * 1.0f / (COLS - 1);
                
                //test
                float tx = tu * 2 -1;
                float ty = tv * 2 -1;
                
//                float ty2 = ty / viewportWidth * viewportHeight;
                
                float tr = sqrt(tx * tx + ty * ty);
                float shrinkRatio = tr>0? distortInverse(tr)/tr : 1;
                
                shrinkRatio *= 3;//8;
                float xScreen = tx * shrinkRatio;
                float yScreen = ty * shrinkRatio;
                
                tu /=2;
                //tu-=1;
                if(!left) {
                    tu += 0.5;
                }
//                if(!left) {
//                    tu += 1.5;
//                }
                //test
                
//                float tx = tu * viewportTextureWidth;    //in actual texture space
//                float ty = tv * viewportTextureHeight;
//                
//                float diffX = tx - xEyeOffsetMScreen;
//                float diffY = ty - yEyeOffsetMScreen;
//                
//                float diffR = sqrt(diffX*diffX + diffY*diffY);
//                
//                float shrinkRatio = diffR > 0.0F ? distortInverse(diffR) / diffR : 1.0F;
//                
//                shrinkRatio *= scale;
//                
//                float xScreen = diffX * shrinkRatio + xEyeOffsetMScreen;
//                float yScreen = diffY * shrinkRatio + yEyeOffsetMScreen;
//                
//                float uScreen = xScreen / viewportTextureWidth;
//                float vScreen = yScreen / viewportTextureHeight;
//                //core part : calculate on UV space, and model space
                
                //For vigneet
                float vigneet = 1;
                
//                if (enable_vignette) {
//                    float vigneetSizeX = vigneetWidth * viewportTextureWidth;
//                    float vigneetSizeY = vigneetWidth * viewportTextureHeight;
//                    float maxVigneetDistance = sqrt(vigneetSizeX * vigneetSizeX + vigneetSizeY * vigneetSizeY);
//                    
//                    float validX = clamp(tx, vigneetSizeX, viewportTextureWidth - vigneetSizeX);
//                    float validY = clamp(ty, vigneetSizeY, viewportTextureHeight - vigneetSizeY);
//                    
//                    float disX = tx - validX;
//                    float disY = ty - validY;
//                    
//                    float distance = (float) sqrt(disX * disX + disY * disY);
//                    vigneet = 1 - clamp(distance / maxVigneetDistance, 0, 1);
//                }
                
                //projection to whole texture scale
//                float posX = uScreen*2-1;//scale * (uScreen*2-1);
//                float posY = vScreen*2-1;//scale * (vScreen*2-1);
////                float posX;
////                float posY = vScreen*2-1;//scale * (vScreen*2-1);
//                
//                if(left) {
//                    tu /=2;
////                    posX = (uScreen - 1); //scale * ;
//                } else {
////                    posX = uScreen; //scale *
//                    tu /= 2;
//                    tu += 0.5;
//                }
//                
//                Vertex v = {vec2(posX, posY), vec2(tu, tv), vigneet};
                
                //test
                Vertex v = {vec2(xScreen, yScreen), vec2(tu, tv), vigneet};
                //test
                
                if(left) {
                    vertices1.push_back( v );
                } else {
                    vertices2.push_back( v );
                }

                printf("%f, %f -> %f, %f : %f, %f\n", xScreen, yScreen, tu, tv, vigneet, shrinkRatio);
                //printf("%f, %f -> %f, %f : %f, %f\n", posX, posY, tu, tv, vigneet, shrinkRatio);
            }
        }
        
        /*-----------------------------------------------------------------------------
         *  Create indices into the vertex array (triangle strip)
         *-----------------------------------------------------------------------------*/
        
        int vertexOffset = 0;
        
        //only run once for indices
        if(indices.size()<1) {
            for (int row = 0; row < ROWS-1; row++) {
                if (row > 0) {
                    indices.push_back(indices.at(indices.size()-1));
                }
                for (int col = 0; col < COLS; col++) {
                    if (col > 0) {
                        if (row % 2 == 0) { //row!
                            vertexOffset++;
                        } else {
                            vertexOffset--;
                        }
                    }
                    
                    indices.push_back(vertexOffset);
                    indices.push_back(vertexOffset + COLS);
                    
                }
                vertexOffset += COLS;
            }
        }
        
        /*-----------------------------------------------------------------------------
         *  Load img data
         *-----------------------------------------------------------------------------*/
//        Bitmap img("resources/flower.bmp");
//        tw = img.width;
//        th = img.height;
        
        /*-----------------------------------------------------------------------------
         *  Create Shader
         *-----------------------------------------------------------------------------*/
        shader = new Shader(vert,frag);
        
        /*-----------------------------------------------------------------------------
         *  Get Attribute Locations
         *-----------------------------------------------------------------------------*/
        positionID = glGetAttribLocation( shader->id(), "position" );
        textureCoordinateID = glGetAttribLocation( shader->id(), "textureCoordinate");
        vigneetID = glGetAttribLocation( shader->id(), "vigneet");
        
        //  samplerID = glGetUniformLocation( shader->id(), "texture" );               //<-- unnecessary if only using one texture
        
        /*-----------------------------------------------------------------------------
         *  Generate And Bind Vertex Array Object
         *-----------------------------------------------------------------------------*/
        /*-----------------------------------------------------------------------------
         *  Generate Vertex Buffer Object
         *-----------------------------------------------------------------------------*/
        
        GLuint bufferID;
        if (left) {
            GENVERTEXARRAYS(1,&arrayID1);
            BINDVERTEXARRAY(arrayID1);
            glGenBuffers(1, &bufferID);
            glBindBuffer( GL_ARRAY_BUFFER, bufferID);
            //glBufferData( GL_ARRAY_BUFFER,  4 * sizeof(Vertex), slab, GL_STATIC_DRAW );
            glBufferData( GL_ARRAY_BUFFER, vertices1.size() * sizeof(Vertex), vertices1.data(), GL_STATIC_DRAW);

        } else {
            GENVERTEXARRAYS(1,&arrayID2);
            BINDVERTEXARRAY(arrayID2);
            glGenBuffers(1, &bufferID);
            glBindBuffer( GL_ARRAY_BUFFER, bufferID);
            //glBufferData( GL_ARRAY_BUFFER,  4 * sizeof(Vertex), slab, GL_STATIC_DRAW );
            glBufferData( GL_ARRAY_BUFFER, vertices2.size() * sizeof(Vertex), vertices2.data(), GL_STATIC_DRAW);

        }

        GL::error("buffer array data");
        
        /*-----------------------------------------------------------------------------
         *  CREATE THE ELEMENT ARRAY BUFFER OBJECT
         *-----------------------------------------------------------------------------*/
        
        //the indices can be calculated once
        //but has to bind seperatedly for each vertex array
        GLuint elementID;
        glGenBuffers(1, &elementID);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, elementID);
        glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(GLushort),
                     indices.data(),
                     GL_STATIC_DRAW );

        GL::error("buffer element data");
        
        /*-----------------------------------------------------------------------------
         *  Enable Vertex Attributes and Point to them
         *-----------------------------------------------------------------------------*/
        glEnableVertexAttribArray(positionID);
        glEnableVertexAttribArray(textureCoordinateID);
        glEnableVertexAttribArray(vigneetID);
        
        glVertexAttribPointer( positionID, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0 );
        glVertexAttribPointer( textureCoordinateID, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(vec2) );
        glVertexAttribPointer( vigneetID, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (2 * sizeof(vec2)));
        
        /*-----------------------------------------------------------------------------
         *  Unbind Vertex Array Object and the Vertex Array Buffer
         *-----------------------------------------------------------------------------*/
        BINDVERTEXARRAY(0);
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
    }
    
    int createTexture(int width, int height) {
        GLuint textureIds[1];
        /*-----------------------------------------------------------------------------
         *  Generate Texture and Bind it
         *-----------------------------------------------------------------------------*/
        glGenTextures(1, textureIds);
        glBindTexture(GL_TEXTURE_2D, textureIds[0]);
        
        /*-----------------------------------------------------------------------------
         *  Allocate Memory on the GPU
         *-----------------------------------------------------------------------------*/
        // target | lod | internal_format | width | height | border | format | type | data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
        
        // Set these parameters to avoid a black screen
        // caused by improperly mipmapped textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  
        /*-----------------------------------------------------------------------------
         *  Load data onto GPU (bitmaps flip RGB order)
         *-----------------------------------------------------------------------------*/
        // target | lod | xoffset | yoffset | width | height | format | type | data
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw, th, GL_BGR,GL_UNSIGNED_BYTE, img.pixels.data());
        
        //Mipmaps are good -- the regenerate the texture at various scales
        // and are necessary to avoid black screen if texParameters below are not set
        glGenerateMipmap(GL_TEXTURE_2D);
        
        /*-----------------------------------------------------------------------------
         *  Unbind texture
         *-----------------------------------------------------------------------------*/
        glBindTexture(GL_TEXTURE_2D, 0);

        
        return textureIds[0];
    }
    
    void createCustomeProgram() {
        
        cShader = new Shader( cvert, cfrag );
        //Specify the 8 VERTICES of A Cube
        Vertex3 cube[] = {
            {glm::vec3( 1, -1,  1), glm::vec4(0,0,1,1 )},
            {glm::vec3( 1,  1,  1), glm::vec4(0,1,1,1 )},
            {glm::vec3(-1,  1,  1), glm::vec4(0,1,0,1 )},
            {glm::vec3(-1, -1,  1), glm::vec4(1,1,0,1 )},
            
            {glm::vec3( 1, -1, -1), glm::vec4(1,1,1,1 )},
            {glm::vec3( 1,  1, -1), glm::vec4(0,1,1,1 )},
            {glm::vec3(-1,  1, -1), glm::vec4(0,0,1,1 )},
            {glm::vec3(-1, -1, -1), glm::vec4(0,1,1,1 )}
        };
        
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
        
        cPositionID = glGetAttribLocation(cShader -> id(), "position");
        cColorID = glGetAttribLocation(cShader -> id(), "color");
        
        // Get uniform locations
        cModelID = glGetUniformLocation(cShader -> id(), "model");
        cViewID = glGetUniformLocation(cShader -> id(), "view");
        cProjectionID = glGetUniformLocation(cShader -> id(), "projection");
        
        
        /*-----------------------------------------------------------------------------
         *  CREATE THE VERTEX ARRAY OBJECT
         *-----------------------------------------------------------------------------*/
        GENVERTEXARRAYS(1, &cArrayID);
        BINDVERTEXARRAY(cArrayID);
        
        /*-----------------------------------------------------------------------------
         *  CREATE THE VERTEX BUFFER OBJECT
         *-----------------------------------------------------------------------------*/
        // Generate one buffer
        glGenBuffers(1, &cBufferID);
        glBindBuffer( GL_ARRAY_BUFFER, cBufferID);
        glBufferData( GL_ARRAY_BUFFER, 8 * sizeof(Vertex3), cube, GL_STATIC_DRAW );
        
        
        /*-----------------------------------------------------------------------------
         *  CREATE THE ELEMENT ARRAY BUFFER OBJECT
         *-----------------------------------------------------------------------------*/
        glGenBuffers(1, &cElementID);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, cElementID);
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, 24 * sizeof(GLubyte), indices, GL_STATIC_DRAW );
        
        
        /*-----------------------------------------------------------------------------
         *  ENABLE VERTEX ATTRIBUTES
         *-----------------------------------------------------------------------------*/
        glEnableVertexAttribArray(cPositionID);
        glEnableVertexAttribArray(cColorID);
        
        // Tell OpenGL how to handle the buffer of data that is already on the GPU
        //                      attrib    num   type     normalize   stride     offset
        glVertexAttribPointer( cPositionID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3), 0 );
        glVertexAttribPointer( cColorID, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3), (void*) sizeof(glm::vec3) );
        
        // Unbind Everything (NOTE: unbind the vertex array object first)
        BINDVERTEXARRAY(0);
        glBindBuffer( GL_ARRAY_BUFFER, 0);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    
    void initCustom() {
        
        textureID = createTexture(viewportWidth * 2, viewportHeight);
        GL::error("create texture");
        
        GLuint renderbufferIds[1];
        glGenRenderbuffers(1, renderbufferIds);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbufferIds[0]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, viewportWidth * 2, viewportHeight);
        renderbufferID = renderbufferIds[0];
        GL::error("create render buffer");
        
        GLuint framebufferIds[1];
        glGenFramebuffers(1, framebufferIds);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferIds[0]);
        framebufferID = framebufferIds[0];
        GL::error("create frame buffer");

        printf("Renderbuffer ID : %d, Framebuffer ID : %d \n", renderbufferID,framebufferID);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbufferID);
        
        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            cout << "Framebuffer is not complete: " << status <<endl;
            exit(1);
        }
            
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        createCustomeProgram();
    }
    
    void init(){
        initMesh(true, viewportWidth, viewportHeight);
        initMesh(false, viewportWidth, viewportHeight);
        initCustom();
    }
    
    void onDraw(){
        
        static float time = 0;
        time += 0.05;
        
        glEnable(GL_SCISSOR_TEST);
        
        if(enable_distortion) {
            beforeDraw();
            customDraw(time);
            afterDraw();
        } else {
            customDraw(time);
        }

        
        BINDVERTEXARRAY(0);                    //<-- 5. Unbind the VAO
        glBindTexture( GL_TEXTURE_2D, 0);      //<-- 6. Unbind the texture
        glUseProgram( 0 );                     //<-- 7. Unbind the shader
        glDisable(GL_SCISSOR_TEST);
    }
    
    void drawEye(bool left, float time) {
        
        if (left) {
            glViewport(0, 0, viewportWidth, viewportHeight);
            glScissor(0, 0, viewportWidth, viewportHeight);
        } else {
            glViewport(viewportWidth, 0, viewportWidth, viewportHeight);
            glScissor(viewportWidth, 0, viewportWidth, viewportHeight);
        }
        
        glClearColor(0.1, 0.1, 0.1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glm::mat4 view = glm::lookAt( glm::vec3(moveX + (left? -0.5 : 0.5),0, moveZ + 5), glm::vec3(0,0,-2), glm::vec3(0,1,0) );
        glm::mat4 proj = glm::perspective( 3.14f / 3.f, (float)viewportWidth/viewportHeight, 0.1f,-10.f);
        //glm::mat4 model = glm::rotate( glm::mat4(), time, glm::vec3(0,1,0));
        glm::mat4 model = glm::rotate( glm::mat4(), 0.0f, glm::vec3(0,1,0));
        
        glUniformMatrix4fv( cViewID, 1, GL_FALSE, glm::value_ptr(view) );
        glUniformMatrix4fv( cProjectionID, 1, GL_FALSE, glm::value_ptr(proj) );
        glUniformMatrix4fv( cModelID, 1, GL_FALSE, glm::value_ptr(model) );
        
        BINDVERTEXARRAY(cArrayID);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, cElementID);

        glDrawElements(GL_QUADS, 24, GL_UNSIGNED_BYTE, 0);
        
        BINDVERTEXARRAY(0);
    }
    
    
    void customDraw(float time) {
    
        glUseProgram( cShader->id() );
        
        drawEye(true, time);
        drawEye(false, time);
        

        glUseProgram(0);
        
    }
    
    void beforeDraw() {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &screenBufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    }
    
    void drawDistortion() {
        
        glUseProgram( shader->id() );                   //<-- 1. Bind Shader
        glBindTexture( GL_TEXTURE_2D, textureID );      //<-- 2. Bind Texture, the texture is rendered from custom enderer
        
        glViewport(0, 0, viewportWidth, viewportHeight);
        glScissor(0, 0, viewportWidth, viewportHeight);
        glClearColor(0.25, 0.25, 0.25, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        BINDVERTEXARRAY(arrayID1);              //<-- 3. Bind VAO
        glDrawElements(GL_TRIANGLE_STRIP, indices.size(), GL_UNSIGNED_SHORT, 0);
        //GL_TRIANGLE_STRIP
        
        glViewport(viewportWidth, 0, viewportWidth, viewportHeight);
        glScissor(viewportWidth, 0, viewportWidth, viewportHeight);
        glClearColor(0.25, 0.25, 0.25, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        BINDVERTEXARRAY(arrayID2);              //<-- 3. Bind VAO
        glDrawElements(GL_TRIANGLE_STRIP, indices.size(), GL_UNSIGNED_SHORT, 0);
        
        //glDrawElements(GL_LINE_STRIP, indices.size(), GL_UNSIGNED_SHORT, 0);
        //glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);

        glUseProgram(0);
    }
    
    void afterDraw() {
        
        glBindFramebuffer(GL_FRAMEBUFFER, screenBufferID);
        drawDistortion();
    }
    
};


int main(int argc, const char * argv[]){
    
    MyApp app;
    app.start();
    
    return 0;
}
