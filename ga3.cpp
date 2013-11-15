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

#define ANGLE 15.0
#define MAX_RECUR 10

using namespace std;

// Globals


float limit;
bool adaptive = false;
int numPatches;
bool lines = false;
GLfloat mat_specular[] = {21.0, 21.0, 21.0, 1.0};
GLfloat mat_diffuse[] = {12.5, 12.5, 12.5, 0.5};
GLfloat mat_shininess[] = {20.0};
GLfloat light_position[] = {-1.0, -1.0, -1.0, -1.0};

// Classes

class Viewport;

class Viewport {
  public:
    int w, h; // width and height
};

Viewport	viewport;

class Point {
public:
    Point(): x(0), y(0), z(0), u(0), v(0) {}
    Point(float X, float Y, float Z): x(X), y(Y), z(Z), u(0), v(0) {}
    Point(float X, float Y, float Z, float U, float V): x(X), y(Y), z(Z), u(U), v(V) {}
    Point operator+(Point p) {
	return Point(x+p.x, y+p.y, z+p.z, u+p.u, v+p.v);
    }
    Point add(Point p) {
	return Point(x+p.x, y+p.y, z+p.z, u+p.u, v+p.v);
    }	
    Point operator*(float m) {
	return Point(x*m, y*m, z*m, u*m, v*m);
    }
    float distance(Point p) {
	return sqrt(pow(x-p.x, 2) + pow(y-p.y, 2) + pow(z-p.z, 2));
    }
    Point midpoint(Point p) {
	return add(p) * 0.5;
    }
    bool far(Point p) {
	return distance(p) > limit;
    }
    void putVertex() {
	glVertex3f(x, y, z);
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
    
    float x, y, z, u, v;
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
	return Curve(pts[u], pts[u+4], pts[u+8], pts[u+12]);
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

Patch** patches;

//****************************************************
// Simple init function
//****************************************************
void initScene() {

  // Nothing to do here for this simple example.
    glLoadIdentity();
    glutInitDisplayMode(GLUT_DEPTH);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_SMOOTH);

    //glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    //glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    //glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
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
  glOrtho(-4, 4, -4, 4, 4, -4);
  //gluOrtho2D(0, viewport.w, 0, viewport.h);

}

// counterclockwise from face up
void putNormal(Point a, Point b, Point c) {
    Point ab = a * -1.0 + b;
    Point ac = a * -1.0 + c;
    glNormal3f(-ab.y*ac.z + ab.z*ac.y, -ab.z*ac.x+ab.x*ac.z, -ab.x*ac.y+ab.y*ac.x);
}

void interpolatePoints(Point* points, int patchNum, int numSteps) {
    for (int j = 0; j < numSteps - 1; j++) {
	for (int i = 0; i < numSteps - 1; i++) {
	    points[j*numSteps+i] = patches[patchNum]->interpolate(i*limit, j*limit);
	}
	points[(j+1)*numSteps-1] = patches[patchNum]->interpolate(1.0, j*limit);
    }
    for (int i = 0; i < numSteps - 1; i++) {
	points[(numSteps-1)*numSteps+i] = patches[patchNum]->interpolate(i*limit, 1.0);
    }
    points[numSteps*numSteps-1] = patches[patchNum]->interpolate(1.0, 1.0);
}

void printPoints(Point *points, int numSteps) {
    for (int j = 0; j < numSteps; j++) {
	for (int i = 0; i < numSteps; i++) {
	    Point p = points[j*numSteps+i];
	    printf("%.2f %.2f %.2f  ", p.x, p.y, p.z);
	}
	printf("\n");
    }
}

void uniTesQuad() {
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // should be in init, then keyboard
    glBegin(GL_QUADS);
    int numSteps = (int)(1.0/ limit) + 2;
    for (int x = 0; x < numPatches; x++) {
	Point* points = new Point[numSteps*numSteps];
	interpolatePoints(points, x, numSteps);
	//printPoints(points, numSteps);
	for(int j = 0; j < numSteps - 1; j++) {
	    for(int i = 0; i < numSteps - 1; i++) {
		putNormal(points[j*numSteps+i], points[j*numSteps+i+1], points[(j+1)*numSteps+i]);
		points[j*numSteps+i].putVertex();
		points[j*numSteps+i+1].putVertex();
		points[(j+1)*numSteps+i+1].putVertex();
		points[(j+1)*numSteps+i].putVertex();
	    }
	}
    }
    glEnd();
}

void uniformTesselation() {
    int numSteps = (int)(1.0 / limit) + 1;
    for (int i = 0; i < numPatches; i++) {
	glColor3f(((float)i)/numPatches, 1.0 - ((float)i)/numPatches, 0.0f);
	//printf("%d", numSteps);
	Point* curr = new Point[numSteps + 1];
	Point* prev = new Point[numSteps + 1];
	Point* temp;
	prev[0] = patches[i]->pts[0];
	for (int u = 1; u < numSteps; u++) {
	    prev[u] = patches[i]->interpolate(u*limit,0.0);
	    prev[u].drawFrom(prev[u-1]);
	}
	prev[numSteps] = patches[i]->interpolate(1.0, 0.0);
	prev[numSteps].drawFrom(prev[numSteps-1]);
	for (int v = 1; v < numSteps; v++) {
	    curr[0] = patches[i]->interpolate(0.0, v*limit);
	    curr[0].drawFrom(prev[0]);
	    for (int u = 1; u < numSteps; u++) {
		curr[u] = patches[i]->interpolate(u*limit, v*limit);
		curr[u].drawFrom(curr[u-1]);
		curr[u].drawFrom(prev[u]);
	    }
	    curr[numSteps] = patches[i]->interpolate(1.0, v*limit);
	    curr[numSteps].drawFrom(curr[numSteps-1]);
	    curr[numSteps].drawFrom(prev[numSteps]);
	    temp = prev;
	    prev = curr;
	    curr = temp;
	}
	curr[0] = patches[i]->interpolate(0.0, 1.0);
	curr[0].drawFrom(prev[0]);
	for (int u = 1; u < numSteps; u++) {
	    curr[u] = patches[i]->interpolate(u*limit, 1.0);
	    curr[u].drawFrom(curr[u-1]);
	    curr[u].drawFrom(prev[u]);
	}
    }
}

void drawTriangle(Point a, Point b, Point c, int patchNum, int recur) {
    if (recur > MAX_RECUR) {
	putNormal(a, b, c);
	a.putVertex();
	b.putVertex();
	c.putVertex();
	return;
    }
    Point ab = a.midpoint(b);
    Point nab = patches[patchNum]->interpolate(ab.u, ab.v);
    Point bc = b.midpoint(c);
    Point nbc = patches[patchNum]->interpolate(bc.u, bc.v);
    Point ca = c.midpoint(a);
    Point nca = patches[patchNum]->interpolate(ca.u, ca.v);
    // the latter should probably go in the close function
    bool eab = ab.far(nab);
    bool ebc = bc.far(nbc);
    bool eca = ca.far(nca);
    if (eab && ebc && eca) {
	drawTriangle(a, nab, nca, patchNum, recur + 1); // ok
	drawTriangle(b, nbc, nab, patchNum, recur + 1); // ok
	drawTriangle(c, nca, nbc, patchNum, recur + 1); // ok
    } else if (eab && ebc) {
	drawTriangle(b, nbc, nab, patchNum, recur + 1);
	drawTriangle(c, nab, nbc, patchNum, recur + 1);
	drawTriangle(a, nab, c, patchNum, recur + 1); // need to change
    } else if (ebc && eca) {
	drawTriangle(c, nca, nbc, patchNum, recur + 1);
	drawTriangle(a, nbc, nca, patchNum, recur + 1);
	drawTriangle(b, nbc, a, patchNum, recur + 1);
    } else if (eca && eab) {
	drawTriangle(a, nab, nca, patchNum, recur + 1);
	drawTriangle(b, nca, nab, patchNum, recur + 1);
	drawTriangle(c, nca, b, patchNum, recur + 1);
    } else if (eab) {
	drawTriangle(c, a, nab, patchNum, recur + 1);
	drawTriangle(c, nab, b, patchNum, recur + 1);
    } else if (ebc) {
	drawTriangle(a, b, nbc, patchNum, recur + 1);
	drawTriangle(a, nbc, c, patchNum, recur + 1);
    } else if (eca) {
	drawTriangle(b, a, nca, patchNum, recur + 1);
	drawTriangle(b, nca, a, patchNum, recur + 1);
    } else {
	putNormal(a, b, c);
	a.putVertex();
	b.putVertex();
	c.putVertex();
    }
}

void adaptiveTessalation() {
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < numPatches; i++) {
	//	printf("what about");
	Point a = patches[i]->pts[0];
	Point b = patches[i]->pts[3];
	Point c = patches[i]->pts[12];
	Point d = patches[i]->pts[15];
	b.u = 1;
	c.v = 1;
	d.u = 1;
	d.v = 1;
	//	printf("getshere");
	drawTriangle(a, c, b, i, 0);
	drawTriangle(d, b, c, i, 0);
	//	printf("here?");
    }
    glEnd();
}



