// tag::C++11check[]
#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if __cplusplus < 201103L
	#pragma message("WARNING: the compiler may not be C++11 compliant. __cplusplus version is : " STRING(__cplusplus))
#endif
// end::C++11check[]

// tag::includes[]
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

#include <GL/glew.h>
#include <SDL.h>

#include <SOIL.h>

#include <chrono> // time related stuff
// end::includes[]

// tag::using[]
// see https://isocpp.org/wiki/faq/Coding-standards#using-namespace-std
// don't use the whole namespace, either use the specific ones you want, or just type std::
using std::cout;
using std::endl;
using std::max;
using namespace std::chrono;
// end::using[]


// tag::globalVariables[]
std::string exeName;
SDL_Window *win; //pointer to the SDL_Window
SDL_GLContext context; //the SDL_GLContext
int frameCount = 0;
std::string frameLine = "";
// end::globalVariables[]

// tag::vertexShader[]
//string holding the **source** of our vertex shader, to save loading from a file
const std::string strVertexShader = R"(
	#version 330
	in vec2 position;
	in vec2 texcoord;
	uniform vec2 offset;
	uniform float rotation = 0.0;
	out vec2 Texcoord;
	void main()
	{
		// rotation
		vec2 newPosition = vec2((position.x * cos(rotation)) - (position.y * sin(rotation)), (position.y * cos(rotation)) + (position.x * sin(rotation)));		

			vec2 trianglePos = newPosition + offset;
		gl_Position = vec4(trianglePos, 0.0, 1.0);
		Texcoord = texcoord;
	}
)";
// end::vertexShader[]

// tag::fragmentShader[]
//string holding the **source** of our fragment shader, to save loading from a file
const std::string strFragmentShader = R"(
	#version 330
	in vec2 Texcoord;
	out vec4 outputColor;
	uniform vec3 color;
	uniform sampler2D tex;
	void main()
	{
		//outputColor = vec4(color, 1.0f);
		outputColor = texture(tex, Texcoord);
	}
)";
// end::fragmentShader[]

// tag::ourVariables[]
//our variables
bool done = false;
high_resolution_clock::time_point timePrev;

//the data about our geometry
const GLfloat vertexData[] = {
//      X      Y     Texture Points
// Paddle
	0.000f,	0.200f,  0.0f, 0.0f, // 1st triangle
	0.000f,	-0.200f, 0.0f, 1.0f,
	0.050f,	-0.200f, 1.0f, 1.0f,
	0.000f, 0.200f,  0.0f, 0.0f, // 2nd triangle
	0.050f, 0.200f,  1.0f, 0.0f,
	0.050f, -0.200f, 1.0f, 1.0f
};

const GLfloat ballVertexData[] = {
	// Ball
	//  X       Y     Texture Points
	-0.03f, 0.03f,  0.0f, 0.0f,  // 1st triangle
	-0.03f, -0.03f, 0.0f, 1.0f,
	0.03f, 0.03f,   1.0f, 0.0f,
	0.03f, 0.03f,   1.0f, 0.0f, // 2nd triangle
	-0.03f, -0.03f, 0.0f, 1.0f,
	0.03f, -0.03f,  1.0f, 1.0f
};

const GLfloat scoreVertexData[] = {
	// Score
	//  X       Y      Texture Points
	-0.03f, 0.05f,  0.0f, 0.0f, // 1st triangle
	-0.03f, -0.05f, 0.0f, 1.0f,
	0.03f, 0.05f,   1.0f, 0.0f,
	0.03f, 0.05f,   1.0f, 0.0f, // 2nd triangle
	-0.03f, -0.05f, 0.0f, 1.0f,
	0.03f, -0.05f,  1.0f, 1.0f
};

const GLfloat backgroundVertexData[] = {
	// Background
	//  X       Y   Texture Points
	-0.97f, 0.95f,  0.0f, 0.0f, // 1st triangle
	-0.97f, -0.95f, 0.0f, 1.0f,
	0.97f, 0.95f,   1.0f, 0.0f,
	0.97f, 0.95f,   1.0f, 0.0f, // 2nd triangle
	-0.97f, -0.95f, 0.0f, 1.0f,
	0.97f, -0.95f,  1.0f, 1.0f
};

