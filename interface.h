/*
	interface.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

struct sbox;
class graphic;

class interface //User interface module
{
	public:
	static void init(); //Initialise datastructures
	static void setup(); //Setup layout and stuff
	static void poll(); //Main interface cycle, polling for keyboard and other input
	static void printtocons(const char* fmt,...); //Print to the game console
	static void spritetocons(graphic* sprite_index); //Draw an underlay sprite to the console
	static void printtomesg(const char* fmt,...); //Print to the message tickertape
	static void render(); //Update the interface to screen
	static bool getline(char* put,bool hide); //Make sure the editing buffer is active and return true if text has been entered and put it into the given string buffer, hide determining password field style

	static sbox viewb,radarb,barsb,panelb; //Bounding boxes for radar, main view, indicator bars and the rightside panel
	static bool inp; //In input mode?
	static int lkey; //Keysym of last key pressed
	static unsigned char lasc; //Ascii value of last key pressed
	static unsigned char* keys; //Pointer to keyboard state

	private:
	static void lineedit(); //Function to handle line-editing

	static sbox consb,mesgb,editb; //Bounding boxes for console, message and editing panes
	static char* cons; //Console text
	static char* mesg[8]; //Message ticker tape lines
	static char edit[65]; //Editing buffer
	static bool pwd; //In password mode for editing buffer?
	static bool ent; //Line entering completed?
	static graphic* console_sprite_index; //Underlay console sprite
	static int mtmo; //Messages timeout (for disappearing)
};
