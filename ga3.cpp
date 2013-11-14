#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <IL/il.h>

using namespace std;

// Classes

class Viewport;

class Viewport {
  public:
    int w, h; // width and height
};

class Point {
public:
    Point(): x(0), y(0), z(0) {}
    Point(float X, float Y, float Z): x(X), y(Y), z(Z) {}
    Point operator+(Point p) {
	return Point(x+p.x, y+p.y, z+p.z);
    }
    Point operator*(float m) {
	return Point(x*m, y*m, z*m);
    }
    void putVertex() {
	glVertex3f(x, y, z); // negate z?
	return;
    }
    void drawFrom(Point p) {
	glBegin(GL_LINES);
	p.putVertex();
	putVertex();
	//	this.putVertex();
	glEnd();
	return;
    }
    float x, y, z;
};

class Curve {
public:
    Curve(Point p0, Point p1, Point p2, Point p3): pt0(p0), pt1(p1), pt2(p2), pt3(p3) {}
    Point interpolate(float t) {
	Point a = pt0*(1.0-t) + pt1*t;
	Point b = pt1*(1.0-t) + pt2*t;
	Point c = pt2*(1.0-t) + pt3*t;
	
	Point d = a*(1.0-t) + b*t;
	Point e = b*(1.0-t) + c*t;
	return d*(1.0-t) + e*t;
    }
    Point pt0, pt1, pt2, pt3;
};

class Patch {
public:
    Curve getCurveU(int v) { // might not be good to create a new curve every time
	return Curve(pts[v*4], pts[v*4+1], pts[v*4+2], pts[v*4+3]);
    }
    Curve getCurveV(int u) {
	return Curve(pts[u], pts[u+4], pts[u+8], pts[u*12]);
    }
    Curve interpolateU(float v) {
	Point a = getCurveV(0).interpolate(v);
	Point b = getCurveV(1).interpolate(v);
	Point c = getCurveV(2).interpolate(v);
	Point d = getCurveV(3).interpolate(v);
	return Curve(a, b, c, d);
    }
    Curve interpolateV(float u) {
	Point a = getCurveU(0).interpolate(u);
	Point b = getCurveU(1).interpolate(u);
	Point c = getCurveU(2).interpolate(u);
	Point d = getCurveU(3).interpolate(u);
	return Curve(a, b, d, d);
    }
    Point interpolate(float u, float v) {
	return interpolateU(v).interpolate(u);
    }
    Point pts[16];
};

// Globals

Viewport	viewport;
float limit;
bool adaptive = false;
int numPatches;
Patch** patches;

//****************************************************
// Simple init function
//****************************************************
void initScene() {

  // Nothing to do here for this simple example.

}

//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
  viewport.w = w;
  viewport.h = h;

  glViewport (0,0,viewport.w,viewport.h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, viewport.w, 0, viewport.h);

}

void uniformTesselation() {
    //gl stuff (color?) here?
    for (int i = 0; i < numPatches; i++) {
	int numSteps = (int)(1.0 / limit);
	Point* curr = new Point[numSteps];
	Point* prev = new Point[numSteps];
	curr[0] = patches[i]->pts[0];
	for (int u = 1; u < numSteps; u++) {
	    prev[u] = patches[i]->interpolate(u*limit,0.0);
	    prev[u].drawFrom(curr[u-1]);
	}
	for (int v = 1; v < numSteps; v++) {
	    curr[0] = patches[i]->interpolate(0.0, v*limit);
	    curr[0].drawFrom(prev[0]);
	    for (int u = 1; u < numSteps; u++) { // tolerance?
		curr[u] = patches[i]->interpolate(u*limit, v*limit);
		curr[u].drawFrom(curr[u-1]);
		curr[u].drawFrom(prev[u]);
	    }
	    prev = curr;
	}
    }
}

//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {

    glClear(GL_COLOR_BUFFER_BIT);				// clear the color buffer

    glMatrixMode(GL_MODELVIEW);			        // indicate we are specifying camera transformations
    glLoadIdentity();				        // make sure transformation is "zero'd"


    // Start drawing
    glColor3f(1.0f, 0.0f, 0.0f);
    
    uniformTesselation();

    glFlush();
    glutSwapBuffers();					// swap buffers (we earlier set double buffer)

}

