#include <iostream>
#include "esUtil.h"
#include <pthread.h>
#include <sstream>
#include <iomanip>
#include <ctime>

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

// CAMERA #

   int numCameraIndices; 
   int numCameraVertices;
   GLfloat  *cameraVertices;
   GLfloat  *cameraTexCoords;
   GLuint   *cameraIndices;

// DATE

   int numDateIndices; 
   int numDateVertices;
   GLfloat  *dateVertices;
   GLfloat  *dateTexCoords;
   GLuint   *dateIndices;

// context
   GLfloat   angle;
   ESMatrix  mvpMatrix;

// text
    ESMatrix mvpTextMatrix;
    GLint mvpTextLoc;

   GLuint textureImage;
   GLuint textureGreen;
   GLuint textureBlack;

} UserData;

char* shared_I;
const char* camText = "CAMERA1";
char* timeText;


int getTexCoords ( GLfloat **texCoords, const GLuint numVertices, char *text)
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
        //char* text = shared_I;
        // смещение по осям s, t считается по индексу позиции символа на текстуре
        GLuint s_delta, t_delta;
        if(text[i] == '\0')
            break;
        
        symbol = text[i];
        GLuint isSymbolAble = 0;

        for ( int k = 0; k < shHeigth; k++ )
            for ( int j = 0; j < shWidth; j++ )
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
    for ( int i = 0; i < 30; i++)
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

    // for (int i = 0 ; i < 20; i++){
    //     printf("\n %f", userData->imgCoords[i]);
    // }
    // for (int i = 0 ; i < 6; i++){
    //     printf("\n %d", userData->imgIndices[i]);
    // }

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
    // for (int i = 0 ; i < 40; i++){
    //     printf("\n %f", userData->frameCoords[i]);
    // }

    //__________________ FRAME END ___________________

   //  //__________________ TEXT _______________________
      
   userData->numTextIndices = 6;
   GLfloat y_min = -0.80f,
            y_max = -0.30f,
            x_min = -1.35f,
            x_max = -0.15f;
            
    GLuint numBlocks = 1;
    GLfloat firstBlock = x_min;
    //printf("\nEnter text: ");
    char *text = shared_I;
    //gets(text);
    numBlocks = symCount(text);
    printf("numbl %d", numBlocks);
    
    GLfloat xlen = x_max - x_min;
    GLfloat ylen = y_max - y_min;
    GLfloat blockLen = xlen / numBlocks;

    userData->numTextIndices *= numBlocks;
    userData-> numTextVertices = 4 * numBlocks;

    GLfloat w = blockLen * numBlocks; // для  координат - зависит ширина блока
     
    GLfloat blocksVerts[userData->numTextVertices*3];
    GLfloat shiftY = (ylen - blockLen) / 2.0;

    // координаты контейнера ----

    
    GLfloat contVert[24]=
    {
        x_min, y_min, 0.004f,
        x_min, y_min + shiftY, 0.004f,
        x_max, y_min + shiftY, 0.004f,
        x_max, y_min, 0.004f,

        x_min, y_max - shiftY, 0.004f,
        x_min, y_max, 0.004f,
        x_max, y_max, 0.004f,
        x_max, y_max - shiftY, 0.004f
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
        6, 7, 4,
    };

    userData->contIndices = malloc ( sizeof(GLuint) * 12 );
    memcpy(  userData->contIndices, contInd, sizeof(GLuint) * 12);

    // for  (int i =0; i < 24; i++){
    //     printf("\n contv %f", userData->contVertices[i]);
    // }
    // for  (int i =0; i < 16; i++){
    //     printf("\n contt %f", userData->contTexCoords[i]);
    // }
    // for  (int i =0; i < 12; i++){
    //     printf("\n conti %d", userData->contIndices[i]);
    // }
    
    // координаты для текста ----

    for ( int i = 0; i < numBlocks; i++ )
    {
        blocksVerts[0 + 12*i] = (firstBlock + (float)(w/numBlocks * i));       // 0  x
        blocksVerts[1 + 12*i] = y_min + shiftY;                               //    y
        blocksVerts[2 + 12*i] = 0.004f; 
        
        blocksVerts[3 + 12*i] = (firstBlock + (float)(w/numBlocks * i));       // 1
        blocksVerts[4 + 12*i] = y_max - shiftY;
        blocksVerts[5 + 12*i] = 0.004f;

        blocksVerts[6 + 12*i] = (float)(firstBlock + (w/numBlocks * (i + 1))); // 2
        blocksVerts[7 + 12*i] = y_max - shiftY;
        blocksVerts[8 + 12*i] = 0.004f;

        blocksVerts[9 + 12*i] = (float)(firstBlock + (w/numBlocks * (i + 1))); // 3
        blocksVerts[10 + 12*i] = y_min + shiftY;
        blocksVerts[11 + 12*i] = 0.004f;
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
    // for  (int i =0; i < 48; i++){
    //     printf("\n textv %f", userData->textVertices[i]);
    // }
    // for  (int i =0; i < 24; i++){
    //     printf("\n texti %d", userData->textIndices[i]);
    // }
    //__________________ TEXT END ___________________

    //__________________ CAMERA _______________________

     userData->numCameraIndices = 6;

    GLfloat yCamera_min = 0.98f,
            yCamera_max = 1.05f,
            xCamera_min = -1.5f,
            xCamera_max = -1.0f;
            
    GLuint numCameraBlocks = 1;
    GLfloat firstCameraBlock = xCamera_min;
    
    numCameraBlocks = symCount(camText);
    // printf("\n numbl CAMERA %d", numCameraBlocks);
    
    GLfloat xCameralen = xCamera_max - xCamera_min;
    GLfloat yCameralen = yCamera_max - yCamera_min;
    GLfloat blockCameraLen = xCameralen / numCameraBlocks;

    userData->numCameraIndices *= numCameraBlocks;
    userData->numCameraVertices = 4 * numCameraBlocks;

    GLfloat wCamera = blockCameraLen * numCameraBlocks; // для  координат - зависит ширина блока
     
    GLfloat blocksCamera[userData->numCameraVertices*3];
    GLfloat shiftCameraY = (yCameralen - blockCameraLen) / 2.0;

    for ( int i = 0; i < numCameraBlocks; i++ )
    {
        blocksCamera[0 + 12*i] = (firstCameraBlock + (float)(wCamera/numCameraBlocks * i));       // 0  x
        blocksCamera[1 + 12*i] = yCamera_min + shiftCameraY;                               //    y
        blocksCamera[2 + 12*i] = 0.004f; 
        
        blocksCamera[3 + 12*i] = (firstCameraBlock + (float)(wCamera/numCameraBlocks * i));       // 1
        blocksCamera[4 + 12*i] = yCamera_max - shiftCameraY;
        blocksCamera[5 + 12*i] = 0.004f;

        blocksCamera[6 + 12*i] = (float)(firstCameraBlock + (wCamera/numCameraBlocks * (i + 1))); // 2
        blocksCamera[7 + 12*i] = yCamera_max - shiftCameraY;
        blocksCamera[8 + 12*i] = 0.004f;

        blocksCamera[9 + 12*i] = (float)(firstCameraBlock + (wCamera/numCameraBlocks * (i + 1))); // 3
        blocksCamera[10 + 12*i] = yCamera_min + shiftCameraY;
        blocksCamera[11 + 12*i] = 0.004f;
    }
    

    userData->cameraVertices = malloc ( sizeof(GLfloat) * 3 * userData->numCameraVertices );
    memcpy( userData->cameraVertices, blocksCamera, sizeof(GLfloat) * 3 * userData->numCameraVertices  );

    GLuint camIndices[userData->numCameraIndices];

    // printf("\n indeces: %d", userData->numCameraIndices);

    for ( int i = 0; i < numCameraBlocks; i++ )
    {
        int buf = 4 * i;
        camIndices[0 + 6*i] = buf; // 1 triangle
        camIndices[1 + 6*i] = buf + 1;
        camIndices[2 + 6*i] = buf + 2;

        camIndices[3 + 6*i] = buf; // 2 triangle
        camIndices[4 + 6*i] = buf + 2;
        camIndices[5 + 6*i] = buf + 3;

    }
    //getTexCoords( &userData->texCoords, numVertices/*, text*/);
    
    userData->cameraIndices = malloc ( sizeof(GLuint) * userData->numCameraIndices );
    memcpy( userData->cameraIndices, camIndices, sizeof(GLuint) * userData->numCameraIndices );
  
    // for  (int i =0; i < userData->numCameraIndices; i++){
    //     printf("\n cami %d", userData->cameraIndices[i]);
    // }
    //__________________ CAMERA END ___________________
 
	time_t     now = time(0);
    struct tm  tstruct;
    char       buf[30];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y.%m.%d_%H.%M", &tstruct);
    // cout << endl << "time" << buf << endl;
    timeText = (char*)malloc(30);
	memcpy(timeText, buf, 30);

    //__________________ DATE _______________________

     userData->numDateIndices = 6;

    GLfloat yDate_min = 0.98f,
            yDate_max = 1.05f,
            xDate_min = 0.0f,
            xDate_max = 1.2f;
            
    GLuint numDateBlocks = 1;
    GLfloat firstDateBlock = xDate_min;
    
    numDateBlocks = symCount(timeText);
    // printf("\n numbl CAMERA %d", numCameraBlocks);
    
    GLfloat xDatelen = xDate_max - xDate_min;
    GLfloat yDatelen = yDate_max - yDate_min;
    GLfloat blockDateLen = xDatelen / numDateBlocks;

    userData->numDateIndices *= numDateBlocks;
    userData->numDateVertices = 4 * numDateBlocks;

    GLfloat wDate = blockDateLen * numDateBlocks; // для  координат - зависит ширина блока
     
    GLfloat blocksDate[userData->numDateVertices*3];
    GLfloat shiftDateY = (yDatelen - blockDateLen) / 2.0;

    for ( int i = 0; i < numDateBlocks; i++ )
    {
        blocksDate[0 + 12*i] = (firstDateBlock + (float)(wDate/numDateBlocks * i));       // 0  x
        blocksDate[1 + 12*i] = yDate_min + shiftDateY;                               //    y
        blocksDate[2 + 12*i] = 0.004f; 
        
        blocksDate[3 + 12*i] = (firstDateBlock + (float)(wDate/numDateBlocks * i));       // 1
        blocksDate[4 + 12*i] = yDate_max - shiftDateY;
        blocksDate[5 + 12*i] = 0.004f;

        blocksDate[6 + 12*i] = (float)(firstDateBlock + (wDate/numDateBlocks * (i + 1))); // 2
        blocksDate[7 + 12*i] = yDate_max - shiftDateY;
        blocksDate[8 + 12*i] = 0.004f;

        blocksDate[9 + 12*i] = (float)(firstDateBlock + (wDate/numDateBlocks * (i + 1))); // 3
        blocksDate[10 + 12*i] = yDate_min + shiftDateY;
        blocksDate[11 + 12*i] = 0.004f;
    }
    

    userData->dateVertices = malloc ( sizeof(GLfloat) * 3 * userData->numDateVertices );
    memcpy( userData->dateVertices, blocksDate, sizeof(GLfloat) * 3 * userData->numDateVertices  );

    GLuint dateIndices[userData->numDateIndices];

    // printf("\n indeces: %d", userData->numCameraIndices);

    for ( int i = 0; i < numDateBlocks; i++ )
    {
        int buf = 4 * i;
        dateIndices[0 + 6*i] = buf; // 1 triangle
        dateIndices[1 + 6*i] = buf + 1;
        dateIndices[2 + 6*i] = buf + 2;

        dateIndices[3 + 6*i] = buf; // 2 triangle
        dateIndices[4 + 6*i] = buf + 2;
        dateIndices[5 + 6*i] = buf + 3;

    }
    //getTexCoords( &userData->texCoords, numVertices/*, text*/);
    
    userData->dateIndices = malloc ( sizeof(GLuint) * userData->numDateIndices );
    memcpy( userData->dateIndices, dateIndices, sizeof(GLuint) * userData->numDateIndices );
  
    // for  (int i =0; i < userData->numCameraIndices; i++){
    //     printf("\n cami %d", userData->cameraIndices[i]);
    // }
    //__________________ DATE END ___________________


    userData->textureImage = createTexture("tex/backTexture.tga");
    userData->textureBlack = createTexture("tex/black.tga");
    userData->textureGreen = createTexture("tex/green.tga");
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
       { 0.0f, 0.0f, 0.0f, 0.0f }, // black 0 - полностью прозрачный
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
   
   glUniform4fv( userData->pColorLoc, 1, colors[0]);
   glUniform4fv( userData->uColorLoc, 1, colors[2]);

   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 3 * sizeof(GLfloat), userData->contVertices );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 2 * sizeof(GLfloat),  userData->contTexCoords);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   glUniformMatrix4fv( userData->mvpTextLoc, 1, GL_FALSE, (GLfloat*) &userData->mvpTextMatrix.m[0][0] );

    glDrawElements( GL_TRIANGLES, 12, GL_UNSIGNED_INT, userData->contIndices ); // container

   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 3 * sizeof(GLfloat), userData->textVertices );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 2 * sizeof(GLfloat),  userData->textTexCoords);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );
   
   glDrawElements( GL_TRIANGLES, userData->numTextIndices, GL_UNSIGNED_INT, userData->textIndices );  // text

    //__________________ TEXT END ___________________

    //__________________ CAMERA _______________________

    glActiveTexture ( GL_TEXTURE2 ); // текстурный блок 
   glBindTexture ( GL_TEXTURE_2D, userData->textureGreen );
   glUniform1i ( userData->samplerLoc, 2 );
   
   glUniform4fv( userData->pColorLoc, 1, colors[0]);
   glUniform4fv( userData->uColorLoc, 1, colors[2]);

   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 3 * sizeof(GLfloat), userData->cameraVertices );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 2 * sizeof(GLfloat),  userData->cameraTexCoords);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   glUniformMatrix4fv( userData->mvpLoc, 1, GL_FALSE, (GLfloat*) &userData->mvpMatrix.m[0][0] );

    glDrawElements( GL_TRIANGLES, userData->numCameraIndices, GL_UNSIGNED_INT, userData->cameraIndices );

    //__________________ CAMERA END ___________________

    //__________________ DATE _______________________
 
   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 3 * sizeof(GLfloat), userData->dateVertices );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 2 * sizeof(GLfloat),  userData->dateTexCoords);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   glUniformMatrix4fv( userData->mvpLoc, 1, GL_FALSE, (GLfloat*) &userData->mvpMatrix.m[0][0] );

    glDrawElements( GL_TRIANGLES, userData->numDateIndices, GL_UNSIGNED_INT, userData->dateIndices );

    //__________________ DATE END ___________________

   eglSwapBuffers ( esContext->eglDisplay, esContext->eglSurface );
}


