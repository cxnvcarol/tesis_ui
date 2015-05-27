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

#ifndef __PARTICLESYSTEM_H__
#define __PARTICLESYSTEM_H__

#define DEBUG_GRID 0
#define DO_TIMING 0

#include <helper_functions.h>
//#include "particles_kernel.cuh"
#include "vector_functions.h"

//custom
#include "ModelOBJ.h"
#include <QString>
#include <iomanip>


#define MAX_CELLS	2500000

//#define PATH_INI	"."
#define PATH_INI	"../tesis_ui"
#define PATH_INI2	"/home/cxnv/workspace/Simulator"
#define OBJ_PATH	PATH_INI"/data/models3d/volumen.obj"
#define DATAFILE_PATH	PATH_INI"/data/datos.csv"
//#define DATAFILE_PATH	PATH_INI"/data/datatime.csv"

#define MAX_INT 2147483648
#define MAX_ITERATIONS 50
#define MAX_HISTOGRAM_INTERVALS 100

// Particle system class
class ParticleSystem {


private:
    void initConstants();
public:
    struct ColorValue
    {
        float* colorRGB=(float*)calloc(3,sizeof(float));
        float value;
        bool hidden=false;//TODO set hidden according to UI
    };
    ParticleSystem(bool bUseOpenGL);
	~ParticleSystem();

    ParticleSystem() {
    }

    int nframes,currentFrame;
	bool clipped;
	int currentVariable;
	int colorRangeMode;
	bool demoCutting;
    bool enableCutting,displayLow,displayMiddle,displayHigh;
	float particleRadius;

	float maxTotal;

	float maxLocalVar, minLocalVar, width_histogram;

    float* gradientInitialColor;
    float* gradientFinalColor;
    float* highColor;
    float* lowColor;

    //TODO refactor when variables are indexed!
    int ncolorsTemp;
    int ncolorsPress;
    int ncolorsVel;
    ColorValue* colorsTemp;
    ColorValue* colorsPress;
    ColorValue* colorsVel;

    void toggleVisibility(int range);

    void loadColorConfiguration();
    void insertColorValue(float * newColor,float newValue, int indexVar);
    bool deleteColorValue(int indexColorVal);
    void editColorValue(int indexColorVal,float * newColor,float newValue);

	enum FixedVariables {
		VAR_TEMPERATURE, VAR_PRESSURE, VAR_VELOCITY, _NUM_VARIABLES
	};
	enum FixedColorRangeModes {
		COLOR_GRADIENT, COLOR_SHORT_RAINBOW, COLOR_FULL_RAINBOW, _NUM_MODES
	};
	enum ParticleConfig {
		CONFIG_RANDOM, CONFIG_GRID, CONFIG_SIMULATION_DATA,CONFIG_SIMULATION_DATA_VEL, _NUM_CONFIGS

	};

	enum ParticleArray {
		POSITION, VELOCITY,POSITION_VEL
	};


	struct cutter{
		float3 pos;
		float3 size;
	};
	cutter cutterX,cutterY,cutterZ,cutterBox;
	void initCutters();
	void initCutters2();

    int currentCutter;
	bool forwardDirectionCutter;
	void forwardCutterX();
	void rewindCutterX();
	void forwardCutterY();
	void rewindCutterY();
	void forwardCutterZ();
	void rewindCutterZ();
	void advanceCutter();

    char const * getCurrentVarName()
	{
		switch(currentVariable)
		{
		case 0:
			return "Temperature(K)";
		case 1:
			return "Pressure(Pa)";
		case 2:
			return "Velocity(m/s)";
		}
		return "";
	}
    int getTotalColorValues()
    {
        switch(currentVariable)
        {
            case VAR_TEMPERATURE: return ncolorsTemp;
            case VAR_PRESSURE: return ncolorsPress;
            case VAR_VELOCITY: return ncolorsVel;
        }
    }
    ColorValue* getColorValues()
    {
        switch(currentVariable)
        {
        case VAR_TEMPERATURE:
            return colorsTemp;
        case VAR_PRESSURE:
            return colorsPress;
        case VAR_VELOCITY:
            return colorsVel;
        }
    }

