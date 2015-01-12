/*
 * Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

/*
 This project was based on Particle system example with collisions using uniform grid

 CUDA 2.1 SDK release 12/2008
 - removed atomic grid method, some optimization, added demo mode.

 CUDA 2.2 release 3/2009
 - replaced sort function with latest radix sort, now disables v-sync.
 - added support for automated testing and comparison to a reference value.
 */

// OpenGL Graphics includes
#include <GL/glew.h>
#if defined (WIN32)
#include <GL/wglew.h>
#endif

#if defined(__APPLE__) || defined(__MACOSX)
#include <GLUT/glut.h>
#ifndef glutCloseFunc
#define glutCloseFunc glutWMCloseFunc
#endif
#else
#include <GL/freeglut.h>
#endif

// CUDA runtime
#include <cuda_runtime.h>

// CUDA utilities and system includes
#include <helper_functions.h>
#include <helper_cuda.h>    // includes cuda.h and cuda_runtime_api.h
#include <helper_cuda_gl.h> // includes cuda_gl_interop.h// includes cuda_gl_interop.h
// Includes
#include <stdlib.h>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include "particleSystem.h"
#include "render_particles.h"
#include "paramgl.h"
#include "gnuplot-iostream.h"


#include <QApplication>


#define DEFAULT_COLOR_CONFIG PATH_INI"/colores.config"

Gnuplot gp;

uint width = 640, height = 480;

// Scale reference:

int lenCol;
float** coloresScale;
float* valoresScale;
char* currentVarName;

// view params
int ox, oy;
int buttonState = 0;
float camera_trans[] = { 0, 0, -3 };
float camera_rot[] = { 0, 0, 0 };
float camera_trans_lag[] = { 0, 0, -3 };
float camera_rot_lag[] = { 0, 0, 0 };
const float inertia = 0.1f;
ParticleRenderer::DisplayMode displayMode = ParticleRenderer::PARTICLE_SPHERES;
//ParticleRenderer::DisplayMode displayMode = ParticleRenderer::PARTICLE_POINTS; //important!!

int mode = 0;
bool playMode = false;
bool displayEnabled = true;
bool bPause = false;
bool displaySliders = false;

bool demoMode = false;

int idleCounter = 0;
int demoCounter = 0;
int playCounter = 0;
const int idleDelay = 2000;

enum {
	M_VIEW = 0, M_MOVE
};

uint numParticles = 0;


// simulation parameters
float timestep = 0.5f;
int rangeColor = 200;

ParticleSystem *psystem = 0;

// fps
static int fpsCount = 0;
static int fpsLimit = 1;
StopWatchInterface *timer = NULL;

ParticleRenderer *renderer = 0;

Model_OBJ obj;

bool obj_drawmode = false;
float obj_alpha = 0.4f;

float modelView[16];
float modelView2[16];

ParamListGL *params;
ParamListGL *videoPlayer;

void *m_font = (void *)  GLUT_BITMAP_8_BY_13;

// Auto-Verification Code
const int frameCheckNumber = 4;
unsigned int frameCount = 0;
unsigned int g_TotalErrors = 0;
char *g_refFile = NULL;

const char *sSDKsample = "Xplotion Simulation";

extern "C" void cudaInit(int argc, char **argv);
extern "C" void cudaGLInit(int argc, char **argv);
extern "C" void copyArrayFromDevice(void *host, const void *device,
		unsigned int vbo, int size);



void refreshLegend()
{
	lenCol=psystem->totalValuesScale;
	coloresScale=psystem->getColorsScale();
	currentVarName=psystem->getCurrentVarName();
	valoresScale=psystem->getValuesScale();

	printf("legend refreshed:\n");
	printf("%s\n",currentVarName);
	printf("valores:(%f,%f,%f,%f)",valoresScale[0],valoresScale[1],valoresScale[2],valoresScale[3]);
	fflush(stdout);
}

