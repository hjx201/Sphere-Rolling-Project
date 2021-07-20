/************************************************************
 * Handout: rotate-cube-new.cpp (A Sample Code for Shader-Based OpenGL ---
                                 for OpenGL version 3.1 and later)
 * Originally from Ed Angel's textbook "Interactive Computer Graphics" 6th Ed
              sample code "example3.cpp" of Chapter 4.
 * Moodified by Yi-Jen Chiang to include the use of a general rotation function
   Rotate(angle, x, y, z), where the vector (x, y, z) can have length != 1.0,
   and also to include the use of the function NormalMatrix(mv) to return the
   normal matrix (mat3) of a given model-view matrix mv (mat4).

   (The functions Rotate() and NormalMatrix() are added to the file "mat-yjc-new.h"
   by Yi-Jen Chiang, where a new and correct transpose function "transpose1()" and
   other related functions such as inverse(m) for the inverse of 3x3 matrix m are
   also added; see the file "mat-yjc-new.h".)

 * Extensively modified by Yi-Jen Chiang for the program structure and user
   interactions. See the function keyboard() for the keyboard actions.
   Also extensively re-structured by Yi-Jen Chiang to create and use the new
   function drawObj() so that it is easier to draw multiple objects. Now a floor
   and a rotating cube are drawn.

** Perspective view of a color cube using LookAt() and Perspective()

** Colors are assigned to each vertex and then the rasterizer interpolates
   those colors across the triangles.
**************************************************************/

#include "Angel-yjc.h"

#include <fstream>
#include <iostream>
using namespace std;

typedef Angel::vec3  color3;
typedef Angel::vec3  point3;

typedef Angel::vec4 color4;
typedef Angel::vec4 point4;

#define PI 3.141592653589793238463f

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

//GLuint program;       /* shader program object id */
GLuint program_shaded;       /* shader program object id */

GLuint floor_buffer;  /* vertex buffer object id for floor */
GLuint sphere_buffer;  /* vertex buffer object id for sphere */
GLuint axis_buffer;    /* vertex buffer object ids for axes */

// Projection transformation parameters
GLfloat  fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat  aspect;       // Viewport aspect ratio
GLfloat  zNear = 0.5, zFar = 15.0;

//----------------------------------------------------------------------
//globals from HW2

//vec4 init_eye(3.0, 2.0, 0.0, 1.0); // initial viewer position
vec4 init_eye(7.0, 3.0, -10.0, 1.0); // initial viewer position
vec4 eye = init_eye;               // current viewer position

const int axis_NumVertices = 9;
point3 axis_points[9];
color3 axis_colors[9];

const int floor_NumVertices = 6; //(1 face)*(2 triangles/face)*(3 vertices/triangle)
point3 floor_points[floor_NumVertices]; // positions for all vertices
color3 floor_colors[floor_NumVertices]; // colors for all vertices

// RGBA colors
color3 vertex_colors[9] = {
    color3(0.0, 0.0, 0.0),  // black
    color3(1.0, 0.0, 0.0),  // red
    color3(1.0, 1.0, 0.0),  // yellow
    color3(0.0, 1.0, 0.0),  // green
    color3(0.0, 0.0, 1.0),  // blue
    color3(1.0, 0.0, 1.0),  // magenta
    color3(1.0, 1.0, 1.0),  // white
    color3(0.0, 1.0, 1.0),   // cyan
    color3(1.0, .84, 0)       //golden yellow
};

//vertices of floor (changed for hw4)
point3 floorpoints[4] = {
    point3(-5.0, 0.0, 8.0),
    point3(5.0, 0.0, 8.0),
    point3(5.0, 0.0, -4.0),
    point3(-5.0, 0.0, -4.0)
};

//sphere variables
int sphere_NumVertices;
point3* sphere_points;  //arrays for storing sphere data
color3* sphere_colors;
float radius;

//transformation vars
#define NUMTARGETS 3
point3 targetpoints[] = { point3(-4.0, 1.0, 4.0), point3(3.0, 1.0, -4.0), point3(-3.0, 1.0, -3.0) }; //list of points to roll to
int currtarget;
point3 prevtarget; //stores sphere's previous point
point3 sphere_center; //initialize sphere center

mat4 rmatrix = Angel::identity(); //used to store cumulative rotations at target points.

point3 rotationaxis; //keeps current rotation axis
point3 transmat;

bool isInitialized = false; //flags for keyboard/mouse input
bool isBegun = false;
bool isRolling = true;


GLfloat angle = 0.0; // rotation angle in degrees

//method declarations
point3 diff(point3 a, point3 b);
point3 xprod(point3 a, point3 b);
float norm(point3 a);
point3 getRotationAxis(point3 ab);

void quit();
void initmenu();
void menu(int index);

void updateSphere();

//----------------------------------------------------------------------
//globals for HW3

//for shadow + shadow calculations
GLuint shadow_buffer;
color4* shadow_colors;

point4 light = point4(-14.0, 12.0, -3.0, 1.0); //light source

mat4 shadowproj; //shadow projection matrix

//flags
bool isWireframe = 1;
bool EnableShadow = 0;
bool EnableLighting = 0;
bool isFlat = 0;
bool isPointSource = 1;
bool EnablePosLight = 0; //fix from HW3

//globals for HW3 part C
GLuint sphere_buffer_flat;
GLuint sphere_buffer_smooth;

vec3* flat_normals;     //data structures to hold normals for each triangle
vec3* smooth_normals;

vec3 floor_normals[6]; //used in ligthing the floor
point4 floor_points_shaded[6];
GLuint floor_buffer_shaded;

point4* sphere_points_shaded;

void setFlatNormal(point3 triangle[3], int i);
void setSmoothNormal(point3 point, int i);
void setFloorNormals();

//----------------------------------------------------------------------
//globals for HW4

enum FogType { NONE, LINEAR, EXPONENTIAL, EXSQUARE };

//part a
FogType fogtype; //fog types: linear, exponential, exsquare

//part b
bool isShadowBlended; //so many booleans :flushed emoji:

//part c
bool isFloorTextured;
    
