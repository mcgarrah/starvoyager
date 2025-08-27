/*
	interface.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "graphic.h"
#include "error.h"
#include "protocol.h"
#include "calc.h"
#include "interface.h"

void interface::init()
{
	cons=NULL;
	for(int i=0;i<8;i++)
		mesg[i]=NULL;
	edit[0]='\0';
	inp=false;
	ent=false;
	console_sprite_index=NULL;
}

void interface::setup()
{
	sbox tot; //Total screen area

	tot=graphic::dimension();

	viewb.x=tot.x;
	viewb.y=tot.y;
	viewb.w=tot.h;
	viewb.h=tot.h;

	panelb.x=viewb.x+viewb.w;
	panelb.y=tot.y;
	panelb.w=(tot.x+tot.w)-panelb.x;
	panelb.h=400;

	consb.x=panelb.x+5;
	consb.y=panelb.y+200;
	consb.w=(panelb.x+panelb.w)-consb.x-5;
	consb.h=panelb.h-214;

	radarb.x=panelb.x+5;
	radarb.y=panelb.y+25;
	radarb.w=150;
	radarb.h=radarb.w;

	barsb.x=panelb.x+panelb.w-38;
	barsb.y=panelb.y+75;
	barsb.w=32;
	barsb.h=100;

	editb.x=consb.x;
	editb.y=consb.y+consb.h+1;
	editb.w=consb.w;
	editb.h=10;

	mesgb.x=viewb.x+7;
	mesgb.y=viewb.y+viewb.h-70;
	mesgb.w=viewb.w-20;
	mesgb.h=viewb.y+viewb.h-mesgb.y-8;

	if(cons)
		delete[] cons;
	cons=new char[(consb.w/7)*(consb.h/7)];
	cons[0]='\0';

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);

	mtmo=0;
}

void interface::poll()
{
	SDL_Event evnt; //Event to be polled
	int n; //Unused area for the argument for SDL_GetKeyState

	lkey=-1;
	lasc=0;
	SDL_PumpEvents();
	keys=SDL_GetKeyState(&n);
	while(lkey==-1 && SDL_PollEvent(&evnt))
	{
		if(evnt.type==SDL_KEYDOWN)
		{
			lkey=evnt.key.keysym.sym;
			lasc=evnt.key.keysym.unicode;
			if(inp)
				lineedit();
		}
		if(evnt.type==SDL_QUIT)
			throw error("User requested quit");
	}
}

void interface::printtocons(const char* fmt,...)
{
	va_list fmts; //To help parsing the vargs

	inp=false;
	edit[0]='\0';
	console_sprite_index=NULL;
	va_start(fmts,fmt);
	vsprintf(cons,fmt,fmts);
	va_end(fmts);
}

void interface::spritetocons(graphic* sprite_index)
{
	inp=false;
	edit[0]='\0';
	console_sprite_index=sprite_index;
}

void interface::printtomesg(const char* fmt,...)
{
	va_list fmts; //To help parsing the vargs
	char txt[1024]; //Temporary text buffer

	if(fmt)
	{
		mtmo=100;
		if(mesg[0])
			delete mesg[0];
		mesg[0]=mesg[1];
		mesg[1]=mesg[2];
		mesg[2]=mesg[3];
		mesg[3]=mesg[4];
		mesg[4]=mesg[5];
		mesg[5]=mesg[6];
		mesg[6]=mesg[7];
		mesg[7]=NULL;
		va_start(fmts,fmt);
		vsprintf(txt,fmt,fmts);
		va_end(fmts);
		mesg[7]=new char[strlen(txt)+1];
		strcpy(mesg[7],txt);
		if((int)strlen(txt)>(mesgb.w/6))
			printtomesg("%s",txt+(mesgb.w/6));
	}
	else
	{
		for(int i=0;i<8;i++)
		{
			if(mesg[i])
			{
				delete[] mesg[i];
				mesg[i]=NULL;
			}
		}
	}
}

void interface::render()
{
	char* p1;
	char* p2; //Pointers for parsing text into console
	int y; //Y co-ordinate for writing to console
	char line[256]; //Line of console for scratchpad operations
	int elen; //String length of edit buffer
	int cwid; //Console width (in characters)

	graphic::clip(&mesgb);

	if(mtmo>0)
	{
		mtmo--;
		for(int i=0,y=mesgb.y;i<8;i++,y+=7)
			if(mesg[i])
				graphic::string(mesg[i],mesgb.x,y,false);
	}

	graphic::clip(&consb);

	if(console_sprite_index)
		console_sprite_index->render_to_screen(consb.x+consb.w/2,consb.y+consb.h/2,0,1,0,false);
	cwid=consb.w/6;
	y=consb.y;
	p1=cons;
	p2=cons;
	while(*p2)
	{
		if(*p2=='\0' || *p2=='\n' || p2-p1==cwid || p2-p1>=255)
		{
			memcpy(line,p1,p2-p1+1);
			line[p2-p1+1]='\0';
			graphic::string(line,consb.x,y,true);
			y+=7;
			p1=p2+1;
			if(*p2=='\0')
				break;
		}
		p2++;
	}

	graphic::clip(&editb);
	if(inp)
		graphic::box(&editb,graphic::RED);
	else
		graphic::box(&editb,graphic::BLACK);
	elen=strlen(edit);
	if(pwd)
	{
		for(int i=0;i<elen;i++)
			line[i]='*';
		line[elen]='\0';
		graphic::string(line,editb.x+2,editb.y+2,false);
	}
	else
	{
		if(elen>cwid)
			graphic::string(edit+(elen-cwid),editb.x+2,editb.y+2,false);
		else
			graphic::string(edit,editb.x+2,editb.y+2,false);
	}
}

sbox interface::consb;

bool interface::getline(char* put,bool hide)
{
	pwd=hide;
	if(!inp)
	{
		inp=true;
		ent=false;
		edit[0]='\0';
	}
	if(ent)
	{
		strncpy(put,edit,64);
		put[64]='\0';
		inp=false;
		ent=false;
		return true;
	}
	else
		return false;
}

sbox interface::viewb,interface::radarb,interface::barsb,interface::panelb;
bool interface::inp;
int interface::lkey;
unsigned char interface::lasc;
unsigned char* interface::keys;

void interface::lineedit() //Function to handle line-editing
{
	if(lkey==SDLK_RETURN)
	{
		ent=true;
		return;
	}
	if(lkey==SDLK_BACKSPACE)
	{
		for(int i=1;i<65;i++)
		{
			if(edit[i]=='\0')
			{
				edit[i-1]='\0';
				break;
			}
		}
	}
	if(lasc>=32 && lasc<127)
	{
		for(int i=0;i<64;i++)
		{
			if(edit[i]=='\0')
			{
				edit[i]=lasc;
				edit[i+1]='\0';
				break;
			}
		}
	}
}

sbox interface::mesgb;
sbox interface::editb;
char* interface::cons;
char* interface::mesg[8];
char interface::edit[65];
bool interface::pwd;
bool interface::ent;
graphic* interface::console_sprite_index;
int interface::mtmo;
