// -------------------------------------------
// gMini : a minimal OpenGL/GLUT application
// for 3D graphics.
// Copyright (C) 2006-2008 Tamy Boubekeur
// All rights reserved.
// -------------------------------------------

// -------------------------------------------
// Disclaimer: this code is dirty in the
// meaning that there is no attention paid to
// proper class attribute access, memory
// management or optimisation of any kind. It
// is designed for quick-and-dirty testing
// purpose.
// -------------------------------------------

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <GL/glut.h>
#include <float.h>
#include "src/Vec3.h"
#include "src/Camera.h"

#include <unistd.h>

enum DisplayMode{ WIRE=0, SOLID=1, LIGHTED_WIRE=2, LIGHTED=3 };

struct Triangle {
    inline Triangle () {
        v[0] = v[1] = v[2] = 0;
    }
    inline Triangle (const Triangle & t) {
        v[0] = t.v[0];   v[1] = t.v[1];   v[2] = t.v[2];
    }
    inline Triangle (unsigned int v0, unsigned int v1, unsigned int v2) {
        v[0] = v0;   v[1] = v1;   v[2] = v2;
    }
    unsigned int & operator [] (unsigned int iv) { return v[iv]; }
    unsigned int operator [] (unsigned int iv) const { return v[iv]; }
    inline virtual ~Triangle () {}
    inline Triangle & operator = (const Triangle & t) {
        v[0] = t.v[0];   v[1] = t.v[1];   v[2] = t.v[2];
        return (*this);
    }
    // membres :
    unsigned int v[3];
};


//La structure qui nous intéresse
struct Mesh {
    std::vector< Vec3 > vertices;       //Vertex    1) (Points)
    std::vector< Vec3 > normals;        //Normal    4) ...
    std::vector< Triangle > triangles;  //Triangle  2) (Topologie)
};

Mesh mesh;

//Mesh to generate
Mesh unit_sphere;
Mesh unit_cylinder;

bool display_normals;
bool display_loaded_mesh;
bool display_unit_sphere;
bool display_cylinder;
DisplayMode displayMode;

// -------------------------------------------
// OpenGL/GLUT application code.
// -------------------------------------------

static GLint window;
static unsigned int SCREENWIDTH = 1600;
static unsigned int SCREENHEIGHT = 900;
static Camera camera;
static bool mouseRotatePressed = false;
static bool mouseMovePressed = false;
static bool mouseZoomPressed = false;
static int lastX=0, lastY=0, lastZoom=0;
static bool fullScreen = false;

//Créer des sommets discrétisant (résolution de la sphère en soit)
// On prend notre sphère et on défini le nombre de méridien (vertical) et de parallèle (horizontal)
//  (facile)

int Nx = 20;
int Ny = 20;

void setUnitSphere( Mesh & o_mesh, int nX, int nY)
{
    //Un point 3D sur la sphère peut être obtenue à l'aide de la paramétrisation sphérique
    //fonctions de deux angles theta et phi appartenant à l'intervalle [0 ; 2*PI] x [-PI/2 ; PI/2]
    //x = cos(theta) * cos(phi)
    //y = sin(theta) * cos(phi)
    //z = sin(phi)

    float pi = M_PI;

    //Division de [0,2*PI] en nX points et [-PI/2, PI/2] en nY

    //Partie Verticies / points
    float teta = -1;
    float phi = -pi/2;
    float interX = 2*pi / (nX-1);
    float interY = pi / (nY-1);

    float x, y, z;
    float n1, n2, n3, n4;

    o_mesh.vertices.clear();
    o_mesh.normals.clear();
    
    o_mesh.vertices.push_back(Vec3(cos(2*pi)*cos(pi/2), sin(2*pi)*cos(pi/2), sin(pi/2)));
    o_mesh.normals.push_back(Vec3(cos(2*pi)*cos(pi/2), sin(2*pi)*cos(pi/2), sin(pi/2)));
    while(teta <= 2*pi) {
        x = cos(teta)*cos(phi);
        y = sin(teta)*cos(phi);
        z = sin(phi);

        o_mesh.vertices.push_back(Vec3(x, y, z));
        o_mesh.normals.push_back(Vec3(x, y, z));
        
        while(phi <= pi/2) {
            x = cos(teta)*cos(phi);
            y = sin(teta)*cos(phi);
            z = sin(phi);

            o_mesh.vertices.push_back(Vec3(x, y, z));
            o_mesh.normals.push_back(Vec3(x, y, z));
            printf("point %f %f %f placé \n", x, y, z);
            phi += interY;
        }
        teta += interX;
        phi = -pi/2;
    }

    //Topologies

    o_mesh.triangles.clear();
    for (int i=0; i<nX; i++) {
        for(int j=0; j<nY; j++) {
            n1 = i*nY+j;
            n2 = (i+1)*nY+j;
            n3 = (i+1)*nY+(j+1);
            n4 = i*nY+(j+1);

            o_mesh.triangles.push_back(Triangle(n1, n2, n3));
            o_mesh.triangles.push_back(Triangle(n1, n3, n4));
        }
    }
}