//coordinates map to 24 x 20 chunk on checkerboard
vec2 floor_texcoords[6];

vec2 floor_texpoints[4] = {
    vec2(0.0, 0.0),
    vec2(5.0 / 4.0, 0.0),
    vec2(5.0 / 4.0, 6.0 / 4.0),
    vec2(0.0, 6.0 / 4.0),
};
GLuint floor_buffer_textured;

    //from texmap.c
    #define ImageWidth  32
    #define ImageHeight 32
    GLubyte Image[ImageHeight][ImageWidth][4];

    #define	stripeImageWidth 32
    GLubyte stripeImage[4 * stripeImageWidth];

    GLuint checkertex, stripetex;


//part d
bool isTexSlanted = false; //switch between vertical and slanted
bool isTexEye = false; //switch between eye space and object space for thingy

enum TextureType { OFF, CHECKER, STRIPE };
TextureType sphereTexture; //turn texturing for sphere on/off, set sphere texuture

//part e
bool isLatTilted = false; //switches betwen upright and tilted lattice effect
bool isLatticed = false;;    //toggles lattice effect

//part f
bool enableFireworks = false;
#define NPARTICLES 300

vec3 fireworkColors[NPARTICLES];
point3 fireworkVelocities[NPARTICLES];

GLuint program_firework;
GLuint firework_buffer;

float timeLastStarted;
float animTimeElapsed;
#define RESTART_TIME 10000

//calculate projection matrix for shadow w/ global light and plane at y=0
mat4 getDecalProjection() {

    float dot;
    mat4 shadowproj;

    vec4 ground = vec4(0, 1, 0, 0); //plane defined in terms of Ax + By + Cz + D = 0

    dot = ground.x * light.x + ground.y * light.y + ground.z * light.z;

    shadowproj[0][0] = dot - light.x * ground.x;
    shadowproj[0][1] = 0 - light.x * ground.y;
    shadowproj[0][2] = 0 - light.x * ground.z;
    shadowproj[0][3] = 0 - light.x * 0;

    shadowproj[1][0] = 0 - light.y * ground.x;
    shadowproj[1][1] = dot - light.y * ground.y;
    shadowproj[1][2] = 0 - light.y * ground.z;
    shadowproj[1][3] = 0 - light.y * 0;

    shadowproj[2][0] = 0 - light.z * ground.x;
    shadowproj[2][1] = 0 - light.z * ground.y;
    shadowproj[2][2] = dot - light.z * ground.z;
    shadowproj[2][3] = 0 - light.z * 0;

    shadowproj[3][0] = 0 - light.w * ground.x;
    shadowproj[3][1] = 0 - light.w * ground.y;
    shadowproj[3][2] = 0 - light.w * ground.z;
    shadowproj[3][3] = dot - light.w * 0;

    //cout << shadowproj;

    return shadowproj;
}

//set normals for flat shading (used in initsphere)
void setFlatNormal(point3 triangle[3], int i) {
    vec3 u = triangle[1] - triangle[0];
    vec3 v = triangle[2] - triangle[0];

    flat_normals[3 * i] = flat_normals[3 * i + 1] = flat_normals[3 * i + 2] = normalize(cross(u, v)); //normals are same for whole triangle
}

//set normals for smooth shading (used in initsphere)
void setSmoothNormal(point3 point, int i) {
    smooth_normals[i] = normalize(point);
}

//set normals of floor (called in initfloor)
void setFloorNormals() {
    vec3 u = floor_points[0] - floor_points[1];
    vec3 v = floor_points[2] - floor_points[1];

    vec3 floornorm = normalize(cross(v, u));

    for (int i = 0; i < 6; i++) {
        floor_normals[i] = floornorm; floor_points_shaded[i] = point4(floor_points[i].x, floor_points[i].y, floor_points[i].z, 1.0);
        //cout << floor_normals[i] << "\n";
    }
}

//initializing vertixes/colors for axis and floor
void initaxes() {
    axis_colors[0] = vertex_colors[1]; axis_points[0] = point3(0, 0, 0);
    axis_colors[1] = vertex_colors[1]; axis_points[1] = point3(0, 0, 0);
    axis_colors[2] = vertex_colors[1]; axis_points[2] = point3(10, 0, 0);

    axis_colors[3] = vertex_colors[5]; axis_points[3] = point3(0, 0, 0);
    axis_colors[4] = vertex_colors[5]; axis_points[4] = point3(0, 0, 0);
    axis_colors[5] = vertex_colors[5]; axis_points[5] = point3(0, 10, 0);

    axis_colors[6] = vertex_colors[4]; axis_points[6] = point3(0, 0, 0);
    axis_colors[7] = vertex_colors[4]; axis_points[7] = point3(0, 0, 0);
    axis_colors[8] = vertex_colors[4]; axis_points[8] = point3(0, 0, 10);
}

void initfloor() //changed for hw4
{
    floor_colors[0] = vertex_colors[3]; floor_points[0] = floorpoints[0]; floor_texcoords[0] = floor_texpoints[0];
    floor_colors[1] = vertex_colors[3]; floor_points[1] = floorpoints[1]; floor_texcoords[1] = floor_texpoints[1];
    floor_colors[2] = vertex_colors[3]; floor_points[2] = floorpoints[2]; floor_texcoords[2] = floor_texpoints[2];

    floor_colors[3] = vertex_colors[3]; floor_points[3] = floorpoints[0]; floor_texcoords[3] = floor_texpoints[0];
    floor_colors[4] = vertex_colors[3]; floor_points[4] = floorpoints[2]; floor_texcoords[4] = floor_texpoints[2];
    floor_colors[5] = vertex_colors[3]; floor_points[5] = floorpoints[3]; floor_texcoords[5] = floor_texpoints[3];

    setFloorNormals(); //part 3 thing
}