void showHistogram() {
	//TODO set color, set axis names, set title
	float widthBar = psystem->width_histogram;
	int boxwidth = (int) (widthBar * 0.9);
	gp << "set boxwidth " << boxwidth << "\n";
	gp << "set style fill solid 0.5\n";
	gp << "set xlabel \"x\"\n";
	gp << "set ylabel \"Frequency\"\n";
	gp << "plot \"histog.dat\" using 1:2 smooth freq w boxes lc rgb\"green\" notitle\n";
	gp.flush();
}

void changeObjDrawMode() {
	obj_drawmode = !obj_drawmode;
}

void colorConfig(string configFilePath) {
	if (configFilePath.empty()) {
		psystem->setColorRangeMode(psystem->COLOR_GRADIENT);
		psystem->setColorInitialGradient(new float[3] { 1, 1, 0 });
		psystem->setColorFinalGradient(new float[3] { 1, 0, 0 });
	} else {
		std::ifstream data(configFilePath.c_str());

		std::string line;

		std::cout << line << "\n";
		int nlines = 0;
		while (std::getline(data, line)) {

			std::istringstream is_line(line);
			std::string key;
			if (std::getline(is_line, key, '=')) {
				std::string value;
				if (std::getline(is_line, value)) {
					std::cout << key << "::" << value << "\n";

				}
				if (key.find("colormode") != -1) {

					if (value.find("gradient") != -1) {
						printf("gradient!!!");
						psystem->setColorRangeMode(psystem->COLOR_GRADIENT);
					} else if (value.find("short_rainbow") != -1) {
						psystem->setColorRangeMode(
								psystem->COLOR_SHORT_RAINBOW);
					} else if (value.find("full_rainbow") != -1) {
						psystem->setColorRangeMode(psystem->COLOR_FULL_RAINBOW);
					}
				} else if (key.find("color") != -1) {
					const char* str = value.c_str();
					char *token, *strpos = const_cast<char*>(str);
					;

					float *newcolor = (float*) calloc(3, sizeof(float));
					for (int var = 0; var < 3; ++var) {
						token = strsep(&strpos, ",");
						newcolor[var] = (float) ::atof(token);

					}

					if (key.find("color1") != -1) {
						printf("color1!");
						psystem->setColorInitialGradient(newcolor);
					}

					else if (key.find("color2") != -1) {
						printf("color2!");
						psystem->setColorFinalGradient(newcolor);
					} else if (key.find("color+") != -1) {
						printf("color+!");
						psystem->setColorWarningHigh(newcolor);
					} else if (key.find("color-") != -1) {
						printf("color-!");
						psystem->setColorWarningLow(newcolor);
					}
				}

				else if (key.find("minNormal") != -1) {
					float valor = (float) ::atof(value.c_str());
					if (key.find("Temperature") != -1) {
						psystem->setNormalLow(ParticleSystem::VAR_TEMPERATURE,
								valor);

					} else if (key.find("Pressure") != -1) {
						psystem->setNormalLow(ParticleSystem::VAR_PRESSURE,
								valor);

					} else if (key.find("Velocity") != -1) {
						psystem->setNormalLow(ParticleSystem::VAR_VELOCITY,
								valor);
					}
				} else if (key.find("maxNormal") != -1) {
					float valor = (float) ::atof(value.c_str());
					if (key.find("Temperature") != -1) {
						psystem->setNormalHigh(ParticleSystem::VAR_TEMPERATURE,
								valor);

					} else if (key.find("Pressure") != -1) {
						psystem->setNormalHigh(ParticleSystem::VAR_PRESSURE,
								valor);

					} else if (key.find("Velocity") != -1) {
						psystem->setNormalHigh(ParticleSystem::VAR_VELOCITY,
								valor);
					}

				}
			}

			nlines++;
		}

	}

}
void initSimulationSystem(bool bUseOpenGL, string filePath) {
    psystem = new ParticleSystem(bUseOpenGL);

	if (filePath.empty())
		psystem->setFileSource(DATAFILE_PATH);
	else {
		try {
			psystem->setFileSource(filePath);
		} catch (int ex) {
			psystem->setFileSource(DATAFILE_PATH);
		}
	}
	printf("data simulation loaded");
	psystem->reset(ParticleSystem::CONFIG_SIMULATION_DATA);

	if (bUseOpenGL) {
		renderer = new ParticleRenderer;
		renderer->setParticleRadius(psystem->getParticleRadius());
		renderer->setColorBuffer(psystem->getColorBuffer());
		renderer->setColorVectBuffer(psystem->getColorVectBuffer());
	}

	sdkCreateTimer(&timer);
}

