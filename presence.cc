/*
	presence.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL.h>
#include <string.h>
#include "calc.h"
#include "graphic.h"
#include "protocol.h"
#include "camera.h"
#include "interface.h"
#include "client.h"
#include "error.h"
#include "presence.h"

void presence::init()
{
	me=NULL;
	trg=NULL;
	hl=NULL;
	hltm=0;
	hul=0;
	pow=0;
	shd=0;
	ful=0;
	srng=0;
	lrng=0;
	vel.radius=0;
	vel.angle_degrees=0;
	for(int i=0;i<ISIZE;i++)
		objs[i]=NULL;
}

void presence::purgeall()
{
	for(int i=0;i<ISIZE;i++)
		if(objs[i])
			delete objs[i];
	init();
}

void presence::feed(unsigned char* buf)
{
	int id; //Id to use

	if(buf[0]==SERV_SELF)
	{
		updself(buf);
	}
	if(buf[0]==SERV_NEW)
	{
		id=calc::dattoint(buf+1);
		if(id>=0 && id<ISIZE)
		{
			if(objs[id])
				delete objs[id];
			objs[id]=new presence(id,buf);
		}
	}
	if(buf[0]==SERV_NAME)
	{
		id=calc::dattoint(buf+1);
		if(id>=0 && id<ISIZE)
		{
			if(objs[id])
				objs[id]->name(buf);
		}
	}
	if(buf[0]==SERV_UPD)
	{
		id=calc::dattoint(buf+1);
		if(id>=0 && id<ISIZE)
		{
			if(objs[id])
				objs[id]->update(buf);
		}
	}
	if(buf[0]==SERV_DEL)
	{
		id=calc::dattoint(buf+1);
		if(id>=0 && id<ISIZE)
			if(objs[id])
				delete objs[id];
	}
	if(buf[0]==SERV_HILIGHT)
	{
		id=calc::dattoint(buf+1);
		if(id>=0 && id<ISIZE && objs[id])
		{
			hl=objs[id];
			hltm=120;
		}
	}
}

void presence::interpolateall()
{
	if(hltm>0)
	{
		hltm--;
		if(hltm==0)
			hl=NULL;
	}
	for(int i=0;i<ISIZE;i++)
		if(objs[i])
			objs[i]->interpolate();
}

presence* presence::get(int indx)
{
	if(indx>=0 && indx<ISIZE)
	{
		if(objs[indx])
			return objs[indx];
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}


void presence::render()
{
	sbox bbox; //Box for rendering indicator bar

	graphic::clip(&interface::barsb);
	
	if(me)
	{
		graphic::box(&interface::barsb,graphic::BLACK);

		bbox.w=8;

		bbox.x=interface::barsb.x;
		bbox.y=interface::barsb.y+(interface::barsb.h-(shd*interface::barsb.h)/100);
		bbox.h=interface::barsb.h+interface::barsb.y-bbox.y;
		graphic::box(&bbox,graphic::RED);

		bbox.x+=8;
		bbox.y=interface::barsb.y+(interface::barsb.h-(pow*interface::barsb.h)/100);
		bbox.h=interface::barsb.h+interface::barsb.y-bbox.y;
		graphic::box(&bbox,graphic::GREEN);

		bbox.x+=8;
		bbox.y=interface::barsb.y+(interface::barsb.h-(hul*interface::barsb.h)/100);
		bbox.h=interface::barsb.h+interface::barsb.y-bbox.y;
		graphic::box(&bbox,graphic::WHITE);

		bbox.x+=8;
		bbox.y=interface::barsb.y+(interface::barsb.h-(ful*interface::barsb.h)/100);
		bbox.h=interface::barsb.h+interface::barsb.y-bbox.y;
		graphic::box(&bbox,graphic::BLUE);
	}
}

void presence::controls()
{
	presence* trg; //Target
	
	trg=NULL;
	if(interface::keys[SDLK_LEFT])
		client::action(CLIENT_TURN,-1);
	if(interface::keys[SDLK_RIGHT])
		client::action(CLIENT_TURN,+1);
	if(interface::keys[SDLK_DOWN]) {
		if(interface::keys[SDLK_LSHIFT] || interface::keys[SDLK_RSHIFT])
			client::action(CLIENT_ACCEL,-2);
		else
			client::action(CLIENT_ACCEL,-1);
	}
	if(interface::keys[SDLK_UP]) {
		if(interface::keys[SDLK_LSHIFT] || interface::keys[SDLK_RSHIFT])
			client::action(CLIENT_ACCEL,+2);
		else
			client::action(CLIENT_ACCEL,+1);
	}
	if(!interface::inp)
	{
		if(interface::keys[SDLK_SPACE])
			client::action(CLIENT_SHOOT,0);
		if(interface::lasc==SDLK_z)
			client::action(CLIENT_SHOOT,1);
		if(interface::lasc=='0')
			client::action(CLIENT_CONS,0);
		if(interface::lasc>='1' && interface::lasc<='9')
			client::action(CLIENT_CONS,interface::lasc-'1'+1);
		if(interface::lasc=='t')
			trg=gettarget(PT_SHIP,+1,camera::cov,false,false);
		if(interface::lasc=='T')
			trg=gettarget(PT_SHIP,-1,camera::cov,false,false);
		if(interface::lasc=='e')
			trg=gettarget(PT_SHIP,+1,camera::cov,false,true);
		if(interface::lasc=='E')
			trg=gettarget(PT_SHIP,-1,camera::cov,false,true);
		if(interface::lasc=='p')
			trg=gettarget(PT_PLANET,+1,camera::cov,false,false);
		if(interface::lasc=='P')
			trg=gettarget(PT_PLANET,-1,camera::cov,false,false);
		if(trg)
			client::action(CLIENT_TRG,trg->self);
		if(interface::lasc=='q')
			throw error("User requested quit");
		if(interface::keys[SDLK_MINUS])
			camera::radarzoom(-1);
		if(interface::keys[SDLK_EQUALS] || interface::lasc=='+')
			camera::radarzoom(+1);
		if(interface::lasc=='/')
			camera::viewzoom();
	}
	if(interface::lkey>=SDLK_F1 && interface::lkey<=SDLK_F10)
	{
		client::action(CLIENT_CMOD,interface::lkey-SDLK_F1);
	}
}

void presence::drawat(int sx,short sy,short zout)
{
	int rot; //Rotation frame to use
	long lx,ly; //Co-ordinates of 'link' presence
	long thx,thy; //Thickness offsets
	graphic* pspr; //Graphic for marking position while cloaked
	if(spr)
	{
		rot=((ang+5)/10)%36;
		if(rot<0)
			rot+=36;
		if(vis)
		{
			if(this==trg)
				spr->draw(sx,sy,rot,zout,100-vis,true);
			else
				spr->draw(sx,sy,rot,zout,100-vis,false);
		}
		else
		{
			pspr=graphic::get(graphic::POS);
			pspr->draw(sx,sy,rot,1,0,false);
		}	
	}
	if(col>=0 && link)
	{
		lx=sx-((mov.x_component-link->mov.x_component)*8*age)/(zout*9);
		ly=sy-((mov.y_component-link->mov.y_component)*8*age)/(zout*9);
		graphic::line(sx,sy,lx,ly,col);
		if(zout==1)
		{
			if(lx>0)
			{
				if(ly>0)
				{
					thx=+1;
					thy=-1;
				}
				else
				{
					thx=+1;
					thy=+1;
				}
			}
			else
			{
				if(ly>0)
				{
					thx=-1;
					thy=+1;
				}
				else
				{
					thx=-1;
					thy=-1;
				}
			}
			graphic::line(sx+thx,sy+thy,lx+thx,ly+thy,col);
		}
	}
}

presence* presence::me;
presence* presence::trg;
presence* presence::hl;
int presence::hul;
int presence::pow;
int presence::shd;
int presence::ful;
ipol presence::vel;
long presence::srng,presence::lrng;

presence::presence(int self,unsigned char* buf)
{
	this->self=self;
	age=0;

	buf+=3;

	typ=*buf;
	buf+=1;

	col=calc::dattoint(buf);
	if(col>=0)
	{
		spr=graphic::get(col);
		col=-1;
	}
	else
	{
		spr=NULL;
		col=-col;
	}
	buf+=2;

	link=presence::get(calc::dattoint(buf));
	buf+=2;
}

presence::~presence()
{
	if(self>=0 && self<ISIZE)
		objs[self]=NULL;
	if(me==this)
	{
		me=NULL;
		camera::unbind();
	}
	if(trg==this)
		trg=NULL;
	if(hl==this)
		hl=NULL;
}

presence* presence::gettarget(int typ,short dir,box cov,bool out,bool enem)
{
	int st; //Starting index

	if(!me)
		return NULL;
	if(dir>0)
		dir=+1;
	else
		dir=-1;
	if(trg)
		st=trg->self+dir;
	else
		st=0;
	if(st<0)
		st=ISIZE;
	else if(st>=ISIZE)
		st=0;
	for(int i=0;i<ISIZE;i++)
	{
		if(objs[st] && objs[st]!=me && objs[st]->typ==typ) {
			if(
			objs[st]->loc.x_component<cov.x2 &&
			objs[st]->loc.x_component>cov.x1 &&		
			objs[st]->loc.y_component<cov.y2 &&		
			objs[st]->loc.y_component>cov.y1
			)
			{
				if(!out && ((objs[st]->enem && enem) || !enem))
					return objs[st];
			}
			else
			{
				if(out && ((objs[st]->enem && enem) || !enem))
					return objs[st];
			}
		}
				
		st=(st+dir);
		if(st<0)
			st=ISIZE;
		st=st%ISIZE;
	}
	return NULL;
}

void presence::updself(unsigned char* buf)
{
	int i; //Temp, for resolving other presences

	buf+=1;

	i=calc::dattoint(buf);
	me=get(i);
	if(me)
	{
		camera::turnon();
		camera::bind(me);
	}
	else
	{
		camera::unbind();
	}
	buf+=2;

	hul=calc::dattoint(buf);
	buf+=2;

	pow=calc::dattoint(buf);
	buf+=2;

	shd=calc::dattoint(buf);
	buf+=2;

	ful=calc::dattoint(buf);
	buf+=2;

	srng=calc::dattolong(buf);
	buf+=4;

	lrng=calc::dattolong(buf);
	buf+=4;

	i=calc::dattoint(buf);
	if(i>=0 && i<ISIZE && objs[i])
		trg=objs[i];
	else
		trg=NULL;
	buf+=2;
}

void presence::name(unsigned char* buf)
{
	buf+=3;

	memcpy(nam,buf,64);
	nam[64]='\0';
	buf+=64;

	memcpy(anno,buf,64);
	nam[64]='\0';
	buf+=64;
}



void presence::update(unsigned char* buf)
{
	buf+=3;
	loc.x_component=calc::dattolong(buf);
	buf+=4;
	loc.y_component=calc::dattolong(buf);
	buf+=4;

	mov.x_component=calc::dattolong(buf);
	buf+=4;

	mov.y_component=calc::dattolong(buf);
	buf+=4;

	ang=calc::dattoint(buf);
	buf+=2;

	enem=*buf;
	buf+=1;

	vis=*buf;
	if(vis>100)
		vis=100;
	buf+=1;
}

void presence::interpolate()
{
	if(!me && (link || (typ==PT_FRAG && calc::random_int(40)==0)))
	{
		delete this;
		return;
	}
	//if(typ==PT_FRAG && col!=-1 && age>=5)
	//	return;
	if(this==me)
	{
		vel=mov.to_polar_coordinates();
		vel.angle_degrees=ang;
	}
	loc.x_component+=mov.x_component;
	loc.y_component+=mov.y_component;
	age++;
}

presence* presence::objs[ISIZE];
int presence::hltm;
