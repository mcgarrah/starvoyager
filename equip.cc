/*
	equip.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include "database.h"
#include "error.h"
#include "equip.h"

void equip::init()
{
}

void equip::loadlib()
{
	char nam[16]; //Object name to load equipment data from

	for(int i=0;i<LIBSIZE;i++)
	{
		sprintf(nam,"Equipment%hd",i);
		try
		{
			database::select_database_object(nam);
			equips[i]=new equip(i);
			equips[i]->load();
		}
		catch(error it)
		{
		}
	}
}

equip* equip::get(int indx)
{
	if(indx>=0 && indx<LIBSIZE)
	{
		if(equips[indx])
			return equips[indx];
		else
			return NULL;
	}
	else
		return NULL;
}

equip::equip(int self)
{
	this->self=self;
}

void equip::load()
{
	database::retrieve_attribute("Name",nam);
	equipment_type=database::retrieve_attribute("Type");
	mass=database::retrieve_attribute("Mass");
	sprite_index=database::retrieve_attribute("Sprite");
	color_index=database::retrieve_attribute("Colour");
	sound_index=database::retrieve_attribute("Sound");
	power_requirement=database::retrieve_attribute("Power");
	readiness_timer=database::retrieve_attribute("CycleTime");
	capacity=database::retrieve_attribute("Capacity");
	range=database::retrieve_attribute("Range");
	trck=database::retrieve_attribute("Tracking");
	acov=database::retrieve_attribute("Coverage");
	cost=database::retrieve_attribute("Cost");
}

equip* equip::equips[LIBSIZE];