//HW2 Part A - gets input from file. assumes file is properly formatted
void initsphere() {
    radius = 1;

    float xi, yi, zi; //temporary params for x, y, z of each vertexy
    int numtrangles, n; //for now n will be a garbage variable, n will always be 3
    string filename;
    cout << "Enter filename of sphere file: ";
    cin >> filename;

    ifstream spherefile(filename);

    if (!spherefile) {
        cerr << "Couldn't open file";
        exit(1); //will exit if file can't be opened
    }

    spherefile >> numtrangles; //get number of triangles from file
    sphere_NumVertices = numtrangles * 3;   //get number of vertices
    sphere_points = new point3[sphere_NumVertices];
    sphere_colors = new color3[sphere_NumVertices];

    sphere_points_shaded = new point4[sphere_NumVertices];

    flat_normals = new vec3[sphere_NumVertices];
    smooth_normals = new vec3[sphere_NumVertices];

    //cout << numtrangles << "\n"; //debug code
    int iter = 0;

    point3 triangle[3];

    for (int i = 0; i < numtrangles; i++) {
        spherefile >> n;        //n is a garbage variable
        //cout << n << "\n"; //debug code
        for (int j = 0; j < 3; j++) {
            spherefile >> xi >> yi >> zi; //read x y and z of each point
            //cout << xi << " " << yi << " " << zi << "\n"; //debug code
            point3 point(xi, yi, zi);
            sphere_points[iter] = point;  //set points of sphere
            sphere_points_shaded[iter] = point4(xi, yi, zi, 1.0); //for shaded ver., needs to be point4
            sphere_colors[iter] = vertex_colors[8];
            //cout << sphere_points[iter].x << " " << sphere_points[iter].y << " " << sphere_points[iter].z << "\n";
            //cout << sphere_colors[iter].x << " " << sphere_colors[iter].y << " " << sphere_colors[iter].z << "\n";

            triangle[j] = point;

            setSmoothNormal(point, iter);

            iter++;
            //cout << sizeof(sphere_points);
        }
        setFlatNormal(triangle, i);
        //cout << "\n";
    }
}

//HW3 Part A - initialize shadow colors
void initshadow() {

    shadow_colors = new color4[sphere_NumVertices];

    shadowproj = getDecalProjection();

    for (int i = 0; i < sphere_NumVertices; i++) {
        shadow_colors[i] = color4(0.25, 0.25, 0.25, 0.65);
    }

}

/*************************************************************
void image_set_up(void):
  generate checkerboard and stripe images.

* Inside init(), call this function and set up texture objects
  for texture mapping.
  (init() is called from main() before calling glutMainLoop().)
***************************************************************/
void image_set_up(void)
{
    int i, j, c;

    /* --- Generate checkerboard image to the image array ---*/
    for (i = 0; i < ImageHeight; i++)
        for (j = 0; j < ImageWidth; j++)
        {
            c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0));

            if (c == 1) /* white */
            {
                c = 255;
                Image[i][j][0] = (GLubyte)c;
                Image[i][j][1] = (GLubyte)c;
                Image[i][j][2] = (GLubyte)c;
            }
            else  /* green */
            {
                Image[i][j][0] = (GLubyte)0;
                Image[i][j][1] = (GLubyte)150;
                Image[i][j][2] = (GLubyte)0;
            }

            Image[i][j][3] = (GLubyte)255;
        }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /*--- Generate 1D stripe image to array stripeImage[] ---*/
    for (j = 0; j < stripeImageWidth; j++) {
        /* When j <= 4, the color is (255, 0, 0),   i.e., red stripe/line.
           When j > 4,  the color is (255, 255, 0), i.e., yellow remaining texture
         */
        stripeImage[4 * j] = (GLubyte)255;
        stripeImage[4 * j + 1] = (GLubyte)((j > 4) ? 255 : 0);
        stripeImage[4 * j + 2] = (GLubyte)0;
        stripeImage[4 * j + 3] = (GLubyte)255;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /*----------- End 1D stripe image ----------------*/

    /*--- texture mapping set-up is to be done in
          init() (set up texture objects),
          display() (activate the texture object to be used, etc.)
          and in shaders.
     ---*/

} /* end function */

void initTextures() {
    image_set_up(); //set up image textures

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /*--- Create and Initialize a texture object ---*/
    glGenTextures(1, &checkertex);      // Generate texture obj name(s)

    // initialize checker thing
    glActiveTexture(GL_TEXTURE0);  // Set the active texture unit to be 0 
    glBindTexture(GL_TEXTURE_2D, checkertex); // Bind the texture to this texture unit

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImageWidth, ImageHeight,
        0, GL_RGBA, GL_UNSIGNED_BYTE, Image);

    //initialize stripe thing
    glActiveTexture(GL_TEXTURE1);  // Set the active texture unit to be 0 
    glBindTexture(GL_TEXTURE_2D, stripetex); // Bind the texture to this texture unit

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, stripeImageWidth,
        0, GL_RGBA, GL_UNSIGNED_BYTE, stripeImage);
}

float random() {
    return (rand() % 256) / 256.0;
}

void updateFireworks();

void initFireworks() {
    float xi, yi, zi;
    for (int i = 0; i < NPARTICLES; i++) {
        xi = 2.0 * (random() - 0.5);
        yi = 1.2 * 2.0 * random();
        zi = 2.0 * (random() - 0.5);
        fireworkVelocities[i] = vec3(xi, yi, zi);
        fireworkColors[i] = vec3(random(), random(), random());
        //cout << fireworkColors[i].y << '\n';
    }
    timeLastStarted = 0;
    animTimeElapsed = 0;

    updateFireworks();
}