//****************************************************
// function that does the actual drawing of stuff
//***************************************************
void myDisplay() {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);				// clear the color buffer

    glMatrixMode(GL_MODELVIEW);			        // indicate we are specifying camera transformations
    //glLoadIdentity();				        // make sure transformation is "zero'd"


    // Start drawing
    glColor3f(1.0f, 1.0f, 1.0f);

  // Glbegin(Gl_LINE_STRIP);
  // glVertex3f(0.0f, 0.0f, 0.0f);
  // glVertex3f(0.0f, 0.5f, 0.0f);
  // glVertex3f(0.5f, 0.0f, 0.0f);
  // glEnd();  
  
    if (adaptive) {
	adaptiveTessalation();
    } else {
	uniTesQuad();
    }
    glFlush();
    glutSwapBuffers();					// swap buffers (we earlier set double buffer)
    //printf("hello world");
}

void myDirectionalKeys(int key, int mouseX, int mouseY) {
    if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
	switch (key) {
	case GLUT_KEY_RIGHT:
	    glTranslatef(1.0f, 0.0f, 1.0f);
	    break;
	case GLUT_KEY_LEFT:
	    glTranslatef(-1.0f, 0.0f, 1.0f);
	    break;
	case GLUT_KEY_UP:
	    glTranslatef(0.0f, 1.0f, 0.0f);
	    break;
	case GLUT_KEY_DOWN:
	    glTranslatef(0.0f, -1.0f, 0.0f);
	    break;
	    // translate object with shift+arrow keys
	default:
	    break;
	}
    } else {
	switch (key) {
	case GLUT_KEY_RIGHT:
	    glRotatef(ANGLE, 0.0f, 0.0f, 1.0f);
	    break;
	case GLUT_KEY_LEFT:
	    glRotatef(-ANGLE, 0.0f, 0.0f, 1.0f);
	    break;
	case GLUT_KEY_UP:
	    glRotatef(ANGLE, 1.0f, 0.0f, 0.0f);
	    break;
	case GLUT_KEY_DOWN:
	    glRotatef(-ANGLE, 1.0f, 0.0f, 0.0f);
	    break;
	    // translate object with shift+arrow keys
	default:
	    break;
	}
    }
}

