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
#define MAX_RECUR 1

using namespace std;

// Globals


float limit;
bool adaptive = false;
int numPatches;
bool lines = true;
bool smooth = true;
GLfloat mat_specular[] = {0.25, 0.25, 0.25, 1.0};
GLfloat mat_diffuse[] = {0.25, 0.25, 0.25, 1.5};
GLfloat mat_ambient[] = {0.25, 0.25, 0.25, 1.0};
GLfloat mat_shininess[] = {20.0};
GLfloat light_position[] = {0.0, -1.0, -1.0, -1.0};
float translate[] = {0.0, 0.0};
float rotate[] = {0.0, 0.0};


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
    void putNormal() {
	glNormal3f(x, y, z);
    }
    void drawFrom(Point p) {
	glBegin(GL_LINES);
	p.putVertex();
	putVertex();
	//	this.putVertex();
	glEnd();
	return;
    }
    void print() {
	printf(" %.2f %.2f %.2f %.2f %.2f ", x, y, z, u, v);
    }
    float x, y, z, u, v;
};

Point junk;

class Triangle {
public:
    Triangle(Point A, Point B, Point C): a(A), b(B), c(C) {}

    Point a, b, c;
};

class Curve {
public:
    Curve(Point p0, Point p1, Point p2, Point p3): pt0(p0), pt1(p1), pt2(p2), pt3(p3) {}
    Point interpolate(float t, Point *derivative) {
	Point a = pt0*(1.0-t) + pt1*t;
	Point b = pt1*(1.0-t) + pt2*t;
	Point c = pt2*(1.0-t) + pt3*t;
	
	Point d = a*(1.0-t) + b*t;
	Point e = b*(1.0-t) + c*t;
	(*derivative) = (e + d * -1.0) * 3.0;
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
	Point a = getCurveV(0).interpolate(v, &junk);
	Point b = getCurveV(1).interpolate(v, &junk);
	Point c = getCurveV(2).interpolate(v, &junk);
	Point d = getCurveV(3).interpolate(v, &junk);
	return Curve(a, b, c, d);
    }
    Curve interpolateV(float u) {
	Point a = getCurveU(0).interpolate(u, &junk);
	Point b = getCurveU(1).interpolate(u, &junk);
	Point c = getCurveU(2).interpolate(u, &junk);
	Point d = getCurveU(3).interpolate(u, &junk);
	return Curve(a, b, c, d);
    }
    Point interpolate(float u, float v, Point *normal) {
	Point du;
	Point dv;
	Point result = interpolateU(v).interpolate(u, &du);
	interpolateV(u).interpolate(v, &dv);
	float dot = du.x*dv.x + du.y*dv.y + du.z*dv.z;
	Point p;
	float dis = p.distance(du)*p.distance(dv);
	float angle = acos(dot/dis);
	float nx = du.y*dv.z - du.z*dv.y;
	float ny = du.z*dv.x - du.x*dv.z;
	float nz = du.x*dv.y - du.y*dv.x;
	if (angle > 90) {
	    (*normal) = Point(nx, ny, nz);
	} else {
	    (*normal) = Point(-nx, -ny, -nz);
	}
	//(*normal).print();
	return result;
    }
    Point pts[16];
};

Patch** patches;

//****************************************************
// Simple init function
//****************************************************
void initScene() {

  // Nothing to do here for this simple example.

    //glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    //glClearDepth(4.0f);
    glShadeModel(GL_SMOOTH);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    //glDepthMask(GL_TRUE);
    //glDepthFunc(GL_LEQUAL);
    //glEnable(GL_CULL_FACE);
    //glDepthRange(0.0f, 1.0f);
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
  //glFrustum(-4, 4, -4, 4,
  //gluPerspective(90, 1, 0.1, 100); // make field of view wider
  glOrtho(-4, 4, -4, 4, -4, 4);
  //gluOrtho2D(0, viewport.w, 0, viewport.h);

}