//----------------------------------------------------------------------------
// OpenGL initialization
void init()
{
#if 0 //YJC: The following is not needed
    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#endif

    initsphere();
    //buffer for sphere
    glGenBuffers(1, &sphere_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
    //sphere buffer needs to be initialized differently as it is an array pointer
    glBufferData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point3) + sphere_NumVertices * sizeof(color3),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_NumVertices * sizeof(point3), sphere_points);
    glBufferSubData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point3), sphere_NumVertices * sizeof(color3),
        sphere_colors);

    //initialize floor
    initfloor();
    // Create and initialize a vertex buffer object for floor, to be used in display()
    glGenBuffers(1, &floor_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);

    glBufferData(GL_ARRAY_BUFFER, sizeof(floor_points) + sizeof(floor_colors),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_points), floor_points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points), sizeof(floor_colors),
        floor_colors);

    initaxes(); //do same for axes
    glGenBuffers(1, &axis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);

    glBufferData(GL_ARRAY_BUFFER, sizeof(axis_points) + sizeof(axis_colors),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(axis_points), axis_points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(axis_points), sizeof(axis_colors),
        axis_colors);


    //HW3

    initshadow(); //do same for shadow
    glGenBuffers(1, &shadow_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, shadow_buffer);
    //sphere buffer needs to be initialized differently as it is an array pointer
    glBufferData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point3) + sphere_NumVertices * sizeof(color4),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_NumVertices * sizeof(point3), sphere_points);
    glBufferSubData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point3), sphere_NumVertices * sizeof(color4),
        shadow_colors);


    //buffer for shaded floor
    glGenBuffers(1, &floor_buffer_shaded);
    glBindBuffer(GL_ARRAY_BUFFER, floor_buffer_shaded);

    glBufferData(GL_ARRAY_BUFFER, sizeof(floor_points_shaded) + sizeof(floor_normals),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_points_shaded), floor_points_shaded);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points_shaded), sizeof(floor_normals),
        floor_normals);

    //buffers for shaded spheres
    glGenBuffers(1, &sphere_buffer_flat);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer_flat);
    //sphere buffer needs to be initialized differently as it is an array pointer
    glBufferData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point4) + sphere_NumVertices * sizeof(vec3),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_NumVertices * sizeof(point4), sphere_points_shaded);
    glBufferSubData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point4), sphere_NumVertices * sizeof(vec3),
        flat_normals);

    glGenBuffers(1, &sphere_buffer_smooth);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer_smooth);
    //sphere buffer needs to be initialized differently as it is an array pointer
    glBufferData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point4) + sphere_NumVertices * sizeof(vec3),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_NumVertices * sizeof(point4), sphere_points_shaded);
    glBufferSubData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point4), sphere_NumVertices * sizeof(vec3),
        smooth_normals);

    //HW4
    initTextures();

    //create buffer for textured floor
    glGenBuffers(1, &floor_buffer_textured);
    glBindBuffer(GL_ARRAY_BUFFER, floor_buffer_textured);

    glBufferData(GL_ARRAY_BUFFER, sizeof(floor_points_shaded) + sizeof(floor_normals) + sizeof(floor_texcoords),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_points_shaded), floor_points_shaded);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points_shaded), sizeof(floor_normals),
        floor_normals);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points_shaded) + sizeof(floor_normals), sizeof(floor_texcoords),
        floor_texcoords);

    initFireworks();

    //create fireworks buffer
    glGenBuffers(1, &firework_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, firework_buffer);

    glBufferData(GL_ARRAY_BUFFER, sizeof(fireworkVelocities) + sizeof(fireworkColors),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fireworkVelocities), fireworkVelocities);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(fireworkVelocities), sizeof(fireworkColors),
        fireworkColors);


    // Load shaders and create a shader program (to be used in display())
    //program = InitShader("vshader42.glsl", "fshader42.glsl");
    //only shaded program is needed
    program_shaded = InitShader("vshader53.glsl", "fshader53.glsl");
    program_firework = InitShader("vshaderfire.glsl", "fshaderfire.glsl");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.529, 0.807, 0.92, 0.0); //blue bgcolor
    glLineWidth(2.0);

    currtarget = 0;
    sphere_center = targetpoints[0]; //initialize sphere center
    prevtarget = sphere_center;
    updateSphere();
}

//----------------------------------------------------------------------------
// Maths for sphere transformation
point3 diff(point3 b, point3 a) { //calculate vector from point a to point b
    return b - a;
}

point3 xprod(point3 a, point3 b) { //cross product
    return point3(a.y * b.z - a.z * b.y,
        a.x * b.z - a.z * b.x,
        a.x * b.y - a.y * b.x);
}

float norm(point3 a) {
    return sqrt(a.x * a.x + a.y * a.y + a.z * a.z); //calculate norm of vector a
}

float getDistance(point3 a, point3 b) {
    float delx = a.x - b.x;
    float delz = a.z - b.z;
    return sqrt(delx * delx + delz * delz); //calculate euclidean distance between points
}

point3 getRotationAxis(point3 ab) {
    return xprod(point3(0, 1, 0), ab);
}

point3 getTranslation(point3 ab) {
    float d = (angle * PI * float(radius)) / 180.0;
    point3 transm = ab * (1 / norm(ab));
    return transm * d;
}

//updates the sphere
void updateSphere() {
    //angle += 0.02;  //increment angle
    angle += 0.05;  //increment angle
    //angle += 0.1;  //increment angle
    //angle += 1;  //increment angle

    point3 ab;

    //if sphere close to target
    if (getDistance(sphere_center, targetpoints[currtarget]) < 0.02) {
        if (isInitialized) {
            rmatrix = Rotate(angle, rotationaxis.x, rotationaxis.y, rotationaxis.z) * rmatrix; //save current rotation
            //cout << "rmatrix set to: " << rmatrix << "\n";
        }
        isInitialized = true;

        prevtarget = sphere_center; //shift target point
        if (currtarget >= 2) {
            currtarget = 0;
        }
        else
            currtarget++;

        ab = diff(targetpoints[currtarget], prevtarget); //find ab for rotation axis
        rotationaxis = getRotationAxis(ab);
        angle = 0; //reset angle
    }

    ab = diff(targetpoints[currtarget], prevtarget); //find ab for translation (probably redundant)
    transmat = getTranslation(ab);
    sphere_center = prevtarget + transmat;
}

//HW3 Part C - pass floor material information into shader
void setfloormaterial() {
    color4 floor_diffuse(0.0, 1.0, 0.0, 1.0); //floor material colors
    color4 floor_ambient(0.2, 0.2, 0.2, 1.0);
    color4 floor_specular(0.0, 0.0, 0.0, 1.0);
    float shininess = 125.0;

    glUniform4fv(glGetUniformLocation(program_shaded, "MaterialAmbient"), 1,
        floor_ambient);
    glUniform4fv(glGetUniformLocation(program_shaded, "MaterialDiffuse"), 1,
        floor_diffuse);
    glUniform4fv(glGetUniformLocation(program_shaded, "MaterialSpecular"), 1,
        floor_specular);
    glUniform1f(glGetUniformLocation(program_shaded, "Shininess"), shininess);
}