void myKeyboard(unsigned char key, int mouseX, int mouseY) {
    switch (key) {
    case ' ': // make sure this is working
	exit(0);
	break;
    case 's':
	// toggle shading
	break;
    case 'w':
	if (lines) {
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	    lines = false;
	} else {
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    lines = true;
	}
	break;
    case 'h': // optional
	// toggle between filled and hidden-line mode
	break;
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

void print12(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l) {
    printf("%.2f %.2f %.2f  %.2f %.2f %.2f  %.2f %.2f %.2f  %.2f %.2f %.2f\n", a, b, c, d, e, f, g, h, i, j, k, l);
}

void loadPatches(string file) {
    ifstream inpfile(file.c_str());
    if (!inpfile.is_open()) {
	cout << "Unable to open file" << endl;
    } else {
	inpfile >> numPatches;
	patches = new Patch*[numPatches];
	float a, b, c, d, e, f, g, h, m, j, k, l;
	for (int i = 0; i < numPatches; i++) {
	    inpfile >> a >> b >> c >> d >> e >> f >> g >> h >> m >> j >> k >> l;
	    //print12(a, b, c, d, e, f, g, h, m, j, k, l);
	    patches[i] = new Patch();
	    patches[i]->pts[0] = *(new Point(a, b, c)); // I feel like you should do this another way
	    patches[i]->pts[1] = *(new Point(d, e, f));
	    patches[i]->pts[2] = *(new Point(g, h, m));
	    patches[i]->pts[3] = *(new Point(j, k, l));
	    inpfile >> a >> b >> c >> d >> e >> f >> g >> h >> m >> j >> k >> l;
	    //print12(a, b, c, d, e, f, g, h, m, j, k, l);
	    patches[i]->pts[4] = *(new Point(a, b, c));
	    patches[i]->pts[5] = *(new Point(d, e, f));
	    patches[i]->pts[6] = *(new Point(g, h, m));
	    patches[i]->pts[7] = *(new Point(j, k, l));
	    inpfile >> a >> b >> c >> d >> e >> f >> g >> h >> m >> j >> k >> l;
	    //print12(a, b, c, d, e, f, g, h, m, j, k, l);
	    patches[i]->pts[8] = *(new Point(a, b, c));
	    patches[i]->pts[9] = *(new Point(d, e, f));
	    patches[i]->pts[10] = *(new Point(g, h, m));
	    patches[i]->pts[11] = *(new Point(j, k, l));
	    inpfile >> a >> b >> c >> d >> e >> f >> g >> h >> m >> j >> k >> l;
	    //print12(a, b, c, d, e, f, g, h, m, j, k, l);
	    patches[i]->pts[12] = *(new Point(a, b, c));
	    patches[i]->pts[13] = *(new Point(d, e, f));
	    patches[i]->pts[14] = *(new Point(g, h, m));
	    patches[i]->pts[15] = *(new Point(j, k, l));
	} 
	inpfile.close();
    }
}

//****************************************************
// called by glut when there are no messages to handle
 //****************************************************
void myFrameMove() {
  //nothing here for now
#ifdef _WIN32
  Sleep(10);                                   //give ~10ms back to OS (so as not to waste the CPU)
#endif
  glutPostRedisplay(); // forces glut to call the display function (myDisplay())
}

//****************************************************
// the usual stuff, nothing exciting here
//****************************************************
int main(int argc, char *argv[]) {
    //printf("not even");

    //This initializes glut
    glutInit(&argc, argv);
    
    //This tells glut to use a double-buffered window with red, green, and blue channels
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    
    // Initalize theviewport size
    viewport.w = 400;
    viewport.h = 400;

    string file = argv[1];
    limit = atof(argv[2]);
    
    //printf("what about");

    if (argc > 3 && !strncmp(argv[3], "-a", 2)) {
	adaptive = true;
    }

    //printf("okay, here?");

    loadPatches(file);
    //printPatches();

  //The size and position of the window
  glutInitWindowSize(viewport.w, viewport.h);
  glutInitWindowPosition(0,0);
  glutCreateWindow(argv[0]);

  initScene();							// quick function to set up scene

  glutDisplayFunc(myDisplay);				// function to run when its time to draw something
  glutReshapeFunc(myReshape);				// function to run when the window gets resized
  glutKeyboardFunc(myKeyboard);
  glutSpecialFunc(myDirectionalKeys);
  glutIdleFunc(myFrameMove);

  glutMainLoop();							// infinite loop that will keep drawing and resizing
  // and whatever else

  return 0;
}
