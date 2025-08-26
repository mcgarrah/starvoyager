/*
	frag.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <signal.h>
#include <SDL.h>
#include "calc.h"
#include "database.h"
#include "error.h"
#include "ship.h"
#include "planet.h"
#include "protocol.h"
#include "frag.h"

frag::frag(cord loc,int typ,short spr,short col,ship* trg,ship* own,vect mov,short rot,short pow,short trck,short rng)
{
	self=-1;
	for(int i=0,j=0;i<50;i++)
	{
		j=calc::random_int(ISIZE);
		if(!frags[j])
		{
			self=j;
			break;
		}
	}
	if(self==-1)
		throw error("No free slots for frag");
	frags[self]=this;

	this->loc=loc;
	this->typ=typ;
	this->spr=spr;
	this->col=col;
	this->trg=trg;
	this->own=own;
	this->mov=mov;
	this->rot=rot;
	this->pow=pow;
	this->trck=trck;
	this->rng=rng;
}

frag::~frag()
{
	if(self!=-1)
		frags[self]=NULL;
}

void frag::init()
{
	for(int i=0;i<ISIZE;i++)
		frags[i]=NULL;
}

void frag::purgeall()
{
	for(int i=0;i<ISIZE;i++)
		if(frags[i])
			delete frags[i];
}

void frag::simulateall()
{
	for(int i=0;i<ISIZE;i++)
	{
		if(frags[i])
			frags[i]->physics();
		if(frags[i] && frags[i]->trg)
			frags[i]->home();
	}	
}

void frag::notifydelete(ship* spawned_ship)
{
	for(int i=0;i<ISIZE;i++)
	{
		if(frags[i])
		{
			if(frags[i]->trg==spawned_ship)
				frags[i]->trg=NULL;
			if(frags[i]->own==spawned_ship)
				frags[i]->own=NULL;
		}
	}
}

void frag::saveall()
{
	char obsc[33]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		sprintf(obsc,"Frag%hd",i);
		if(frags[i])
		{
			database::putobject(obsc);
			frags[i]->save();
		}
	}
}

void frag::loadall()
{
	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			new frag(i);
		}
		catch(error it)
		{
		}
	}
}

frag* frag::get(int indx)
{
	if(indx>=0 && indx<ISIZE)
		if(frags[indx])
			return frags[indx];
		else
			return NULL;
	else
		return NULL;
}

void frag::serialize_to_network(int typ,unsigned char* buf)
{
	if(!buf)
		return;
	
	buf[0]=typ;
	buf+=1;

	calc::inttodat(frag2pres(self),buf);
	buf+=2;
	switch(typ)
	{
		case SERV_NEW:
		*buf=PT_FRAG;
		buf+=1;
		if(col>=0)
			calc::inttodat(-col,buf);
		else
			calc::inttodat(spr,buf);
		buf+=2;
		if(own)
			calc::inttodat(ship2pres(own->self),buf);
		else
			calc::inttodat(-1,buf);
		break;

		case SERV_UPD:
		calc::longtodat(loc.x_component,buf);
		buf+=4;
		calc::longtodat(loc.y_component,buf);
		buf+=4;
		calc::longtodat(mov.x_component,buf);
		buf+=4;
		calc::longtodat(mov.y_component,buf);
		buf+=4;
		calc::inttodat(rot*10,buf);
		buf+=2;
		*buf=0;
		buf+=1;
		*buf=100;
		buf+=1;
		break;
	}
}

frag::frag(int self)
{
	char obsc[16]; //Object name scratchpad

	this->self=-1;
	sprintf(obsc,"Frag%hd",self);
	database::select_database_object(obsc);
	load();
	this->self=self;
	frags[self]=this;
}

void frag::physics()
{
	rng--;
	if(rng<1)
	{
		delete this;
		return;
	}
	if(typ==2)
		rot=(rot+3)%36;
	if(trg && (typ==1 || typ==2))
	{
		if(trg->detect_collision(loc,mov))
		{
			trg->take_damage(pow,loc,mov,own);
			delete this;
			return;
		}
	}
	
	loc.x_component+=mov.x_component;
	loc.y_component+=mov.y_component;
}

void frag::home()
{
	vect trv; //Target vector
	pol trp; //Target polar

	trv.x_component=((trg->loc.x_component+trg->mov.x_component)-(loc.x_component+mov.x_component));
	trv.y_component=((trg->loc.y_component+trg->mov.y_component)-(loc.y_component+mov.y_component));
	trp=trv.to_polar_coordinates();

	//Enforce acceleration restriction
	if(trp.radius>trck)
		trp.radius=trck;

	trv=trp.to_vector_coordinates();
	mov.x_component+=trv.x_component;
	mov.y_component+=trv.y_component;
}

void frag::save()
{
	database::store_attribute("Type",typ);
	database::store_attribute("Sprite",spr);
	database::store_attribute("Colour",col);
	if(trg)
		database::store_attribute("Target",trg->self);
	if(own)
		database::store_attribute("Owner",own->self);
	database::store_attribute("XLoc",loc.x_component);
	database::store_attribute("YLoc",loc.y_component);
	database::store_attribute("XVect",mov.x_component);
	database::store_attribute("YVect",mov.y_component);
	database::store_attribute("Rotation",rot);
	database::store_attribute("Power",pow);
	database::store_attribute("Tracking",trck);
	database::store_attribute("Range",rng);
}

void frag::load()
{
	typ=database::retrieve_attribute("Type");
	spr=database::retrieve_attribute("Sprite");
	col=database::retrieve_attribute("Colour");
	trg=ship::find_by_index(database::retrieve_attribute("Target"));
	own=ship::find_by_index(database::retrieve_attribute("Owner"));
	loc.x_component=database::retrieve_attribute("XLoc");
	loc.y_component=database::retrieve_attribute("YLoc");
	mov.x_component=database::retrieve_attribute("XVect");
	mov.y_component=database::retrieve_attribute("YVect");
	rot=database::retrieve_attribute("Rotation");
	pow=database::retrieve_attribute("Power");
	trck=database::retrieve_attribute("Tracking");
	rng=database::retrieve_attribute("Range");
}

frag* frag::frags[ISIZE];
