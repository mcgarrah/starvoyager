/*
	graphic.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL.h>
#include <string.h>
#include "SDL_rotozoom.h"
#include "SDL_gfxPrimitives.h"
#include "calc.h"
#include "error.h"
#include "interface.h"
#include "graphic.h"

void graphic::init()
{
	screen=NULL;
	for(int i=0;i<ISIZE;i++)
		graphics[i]=NULL;
	nd=0;
}

void graphic::setup(bool big,bool full)
{
	Uint32 flags; //Flags for setting video;
	SDL_Surface* tmp; //Temporary holding place while sprites are converted
	char* path; //Path to load bmp from

	if(SDL_InitSubSystem(SDL_INIT_VIDEO)==-1)
		throw error(SDL_GetError());
	flags=SDL_DOUBLEBUF|SDL_HWSURFACE;
	if(full)
		flags=flags|SDL_FULLSCREEN;
	if(big)
		screen=SDL_SetVideoMode(800,600,0,flags);
	else
		screen=SDL_SetVideoMode(600,400,0,flags);
	if(!screen)
		throw error(SDL_GetError());
	crct.x=0;
	crct.y=0;
	crct.w=screen->w;
	crct.h=screen->h;
	SDL_WM_SetCaption("Star Voyager","Star Voyager");
	SDL_ShowCursor(0);

	cols[BLACK]=SDL_MapRGB(screen->format,0,0,0);
	cols[RED]=SDL_MapRGB(screen->format,255,0,0);
	cols[LIGHTRED]=SDL_MapRGB(screen->format,255,100,100);
	cols[GREEN]=SDL_MapRGB(screen->format,0,255,0);
	cols[LIGHTGREEN]=SDL_MapRGB(screen->format,100,255,100);
	cols[BLUE]=SDL_MapRGB(screen->format,0,0,255);
	cols[LIGHTBLUE]=SDL_MapRGB(screen->format,100,100,255);
	cols[YELLOW]=SDL_MapRGB(screen->format,255,255,0);
	cols[ORANGE]=SDL_MapRGB(screen->format,255,100,100);
	cols[PURPLE]=SDL_MapRGB(screen->format,255,0,255);
	cols[GREY]=SDL_MapRGB(screen->format,180,180,180);
	cols[DARKGREY]=SDL_MapRGB(screen->format,64,64,64);
	cols[WHITE]=SDL_MapRGB(screen->format,255,255,255);

	path=new char[strlen(DATADIR)+32];
	sprintf(path,"%s/gfx/font.bmp",DATADIR);
	font=SDL_LoadBMP(path);
	delete[] path;
	if(!font)
		throw error(SDL_GetError());
	tmp=SDL_ConvertSurface(font,screen->format,SDL_SWSURFACE);
	SDL_FreeSurface(font);
	font=tmp;
	SDL_SetColorKey(font,SDL_SRCCOLORKEY|SDL_RLEACCEL,cols[BLACK]);

	path=new char[strlen(DATADIR)+32];
	sprintf(path,"%s/gfx/haze.bmp",DATADIR);
	cloak=SDL_LoadBMP(path);
	delete[] path;
	if(!cloak)
		throw error(SDL_GetError());
	tmp=SDL_ConvertSurface(cloak,screen->format,SDL_SWSURFACE);
	SDL_FreeSurface(cloak);
	cloak=tmp;
	SDL_SetColorKey(cloak,SDL_SRCCOLORKEY|SDL_RLEACCEL,cols[WHITE]);
}

void graphic::blit()
{
	SDL_Flip(screen);
}

graphic* graphic::get(int indx)
{
	if(!(indx>=0 && indx<ISIZE))
		return NULL;
	if(!graphics[indx])
	{
		graphics[indx]=new graphic(indx);
	}
	return graphics[indx];
}

void graphic::string(char* str,int x,short y,bool opq)
{
	SDL_Rect srct,drct; //Source rect and destination rect
	int i; //Position in text
	char c; //Character in use
	char l; //Letter to print


	if(opq)
	{
		drct.x=x;
		drct.y=y;
		drct.w=6*strlen(str);
		drct.h=5;
		SDL_FillRect(screen,&drct,cols[BLACK]);
	}

	i=0;
	srct.y=1;
	srct.w=5;
	srct.h=5;
	drct.x=x;
	drct.y=y;
	drct.w=5;
	drct.h=5;
	while(str[i]!=0)
	{
		c=str[i];
		if(c=='\n')
			c=32;
		if(c>32 && c<127)
		{
			l=c-33;
			srct.x=l*6+1;
			SDL_BlitSurface(font,&srct,screen,&drct);
			drct.x+=6;		
		}
		if(c==' ')
			drct.x+=6;
		i++;
	}
	srct.x=x;
	srct.y=y;
	srct.w=drct.x-x;
	srct.h=5;
	if(nd<MAX_DRAW_OPERATIONS)
	{
		dtyp[nd]=DTYP_RECT;
		dpos[nd]=srct;
		nd++;
	}
}

void graphic::box(sbox* box,int col)
{
	SDL_Rect rect; //Rectangle to draw

	rect.x=box->x;
	rect.y=box->y;
	rect.w=box->w;
	rect.h=box->h;
	SDL_FillRect(screen,&rect,cols[col]);
	if(nd<MAX_DRAW_OPERATIONS)
	{
		dtyp[nd]=DTYP_RECT;
		dpos[nd]=rect;	
		nd++;
	}
}

void graphic::clip(sbox* box)
{
	SDL_Rect rect; //Rectangle to clip with

	rect.x=box->x;
	rect.y=box->y;
	rect.w=box->w;
	rect.h=box->h;
	crct=rect;
	SDL_SetClipRect(screen,&rect);
	if(nd<MAX_DRAW_OPERATIONS)
	{
		dtyp[nd]=DTYP_CLIP;
		dpos[nd]=rect;	
		nd++;
	}
}

void graphic::pix(int x,short y,short col)
{
	Uint8 r,g,b;
	SDL_GetRGB(cols[col],screen->format,&r,&g,&b);
	pixelColor(screen,x,y,(r<<24)|(g<<16)|(b<<8)|0xFF);
	if(nd<MAX_DRAW_OPERATIONS)
	{
		dtyp[nd]=DTYP_PIX;
		dpos[nd].x=x;
		dpos[nd].y=y;
		nd++;
	}
}

sbox graphic::dimension()
{
	sbox out; //Value to output

	out.x=0;
	out.y=0;
	out.w=screen->w;
	out.h=screen->h;
	return out;
}

void graphic::line(int x1,short y1,short x2,short y2,short col)
{
	if(col>=0 && col<16)
	{
		Uint8 r,g,b;
		SDL_GetRGB(cols[col],screen->format,&r,&g,&b);
		lineColor(screen,x1,y1,x2,y2,(r<<24)|(g<<16)|(b<<8)|0xFF);
		if(nd<MAX_DRAW_OPERATIONS)
		{
			dtyp[nd]=DTYP_LINE;
			dpos[nd].x=x1;
			dpos[nd].y=y1;
			dpos[nd].w=x2;
			dpos[nd].h=y2;
			nd++;
		}
	}
}

void graphic::draw(int x,short y,short rot,short zout,short haze,bool trg)
{
	graphic* tspr; //Targetting sprite
	SDL_Rect dst; //Destination rect
	int hw,hh; //Half-width and half-height, for centering

	if(miss)
	{
		string(const_cast<char*>("Graphic missing"),x,y,true);
	}
	else
	{
		if(!(rot>=0 && rot<36 && zout>=0 && zout<4))
			return;
		if(!rots[rot][zout-1])
			calculate(rot,zout);
		hw=(rots[rot][zout-1]->w)/2;
		hh=(rots[rot][zout-1]->h)/2;
		dst.x=x-hw;
		dst.y=y-hh;
		dst.w=hw*2;
		dst.h=hh*2;
		SDL_BlitSurface(rots[rot][zout-1],NULL,screen,&dst);
		if(trg)
		{
			tspr=get(TRG);
			if(tspr)
			{
				tspr->draw(x,y-hh,0,1,0,false);
				tspr->draw(x,y+hh,18,1,0,false);
				tspr->draw(x-hw,y,27,1,0,false);
				tspr->draw(x+hw,y,9,1,0,false);
			}
		}
		if(nd<MAX_DRAW_OPERATIONS)
		{
			dtyp[nd]=DTYP_RECT;
			dpos[nd]=dst;
			nd++;
		}
		if(haze>0 && hw*2>cloak->w && hh*2>cloak->h)
		{
			dst.w=2;
			dst.h=2;
			for(int i=0;i<haze;i++)
			{
				dst.x=x-hw+calc::rnd(hw*2-cloak->w);
				dst.y=y-hh+calc::rnd(hh*2-cloak->h);
				SDL_BlitSurface(cloak,NULL,screen,&dst);
			}
		}

	}
}

void graphic::embed()
{
	nd=0;
}

void graphic::clean()
{
	SDL_SetClipRect(screen,NULL);
	for(int i=0;i<nd;i++)
	{
		switch(dtyp[i])
		{
			case DTYP_CLIP:
			SDL_SetClipRect(screen,&dpos[i]);
			break;
			
			case DTYP_PIX:
			{
				Uint8 r,g,b;
				SDL_GetRGB(cols[BLACK],screen->format,&r,&g,&b);
				pixelColor(screen,dpos[i].x,dpos[i].y,(r<<24)|(g<<16)|(b<<8)|0xFF);
			}
			break;
			
			case DTYP_LINE:
			{
				Uint8 r,g,b;
				SDL_GetRGB(cols[BLACK],screen->format,&r,&g,&b);
				lineColor(screen,dpos[i].x,dpos[i].y,dpos[i].w,dpos[i].h,(r<<24)|(g<<16)|(b<<8)|0xFF);
			}
			break;
		
			case DTYP_RECT:
			SDL_FillRect(screen,&dpos[i],cols[BLACK]);
			break;
		}
	}
	nd=0;
}

graphic::graphic(int indx)
{
	self=indx;
	miss=false;
	for(int i=0;i<36;i++)
		for(int j=0;j<4;j++)
			rots[i][j]=NULL;
	try
	{
		load();
	}
	catch(error it)
	{
		miss=true;
	}
}

void graphic::load()
{
	SDL_Surface* tmp; //Temporary holding place while sprites are converted
	char* path; //Path to load sprite from

	int path_len = strlen(DATADIR)+32;
	path=new char[path_len];
	snprintf(path,path_len,"%s/gfx/%i.bmp",DATADIR,self);
	orig=SDL_LoadBMP(path);
	delete[] path;
	if(!orig)
		throw error(SDL_GetError());
	tmp=SDL_ConvertSurface(orig,screen->format,SDL_SWSURFACE);
	SDL_FreeSurface(orig);
	orig=tmp;
	SDL_SetColorKey(orig,SDL_SRCCOLORKEY|SDL_RLEACCEL,cols[BLACK]);
	calculate(0,1);
}

void graphic::calculate(int rot,short zout)
{
	int ang; //Angle to rotate by

	ang=360-rot*10;
	if(self>=16)
		rots[rot][zout-1]=rotozoomSurface(orig,ang,1.0/zout,1);
	else
		rots[rot][zout-1]=rotozoomSurface(orig,ang,1.0/zout,0);
	SDL_SetColorKey(rots[rot][zout-1],SDL_SRCCOLORKEY|SDL_RLEACCEL,cols[BLACK]);
	if(!rots[rot][zout-1])
		throw error(SDL_GetError());
}

graphic* graphic::graphics[ISIZE];
SDL_Surface* graphic::screen;
SDL_Surface* graphic::font;
SDL_Surface* graphic::cloak;
unsigned long graphic::cols[16];
int graphic::nd;
int graphic::dtyp[1024];
SDL_Rect graphic::dpos[1024];
SDL_Rect graphic::crct;
