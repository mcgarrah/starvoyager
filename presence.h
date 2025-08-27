/*
	presence.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

struct box;
struct icord;
struct ivect;
class graphic;

class presence //Client side view of the universe and its simple objects
{
	public:
	enum {ISIZE=2048}; //Size of presence object index
	static void init(); //Initialise the presence system datastructures
	static void purgeall(); //Purge all presence data after use
	static void update_display_data(unsigned char* buf); //Update/create a presence from a buffer
	static void interpolateall(); //Run interpolation on all the presence objects
	static presence* get(int indx); //Return the presence object of given index
	static void render(); //Render client/presence info, such as realtime information indicators
	static void controls(); //Run the in game ship controls

	void drawat(int sx,short sy,short zout); //Draw at the given screen location with the given zoom-out scaling

	static presence* me; //Presence played by this client
	static presence* trg; //Personal target
	static presence* hl; //Hilight target
	static int hul,pow,shd,ful; //Hull, power, shield and fuel percentages
	static ipol vel; //Velocity of self
	static long srng,lrng; //Sensor range and long range

	int self; //Presence object self index
	int typ; //Object type
	char nam[65]; //Name of object
	char anno[65]; //Annotative data, e.g. alliance
	icord loc; //Location
	ivect mov; //Motion vector
	bool enem; //Is enemy?

	private:
	presence(int self,unsigned char* buf); //Create a new object with a given network buffer input and given self index
	~presence(); //Delete this presence object

	static presence* gettarget(int typ,short dir,box cov,bool out,bool enem); //Flip through targets of given type, flicking either forwards or backwards(dir -1 or +1) through them, within the given bounding box, out refers to outside the box or not, enem if you want hostiles only
	static void updself(unsigned char* buf); //Update self data with given buffer

	void name(unsigned char* buf); //Name the object with given network buffer
	void update(unsigned char* buf);  //Update an object with given buffer
	void interpolate(); //Run motion interpolation on this presence

	static presence* objs[ISIZE]; //Index of presence objects
	static int hltm; //Hilight time remaining

	graphic* spr; //Sprite
	int col; //Colour (makes it a beam)
	int vis; //Visibility percentage (for cloaking stuff)
	int ang; //Angle of orientation
	int age; //Age of presence object
	presence* link; //Link to another presence, for example owner of a frag
};