// clockwise from face up
void putNormal(Point a, Point b, Point c) {
    Point ab = a * -1.0 + b;
    Point ac = a * -1.0 + c;
    float dot = ab.x*ac.x + ab.y*ac.y + ab.z*ac.z;
    Point p;
    float dis = p.distance(ab)*p.distance(ac);
    float angle = acos(dot/dis);
    float cx = ab.y*ac.z - ab.z*ac.y;
    float cy = ab.z*ac.x - ab.x*ac.z;
    float cz = ab.x*ac.y - ab.y*ac.x;
    if (angle > 90.0) {
        glNormal3f(-cx, -cy, -cz);
    } else {
        glNormal3f(cx, cy, cz);
    }
}

void interpolatePoints(Point* points, int patchNum, int numSteps, Point* normals) {
    Point normal;
    for (int j = 0; j < numSteps - 1; j++) {
	for (int i = 0; i < numSteps - 1; i++) {
	    points[j*numSteps+i] = patches[patchNum]->interpolate(i*limit, j*limit, &normal);
	    normals[j*numSteps+i] = normal;
	}
	points[(j+1)*numSteps-1] = patches[patchNum]->interpolate(1.0, j*limit, &normal);
	normals[(j+1)*numSteps-1] = normal;
    }
    for (int i = 0; i < numSteps - 1; i++) {
	points[(numSteps-1)*numSteps+i] = patches[patchNum]->interpolate(i*limit, 1.0, &normal);
	normals[(numSteps-1)*numSteps+i] = normal;
    }
    points[numSteps*numSteps-1] = patches[patchNum]->interpolate(1.0, 1.0, &normal);
    normals[numSteps*numSteps-1] = normal;
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
	Point* normals = new Point[numSteps*numSteps];
	interpolatePoints(points, x, numSteps, normals);
	//printPoints(points, numSteps);
	if (smooth) { //smooth
	    for(int j = 0; j < numSteps - 1; j++) {
		for(int i = 0; i < numSteps - 1; i++) {
		    //putNormal(points[j*numSteps+i], points[(j+1)*numSteps+i], points[j*numSteps+i+1]);
		    //((normals[j*numSteps+i] + normals[(j+1)*numSteps+i] + normals[(j+1)*numSteps+i+1] + normals[j*numsteps+i+1])*0.25).putNormal();
		    normals[j*numSteps+i].putNormal();
		    points[j*numSteps+i].putVertex();
		    normals[(j+1)*numSteps+i].putNormal();
		    points[(j+1)*numSteps+i].putVertex();
		    normals[(j+1)*numSteps+i+1].putNormal();
		    points[(j+1)*numSteps+i+1].putVertex();
		    normals[j*numSteps+i+1].putNormal();
		    points[j*numSteps+i+1].putVertex();

		}
	    }
	} else {
	    for(int j = 0; j < numSteps - 1; j++) {
		for(int i = 0; i < numSteps - 1; i++) {
		    //putNormal(points[j*numSteps+i], points[(j+1)*numSteps+i], points[j*numSteps+i+1]);
		    ((normals[j*numSteps+i] + normals[(j+1)*numSteps+i] + normals[(j+1)*numSteps+i+1] + normals[j*numSteps+i+1])*0.25).putNormal();
		    points[j*numSteps+i].putVertex();
		    points[(j+1)*numSteps+i].putVertex();
		    points[(j+1)*numSteps+i+1].putVertex();
		    points[j*numSteps+i+1].putVertex();

		}
	    }
	}
    }
    glEnd();
}