void setspherematerial() {
    color4 sphere_diffuse(1.0, 0.84, 0.0, 1.0); //sphere material colors
    color4 sphere_ambient(0.2, 0.2, 0.2, 1.0);
    color4 sphere_specular(1.0, 0.84, 0.0, 1.0);
    float shininess = 125.0;

    glUniform4fv(glGetUniformLocation(program_shaded, "MaterialAmbient"), 1,
        sphere_ambient);
    glUniform4fv(glGetUniformLocation(program_shaded, "MaterialDiffuse"), 1,
        sphere_diffuse);
    glUniform4fv(glGetUniformLocation(program_shaded, "MaterialSpecular"), 1,
        sphere_specular);
    glUniform1f(glGetUniformLocation(program_shaded, "Shininess"), shininess);
}

//light variables
color4 global_ambient(1.0, 1.0, 1.0, 1.0); //global ambient light

//directional light source
color4 dirlight_ambient(0.0, 0.0, 0.0, 1.0);
color4 dirlight_specular(0.2, 0.2, 0.2, 1.0);
color4 dirlight_diffuse(0.8, 0.8, 0.8, 1.0);

vec3 dirlight_direction(0.1, 0.0, -1.0);

//sets light stuff
void setdirectionallight(mat4 mv) {
    //passes light params into the shader
    glUniform4fv(glGetUniformLocation(program_shaded, "GlobAmbient"),
        1, global_ambient);

    glUniform4fv(glGetUniformLocation(program_shaded, "DirAmbient"),
        1, dirlight_ambient);
    glUniform4fv(glGetUniformLocation(program_shaded, "DirDiffuse"),
        1, dirlight_diffuse);
    glUniform4fv(glGetUniformLocation(program_shaded, "DirSpecular"),
        1, dirlight_specular);

    glUniform3fv(glGetUniformLocation(program_shaded, "DirLightDir"),
        1, dirlight_direction);
}

//positional light source
color4 poslight_ambient(0.0, 0.0, 0.0, 1.0);
color4 poslight_specular(1.0, 1.0, 1.0, 1.0);
color4 poslight_diffuse(1.0, 1.0, 1.0, 1.0);

float const_att = 2.0;
float linear_att = 0.01;
float quad_att = 0.001;

vec4 spot_direction = (-6.0, 0.0, -4.5, 1.0);
float spot_angle = 20.0;
float spot_exponent = 15.0;

//
void setposlight(mat4 mv) {
    //passes light params into the shader
    glUniform1f(glGetUniformLocation(program_shaded, "EnablePosLight"),
        float(EnablePosLight));

    glUniform4fv(glGetUniformLocation(program_shaded, "LightAmbient"),
        1, poslight_ambient);
    glUniform4fv(glGetUniformLocation(program_shaded, "LightDiffuse"),
        1, poslight_diffuse);
    glUniform4fv(glGetUniformLocation(program_shaded, "LightSpecular"),
        1, poslight_specular);

    vec4 light_position_eyeFrame = mv * light;
    glUniform4fv(glGetUniformLocation(program_shaded, "LightPosition"), //point4(-14.0, 12.0, -3.0, 1.0)
        1, light_position_eyeFrame);

    glUniform1f(glGetUniformLocation(program_shaded, "ConstAtt"),
        const_att);
    glUniform1f(glGetUniformLocation(program_shaded, "LinearAtt"),
        linear_att);
    glUniform1f(glGetUniformLocation(program_shaded, "QuadAtt"),
        quad_att);

    if (isPointSource) {
        glUniform1f(glGetUniformLocation(program_shaded, "isPointSource"),
            1.0);
    }
    else {
        glUniform1f(glGetUniformLocation(program_shaded, "isPointSource"),
            0.0);

        vec4 spot_direction_eyeFrame = mv * spot_direction;
        glUniform4fv(glGetUniformLocation(program_shaded, "SpotDirection"),
            1, spot_direction_eyeFrame);

        glUniform1f(glGetUniformLocation(program_shaded, "SpotAngle"),
            cos(spot_angle * PI / 180));

        glUniform1f(glGetUniformLocation(program_shaded, "SpotExp"),
            spot_exponent);


    }
}

void setfog() {
    glUniform4fv(glGetUniformLocation(program_shaded, "Eye"),
        1, eye);

    glUniform1f(glGetUniformLocation(program_shaded, "FogType"),
        float(fogtype));
}

void setSphereTexFlags() {
    glUniform1f(glGetUniformLocation(program_shaded, "isTexSphere"),
        1.0);
    glUniform1f(glGetUniformLocation(program_shaded, "isTexSlanted"),
        float(isTexSlanted));
    glUniform1f(glGetUniformLocation(program_shaded, "isTexEye"),
        float(isTexEye));
    glUniform1f(glGetUniformLocation(program_shaded, "TexType"),
        float(sphereTexture));

    glUniform1i(glGetUniformLocation(program_shaded, "texture_1D"), 1);
    glUniform1i(glGetUniformLocation(program_shaded, "texture_2D"), 0);
}

void setSphereLatFlags() {
    glUniform1f(glGetUniformLocation(program_shaded, "isLatticed"),
        float(isLatticed));
    glUniform1f(glGetUniformLocation(program_shaded, "isLatTilted"),
        float(isLatTilted));
}

void unsetSphereFlags() {
    glUniform1f(glGetUniformLocation(program_shaded, "isTexSphere"),
        0.0);
    glUniform1f(glGetUniformLocation(program_shaded, "isLatticed"),
        0.0);
}

//----------------------------------------------------------------------------
// drawObj(buffer, num_vertices):
//   draw the object that is associated with the vertex buffer object "buffer"
//   and has "num_vertices" vertices.
//

//altered so that program only uses one shader