bool saveOFF( const std::string & filename ,
              std::vector< Vec3 > & i_vertices ,
              std::vector< Vec3 > & i_normals ,
              std::vector< Triangle > & i_triangles,
              bool save_normals = true ) {
    std::ofstream myfile;
    myfile.open(filename.c_str());
    if (!myfile.is_open()) {
        std::cout << filename << " cannot be opened" << std::endl;
        return false;
    }

    myfile << "OFF" << std::endl ;

    unsigned int n_vertices = i_vertices.size() , n_triangles = i_triangles.size();
    myfile << n_vertices << " " << n_triangles << " 0" << std::endl;

    for( unsigned int v = 0 ; v < n_vertices ; ++v ) {
        myfile << i_vertices[v][0] << " " << i_vertices[v][1] << " " << i_vertices[v][2] << " ";
        if (save_normals) myfile << i_normals[v][0] << " " << i_normals[v][1] << " " << i_normals[v][2] << std::endl;
        else myfile << std::endl;
    }
    for( unsigned int f = 0 ; f < n_triangles ; ++f ) {
        myfile << 3 << " " << i_triangles[f][0] << " " << i_triangles[f][1] << " " << i_triangles[f][2];
        myfile << std::endl;
    }
    myfile.close();
    return true;
}

    float n1, n2, n3, n4;
void openOFF( std::string const & filename,
              std::vector<Vec3> & o_vertices,
              std::vector<Vec3> & o_normals,
              std::vector< Triangle > & o_triangles,
              bool load_normals = true )
{
    std::ifstream myfile;
    myfile.open(filename.c_str());
    if (!myfile.is_open())
    {
        std::cout << filename << " cannot be opened" << std::endl;
        return;
    }

    std::string magic_s;

    myfile >> magic_s;

    if( magic_s != "OFF" )
    {
        std::cout << magic_s << " != OFF :   We handle ONLY *.off files." << std::endl;
        myfile.close();
        exit(1);
    }

    int n_vertices , n_faces , dummy_int;
    myfile >> n_vertices >> n_faces >> dummy_int;

    o_vertices.clear();
    o_normals.clear();

    for( int v = 0 ; v < n_vertices ; ++v )
    {
        float x , y , z ;

        myfile >> x >> y >> z ;
        o_vertices.push_back( Vec3( x , y , z ) );

        if( load_normals ) {
            myfile >> x >> y >> z;
            o_normals.push_back( Vec3( x , y , z ) );
        }
    }

    o_triangles.clear();
    for( int f = 0 ; f < n_faces ; ++f )
    {
        int n_vertices_on_face;
        myfile >> n_vertices_on_face;

        if( n_vertices_on_face == 3 )
        {
            unsigned int _v1 , _v2 , _v3;
            myfile >> _v1 >> _v2 >> _v3;

            o_triangles.push_back(Triangle( _v1, _v2, _v3 ));
        }
        else if( n_vertices_on_face == 4 )
        {
            unsigned int _v1 , _v2 , _v3 , _v4;
            myfile >> _v1 >> _v2 >> _v3 >> _v4;

            o_triangles.push_back(Triangle(_v1, _v2, _v3 ));
            o_triangles.push_back(Triangle(_v1, _v3, _v4));
        }
        else
        {
            std::cout << "We handle ONLY *.off files with 3 or 4 vertices per face" << std::endl;
            myfile.close();
            exit(1);
        }
    }

}


