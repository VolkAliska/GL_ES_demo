#include <iostream>
#include "esUtil.h"
#include <pthread.h>
#include <sstream>
#include <iomanip>

#include "windows.h"

using namespace std;

char* myIterator;
typedef struct
{
   GLuint programObject;
   GLint  positionLoc;
   GLint  mvpLoc;
   GLint  colorLoc;
   GLint  texCoordLoc;
   GLint  samplerLoc;

//    GLfloat  *vertices;
//    GLfloat  *texCoords;
//    GLuint   *indices;

// IMAGE

    GLfloat *imgCoords;
    GLuint  *imgIndices;
    int numImgIndices;


//     int numVertices;
//    int       numIndices;
//    int       numIndicesColor;

   GLfloat   angle;
   ESMatrix  mvpMatrix;

   GLuint textureImage;
   GLuint textureGreen;
   GLuint textureBlack;

} UserData;

GLuint createTexture(char *textureName )
{
  int width, height;
   char *buffer = esLoadTGA (textureName, &width, &height );
   GLuint texId;

   if ( buffer == NULL )
   {
      esLogMessage ( "Error loading (%s) image\n", textureName );
      return 0;
   }

   glGenTextures ( 1, &texId );
   glBindTexture ( GL_TEXTURE_2D, texId );

   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); 
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   free ( buffer );

   return texId;
}

int Init ( ESContext *esContext )
{
   UserData *userData = (UserData*)esContext->userData;
    int win_width = esContext->width;
    int win_height = esContext->height;

    GLbyte vShaderStr[] =  
      "uniform mat4 u_mvpMatrix;                   \n" 
      "attribute vec4 vPosition;                   \n" 
      "attribute vec2 a_texCoord;                  \n" 
      "varying vec2 v_texCoord;                    \n" 
      "void main()                                 \n"
      "{                                           \n"
      "   gl_Position = u_mvpMatrix * vPosition;   \n"
      "   v_texCoord = a_texCoord;                 \n"
      "}                                           \n";
   
    GLbyte fShaderStr[] =  
      "precision mediump float;                                       \n"
      "varying vec2 v_texCoord;                                       \n" 
      "uniform sampler2D s_texture;                                   \n"
      "void main()                                                    \n"
      "{                                                              \n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord ) ; \n" 
      "}                                                              \n";

   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   userData->positionLoc = glGetAttribLocation ( userData->programObject, "vPosition" );
   userData->texCoordLoc = glGetAttribLocation ( userData->programObject, "a_texCoord" );
   // Get the sampler location
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );
   userData->mvpLoc = glGetUniformLocation( userData->programObject, "u_mvpMatrix" );
   userData->colorLoc = glGetUniformLocation ( userData->programObject, "u_color" );


    //__________________ IMAGE ___________________

    GLfloat imageCoords[20]=
    {
        -0.1f, -0.4f, 0.0f,
        0.0f, 1.0f,
        -0.1f, 1.15f, 0.0f,
        0.0f, 0.0f,
        1.5f, 1.15f, 0.0f,
        1.0f, 0.0f,
        1.5f, -0.4, 0.0f,
        1.0f, 1.0f
    };

    userData->imgCoords = malloc ( sizeof(GLfloat) * 20 );
    memcpy(  userData->imgCoords, imageCoords, sizeof(GLfloat) * 20);

    GLuint imgInd[6]=
    {
        0, 1, 2,
        2, 3, 0
    };

    userData->imgIndices = malloc ( sizeof(GLuint) * 6 );
    memcpy(  userData->imgIndices, imgInd, sizeof(GLuint) * 6);

    userData->numImgIndices = 6;

    for (int i = 0 ; i < 20; i++){
        printf("\n %f", userData->imgCoords[i]);
    }
    for (int i = 0 ; i < 6; i++){
        printf("\n %d", userData->imgIndices[i]);
    }

    //__________________ IMAGE END ___________________


    userData->textureImage = createTexture("tex/backTexture.tga");
    userData->angle = 0.0f;
    glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
    return TRUE;
}