	float** getColorsScale()
    {
        float** colorsScale;

        switch(currentVariable)
        {
        case VAR_TEMPERATURE:
            colorsScale=(float**)calloc(ncolorsTemp,sizeof(float*));
            for (int var = 0; var < ncolorsTemp; ++var) {
                colorsScale[var]=colorsTemp[var].colorRGB;
            }
            break;
        case VAR_PRESSURE:
            colorsScale=(float**)calloc(ncolorsPress,sizeof(float*));
            for (int var = 0; var < ncolorsPress; ++var) {
                colorsScale[var]=colorsPress[var].colorRGB;
            }
            break;
        case VAR_VELOCITY:
            colorsScale=(float**)calloc(ncolorsVel,sizeof(float*));
            for (int var = 0; var < ncolorsVel; ++var) {
                colorsScale[var]=colorsVel[var].colorRGB;
            }
            break;
        }

        return colorsScale;

	}
	float* getValuesScale()
	{
        float* valuesScale;

        switch(currentVariable)
        {
        case VAR_TEMPERATURE:
            valuesScale=(float*)calloc(ncolorsTemp,sizeof(float*));
            for (int var = 0; var < ncolorsTemp; ++var) {
                valuesScale[var]=colorsTemp[var].value;
            }
            break;
        case VAR_PRESSURE:
            valuesScale=(float*)calloc(ncolorsPress,sizeof(float*));
            for (int var = 0; var < ncolorsPress; ++var) {
                valuesScale[var]=colorsPress[var].value;
            }
            break;
        case VAR_VELOCITY:
            valuesScale=(float*)calloc(ncolorsVel,sizeof(float*));
            for (int var = 0; var < ncolorsVel; ++var) {
                valuesScale[var]=colorsVel[var].value;
            }
            break;
        }

        return valuesScale;

    }
    ColorValue getRange(float percentage);
    int getRangeValue(float val);
    float getValue(float percentage);
	void updateColor();
	void updateColorVect();
	void updateFrame();
	void toggleDisplayLow()
	{
		displayLow=!displayLow;
		updateColor();
	}
	void toggleDisplayMiddle()
	{
		displayMiddle=!displayMiddle;
		updateColor();
	}
	void toggleDisplayHigh()
	{
		displayHigh=!displayHigh;
		updateColor();
	}
	void setCurrentFrame(int newframe);
	void forward() {
		setCurrentFrame(currentFrame + 1);
	}
	void rewind() {

		setCurrentFrame(currentFrame - 1);
	}
	void changeActiveVariable();
    void changeActiveVariable(int iv);
	void setAlpha(float al);
	void setClipped(bool cl) {
		clipped = cl;
	}
	void setNormalHigh(int variable, float th) {
		switch(variable)
		{
		case VAR_TEMPERATURE:
			n_tmax=th;
			break;
		case VAR_PRESSURE:
			n_pmax=th;
			break;
		case VAR_VELOCITY:
			n_vmax=th;
			break;
		}
	}
	void setNormalLow(int variable, float th) {
		switch(variable)
		{
		case VAR_TEMPERATURE:
			n_tmin=th;
			break;
		case VAR_PRESSURE:
			n_pmin=th;
			break;
		case VAR_VELOCITY:
			n_vmin=th;
			break;
		}
	}
	void setColorRangeMode(int mode) {
		colorRangeMode = mode;
	}
	void setColorInitialGradient(float* ini) {
        gradientInitialColor = ini;
        //updateColor();
	}
	void setColorFinalGradient(float* fini) {
		gradientFinalColor = fini;
        //updateColor();

	}
	void setColorWarningHigh(float* fini) {
		highColor = fini;
		printf("\nhighcolor: (%f,%f,%f)\n",fini[0],fini[1],fini[2]);
        //updateColor();

	}
	void setColorWarningLow(float* fini) {
		lowColor = fini;
		printf("\nlowcolor: (%f,%f,%f)\n",fini[0],fini[1],fini[2]);
        //updateColor();

	}

