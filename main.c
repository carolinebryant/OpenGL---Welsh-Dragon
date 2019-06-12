#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>

// #define DO_AXES
// #define RENDER_LIGHTS

#define FLOOR_Y (-1.6f)

#define FLOOR_LIST 1
#define BOX_LIST 2

#define TEX_FLOOR 1

#define BLEND_REPLACE 1
#define BLEND_NONE 2

#define UNPADDED_FACE_SIZE 13
#define SCALE_FACTOR 12

float eye[] = {8.47, 3.2, 8.50};
float viewpt[] = {0.0, -0.40, 0.0};
float up[] = {0.0, 1.0, 0.0};
float light0_position[] = {-2.0, 3.3, 4.0, 1.0};
float light0_diffuse[] = {1.0, 1.0, 1.0, 0.35};
float light0_specular[] = {0.95, 0.98, 1.0, 0.35};
float light1_position[] = {-0.75, 1.0, -5.0, 1.0};
float light1_diffuse[] = {1.0, 1.0, 1.0, 0.09};
float light1_specular[] = {0.2, 0.63, 0.79, 0.09};
float light2_position[] = {2.0, 0.15, 4.0, 1.0};
float light2_diffuse[] = {0.3, 0.7, 0.8, 0.1};
float light2_specular[] = {0.3, 0.7, 0.8, 0.1};

float light3_position[4] = {-2.0, 3.3 + FLOOR_Y - 2.0, 4.0, 1.0};
float light4_position[4] = {-0.75, 1.0 + FLOOR_Y - 2.0, -5.0, 1.0};
float light5_position[4] = {2.0, 0.15 + FLOOR_Y - 2.0, 4.0, 1.0};


typedef struct vector_t {
    float x, y, z;
} vector;

typedef struct face_t {
    int v1, v2, v3;
} face;

typedef struct model_t {
    GLfloat *data;
    int numVertex;
} model;

float inputX = 0;
float inputY = 0;

float angle;
float timeStart = 0;

model *modelData = NULL;
unsigned int modelBuf = 1;
unsigned int shaderProgID;
int lightsEnabled[] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE};

double dot(vector *a, vector *b) {
    return a->x * b->x +
        a->y * b->y +
        a->z * b->z;
}

vector *cross(vector *a, vector *b) {
    vector *ret = (vector *) malloc(sizeof(vector));
    ret->x = (a->y * b->z) - (a->z * b->y);
    ret->y = (a->z * b->x) - (a->x * b->z);
    ret->z = (a->x * b->y) - (a->y * b->x);
    return ret;
}

void vect_divide(vector *vect, double scalar) {
    vect->x /= scalar;
    vect->y /= scalar;
    vect->z /= scalar;
}

vector *difference(vector *a, vector *b) {
    vector *ret = (vector *) malloc(sizeof(vector));
    ret->x = a->x - b->x;
    ret->y = a->y - b->y;
    ret->z = a->z - b->z;
    return ret;
}

vector *sum(vector *a, vector *b) {
    vector *ret = (vector *) malloc(sizeof(vector));
    ret->x = a->x + b->x;
    ret->y = a->y + b->y;
    ret->z = a->z + b->z;
    return ret;
}

double length(vector *v) {
    return sqrt((v->x * v->x) + (v->y * v->y) + (v->z * v->z));
}

void normalize(vector *v) {
    vect_divide(v, length(v));
}

