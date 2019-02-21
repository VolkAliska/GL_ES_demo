#include <stdio.h>
#include "esUtil.h"
#include <pthread.h>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "windows.h"

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
   GLuint *frameIndices;

// TEXT 

   int numTextIndices; // text
   int numTextVertices;
   GLfloat  *textVertices;
   GLfloat  *textTexCoords;
   GLuint   *textIndices;

   GLfloat  *contVertices; // text container
   GLfloat  *contTexCoords;
   GLuint   *contIndices;

// text box

   int numTBIndices; // text
   int numTBVertices;
   GLfloat  *tbVertices;
   GLfloat  *tbTexCoords;
   GLuint   *tbIndices;

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

int frameVert(GLfloat** frameVerts, GLuint** frameInd, GLfloat* points)
{
    GLfloat shX = 0.02f;
    GLfloat shY = 0.02f;
    int numVert = 12 * 5;
    int numInd = 24;
    GLfloat frCoords[numVert]=
    {
        points[0], points[1], points[2], // 0
        0.0f, 0.0f,
        points[3], points[4], points[5], // 1
        0.0f, 1.0f,
        points[6], points[7], points[8], // 2
        1.0f, 1.0f,
        points[9], points[10], points[11], // 3
        1.0f, 0.0f,

        points[0] + shX, points[1], points[2], // 10
        1.0f, 0.0f,
        points[3] + shX, points[4], points[5], // 4
        1.0f, 1.0f,
        points[6] - shX, points[7], points[8], // 5
        0.0f, 1.0f,
        points[9] - shX, points[10], points[11], // 11
        0.0f, 0.0f,

        points[0] + shX, points[1] + shY, points[2], // 8
        1.0f, 1.0f,
        points[3] + shX, points[4] - shY, points[5], // 6
        1.0f, 0.0f,
        points[6] - shX, points[7] - shY, points[8], // 7
        1.0f, 0.0f,
        points[9] - shX, points[10] + shY, points[11], // 9
        1.0f, 1.0f
    };
    
    *frameVerts = malloc (sizeof(GLfloat)*numVert);
    memcpy(*frameVerts, frCoords, sizeof(GLfloat)*numVert);

    GLuint frInds[numInd] = {
        0, 1, 4,
        1, 5, 4,
        
        5, 6, 10,
        10, 9, 5,
        
        7, 6, 2,
        2, 3, 7,

        7, 11, 8,
        7, 4, 8
    };

    *frameInd = malloc (sizeof(GLfloat)*numInd);
    memcpy(*frameInd, frInds, sizeof(GLfloat)*numInd);

    return 0;
}