// void uniformTesselation() {
//     int numSteps = (int)(1.0 / limit) + 1;
//     for (int i = 0; i < numPatches; i++) {
// 	glColor3f(((float)i)/numPatches, 1.0 - ((float)i)/numPatches, 0.0f);
// 	//printf("%d", numSteps);
// 	Point* curr = new Point[numSteps + 1];
// 	Point* prev = new Point[numSteps + 1];
// 	Point* temp;
// 	prev[0] = patches[i]->pts[0];
// 	for (int u = 1; u < numSteps; u++) {
// 	    prev[u] = patches[i]->interpolate(u*limit,0.0);
// 	    prev[u].drawFrom(prev[u-1]);
// 	}
// 	prev[numSteps] = patches[i]->interpolate(1.0, 0.0);
// 	prev[numSteps].drawFrom(prev[numSteps-1]);
// 	for (int v = 1; v < numSteps; v++) {
// 	    curr[0] = patches[i]->interpolate(0.0, v*limit);
// 	    curr[0].drawFrom(prev[0]);
// 	    for (int u = 1; u < numSteps; u++) {
// 		curr[u] = patches[i]->interpolate(u*limit, v*limit);
// 		curr[u].drawFrom(curr[u-1]);
// 		curr[u].drawFrom(prev[u]);
// 	    }
// 	    curr[numSteps] = patches[i]->interpolate(1.0, v*limit);
// 	    curr[numSteps].drawFrom(curr[numSteps-1]);
// 	    curr[numSteps].drawFrom(prev[numSteps]);
// 	    temp = prev;
// 	    prev = curr;
// 	    curr = temp;
// 	}
// 	curr[0] = patches[i]->interpolate(0.0, 1.0);
// 	curr[0].drawFrom(prev[0]);
// 	for (int u = 1; u < numSteps; u++) {
// 	    curr[u] = patches[i]->interpolate(u*limit, 1.0);
// 	    curr[u].drawFrom(curr[u-1]);
// 	    curr[u].drawFrom(prev[u]);
// 	}
//     }
// }

void triangleIter(Triangle first, Triangle second, int patchNum) {
    bool isGood = false;
    while (!isGood) {

    }
}