void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = (UserData*) esContext->userData;
   ESMatrix perspective;
   ESMatrix modelview;
   ESMatrix modelviewText;
   float    aspect;
   getTexCoords( &userData->textTexCoords, userData->numTextVertices, shared_I);
   getTexCoords( &userData->cameraTexCoords, userData->numCameraVertices, camText);
   getTexCoords( &userData->dateTexCoords, userData->numDateVertices, timeText);

//    for (int i = 0 ; i < userData->numTextVertices * 2; i++){
//        printf("\n tex %d, %f", i, userData->textTexCoords[i]);
//    }
//    printf("____________________");

   userData->angle += ( deltaTime * 40.0f );
   if( userData->angle >= 360.0f )
      userData->angle -= 360.0f;

   aspect = (GLfloat) esContext->width / (GLfloat) esContext->height;
   esMatrixLoadIdentity( &perspective );
   esPerspective( &perspective, 60.0f, aspect, 1.0f, 30.0f );

   esMatrixLoadIdentity( &modelview );
   esMatrixLoadIdentity( &modelviewText );

   // Translate away from the viewer
   esTranslate( &modelview, 0.0, 0.0, -2.0 );
   esTranslate( &modelviewText, 0.0, 0.0, -2.0 );
   esRotate( &modelviewText, userData->angle, 1.0, 1.0, 1.0 );
   
   esMatrixMultiply( &userData->mvpMatrix, &modelview, &perspective );
   esMatrixMultiply( &userData->mvpTextMatrix, &modelviewText, &perspective );
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
 	Sleep(300);

	cout << "task1 says: " << " " << setfill('0') << setw(4) << i << endl;
	std::stringstream buffer;
	buffer << setfill('0') << setw(4) << i;
	const std::string tmp = buffer.str();
	const char* cstr = tmp.c_str();
	shared_I = (char*)malloc(10);
	memcpy(shared_I, cstr, 10);

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