void cleanup() {
	sdkDeleteTimer(&timer);

    if (psystem) {
        delete psystem;
    }
}

// initialize OpenGL
void initGL(int *argc, char **argv) {
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	//	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(width, height);
	glutCreateWindow("Simulation");
    glewInit();

    if (!glewIsSupported(
            "GL_VERSION_2_0 GL_VERSION_1_5 GL_ARB_multitexture GL_ARB_vertex_buffer_object")) {
        fprintf(stderr, "Required OpenGL extensions missing.");
        exit(EXIT_FAILURE);
    }

#if defined (WIN32)

	if (wglewIsSupported("WGL_EXT_swap_control"))
	{
		// disable vertical sync
		wglSwapIntervalEXT(0);
	}

#endif

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);
	glClearColor(0.2, 0.2, 0.2, 1.0); //TODO should be in colores.config

	//enable alpha for simulation points
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glutReportErrors();
}

void computeFPS() {
	frameCount++;
	fpsCount++;

	if (fpsCount == fpsLimit) {
		char fps[256];
		float ifps = 1.f / (sdkGetAverageTimerValue(&timer) / 1000.f);
		sprintf(fps, "Xplotion simulation: %3.1f fps", ifps);

		glutSetWindowTitle(fps);
		fpsCount = 0;

		fpsLimit = (int) MAX(ifps, 1.f);
		sdkResetTimer(&timer);
	}
}