// ------------------------------------

void initLight () {
    GLfloat light_position1[4] = {22.0f, 16.0f, 50.0f, 0.0f};
    GLfloat direction1[3] = {-52.0f,-16.0f,-50.0f};
    GLfloat color1[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat ambient[4] = {0.3f, 0.3f, 0.3f, 0.5f};

    glLightfv (GL_LIGHT1, GL_POSITION, light_position1);
    glLightfv (GL_LIGHT1, GL_SPOT_DIRECTION, direction1);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, color1);
    glLightfv (GL_LIGHT1, GL_SPECULAR, color1);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, ambient);
    glEnable (GL_LIGHT1);
    glEnable (GL_LIGHTING);
}

void init () {
    camera.resize (SCREENWIDTH, SCREENHEIGHT);
    initLight ();
    glCullFace (GL_BACK);
    glEnable (GL_CULL_FACE);
    glDepthFunc (GL_LESS);
    glEnable (GL_DEPTH_TEST);
    glClearColor (0.2f, 0.2f, 0.3f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);

    displayMode = LIGHTED;
    display_normals = false;
    display_unit_sphere = false;
    display_cylinder = false;
    display_loaded_mesh = true;

    glLineWidth(1.);
    glPointSize(4.);
}

// ------------------------------------
// rendering.
// ------------------------------------

//Affichage avec les normales -> raccourcis N
void drawVector( Vec3 const & i_from, Vec3 const & i_to ) {
    glBegin(GL_LINES);
    glVertex3f( i_from[0] , i_from[1] , i_from[2] );
    glVertex3f( i_to[0] , i_to[1] , i_to[2] );
    glEnd();
}

//Affichage avec les vertexes raccourcis W
void drawVertices( Mesh const & i_mesh ) {
    glBegin(GL_POINTS); //Fonction OpenGL de dessin de points
    for(unsigned int vIt = 0 ; vIt < i_mesh.vertices.size(); ++vIt) {
        Vec3 p = i_mesh.vertices[vIt];

        glVertex3f( p[0] , p[1] , p[2] );
    }
    glEnd();
}

//Affiche les triangles qui comporte le mesh
void drawTriangleMesh( Mesh const & i_mesh ) {
    if( i_mesh.triangles.size() > 0 ){
        if( i_mesh.normals.size() > 0 ){
            //Fonction de dessin en utilisant les normales au sommet
            glBegin(GL_TRIANGLES); //Fonction OpenGL de dessin de triangles
            for(unsigned int tIt = 0 ; tIt < i_mesh.triangles.size(); ++tIt) {
                Vec3 p0 = i_mesh.vertices[i_mesh.triangles[tIt][0]];
                Vec3 n0 = i_mesh.normals[i_mesh.triangles[tIt][0]];

                Vec3 p1 = i_mesh.vertices[i_mesh.triangles[tIt][1]];
                Vec3 n1 = i_mesh.normals[i_mesh.triangles[tIt][1]];

                Vec3 p2 = i_mesh.vertices[i_mesh.triangles[tIt][2]];
                Vec3 n2 = i_mesh.normals[i_mesh.triangles[tIt][2]];

                glNormal3f( n0[0] , n0[1] , n0[2] );
                glVertex3f( p0[0] , p0[1] , p0[2] );
                glNormal3f( n1[0] , n1[1] , n1[2] );
                glVertex3f( p1[0] , p1[1] , p1[2] );
                glNormal3f( n2[0] , n2[1] , n2[2] );
                glVertex3f( p2[0] , p2[1] , p2[2] );
            }
            glEnd();
        } else {
            //Fonction de dessin en utilisant sans normales
            glBegin(GL_TRIANGLES); //Fonction OpenGL de dessin de triangles
            for(unsigned int tIt = 0 ; tIt < i_mesh.triangles.size(); ++tIt) {
                Vec3 p0 = i_mesh.vertices[i_mesh.triangles[tIt][0]];
                Vec3 p1 = i_mesh.vertices[i_mesh.triangles[tIt][1]];
                Vec3 p2 = i_mesh.vertices[i_mesh.triangles[tIt][2]];

                //Dessin des trois sommets formant le triangle
                glVertex3f( p0[0] , p0[1] , p0[2] );
                glVertex3f( p1[0] , p1[1] , p1[2] );
                glVertex3f( p2[0] , p2[1] , p2[2] );
            }
            glEnd();
        }
    } else {
        drawVertices(i_mesh);
    }

    if(display_normals){

        glColor3f(1.,0.,0.);
        for(unsigned int pIt = 0 ; pIt < i_mesh.normals.size() ; ++pIt) {
            Vec3 to = i_mesh.vertices[pIt] + 0.02*i_mesh.normals[pIt];
            drawVector(i_mesh.vertices[pIt], to);
        }

    }

}