int makeText( int numInd, int numVert, GLuint** inds, GLfloat** verts, GLfloat* points)
{
   GLfloat y_min = points[0],
            y_max = points[1],
            x_min = points[2],
            x_max = points[3];
        
    GLfloat firstBlock = x_min;
    
    int numBlocks = numVert / 4;
    
    GLfloat xlen = x_max - x_min;
    GLfloat ylen = y_max - y_min;
    GLfloat blockLen = xlen / numBlocks;

    GLfloat w = blockLen * numBlocks; // для  координат - зависит ширина блока
     
    GLfloat blocksVerts[numVert*3];
    GLfloat shiftY = (ylen - blockLen) / 2.0;

    for ( int i = 0; i < numBlocks; i++ )
    {
        blocksVerts[0 + 12*i] = (firstBlock + (float)(w/numBlocks * i));       // 0  x
        blocksVerts[1 + 12*i] = y_min + shiftY;                               //    y
        blocksVerts[2 + 12*i] = 0.01f; 
        
        blocksVerts[3 + 12*i] = (firstBlock + (float)(w/numBlocks * i));       // 1
        blocksVerts[4 + 12*i] = y_max - shiftY;
        blocksVerts[5 + 12*i] = 0.01f;

        blocksVerts[6 + 12*i] = (float)(firstBlock + (w/numBlocks * (i + 1))); // 2
        blocksVerts[7 + 12*i] = y_max - shiftY;
        blocksVerts[8 + 12*i] = 0.01f;

        blocksVerts[9 + 12*i] = (float)(firstBlock + (w/numBlocks * (i + 1))); // 3
        blocksVerts[10 + 12*i] = y_min + shiftY;
        blocksVerts[11 + 12*i] = 0.01f;
    }

    
    *verts = malloc ( sizeof(GLfloat) * 3 * numVert);
    memcpy( *verts, blocksVerts, sizeof(GLfloat) * 3 * numVert );

    GLuint sqIndices[numInd];

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
    
    *inds = malloc ( sizeof(GLuint) * numInd );
    memcpy( *inds, sqIndices, sizeof(GLuint) * numInd );
    
    return 0;
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
        -0.2f, -0.65f, -0.6f,
        -0.1f, 1.0f,
        -0.2f, 1.25f, -0.6f,
        -0.1f, 0.0f,
        1.8f, 1.25f, -0.6f,
        1.0f, 0.0f,
        1.8f, -0.65, -0.6f,
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

    //__________________ IMAGE END ___________________

    //__________________ FRAME ___________________

    GLfloat points[3*4] = {
        -1.90f, -1.34f, -0.56f,
        -1.90f, -0.1f, -0.56f,
        -0.1f, -0.1f, -0.56f,
        -0.1f, -1.34, -0.56f
    };

    frameVert(&userData->frameCoords, &userData->frameIndices, points);

    //__________________ FRAME END ___________________

   //  //__________________ TEXT _______________________
      
   userData->numTextIndices = 6;
   GLfloat y_min = -0.7f,
            y_max = -0.4f,
            x_min = -1.20f,
            x_max = -0.5f;
            
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
        x_min, y_min, 0.01f,
        x_min, y_min + shiftY, 0.01f,
        x_max, y_min + shiftY, 0.01f,
        x_max, y_min, 0.01f,

        x_min, y_max - shiftY, 0.01f,
        x_min, y_max, 0.01f,
        x_max, y_max, 0.01f,
        x_max, y_max - shiftY, 0.01f
    };

    // объем для вертелки

    GLfloat contTBVert[24]=
    {
        x_min, y_min, -0.03f, //far
        x_max, y_min, -0.03f,
        x_min, y_max, -0.03f,
        x_max, y_max, -0.03f,
        x_min, y_min, 0.01f, // near
        x_max, y_min, 0.01f,
        x_min, y_max, 0.01f,
        x_max, y_max, 0.01f
    };

    userData->tbVertices = malloc ( sizeof(GLfloat) * 24 );
    memcpy(  userData->tbVertices, contTBVert, sizeof(GLfloat) * 24);

    GLfloat tbTex[16];
    for (int i = 0; i < 16; i++){
        tbTex[i] = 0.0f;
    }

    userData->tbTexCoords = malloc ( sizeof(GLfloat) * 16 );
    memcpy(  userData->tbTexCoords, tbTex, sizeof(GLfloat) * 16);

    GLuint tbInd[30]=
    {
        0, 2, 3, // far plate
        3, 1, 0,
        4, 6, 2, // left
        2, 0, 4,
        6, 2, 3, // top
        3, 7, 6,
        7, 3, 1, // right
        1, 5, 7,
        1, 5, 4, // bottom
        4, 0, 1
    };

    userData->tbIndices = malloc ( sizeof(GLuint) * 30 );
    memcpy(  userData->tbIndices, tbInd, sizeof(GLuint) * 30);

    // кончился объем для вертелки

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

    GLfloat textPoints[4] = 
    {
        y_min, y_max, 
        x_min, x_max
    };

    makeText( userData->numTextIndices, userData->numTextVertices, &userData->textIndices, &userData->textVertices, textPoints);

    //__________________ TEXT END ___________________

    //__________________ CAMERA _______________________

     userData->numCameraIndices = 6;

    GLfloat yCamera_min = 0.98f,
            yCamera_max = 1.05f,
            xCamera_min = -1.5f,
            xCamera_max = -1.0f;
            
    GLuint numCameraBlocks = 1;
    numCameraBlocks = symCount(camText);
    userData->numCameraIndices *= numCameraBlocks;
    userData->numCameraVertices = 4 * numCameraBlocks;

    GLfloat cameraPoints[4] = 
    {
        yCamera_min, yCamera_max, 
        xCamera_min, xCamera_max
    };

    makeText( userData->numCameraIndices, userData->numCameraVertices, &userData->cameraIndices, &userData->cameraVertices, cameraPoints);

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
            xDate_min = 0.2f,
            xDate_max = 1.4f;
            
    GLuint numDateBlocks = 1;
    numDateBlocks = symCount(timeText);
    userData->numDateIndices *= numDateBlocks;
    userData->numDateVertices = 4 * numDateBlocks;

    GLfloat datePoints[4] = 
    {
        yDate_min, yDate_max, 
        xDate_min, xDate_max
    };

    makeText( userData->numDateIndices, userData->numDateVertices, &userData->dateIndices, &userData->dateVertices, datePoints);

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
        { 0.3f, 0.3f, 0.3f, 1.0f }, // gray
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
    glActiveTexture ( GL_TEXTURE0 ); // текстурный блок 
   glBindTexture ( GL_TEXTURE_2D, userData->textureImage );
   glUniform1i ( userData->samplerLoc, 0 );

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
   glDrawElements ( GL_TRIANGLES, 24, GL_UNSIGNED_INT, userData->frameIndices);

   

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

    // вертелка 
   
   glUniform4fv( userData->pColorLoc, 1, colors[3]);
   glUniform4fv( userData->uColorLoc, 1, colors[3]);

   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 3 * sizeof(GLfloat), userData->tbVertices );
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                            GL_FALSE, 2 * sizeof(GLfloat),  userData->tbTexCoords);                        

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   glUniformMatrix4fv( userData->mvpTextLoc, 1, GL_FALSE, (GLfloat*) &userData->mvpTextMatrix.m[0][0] );

    glDrawElements( GL_TRIANGLES, 30, GL_UNSIGNED_INT, userData->tbIndices ); 

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
   esTranslate( &modelviewText, 0.0 , 0.0, -2.0 );  

   esRotate( &modelviewText, userData->angle, -0.55, -0.4, -0.1 );
   
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

int myiterator(int i, int size, int sec)
{
 	Sleep(sec);
    char buf[size];
    int iter = i;
    for (int t = 0; t < size; t++)
    {
        buf[t] = '0';
    }
    for(int t = 0; t < size; t++)
    {
        int num = iter % 10;
        char sym;
        if (num >= 1)
        {
            itoa(num, &sym, 10);
            buf[size - t - 1] = sym;
        }
        iter /= 10;
    }
    buf[size] = '\0';
    shared_I = (char*)malloc(10);
	memcpy(shared_I, buf, 10);
    printf("\ntask says: ");
    printf(shared_I);
    
	return 0;
}

int main(int argc, char *argv[])
{
 	pthread_t glThread;

	int status = pthread_create(&glThread, NULL, glTask, NULL);
    if (status != 0) {
        printf("main error: can't create thread, status = %d\n", status);
    }

	for (int i = 0; i < 1000; ++i)
	{
        myiterator(i, 4, 250);
	}
	pthread_join(glThread, NULL);
}