void glArrayBox(float w, float h, float d) {

	glBegin(GL_LINES);
	glVertex3f(-w / 2, -h / 2, d / 2);
	glVertex3f(-w / 2, h / 2, d / 2);
	glVertex3f(-w / 2, h / 2, d / 2);
	glVertex3f(-w / 2, h / 2, -d / 2);
	glVertex3f(-w / 2, h / 2, -d / 2);
	glVertex3f(-w / 2, -h / 2, -d / 2);
	glVertex3f(-w / 2, -h / 2, -d / 2);
	glVertex3f(-w / 2, -h / 2, d / 2);

	glVertex3f(w / 2, -h / 2, d / 2);
	glVertex3f(w / 2, h / 2, d / 2);
	glVertex3f(w / 2, h / 2, d / 2);
	glVertex3f(w / 2, h / 2, -d / 2);
	glVertex3f(w / 2, h / 2, -d / 2);
	glVertex3f(w / 2, -h / 2, -d / 2);
	glVertex3f(w / 2, -h / 2, -d / 2);
	glVertex3f(w / 2, -h / 2, d / 2);

	glVertex3f(-w / 2, -h / 2, d / 2);
	glVertex3f(w / 2, -h / 2, d / 2);
	glVertex3f(-w / 2, h / 2, d / 2);
	glVertex3f(w / 2, h / 2, d / 2);
	glVertex3f(-w / 2, h / 2, -d / 2);
	glVertex3f(w / 2, h / 2, -d / 2);
	glVertex3f(-w / 2, -h / 2, -d / 2);
	glVertex3f(w / 2, -h / 2, -d / 2);

	glEnd();
}
void paintColorBoxScale(const char *nameVar, float** colors,float* values,int length) {
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	beginWinCoords();

	float heightTotal=300, diffTotal=values[length-1]-values[0];
	float gradHeight = heightTotal/(length-1), gradWidth = 50;
	int len=0;

	float posY=heightTotal+20;
	char * testing=(char*)calloc(20,sizeof(char));
	int var=0;
	for (var = 0; var < length-1; ++var) {
		gradHeight=(values[var+1]-values[var])*heightTotal/diffTotal;
		float * col1=colors[var];
		float * col2=colors[var+1];
		glBegin(GL_QUADS);
		//glColor3f(1.0f, 0.0f, 0.0f); // make this vertex red
		glColor3fv(col1);
		glVertex2f(width - gradWidth, posY);
		glVertex2f(width - 5, posY);
		//glColor3f(1.0f, 1.0f, 0.0f); // make this vertex yellow
		glColor3fv(col2);
		glVertex2f(width - 5, posY-gradHeight);
		glVertex2f(width - gradWidth, posY-gradHeight);
		glEnd();

		glColor3f(1.0f, 1.0f, 1.0f);
		//glColor3f(0,0,0);

		testing=(char*)calloc(20,sizeof(char));
		snprintf(testing,20,"%.2f",values[var]);
		len = (int) strlen(testing)*8;
		glPrint(width - gradWidth-len, posY+10,testing,m_font);
		//glPrint(width - gradWidth, posY+10,testing,m_font);

		posY-=gradHeight;
	}
	testing=(char*)calloc(20,sizeof(char));
	snprintf(testing,20,"%.2f",values[var]);
	len = (int) strlen(testing)*8;
	glPrint(width - gradWidth-len, posY+10,testing,m_font);

	len = (int) strlen(nameVar)*8;
	glPrint(width - len, 15,nameVar,m_font);

	endWinCoords();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
void paintBoxCutter() {

	glPushMatrix();
	float3 p = psystem->getCutterBoxPos();
	glTranslatef(p.x, p.y, p.z);
	glColor3f(1.0, 0.0, 0.0);
	float3 tamSel = psystem->getSelectSize();
	glArrayBox(tamSel.x, tamSel.y, tamSel.z);

	glPopMatrix();

}
void paintPosition(float3 pos) {
	glPushMatrix();
	float3 p2 = pos;
	glTranslatef(p2.x, p2.y, p2.z);
	glColor3f(1.0, 1.0, 0.0);
	float3 tamSel = make_float3(0.1, 0.1, 0.1);
	glutSolidCylinder(tamSel.x, tamSel.y, 10, 10);

	float3 p = pos;
	p.x += 0.2f;
	p.z -= 3;

	glBegin(GL_LINES);
	glVertex3f(p.x, p.y, -11);
	glVertex3f(p.x, p.y, -3);

	glVertex3f(p.x, -4, p.z);
	glVertex3f(p.x, 4, p.z);

	glVertex3f(-4, p.y, p.z);
	glVertex3f(4, p.y, p.z);

	glEnd();

	glPopMatrix();

}
void paintCutter(float3 p, float3 tamSel) {
	glPushMatrix();
	glTranslatef(p.x, p.y, p.z);
	glColor3f(0.0, 1.0, 1.0);
	glArrayBox(tamSel.x, tamSel.y, tamSel.z);
	glPopMatrix();

}
void paintAxis() {
	glPushMatrix();

	float widln = 0;
	glGetFloatv(GL_LINE_WIDTH, &widln);
	glLineWidth(widln * 5);
	glBegin(GL_LINES);
	glColor3f(1.0, 0.0, 0.0);
	glVertex3f(-1, -1, -1);
	glVertex3f(-0.5f, -1, -1);

	glColor3f(0, 1.0, 0.0);
	glVertex3f(-1, -1, -1);
	glVertex3f(-1, -0.5f, -1);

	glColor3f(0, 0.0, 1.0);
	glVertex3f(-1, -1, -1);
	glVertex3f(-1, -1, -0.5f);

	glEnd();
	glLineWidth(widln);
	glPopMatrix();

}
void display() {
	sdkStartTimer(&timer);

	// update the simulation
	renderer->setVertexBuffer(psystem->getCurrentReadBuffer(),
			psystem->getNumParticles());

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// view transform
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, width, height);
	glLoadIdentity();

	for (int c = 0; c < 3; ++c) {
		camera_trans_lag[c] += (camera_trans[c] - camera_trans_lag[c])
				* inertia;
		camera_rot_lag[c] += (camera_rot[c] - camera_rot_lag[c]) * inertia;
	}

	glTranslatef(camera_trans_lag[0], camera_trans_lag[1], camera_trans_lag[2]);
	glRotatef(camera_rot_lag[0], 1.0, 0.0, 0.0);
	glRotatef(camera_rot_lag[1], 0.0, 1.0, 0.0);

	glGetFloatv(GL_MODELVIEW_MATRIX, modelView);

	// cube
	glColor3f(1.0, 1.0, 1.0);
	glutWireCube(2.0);

	paintAxis();

	paintBoxCutter();
	paintCutter(psystem->cutterX.pos, psystem->cutterX.size);
	paintCutter(psystem->cutterY.pos, psystem->cutterY.size);
	paintCutter(psystem->cutterZ.pos, psystem->cutterZ.size);

	//paint wireframe
	if (obj_alpha > 0 && obj_drawmode)
		obj.DrawMode(4, obj_alpha); //4 is for triangles

	if (renderer && displayEnabled) {
		renderer->display(displayMode);
	}

	if (displaySliders) {
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO); // invert color

		params->Render(0, 0);

		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	}
	{ //display slider of player
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO); // invert color

		videoPlayer->Render(0, height - 20);

		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	paintColorBoxScale(currentVarName,coloresScale,valoresScale,lenCol);

	glMatrixMode(GL_MODELVIEW);
	glViewport(0, height * 3 / 4, width * 1 / 4, height * 1 / 4);
	glLoadIdentity();

	//glClearColor(0,0,0.5,1);

	glTranslatef(0.2f, 0, -3);

	glGetFloatv(GL_MODELVIEW_MATRIX, modelView2);

	// cube
	glColor3f(1.0, 1.0, 1.0);
	glutWireCube(2.0);

	//obj.DrawMode(GL_POINTS,0.1f);
	paintPosition(
			make_float3(camera_trans_lag[0], camera_trans_lag[1],
					camera_trans_lag[2]));

	sdkStopTimer(&timer);

	glutSwapBuffers();
	glutReportErrors();

	computeFPS();
}

