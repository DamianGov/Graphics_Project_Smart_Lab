#define _CRT_SECURE_NO_WARNINGS
#include<gl/glut.h>
#include<windows.h>
#include<math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// Global Declaration of variables
// Initialisation of Arrays
GLfloat light[] = { 1.0,1.0,1.0,1 };
GLfloat light_position0[] = { 0,28,20,1.0 };
GLfloat light_position1[] = { 0,28,-20,1.0 };
GLfloat white_light[] = { 1.0,1.0,1.0,1.0 };
GLfloat model_ambient[] = { 0.05f,0.05f,0.05f,1.0f };
GLfloat mat_specular[] = { 0.8,1.0,1.0,1.0 };
GLfloat mat_shininess[] = { 5.0 };
GLfloat mat_ambient[] = { 0.1,0.1,0.1,1 };
GLfloat	no_mat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
GLfloat	mat_diffuse1[] = { 0.1f, 0.5f, 0.8f, 1.0f };
GLfloat	no_shininess[] = { 0.0f };

GLint	WinWidth;
GLint	WinHeight;

// Declare texture variables
GLuint texSmartBoard, texWindow, texDesk, texCeiling, texDoor, texFloor, texBackwall;

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

// Function to convert RGB to OpenGL Colour Codes
void RGBtoGLColour(int r, int g, int b)
{
	GLfloat red = static_cast<GLfloat>(r) / 255.0f;
	GLfloat green = static_cast<GLfloat>(g) / 255.0f;
	GLfloat blue = static_cast<GLfloat>(b) / 255.0f;

	glColor3f(red, green, blue);
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
	glNormal3f(0.0f, -1.0f, 0.0f);	// Definition of the normal vector
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-40.0f, 30.0f, 30.0f);
	glTexCoord2f(0.0f, 3.0f);
	glVertex3f(-40.0f, 30.0f, -30.0f);
	glTexCoord2f(6.0f, 3.0f);
	glVertex3f(40.0f, 30.0f, -30.0f);
	glTexCoord2f(6.0f, 0.0f);
	glVertex3f(40.0f, 30.0f, 30.0f);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	// The floor is painted
	glColor3f(0.8f, 1.0f, 0.8f);
	glBegin(GL_QUADS);
	glNormal3f(0.0f, 1.0f, 0.0f);	
	glVertex3f(-40.0f, 0.0f, 30.0f);
	glVertex3f(-40.0f, 0.0f, -30.0f);
	glVertex3f(40.0f, 0.0f, -30.0f);
	glVertex3f(40.0f, 0.0f, 30.0f);
	glEnd();

	// Left wall is drawn
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_QUADS);
	glNormal3f(1.0f, 0.0f, 0.0f);	                  
	glVertex3f(-40.0f, 0.0f, 30.0f);
	glVertex3f(-40.0f, 30.0f, 30.0f);
	glVertex3f(-40.0f, 30.0f, -30.0f);
	glVertex3f(-40.0f, 0.0f, -30.0f);
	glEnd();

	// Right wall is drawn
	glColor3f(0.8f, 0.8f, 0.8f);
	glBegin(GL_QUADS);
	glNormal3f(-1.0f, 0.0f, 0.0f); 
	glVertex3f(40.0f, 0.0f, 30.0f);
	glVertex3f(40.0f, 30.0f, 30.0f);
	glVertex3f(40.0f, 30.0f, -30.0f);
	glVertex3f(40.0f, 0.0f, -30.0f);
	glEnd();

	// Windows are drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texWindow);
	for (int n = 0; n <= 1; n++)
	{
		glBegin(GL_QUADS);
		glNormal3f(-1.0, 0.0f, 0.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(39.9, 10, -8 + n * 18);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(39.9, 20, -8 + n * 18);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(39.9, 20, -18 + n * 18);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(39.9, 10, -18 + n * 18);
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);

	// Backwall is drawn
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texBackwall);
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
	glBindTexture(GL_TEXTURE_2D, texBackwall);
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
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-20.0, 8.0f, -29.9f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-20.0, 18.0f, -29.9f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(20.0, 18.0f, -29.9f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(20.0, 8.0f, -29.9f);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	// Door is drawn
	//glColor3f(0.521f, 0.121f, 0.0547f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texDoor);
	glBegin(GL_QUADS);
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-39.9f, 0.0f, -25.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-39.9f, 14.0f, -25.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-39.9f, 14.0f, -19.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-39.9f, 0.0f, -19.0f);
	glEnd();
	glDisable(GL_TEXTURE_2D);

}