// offset
GLfloat offsetLeftBat[] = { -0.95, 0.0 };
GLfloat offsetRightBat[] = { 0.9, 0.0 };
GLfloat offsetUpdateSpeed = 1.3;

GLfloat offsetBall[] = { 0.0, 0.0 };
GLfloat ballSpeed[] = { 0.4, 0.8 }; // (x direction, y direction)

GLfloat scoreOffset[] = { 0.0, -0.95 };
const GLfloat SCORE_X_CHANGE = 0.03; // amount of change in the x axis with each new score
const GLuint SCORE_LIMIT = 5;
bool gameOver = false;

GLfloat angle = 0.0f; // angle of rotation

const GLfloat paddleDimensions[] = { 0.05, 0.2 }; // width and height needed for collision detection (width is full width, height is half (because of the centre of the shape))
const GLfloat ballDimensions[] = { 0.03, 0.03 }; // half of width and height of ball

// directions of paddles - only needs up and down
GLfloat leftPaddleDirection = 0.0;
GLfloat rightPaddleDirection = 0.0;

// colliding bools
bool collidingWithSides[] = { false, false }; // if ball is colliding with left/right & top/bottom
bool collidingWithPaddle = false; // if ball is colliding with paddle

// score
int score[] = { 0, 0 }; // (left score, right score)

//the color we'll pass to the GLSL
GLfloat color[] = { 1.0f, 1.0f, 1.0f }; //using different values from CPU and static GLSL examples, to make it clear this is working

//our GL and GLSL variables

GLuint theProgram; //GLuint that we'll fill in to refer to the GLSL program (only have 1 at this point)
GLint positionLocation; //GLuint that we'll fill in with the location of the `position` attribute in the GLSL
GLint colorLocation; //GLuint that we'll fill in with the location of the `color` variable in the GLSL
GLint offsetLocation;
GLint textureLocation;
GLint textureImageLocation;
GLint rotationPosition;

GLuint vertexDataBufferObject;
GLuint vertexArrayObject;

GLuint ballDBO;
GLuint ballVAO;

GLuint scoreDBO;
GLuint scoreVAO;

GLuint backgroundDBO;
GLuint backgroundVAO;

GLuint textures[4];

// end::ourVariables[]


// end Global Variables
/////////////////////////

GLfloat clamp(GLfloat value, GLfloat min, GLfloat max)
{
	return std::max(std::min(value, max), min);
}

bool checkBounds(GLfloat value, GLfloat min, GLfloat max)
{
	if (value >= max || value <= min) return true;
	else return false;
}

GLfloat getRandomNumber(GLfloat min, GLfloat max)
{
	float randomNum = ((float)rand()) / (float)RAND_MAX;
	float range = max - min;
	float r = randomNum * range;
	return min + r;
}

// tag::initialise[]
void initialise()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		exit(1);
	}
	cout << "SDL initialised OK!\n";
}
// end::initialise[]

