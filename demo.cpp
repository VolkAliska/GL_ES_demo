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

// TEXT 

   int numTextIndices; // text
   int numTextVertices;
   GLfloat  *textVertices;
   GLfloat  *textTexCoords;
   GLuint   *textIndices;

   GLfloat  *contVertices; // text container
   GLfloat  *contTexCoords;
   GLuint   *contIndices;

//     int numVertices;
//    int       numIndices;
//    int       numIndicesColor;

   GLfloat   angle;
   ESMatrix  mvpMatrix;

   GLuint textureImage;
   GLuint textureGreen;
   GLuint textureBlack;

} UserData;

char* shared_I;


int getTexCoords ( GLfloat **texCoords, const GLuint numVertices/*, char *text*/)
{
    
    const int shWidth = 7,
           shHeigth = 6;

   // отображение в коде позиций символов на изображении-текстуре
    char fontSheet[shHeigth][shWidth] =
   {
       {'A', 'B', 'C', 'D', 'E', 'F', '_'},
       {'G', 'H', 'I', 'J', 'K', 'L', '.'},
       {'M', 'N', 'O', 'P', 'Q', 'R', '!'},
       {'S', 'T', 'U', 'V', 'W', 'X', '?'},
       {'Y', 'Z', '0', '1', '2', '3', '@'},
       {'4', '5', '6', '7', '8', '9', '#'}
   };

//   char fontSheet[shHeigth][shWidth] =
//    {
//        {'A', 'B', 'C', 'D', 'E', 'F'},
//        {'G', 'H', 'I', 'J', 'K', 'L'},
//        {'M', 'N', 'O', 'P', 'Q', 'R'},
//        {'S', 'T', 'U', 'V', 'W', 'X'},
//        {'Y', 'Z', '0', '1', '2', '3'},
//        {'4', '5', '6', '7', '8', '9'}
//    };

//     char fontSheet[shHeigth][shWidth] =
//    {
//        {'A', 'B', 'V', 'G', 'D', 'E'},
//        {'1', 'J', 'Z', 'I', '2', 'K'},
//        {'L', 'M', 'N', 'O', 'P', 'R'},
//        {'S', 'T', 'U', 'F', 'X', 'C'},
//        {'4', 'W', '5', '6', '7', '8'},
//        {'4', 'Y', '6', '_', '0', '9'}
//    };
    const GLuint texCount = numVertices * 2;
    GLfloat tex[numVertices * 2];
    char symbol;
    int numBlocks = numVertices / 4;
   for (int i = 0; i < numBlocks; i++)
   {
        // координаты вершин квадрата с нужным символом
        GLfloat s_max, s_min, t_max, t_min;
        char* text = shared_I;
        // смещение по осям s, t считается по индексу позиции символа на текстуре
        GLuint s_delta, t_delta;
        if(text[i] == '\0')
            break;
        
        symbol = text[i];
        GLuint isSymbolAble = 0;

        for ( int k = 0; k < 6; k++ )
            for ( int j = 0; j < 6; j++ )
                if ( symbol == fontSheet[k][j])
                {
                    s_delta = j;
                    t_delta = k;
                    isSymbolAble = 1;
                }
                    
        if (!isSymbolAble)
        {
            printf("\nEnable symbol: %c\n", symbol);
            break;
        }
        // текстурные координаты вычисляются разбиванием их на равное количество секций 
        // (например, на равные прямоугольники - по прямоугольнику для каждого символа)
        // и задается смещение - какой номер блока по осям s, t нужно отобразить
        
        s_max = 1.0 / shWidth * (s_delta + 1);
        s_min = 1.0 / shWidth * s_delta;
        t_max = 1.0 / shHeigth * (t_delta + 1);
        t_min = 1.0 / shHeigth * t_delta;

        tex[0 + 8*i] = s_min;
        tex[1 + 8*i] = t_max;
        
        tex[2 + 8*i] = s_min; //4
        tex[3 + 8*i] = t_min;

        tex[4 + 8*i] = s_max; //5
        tex[5 + 8*i] = t_min;

        tex[6 + 8*i] = s_max; //6
        tex[7 + 8*i] = t_max;

        
   }

       *texCoords = malloc ( sizeof(GLfloat) * 2 * numVertices );
       memcpy(  *texCoords, tex, sizeof(GLfloat) * 2 * numVertices);
       return 0;
}