model *readPlyFile(char const *filename) {
    model *ret = (model *)malloc(sizeof(model));
    float sumX = 0.0, sumY = 0.0, sumZ = 0.0, maxX = 0, maxY = 0, maxZ = 0;

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Invalid file %s\n", filename);
        exit(-1);
    }
    // Skip the 'ply\n'
    fseek(fp, 4, SEEK_SET);

    int endHeader = 0;
    int properties = 0;
    int numVertex = 0, numFaces = 0;
    while (!feof(fp) && !endHeader) {
        char *line = NULL;
        size_t len = 0;
        getline(&line, &len, fp);
        switch(line[0]) {
            case 'p':
                properties += 1;
                break;
            case 'e':
                if(line[1] == 'n') {
                    endHeader = 1;
                }
                else {
                    char *type = (char *)malloc(7);
                    int size = 0;
                    sscanf(line, "element %s %d", type, &size);
                    if (type[0] == 'v') {
                        numVertex = size;
                    }
                    else {
                        numFaces = size;
                    }
                }
        }
    }
    if (properties != 4) {
        fprintf(stderr, "Error, ply file does not have exactly 4 properties, it has %d properties\n", properties);
        exit(-1);
    }

    vector *vertices = (vector *)calloc(numVertex, sizeof(vector));
    fread(vertices, 1, sizeof(vector) * numVertex, fp);

    face *faces = (face *)calloc(numFaces, sizeof(face));
    void *faceBuffer = calloc(numFaces, UNPADDED_FACE_SIZE);
    fread(faceBuffer, 1, UNPADDED_FACE_SIZE * numFaces, fp);
    char *ptr = (char *)faceBuffer;
    for (int i = 0; i < numFaces; i++) {
        int v = *(unsigned char *)(ptr++);
        if (v != 3) {
            fprintf(stderr, "A face has %d vertices, can only have 3.\n", v);
            exit(-1);
        }
        faces[i].v1 = *(int *)(ptr);
        ptr += sizeof(int);
        faces[i].v2 = *(int *)(ptr);
        ptr += sizeof(int);
        faces[i].v3 = *(int *)(ptr);
        ptr += sizeof(int);
    }

    for (int i = 0; i < numVertex; i++) {
        sumX += vertices[i].x;
        sumY += vertices[i].y;
        sumZ += vertices[i].z;
        if (vertices[i].x > maxX) maxX = vertices[i].x;
        if (vertices[i].y > maxY) maxY = vertices[i].y;
        if (vertices[i].z > maxZ) maxZ = vertices[i].z;
    }
    sumX /= numVertex;
    sumY /= numVertex;
    sumZ /= numVertex;

    float scale = maxX;
    if (maxY > scale) scale = maxY;
    if (maxZ > scale) scale = maxZ;
    scale /= SCALE_FACTOR;

    for (int i = 0; i < numVertex; i++) {
        vertices[i].x -= sumX;
        vertices[i].y -= sumY;
        vertices[i].z -= sumZ;
        vect_divide(&vertices[i], scale);
    }

    GLfloat *facesAndNormals = (GLfloat *) calloc(numFaces * 6, sizeof(GLfloat) * 3);
    int ndx = 0;
    int ndx2 = numFaces * 9;
    for (int i = 0; i < numFaces; i++) {
        vector *p1, *p2, *n;
        vector *v1 = &vertices[faces[i].v1];
        vector *v2 = &vertices[faces[i].v2];
        vector *v3 = &vertices[faces[i].v3];
        p1 = difference(v2, v1);
        p2 = difference(v3, v1);
        n = cross(p1, p2);
        normalize(n);
        free(p1);
        free(p2);

        for (int j = 0; j < 3; j++) {
            facesAndNormals[ndx2++] = n->x;
            facesAndNormals[ndx2++] = n->y;
            facesAndNormals[ndx2++] = n->z;
        }
        free(n);

        facesAndNormals[ndx++] = v1->x;
        facesAndNormals[ndx++] = v1->y;
        facesAndNormals[ndx++] = v1->z;
        facesAndNormals[ndx++] = v2->x;
        facesAndNormals[ndx++] = v2->y;
        facesAndNormals[ndx++] = v2->z;
        facesAndNormals[ndx++] = v3->x;
        facesAndNormals[ndx++] = v3->y;
        facesAndNormals[ndx++] = v3->z;
    }

    ret->data = facesAndNormals;
    ret->numVertex = numFaces * 3;

    return ret;
}

char *read_shader_program(char const *filename) {
    FILE *fp;
    char *content = NULL;
    int fd, count;
    fd = open(filename,O_RDONLY);
    if (fd == 0) {
        fprintf(stderr, "Shader source `%s` not found\n", filename);
        exit(-1);
    }
    count = lseek(fd,0,SEEK_END);
    close(fd);
    content = (char *)calloc(1,(count+1));
    fp = fopen(filename,"r");
    count = fread(content,sizeof(char),count,fp);
    content[count] = '\0';
    fclose(fp);
    return content;
}