void drawTriangle(Point a, Point b, Point c, int patchNum, int recur) {
    // printf("triangle:");
    // a.print();
    // b.print();
    // c.print();
    // printf("\n");
    if (recur > MAX_RECUR) {
	putNormal(a, b, c);
	a.putVertex();
	b.putVertex();
	c.putVertex();
	return;
    }
    Point ab = a.midpoint(b);
    Point nab = patches[patchNum]->interpolate(ab.u, ab.v, &junk);
    Point bc = b.midpoint(c);
    Point nbc = patches[patchNum]->interpolate(bc.u, bc.v, &junk);
    Point ca = c.midpoint(a);
    Point nca = patches[patchNum]->interpolate(ca.u, ca.v, &junk);
    // the latter should probably go in the close function
    bool eab = ab.far(nab);
    bool ebc = bc.far(nbc);
    bool eca = ca.far(nca);
    // printf("depth: %d split at:", recur);
    // if (eab) {
    // 	ab.print();
    // 	printf("is far from");
    // 	nab.print();
    // } if (ebc) {
    // 	bc.print();
    // 	printf("is far from");
    // 	nbc.print();
    // } if (eca) {
    // 	ca.print();
    // 	printf("is far from");
    // 	nca.print();
    // }
    // printf("\n");
    if (eab || ebc || eca) {
	nab.u = ab.u;
	nab.v = ab.v;
	nbc.u = bc.u;
	nbc.v = bc.v;
	nca.u = ca.u;
	nca.v = ca.v;
	drawTriangle(a, nab, nca, patchNum, recur + 1); // ok
	drawTriangle(b, nbc, nab, patchNum, recur + 1); // ok
	drawTriangle(c, nca, nbc, patchNum, recur + 1); // ok
	drawTriangle(nab, nbc, nca, patchNum, recur + 1);
    } else if (eab && ebc) { // not working
	nab.u = ab.u;
	nab.v = ab.v;
	nbc.u = bc.u;
	nbc.v = bc.v;	
	drawTriangle(b, nbc, nab, patchNum, recur + 1);
	drawTriangle(c, nab, nbc, patchNum, recur + 1);
	drawTriangle(a, nab, c, patchNum, recur + 1); // need to change
    } else if (ebc && eca) { // not working
	nbc.u = bc.u;
	nbc.v = bc.v;
	nca.u = ca.u;
	nca.v = ca.v;
	drawTriangle(c, nca, nbc, patchNum, recur + 1);
	drawTriangle(a, nbc, nca, patchNum, recur + 1);
	drawTriangle(b, nbc, a, patchNum, recur + 1);
    } else if (eab && eca) { // weird
	nab.u = ab.u;
	nab.v = ab.v;
	nca.u = ca.u;
	nca.v = ca.v;
	drawTriangle(a, nab, nca, patchNum, recur + 1);
	drawTriangle(b, nca, nab, patchNum, recur + 1);
	drawTriangle(c, nca, b, patchNum, recur + 1);
    } else if (eab) { // weird
	nab.u = ab.u;
	nab.v = ab.v;
	drawTriangle(c, a, nab, patchNum, recur + 1);
	drawTriangle(c, nab, b, patchNum, recur + 1);
    } else if (ebc) { // weird
	nbc.u = bc.u;
	nbc.v = bc.v;
	drawTriangle(a, b, nbc, patchNum, recur + 1);
	drawTriangle(a, nbc, c, patchNum, recur + 1);
    } else if (eca) {
	nca.u = ca.u;
	nca.v = ca.v;
	drawTriangle(b, c, nca, patchNum, recur + 1);
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
	    // change this
	    //for (int v = 0; v < 4; v++) {
	    //    for (int u = 0; u < 4; u++) {
	    //        patches[i]->pts[v*4+u].u = u / 3.0;
	    //        patches[i]->pts[v*4+u].v = v / 3.0;
	    //    }
	    //}
	    //for (int v = 0; v < 3; v++) {
	    //    for (int u = 0; u < 3; u++) {
	    //        drawTriangle(patches[i]->pts[v*4+u], patches[i]->pts[(v+1)*4+u], patches[i]->pts[v*4+u+1], i, 0);
	    //        drawTriangle(patches[i]->pts[(v+1)*4+u+1], patches[i]->pts[v*4+u+1], patches[i]->pts[(v+1)*4+u], i, 0);
	    //    }
	    //}
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
	    drawTriangle( d, b, c, i, 0);
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
    glLoadIdentity();				        // make sure transformation is "zero'd"
    glPushMatrix();
    glTranslatef(translate[0],translate[1],0);
    glRotatef(rotate[0], 1, 0, 0);
    glRotatef(rotate[1], 0, 1, 0);
    // Start drawing
    glColor3f(1.0f, 1.0f, 1.0f);
  
    if (adaptive) {
	adaptiveTessalation();
    } else {
	uniTesQuad();
    }
    glPopMatrix();
    glFlush();
    glutSwapBuffers();					// swap buffers (we earlier set double buffer)
    //printf("hello world");
}

void myDirectionalKeys(int key, int mouseX, int mouseY) {
    if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
	switch (key) {
	case GLUT_KEY_RIGHT:
	    //glTranslatef(1.0f, 0.0f, 1.0f);
	    translate[0] += 0.1;
	    break;
	case GLUT_KEY_LEFT:
	    //glTranslatef(-1.0f, 0.0f, 1.0f);
	    translate[0] -= 0.1;
	    break;
	case GLUT_KEY_UP:
	    //glTranslatef(0.0f, 1.0f, 0.0f);
	    translate[1] += 0.1;
	    break;
	case GLUT_KEY_DOWN:
	    //glTranslatef(0.0f, -1.0f, 0.0f);
	    translate[1] -= 0.1;
	    break;
	    // translate object with shift+arrow keys
	default:
	    break;
	}
    } else {
	switch (key) {
	case GLUT_KEY_RIGHT:
	    //glRotatef(ANGLE, 0.0f, 0.0f, 1.0f);
	    rotate[1] += 10.0;
	    break;
	case GLUT_KEY_LEFT:
	    //glRotatef(-ANGLE, 0.0f, 0.0f, 1.0f);
	    rotate[1] -= 10.0;
	    break;
	case GLUT_KEY_UP:
	    //glRotatef(ANGLE, 1.0f, 0.0f, 0.0f);
	    rotate[0] -= 10.0;
	    break;
	case GLUT_KEY_DOWN:
	    //glRotatef(-ANGLE, 1.0f, 0.0f, 0.0f);
	    rotate[0] += 10.0;
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
	if (smooth) {
	    glShadeModel(GL_FLAT);
	    smooth = false;
	} else {
	    glShadeModel(GL_SMOOTH);
	    smooth = true;
	}
	break;
    case 'w':
	if (lines) {
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	    glEnable(GL_LIGHTING);
	    glEnable(GL_LIGHT0);
	    glEnable(GL_LIGHT1);
	    glEnable(GL_DEPTH_TEST);
	    lines = false;
	} else {
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	    glDisable(GL_LIGHTING);
	    glDisable(GL_LIGHT0);
	    glDisable(GL_LIGHT1);
	    glDisable(GL_DEPTH_TEST);
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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    
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