int symCount ( char *text )
{
    GLuint symCount = 0;
    for ( int i = 0; i < 10; i++)
    {
        if(text[i] != '\0')
            symCount++;
        else
            break;
    }
    return symCount;
}

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
        -1.45f, -1.1f, 0.001f,
        0.0f, 0.0f,
        -1.45f, 0.0f, 0.001f,
        1.0f, 0.0f,
        -0.05f, 0.0f, 0.001f,
        1.0f, 1.0f,
        -0.05f, -1.1, 0.001f,
        0.0f, 1.0f,

        -1.44f, -1.09f, 0.002f,
        0.0f, 0.0f,
        -1.44f, -0.01f, 0.002f,
        1.0f, 0.0f,
        -0.06f, -0.01f, 0.002f,
        1.0f, 1.0f,
        -0.06f, -1.09, 0.002f,
        0.0f, 1.0f
    };

    userData->frameCoords = malloc ( sizeof(GLfloat) * 40 );
    memcpy(  userData->frameCoords, frCoords, sizeof(GLfloat) * 40);
    for (int i = 0 ; i < 40; i++){
        printf("\n %f", userData->frameCoords[i]);
    }

    //__________________ FRAME END ___________________

   //  //__________________ TEXT _______________________
      
   userData->numTextIndices = 6;
   printf("kek");
   GLfloat y_min = -0.75f,
            y_max = -0.35f,
            x_min = -1.15f,
            x_max = -0.35f;
            
    GLuint numBlocks = 1;
    GLfloat firstBlock = x_min;
    //printf("\nEnter text: ");
    char *text = shared_I;
    //gets(text);
    numBlocks = symCount(text);
    
    GLfloat xlen = x_max - x_min;
    GLfloat ylen = y_max - y_min;
    GLfloat blockLen = xlen / numBlocks;

    userData->numTextIndices *= numBlocks;
    userData-> numTextVertices = 4 * numBlocks;

    GLfloat w = 1.6f; // для текстурных координат
    
    GLfloat blocksVerts[userData->numTextVertices*3];
    GLfloat shiftY = (ylen - blockLen) / 2.0;

    // координаты контейнера ----

    
    GLfloat contVert[24]=
    {
        x_min, y_min, 0.0f,
        x_min, y_min + shiftY, 0.0f,
        x_max, y_min + shiftY, 0.0f,
        x_max, y_min, 0.0f,

        x_min, y_max - shiftY, 0.0f,
        x_min, y_max, 0.0f,
        x_max, y_max, 0.0f,
        x_max, y_max - shiftY, 0.0f
    };

    userData->contVertices = malloc ( sizeof(GLfloat) * 24 );
    memcpy(  userData->contVertices, contVert, sizeof(GLfloat) * 24);

    GLfloat contTex[16];
    for (int i = 0; i < 16; i++){
        contTex[i] = 0.0f;
    }

    userData->contTexCoords = malloc ( sizeof(GLfloat) * 16 );
    memcpy(  userData->contTexCoords, contTex, sizeof(GLfloat) * 16);

    GLuint contInd[12]=
    {
        0, 1, 2,
        2, 3, 0,
        4, 5, 6,
        6, 7, 4
    };

    userData->contIndices = malloc ( sizeof(GLuint) * 12 );
    memcpy(  userData->contIndices, contInd, sizeof(GLuint) * 12);
    
    // координаты для текста ----

    for ( int i = 0; i < numBlocks; i++ )
    {
        blocksVerts[0 + 12*i] = (firstBlock + (float)(w/numBlocks * i));       // 0  x
        blocksVerts[1 + 12*i] = y_min + shiftY;                               //    y
        blocksVerts[2 + 12*i] = 0.0004f; 
        
        blocksVerts[3 + 12*i] = (firstBlock + (float)(w/numBlocks * i));       // 1
        blocksVerts[4 + 12*i] = y_max - shiftY;
        blocksVerts[5 + 12*i] = 0.0004f;

        blocksVerts[6 + 12*i] = (float)(firstBlock + (w/numBlocks * (i + 1))); // 2
        blocksVerts[7 + 12*i] = y_max - shiftY;
        blocksVerts[8 + 12*i] = 0.0004f;

        blocksVerts[9 + 12*i] = (float)(firstBlock + (w/numBlocks * (i + 1))); // 3
        blocksVerts[10 + 12*i] = y_min + shiftY;
        blocksVerts[11 + 12*i] = 0.0004f;
    }
    
    userData->textVertices = malloc ( sizeof(GLfloat) * 3 * userData->numTextVertices );
    memcpy( userData->textVertices, blocksVerts, sizeof(GLfloat) * 3 * userData->numTextVertices  );

    GLuint sqIndices[userData->numTextIndices];

    for ( int i = 0; i < numBlocks; i++ )
    {
        int buf = 4 * i;
        sqIndices[0 + 6*i] = buf; // 1 triangle
        sqIndices[1 + 6*i] = buf + 1;
        sqIndices[2 + 6*i] = buf + 2;

        sqIndices[3 + 6*i] = buf; // 2 triangle
        sqIndices[4 + 6*i] = buf + 2;
        sqIndices[5 + 6*i] = buf + 3;

    }

    //getTexCoords( &userData->texCoords, numVertices/*, text*/);
    
    userData->textIndices = malloc ( sizeof(GLuint) * userData->numTextIndices );
    memcpy( userData->textIndices, sqIndices, sizeof(GLuint) * userData->numTextIndices );

   //  //__________________ TEXT END ___________________

    userData->textureImage = createTexture("tex/backTexture.tga");
    userData->textureBlack = createTexture("tex/with_back.tga");
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

    // __________________ TEXT _______________________
    
   
   
    glActiveTexture ( GL_TEXTURE1 ); // текстурный блок 
   glBindTexture ( GL_TEXTURE_2D, userData->textureBlack );
   glUniform1i ( userData->samplerLoc, 1 );
    glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 3 * sizeof(GLfloat), userData->contVertices );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 2 * sizeof(GLfloat),  userData->contTexCoords);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   glUniformMatrix4fv( userData->mvpLoc, 1, GL_FALSE, (GLfloat*) &userData->mvpMatrix.m[0][0] );

   //--------------------

    glDrawElements( GL_TRIANGLES, 12, GL_UNSIGNED_INT, userData->contIndices );
   //

   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 3 * sizeof(GLfloat), userData->textVertices );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 2 * sizeof(GLfloat),  userData->textTexCoords);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );
   
   glDrawElements( GL_TRIANGLES, userData->numTextIndices, GL_UNSIGNED_INT, userData->textIndices );

    //__________________ TEXT END ___________________

   eglSwapBuffers ( esContext->eglDisplay, esContext->eglSurface );
}


void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = (UserData*) esContext->userData;
   ESMatrix perspective;
   ESMatrix modelview;
   float    aspect;
   getTexCoords( &userData->textTexCoords, userData->numTextVertices/*, text*/);
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
 	Sleep(100);

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
	     updateNumber(i, 1);
	}

	pthread_join(glThread, NULL);
}