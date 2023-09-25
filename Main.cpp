#define _CRT_SECURE_NO_WARNINGS
#include <gl/glut.h>
#include <GL/freeglut.h>
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// Global Declaration of variables
// Initialisation of Arrays
GLfloat light[] = { 0.5,0.5,0.5,1 };
GLfloat light_position0[] = { 0,30,20,1.0 };
GLfloat model_ambient[] = { 0.05f,0.05f,0.05f,1.0f };
GLfloat mat_specular[] = { 0.8,1.0,1.0,1.0 };
GLfloat mat_shininess[] = { 5.0 };
GLfloat mat_ambient[] = { 0.1,0.1,0.1,1 };
GLfloat	no_mat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
GLfloat	mat_diffuse1[] = { 0.1f, 0.5f, 0.8f, 1.0f };
GLfloat	no_shininess[] = { 0.0f };

GLint	WinWidth;
GLint	WinHeight;

double limbAngle = 0.0; // Limb Angle for robot

// Declare texture variables
GLuint texSmartBoard, texWindow, texDesk, texCeiling, texDoor, texFloor, texoppwindowWall, texfrontWall, texotherWalls, texChairBase, texChairBaseBeam, texChairLeg, texTableTop, texTableTopBeam, texTableLeg, texPlug;

// Definition of viewpoint for viewer
typedef struct EyePoint
{
	GLfloat	x, y, z;
}

EyePoint;
EyePoint myEye;
EyePoint vPoint;
GLfloat pro_up_down = 29.0f;
GLfloat vAngle = 0;

// Function declarations: 
int is_Num_Pow_2(int n)
{
	if (n <= 0)
		return 0;

	return (n & (n - 1)) == 0;
}

// Functions to load our Bitmaps into textures
// Definition of the length of the Bitmap header 
#define BMP_Header_Length 54
void grab(void)
{
	FILE* pDummyFile; FILE* pWritingFile;
	GLubyte* pPixelData;
	GLubyte BMP_Header[BMP_Header_Length];
	GLint    i, j;
	GLint    PixelDataLength;

	// Calculate the length of the pixel data [Width and Height]
	// Initialise pixel length for each row. Each pixel will consist of RGB colour component [multiplication by 3]
	i = WinWidth * 3;

	// Calculate padding for pixel data [multiples of 4 bytes]
	while (i % 4 != 0)
		++i;

	// Calculate total length of pixel data for image
	PixelDataLength = i * WinHeight;
	// Memory allocation to dynamcially store pixel data
	pPixelData = (GLubyte*)malloc(PixelDataLength);
	if (pPixelData == 0)
		exit(0);
	pDummyFile = fopen("dummy.bmp", "rb");
	if (pDummyFile == 0)
		exit(0);
	pWritingFile = fopen("grab.bmp", "wb");
	if (pWritingFile == 0)
		exit(0);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glReadPixels(0, 0, WinWidth, WinHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, pPixelData);

	// BMP header from dummy bitmap file is written to the grab bitmap file. Ensures output of BMP has proper BMP header.
	fread(BMP_Header, sizeof(BMP_Header), 1, pDummyFile);
	fwrite(BMP_Header, sizeof(BMP_Header), 1, pWritingFile);
	fseek(pWritingFile, 0x0012, SEEK_SET);
	i = WinWidth;
	j = WinHeight;
	fwrite(&i, sizeof(i), 1, pWritingFile);
	fwrite(&j, sizeof(j), 1, pWritingFile);
	fseek(pWritingFile, 0, SEEK_END);
	fwrite(pPixelData, PixelDataLength, 1, pWritingFile);
	fclose(pDummyFile); fclose(pWritingFile); free(pPixelData);
}

// Function that receives a Bitmap as a parameter and returns the relevant texture number
GLuint load_texture(const char* file_name)
{
	GLint width, height, total_bytes;
	GLubyte* pixels = 0;
	GLint last_texture_ID = 0;
	GLuint texture_ID = 0;

	// Open the Bitmap, if it fails to open we return nothing
	FILE* pFile = fopen(file_name, "rb");
	if (pFile == 0)
		return 0;

	// The height and width is read of the opened image
	fseek(pFile, 0x0012, SEEK_SET);
	fread(&width, 4, 1, pFile);
	fread(&height, 4, 1, pFile);
	fseek(pFile, BMP_Header_Length, SEEK_SET);

	// The number of bytes of each line of pixels is calculated, then the total pixel bytes is calculated.
	{
		GLint line_bytes = width * 3;
		while (line_bytes % 4 != 0)
			++line_bytes;
		total_bytes = line_bytes * height;
	}

	// Memory allocation of the total pixel bytes
	pixels = (GLubyte*)malloc(total_bytes);
	if (pixels == 0)
	{
		fclose(pFile);
		return 0;
	}

	// Now read the pixel data
	if (fread(pixels, total_bytes, 1, pFile) <= 0)
	{
		free(pixels);
		fclose(pFile);
		return 0;
	}

	/*
	In the event of the height and width not being integer powers,
	or even if the height and width exceed the maximum height and width
	supported by the current implementation of OpenGL,
	zooming of the image occurs.
	*/
	{
		GLint max;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
		if (!is_Num_Pow_2(width) || !is_Num_Pow_2(height) || width > max || height > max)
		{
			// Set a new width and height after we scale.
			const GLint new_width = 256;
			const GLint new_height = 256;
			GLint new_line_bytes, new_total_bytes;
			GLubyte* new_pixels = 0;

			// The number of bytes of each line of pixels is calculated, then the total pixel bytes is calculated.
			new_line_bytes = new_width * 3;
			while (new_line_bytes % 4 != 0)
				++new_line_bytes;
			new_total_bytes = new_line_bytes * new_height;

			// Memory allocation occurs
			new_pixels = (GLubyte*)malloc(new_total_bytes);
			if (new_pixels == 0)
			{
				free(pixels);
				fclose(pFile);
				return 0;
			}

			// Scale the pixels
			gluScaleImage(GL_RGB, width, height, GL_UNSIGNED_BYTE, pixels, new_width, new_height, GL_UNSIGNED_BYTE, new_pixels);

			// Free old pixel data (orignal) and assign pixels to new pixel data, then set width and height to new width and height
			free(pixels);
			pixels = new_pixels;
			width = new_width;
			height = new_height;
		}
	}

	// New texture number is allocated
	glGenTextures(1, &texture_ID);
	if (texture_ID == 0)
	{
		free(pixels);
		fclose(pFile);
		return 0;
	}

	// Get the texture number of the original binding, so it can be restored, before the new texture is bound.
	// Then the new texture is bound, then load and set texture parameters
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture_ID);
	glBindTexture(GL_TEXTURE_2D, texture_ID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, last_texture_ID);

	// Free previously allocated memory that was for pixels
	free(pixels);

	return texture_ID;
}