inline float frand() {
	return rand() / (float) RAND_MAX;
}

void reshape(int w, int h) {
	width = w;
	height = h;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (float) w / (float) h, 0.1, 100.0);

	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, w, h);

	if (renderer) {
		renderer->setWindowSize(w, h);
		renderer->setFOV(60.0);
	}
}

void mouse(int button, int state, int x, int y) {
	int mods;

	if (state == GLUT_DOWN) {
		buttonState |= 1 << button;
	} else if (state == GLUT_UP) {
		buttonState = 0;
	}

	mods = glutGetModifiers();

	if (mods & GLUT_ACTIVE_SHIFT) //MOVE
	{
		buttonState = 2;
	} else if (mods & GLUT_ACTIVE_CTRL) //ZOOM
	{
		buttonState = 3;
	}

	ox = x;
	oy = y;

	demoMode = false;
	idleCounter = 0;

	int last = psystem->getCurrentFrame();
	videoPlayer->Mouse(x, y, button, state);
	if (last != psystem->getCurrentFrame()) {
		psystem->updateFrame();

	}
	if (displaySliders) {
		if (params->Mouse(x, y, button, state)) {
			glutPostRedisplay();
			return;
		}
	}

	glutPostRedisplay();
}

void resetView() {
	camera_trans[0] = 0;
	camera_trans[1] = 0;
	camera_trans[2] = -3;

	camera_rot[0] = 0;
	camera_rot[1] = 0;
	camera_rot[2] = 0;

}

// transfrom vector by matrix
void xform(float *v, float *r, GLfloat *m) {
    r[0] = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + m[12];
    r[1] = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + m[13];
    r[2] = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + m[14];
}

// transform vector by transpose of matrix
void ixform(float *v, float *r, GLfloat *m) {
    r[0] = v[0] * m[0] + v[1] * m[1] + v[2] * m[2];
    r[1] = v[0] * m[4] + v[1] * m[5] + v[2] * m[6];
    r[2] = v[0] * m[8] + v[1] * m[9] + v[2] * m[10];
}