// tag::createWindow[]
void createWindow()
{
	//get executable name, and use as window title
	int beginIdxWindows = exeName.rfind("\\"); //find last occurrence of a backslash
	int beginIdxLinux = exeName.rfind("/"); //find last occurrence of a forward slash
	int beginIdx = max(beginIdxWindows, beginIdxLinux);
	std::string exeNameEnd = exeName.substr(beginIdx + 1);
	const char *exeNameCStr = exeNameEnd.c_str();

	//create window
	win = SDL_CreateWindow(exeNameCStr, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 600, SDL_WINDOW_OPENGL); //same height and width makes the window square ...

	//error handling
	if (win == nullptr)
	{
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "SDL CreatedWindow OK!\n";
}
// end::createWindow[]

// tag::setGLAttributes[]
void setGLAttributes()
{
	int major = 3;
	int minor = 3;
	cout << "Built for OpenGL Version " << major << "." << minor << endl; //ahttps://en.wikipedia.org/wiki/OpenGL_Shading_Language#Versions
	// set the opengl context version
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //core profile
	cout << "Set OpenGL context to version " << major << "." << minor << " OK!\n";
}
// tag::setGLAttributes[]

// tag::createContext[]
void createContext()
{
	setGLAttributes();

	context = SDL_GL_CreateContext(win);
	if (context == nullptr){
		SDL_DestroyWindow(win);
		std::cout << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}
	cout << "Created OpenGL context OK!\n";
}
// end::createContext[]

// tag::initGlew[]
void initGlew()
{
	GLenum rev;
	glewExperimental = GL_TRUE; //GLEW isn't perfect - see https://www.opengl.org/wiki/OpenGL_Loading_Library#GLEW
	rev = glewInit();
	if (GLEW_OK != rev){
		std::cout << "GLEW Error: " << glewGetErrorString(rev) << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLEW Init OK!\n";
	}
}
// end::initGlew[]

// tag::createShader[]
GLuint createShader(GLenum eShaderType, const std::string &strShaderFile)
{
	GLuint shader = glCreateShader(eShaderType);
	//error check
	const char *strFileData = strShaderFile.c_str();
	glShaderSource(shader, 1, &strFileData, NULL);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char *strShaderType = NULL;
		switch (eShaderType)
		{
		case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
		case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
		case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}

	return shader;
}
// end::createShader[]

// tag::createProgram[]
GLuint createProgram(const std::vector<GLuint> &shaderList)
{
	GLuint program = glCreateProgram();

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glAttachShader(program, shaderList[iLoop]);

	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;
	}

	for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glDetachShader(program, shaderList[iLoop]);

	return program;
}
// end::createProgram[]

// tag::initializeProgram[]
void initializeProgram()
{
	std::vector<GLuint> shaderList;

	shaderList.push_back(createShader(GL_VERTEX_SHADER, strVertexShader));
	shaderList.push_back(createShader(GL_FRAGMENT_SHADER, strFragmentShader));

	theProgram = createProgram(shaderList);
	if (theProgram == 0)
	{
		cout << "GLSL program creation error." << std::endl;
		SDL_Quit();
		exit(1);
	}
	else {
		cout << "GLSL program creation OK! GLUint is: " << theProgram << std::endl;
	}

	positionLocation = glGetAttribLocation(theProgram, "position");
	textureLocation = glGetAttribLocation(theProgram, "texcoord");
	colorLocation = glGetUniformLocation(theProgram, "color");
	offsetLocation = glGetUniformLocation(theProgram, "offset");
	textureImageLocation = glGetUniformLocation(theProgram, "tex");
	rotationPosition = glGetUniformLocation(theProgram, "rotation");

	//clean up shaders (we don't need them anymore as they are no in theProgram
	for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
}
// end::initializeProgram[]