void set_light() {
    glLightfv(GL_LIGHT0,GL_POSITION,light0_position);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,light0_diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR,light0_specular);

	glLightfv(GL_LIGHT1,GL_POSITION,light1_position);
	glLightfv(GL_LIGHT1,GL_DIFFUSE,light1_diffuse);
	glLightfv(GL_LIGHT1,GL_SPECULAR,light1_specular);

	glLightfv(GL_LIGHT2,GL_POSITION,light2_position);
	glLightfv(GL_LIGHT2,GL_DIFFUSE,light2_diffuse);
	glLightfv(GL_LIGHT2,GL_SPECULAR,light2_specular);

    glLightfv(GL_LIGHT3,GL_POSITION,light3_position);
	glLightfv(GL_LIGHT3,GL_DIFFUSE,light0_diffuse);
	glLightfv(GL_LIGHT3,GL_SPECULAR,light0_specular);

    glLightfv(GL_LIGHT4,GL_POSITION,light4_position);
	glLightfv(GL_LIGHT4,GL_DIFFUSE,light1_diffuse);
	glLightfv(GL_LIGHT4,GL_SPECULAR,light1_specular);

    glLightfv(GL_LIGHT5,GL_POSITION,light5_position);
	glLightfv(GL_LIGHT5,GL_DIFFUSE,light2_diffuse);
	glLightfv(GL_LIGHT5,GL_SPECULAR,light2_specular);
}

void send_light_states(int p) {
    glUseProgram(p);
    int location = glGetUniformLocation(p,"lightsEnabled");
    glUniform1iv(location, 6, lightsEnabled);
}

void send_blend_mode(int p, GLfloat mode) {
    glUseProgram(p);
    int location = glGetUniformLocation(p,"blend_mode");
    glUniform1i(location, mode);
}

void set_material(float alpha) {
    float mat_diffuse[] = {0.6980, 0.1333, 0.0533, alpha};
    float mat_specular[] = {0.5, 0.5, 0.5, 1.0};
    float mat_shininess[] = {5.0};

    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_diffuse);
    glMaterialfv(GL_FRONT,GL_SPECULAR,mat_specular);
    glMaterialfv(GL_FRONT,GL_SHININESS,mat_shininess);
}

void load_texture(const char *filename, int textureID) {
  FILE *fptr;
  char buf[512], *parse;
  int im_size, im_width, im_height, max_color;
  unsigned char *texture_bytes;

  fptr=fopen(filename,"r");
  fgets(buf,512,fptr);
  do{
	  fgets(buf,512,fptr);
  } while(buf[0]=='#');
  parse = strtok(buf," \t");
  im_width = atoi(parse);

  parse = strtok(NULL," \n");
  im_height = atoi(parse);

  fgets(buf,512,fptr);
  parse = strtok(buf," \n");
  max_color = atoi(parse);

  im_size = im_width*im_height;
  texture_bytes = (unsigned char *)calloc(3,im_size);
  fread(texture_bytes,3,im_size,fptr);
  fclose(fptr);

  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,im_width,im_height,0,GL_RGB,
	  GL_UNSIGNED_BYTE,texture_bytes);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  cfree(texture_bytes);
}

void view_volume() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0,1.0,1.0,27.5);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(eye[0],eye[1],eye[2],viewpt[0],viewpt[1],viewpt[2],up[0],up[1],up[2]);
}