//Raffraichi de différente manière 
// notre mesh
void draw () {
    if(displayMode == LIGHTED || displayMode == LIGHTED_WIRE){
        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_LIGHTING);

    }  else if(displayMode == WIRE){
        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
        glDisable (GL_LIGHTING);

    }  else if(displayMode == SOLID ){
        glDisable (GL_LIGHTING);
        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    }

    if( display_unit_sphere ){
        glColor3f(1,1,0);
        drawTriangleMesh(unit_sphere);
    }
    if (display_cylinder) {
        glColor3f(1,1,0);
        drawTriangleMesh(unit_cylinder);
    }

    if( display_loaded_mesh ){
        glColor3f(0.8,0.8,1);
        drawTriangleMesh(mesh);
    }

    if(displayMode == SOLID || displayMode == LIGHTED_WIRE){
        glEnable (GL_POLYGON_OFFSET_LINE);
        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth (1.0f);
        glPolygonOffset (-2.0, 1.0);

        glColor3f(0.,0.,0.);
        if( display_unit_sphere )
            drawTriangleMesh(unit_sphere);
        
        if( display_cylinder)
            drawTriangleMesh(unit_cylinder);

        if( display_loaded_mesh )
            drawTriangleMesh(mesh);

        glDisable (GL_POLYGON_OFFSET_LINE);
        glEnable (GL_LIGHTING);
    }
}

void changeDisplayMode(){
    if(displayMode == LIGHTED)
        displayMode = LIGHTED_WIRE;
    else if(displayMode == LIGHTED_WIRE)
        displayMode = SOLID;
    else if(displayMode == SOLID)
        displayMode = WIRE;
    else
        displayMode = LIGHTED;
}

//A définir
void display () {
    glLoadIdentity ();
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera.apply ();
    draw ();
    glFlush ();
    glutSwapBuffers ();
}

void idle () {
    glutPostRedisplay ();
}

//Fonction qui interprète les touches de l'utilisateur.
void key (unsigned char keyPressed, int x, int y) {
    switch (keyPressed) {
    case 'f':
        if (fullScreen == true) {
            glutReshapeWindow (SCREENWIDTH, SCREENHEIGHT);
            fullScreen = false;
        } else {
            glutFullScreen ();
            fullScreen = true;
        }
        break;


    case 'w': //Change le mode d'affichage
        changeDisplayMode();
        break;


    case 'n': //Press n key to display normals
        display_normals = !display_normals;
        break;

    case '1': //Toggle loaded mesh display
        display_loaded_mesh = !display_loaded_mesh;
        break;

    case '2': //Toggle unit sphere mesh display
        display_unit_sphere = !display_unit_sphere;
        display_cylinder = !display_cylinder;
        break;

    case '-':
        Nx--;
        Ny--;
        //setUnitSphere(unit_sphere, Nx, Ny);   //La méthode que l'on doit écrire
        break;

    case '+':
        Nx++;
        Ny++;
        //setUnitSphere(unit_sphere, Nx, Ny);   //La méthode que l'on doit écrire
        break;

    default:
        break;
    }
    idle ();
}

void mouse (int button, int state, int x, int y) {
    if (state == GLUT_UP) {
        mouseMovePressed = false;
        mouseRotatePressed = false;
        mouseZoomPressed = false;
    } else {
        if (button == GLUT_LEFT_BUTTON) {
            camera.beginRotate (x, y);
            mouseMovePressed = false;
            mouseRotatePressed = true;
            mouseZoomPressed = false;
        } else if (button == GLUT_RIGHT_BUTTON) {
            lastX = x;
            lastY = y;
            mouseMovePressed = true;
            mouseRotatePressed = false;
            mouseZoomPressed = false;
        } else if (button == GLUT_MIDDLE_BUTTON) {
            if (mouseZoomPressed == false) {
                lastZoom = y;
                mouseMovePressed = false;
                mouseRotatePressed = false;
                mouseZoomPressed = true;
            }
        }
    }
    idle ();
}