//draw unshaded object
void drawObj(GLuint buffer, int num_vertices)
{
    glUniform1f(glGetUniformLocation(program_shaded, "PerformShading"),
        0.0);
    glUniform1f(glGetUniformLocation(program_shaded, "hasAlpha"),
        0.0);
    glUniform1f(glGetUniformLocation(program_shaded, "isTextured"),
        0.0);

    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition3 = glGetAttribLocation(program_shaded, "vPosition3");
    glEnableVertexAttribArray(vPosition3);
    glVertexAttribPointer(vPosition3, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(0));

    GLuint vColor3 = glGetAttribLocation(program_shaded, "vColor3");
    glEnableVertexAttribArray(vColor3);
    glVertexAttribPointer(vColor3, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(point3) * num_vertices));
    // the offset is the (total) size of the previous vertex attribute array(s)

  /* Draw a sequence of geometric objs (triangles) from the vertex buffer
     (using the attributes specified in each enabled vertex attribute array) */
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition3);
    glDisableVertexAttribArray(vColor3);
}   

//draw unshaded object w/ alpha channel
void drawObj_alpha(GLuint buffer, int num_vertices) //draws object with an alpha channel
{
    glUniform1f(glGetUniformLocation(program_shaded, "PerformShading"),
        0.0);
    glUniform1f(glGetUniformLocation(program_shaded, "hasAlpha"),
        1.0);
    glUniform1f(glGetUniformLocation(program_shaded, "isTextured"),
        0.0);

    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition3 = glGetAttribLocation(program_shaded, "vPosition3");
    glEnableVertexAttribArray(vPosition3);
    glVertexAttribPointer(vPosition3, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(0));

    GLuint vColor4 = glGetAttribLocation(program_shaded, "vColor4");
    glEnableVertexAttribArray(vColor4);
    glVertexAttribPointer(vColor4, 4, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(point3) * num_vertices));
    // the offset is the (total) size of the previous vertex attribute array(s)

  /* Draw a sequence of geometric objs (triangles) from the vertex buffer
     (using the attributes specified in each enabled vertex attribute array) */
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition3);
    glDisableVertexAttribArray(vColor4);
}

//draw shaded object
void drawObj_shaded(GLuint buffer, int num_vertices) {
    glUniform1f(glGetUniformLocation(program_shaded, "PerformShading"),
        1.0);

    glUniform1f(glGetUniformLocation(program_shaded, "isTextured"),
        0.0);

    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(program_shaded, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(0));

    GLuint vNormal = glGetAttribLocation(program_shaded, "vNormal");
    glEnableVertexAttribArray(vNormal);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(point4) * num_vertices));
    // the offset is the (total) size of the previous vertex attribute array(s)

  /* Draw a sequence of geometric objs (triangles) from the vertex buffer
     (using the attributes specified in each enabled vertex attribute array) */
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vNormal);
}

//draw object w/ texcoords in buffer, specifically the floor (ik, misleading name)
void drawObj_textured(GLuint buffer, int num_vertices) {
    glUniform1f(glGetUniformLocation(program_shaded, "PerformShading"),
        1.0);

    glUniform1f(glGetUniformLocation(program_shaded, "isTextured"),
        1.0);

    glUniform1i(glGetUniformLocation(program_shaded, "texture_2D"), 0);

    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(program_shaded, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(0));

    GLuint vNormal = glGetAttribLocation(program_shaded, "vNormal");
    glEnableVertexAttribArray(vNormal);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(point4) * num_vertices));

    GLuint vTexCoord = glGetAttribLocation(program_shaded, "vTexCoord");
    glEnableVertexAttribArray(vTexCoord);
    glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET((sizeof(point4) + sizeof(point3)) * num_vertices));

    // the offset is the (total) size of the previous vertex attribute array(s)

  /* Draw a sequence of geometric objs (triangles) from the vertex buffer
     (using the attributes specified in each enabled vertex attribute array) */
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vNormal);
    glDisableVertexAttribArray(vTexCoord);

    glUniform1f(glGetUniformLocation(program_shaded, "isTextured"),
        0.0);
}


void updateFireworks() {
    if (enableFireworks == true) {
        animTimeElapsed = (float)glutGet(GLUT_ELAPSED_TIME) - timeLastStarted;
    
        if (animTimeElapsed > RESTART_TIME) {
            timeLastStarted = (float)glutGet(GLUT_ELAPSED_TIME);
            animTimeElapsed = 0;
        }
    }
    //cout << animTimeElapsed << "\n";
}

//used in drawing fireworks
void drawParticles(GLuint buffer, int num_particles) {

    glUniform1f(glGetUniformLocation(program_firework, "time"),
        animTimeElapsed);   //pass animtimeelapsed into vertex shader

    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint pVelocity = glGetAttribLocation(program_firework, "pVelocity");
    glEnableVertexAttribArray(pVelocity);
    glVertexAttribPointer(pVelocity, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(0));

    GLuint pColor = glGetAttribLocation(program_firework, "pColor");
    glEnableVertexAttribArray(pColor);
    glVertexAttribPointer(pColor, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(vec3) * num_particles));
    // the offset is the (total) size of the previous vertex attribute array(s)

  /* Draw a sequence of geometric objs (triangles) from the vertex buffer
     (using the attributes specified in each enabled vertex attribute array) */
    glDrawArrays(GL_TRIANGLES, 0, num_particles);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(pVelocity);
    glDisableVertexAttribArray(pColor);
}

//moved floor's drawing code into method to make it easier to call again for shadow blending
void drawFloor(mat3 normal_matrix_floor , mat4 mv) {
    if (eye.y > 0) glDepthMask(GL_FALSE); //disable and re-enable z-buffer for floor (for drawing correct shadow)
    if (EnableLighting) {
        setdirectionallight(mv);
        setposlight(mv);
        setfog();
        setfloormaterial();
        glUniformMatrix3fv(glGetUniformLocation(program_shaded, "Normal_Matrix"),
            1, GL_TRUE, normal_matrix_floor);
        if (isFloorTextured) {
            drawObj_textured(floor_buffer_textured, floor_NumVertices);
        }
        else drawObj_shaded(floor_buffer_shaded, floor_NumVertices); //draw the floor
    }
    else {
        drawObj(floor_buffer, floor_NumVertices);  // draw the floor
    }
    if (eye.y > 0) glDepthMask(GL_TRUE); //re-enable depth
}