void renderScene() {
	glClearColor(0.022,0.020,0.018,1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Moves the camera around the scene according to the keys
	float timeDelta = glutGet(GLUT_ELAPSED_TIME) - timeStart;
    timeStart += timeDelta;

	float forwardX = viewpt[0] - eye[0];
	float forwardZ = viewpt[2] - eye[2];
	float forwardXzDist = sqrt(forwardX*forwardX + forwardZ*forwardZ);

	float angle = atan2(forwardX, forwardZ);
	angle += inputX * M_PI/500 * timeDelta;
	eye[0] = viewpt[0] - sin(angle)*forwardXzDist;
	eye[2] = viewpt[2] - cos(angle)*forwardXzDist;

	eye[1] += up[1] * -inputY * 0.01 * timeDelta;
	viewpt[1] += up[1] * -inputY * 0.01 * timeDelta;

	eye[1] = fmax(FLOOR_Y + 1, fmin(eye[1], 10));
	viewpt[1] = fmax(FLOOR_Y - 2.5, fmin(viewpt[1], 6.5));

	glLoadIdentity();
	gluLookAt(eye[0],eye[1],eye[2],viewpt[0],viewpt[1],viewpt[2],up[0],up[1],up[2]);
    set_light();

	//Render the ground dragon
    glUseProgram(shaderProgID);
    lightsEnabled[0] = lightsEnabled[1] = lightsEnabled[2] = GL_FALSE;
    lightsEnabled[3] = lightsEnabled[4] = lightsEnabled[5] = GL_TRUE;
    send_light_states(shaderProgID);
    set_material(0.0);
    send_blend_mode(shaderProgID, BLEND_NONE);
    glPushMatrix();
        glTranslated(0, FLOOR_Y * 2 - .015, 0);
        glScaled(1, -1, 1);
        glRotatef(180, 0, 1, 0);
        glRotatef(-90, 1, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, modelData->numVertex);
    glPopMatrix();

	//Render the floor and sky dragon
    lightsEnabled[0] = lightsEnabled[1] = lightsEnabled[2] = GL_TRUE;
    lightsEnabled[3] = lightsEnabled[4] = lightsEnabled[5] = GL_FALSE;
    send_light_states(shaderProgID);
    glEnable(GL_BLEND);
        set_material(0.22); // previous: 0.14
        glBlendFunc(GL_ONE, GL_SRC_ALPHA);
        glUseProgram(shaderProgID);
        send_blend_mode(shaderProgID, BLEND_REPLACE);
        glCallList(FLOOR_LIST);
    glDisable(GL_BLEND);

    set_material(0.0);
    send_blend_mode(shaderProgID, BLEND_NONE);
    glPushMatrix();
        glRotatef(180, 0, 1, 0);
        glRotatef(-90, 1, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, modelData->numVertex);
    glPopMatrix();


    #ifdef DO_AXES
    glUseProgram(0);
    glLineWidth(2.0);
    glBegin(GL_LINES);
        glColor3f(1.0, 0, 0);
        glVertex3f(0, 0, 0);
        glVertex3f(100, 0, 0);
    glEnd();
    glBegin(GL_LINES);
        glColor3f(0, 1, 0);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 100, 0);
    glEnd();
    glBegin(GL_LINES);
        glColor3f(0, 0, 1);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, 100);
    glEnd();
    #endif

    #ifdef RENDER_LIGHTS
    glUseProgram(0);
	// Light 0
    glPushMatrix();
        glColor3f(light0_diffuse[0], light0_diffuse[1], light0_diffuse[2]);
        glTranslatef(light0_position[0], light0_position[1], light0_position[2]);
        glutSolidSphere(0.5, 10, 10);
    glPopMatrix();
	// Light 1
    glPushMatrix();
	    glColor3f(light1_diffuse[0],light1_diffuse[1], light1_diffuse[2]);
	    glTranslated(light1_position[0], light1_position[1], light1_position[2]);
	    glutSolidSphere(0.5, 10, 10);
    glPopMatrix();
	// Light 2
    glPushMatrix();
	    glColor3f(light2_diffuse[0], light2_diffuse[1], light2_diffuse[2]);
	    glTranslatef(light2_position[0], light2_position[1], light2_position[2]);
	    glutSolidSphere(0.5, 10, 10);
    glPopMatrix();
	// Light 3
    glPushMatrix();
        glColor3f(light0_diffuse[0], light0_diffuse[1], light0_diffuse[2]);
        glTranslatef(light3_position[0], light3_position[1], light3_position[2]);
        glutSolidSphere(0.5, 10, 10);
    glPopMatrix();
	// Light 4
    glPushMatrix();
        glColor3f(light1_diffuse[0],light1_diffuse[1], light1_diffuse[2]);
        glTranslated(light4_position[0], light4_position[1], light4_position[2]);
        glutSolidSphere(0.5, 10, 10);
    glPopMatrix();
    // Light 5
    glPushMatrix();
        glColor3f(light2_diffuse[0], light2_diffuse[1], light2_diffuse[2]);
        glTranslatef(light5_position[0], light5_position[1], light5_position[2]);
        glutSolidSphere(0.5, 10, 10);
    glPopMatrix();
    #endif

    glutSwapBuffers();
}

int compileShader(GLuint shaderID, char const *shaderName) {
    int result;
    glCompileShader(shaderID);
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
    if(result == GL_FALSE) {
        GLint logSize = 0;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logSize);
        char *compileLog = (char *)malloc(logSize);
        GLsizei written = 0;
        glGetShaderInfoLog(shaderID, logSize, &written, compileLog);
        fprintf(stderr, "Error compiling %s shader:\n%s\n", shaderName, compileLog);
        glDeleteShader(shaderID);
        return GL_FALSE;
    }
    return GL_TRUE;
}