void Draw ( ESContext *esContext )
{
   UserData *userData = (UserData*)esContext->userData;
   glViewport ( 0, 0, esContext->width, esContext->height );
   
   // Enable depth test
   glEnable(GL_DEPTH_TEST);
   // Accept fragment if it closer to the camera than the former one
   glDepthFunc(GL_LESS);
   
   glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
   glUseProgram ( userData->programObject );

   //__________________ IMAGE ___________________

   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 5 * sizeof(GLfloat), userData->imgCoords );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 5 * sizeof(GLfloat),  &userData->imgCoords[3]);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   glUniformMatrix4fv( userData->mvpLoc, 1, GL_FALSE, (GLfloat*) &userData->mvpMatrix.m[0][0] );
 
    //--------------------
   glActiveTexture ( GL_TEXTURE0 ); // текстурный блок 
   glBindTexture ( GL_TEXTURE_2D, userData->textureImage );
   glUniform1i ( userData->samplerLoc, 0 );
   //--------------------

    glDrawElements( GL_TRIANGLES, userData->numImgIndices, GL_UNSIGNED_INT, userData->imgIndices );

    //__________________ IMAGE END ___________________

   eglSwapBuffers ( esContext->eglDisplay, esContext->eglSurface );
}


void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = (UserData*) esContext->userData;
   ESMatrix perspective;
   ESMatrix modelview;
   float    aspect;
   
   // userData->angle += ( deltaTime * 40.0f );
   // if( userData->angle >= 360.0f )
   //    userData->angle -= 360.0f;

   aspect = (GLfloat) esContext->width / (GLfloat) esContext->height;
   esMatrixLoadIdentity( &perspective );
   esPerspective( &perspective, 60.0f, aspect, 1.0f, 30.0f );

   esMatrixLoadIdentity( &modelview );

   // Translate away from the viewer
   esTranslate( &modelview, 0.0, 0.0, -2.0 );

   //esRotate( &modelview, userData->angle, 1.0, 1.0, 1.0 );
   
   esMatrixMultiply( &userData->mvpMatrix, &modelview, &perspective );
}

void ShutDown ( ESContext *esContext )
{
   UserData *userData = (UserData*)esContext->userData;

   if ( userData->imgCoords != NULL )
   {
      free ( userData->imgCoords );
   }

   if ( userData->imgIndices != NULL )
   {
      free ( userData->imgIndices );
   }
   glDeleteProgram ( userData->programObject );
}

void* glTask(void* ptr)
{
   ESContext esContext;
   UserData  userData;

   esInitContext ( &esContext );
   esContext.userData = &userData;

   esCreateWindow ( &esContext, "GL ES demo", 640, 460, ES_WINDOW_RGB | ES_WINDOW_DEPTH  );
   
   if ( !Init ( &esContext ) )
      return 0;

   esRegisterDrawFunc ( &esContext, Draw );
   esRegisterUpdateFunc ( &esContext, Update );

   esMainLoop ( &esContext );
   ShutDown ( &esContext );
}

int updateNumber(int i, int sec)
{
 	Sleep(60);

	cout << "task1 says: " << " " << setfill('0') << setw(4) << i << endl;
	std::stringstream buffer;
	buffer << setfill('0') << setw(4) << i;
	const std::string tmp = buffer.str();
	const char* cstr = tmp.c_str();
	myIterator = (char*)malloc(10);
	memcpy(myIterator, cstr, 10);

	return 0;
}

int main(int argc, char *argv[])
{
 	pthread_t glThread;
	pthread_create(&glThread, NULL, glTask, NULL);

	for (int i = 0; i < 1000; ++i)
	{
	     //updateNumber(i, 1);
	}

	pthread_join(glThread, NULL);
}