//----------------------------------------------------------------------------
void display(void)
{
    //(display program can be streamlined)

    GLuint  model_view;  // model-view matrix uniform shader variable location
    GLuint  projection;  // projection matrix uniform shader variable location

    /*---  Set up and pass on Model-View matrix to the shader ---*/
    // eye is a global variable of vec4 set to init_eye and updated by keyboard()
    vec4    at(0.0, 0.0, 0.0, 1.0);
    vec4    up(0.0, 1.0, 0.0, 0.0);

    mat4 mv = LookAt(eye, at, up);

    mat4 eyeframe = mv;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*---  Set up and pass on Projection matrix to the shader ---*/
    mat4  p = Perspective(fovy, aspect, zNear, zFar);

    //initialize all model matrices here
    glUseProgram(program_shaded); // Use the shader program
    model_view = glGetUniformLocation(program_shaded, "ModelView"); //unsure if setting this multiple times is necessary but it works
    projection = glGetUniformLocation(program_shaded, "Projection");
    mat3 normal_matrix_floor = NormalMatrix(mv, 1);

    mat4 mv_sphere = rmatrix;
    mv_sphere = Rotate(angle, rotationaxis.x, rotationaxis.y, rotationaxis.z) * mv_sphere; //rotate first
    mv_sphere = Translate(sphere_center) * mv_sphere; //then translate
    mat4 mv_shadow = mv_sphere; //save this for later
    mv_sphere = mv * mv_sphere; //then look

    mat3 normal_matrix_sphere = NormalMatrix(mv_sphere, 1);

    mv_shadow = shadowproj * mv_shadow;
    mv_shadow = mv * mv_shadow;

/*----- Set up the Mode-View matrix for the floor -----*/
    glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    drawObj(axis_buffer, axis_NumVertices);  // draw the axes

    //draw the floor
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    drawFloor(normal_matrix_floor, mv);

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major

    //draw the sphrer
    if (isWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv_sphere); //set model matrix for sphere

    if (EnableLighting && !isWireframe) {

        setdirectionallight(eyeframe);
        setposlight(eyeframe);
        setspherematerial();
        glUniformMatrix3fv(glGetUniformLocation(program_shaded, "Normal_Matrix"),
            1, GL_TRUE, normal_matrix_sphere);
        
        if (sphereTexture != OFF) {
            setSphereTexFlags();
        }
        setSphereLatFlags();

        if (isFlat) {
            drawObj_shaded(sphere_buffer_flat, sphere_NumVertices);
        }
        else {
            drawObj_shaded(sphere_buffer_smooth, sphere_NumVertices);
        }

        unsetSphereFlags();
    }
    else {
        drawObj(sphere_buffer, sphere_NumVertices);
    }

    //draw the shadowo
    if (EnableShadow && eye.y > 0) {
        glUniformMatrix4fv(model_view, 1, GL_TRUE, mv_shadow); // GL_TRUE: matrix is row-major
        glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major

        if (!isWireframe && EnableLighting) {
            setSphereLatFlags();
        }

        if (isShadowBlended) {
            glEnable(GL_BLEND); //disable/enable alphas
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);  //disable z buffer for floor
        }

        drawObj_alpha(shadow_buffer, sphere_NumVertices);

        if (isShadowBlended) {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE); //re enable z buffer
            //write floor to z buffer only
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            drawFloor(normal_matrix_floor, mv);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }

        unsetSphereFlags();
    }

    if (enableFireworks) {
        glUseProgram(program_firework);

        model_view = glGetUniformLocation(program_firework, "ModelView"); //unsure if setting this multiple times is necessary but it works
        projection = glGetUniformLocation(program_firework, "Projection");

        glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
        glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major

        //firework stuff
        glPointSize(3.0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        drawParticles(firework_buffer, NPARTICLES);
    }

    glutSwapBuffers();
}
//---------------------------------------------------------------------------
void idle(void)
{
    updateSphere(); //update sphere in this method
    updateFireworks();
    glutPostRedisplay();
}
//----------------------------------------------------------------------------
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 033: // Escape Key
    case 'q': case 'Q':
        quit();
        //exit( EXIT_SUCCESS );
        break;

    case 'X': eye[0] += 1.0; break;
    case 'x': eye[0] -= 1.0; break;
    case 'Y': eye[1] += 1.0; break;
    case 'y': eye[1] -= 1.0; break;
    case 'Z': eye[2] += 1.0; break;
    case 'z': eye[2] -= 1.0; break;

    case 'b': case'B':
        isBegun = true;

        if (isBegun)
            glutIdleFunc(idle);
        else
            glutIdleFunc(NULL);
        break;

    case 'v': case 'V':
        if (sphereTexture != OFF) isTexSlanted = false;
        break;
    case 's': case 'S':
        if (sphereTexture != OFF) isTexSlanted = true;
        break;

    case 'o': case 'O':
        if (sphereTexture != OFF) isTexEye = false;
        break;
    case 'e': case 'E':
        if (sphereTexture != OFF) isTexEye = true;
        break;

        //lattice cases
    case 'l': case 'L':
        isLatticed = !isLatticed;
        break;

    case 'u': case 'U':
        if (isLatticed) isLatTilted = false;
        break;
    case 't': case 'T':
        if (isLatticed) isLatTilted = true;
        break;

    case ' ':  // reset to initial viewer/eye position
        eye = init_eye;
        break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP && isBegun)
        isRolling = !isRolling;

    if (isRolling && isBegun)
        glutIdleFunc(idle);
    else
        glutIdleFunc(NULL);
}

//----------------------------------------------------------------------------
void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    aspect = (GLfloat)width / (GLfloat)height;
    glutPostRedisplay();
}
//----------------------------------------------------------------------------