void ixformPoint(float *v, float *r, GLfloat *m) {
    float x[4];
    x[0] = v[0] - m[12];
    x[1] = v[1] - m[13];
    x[2] = v[2] - m[14];
    x[3] = 1.0f;
    ixform(x, r, m);
}

void motion(int x, int y) {
	float dx, dy;
	dx = (float) (x - ox);
	dy = (float) (y - oy);

	if (displaySliders) {
		if (params->Motion(x, y)) {
			ox = x;
			oy = y;
			glutPostRedisplay();
			return;
		}
	}

	switch (mode) {
	case M_VIEW:
		if (buttonState == 3) {
			// left+middle = zoom
			camera_trans[2] += (dy / 100.0f) * 0.5f * fabs(camera_trans[2]);
		} else if (buttonState & 2) {
			// middle = translate
			camera_trans[0] += dx / 100.0f;
			camera_trans[1] -= dy / 100.0f;
		} else if (buttonState & 1) {
			// left = rotate
			camera_rot[0] += dy / 5.0f;
			camera_rot[1] += dx / 5.0f;
		}

		break;

	case M_MOVE: {
		float translateSpeed = 0.003f;
		float3 p = psystem->getCutterBoxPos();

		if (buttonState == 1) {
			float v[3], r[3];
			v[0] = dx * translateSpeed;
			v[1] = -dy * translateSpeed;
			v[2] = 0.0f;
            ixform(v, r, modelView);
			p.x += r[0];
			p.y += r[1];
			p.z += r[2];
		} else if (buttonState == 2) { //shift
			float v[3], r[3];
			v[0] = 0.0f;
			v[1] = 0.0f;
			v[2] = dy * translateSpeed;
            ixform(v, r, modelView);
			p.x += r[0];
			p.y += r[1];
			p.z += r[2];
		} else if (buttonState == 3) //ctrl
				{
			//escala la caja de seleccion
			float v[3], r[3];
			v[0] = dx * translateSpeed;
			v[1] = -dy * translateSpeed;
			v[2] = 0.0f;
            ixform(v, r, modelView);

			//TODO no tiene en cuenta la direccion del arrastre con respecto a la posicion de la caja. De espaldas escala diferente a atras en perspectiva
			float3 tamSel = psystem->getSelectSize();
			tamSel.x += r[0];
			if (tamSel.x < 0.01)
				tamSel.x = 0.01;
			tamSel.y += r[1];
			if (tamSel.y < 0.01)
				tamSel.y = 0.01;
			tamSel.z += r[2];
			if (tamSel.z < 0.01)
				tamSel.z = 0.01;
			psystem->setSelectedSize(tamSel);

		}

		psystem->setCutterBoxPos(p);
	}
		break;
	}

	ox = x;
	oy = y;

	demoMode = false;
	idleCounter = 0;

	glutPostRedisplay();
}

