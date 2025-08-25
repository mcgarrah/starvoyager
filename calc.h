/*
	calc.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
	
	COORDINATE SYSTEM NOTE:
	This game uses COMPASS/NAVIGATION coordinate system:
	- 0° = North (positive Y)
	- 90° = East (positive X) 
	- 180° = South (negative Y)
	- 270° = West (negative X)
	Angles increase clockwise. See COORDINATE_SYSTEM.md for details.
*/

#ifndef CALC_H
#define CALC_H

#include <math.h>
#include <SDL_types.h>
#include <SDL_endian.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class vect;
class pol //A polar vector
{
	public:
	inline vect to_vector_coordinates(); //Returns conversion to a vect

        double angle_degrees; //Angle
	double radius; //Radius
};

class vect //A vector
{
	public:
	inline pol to_polar_coordinates(); //Returns conversion to a pol
	
	double x_component;
	double y_component; //X and Y components
};

inline vect pol::to_vector_coordinates()
{
	vect out; //Return value
	double cang; //Converted angle

	// COORDINATE SYSTEM: Game uses compass convention (0°=North, 90°=East)
	// Mathematical convention is (0°=East, 90°=North)
	// Subtract 90° to convert from game angle to mathematical angle
	cang=((angle_degrees-90)*M_PI)/180;
	out.x_component=radius*cos(cang);
	out.y_component=radius*sin(cang);
	return out;
}

inline pol vect::to_polar_coordinates()
{
	pol out; //Return value

	// COORDINATE SYSTEM: Converts Cartesian to compass angles
	out.radius=sqrt(x_component*x_component+y_component*y_component);
	if(x_component!=0)
	{
		out.angle_degrees=atan(y_component/x_component)*(180/M_PI);
		if(y_component>0)
		{
			if(x_component>0)
				out.angle_degrees=90+out.angle_degrees;
			else
				out.angle_degrees=270+out.angle_degrees;
		}
		else
		{
			if(x_component>0)
				out.angle_degrees=90+out.angle_degrees;
			else
				out.angle_degrees=360-(90-out.angle_degrees);
		}
	}
	else
	{
		if(y_component>0)
			out.angle_degrees=180; // South (positive Y in compass system)
		else
			out.angle_degrees=0;   // North (negative Y in compass system)
	}
	return out;
}

class ivect;
class ipol //Integer version of pol
{
	public:
	inline ivect to_vector_coordinates(); //Returns conversion to an ivect

	int angle_degrees;
	long radius;
};

class ivect //Integer version of vect
{
	public:
	inline ipol to_polar_coordinates(); //Returns conversion to an ipol

	long x_component;
	long y_component;
};

inline ivect ipol::to_vector_coordinates()
{
	ivect out; //Return value
	double cang; //Converted angle

	// COORDINATE SYSTEM: Game uses compass convention (0°=North, 90°=East)
	// Subtract 90° to convert from game angle to mathematical angle
	cang=(((double)angle_degrees-90)*M_PI)/180;
	out.x_component=(long)(radius*cos(cang));
	out.y_component=(long)(radius*sin(cang));
	return out;
}

inline ipol ivect::to_polar_coordinates()
{
	ipol out; //Return value

	// COORDINATE SYSTEM: Converts Cartesian to compass angles (integer version)
	out.radius=(long)sqrt(x_component*x_component+y_component*y_component);
	if(x_component!=0)
	{
		out.angle_degrees=(int)((double)atan(y_component/x_component)*(180/M_PI));
		if(y_component>0)
		{
			if(x_component>0)
				out.angle_degrees=90+out.angle_degrees;
			else
				out.angle_degrees=270+out.angle_degrees;
		}
		else
		{
			if(x_component>0)
				out.angle_degrees=90+out.angle_degrees;
			else
				out.angle_degrees=360-(90-out.angle_degrees);
		}
	}
	else
	{
		if(y_component>0)
			out.angle_degrees=180; // South (positive Y in compass system)
		else
			out.angle_degrees=0;   // North (negative Y in compass system)
	}
	return out;
}

struct cord //A co-ordinate in the game
{
	double x_component;
	double y_component; //X and Y components
};

struct icord //Integer version of cord
{
	long x_component;
	long y_component;
};


struct box //A universe bounding box
{
	long x1,y1; //Top-left corner
	long x2,y2; //Top-right corner
};

class calc //Mathematics module
{
        public:
		static void init(); //Initialise some calculation data
        static long random_int(long max); //Return random integer from 0 to max-1
        static void getspeed(long spd,char* put); //Convert given game velocity to a string
        inline static long dattolong(unsigned char* in) //Converts a four byte buffer portably into a long
		{
			Uint32 tmp; //Temporary value holder
			unsigned char* tmpp=(unsigned char*)&tmp; //Accessor for tmp
			long out; //Value to output

			tmpp[0]=in[0];
			tmpp[1]=in[1];
			tmpp[2]=in[2];
			tmpp[3]=in[3];
			#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			tmp=SDL_Swap32(tmp);
			#endif	
			out=tmp-2147483647;

			return out;
		}
        inline static void longtodat(long in,unsigned char* out) //Puts a long portably into a four byte buffer
		{
			Uint32 tmp; //Temporary value holder
			unsigned char* tmpp=(unsigned char*)&tmp; //Accessor for tmp

			tmp=in+2147483647;
			#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			tmp=SDL_Swap32(tmp);
			#endif	
			out[0]=tmpp[0];
			out[1]=tmpp[1];
			out[2]=tmpp[2];
			out[3]=tmpp[3];
			return;
		}
        inline static int dattoint(unsigned char* in) //Converts a two byte buffer portably into a short
		{
			Uint16 tmp; //Temporary value holder
			unsigned char* tmpp=(unsigned char*)&tmp; //Accessor for tmp
			int out; //Value to output

			tmpp[0]=in[0];
			tmpp[1]=in[1];
			#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			tmp=SDL_Swap16(tmp);
			#endif	
			out=tmp-32768;

			return out;
		}
        inline static void inttodat(short in,unsigned char* out) //Puts a long portably into a four byte buffer
		{
			Uint16 tmp; //Temporary value holder
			unsigned char* tmpp=(unsigned char*)&tmp; //Accessor for tmp

			tmp=in+32768;
			#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			tmp=SDL_Swap16(tmp);
			#endif	
			out[0]=tmpp[0];
			out[1]=tmpp[1];
			return;
		}
        static bool data_arrays_equal(unsigned char* d1,unsigned char* d2,int n); //Test two data streams for equality, up to n bytes
		static void encrypt_password(char* str); //Munges the string so it is no longer human readable; the munging is consisten, like a very weak crypt

        private:
        static long warp_speed_table[10]; //Warp speed table
        static char speed_string_buffer[33]; //Speed string (saves having to malloc, but it ain't threadsafe!)
};

#endif // CALC_H