void motion (int x, int y) {
    if (mouseRotatePressed == true) {
        camera.rotate (x, y);
    }
    else if (mouseMovePressed == true) {
        camera.move ((x-lastX)/static_cast<float>(SCREENWIDTH), (lastY-y)/static_cast<float>(SCREENHEIGHT), 0.0);
        lastX = x;
        lastY = y;
    }
    else if (mouseZoomPressed == true) {
        camera.zoom (float (y-lastZoom)/SCREENHEIGHT);
        lastZoom = y;
    }
}

void reshape(int w, int h) {
    camera.resize (w, h);
}

void setCylinder(Mesh & o_mesh, float r, float h, float angle, int nX) {
    float pi = M_PI;

    o_mesh.vertices.clear();
    o_mesh.normals.clear();
    o_mesh.triangles.clear();

    float x, y;
    float n1, n2, n3;

    o_mesh.vertices.push_back(Vec3(0, 0, 0));
    for(float teta = 0; teta <= 2*pi ; teta += (2*pi)/nX) {
        x = r*cos(teta);
        y = r*sin(teta);
        
        o_mesh.vertices.push_back(Vec3(x, y, 0));
        o_mesh.normals.push_back(Vec3(x, y, 0));
    }
    
    o_mesh.vertices.push_back(Vec3(0, 0, h));
    for(float teta = 0; teta <= 2*pi ; teta += (2*pi)/nX) { 
        x = r*cos(teta);
        y = r*sin(teta);

        o_mesh.vertices.push_back(Vec3(x, y, h));
        o_mesh.normals.push_back(Vec3(x, y, h));
    }

    o_mesh.triangles.clear();
    
    o_mesh.triangles.push_back(Triangle(0, 1, nX));
    for (int i=0; i <= nX-2; i++) {
        n1 = 0;
        n2 = i+1;
        n3 = i+2;

        o_mesh.triangles.push_back(Triangle(n1, n3, n2));
    }

    for (int i=nX+1; i <= nX*2-1; i++) {
        n1 = nX+1;
        n2 = i+1;
        n3 = i+2;

        o_mesh.triangles.push_back(Triangle(n1, n2, n3));
    }
    o_mesh.triangles.push_back(Triangle(nX+1, nX*2+1, nX+2));

    
    for (int i=1; i <= nX-1 ; i++) {
        //if (i != nX+1 || i != nX) {
            n1 = i;
            n2 = i+nX;
            n3 = i+nX + 1;
            n4 = i+1;

            o_mesh.triangles.push_back(Triangle(n1, n3, n2));
            o_mesh.triangles.push_back(Triangle(n1, n4, n3));
        //}
    }
    ////////o_mesh.triangles.push_back(Triangle());
}

int main (int argc, char ** argv) {
    if (argc > 2) {
        exit (EXIT_FAILURE);
    }
    glutInit (&argc, argv);
    glutInitDisplayMode (GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize (SCREENWIDTH, SCREENHEIGHT);
    window = glutCreateWindow ("TP HAI714I");

    init ();
    glutIdleFunc (idle);
    glutDisplayFunc (display);
    glutKeyboardFunc (key);
    glutReshapeFunc (reshape);
    glutMotionFunc (motion);
    glutMouseFunc (mouse);
    key ('?', 0, 0);

    //Unit sphere mesh loaded with precomputed normals
    //Charge notre fichier dans notre structure.
    
    //Uncomment to see other meshes
    //openOFF("data/elephant_n.off", mesh.vertices, mesh.normals, mesh.triangles);

    openOFF("data/unit_sphere_n.off", mesh.vertices, mesh.normals, mesh.triangles);

    float rayon = 1.;
    float hauteur = 2.;
    float angle = 0.5;

    setCylinder(unit_cylinder, rayon, hauteur, angle, Nx);
    //setUnitSphere(unit_sphere, Nx, Ny);   //La méthode que l'on doit écrire

    glutMainLoop ();
    return EXIT_SUCCESS;
}
