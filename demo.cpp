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
   GLint  pColorLoc;
   GLint  uColorLoc;
   GLint  texCoordLoc;
   GLint  samplerLoc;

//    GLfloat  *vertices;
//    GLfloat  *texCoords;
//    GLuint   *indices;

// IMAGE

    GLfloat *imgCoords;
    GLuint  *imgIndices;
    int numImgIndices;

// FRAME

   GLfloat *frameCoords;

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
      "uniform vec4  p_color;                              \n" // перекрыть
      "uniform vec4  u_color;                              \n" // заркасить
      "varying vec2 v_texCoord;                                       \n" 
      "uniform sampler2D s_texture;                                   \n"
      "void main()                                                    \n"
      "{                                                              \n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord ) * p_color + u_color ;          \n" 
      "}                                                              \n";

   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   userData->positionLoc = glGetAttribLocation ( userData->programObject, "vPosition" );
   userData->texCoordLoc = glGetAttribLocation ( userData->programObject, "a_texCoord" );
   // Get the sampler location
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );
   userData->mvpLoc = glGetUniformLocation( userData->programObject, "u_mvpMatrix" );
   userData->pColorLoc = glGetUniformLocation ( userData->programObject, "p_color" );
   userData->uColorLoc = glGetUniformLocation ( userData->programObject, "u_color" );


    //__________________ IMAGE ___________________

    GLfloat imageCoords[20]=
    {
        -0.3f, -0.55f, 0.0f,
        -0.1f, 1.0f,
        -0.3f, 1.15f, 0.0f,
        -0.1f, 0.0f,
        1.5f, 1.15f, 0.0f,
        1.0f, 0.0f,
        1.5f, -0.55, 0.0f,
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

    //__________________ FRAME ___________________

    GLfloat frCoords[40]=
    {
        -1.45f, -1.1f, 0.01f,
        0.0f, 0.0f,
        -1.45f, 0.0f, 0.01f,
        1.0f, 0.0f,
        -0.05f, 0.0f, 0.01f,
        1.0f, 1.0f,
        -0.05f, -1.1, 0.01f,
        0.0f, 1.0f,

        -1.44f, -1.09f, 0.011f,
        0.0f, 0.0f,
        -1.44f, -0.01f, 0.011f,
        1.0f, 0.0f,
        -0.06f, -0.01f, 0.011f,
        1.0f, 1.0f,
        -0.06f, -1.09, 0.011f,
        0.0f, 1.0f
    };

    userData->frameCoords = malloc ( sizeof(GLfloat) * 40 );
    memcpy(  userData->frameCoords, frCoords, sizeof(GLfloat) * 40);
    for (int i = 0 ; i < 40; i++){
        printf("\n %f", userData->frameCoords[i]);
    }

    //__________________ FRAME END ___________________


    userData->textureImage = createTexture("tex/backTexture.tga");
    userData->textureGreen = createTexture("tex/flower.tga");
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

   GLfloat  colors[][4] = { 
       { 1.0f, 1.0f, 1.0f, 0.0f }, // nothing
       { 0.0f, 1.0f, 0.0f, 1.0f }, // green
       { 0.0f, 0.0f, 0.0f, 0.0f }  //black
   };

   //__________________ IMAGE ___________________

   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 5 * sizeof(GLfloat), userData->imgCoords );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 5 * sizeof(GLfloat),  &userData->imgCoords[3]);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );
   glUniform4fv( userData->uColorLoc, 1, colors[2]);
   glUniform4fv( userData->pColorLoc, 1, colors[0]);

   glUniformMatrix4fv( userData->mvpLoc, 1, GL_FALSE, (GLfloat*) &userData->mvpMatrix.m[0][0] );
 
    //--------------------
   glActiveTexture ( GL_TEXTURE0 ); // текстурный блок 
   glBindTexture ( GL_TEXTURE_2D, userData->textureImage );
   glUniform1i ( userData->samplerLoc, 0 );
   //--------------------

    glDrawElements( GL_TRIANGLES, userData->numImgIndices, GL_UNSIGNED_INT, userData->imgIndices );

    //__________________ IMAGE END ___________________

    //__________________ FRAME ___________________

     GLuint frameInd[][6] =
    {
       {0, 1, 2,
       2, 3, 0},
      {4, 5, 6,
       6, 7, 4}
    };

   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 5 * sizeof(GLfloat), userData->frameCoords );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 5 * sizeof(GLfloat),  &userData->frameCoords[3]);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   glUniform4fv( userData->pColorLoc, 1, colors[1]);
   glUniform4fv( userData->uColorLoc, 1, colors[1]);
   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, frameInd[0] );

   
   glUniform4fv( userData->pColorLoc, 1, colors[2]);
   glUniform4fv( userData->uColorLoc, 1, colors[2]);
   glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, frameInd[1] );

    //__________________ FRAME END ___________________


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

   esCreateWindow ( &esContext, "GL ES demo", 620, 440, ES_WINDOW_RGB | ES_WINDOW_DEPTH  );
   
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