    void colorTemperature(int t, float* r);
    int colorVar(int t, float* r);/* return range*/
    char* getColor(float valor);
	void setFileSource(string filePath);
	void update(float deltaTime);
	void initDefaultData();
	void loadSimulationData(string fileP);
	void reset(ParticleConfig config);

	float *getArray(ParticleArray array);
	void setArray(ParticleArray array, const float *data, int start, int count);
	void calculateSecondPoint(float *p1,float *p2,int index);

	int getNumParticles() const {
		return m_numParticles;
	}

	unsigned int getCurrentReadBuffer() const {
		return m_posVbo;
	}
	unsigned int getColorBuffer() const {
		return m_colorVBO;
	}
	unsigned int getColorVectBuffer() const {
		return m_colorVBO_vect;
	}

	int getFramesCount() {
		return nframes;
	}
	int getCurrentFrame() {
		return currentFrame;
	}
	int* getFramePointer() {
		return &currentFrame;
	}
	void *getCudaPosVBO() const {
		return (void *) m_cudaPosVBO;
	}
	void *getCudaColorVBO() const {
		return (void *) m_cudaColorVBO;
	}

	void setCutterBoxPos(float3 x) {
		cutterBox.pos=x;
	}

	void setSelectedSize(float3 x) {
		cutterBox.size=x;
	}

	float getParticleRadius() {
		return particleRadius;
	}
	float3 getCutterBoxPos() {
		return cutterBox.pos;
	}

	float3 getSelectSize() {
		return cutterBox.size;
	}


	float getAlpha() {
		return alpha;
	}
	void generateHistogram();
	void histogramFunc(int index);

protected:
	// methods
	uint createVBO(uint size);

	void _initialize(int numParticles);
	void _finalize();

	void initGrid(uint *size, float spacing, float jitter, uint numParticles);

protected:
	// data
	bool m_bInitialized, m_bUseOpenGL;
	uint m_numParticles;

	// CPU data
	float *m_hPos;              // particle positions
	float *m_hVel;              // particle velocities

	// GPU data
	float *m_dPos;
	//float *m_dVel;

	float *m_dSortedPos;
	float *m_dSortedVel;

	uint m_posVbo;            // vertex buffer object for particle positions
	uint m_temperatureColor;
	uint m_pressureColor;
	uint m_colorVBO;          // vertex buffer object for colors
	uint m_colorVBO_vect;          // vertex buffer object for colors

	float *m_cudaPosVBO;        // these are the CUDA deviceMem Pos
	float *m_cudaColorVBO;      // these are the CUDA deviceMem Color

	uint* m_histogram;
	uint m_numberHistogramIntervals;


	struct cudaGraphicsResource *m_cuda_posvbo_resource; // handles OpenGL-CUDA exchange
	struct cudaGraphicsResource *m_cuda_colorvbo_resource; // handles OpenGL-CUDA exchange
	struct cudaGraphicsResource *m_cuda_colorvbo_vect_resource; // handles OpenGL-CUDA exchange

	struct velocity {
		float direction[3];
		float magnitude;
	};
	struct dataframe {
		float time;
		float* temperaturePointer;
		float* pressurePointer;
		velocity* velocityPointer;
	};

	uint m_numGridCells;

	StopWatchInterface *m_timer;

	uint m_solverIterations;

	dataframe frames[MAX_ITERATIONS]; //pointers to data of each frame//calloc?

	//following are used for current frame:
	float * xArray;
	float * yArray;
	float * zArray;
	float * temp;
	float * pressureArray;
	velocity * velArray;


	int tamMax;
	float xmax, ymax, zmax;
	float xmin, ymin, zmin;
	float tmin, tmax, pmin, pmax, vmax,vmin;

	float n_tmin,n_tmax,n_pmin,n_pmax,n_vmin,n_vmax;

	float alpha; //rango normal de 0 a 1
	float xMaxAllowed, yMaxAllowed, zMaxAllowed;


};

#endif // __PARTICLESYSTEM_H__