// Draw big scene of lab
void drawbigscence()
{
	// Material parameters are set
	glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse1);
	glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
	glMaterialfv(GL_FRONT, GL_SHININESS, no_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	// Ceiling is drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texCeiling);
	glColor3f(0.3, 0.3, 0.3);
	glBegin(GL_QUADS);
	glNormal3f(0.3f, 0.3f, 0.3f);	// Definition of the normal vector
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-40.0f, 30.0f, 30.0f);
	glTexCoord2f(0.0f, 6.0f);
	glVertex3f(-40.0f, 30.0f, -30.0f);
	glTexCoord2f(12.0f, 6.0f);
	glVertex3f(40.0f, 30.0f, -30.0f);
	glTexCoord2f(12.0f, 0.0f);
	glVertex3f(40.0f, 30.0f, 30.0f);
	glEnd();

	// The floor is drawn
	glBindTexture(GL_TEXTURE_2D, texFloor);
	glColor3f(0.8f, 1.0f, 0.8f);
	glBegin(GL_QUADS);
	glNormal3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-40.0f, 0.0f, 40.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-40.0f, 0.0f, -40.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(40.0f, 0.0f, -40.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(40.0f, 0.0f, 40.0f);
	glEnd();

	// Left wall is drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texoppwindowWall);
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_QUADS);
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-40.0f, 0.0f, 30.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-40.0f, 30.0f, 30.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-40.0f, 30.0f, -30.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-40.0f, 0.0f, -30.0f);
	glEnd();

	// Left wall-plug sockets are drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texPlug);
	for (int x = 0; x <= 1; x++)
	{
		glBegin(GL_QUADS);
		glNormal3f(-1.0, 0.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-39.9, 4, 15 + x * -20);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-39.9, 8, 15 + x * -20);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-39.9, 8, 12.5 + x * -20);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-39.9, 4, 12.5 + x * -20);
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);

	// Right wall is drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texotherWalls);
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_QUADS);
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(40.0f, 0.0f, 30.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(40.0f, 30.0f, 30.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(40.0f, 30.0f, -30.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(40.0f, 0.0f, -30.0f);
	glEnd();

	// Right wall-plug sockets are drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texPlug);
	for (int x = 0; x <= 1; x++)
	{
		glBegin(GL_QUADS);
		glNormal3f(-1.0, 0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(39.9, 4, 15 + x * -20);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(39.9, 8, 15 + x * -20);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(39.9, 8, 12.5 + x * -20);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(39.9, 4, 12.5 + x * -20);
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);

	// Windows are drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texWindow);
	for (int n = 0; n <= 1; n++)
	{
		glBegin(GL_QUADS);
		glNormal3f(-1.0, 0.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(39.9, 10, -5 + n * 18);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(39.9, 25, -5 + n * 18);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(39.9, 25, -15 + n * 18);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(39.9, 10, -15 + n * 18);
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);

	// Backwall is drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texotherWalls);
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_QUADS);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-40.0f, 0.0f, 30.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-40.0f, 30.0f, 30.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(40.0f, 30.0f, 30.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(40.0f, 0.0f, 30.0f);
	glEnd();

	// Frontwall is drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texfrontWall);
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_QUADS);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-40.0f, 0.0f, -30.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-40.0f, 30.0f, -30.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(40.0f, 30.0f, -30.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(40.0f, 0.0f, -30.0f);
	glEnd();

	// Smartboard is drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texSmartBoard);
	glBegin(GL_QUADS);
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-15.0, 8.0f, -29.9f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-15.0, 23.0f, -29.9f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(15.0, 23.0f, -29.9f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(15.0, 8.0f, -29.9f);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	// Door is drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texDoor);
	glBegin(GL_QUADS);
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-39.9f, 0.0f, -25.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-39.9f, 23.0f, -25.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-39.9f, 23.0f, -14.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-39.9f, 0.0f, -14.0f);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

// Tables
// Set vertices and textures for Tables
void drawSetTables(int x)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texTableTop);
	glBegin(GL_QUADS);
	// Top Part of Table
	// Back
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-10.0f, -0.2f, 4.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(10.0f, -0.2f, 4.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(10.0f, 0.2f, 4.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-10.0f, 0.2f, 4.0f);
	// Right
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(10.0f, -0.2f, -4.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(10.0f, 0.2f, -4.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(10.0f, 0.2f, 4.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(10.0f, -0.2f, 4.0f);
	// Front
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-10.0f, -0.2f, -4.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-10.0f, 0.2f, -4.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(10.0f, 0.2f, -4.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(10.0f, -0.2f, -4.0f);
	// Left
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-10.0f, -0.2f, -4.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-10.0f, -0.2f, 4.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-10.0f, 0.2f, 4.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-10.0f, 0.2f, -4.0f);
	// Bottom
	glNormal3f(0.0f, -1.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(10.0f, -0.2f, 4.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-10.0f, -0.2f, 4.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-10.0f, -0.2f, -4.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(10.0f, -0.2f, -4.0f);
	glEnd();
	// Top
	if (x == 1)
		glBindTexture(GL_TEXTURE_2D, texTableTopBeam);
	else
		glBindTexture(GL_TEXTURE_2D, texTableTop);
	glBegin(GL_QUADS);
	glNormal3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(10.0f, 0.2f, 4.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-10.0f, 0.2f, 4.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-10.0f, 0.2f, -4.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(10.0f, 0.2f, -4.0f);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, texTableLeg);
	glBegin(GL_QUADS);
	// Right leg
	// Front
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(9.8f, -0.2f, 3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(9.4f, -0.2f, 3.6f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(9.4f, -10.0f, 3.6f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(9.8f, -10.0f, 3.6f);
	// Back
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(9.8f, -0.2f, 3.2f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(9.4f, -0.2f, 3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(9.4f, -10.0f, 3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(9.8f, -10.0f, 3.2f);
	// Right
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(9.8f, -0.2f, 3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(9.8f, -0.2f, 3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(9.8f, -10.0f, 3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(9.8f, -10.0f, 3.6f);
	// Left
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(9.4f, -0.2f, 3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(9.4f, -0.2f, 3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(9.4f, -10.0f, 3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(9.4f, -10.0f, 3.6f);

	// Right leg back
	// Front
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(9.8f, -0.2f, -3.2f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(9.4f, -0.2f, -3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(9.4f, -10.0f, -3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(9.8f, -10.0f, -3.2f);
	// Back
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(9.8f, -0.2f, -3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(9.4f, -0.2f, -3.6f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(9.4f, -10.0f, -3.6f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(9.8f, -10.0f, -3.6f);
	// Right
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(9.8f, -0.2f, -3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(9.8f, -0.2f, -3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(9.8f, -10.0f, -3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(9.8f, -10.0f, -3.6f);
	// Left								 
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(9.4f, -0.2f, -3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(9.4f, -0.2f, -3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(9.4f, -10.0f, -3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(9.4f, -10.0f, -3.6f);

	// Left leg
	// Front
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-9.8f, -0.2f, 3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-9.4f, -0.2f, 3.6f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-9.4f, -10.0f, 3.6f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-9.8f, -10.0f, 3.6f);
	// Back
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-9.8f, -0.2f, 3.2f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-9.4f, -0.2f, 3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-9.4f, -10.0f, 3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-9.8f, -10.0f, 3.2f);
	//right								  
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-9.8f, -0.2f, 3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-9.8f, -0.2f, 3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-9.8f, -10.0f, 3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-9.8f, -10.0f, 3.6f);
	//left								  
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-9.4f, -0.2f, 3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-9.4f, -0.2f, 3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-9.4f, -10.0f, 3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-9.4f, -10.0f, 3.6f);

	// Left leg back
	//front
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-9.8f, -0.2f, -3.2f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-9.4f, -0.2f, -3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-9.4f, -10.0f, -3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-9.8f, -10.0f, -3.2f);
	//back								  
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-9.8f, -0.2f, -3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-9.4f, -0.2f, -3.6f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-9.4f, -10.0f, -3.6f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-9.8f, -10.0f, -3.6f);
	//right								  
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-9.8f, -0.2f, -3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-9.8f, -0.2f, -3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-9.8f, -10.0f, -3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-9.8f, -10.0f, -3.6f);
	//left								  
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-9.4f, -0.2f, -3.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-9.4f, -0.2f, -3.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-9.4f, -10.0f, -3.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-9.4f, -10.0f, -3.6f);

	glEnd();
	glDisable(GL_TEXTURE_2D);
}

// Draw the tables
void drawtables()
{
	// Make tables on the left and right of lab
	for (int y = 0; y <= 1; y++)
	{
		for (int x = 0; x <= 1; x++)
		{
			glPushMatrix();
			glTranslatef(-20.0 + x * 40, 7, -16 + y * 20);
			drawSetTables(x);
			glPopMatrix();
		}
	}
}

// Chair
// Set vertices and textures for Chairs
void drawSetChairs(int flag)
{
	glEnable(GL_TEXTURE_2D);
	if (flag == 1)
		glBindTexture(GL_TEXTURE_2D, texChairBaseBeam);
	else
		glBindTexture(GL_TEXTURE_2D, texChairBase);
	glBegin(GL_QUADS);
	// Chair Base
	// Front of chair
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.0f, -0.2f, 2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(2.0f, -0.2f, 2.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(2.0f, 0.2f, 2.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-2.0f, 0.2f, 2.0f);
	// Right
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(2.0f, -0.2f, -2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(2.0f, 0.2f, -2.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(2.0f, 0.2f, 2.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(2.0f, -0.2f, 2.0f);
	// Top
	glNormal3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(2.0f, 0.2f, 2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-2.0f, 0.2f, 2.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-2.0f, 0.2f, -2.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(2.0f, 0.2f, -2.0f);
	// Back
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.0f, -0.2f, -2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-2.0f, 0.2f, -2.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(2.0f, 0.2f, -2.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(2.0f, -0.2f, -2.0f);
	// Left
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.0f, -0.2f, -2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-2.0f, -0.2f, 2.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-2.0f, 0.2f, 2.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-2.0f, 0.2f, -2.0f);
	// Bottom
	glNormal3f(0.0f, -1.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(2.0f, -0.2f, 2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-2.0f, -0.2f, 2.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-2.0f, -0.2f, -2.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(2.0f, -0.2f, -2.0f);

	// Chair back rest
	// Front
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, 0.2f, 1.8f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.8f, 0.2f, 1.8f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.8f, 4.5f, 1.8f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.8f, 4.5f, 1.8f);
	// Back
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.8f, 0.2f, 1.8f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.8f, 0.2f, 1.8f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.8f, 4.5f, 1.8f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.8f, 4.5f, 1.8f);
	// Bottom
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, 0.2f, 2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.8f, 0.2f, 2.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.8f, 4.5f, 2.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.8f, 4.5f, 2.0f);
	// Left
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, 0.2f, 2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.8f, 4.5f, 2.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.8f, 4.5f, 1.8f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.8f, 0.2f, 1.8f);
	// Right
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.8f, 0.2f, 2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.8f, 4.5f, 2.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.8f, 4.5f, 1.8f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.8f, 0.2f, 1.8f);
	// Top
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, 4.5f, 2.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.8f, 4.5f, 1.8f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.8f, 4.5f, 1.8f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.8f, 4.5f, 2.0f);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, texChairLeg);
	glBegin(GL_QUADS);
	// Front right leg
	// Front
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.8f, -0.2f, 1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.4f, -0.2f, 1.6f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.4f, -5.0f, 1.6f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.8f, -5.0f, 1.6f);
	// Back
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.8f, -0.2f, 1.2f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.4f, -0.2f, 1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.4f, -5.0f, 1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.8f, -5.0f, 1.2f);
	// Right
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.8f, -0.2f, 1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.8f, -0.2f, 1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.8f, -5.0f, 1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.8f, -5.0f, 1.6f);
	// Left
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.4f, -0.2f, 1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.4f, -0.2f, 1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.4f, -5.0f, 1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.4f, -5.0f, 1.6f);

	// Back right leg 
	//Front
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.8f, -0.2f, -1.2f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.4f, -0.2f, -1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.4f, -5.0f, -1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.8f, -5.0f, -1.2f);
	// Back
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.8f, -0.2f, -1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.4f, -0.2f, -1.6f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.4f, -5.0f, -1.6f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.8f, -5.0f, -1.6f);
	// Right
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.8f, -0.2f, -1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.8f, -0.2f, -1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.8f, -5.0f, -1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.8f, -5.0f, -1.6f);
	// Left
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.4f, -0.2f, -1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.4f, -0.2f, -1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.4f, -5.0f, -1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.4f, -5.0f, -1.6f);

	// Front left leg
	// Front
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, -0.2f, 1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.4f, -0.2f, 1.6f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.4f, -5.0f, 1.6f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.8f, -5.0f, 1.6f);
	// Back
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, -0.2f, 1.2f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.4f, -0.2f, 1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.4f, -5.0f, 1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.8f, -5.0f, 1.2f);
	// Right
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, -0.2f, 1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.8f, -0.2f, 1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.8f, -5.0f, 1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.8f, -5.0f, 1.6f);
	// Left
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.4f, -0.2f, 1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.4f, -0.2f, 1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.4f, -5.0f, 1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.4f, -5.0f, 1.6f);

	// Back left leg
	// Front
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, -0.2f, -1.2f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.4f, -0.2f, -1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.4f, -5.0f, -1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.8f, -5.0f, -1.2f);
	// Back
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, -0.2f, -1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.4f, -0.2f, -1.6f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.4f, -5.0f, -1.6f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.8f, -5.0f, -1.6f);
	// Right
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.8f, -0.2f, -1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.8f, -0.2f, -1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.8f, -5.0f, -1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.8f, -5.0f, -1.6f);
	// Left
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.4f, -0.2f, -1.6f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.4f, -0.2f, -1.2f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.4f, -5.0f, -1.2f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.4f, -5.0f, -1.6f);

	glEnd();
	glDisable(GL_TEXTURE_2D);
}

// Draw the chairs
void drawchairs()
{
	// Create chairs for each desk
	for (int j = 0; j <= 1; j++)
	{
		for (int i = 0; i <= 1; i++)
		{
			glPushMatrix();
			glTranslatef(-25 + i * 12, 4.5, -5.8 + j * 20);
			drawSetChairs(0);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(15 + i * 12, 4.5, -5.8 + j * 20);
			if (j == 0 && i == 1)
				drawSetChairs(1);
			else drawSetChairs(0);
			glPopMatrix();
		}
	}
}

// Robot
// Create body and head of robot
void drawSetRobot() 
{
	// Head
	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslated(0.0, 1.0, 0.0);
	glutSolidSphere(.25, 20, 20);
	glPopMatrix();

	// Eyes
	glColor3f(1.0, 1.0, 1.0);
	glPushMatrix();
	glTranslated(0.11, 1.1, 0.15);
	glutSolidSphere(.07, 20, 20);
	glPopMatrix();
	glPushMatrix();
	glTranslated(-0.11, 1.1, 0.15);
	glutSolidSphere(.07, 20, 20);
	glPopMatrix();
	glColor3f(1.0f, 1.0f, 0.6f);
	glPushMatrix();
	glTranslated(0.12, 1.12, 0.2);
	glutSolidSphere(.03, 20, 20);
	glPopMatrix();
	glPushMatrix();
	glTranslated(-0.12, 1.12, 0.2);
	glutSolidSphere(.03, 20, 20);
	glPopMatrix();
	glColor3f(0.0, 0.0, 0.0);
	glPushMatrix();
	glTranslated(0.121, 1.125, 0.222);
	glutSolidSphere(.01, 20, 20);
	glPopMatrix();
	glPushMatrix();
	glTranslated(-0.121, 1.125, 0.222);
	glutSolidSphere(.01, 20, 20);
	glPopMatrix();

	// Mouth
	glColor3f(0.0f, 0.5f, 0.5f);
	glPushMatrix();
	glTranslated(0.0, 0.92, 0.180);
	glScaled(2.0, 0.60, 1.0);
	glutSolidSphere(.07, 20, 20);
	glPopMatrix();
	glColor3f(0.0, 0.0, 0.0);
	glPushMatrix();
	glTranslated(0.0, 0.92, 0.190);
	glRotated(20, 1.0, 0.0, 0.0);
	glScaled(1.04, 0.65, 0.6);
	glutSolidSphere(.07, 20, 20);
	glPopMatrix();

	// Nose
	glColor3f(0.7f, 0.8f, 0.9f);
	glPushMatrix();
	glTranslated(0.0, 1.02, 0.29);
	glScaled(0.40, 0.40, 0.5);
	glutSolidSphere(.06, 20, 20);
	glPopMatrix();

	// Shirt
	glColor3f(0.6f, 0.8f, 0.2f);
	glPushMatrix();
	glTranslated(0.0, 0.40, 0.0);
	glScaled(1.0, 1.5, 0.30);
	glutSolidCube(.50);
	glPopMatrix();

	// Buttons
	glColor3f(1.0, 0.0, 0.0);
	glPushMatrix();
	glTranslated(0.0, 0.68, 0.05);
	glutSolidSphere(0.07, 20, 20);
	glPopMatrix();
	glColor3f(0.0, 1.0, 0.0);
	glPushMatrix();
	glTranslated(0.0, 0.53, 0.05);
	glutSolidSphere(0.07, 20, 20);
	glPopMatrix();
	glColor3f(0.0, 0.0, 1.0);
	glPushMatrix();
	glTranslated(0.0, 0.38, 0.05);
	glutSolidSphere(0.07, 20, 20);
	glPopMatrix();
	glColor3f(0.0, 0.0, 0.0);
	glPushMatrix();
	glTranslated(0.0, 0.23, 0.05);
	glutSolidSphere(0.07, 20, 20);
	glPopMatrix();
}

// Robot left arm
void leftArm() 
{
	// Arm
	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslated(-0.30, 0.50, 0.0);
	glScaled(0.07, 1.0, 0.20);
	glutSolidCube(.50); // Create a cube
	glPopMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslated(-0.270, 0.50, 0.0);
	glScaled(0.07, 1.0, 0.20);
	glutSolidCube(.50);
	glPopMatrix();
	glPushMatrix();
	glTranslated(-0.33, 0.50, 0.0);
	glScaled(0.07, 1.0, 0.20);
	glutSolidCube(.50);
	glPopMatrix();

	// Hand
	glColor3f(0.3f, 0.3f, 0.3f);
	glPushMatrix();
	glTranslated(-0.30, 0.17, 0.0);
	glScaled(0.80, 1.0, 1.0);
	glutSolidSphere(.08, 20, 20); // Create a sphere
	glPopMatrix();
}

// Robot right arm
void rightArm() 
{
	// Arm
	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslated(0.30, 0.50, 0.0);
	glScaled(0.07, 1.0, 0.20);
	glutSolidCube(.50);
	glPopMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslated(0.270, 0.50, 0.0);
	glScaled(0.07, 1.0, 0.20);
	glutSolidCube(.50);
	glPopMatrix();
	glPushMatrix();
	glTranslated(0.33, 0.50, 0.0);
	glScaled(0.07, 1.0, 0.20);
	glutSolidCube(.50);
	glPopMatrix();

	// Hand
	glColor3f(0.3f, 0.3f, 0.3f);
	glPushMatrix();
	glTranslated(0.30, 0.17, 0.0);
	glScaled(0.80, 1.0, 1.0);
	glutSolidSphere(.08, 20, 20);
	glPopMatrix();
}

// Robot left leg
void leftLeg() 
{
	// Leg
	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslated(-0.20, -0.220, 0.0);
	glScaled(0.20, 1.0, 0.20);
	glutSolidCube(.50);
	glPopMatrix();

	// Foot
	glColor3f(0.3f, 0.3f, 0.3f);
	glPushMatrix();
	glTranslated(-0.20, -0.50, 0.15);
	glScaled(1.0, 0.60, 1.50);
	glutSolidSphere(.1, 20, 20);
	glPopMatrix();
	glPushMatrix();
	glTranslated(-0.20, -0.485, 0.0);
	glScaled(1.0, 0.65, 1.0);
	glutSolidSphere(.1, 20, 20);
	glPopMatrix();
}

// Robot right leg
void rightLeg() 
{
	// Leg
	glColor3f(1.0f, 1.0f, 1.0f);
	glPushMatrix();
	glTranslated(0.20, -0.220, 0.0);
	glScaled(0.20, 1.0, 0.20);
	glutSolidCube(.50);
	glPopMatrix();

	// Foot
	glColor3f(0.3f, 0.3f, 0.3f);
	glPushMatrix();
	glTranslated(0.20, -0.50, 0.15);
	glScaled(1.0, 0.60, 1.50);
	glutSolidSphere(.1, 20, 20);
	glPopMatrix();
	glPushMatrix();
	glTranslated(0.20, -0.485, 0.0);
	glScaled(1.0, 0.65, 1.0);
	glutSolidSphere(.1, 20, 20);
	glPopMatrix();
}

// Make robot move
void robotInitialise()
{
	// Make the robot move
	glPushMatrix();
	glScalef(3.5f, 3.5f, 3.5f);
	glRotated(0.0f, 1.0, 0.0, 0.0f);
	glRotated(0.0f, 0.0, 1.0, 0.0f);
	drawSetRobot();
	glPopMatrix();

	// Left arm move
	glPushMatrix();
	glScalef(3.5f, 3.5f, 3.5f);
	glRotated(0.0f, 1.0, 0.0, 0.0f);
	glRotated(0.0f, 0.0, 1.0, 0.0f);
	glTranslated(-0.30, 1.0, 0.0);
	glRotated(limbAngle, 1.0, 0.0, 0.0f);
	glTranslated(0.30, -1.0, 0.0);
	leftArm();
	glPopMatrix();

	// Right arm move
	glPushMatrix();
	glScalef(3.5f, 3.5f, 3.5f);
	glRotated(0.0f, 1.0, 0.0, 0.0f);
	glRotated(0.0f, 0.0, 1.0, 0.0f);
	glTranslated(0.30, 1.0, 0.0);
	glRotated(-limbAngle, 1.0, 0.0, 0.0f);
	glTranslated(-0.30, -1.0, 0.0);
	rightArm();
	glPopMatrix();

	// Left leg move
	glPushMatrix();
	glScalef(3.5f, 3.5f, 3.5f);
	glRotated(0.0f, 1.0, 0.0, 0.0f);
	glRotated(0.0f, 0.0, 1.0, 0.0f);
	glRotated(-limbAngle, 1.0, 0.0, 0.0f);
	leftLeg();
	glPopMatrix();

	// Right leg move
	glPushMatrix();
	glScalef(3.5f, 3.5f, 3.5f);
	glRotated(0.0f, 1.0, 0.0, 0.0f);
	glRotated(0.0f, 0.0, 1.0, 0.0f);
	glRotated(limbAngle, 1.0, 0.0, 0.0f);
	rightLeg();
	glPopMatrix();
}

// Draw the robots
void drawrobot()
{
	// Make robots on tables that are on the left and right of lab
	for (int y = 0; y <= 1; y++)
	{
		for (int x = 0; x <= 1; x++)
		{
			glPushMatrix();
			glTranslatef(-25.0 + x * 40, 9.2f, -16 + y * 20);
			robotInitialise();
			glPopMatrix();

			glPushMatrix();
			glTranslatef(-25.0 + x * 40, 9.2f, -16 + y * 20);
			robotInitialise();
			glPopMatrix();
		}
	}
}

// Student
// Ball for the hat
void hatball()
{
	glPushMatrix();
	glTranslatef(0, 2.2, 0);
	glScalef(.9, .9, .9);
	gluSphere(gluNewQuadric(), .18, 100, 100);
	glPopMatrix();
}

// Main part of the hat
void hatmain()
{
	glPushMatrix();
	glTranslatef(0, 1.3, 0);
	glScalef(.98, .98, .98);
	gluSphere(gluNewQuadric(), .9, 100, 100);
	glPopMatrix();
}

// Ring for the hat
void hatring()
{
	glPushMatrix();
	glTranslatef(0, 1.4, 0);
	glScalef(.84, .84, .84);
	glRotatef(90.0, 1, 0, 0);
	glutSolidTorus(.1, 1.0, 20, 20);
	glPopMatrix();
}

// Head
void head()
{
	glPushMatrix();
	glTranslatef(0, 1.2, 0);
	glScalef(.9, .9, .9);
	gluSphere(gluNewQuadric(), 1, 100, 100);
	glPopMatrix();
}

// Left eyebrow
void eyebrowleft()
{
	glPushMatrix();
	glTranslatef(-.5, 1.3, .86);;
	glRotatef(90, 0, 1, 0);
	glRotatef(-20, 1, 0, 0);
	glColor3f(0.0, 0.0, 0.0);
	gluCylinder(gluNewQuadric(), .05, .01, .3, 4, 6);
	glPopMatrix();
}

// Right eyebrow
void eyebrowright()
{
	glPushMatrix();
	glTranslatef(.5, 1.3, .86);
	glRotatef(270, 0, 1, 0);
	glRotatef(-20, 1, 0, 0);
	gluCylinder(gluNewQuadric(), .05, .01, .3, 4, 6);
	glPopMatrix();
}

// Right eye
void eyeright()
{
	glPushMatrix();
	glTranslatef(.17, 1.1, .75);
	glRotatef(-45, 0, 0, 1);
	glScalef(.9, .7, .7);
	glColor3f(1.0, 1.0, 1.0);
	gluSphere(gluNewQuadric(), .3, 100, 100);
	glPopMatrix();
}

// Left eye
void eyeleft()
{
	glPushMatrix();
	glTranslatef(-.17, 1.1, .75);
	glRotatef(45, 0, 0, 1);
	glScalef(.9, .7, .7);
	glColor3f(1.0, 1.0, 1.0);
	gluSphere(gluNewQuadric(), .3, 100, 100);
	glPopMatrix();
}

// Left pupil
void pupilleft()
{
	glPushMatrix();
	glTranslatef(-.17, 1.1, .88);
	glScalef(.9, .9, .9);
	gluSphere(gluNewQuadric(), .1, 100, 100);
	glPopMatrix();
}

// Right pupil
void pupilright()
{
	glPushMatrix();
	glTranslatef(.17, 1.1, .88);
	glScalef(.9, .9, .9);
	gluSphere(gluNewQuadric(), .1, 100, 100);
	glPopMatrix();
}

// Mouth
void mouth()
{
	glPushMatrix();
	glTranslatef(0, .78, .74);
	glScalef(.4, .4, .1);
	glColor3f(0.0, 0.0, 0.0);
	gluSphere(gluNewQuadric(), .4, 100, 100);
	glPopMatrix();
}

// Teeth
void teeth()
{
	glPushMatrix();
	glColor3f(1.0, 1.0, 1.0);
	glTranslatef(-.08, .72, .76);
	glTranslatef(.055, 0, .005);
	glutSolidCube(.035);
	glTranslatef(.055, 0, 0);
	glutSolidCube(.035);
	glPopMatrix();
}

// Collar
void collar()
{
	glPushMatrix();
	glTranslatef(0, .5, 0);
	glScalef(.59, .59, .59);
	glRotatef(90.0, 1, 0, 0);
	glutSolidTorus(.1, 1.0, 20, 20);
	glPopMatrix();
}

// Upper coat
void coat()
{
	glPushMatrix();
	glTranslatef(0, .5, 0);
	glScalef(1, .7, 1);
	glRotatef(90.0, 1, 0, 0);
	gluCylinder(gluNewQuadric(), .6, .8, 1, 100, 100);
	glPopMatrix();
}

// Bottom of Coat
void coatbottom()
{
	glPushMatrix();
	glTranslatef(0, -.2, 0);
	glScalef(1, .7, 1);
	glRotatef(90.0, 1, 0, 0);
	gluDisk(gluNewQuadric(), 0, .8, 30, 30);
	glPopMatrix();
}

// Left arm
void armleft()
{
	glPushMatrix();
	glTranslatef(-.82, 0, .1);
	glRotatef(90, 0, 1, 0);
	glRotatef(-50, 1, 0, 0);
	gluCylinder(gluNewQuadric(), .15, .15, .48, 30, 6);
	glPopMatrix();
}

// Right arm
void armright()
{
	glPushMatrix();
	glTranslatef(.82, 0, .1);
	glRotatef(90, 0, 1, 0);
	glRotatef(-130, 1, 0, 0);
	gluCylinder(gluNewQuadric(), .15, .15, .48, 30, 6);
	glPopMatrix();
}

// Left hand
void handleft()
{
	glPushMatrix();
	glTranslatef(.82, 0, .1);
	glScalef(.4, .3, .3);
	gluSphere(gluNewQuadric(), .4, 100, 100);
	glPopMatrix();
}

// Right hand
void handright()
{
	glPushMatrix();
	glTranslatef(-.82, 0, .1);
	glScalef(.4, .3, .3);
	gluSphere(gluNewQuadric(), .4, 100, 100);
	glPopMatrix();
}

// Left leg
void legleft()
{
	glPushMatrix();
	glTranslatef(.3, -.14, -0.4);
	glScalef(0.5, 0.7, 2);
	gluCylinder(gluNewQuadric(), .5, .5, .52, 30, 6);
	glPopMatrix();
}

// Right leg
void legright()
{
	glPushMatrix();
	glTranslatef(-.3, -.14, -0.4);
	glScalef(0.5, 0.7, 2);
	gluCylinder(gluNewQuadric(), .5, .5, .52, 30, 6);
	glPopMatrix();
}

// Left foot
void footleft()
{
	glPushMatrix();
	glTranslatef(-.3, -.2, 0.7);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	glScalef(1.5, .3, 1.5);
	gluSphere(gluNewQuadric(), .3, 100, 100);
	glPopMatrix();
}

// Right foot
void footright()
{
	glPushMatrix();
	glTranslatef(.3, -.2, 0.7);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	glScalef(1.5, .3, 1.5);
	gluSphere(gluNewQuadric(), .3, 100, 100);
	glPopMatrix();
}

// Draw the students
void drawstudents()
{
	// Make the students sitting on chairs on the left and right of lab
	for (int x = 0; x <= 1; x++)
	{
		for (int n = 0; n <= 1; n++)
		{
			glPushMatrix();
			glTranslatef(-25 + n * 39.8, 6, -6.7 + x * 20);
			glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
			glScalef(3.0f, 3.0f, 3.0f);

			eyebrowleft();
			eyebrowright();
			eyeright();
			eyeleft();
			mouth();
			teeth();

			// Apply different colours (to each part of their body) for each student
			if (x == 0 && n == 0)  
				glColor3ub(230, 0, 0);
			else if (x == 0 && n == 1)
				glColor3ub(212, 173, 252);  
			else if (x == 1 && n == 0)
				glColor3ub(7, 25, 82);  
			else
				glColor3ub(255, 255, 255);  
			hatball();
			if (x == 0 && n == 0)  
				glColor3ub(179, 0, 0);
			else if (x == 0 && n == 1)
				glColor3ub(12, 19, 79);  
			else if (x == 1 && n == 0)
				glColor3ub(0, 0, 0);  
			else
				glColor3ub(255, 102, 0);  
			hatmain();
			if (x == 0 && n == 0)  
				glColor3ub(230, 0, 0);
			else if (x == 0 && n == 1)
				glColor3ub(92, 70, 156);  
			else if (x == 1 && n == 0)
				glColor3ub(255, 255, 255);  
			else
				glColor3ub(0,0,0);  
			hatring();

			if (x == 0 && n == 0)
				glColor3ub(102, 51, 0);
			else if (x == 0 && n == 1)
				glColor3ub(255, 229, 204);
			else if (x == 1 && n == 0)
				glColor3ub(255, 214, 153);
			else
				glColor3ub(255, 204, 179);
			head();

			if (x == 0 && n == 0)  
				glColor3ub(204, 0, 0);
			else if (x == 0 && n == 1)
				glColor3ub(230, 206, 253);  
			else if (x == 1 && n == 0)
				glColor3ub(153, 153, 255);  
			else
				glColor3ub(255, 102, 0);  
			armleft();
			armright();
			coat();
			coatbottom();

			if (x == 0 && n == 0)  
				glColor3ub(204, 0, 0);
			else if (x == 0 && n == 1)
				glColor3ub(12, 19, 79);  
			else if (x == 1 && n == 0)
				glColor3ub(7, 25, 82);  
			else
				glColor3ub(255, 255, 255);  
			collar();
			handleft();
			handright();

			glColor3ub(50, 40, 60);
			pupilleft();
			pupilright();
			legright();
			legleft();

			if (x == 0 && n == 1)
				glColor3ub(0, 0, 0);  
			else if (x == 1 && n == 0)
				glColor3ub(7, 25, 82);  
			else
				glColor3ub(255, 255, 255);  
			footleft();
			footright();
			
			glPopMatrix();
		}
	}
}

// Rendering functions
// Function for realtime refresh
void reshape(int width, int height)
{
	WinWidth = width;
	WinHeight = height;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0f, (GLfloat)width / (GLfloat)height, 0.01f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(myEye.x, myEye.y, myEye.z, vPoint.x + 30 * sin(vAngle), vPoint.y, -30 * cos(vAngle), 0.0f, 1.0f, 0.0f);
}

// Function to initialise parameters
void initialise()
{
	glClearColor(0, 0, 0, 0);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Lighting
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, model_ambient);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, mat_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);

	glShadeModel(GL_SMOOTH);

	// Material shade for faces
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	// Material reflection of specular light
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
}

// Whilst screen is idle
GLvoid OnIdle()
{
	// Define static movement factor
	static float mf = 0.3;

    // Add movement factor onto limb angle
	limbAngle += mf;

	// Set mf to a certain range if angle is out of range
	if (limbAngle > 30.0)
	{
		mf = -0.50;
	}
	if (limbAngle < -30.0)
	{
		mf = 0.50;
	}

	glutPostRedisplay();
}

/*
Changes display when certain keys (esc, w, s, a, d) are pressed:
esc - Exit application
w - move forward
s - move backward
a - move left
d - move right
Depending on angle currently viewing w, s, a, d can change.

If r is pressed it resets the eyeview to the original.
*/
GLvoid OnKeyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 97:
		myEye.x -= 0.5;
		vPoint.x -= 0.5;
		if (myEye.x <= -40)
			myEye.x = -40;
		break;
	case 100:
		myEye.x += 0.5;
		vPoint.x += 0.5;
		if (myEye.x >= 40)
			myEye.x = 40;
		break;
	case 119:
		myEye.z -= 0.5;
		if (myEye.z <= -30)
			myEye.z = -30;
		break;
	case 115:
		myEye.z += 0.5;
		if (myEye.z >= 30)
			myEye.z = 30;
		break;
	case 114:
		myEye.x = 0;
		myEye.y = 15;
		myEye.z = 25;
		vPoint.x = 0;
		vPoint.y = 15;
		vPoint.z = -30;
		vAngle = 0;
		break;
	case 27:
		exit(0);

	}
	reshape(WinWidth, WinHeight);
	glutPostRedisplay();
}

// Function responsible for special keyboard actions - like change of angle, etc.
GLvoid OnDirection(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		vAngle -= 0.05;
		break;
	case GLUT_KEY_RIGHT:
		vAngle += 0.05;
		break;
	case GLUT_KEY_UP:
		myEye.y += 0.05;
		if (myEye.y >= 30)
			myEye.y = 30;
		break;
	case GLUT_KEY_DOWN:
		myEye.y -= 0.5;
		if (myEye.y <= 0)
			myEye.y = 30;
		break;
	case GLUT_KEY_PAGE_DOWN:
		myEye.z += 0.5;
		if (myEye.z >= 30)
			myEye.z = 30;
		break;
	case GLUT_KEY_PAGE_UP:
		myEye.z -= 0.5;
		if (myEye.z <= -30)
			myEye.z = -30;
		break;
	}
	reshape(WinWidth, WinHeight);
	glutPostRedisplay();
}

// Display scene
void myDisplay()
{
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Call functions to draw
	drawbigscence();
	drawchairs();
	drawtables();
	drawrobot();
	drawstudents();

	glutSwapBuffers();
	glFlush();
}

void printInst()
{
	printf("Welcome to Group 1 Lab Scene:\n\n");
	std::cout << "\033[1;31m";
	printf("[Please ensure your caps lock is off]\n\n");
	std::cout << "\033[0m";
	printf("These are the options to navigate:\n");
	printf("To move FORWARD: w\n");
	printf("To move BACKWARD: s\n");
	printf("To move LEFT: a\n");
	printf("To move RIGHT: d\n\n");
	printf("Viewing angle UP: UP ARROW\n");
	printf("Viewing angle DOWN: DOWN ARROW\n");
	printf("Look to your Left: LEFT ARROW\n");
	printf("Look to your Right: RIGHT ARROW\n");
	printf("Reset view: r\n\n");
	printf("To EXIT: esc");
}

int main(int argc, char* argv[])
{
	myEye.x = 0;
	myEye.y = 15;
	myEye.z = 25;
	vPoint.x = 0;
	vPoint.y = 15;
	vPoint.z = -30;
	vAngle = 0;
	glEnable(GL_DEPTH_TEST);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
	glutInitWindowPosition(400, 0);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Graphics Project - Group 1");
	initialise();
	glutDisplayFunc(&myDisplay);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(OnKeyboard);
	glutSpecialFunc(OnDirection);
	glutIdleFunc(OnIdle);

	// Load texture Bitmaps into variables
	texSmartBoard = load_texture("smartboard.bmp");
	texWindow = load_texture("window.bmp");
	texCeiling = load_texture("ceiling.bmp");
	texDoor = load_texture("inside_door.bmp");
	texFloor = load_texture("floor.bmp");
	texChairBase = load_texture("chair_base.bmp");
	texChairBaseBeam = load_texture("chair_base_beam.bmp");
	texChairLeg = load_texture("chair_leg.bmp");
	texTableTop = load_texture("table_top.bmp");
	texTableTopBeam = load_texture("table_top_beam.bmp");
	texTableLeg = load_texture("table_leg.bmp");
	texotherWalls = load_texture("other_wall.bmp");
	texfrontWall = load_texture("front_wall.bmp");
	texoppwindowWall = load_texture("opp_window.bmp");
	texPlug = load_texture("plugpoint.bmp");

	printInst();
	// Start scene
	glutMainLoop();
	return 0;
}