unsigned int set_shaders() {
    char *vs, *fs;
    GLuint v, f, p;

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);
    vs = read_shader_program("main.vert");
    fs = read_shader_program("main.frag");
    glShaderSource(v,1,(const char **)&vs,NULL);
    glShaderSource(f,1,(const char **)&fs,NULL);
    free(vs);
    free(fs);
    if (compileShader(v, "vertex") == GL_FALSE) {
        exit(-1);
    }
    if (compileShader(f, "fragment") == GL_FALSE) {
        exit(-1);
    }
    p = glCreateProgram();
    glAttachShader(p,f);
    glAttachShader(p,v);
    glLinkProgram(p);
    glUseProgram(p);
    return p;
}

void setup_scene_lists() {
    glNewList(FLOOR_LIST,GL_COMPILE);
    	glEnable(GL_TEXTURE_2D);
    	glBindTextureEXT(GL_TEXTURE_2D, TEX_FLOOR);
    	glNormal3f(0.0, 1.0, 0.0);
    	glColor4f(0.4, 0.4, 0.4, 0.6);
    	glBegin(GL_QUADS);
    		glTexCoord2f(0.0, 0.0);
    		glVertex3f(-20.0, FLOOR_Y - 0.01, 20.0);
    		glTexCoord2f(4.0, 0.0);
    		glVertex3f(-20.0, FLOOR_Y - 0.01, -20.0);
    		glTexCoord2f(4.0, 4.0);
    		glVertex3f(20.0, FLOOR_Y - 0.01, -20.0);
    		glTexCoord2f(0.0, 4.0);
    		glVertex3f(20.0, FLOOR_Y - 0.01, 20.0);
    	glEnd();
    	glDisable(GL_TEXTURE_2D);
    glEndList();
}

void set_uniform_textures(int p) {
  int location;
  location = glGetUniformLocation(p,"mytexture");
  glUniform1i(location, 0);
}

void HandleKeyDown(unsigned char key, int x, int y) {
    switch(key)
	{
		case 'w':
			inputY = -0.25;
			break;
        case 'W':
            inputY = -1.0;
            break;
		case 's':
			inputY = 0.25;
			break;
        case 'S':
            inputY = 1.0;
            break;
		case 'a':
			inputX = -0.25;
			break;
        case 'A':
            inputX = -1.0;
            break;
		case 'd':
			inputX = 0.25;
			break;
        case 'D':
            inputX = 1.0;
            break;
        case 'p':
            printf("Current camera position: %lf, %lf, %lf\n", eye[0], eye[1], eye[2]);
            printf("Current view point: %lf, %lf, %lf\n", viewpt[0], viewpt[1], viewpt[2]);
            break;
		case 'q':
            glDeleteBuffers(1, &modelBuf);
			exit(1);
	}
}

void HandleKeyUp(unsigned char key, int x, int y) {
    switch(key)
	{
		case 'w':
        case 'W':
			inputY = (inputY < 0) ? 0 : inputY;
			break;
		case 's':
        case 'S':
			inputY = (inputY > 0) ? 0 : inputY;
			break;
		case 'a':
        case 'A':
			inputX = (inputX < 0) ? 0 : inputX;
			break;
		case 'd':
        case 'D':
			inputX = (inputX > 0) ? 0 : inputX;
			break;
		case 'q':
            glDeleteBuffers(1, &modelBuf);
			exit(1);
	}
}

void send_model(model *model) {
    glBindBuffer(GL_ARRAY_BUFFER, modelBuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vector) * model->numVertex * 2, model->data, GL_STATIC_DRAW);
    glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), 0);
    glNormalPointer(GL_FLOAT, 3 * sizeof(GLfloat), (void *)(3 * model->numVertex * sizeof(GLfloat)));
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
}

int main(int argc, char **argv) {
    modelData = readPlyFile("welsh-dragon.ply");
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(768, 768);
    glutCreateWindow("Welsh Dragon");
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE_ARB);
    send_model(modelData);
    load_texture("WoodFloor-raw.ppm", TEX_FLOOR);
    view_volume();
    set_light();
    set_material(0.0);
    setup_scene_lists();
    shaderProgID = set_shaders();
    send_light_states(shaderProgID);
    set_uniform_textures(shaderProgID);
    glutDisplayFunc(renderScene);
	glutIdleFunc(renderScene);
    glutKeyboardFunc(HandleKeyDown);
	glutKeyboardUpFunc(HandleKeyUp);
    glutMainLoop();
    return 0;
}