void myKeyboard(unsigned char key, int mouseX, int mouseY) {
    switch (key) {
    case 's':
	// toggle shading
	break;
    case 'w':
	// toggle between filled and wireframe
	break;
    case 'h': // optional
	// toggle between filled and hidden-line mode
	break;
    // rotate object with arrow keys
    // translate object with shift+arrow keys
    default:
	break;
    }
}

void printPatches() {
    for (int i = 0; i < numPatches; i++) {
	for (int j = 0; j < 4; j++) {
	    printf("%.2f %.2f %.2f  ", patches[i]->pts[j].x, patches[i]->pts[j].y, patches[i]->pts[j].z);
	    printf("%.2f %.2f %.2f  ", patches[i]->pts[j+1].x, patches[i]->pts[j+1].y, patches[i]->pts[j+1].z);
	    printf("%.2f %.2f %.2f  ", patches[i]->pts[j+2].x, patches[i]->pts[j+2].y, patches[i]->pts[j+2].z);
	    printf("%.2f %.2f %.2f\n", patches[i]->pts[j+3].x, patches[i]->pts[j+3].y, patches[i]->pts[j+3].z);
	}
    }
}

void loadPatches(string file) {
    ifstream inpfile(file.c_str());
    if (!inpfile.is_open()) {
	cout << "Unable to open file" << endl;
    } else {
	inpfile >> numPatches;
	patches = new Patch*[numPatches];
	float a, b, c, d, e, f, g, h, i, j, k, l;
	for (int i = 0; i < numPatches; i++) {
	    inpfile >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j >> k >> l;
	    patches[i] = new Patch();
	    patches[i]->pts[0] = *(new Point(a, b, c)); // I feel like you should do this another way
	    patches[i]->pts[1] = *(new Point(d, e, f));
	    patches[i]->pts[2] = *(new Point(g, h, i));
	    patches[i]->pts[3] = *(new Point(j, k, l));
	    inpfile >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j >> k >> l;
	    patches[i]->pts[4] = *(new Point(a, b, c));
	    patches[i]->pts[5] = *(new Point(d, e, f));
	    patches[i]->pts[6] = *(new Point(g, h, i));
	    patches[i]->pts[7] = *(new Point(j, k, l));
	    inpfile >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j >> k >> l;
	    patches[i]->pts[8] = *(new Point(a, b, c));
	    patches[i]->pts[9] = *(new Point(d, e, f));
	    patches[i]->pts[10] = *(new Point(g, h, i));
	    patches[i]->pts[11] = *(new Point(j, k, l));
	    inpfile >> a >> b >> c >> d >> e >> f >> g >> h >> i >> j >> k >> l;
	    patches[i]->pts[12] = *(new Point(a, b, c));
	    patches[i]->pts[13] = *(new Point(d, e, f));
	    patches[i]->pts[14] = *(new Point(g, h, i));
	    patches[i]->pts[15] = *(new Point(j, k, l));
	} 
	inpfile.close();
    }
}

//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {
    //This initializes glut
    glutInit(&argc, argv);
    
    //This tells glut to use a double-buffered window with red, green, and blue channels
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    
    // Initalize theviewport size
    viewport.w = 400;
    viewport.h = 400;

    string file = argv[1];
    limit = atof(argv[2]);

    if (argc > 3 && !strncmp(argv[3], "-a", 2)) {
        adaptive = true;
    }

    loadPatches(file);
    printPatches();

  //The size and position of the window
  glutInitWindowSize(viewport.w, viewport.h);
  glutInitWindowPosition(0,0);
  glutCreateWindow(argv[0]);

  initScene();							// quick function to set up scene

  glutDisplayFunc(myDisplay);				// function to run when its time to draw something
  glutReshapeFunc(myReshape);				// function to run when the window gets resized
  glutKeyboardFunc(myKeyboard);

  glutMainLoop();							// infinite loop that will keep drawing and resizing
  // and whatever else

  return 0;
}