void shadow_menu(int index) {
    switch (index) {
    case(3):
        EnableShadow = false;
        break;
    case(4):
        EnableShadow = true;
        break;
    }

    display();
}

void lighting_menu(int index) {
    switch (index) {
    case(5):
        EnableLighting = true;
        break;
    case(6):
        EnableLighting = false;
        isFloorTextured = false;
        sphereTexture = OFF;
        break;
    }

    display();
}

void shading_menu(int index) {
    switch (index) {
    case(7):
        isFlat = true;
        EnableLighting = true;
        isWireframe = false;
        break;
    case(8):
        isFlat = false;
        EnableLighting = true;
        isWireframe = false;
        break;
    }

    display();
}

void lightsource_menu(int index) {
    switch (index) {
    case(9):
        isPointSource = true;
        EnablePosLight = true;

        break;
    case(10):
        isPointSource = false;
        EnablePosLight = true;

        break;
    case(11):
        EnablePosLight = false;
        break;
    }
    display();
}

void fog_menu(int index) {
    EnableLighting = true;

    switch (index) {
    case(11):
        fogtype = NONE;
        break;
    case(12):
        fogtype = LINEAR;
        break;
    case(13):
        fogtype = EXPONENTIAL;
        break;
    case(14):
        fogtype = EXSQUARE;
        break;
    }

    //cout << float(fogtype);
    display();
}

void blendshadow_menu(int index) {
    switch (index) {
    case(15):
        isShadowBlended = true;
        break;
    case(16):
        isShadowBlended = false;
        break;
    }

    display();
}

void texground_menu(int index) {
    switch (index) {
    case(17):
        EnableLighting = true;
        isFloorTextured = true;
        break;
    case(18):
        isFloorTextured = false;
        break;
    }

    display();
}

void texsphere_menu(int index) {
    EnableLighting = true;
    isWireframe = false;

    switch (index) {
    case(19):
        sphereTexture = STRIPE;
        break;
    case(20):
        sphereTexture = CHECKER;
        break;
    case(21):
        sphereTexture = OFF;
        break;
    }

    display();
}

void firework_menu(int index) {
    switch (index) {
    case(22):
        enableFireworks = true;
        timeLastStarted = (float)glutGet(GLUT_ELAPSED_TIME);
        animTimeElapsed = 0;
        break;
    case(23):
        enableFireworks = false;
        break;
    }

    display();
}

//initialize menu for Part E
void initmenu() {
    int shadow = glutCreateMenu(shadow_menu);
    glutAddMenuEntry("Yes", 4);
    glutAddMenuEntry("No", 3);

    int lighting = glutCreateMenu(lighting_menu);
    glutAddMenuEntry("Yes", 5);
    glutAddMenuEntry("No", 6);

    int shading = glutCreateMenu(shading_menu);
    glutAddMenuEntry("Flat Shading", 7);
    glutAddMenuEntry("Smooth Shading", 8);

    int lightsource = glutCreateMenu(lightsource_menu);
    glutAddMenuEntry("Point Source", 9);
    glutAddMenuEntry("Spot Light", 10);
    glutAddMenuEntry("Directional Only", 11);

    int fog = glutCreateMenu(fog_menu);
    glutAddMenuEntry("None", 11);
    glutAddMenuEntry("Linear", 12);
    glutAddMenuEntry("Exponential", 13);
    glutAddMenuEntry("Exponential Square", 14);

    int blendsh = glutCreateMenu(blendshadow_menu);
    glutAddMenuEntry("Yes", 15);
    glutAddMenuEntry("No", 16);

    int texground = glutCreateMenu(texground_menu);
    glutAddMenuEntry("Yes", 17);
    glutAddMenuEntry("No", 18);

    int texsphere = glutCreateMenu(texsphere_menu);
    glutAddMenuEntry("Yes - Contour Lines", 19);
    glutAddMenuEntry("Yes - Checkerboard", 20);
    glutAddMenuEntry("No", 21);

    int firework = glutCreateMenu(firework_menu);
    glutAddMenuEntry("Yes", 22);
    glutAddMenuEntry("No", 23);

    glutCreateMenu(menu);

    glutAddMenuEntry("Toggle Filled/Wireframe Sphere", 0);
    glutAddSubMenu("Projection Shadow", shadow);
    glutAddSubMenu("Enable Lighting", lighting);
    glutAddSubMenu("Shading", shading);
    glutAddSubMenu("Light Source", lightsource);
    glutAddSubMenu("Fog Options", fog);
    glutAddSubMenu("Blending Shadow", blendsh);
    glutAddSubMenu("Texture Mapped Ground", texground);
    glutAddSubMenu("Texture Mapped Sphere", texsphere);
    glutAddSubMenu("Firework", firework);
    glutAddMenuEntry("Default View Point", 1);
    glutAddMenuEntry("Quit", 2);
    glutAttachMenu(GLUT_LEFT_BUTTON);
}

void menu(int index) {
    switch (index) {
    case(0):
        isWireframe = !isWireframe;
        break;
    case(1):
        eye = init_eye;
        break;
    case(2):
        quit();
        break;
    }

    display();

}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
#ifdef __APPLE__ // Enable core profile of OpenGL 3.2 on macOS.
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutInitWindowSize(512, 512);
    glutCreateWindow("HW4 Sphere Rolling");

    initmenu();

#ifdef __APPLE__ // on macOS
    // Core profile requires to create a Vertex Array Object (VAO).
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#else           // on Linux or Windows, we still need glew
    /* Call glewInit() and error checking */
    int err = glewInit();
    if (GLEW_OK != err)
    {
        printf("Error: glewInit failed: %s\n", (char*)glewGetErrorString(err));
        exit(1);
    }
#endif

    // Get info of GPU and supported OpenGL version
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version supported %s\n", glGetString(GL_VERSION));

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(NULL);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);

    init();
    glutMainLoop();

    quit();

    return 0;
}

void quit() { //delete pointers
    delete[] sphere_points;    //delete pointers after usage
    delete[] sphere_colors;

    delete[] shadow_colors;
    delete[] flat_normals;
    delete[] smooth_normals;

    delete[] sphere_points_shaded;

    exit(1);
}