// commented out to remove unused parameter warnings in Linux
void key(unsigned char k, int /*x*/, int /*y*/) {
	char kMayus = k;
	if (k > 96)
		kMayus = k - 32; //toUpperCase()
	switch (kMayus) {
	//custom cases:
	case 'R':
		resetView();
		break;
	case 'X':
		//printf("\nframe::%d\n",psystem->getCurrentFrame());
		psystem->changeActiveVariable();
		if (psystem->currentVariable == ParticleSystem::VAR_VELOCITY) {
			psystem->reset(ParticleSystem::CONFIG_SIMULATION_DATA_VEL);
			displayMode = ParticleRenderer::PARTICLE_ARROWS;
		} else if (psystem->currentVariable == 0) {
			psystem->reset(ParticleSystem::CONFIG_SIMULATION_DATA);
			displayMode = ParticleRenderer::PARTICLE_SPHERES;
		}
		refreshLegend();

		break;

	case 'C':
		psystem->clipped = !psystem->clipped;
		psystem->updateColor();
		break;

	case 'T':
		psystem->toggleDisplayLow();
		break;
	case 'Y':
		psystem->toggleDisplayMiddle();
		break;
	case 'U':
		psystem->toggleDisplayHigh();
		break;
	case 'O':
		changeObjDrawMode();
		break;

	case 'P': //play and pause simulation
		playMode = !playMode;
		break;
	case 'Q':
		psystem->setAlpha(
				psystem->getAlpha() >= 1 ? 1.0f : psystem->getAlpha() + 0.1f);
		break;
	case 'A':
		psystem->setAlpha(
				psystem->getAlpha() <= 0 ? 0.0f : psystem->getAlpha() - 0.1f);
		break;

	case 'I':
		obj_alpha = obj_alpha >= 1 ? 1.0f : obj_alpha + 0.1f;

		break;
	case 'K':
		obj_alpha = obj_alpha <= 0 ? 0 : obj_alpha - 0.1f;
		break;

	case 'V':
		mode = M_VIEW;
		break;

	case 'S':
		mode = M_MOVE;
		break;

	case 'M':
		displayMode = (ParticleRenderer::DisplayMode) ((displayMode + 1)
				% ParticleRenderer::PARTICLE_NUM_MODES);
		if (displayMode == ParticleRenderer::PARTICLE_ARROWS
				&& psystem->currentVariable != ParticleSystem::VAR_VELOCITY) {
			key('M', 0, 0);
		}
		break;
	case 'D':
		displaySliders = !displaySliders;
		break;
	case 'H':
		psystem->generateHistogram();
		showHistogram();

		break;

	case '\r':
		psystem->demoCutting = !psystem->demoCutting;
		break;

	case '\033':
		// cudaDeviceReset causes the driver to clean up all state. While
		// not mandatory in normal operation, it is good practice.  It is also
		// needed to ensure correct operation when the application is being
		// profiled. Calling cudaDeviceReset causes all profile data to be
		// flushed before the application exits
		cudaDeviceReset();
		gp << "exit\n";
		exit(EXIT_SUCCESS);
		break;
	}

	demoMode = false;
	idleCounter = 0;
	glutPostRedisplay();
}

void special(int k, int x, int y) {

	if (displaySliders) {
		params->Special(k, x, y);
	} else {
		switch (k) {

		case GLUT_KEY_UP:
			psystem->forwardDirectionCutter = true;
			psystem->advanceCutter();
			break;
		case GLUT_KEY_DOWN:
			psystem->forwardDirectionCutter = false;
			psystem->advanceCutter();
			break;
		case GLUT_KEY_LEFT:
			psystem->rewind();
			break;

		case GLUT_KEY_RIGHT:
			psystem->forward();
			break;
		case GLUT_KEY_F1:
			psystem->currentCutter = 0;
			psystem->initCutters();
			psystem->demoCutting = true;
			psystem->clipped = true;
			break;
		case GLUT_KEY_F2:
			psystem->currentCutter = 1;
			psystem->initCutters();
			psystem->demoCutting = true;
			psystem->clipped = true;
			break;
		case GLUT_KEY_F3:
			psystem->currentCutter = 2;
			psystem->initCutters();
			psystem->demoCutting = true;
			psystem->clipped = true;
			break;
		case GLUT_KEY_F4:
			psystem->demoCutting = false;
			psystem->initCutters2();
			psystem->enableCutting = false;
			psystem->clipped = false;
			psystem->updateColor();
			break;
		}
	}

	demoMode = false;
	idleCounter = 0;
}

void idle(void) {

	if (psystem->demoCutting) {
		demoCounter++;
		if (demoCounter++ > 20) {

			psystem->advanceCutter();
			demoCounter = 0;
            glutPostRedisplay();
		}
	}
	if (playMode) {
		playCounter++;
		if (playCounter++ > 15) {

			psystem->forward();
			playCounter = 0;
            glutPostRedisplay();
        }
	}


}