// tag::initializeVertexArrayObject[]
//setup a GL object (a VertexArrayObject) that stores how to access data and from where
void initializeVertexArrayObject()
{
	glGenVertexArrays(1, &vertexArrayObject); //create a Vertex Array Object
	cout << "Vertex Array Object created OK! GLUint is: " << vertexArrayObject << std::endl;

	glGenVertexArrays(1, &ballVAO); //create a Vertex Array Object
	cout << "Ball Vertex Array Object created OK! GLUint is: " << ballVAO << std::endl;

	glGenVertexArrays(1, &scoreVAO); //create a Vertex Array Object
	cout << "Score Vertex Array Object created OK! GLUint is: " << scoreVAO << std::endl;

	glGenVertexArrays(1, &backgroundVAO); //create a Vertex Array Object
	cout << "Background Vertex Array Object created OK! GLUint is: " << backgroundVAO << std::endl;


	glBindVertexArray(vertexArrayObject); //make the just created vertexArrayObject the active one

		glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObject); //bind vertexDataBufferObject

		glEnableVertexAttribArray(positionLocation); //enable attribute at index positionLocation

		glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (void*)0); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation

		glEnableVertexAttribArray(textureLocation); //enable attribute at index textureLocation

		glVertexAttribPointer(textureLocation, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (void*)(2*sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index textureLocation

	glBindVertexArray(ballVAO); //make the just created vertexArrayObject the active one

		glBindBuffer(GL_ARRAY_BUFFER, ballDBO); //bind vertexDataBufferObject

		glEnableVertexAttribArray(positionLocation); //enable attribute at index positionLocation

		glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation

		glEnableVertexAttribArray(textureLocation); //enable attribute at index textureLocation

		glVertexAttribPointer(textureLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index textureLocation

	glBindVertexArray(scoreVAO); //make the just created vertexArrayObject the active one

		glBindBuffer(GL_ARRAY_BUFFER, scoreDBO); //bind vertexDataBufferObject

		glEnableVertexAttribArray(positionLocation); //enable attribute at index positionLocation

		glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation

		glEnableVertexAttribArray(textureLocation); //enable attribute at index textureLocation

		glVertexAttribPointer(textureLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index textureLocation

	glBindVertexArray(backgroundVAO); //make the just created vertexArrayObject the active one

		glBindBuffer(GL_ARRAY_BUFFER, backgroundDBO); //bind vertexDataBufferObject

		glEnableVertexAttribArray(positionLocation); //enable attribute at index positionLocation

		glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0); //specify that position data contains four floats per vertex, and goes into attribute index positionLocation

		glEnableVertexAttribArray(textureLocation); //enable attribute at index textureLocation

		glVertexAttribPointer(textureLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat))); //specify that position data contains four floats per vertex, and goes into attribute index textureLocation

	glBindVertexArray(0); //unbind the vertexArrayObject so we can't change it

	//cleanup
	glDisableVertexAttribArray(positionLocation); //disable vertex attribute at index positionLocation
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind array buffer

}
// end::initializeVertexArrayObject[]

