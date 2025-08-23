/*
	graphic.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL_video.h>

struct sbox //Bounding box on screen
{
	int x,y,w,h; //Co-ordinates, width and height
};

class graphic //A sprite object
{
	public:
	static const int ISIZE=512; //Graphics index size
	static const int MAX_DRAW_OPERATIONS=1024; //Maximum number of draw operations to track
	enum {LOGO=0,FIRE=1,NAV=2,WARP=3,PANEL=4,GRID=5,POS=6,TRG=7}; //Special gfx index/file numbers
	enum {BLACK=0,RED=1,LIGHTRED=2,GREEN=3,LIGHTGREEN=4,BLUE=5,LIGHTBLUE=6,YELLOW=7,ORANGE=8,PURPLE=9,GREY=10,DARKGREY=11,WHITE=12}; //Colour values

	static void init(); //Initialise the graphic system data
	static void setup(bool big,bool full); //Setup the screen, with big as true sets up an extra large window, full for fullscreen
	static void blit(); //Commit all graphics to the screen
	static graphic* get(int indx); //Fetch a graphic sprite by index
	static void string(char* str,int x,short y,bool opq); //Render a string to the screen, string and co-ordinates given, can put it in a (black) opaque box if necessary
	static void box(sbox* box,int col); //Render a box of given dimensions and color to the screen
	static void clip(sbox* box); //Set the graphics clipping box
	static void pix(int x,short y,short col); //Render a pixel to the screen
	static sbox dimension(); //Return the dimensions of the screen as an sbox
	static void line(int x1,short y1,short x2,short y2,short col); //Draw a line with given colour and co-ordinates
	static void embed(); //Embed screen so far as part of unerasable background
	static void clean(); //Erase off screen that which has been drawn since last time

	void draw(int x,short y,short rot,short zout,short haze,bool trg); //Draw this graphic at given co-ordinates, rotation and zoom-out, with given haze percentage. trg determines if targetting crosshairs are drawn

	private:
	enum {DTYP_PIX,DTYP_LINE,DTYP_RECT,DTYP_CLIP}; //Drawn types (see dtyp)
	
	graphic(int indx); //Constructor to create a clean graphic object
	void load(); //Load sprite from file
	void calculate(int rot,short zout); //Calculate a rotation and zoom

	static graphic* graphics[ISIZE]; //Index of available graphic objects (sprites)
	static SDL_Surface* screen; //Main drawing surface
	static SDL_Surface* font; //Font store
	static SDL_Surface* cloak; //Cloaking haze image
	static unsigned long cols[16]; //Colour keys, defined at initialisation
	static int nd; //Next 'drawn' slot
	static int dtyp[MAX_DRAW_OPERATIONS]; //Stores the type of drawn objects
	static SDL_Rect dpos[MAX_DRAW_OPERATIONS]; //Stores the position of drawn objects
	static SDL_Rect crct; //Stored clipping rect

	int self; //Self index of this sprite
	SDL_Surface* orig; //Original loaded graphic
	SDL_Surface* rots[36][4]; //Rotations and zooms, calculated as needed (rot[0][0] loaded from disk)
	bool imem; //In memory and loaded?
	bool miss; //Sprite missing? Keep this as a record and try not to load it again
};