void initParams() {
	if (g_refFile) {
		timestep = 0.0f;

	} else {

		// create a new parameter list
		params = new ParamListGL("misc");
		params->AddParam(
				new Param<float>("time step", timestep, 0.0f, 1.0f, 0.01f,
						&timestep));
		params->AddParam(
				new Param<int>("range color", rangeColor, 0, 256, 200,
						&rangeColor));
	}

	//create videoplayer
	videoPlayer = new ParamListGL("videoplayer");
	printf("total frames: %d\n\n", psystem->getFramesCount());
	videoPlayer->AddParam(
			new Param<int>("frame playing", psystem->getCurrentFrame(), 0,
					psystem->getFramesCount() + 1, 1,
					psystem->getFramePointer()));
}

void mainMenu(int i) {
	key((unsigned char) i, 0, 0);
}

void initMenus() {
	glutCreateMenu(mainMenu);
	glutAddMenuEntry("Reset simulation [0]", '0');
	glutAddMenuEntry("Reset block [1]", '1');
	glutAddMenuEntry("Reset random [2]", '2');

	glutAddMenuEntry("View mode [v]", 'v');
	glutAddMenuEntry("Move cursor mode [m]", 'm');
	glutAddMenuEntry("Reset camera [s]", 's');
	glutAddMenuEntry("Change active variable [l]", 'l');
	glutAddMenuEntry("Toggle sliders [d]", 'd');
	glutAddMenuEntry("Quit (esc)", '\033');
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
int main2(int argc, char **argv) {

    //MyGLutThread mythr(argc,argv);
    //mythr.start();


#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif


	printf("%s Starting...\n\n", sSDKsample);


    if (checkCmdLineFlag(argc, (const char **) argv, "device")) {
        printf("[%s]\n", argv[0]);
        printf("   Does not explicitly support -device=n in OpenGL mode\n");
        printf(
                "   To use -device=n, the sample must be running w/o OpenGL\n\n");
        printf(" > %s -device=n -file=<*.bin>\n", argv[0]);
        printf("exiting...\n");
        exit(EXIT_SUCCESS);
    }

    initGL(&argc, argv);
    cudaGLInit(argc, argv);


    if (checkCmdLineFlag(argc, (const char **) argv, "datafile")) {//qcore arguments
		char* pth;
		getCmdLineArgumentString(argc, (const char **) argv, "datafile", &pth);
		fflush(stdout);

		printf("datafile: %s\n", pth);
		fflush(stdout);
        initSimulationSystem(true, pth);
	} else
        initSimulationSystem(true, DATAFILE_PATH);

	if (checkCmdLineFlag(argc, (const char **) argv, "colorconfig")) {
		char* pth;
		getCmdLineArgumentString(argc, (const char **) argv, "colorconfig",
				&pth);
		colorConfig(pth);
	} else {
		colorConfig(DEFAULT_COLOR_CONFIG);
	}
	initParams();

	refreshLegend();


	printf("\nantes del obj\n");
	fflush(stdout);
	obj.Load(OBJ_PATH, psystem->maxTotal);
	printf("\nvolumen cargado\n");
	fflush(stdout);

	if (!g_refFile) {
        initMenus();//ignore now (a lot of glut)
	}


    glutDisplayFunc(display);//paint
    glutReshapeFunc(reshape);//resize
    glutMouseFunc(mouse);//pressevent, release event (pending scroll??)
    glutMotionFunc(motion);//motion
    glutKeyboardFunc(key);//don't know yet
    glutSpecialFunc(special);//don't know yet
    glutIdleFunc(idle);//override timerEvent

    glutCloseFunc(cleanup);//cleanup function in :~Widget

    glutMainLoop();//no need (app.exec())

    printf("So here we go!");
    fflush(stdout);


    if (psystem) {
        delete psystem;
    }

    // cudaDeviceReset causes the driver to clean up all state. While
    // not mandatory in normal operation, it is good practice.  It is also
    // needed to ensure correct operation when the application is being
    // profiled. Calling cudaDeviceReset causes all profile data to be
    // flushed before the application exits
    cudaDeviceReset();



    exit(g_TotalErrors > 0 ? EXIT_FAILURE : EXIT_SUCCESS);

}