// tag::initializeVertexBuffer[]
void initializeVertexBuffer()
{
	glGenBuffers(1, &vertexDataBufferObject);

	glBindBuffer(GL_ARRAY_BUFFER, vertexDataBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "vertexDataBufferObject created OK! GLUint is: " << vertexDataBufferObject << std::endl;

	glGenBuffers(1, &ballDBO);

	glBindBuffer(GL_ARRAY_BUFFER, ballDBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ballVertexData), ballVertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "ball vertexDataBufferObject created OK! GLUint is: " << ballDBO << std::endl;

	glGenBuffers(1, &scoreDBO);

	glBindBuffer(GL_ARRAY_BUFFER, scoreDBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(scoreVertexData), scoreVertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "score vertexDataBufferObject created OK! GLUint is: " << scoreDBO << std::endl;

	glGenBuffers(1, &backgroundDBO);

	glBindBuffer(GL_ARRAY_BUFFER, backgroundDBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundVertexData), backgroundVertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cout << "background vertexDataBufferObject created OK! GLUint is: " << backgroundDBO << std::endl;

	initializeVertexArrayObject();
}
// end::initializeVertexBuffer[]

void initializeTextures()
{
	// allows the transparency to work
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenTextures(4, textures);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	int width, height;
	unsigned char* image = SOIL_load_image("paddle.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	image = SOIL_load_image("ball.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	image = SOIL_load_image("background.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures[3]);
	image = SOIL_load_image("score.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image); // uses RGBA (transparency)
	SOIL_free_image_data(image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
}

// tag::loadAssets[]
void loadAssets()
{
	initializeProgram(); //create GLSL Shaders, link into a GLSL program, and get IDs of attributes and variables

	initializeVertexBuffer(); //load data into a vertex buffer

	initializeTextures();

	cout << "Loaded Assets OK!\n";

	timePrev = high_resolution_clock::now(); // set the last time
}
// end::loadAssets[]

// tag::handleInput[]
void handleInput()
{
	//Event-based input handling
	//The underlying OS is event-based, so **each** key-up or key-down (for example)
	//generates an event.
	//  - https://wiki.libsdl.org/SDL_PollEvent
	//In some scenarios we want to catch **ALL** the events, not just to present state
	//  - for instance, if taking keyboard input the user might key-down two keys during a frame
	//    - we want to catch based, and know the order
	//  - or the user might key-down and key-up the same within a frame, and we still want something to happen (e.g. jump)
	//  - the alternative is to Poll the current state with SDL_GetKeyboardState

	SDL_Event event; //somewhere to store an event

	//NOTE: there may be multiple events per frame
	while (SDL_PollEvent(&event)) //loop until SDL_PollEvent returns 0 (meaning no more events)
	{
		switch (event.type)
		{
		case SDL_QUIT:
			done = true; //set donecreate remote branch flag if SDL wants to quit (i.e. if the OS has triggered a close event,
							//  - such as window close, or SIGINT
			break;

			//keydown handling - we should to the opposite on key-up for direction controls (generally)
		case SDL_KEYDOWN:
			//Keydown can fire repeatable if key-repeat is on.
			//  - the repeat flag is set on the keyboard event, if this is a repeat event
			//  - in our case, we're going to ignore repeat events
			//  - https://wiki.libsdl.org/SDL_KeyboardEvent
			if (!event.key.repeat)
				switch (event.key.keysym.sym)
				{
					//hit escape to exit
					case SDLK_ESCAPE: done = true;
						break;
					case SDLK_w:
						leftPaddleDirection += 1.0;
						break;
					case SDLK_s:
						leftPaddleDirection -= 1.0;
						break;
					case SDLK_UP:
						rightPaddleDirection += 1.0;
						break;
					case SDLK_DOWN:
						rightPaddleDirection -= 1.0;
						break;
				}
			break;
		case SDL_KEYUP:
			if (!event.key.repeat)
				switch (event.key.keysym.sym)
				{
				case SDLK_w:
					leftPaddleDirection -= 1.0;
					break;
				case SDLK_s:
					leftPaddleDirection += 1.0;
					break;
				case SDLK_UP:
					rightPaddleDirection -= 1.0;
					break;
				case SDLK_DOWN:
					rightPaddleDirection += 1.0;
					break;
				}
			break;
		}
	}
}
// end::handleInput[]

// Get Delta Function - used to make sure the animation is smooth on all computers
GLdouble getDelta()
{
	auto timeCurrent = high_resolution_clock::now();

	auto timeDiff = duration_cast<nanoseconds>(timeCurrent - timePrev);

	GLdouble delta = timeDiff.count();

	delta /= 1000000000;

	timePrev = timeCurrent;

	return delta;
}

// Check if the ball is colliding with the edges of the screen
bool checkBallBounds(int index)
{
	if (checkBounds(offsetBall[index], -0.95+ballDimensions[index], 0.95- ballDimensions[index])) // check if the ball intersects with the boundary of the screen
	{
		if (!collidingWithSides[index]) // stops the ball getting stuck in the wall
		{
			ballSpeed[index] = -ballSpeed[index]; // if so reverse the ball direction
			collidingWithSides[index] = true;
		}
		return true;
	}
	else
	{
		collidingWithSides[index] = false;
		return false;
	}
}

void checkBallPaddleCollision(bool leftPaddle)
{
	if (leftPaddle)
	{
		if ((offsetLeftBat[0] + paddleDimensions[0]) >= (offsetBall[0] - ballDimensions[0]) && // if the left part of the ball is intersecting with the bat x position
			offsetLeftBat[0] <= offsetBall[0] - ballDimensions[0] &&
			(offsetLeftBat[1] + paddleDimensions[1] + (2*ballDimensions[1])) >= (offsetBall[1] + ballDimensions[1]) && // if the left part of the ball is colliding with the bat y postion
			(offsetLeftBat[1] - paddleDimensions[1] - (2 * ballDimensions[1])) <= offsetBall[1] - ballDimensions[1])
		{
			if (!collidingWithPaddle) { // stops the ball getting stuck in the paddle
				ballSpeed[0] = -ballSpeed[0]; // reverse the ball x direction
				collidingWithPaddle = true;
			}
		}
		else
		{
			collidingWithPaddle = false;
		}
	}
	else
	{
		if (offsetRightBat[0] <= (offsetBall[0] + ballDimensions[0]) &&
			(offsetRightBat[0] + paddleDimensions[0]) >= offsetBall[0] + ballDimensions[0] &&
			(offsetRightBat[1] + paddleDimensions[1] + (2 * ballDimensions[1])) >= (offsetBall[1] + ballDimensions[1]) &&
			(offsetRightBat[1] - paddleDimensions[1] - (2 * ballDimensions[1])) <= offsetBall[1] - ballDimensions[1])
		{
			if (!collidingWithPaddle) {
				ballSpeed[0] = -ballSpeed[0];
				collidingWithPaddle = true;
			}
		}
		else 
		{
			collidingWithPaddle = false;
		}
	}
}

void resetBall()
{
	// reset the rotation
	angle = 0.0f;

	// add score to correct opposing player
	if (offsetBall[0] > 0.0)
		score[0]++;
	else
		score[1]++;

	// reset the ball location
	offsetBall[0] = 0.0;
	offsetBall[1] = 0.0;

	// set a random speed for the ball
	if (ballSpeed[0] < 0.0) // make sure that the ball goes the way
		ballSpeed[0] = getRandomNumber(-1.5f, -0.5f);
	else
		ballSpeed[0] = getRandomNumber(0.5f, 1.5f);

	do { // keep choosing a random speed until ball speed is not -0.3 < x < 0.3 (to make game interesting)
		ballSpeed[1] = getRandomNumber(-1.5f, 1.5f);
	} while (ballSpeed[1] < 0.3 && ballSpeed[1] > -0.3);
}

// tag::updateSimulation[]
void updateSimulation(double simLength = 0.02) //update simulation with an amount of time to simulate for (in seconds)
{
	//WARNING - we should calculate an appropriate amount of time to simulate - not always use a constant amount of time
			// see, for example, http://headerphile.blogspot.co.uk/2014/07/part-9-no-more-delays.html

	GLdouble delta = getDelta();

	if (!gameOver && (score[0] >= SCORE_LIMIT || score[1] >= SCORE_LIMIT)) // if the score reaches the score limit - stop the game
	{
		gameOver = true;
		offsetUpdateSpeed = 0.0; // stop the paddles moving
		ballSpeed[0] = 0.0; // stop the ball moving
		ballSpeed[1] = 0.0;
	}

	offsetLeftBat[1] += (leftPaddleDirection * offsetUpdateSpeed * delta); // update the left bat location
	offsetRightBat[1] += (rightPaddleDirection * offsetUpdateSpeed * delta); // update the right bat location
	
	offsetLeftBat[1] = clamp(offsetLeftBat[1], -0.95+paddleDimensions[1], 0.95-paddleDimensions[1]);
	offsetRightBat[1] = clamp(offsetRightBat[1], -0.95 + paddleDimensions[1], 0.95 - paddleDimensions[1]);

	// update ball position
	offsetBall[0] += (ballSpeed[0] * delta);
	offsetBall[1] += (ballSpeed[1] * delta);

	// check ball boundary
	if (checkBallBounds(0)) { // left & right of screen
		resetBall();
	}
	checkBallBounds(1); // top & bottom

	// check for collision with paddles
	if (offsetBall[0] > 0.0) // only check between one of the paddles at a time
		checkBallPaddleCollision(false);
	else
		checkBallPaddleCollision(true);

	// update the rotation
	angle += delta;

	if (angle > 6.28319)
		angle = 0.0f;
}
// end::updateSimulation[]

// tag::preRender[]
void preRender()
{
	glViewport(0, 0, 1000, 600); //set viewpoint
	glClearColor(0.5f, 0.0f, 0.0f, 1.0f); //set clear colour
	glClear(GL_COLOR_BUFFER_BIT); //clear the window (technical the scissor box bounds)
}
// end::preRender[]

void renderScore()
{
	// LEFT SCORE
	scoreOffset[0] = -0.955;
	glUniform2f(offsetLocation, scoreOffset[0], scoreOffset[1]);

	for (int i = 0; i < score[0]; i++)
	{
		glDrawArrays(GL_TRIANGLES, 0, 6);
		scoreOffset[0] += SCORE_X_CHANGE;
		glUniform2f(offsetLocation, scoreOffset[0], scoreOffset[1]);
	}

	// RIGHT SCORE
	scoreOffset[0] = 0.955;
	glUniform2f(offsetLocation, scoreOffset[0], scoreOffset[1]);

	for (int i = 0; i < score[1]; i++)
	{
		glDrawArrays(GL_TRIANGLES, 0, 6);
		scoreOffset[0] -= SCORE_X_CHANGE;
		glUniform2f(offsetLocation, scoreOffset[0], scoreOffset[1]);
	}
}

// tag::render[]
void render()
{
	glUseProgram(theProgram); //installs the program object specified by program as part of current rendering state

	//load data to GLSL that **may** have changed
	glUniform3f(colorLocation, color[0], color[1], color[2]);
		//alternatively, use glUnivform2fv
		//glUniform2fv(colorLocation, 1, color); //Note: the count is 1, because we are setting a single uniform vec2 - https://www.opengl.org/wiki/GLSL_:_common_mistakes#How_to_use_glUniform

	glActiveTexture(GL_TEXTURE0);

	// ------------------------------ BACKGROUND

	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glUniform1f(textureImageLocation, 2);

	glUniform2f(offsetLocation, 0.0, 0.0); // reset the offset to zero

	glBindVertexArray(backgroundVAO);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	// ------------------------------ BATS

	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glUniform1f(textureImageLocation, 0);

	glUniform2f(offsetLocation, offsetLeftBat[0], offsetLeftBat[1]); // update the position of the bat

	glBindVertexArray(vertexArrayObject);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glUniform2f(offsetLocation, offsetRightBat[0], offsetRightBat[1]); // update the position of the bat

	glDrawArrays(GL_TRIANGLES, 0, 6);

	// ------------------------------ BALL

	glUniform1f(rotationPosition, angle);

	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glUniform1f(textureImageLocation, 1);

	glBindVertexArray(ballVAO);

	glUniform2f(offsetLocation, offsetBall[0], offsetBall[1]); // update the position of the ball

	glDrawArrays(GL_TRIANGLES, 0, 6);

	// ------------------------------ SCORE

	glUniform1f(rotationPosition, 0.0); // reset rotation

	glBindTexture(GL_TEXTURE_2D, textures[3]);
	glUniform1f(textureImageLocation, 3);

	glBindVertexArray(scoreVAO);

	glUniform3f(colorLocation, 1.0, 0.0, 0.0);

	renderScore();

	glBindVertexArray(0);

	glUseProgram(0); //clean up

}
// end::render[]

// tag::postRender[]
void postRender()
{
	SDL_GL_SwapWindow(win);; //present the frame buffer to the display (swapBuffers)
	frameLine += "Frame: " + std::to_string(frameCount++);
	frameLine += " LEFT: " + std::to_string(score[0]) + " Right: " + std::to_string(score[1]);
	cout << "\r" << frameLine << std::flush;
	frameLine = "";
}
// end::postRender[]

// tag::cleanUp[]
void cleanUp()
{
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);
	cout << "Cleaning up OK!\n";
}
// end::cleanUp[]

// tag::main[]
int main( int argc, char* args[] )
{
	exeName = args[0];
	//setup
	//- do just once
	initialise();
	createWindow();

	createContext();

	initGlew();

	glViewport(0,0,1000,600); //should check what the actual window res is?

	SDL_GL_SwapWindow(win); //force a swap, to make the trace clearer

	//do stuff that only needs to happen once
	//- create shaders
	//- load vertex data
	loadAssets();

	while (!done) //loop until done flag is set)
	{
		handleInput(); // this should ONLY SET VARIABLES

		updateSimulation(); // this should ONLY SET VARIABLES according to simulation

		preRender();

		render(); // this should render the world state according to VARIABLES -

		postRender();

	}

	//cleanup and exit
	cleanUp();
	SDL_Quit();

	return 0;
}
// end::main[]