// Draw the desks
void drawdesks()
{
	// Specify desk colour
	GLfloat desk[] = { 1,0.9647,0.56078 };

	for (int y = 0; y <= 4; y++)
	{
		// Make tables on the left and right of lab
		for (int x = 0; x <= 1; x++)
		{
			// Top of Table
			glColor3f(1, 0.9647, 0.56078);
			glPushMatrix();
			glTranslatef(-20.0 + x * 40, 8.1, -17.5 + y * 8);
			glScalef(20, 0.2, 3);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, desk);
			glutSolidCube(1.0f);
			glPopMatrix();

			// Underneath Table
			glColor3f(1, 0.9647, 0.56078);
			glPushMatrix();
			glTranslatef(-20.0 + x * 40, 6.1, -17.5 + y * 8);
			glScalef(19, 0.2, 3);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, desk);
			glutSolidCube(1.0f);
			glPopMatrix();

			//Front of Table
			glColor3f(1, 0.9647, 0.56078);
			glPushMatrix();
			glTranslatef(-20.0 + x * 40, 7, -18.9 + y * 8);
			glScalef(20, 2, 0.2);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, desk);
			glutSolidCube(1.0f);
			glPopMatrix();
			
			// Legs of table
			glColor3f(0.0, 0.0, 0.0);
			glBegin(GL_LINES);
			glLineWidth(5.0f);
			// Vertices of left leg
			glVertex3f(-30.0 + x * 40, 6.0f, -19 + y * 8);
			glVertex3f(-30.0 + x * 40, 0.0f, -19 + y * 8);
			glEnd();
			glBegin(GL_LINES);
			glLineWidth(5.0f);
			// Vertices of right leg
			glVertex3f(-10.0 + x * 40, 6.0f, -19 + y * 8);
			glVertex3f(-10.0 + x * 40, 0.0f, -19 + y * 8);
			glEnd();
		}
	}
}

// Draw the chairs
void drawchairs()
{
	// Chair colours
	GLfloat chair[] = { 0.1,0.67,0.62 };

	for (int j = 0; j <= 4; j++)
	{
		// Create chairs for each desk
		for (int i = 0; i <= 1; i++)
		{
			// Bottom of chair
			glColor3f(0.1, 0.67, 0.62);
			glPushMatrix();
			glTranslatef(-20 + i * 40, 3.1, -14.5 + j * 8);
			glScalef(20, 0.2, 3);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, chair);
			glutSolidCube(1.0f);
			glPopMatrix();

			// Back of chair
			glColor3f(0.1, 0.67, 0.62);
			glPushMatrix();
			glTranslatef(-20 + i * 40, 5, -13 + j * 8);
			glScalef(20, 4, 0.2);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, chair);
			glutSolidCube(1.0f);
			glPopMatrix();

			// Chair legs
			glColor3f(0.0, 0.0, 0.0);
			glBegin(GL_LINES);
			glLineWidth(3.0f);
			glVertex3f(-30 + i * 40, 3.0f, -13 + j * 8);
			glVertex3f(-30 + i * 40, 0.0f, -13 + j * 8);
			glEnd();
			glColor3f(0.0, 0.0, 0.0);
			glBegin(GL_LINES);
			glLineWidth(3.0f);
			glVertex3f(-10.0 + i * 40, 3.0f, -13 + j * 8);
			glVertex3f(-10.0 + i * 40, 0.0f, -13 + j * 8);
			glEnd();

		}
	}
}


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

	glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
	glLightfv(GL_LIGHT1, GL_AMBIENT, mat_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, white_light);
	glLightfv(GL_LIGHT1, GL_SPECULAR, white_light);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);

	glShadeModel(GL_SMOOTH);
	// Material shade for faces
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	// Material reflection of specular light
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);	
	glEnable(GL_DEPTH_TEST);
}

GLvoid OnIdle()
{
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
	drawdesks();
	drawchairs();
	glFlush();
}

void printInst()
{
	printf("Welcome to Group 1 Lab Scene:\n");
	printf("These are the options to navigate:\n");
	std::cout << "\033[1;31m";
	printf("[Please ensure your caps lock is off]\n");
	std::cout << "\033[0m";
	printf("To move FORWARD: w\n");
	printf("To move BACKWARD: s\n");
	printf("To move LEFT: a\n");
	printf("To move RIGHT: d\n\n");
	printf("Viewing angle UP: UP ARROW\n");
	printf("Viewing angle DOWN: DOWN ARROW\n");
	printf("Look to your Left: LEFT ARROW\n");
	printf("Look to your Right: RIGHT ARROW\n\n");
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
	glutCreateWindow("Grap Proj");
	initialise();
	glutDisplayFunc(&myDisplay);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(OnKeyboard);
	glutSpecialFunc(OnDirection);
	glutIdleFunc(OnIdle);
	
	// Load texture Bitmaps into variables
	texSmartBoard = load_texture("blackboard.bmp");
	texWindow = load_texture("window.bmp");
	texCeiling = load_texture("ceiling.bmp");
	texDoor = load_texture("sliding_door.bmp");
	texFloor = load_texture("floor.bmp");
	texBackwall = load_texture("backwall.bmp");

	printInst();
	// Start scene
	glutMainLoop();
	return 0;
}