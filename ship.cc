/*
	ship.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include "calc.h"
#include "error.h"
#include "frag.h"
#include "database.h"
#include "planet.h"
#include "equip.h"
#include "server.h"
#include "alliance.h"
#include "constants.h"
#include "protocol.h"
#include "player.h"
#include "ship.h"

ship::ship(cord loc,ship* lshp,alliance* tali,int aity)
{
	self=-1;
	if(!tali)
		throw error("Null alliance given");
 	if(!lshp)
		throw error("Null ship template given");
	*this=*lshp;
	if(strlen(cls) == 0)
		throw error("Invalid ship template - missing class name");
	vel.ang=calc::rnd(360);
	vel.rad=0;
	this->loc=loc;
	this->all=tali;
	mass_locked=true;
	is_crippled=false;
	ply=NULL;
	this->aity=aity;
	update_equipment_references();
	if(get_available_cargo_space()<=0)
	{
		error::debug(cls,get_available_cargo_space());
	}
}

ship::ship()
{
	self=-1;
	ply=NULL;
	memset(cls, 0, sizeof(cls));
	memset(slots, 0, sizeof(slots));
	for(int i=0;i<32;i++)
		slots[i].pos.rad=-1;
}

ship::~ship()
{
	if(self>=0 && self<ISIZE)
		ships[self]=NULL;
	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i])
		{
			if(ships[i]->enem==this)
				ships[i]->enem=NULL;
			if(ships[i]->frnd==this)
				ships[i]->frnd=NULL;
		}
	}
	frag::notifydelete(this);
	if(ply)
		ply->notifydelete();
}

// ============================================================================
// SHIP CREATION AND MANAGEMENT
// ============================================================================

void ship::init()
{
	for(int i=0;i<ISIZE;i++)
		ships[i]=NULL;
}

void ship::loadlib()
{
	char obnm[33]; //Object name
	
	for(int i=0;i<LIBSIZE;i++)
	{
		try
		{
			lib[i]=new ship();
			try
			{
				sprintf(obnm,"ShipLib%hd",i);
				database::switchobj(obnm);
				lib[i]->load();
				lib[i]->typ=i;
			}
			catch(error it)
			{
				if(lib[i])
					delete lib[i];
				lib[i]=NULL;
			}
		}
		catch(error it)
		{
			lib[i]=NULL;
		}
	}
}

void ship::purgeall()
{
	for(int i=0;i<ISIZE;i++)
		if(ships[i])
			delete ships[i];
}

void ship::simulateall()
{
	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i])
		{
			ships[i]->physics();
			ships[i]->maintain();
		}
	}
}

void ship::behaveall()
{
	master_strobe=(master_strobe+1)%1000; //Increment the master strobe
	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i] && !ships[i]->is_crippled) //Ships that exist and are non-crippled should do behaviour
			ships[i]->execute_ai_behavior();
	}
	for(int i=0,j=0;i<10;i++)
	{
		j=(master_strobe*(i+1))%ISIZE;
		if(ships[j] && !ships[j]->is_crippled) //Non-crippled ships need masslock checking periodically
			ships[j]->mass_locked=planet::masslock(ships[j]->loc);
	}
}

void ship::saveall()
{
	char obsc[16]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i])
		{
			sprintf(obsc,"Ship%hd",i);
			database::putobject(obsc);
			ships[i]->save();
		}
	}
}

void ship::loadall()
{
	char obsc[16]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			new ship(i);
		}
		catch(error it)
		{
		}
	}
	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			if(ships[i])
			{
				sprintf(obsc,"Ship%hd",i);
				database::switchobj(obsc);
				ships[i]->resolve_object_references();
			}
		}
		catch(error it)
		{
		}
	}
}

ship* ship::get(int indx)
{
	if(indx>=0 && indx<ISIZE)
	{
		if(ships[indx])
			return ships[indx];
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

ship* ship::libget(int indx)
{
	if(indx>=0 && indx<LIBSIZE)
	{
		if(lib[indx])
			return lib[indx];
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

bool ship::has_available_ship_slot()
{
	for(int i=0;(i<(ISIZE-server::ISIZE));i++)
		if(!ships[i])
			return true;
	return false;
}

void ship::turn(int dir)
{
	if(power_plant && power_plant->cap>0)
	{
		if(dir==+1)
			vel.ang+=turn_rate;
		if(dir==-1)
			vel.ang-=turn_rate;
		if(vel.ang>=360)
			vel.ang-=360;
		if(vel.ang<0)
			vel.ang+=360;
	}
}

void ship::accel(int dir,bool wrp)
{
	int pcon; //Power consumption
	double nsp; //New speed
	
	nsp=vel.rad;
	//Handle acceleration while at warp
	if(vel.rad>99)
	{
		if(dir==+1)
			nsp=vel.rad+warp_acceleration;
		if(dir==-1)
			nsp=vel.rad-warp_acceleration;
		if(nsp<100) {
			if(wrp)
				nsp=max_impulse_speed;
			else
				nsp=100;
		}
	}
	//Handle acceleration if currently at impulse
	//if(vel.rad<=mip)
	else
	{
		if(dir==+1)
		{
			nsp=vel.rad+impulse_acceleration;
		}
		if(dir==-1)
		{
			nsp=vel.rad-impulse_acceleration;
		}
		if(nsp>max_impulse_speed)
		{
			if(wrp)
				nsp=100;
			else
				nsp=max_impulse_speed;
		}
	}

	if(nsp>max_impulse_speed && nsp<100)
		nsp=max_impulse_speed;
	if(nsp>max_warp_speed)
		nsp=max_warp_speed;

	if(nsp<-(max_impulse_speed/3)) //Prevent going faster backwards than reverse speed
		nsp=-max_impulse_speed/3;

	if(nsp<0 && !wrp && vel.rad>=0) //Prevent moving into reverse if transition not specified
		nsp=0;

	if(nsp>=100)
		pcon=(int)(((vel.rad-nsp)*mass)/2000);
	else
		pcon=(int)(((vel.rad-nsp)*mass)/2);
	if(pcon<0)
		pcon=-pcon;
	if((nsp>=100 && vel.rad<100) || (nsp<100 && vel.rad>=100))
		pcon=0;

	if(power_plant && power_plant->cap>=pcon)
	{
		power_plant->cap-=pcon;
		vel.rad=nsp;
	}
}

// ============================================================================
// SHIP COMBAT SYSTEMS
// ============================================================================

void ship::shoot(bool torp)
{
	pol ptmp;
	vect vtmp; //Temporaries for calculations
	vect corr; //Correction vector
	pol polar_to_target; //Polar to target
	double distance_to_target; //Distance to target
	vect vector_to_target; //Vector to target
	cord emission_location; //Location to emit frag at
	pol emission_polar; //Polar velocity to emit frag at
	vect emission_vector; //Vector to emit frag at
	double angle_difference; //Angle difference
	long weapon_range; //Range
	bool can; //Can shoot from this slot?
	equip* launcher_equipment; //Equipment doing the launching

	if(!see(enem))
		return;
	if(vel.rad>=100)
		return;
	weapon_range=0;
	for(int i=0;i<32;i++)
	{
		if(slots[i].item && ((torp && slots[i].item->equipment_type==equip::LAUNCHER) || (!torp && slots[i].item->equipment_type==equip::PHASER)))
		{
			launcher_equipment=slots[i].item;
			vector_to_target.xx=enem->loc.x-loc.x;
			vector_to_target.yy=enem->loc.y-loc.y;
			polar_to_target=vector_to_target.topol();
			distance_to_target=polar_to_target.rad;

			can=false;
			if(launcher_equipment->equipment_type==equip::PHASER)
			{
				weapon_range=launcher_equipment->range*launcher_equipment->trck;
				if(launcher_equipment->trck)
				{
					corr.xx=((enem->mov.xx-mov.xx)*(polar_to_target.rad/launcher_equipment->trck));
					corr.yy=((enem->mov.yy-mov.yy)*(polar_to_target.rad/launcher_equipment->trck));
					if(vector_to_target.xx && corr.xx*10/vector_to_target.xx<10)
						vector_to_target.xx+=corr.xx;
					if(vector_to_target.yy && corr.yy*10/vector_to_target.yy<10)
						vector_to_target.yy+=corr.yy;
					polar_to_target=vector_to_target.topol();
					polar_to_target.rad=distance_to_target;
				}
			}
			if(launcher_equipment->equipment_type==equip::LAUNCHER)
				weapon_range=((launcher_equipment->range*launcher_equipment->range)/2)*launcher_equipment->trck;

			if(polar_to_target.rad<=weapon_range)
				can=true;
			if(can)
			{
				angle_difference=polar_to_target.ang-(vel.ang+slots[i].face);
				if(angle_difference>180)
					angle_difference=angle_difference-360;
				if(angle_difference<-180)
					angle_difference=angle_difference+360;
				if(angle_difference>(launcher_equipment->acov) || angle_difference<-(launcher_equipment->acov))
					can=false;	
				if(angle_difference>(launcher_equipment->acov) || angle_difference<-(launcher_equipment->acov))
					can=false;		
				if(launcher_equipment->equipment_type==equip::PHASER && power_plant->cap<launcher_equipment->power_requirement)
					can=false;
				if(launcher_equipment->equipment_type==equip::LAUNCHER && !(slots[i].cap>0))
					can=false;
				if(slots[i].rdy!=0)
					can=false;
			}
			if(can)
			{
				uncloak();
				
				ptmp=slots[i].pos;
				ptmp.ang=(ptmp.ang+vel.ang);
				if(ptmp.ang>=360)
					ptmp.ang-=360;
				vtmp=ptmp.tovect();
				
				emission_location.x=vtmp.xx+loc.x;
				emission_location.y=vtmp.yy+loc.y;

				if(torp)
				{
					emission_polar.ang=slots[i].face+vel.ang; //Sort out the angle a torpedo is shot at
					if(emission_polar.ang>=360)
						emission_polar.ang-=360;
					emission_polar.rad=launcher_equipment->trck*2; //and the speed
				}
				else
				{
					emission_polar.ang=polar_to_target.ang; //velocity of phaser fire emission, towards target at fixed speed for this weapon
					emission_polar.rad=launcher_equipment->trck;
				}

				emission_vector=emission_polar.tovect();
				emission_vector.xx+=mov.xx;
				emission_vector.yy+=mov.yy;

				if(launcher_equipment->equipment_type==equip::PHASER)
				{
					power_plant->cap-=launcher_equipment->power_requirement;
					try
					{
						new frag(emission_location,frag::ENERGY,launcher_equipment->sprite_index,launcher_equipment->color_index,enem,this,emission_vector,((polar_to_target.ang+5)/10),launcher_equipment->power_requirement,0,launcher_equipment->range);
					}
					catch(error it)
					{
					}
				}
				if(launcher_equipment->equipment_type==equip::LAUNCHER)
				{
					slots[i].cap--;
					try
					{
						new frag(emission_location,frag::HOMER,launcher_equipment->sprite_index,launcher_equipment->color_index,enem,this,emission_vector,0,launcher_equipment->power_requirement,launcher_equipment->trck,launcher_equipment->range);
					}
					catch(error it)
					{
					}
				}
				server::registernoise(this,launcher_equipment->sound_index);
				slots[i].rdy=launcher_equipment->readiness_timer;
				if(torp)
					break;
			}
		}
	}
}

bool ship::see(ship* target_ship)
{
	double weapon_range; //Effective range

	if(!target_ship) //Null ship
		return false;
	if(target_ship==this) //Can always see self
		return true;
	if(sensor_array) //Set the sensor range, or default if no sensor suite
		weapon_range=sensor_array->item->range;
	else
		weapon_range=1000;
	if(target_ship->vel.rad<20) //Slower ships less visible
		weapon_range-=(((weapon_range/2)*(20-target_ship->vel.rad))/20);
	if(target_ship->cloaking_device && target_ship->cloaking_device->cap==target_ship->cloaking_device->item->capacity) //Cloaked ships even less visible
		weapon_range/=8;
	if((target_ship->loc.x-loc.x)>weapon_range) //Bounds checking
		return false;
	if((target_ship->loc.x-loc.x)<-weapon_range)
		return false;
	if((target_ship->loc.y-loc.y)>weapon_range)
		return false;
	if((target_ship->loc.y-loc.y)<-weapon_range)
		return false;
	return true;
}

bool ship::see(planet* target_planet)
{
	double weapon_range; //Effective range

	if(!target_planet) //Null planet
		return false;
	if(sensor_array) //Set the sensor range, or default if no sensor suite
		weapon_range=sensor_array->item->range;
	else
		weapon_range=1000;
	if(target_planet->typ==planet::STAR) //Can always see stars
		return true;
	if((target_planet->loc.x-loc.x)>weapon_range) //Bounds checking
		return false;
	if((target_planet->loc.x-loc.x)<-weapon_range)
		return false;
	if((target_planet->loc.y-loc.y)>weapon_range)
		return false;
	if((target_planet->loc.y-loc.y)<-weapon_range)
		return false;
	return true;		
}

bool ship::see(frag* tfrg)
{
	double weapon_range; //Effective range

	if(!tfrg) //Null frag
		return false;
	if(sensor_array) //Set the sensor range, or default if no sensor suite
		weapon_range=sensor_array->item->range;
	else
		weapon_range=1000;
	if(tfrg->trg==this || tfrg->own==this) //For bandwidth spamming reasons, only see frags when really close unless they concern you
	{
		if((tfrg->loc.x-loc.x)>weapon_range)
			return false;
		if((tfrg->loc.x-loc.x)<-weapon_range)
			return false;
		if((tfrg->loc.y-loc.y)>weapon_range)
			return false;
		if((tfrg->loc.y-loc.y)<-weapon_range)
			return false;
	}
	else
	{
		if((tfrg->loc.x-loc.x)>500)
			return false;
		if((tfrg->loc.x-loc.x)<-500)
			return false;
		if((tfrg->loc.y-loc.y)>500)
			return false;
		if((tfrg->loc.y-loc.y)<-500)
			return false;
	}
	return true;		
}

int ship::interact(char* txt,short cmod,short opr,ship* player_ship)
{
	char spd[32]; //Speed

	if(!(player_ship && player_ship->ply))
		return -1;
	switch(cmod)
	{
		case CMOD_STAT:
		case CMOD_SCAN:
		if(opr==-1)
		{
			if(player_ship->see(this))
			{
				txt+=sprintf(txt,"%s\n",cls);
				if(player_ship->all->opposes(all))
					txt+=sprintf(txt,"Alignment:%s [hostile]\n",all->nam);
				else
					txt+=sprintf(txt,"Alignment:%s\n",all->nam);
				if(ply)
					txt+=sprintf(txt,"Commanded by %s\n",ply->nam);
				if(shield_generator && shield_generator->cap>0)
					txt+=sprintf(txt,"\nShields: Raised\n");
				else
					txt+=sprintf(txt,"\nShields: Down\n");
				calc::getspeed(max_warp_speed,spd);
				txt+=sprintf(txt,"Maximum velocity: %s\n",spd);
				if(shield_generator)
					txt+=sprintf(txt,"Shield capability: %ld\n",shield_generator->item->capacity);
				else
					txt+=sprintf(txt,"No shields");
				if(power_plant)
					txt+=sprintf(txt,"Maximum power capacity: %ld\n",power_plant->item->capacity);
				else
					txt+=sprintf(txt,"No power plant");
				if(fuel_tank)
					txt+=sprintf(txt,"Maximum fuel storage: %ld\n",fuel_tank->item->capacity);
				else
					txt+=sprintf(txt,"No fuel storage");

				txt+=sprintf(txt,"\nAvailable mass: %hd\n",get_available_cargo_space());

				if(this==player_ship)
				{
//					txt+=sprintf(txt,"\nAvailable mass: %hd\n",get_available_cargo_space());
					txt+=sprintf(txt,"\nCredits: %ld\n",ply->cashi);
				}
				if(this==player_ship->enem)
					txt+=sprintf(txt,"\n[1] Lay in an intercept course\n");
				return spr;
			}
			else
			{
				txt+=sprintf(txt,"Target not visible\n");
				if(this==player_ship->enem)
					txt+=sprintf(txt,"\n[1] Lay in an intercept course\n");
				return -1;
			}
		}
		if(opr==1 && this==player_ship->enem)
		{
			player_ship->aity=AI_AUTOPILOT;
		}
		break;

		
		case CMOD_EQUIP:
		if(!(selected_equipment_index>=0 && selected_equipment_index<32 && slots[selected_equipment_index].item))
		{
			selected_equipment_index=-1;
			for(int i=0;i<32;i++)
			{
				if(slots[i].item)
				{
					selected_equipment_index=i;
					break;
				}
			}
		}
		if(opr==-1)
		{
			txt+=sprintf(txt,"Internal systems\n\n");
			for(int i=0;i<32;i++)
			{
				if(slots[i].item)
				{
					if(slots[i].item->equipment_type==equip::LAUNCHER)
						if(i==selected_equipment_index)
							txt+=sprintf(txt,">%s [%ld]<\n",slots[i].item->nam,slots[i].cap);
						else
							txt+=sprintf(txt," %s [%ld]\n",slots[i].item->nam,slots[i].cap);
					else if(slots[i].item->equipment_type==equip::FUELTANK && slots[i].cap==0)
						if(i==selected_equipment_index)
							txt+=sprintf(txt,">%s< [empty]\n",slots[i].item->nam);
						else
							txt+=sprintf(txt," %s [empty]\n",slots[i].item->nam);
					else
						if(i==selected_equipment_index)
							txt+=sprintf(txt,">%s<\n",slots[i].item->nam);
						else
							txt+=sprintf(txt," %s\n",slots[i].item->nam);
				}
				else
				{
					if(slots[i].pos.rad>=0)
					{
						if(slots[i].face<=90 || slots[i].face>=270)
							txt+=sprintf(txt," <Free forward port>\n");
						else
							txt+=sprintf(txt," <Free rear port>\n");
					}
				}
			}
			txt+=sprintf(txt,"\n");
			if(shield_generator)
				txt+=sprintf(txt,"[1] Toggle shields\n");
			if(cloaking_device)
				txt+=sprintf(txt,"[2] Toggle cloak\n");
			txt+=sprintf(txt,"\n[3] Select equipment\n");
			txt+=sprintf(txt,"[4] Jettison selection\n");
		}
		if(opr==1) {
			if(shield_generator && shield_generator->rdy==-1)
				shieldsup();
			else
				shieldsdown();
		}
		if(opr==2) {
			if(cloaking_device && cloaking_device->rdy==-1)
				cloak();
			else
				uncloak();
		}
		if(opr==3)
		{
			for(int i=0,j=selected_equipment_index+1;i<32;i++,j++)
			{
				if(j>=32)
					j=0;
				if(slots[j].item)
				{
					selected_equipment_index=j;
					break;
				}
			}
		}
		if(opr==4)
		{
			if(selected_equipment_index>=0 && selected_equipment_index<32 && slots[selected_equipment_index].item)
			{
				if(slots[selected_equipment_index].item->equipment_type==equip::TRANSPORTER)
				{
					sprintf(txt,"Cannot jettison transporters");
				}
				else
				{
					sprintf(txt,"%s jettisoned",slots[selected_equipment_index].item->nam);
					slots[selected_equipment_index].item=NULL;
					slots[selected_equipment_index].rdy=0;
					slots[selected_equipment_index].cap=0;
					update_equipment_references();
				}
			}
		}
		break;

		case CMOD_HAIL:
		if(is_crippled)
		{
			if(opr==-1)
			{
				txt+=sprintf(txt,"Hailing ship\n\n");
				txt+=sprintf(txt,"Vessel is disabled\n\n[1] Attempt to recover it");
			}
			if(opr==1)
			{
				player_ship->transport(this);
				enem=NULL;
				plnt=NULL;
				frnd=player_ship;
				for(int i=0;i<ISIZE;i++)
					if(ships[i] && ships[i]->enem==this)
						ships[i]->enem=NULL;
				txt+=sprintf(txt,"Vessel successfully acquired");
				player_ship->ply->transfer(this);
			}
		}
		else
		{
			if(frnd==player_ship)
			{
				if(opr==-1)
				{
					txt+=sprintf(txt,"Hailing ship\n\n");
					txt+=sprintf(txt,"Vessel is under your command\n\n[1] Transfer to this vessel");
				}
				if(opr==1)
				{
					try
					{
						player_ship->transport(this);	
						player_ship->ply->transfer(this);
						txt+=sprintf(txt,"Transfer of command successful");
					}
					catch(error it)
					{
						try
						{
							transport(player_ship);
							player_ship->ply->transfer(this);
						}
						catch(error iti)
						{
							throw it;
						}
					}
				}
			}
			else
			{
				txt+=sprintf(txt,"Hailing ship\n\n");
				txt+=sprintf(txt,"No reply");
			}
		}
		break;
		
		case CMOD_WHOIS:
		if(ply)
		{
			txt+=sprintf(txt,"Player: %s\n",ply->nam);
			txt+=sprintf(txt,"Alliance: %s\n",all->nam);
			return spr;
		}
		else
			txt+=sprintf(txt,"Target not player controlled\n");
		break;
	}
	return -1;
}

int ship::get_available_cargo_space()
{
	int out; //Outputted free space

	out=mass;
	for(int i=0;i<32;i++)
	{
		if(slots[i].item)
			out-=slots[i].item->mass;
	}
	return out;
}

void ship::cloak()
{
	if(cloaking_device && cloaking_device->rdy!=0)
	{
		cloaking_device->rdy=0;
		server::registernoise(this,cloaking_device->item->sound_index);
	}
}

void ship::uncloak()
{
	if(cloaking_device && cloaking_device->rdy!=-1)
	{
		cloaking_device->rdy=-1;
		if(cloaking_device->cap>0)
			cloaking_device->cap=-cloaking_device->cap;
		server::registernoise(this,cloaking_device->item->sound_index);
	}
}

void ship::shieldsup()
{
	if(shield_generator)
		shield_generator->rdy=0;
}

void ship::shieldsdown()
{
	if(shield_generator)
		shield_generator->rdy=-1;
}

void ship::serialize_to_network(int typ,unsigned char* buf)
{
	buf[0]=typ;
	buf+=1;

	calc::inttodat(ship2pres(self),buf);
	buf+=2;
	switch(typ)
	{
		case SERV_SELF:
		if(mass>0)
			calc::inttodat((100*hull_integrity)/max_hull_integrity,buf);
		else
			calc::inttodat(0,buf);
		buf+=2;

		if(power_plant && power_plant->cap>0)
			calc::inttodat((100*power_plant->cap)/(power_plant->item->capacity),buf);
		else
			calc::inttodat(0,buf);
		buf+=2;

		if(shield_generator && shield_generator->cap>0)
			calc::inttodat((100*shield_generator->cap)/(shield_generator->item->capacity),buf);
		else
			calc::inttodat(0,buf);
		buf+=2;

		if(fuel_tank && fuel_tank->cap>0)
			calc::inttodat((100*fuel_tank->cap)/(fuel_tank->item->capacity),buf);
		else
			calc::inttodat(0,buf);
		buf+=2;
		if(sensor_array)
			calc::longtodat(sensor_array->item->range,buf);
		else
			calc::longtodat(0,buf);
		buf+=4;
		calc::longtodat(LIMIT,buf);
		buf+=4;
		if(plnt)
		{
			calc::inttodat(planet2pres(plnt->self),buf);
		}
		else
		{
			if(enem)
				calc::inttodat(ship2pres(enem->self),buf);
			else
				calc::inttodat(-1,buf);
		}
		buf+=2;
		calc::inttodat(-1,buf);
		buf+=2;
		calc::inttodat(-1,buf);
		break;

		case SERV_NEW:
		*buf=PT_SHIP;
		buf+=1;
		calc::inttodat(spr,buf);
		buf+=2;
		calc::inttodat(-1,buf);
		buf+=2;
		break;

		case SERV_NAME:
		sprintf((char*)buf,"%s",cls);
		buf+=64;
		sprintf((char*)buf,"%s",all->nam);
		buf+=64;
		break;

		case SERV_UPD:
		calc::longtodat(loc.x,buf);
		buf+=4;
		calc::longtodat(loc.y,buf);
		buf+=4;
		calc::longtodat(mov.xx,buf);
		buf+=4;
		calc::longtodat(mov.yy,buf);
		buf+=4;
		calc::inttodat(vel.ang,buf);
		buf+=2;
		*buf=0;
		buf+=1;
		if(cloaking_device && cloaking_device->item->capacity)
		{
			if(cloaking_device->cap>=0)
				*buf=100-((100*cloaking_device->cap)/cloaking_device->item->capacity);
			else
				*buf=100+((100*cloaking_device->cap)/cloaking_device->item->capacity);
		}
		else
			*buf=100;
		buf+=1;
		break;

		default:
		break;
	}
}

bool ship::detect_collision(cord fragment_location,vect fragment_velocity)
{
	int rot; //Target rotation
	double x1,y1,x2,y2,xx,yy; //Target bounding box

	rot=(int)(((vel.ang+5)/10))%36;
	xx=(fragment_velocity.xx-mov.xx)/2;
	yy=(fragment_velocity.yy-mov.yy)/2;
	if(xx<0)
		xx=-xx;
	if(yy<0)
		yy=-yy;
	x1=loc.x-(w[rot]*3)/2-xx;
	y1=loc.y-(h[rot]*3)/2-yy;
	x2=loc.x+(w[rot]*3)/2+xx;
	y2=loc.y+(h[rot]*3)/2+yy;
	if(fragment_location.x>x1 && fragment_location.x<x2 && fragment_location.y>y1 && fragment_location.y<y2)
		return true;
	else
		return false;
}

void ship::hit(int mag,cord fragment_location,vect fragment_velocity,ship* src)
{
	cord tmpc;
	vect tmpv; //Temporary for working out detonations
	int rot; //Target rotation
	int debris_count; //Number of debris bits

	uncloak();
	rot=(int)(((vel.ang+5)/10))%36;
	if(shield_generator)
		shield_generator->cap-=mag;
	server::registershake(this,mag/100);
	if(src && enem!=src && !(all->opposes(src->all)) && src->ply)
		src->alert_nearby_ships();
	if(shield_generator && shield_generator->cap>0)
	{
		try
		{
			new frag(fragment_location,frag::DEBRIS,shield_generator->item->sprite_index,-1,NULL,this,mov,calc::rnd(36),0,0,2);
		}
		catch(error it)
		{
		}
	}
	else
	{
		if(shield_generator)
			shield_generator->cap=0;
		fragment_location.x=(fragment_location.x+2*loc.x)/3;
		fragment_location.y=(fragment_location.y+2*loc.y)/3;
		for(int i=0;i<5;i++)
		{
			tmpv=mov;
			fragment_location.x+=calc::rnd(2)-calc::rnd(2);
			fragment_location.y+=calc::rnd(2)-calc::rnd(2);
			tmpv.xx+=calc::rnd(2)-calc::rnd(2);
			tmpv.yy+=calc::rnd(2)-calc::rnd(2);
			try
			{
				new frag(fragment_location,frag::DEBRIS,frag::FIRE,-1,NULL,this,tmpv,calc::rnd(36),0,0,calc::rnd(5)+5);
			}
			catch(error it)
			{
			}
		}
		server::registernoise(this,fsnd);
	}

	if(shield_generator && shield_generator->cap!=-10)
		hull_integrity-=(mag*4)/(shield_generator->cap+10);
	else
		hull_integrity-=(mag*4)/10;

	if(hull_integrity<=0)
	{
		hull_integrity=0;
		debris_count=mass/8+4;
		if(debris_count>70)
			debris_count=70;
		for(int i=0;i<debris_count;i++)
		{	
			if(i==0 || calc::rnd(5)==0)
			{
				tmpc=loc;
				tmpc.x+=calc::rnd(2*w[rot])-calc::rnd(2*w[rot]);
				tmpc.y+=calc::rnd(2*h[rot])-calc::rnd(2*h[rot]);
			}
			tmpv=mov;
			try
			{
				if(calc::rnd(10)<3)
				{
					new frag(tmpc,frag::DEBRIS,frag::FIRE,-1,NULL,this,tmpv,calc::rnd(36),0,0,calc::rnd(20)+5);
				}
				else
				{
					tmpv.xx+=calc::rnd(2)-calc::rnd(2);
					tmpv.yy+=calc::rnd(2)-calc::rnd(2);
					new frag(tmpc,frag::DEBRIS,fspr,-1,NULL,this,tmpv,calc::rnd(36),0,0,calc::rnd(20)+5);
				}
			}
			catch(error it)
			{
			}
		}
		if(src && src->ply && src->all->opposes(all))
		{
			server::hail(NULL,src->ply,"Target destroyed; bounty paid");
			src->ply->credit(mass/2);
		}
		server::registernoise(this,dsnd);
		if(ply)
			server::bulletin("%s has been destroyed",ply->nam);
		delete this;
	}
	else
	{
		if(hull_integrity<max_hull_integrity/2 && src && src->ply && !ply && !is_crippled)
		{
			server::hail(NULL,src->ply,"Target crippled");
			is_crippled=true;
		}
	}
}

void ship::assign(player* ply)
{
	this->ply=ply;
	enem=NULL;
	frnd=NULL;
	is_crippled=false;
	aity=AI_NULL;
	update_equipment_references();
}

long ship::purchase(int prch,short ripo,bool buy)
{
	long cost; //Value to output

	cost=0;
	if(prch==PRCH_FUEL)
	{
		if(fuel_tank)
		{
			if((fuel_tank->cap)<(fuel_tank->item->capacity))
				cost=ripo/4;
			if(buy)
			{
				fuel_tank->cap=fuel_tank->item->capacity;
				ply->debit(cost);
			}
		}
	}
	if(prch==PRCH_ARMS)
	{
		for(int i=0;i<32;i++)
		{
			if(slots[i].item && slots[i].item->equipment_type==equip::LAUNCHER)
			{
				if(slots[i].cap<slots[i].item->capacity)
				{
					cost=(slots[i].item->cost*ripo)/1000;
					if(buy)
					{
						ply->debit(cost);
						slots[i].cap=slots[i].item->capacity;
					}
					break;
				}
			}
		}
	}
	if(prch==PRCH_HULL)
	{
		cost=((max_hull_integrity-hull_integrity)*ripo)/100;
		if(buy)
		{
			ply->debit(cost);
			hull_integrity=max_hull_integrity;
		}
	}
	return cost;
}

long ship::purchase(equip* prch,int ripo,bool buy)
{
	long cost; //Value to output

	if(!prch)
		return 0;
	cost=(prch->cost*ripo)/100;
	if(buy && !(prch->mass>get_available_cargo_space()))
	{
		for(int i=0;i<32;i++)
		{
			if(!slots[i].item)
			{
				if((slots[i].pos.rad>=0 && (prch->equipment_type==equip::PHASER || prch->equipment_type==equip::LAUNCHER)) || (slots[i].pos.rad==-1 && prch->equipment_type!=equip::PHASER && prch->equipment_type!=equip::LAUNCHER))
				{
					ply->debit(cost);
					slots[i].item=prch;
					slots[i].cap=prch->capacity;
					slots[i].rdy=prch->readiness_timer;
					update_equipment_references();
					break;
				}
			}
		}
	}
	return cost;
}

void ship::transport(planet* to)
{
	vect vto;
	pol pto; //Vectors to the target

	vto.xx=to->loc.x-loc.x;
	vto.yy=to->loc.y-loc.y;
	pto=vto.topol();
	if(shield_generator && shield_generator->cap>0)
		throw error("Cannot transport with shields up");
	if(!power_plant)
		throw error("Not enough power to transport");
	if(cloaking_device && cloaking_device->cap!=0)
		throw error("Cannot transport while cloaked");
	for(int i=0;i<32;i++)
	{
		if(slots[i].item && slots[i].item->equipment_type==equip::TRANSPORTER && slots[i].rdy==0 && power_plant->cap>=slots[i].item->power_requirement && slots[i].item->range>=pto.rad)
		{
			power_plant->cap-=slots[i].item->power_requirement;
			slots[i].rdy=slots[i].item->readiness_timer;
			server::registersound(this,slots[i].item->sound_index);
			return;
		}
	}
	throw error("No available transporters ready or powered");
}

void ship::transport(ship* to)
{
	vect vto;
	pol pto; //Vectors to the target

	vto.xx=to->loc.x-loc.x;
	vto.yy=to->loc.y-loc.y;
	pto=vto.topol();
	if(shield_generator && shield_generator->cap>0)
		throw error("Cannot transport with shields up");
	if(!power_plant)
		throw error("Not enough power to transport");
	if(cloaking_device && cloaking_device->cap!=0)
		throw error("Cannot transport while cloaked");
	if(to->shield_generator && to->shield_generator->cap>0)
		throw error("Cannot transport through destination's shields");
	if(to->cloaking_device && to->cloaking_device->cap!=0)
		throw error("Cannot transport through destination's cloak");
	for(int i=0;i<32;i++)
	{
		if(slots[i].item && slots[i].item->equipment_type==equip::TRANSPORTER && slots[i].rdy==0 && power_plant->cap>=slots[i].item->power_requirement && slots[i].item->range>=pto.rad)
		{
			power_plant->cap-=slots[i].item->power_requirement;
			slots[i].rdy=slots[i].item->readiness_timer;
			server::registersound(this,slots[i].item->sound_index);
			return;
		}
	}
	throw error("No available transporters ready or powered");
}

void ship::save()
{
	char atsc[33]; //Attribute scratchpad

	database::putvalue("Class",cls);
	database::putvalue("Type",typ);
	database::putvalue("ShipSprite",spr);
	database::putvalue("Width",w[0]);
	database::putvalue("Height",h[0]);
	if(fspr)
		database::putvalue("FragSprite",fspr);
	if(fsnd)
		database::putvalue("FragSound",fsnd);
	if(dsnd)
		database::putvalue("DeathSound",dsnd);
	if(all)
		database::putvalue("Team",all->self);
	database::putvalue("AIType",aity);
	database::putvalue("XLoc",loc.x);
	database::putvalue("YLoc",loc.y);
	database::putvalue("Heading",vel.ang);
	database::putvalue("Speed",vel.rad);
	database::putvalue("TurnRate",turn_rate);
	database::putvalue("SublightLimit",max_impulse_speed);
	database::putvalue("SublightAcceleration",impulse_acceleration*10);
	database::putvalue("WarpLimit",max_warp_speed);
	database::putvalue("WarpAcceleration",warp_acceleration);
	database::putvalue("Mass",mass);
	database::putvalue("HullStrength",hull_integrity);
	database::putvalue("HullStrengthLimit",max_hull_integrity);
	if(frnd)
		database::putvalue("FriendTarget",frnd->self);
	if(enem)
		database::putvalue("EnemyTarget",enem->self);
	if(plnt)
		database::putvalue("PlanetTarget",plnt->self);
	database::putvalue("MassLock",mass_locked);
	database::putvalue("Crippled",is_crippled);
	for(int i=0;i<32;i++)
	{
		if(slots[i].item || slots[i].pos.rad!=-1)
		{
			sprintf(atsc,"Slot%hdAngle",i);
			database::putvalue(atsc,slots[i].pos.ang);
			sprintf(atsc,"Slot%hdRadius",i);
			database::putvalue(atsc,slots[i].pos.rad);
			sprintf(atsc,"Slot%hdFace",i);
			database::putvalue(atsc,slots[i].face);
			sprintf(atsc,"Slot%hdItem",i);
			if(slots[i].item)
				database::putvalue(atsc,slots[i].item->self);
			else
				database::putvalue(atsc,-1);
			sprintf(atsc,"Slot%hdReadiness",i);
			database::putvalue(atsc,slots[i].rdy);
			sprintf(atsc,"Slot%hdCapacity",i);
			database::putvalue(atsc,slots[i].cap);
		}
	}
}

void ship::load()
{
	char atsc[33]; //Attribute scratchpad
	pol bpol;
	vect vct1,vct2; //Temporaries for calculating the bounding box

	database::getvalue("Class",cls);
	typ=database::getvalue("Type");
	spr=database::getvalue("ShipSprite");
	w[0]=database::getvalue("Width");
	h[0]=database::getvalue("Height");
	for(int i=1;i<36;i++)
	{
		bpol.ang=i*10;
		bpol.rad=h[0];
		vct1=bpol.tovect();
		bpol.ang=(i*10+90)%360;
		bpol.rad=w[0];
		vct2=bpol.tovect();
		if(vct1.xx<0)
			vct1.xx=-vct1.xx;
		if(vct2.xx<0)
			vct2.xx=-vct2.xx;
		if(vct1.yy<0)
			vct1.yy=-vct1.yy;
		if(vct2.yy<0)
			vct2.yy=-vct2.yy;
		if(vct1.xx>vct2.xx)
			w[i]=(int)vct1.xx;
		else
			w[i]=(int)vct2.xx;
		if(vct1.yy>vct2.yy)
			h[i]=(int)vct1.yy;
		else
			h[i]=(int)vct2.yy;
	}
	fspr=database::getvalue("FragSprite");
	fsnd=database::getvalue("FragSound");
	dsnd=database::getvalue("DeathSound");
	all=alliance::get(database::getvalue("Team"));
	aity=database::getvalue("AIType");
	loc.x=database::getvalue("XLoc");
	loc.y=database::getvalue("YLoc");
	vel.ang=database::getvalue("Heading");
	vel.rad=database::getvalue("Speed");
	turn_rate=database::getvalue("TurnRate");
	max_impulse_speed=database::getvalue("SublightLimit");
	impulse_acceleration=(double)database::getvalue("SublightAcceleration")/10;
	max_warp_speed=database::getvalue("WarpLimit");
	warp_acceleration=database::getvalue("WarpAcceleration");
	mass=database::getvalue("Mass");
	hull_integrity=database::getvalue("HullStrength");
	max_hull_integrity=database::getvalue("HullStrengthLimit");
	
	mass_locked=database::getvalue("MassLock");
	is_crippled=database::getvalue("Crippled");

	selected_equipment_index=-1;
	for(int i=0;i<32;i++)
	{
		// Initialize slot to safe defaults
		slots[i].pos.ang=0;
		slots[i].pos.rad=-1;
		slots[i].face=0;
		slots[i].item=NULL;
		slots[i].rdy=0;
		slots[i].cap=0;
		
		try {
			sprintf(atsc,"Slot%hdAngle",i);
			slots[i].pos.ang=database::getvalue(atsc);
			sprintf(atsc,"Slot%hdRadius",i);
			slots[i].pos.rad=database::getvalue(atsc);
			sprintf(atsc,"Slot%hdFace",i);
			slots[i].face=database::getvalue(atsc);
			if(slots[i].face==-1)
				slots[i].face=slots[i].pos.ang;
			sprintf(atsc,"Slot%hdItem",i);
			long item_id = database::getvalue(atsc);
			if(item_id >= 0) {
				slots[i].item=equip::get(item_id);
			}
			sprintf(atsc,"Slot%hdReadiness",i);
			slots[i].rdy=database::getvalue(atsc);
			sprintf(atsc,"Slot%hdCapacity",i);
			slots[i].cap=database::getvalue(atsc);
			if(slots[i].cap==-1 && slots[i].item)
				slots[i].cap=slots[i].item->capacity;
		} catch(...) {
			// If loading fails, leave slot in safe default state
		}
	}

	if(hull_integrity==-1)
		hull_integrity=max_hull_integrity;
	ply=NULL;

	mov.xx=0;
	mov.yy=0;

	frnd=NULL;
	enem=NULL;
	plnt=NULL;

	if(aity==-1)
		aity=AI_NULL;

	update_equipment_references();
}

void ship::insert()
{
	self=-1;
	if(ply)
	{
		for(int i=0;i<ISIZE && self==-1;i++)
			if(!ships[i])
				self=i;
	}
	else
	{
		for(int i=0;(i<(ISIZE-server::ISIZE)) && self==-1;i++)
			if(!ships[i])
				self=i;
	}
	if(self==-1)
		throw error("No free ship index available");
	else
		ships[self]=this;
}

void ship::insert(int self)
{
	this->self=-1;
	if(!(self>=0 && self<ISIZE))
		return;
	this->self=self;
	if(ships[self])
		delete ships[self];
	ships[self]=this;
}

ship::ship(int self)
{
	char obsc[16]; //Object name scratchpad

	sprintf(obsc,"Ship%hd",self);
	database::switchobj(obsc);
	load();
	insert(self);
}

// ============================================================================
// SHIP PHYSICS AND MOVEMENT
// ============================================================================

void ship::physics()
{
	vect nmov; //New movement vector

	//Slow down vessels at warp under masslock influence
	if(vel.rad>=100 && mass_locked)
		vel.rad=max_impulse_speed;
	//Handle ships in between warp 1 and maximum impulse
	if(vel.rad<100 && vel.rad>max_impulse_speed)
		vel.rad=max_impulse_speed;

	//Handle ships going beyond the boundaries of the 'universe'; they bounce
	if(loc.x>LIMIT || loc.x<-LIMIT || loc.y>LIMIT || loc.y<-LIMIT)
	{
		vel.ang=(vel.ang+180);
		if(vel.ang>=360)
			vel.ang-=360;
	}
	if(loc.x>LIMIT)
		loc.x=LIMIT;
	if(loc.x<-LIMIT)
		loc.x=-LIMIT;
	if(loc.y>LIMIT)
		loc.y=LIMIT;
	if(loc.y<-LIMIT)
		loc.y=-LIMIT;
	
	nmov=vel.tovect();
	if(vel.rad<100 && (mass/100)!=0)
	{
		mov.xx+=(nmov.xx-mov.xx)/(mass/100);
		mov.yy+=(nmov.yy-mov.yy)/(mass/100);
		//loc.x+=(nmov.xx-mov.xx);
		//loc.y+=(nmov.yy-mov.yy);
	}
	else
	{
		mov=nmov;
	}

	loc.x+=mov.xx;
	loc.y+=mov.yy;
}


void ship::navigate_to_planet(planet* target_planet)
{
	vect vector_to_target; //Vector to target
	pol polar_to_target; //Polar to target
	double dd; //Directional difference
	double tol; //Angular tolerance

	vector_to_target.xx=(self*497)%800-400+target_planet->loc.x-loc.x;
	vector_to_target.yy=(self*273)%800-400+target_planet->loc.y-loc.y; //Vector to deterministic but arbitrary location near target planet

	polar_to_target=vector_to_target.topol(); //...make polar

	dd=polar_to_target.ang-vel.ang;
	if(dd>180)
		dd=dd-360;
	if(dd<-180)
		dd=dd+360; //Evaluate angle between current heading and target bearing

	tol=turn_rate+2;
	if(vel.rad<=5) 
		tol=20; //Low speed; not too fussed about fine direction finding

	polar_to_target.rad-=150; //Stand off distance

	if(polar_to_target.rad<0) //Don't turn when too close
		dd=0;

	if(dd<tol && dd>-tol) //Don't turn when within angle tolerance
		dd=0;

	if(dd==0 && polar_to_target.rad>0) //Only accelerate when heading at target
	{
		if(vel.rad>=100)
		{
			if(vel.rad<sqrt(2*warp_acceleration*polar_to_target.rad)-warp_acceleration)
				accel(+1,true);
			else if(vel.rad>sqrt(2*warp_acceleration*polar_to_target.rad)+warp_acceleration)
				accel(-1,true);
		}
		else
		{
			if(vel.rad<(sqrt(2*impulse_acceleration*polar_to_target.rad)-impulse_acceleration))  //Intended speed is sqrt(2as)
				accel(+1,true);
			else if(vel.rad>(sqrt(3*impulse_acceleration*polar_to_target.rad))+impulse_acceleration)
				accel(-1,true);
		}
	}
	else
		accel(-1,false);
	if(dd>0)
		turn(+1);
	if(dd<0)
		turn(-1);
}

void ship::follow(ship* target_ship)
{
	vect vector_to_target; //Vector to target
	pol polar_to_target; //Polar to target
	double dd; //Directional difference
	double tol; //Angular tolerance

	polar_to_target.ang=target_ship->vel.ang+90+(self*29)%180; //Find deterministic formation angle to hold at around target ship
	polar_to_target.rad=100+(self*17)%((sensor_array ? sensor_array->item->range : 1000)/16); //Deterministic range to hold based on sensor range
	vector_to_target=polar_to_target.tovect();
		
	vector_to_target.xx+=target_ship->loc.x-loc.x;
	vector_to_target.yy+=target_ship->loc.y-loc.y;
	polar_to_target=vector_to_target.topol(); //Get polar vector to this formation position

	dd=polar_to_target.ang-vel.ang;
	if(dd>180)
		dd=dd-360;
	if(dd<-180)
		dd=dd+360; //Evaluate angle between current heading and target bearing

	if(!see(target_ship))
		polar_to_target.rad-=(sensor_array ? sensor_array->item->range : 1000)/3; //If you can't see the target, stand off a little

	tol=turn_rate*2+2;
	if(vel.rad<=5) 
		tol=20; //Low speed; not too fussed about fine direction finding

	polar_to_target.rad-=150; //Default stand off

	if(polar_to_target.rad<0) //Don't turn when too close
		dd=0;

	if(dd<tol && dd>-tol) //Don't turn when within angle tolerance
		dd=0;

	if(dd==0 && polar_to_target.rad>0) //Only accelerate when heading at target
	{
		if(vel.rad>=100)
		{
			if(vel.rad<sqrt(2*warp_acceleration*polar_to_target.rad)-warp_acceleration)
				accel(+1,true);
			else if(vel.rad>sqrt(2*warp_acceleration*polar_to_target.rad)+warp_acceleration)
				accel(-1,true);
		}
		else
		{
			if(vel.rad<(sqrt(2*impulse_acceleration*polar_to_target.rad)-2*impulse_acceleration))  //Intended speed is sqrt(2as)
				accel(+1,true);
			else if(vel.rad>(sqrt(2*impulse_acceleration*polar_to_target.rad)+2*impulse_acceleration))
				accel(-1,true);
		}
	}
	else
		accel(-1,false);

	if(dd>0)
		turn(+1);
	if(dd<0)
		turn(-1);
}

void ship::execute_attack_maneuvers(ship* target_ship,int str)
{
	vect vector_to_target; //Vector to target
	pol polar_to_target; //Polar to target
	double dd; //Directional difference
	double tol; //Angular tolerance

	if(!see(target_ship) || vel.rad>=100) //If you can't see or are warp pursuing the target ship, default to the follow method
	{
		follow(target_ship);
		return;
	}
	if(str>(200+calc::rnd((self%7)*12)-50)) //Alternate on tailing target from one of two sides
		polar_to_target.ang=target_ship->vel.ang+45+(str-self*29)%135;
	else
		polar_to_target.ang=target_ship->vel.ang-45-(str+self*29)%135;
	polar_to_target.rad=100+(self*17)%((str+(sensor_array ? sensor_array->item->range : 1000))/16); //Back off a little depending on sensor range
	vector_to_target=polar_to_target.tovect();
		
	vector_to_target.xx+=target_ship->loc.x-loc.x;
	vector_to_target.yy+=target_ship->loc.y-loc.y;
	polar_to_target=vector_to_target.topol(); //And finally get a polar to the 'formation' position

	dd=polar_to_target.ang-vel.ang;
	if(dd>180)
		dd=dd-360;
	if(dd<-180)
		dd=dd+360; //Evaluate angle between current heading and target bearing

	if(!see(target_ship))
		polar_to_target.rad-=(sensor_array ? sensor_array->item->range : 1000)/3; //If you can't see the target, stand off a little
	tol=turn_rate*2+2;

	if(vel.rad<=5) 
		tol=20; //Low speed; not too fussed about fine direction finding

	tol+=45; //Widen angle tolerance for close combat flair

	if(polar_to_target.rad<0) //Don't turn when too close
		dd=0;

	if(dd<tol && dd>-tol) //Don't turn when within angle tolerance
		dd=0;

	if(dd==0 && polar_to_target.rad>0) //Only accelerate when heading at target
	{
		if(vel.rad>=100)
		{
			if(polar_to_target.rad && (polar_to_target.rad/vel.rad) && ((vel.rad)/(12*polar_to_target.rad/vel.rad))>=warp_acceleration-30)
				accel(-1,true);
			else
				accel(+1,true);
		}
		else
		{
			if(polar_to_target.rad && ((5*vel.rad*vel.rad)/(polar_to_target.rad))>=impulse_acceleration)
				accel(-1,true);
			else
				accel(+1,true);
		}
	}
	else
		accel(-1,false);
	if(dd>0)
		turn(+1);
	if(dd<0)
		turn(-1);
}

void ship::resolve_object_references()
{
	frnd=get(database::getvalue("FriendTarget"));
	enem=get(database::getvalue("EnemyTarget"));
	plnt=planet::get(database::getvalue("PlanetTarget"));
}

void ship::maintain()
{
	if(is_crippled)
	{
		if(shield_generator)
			shield_generator->rdy=-1;	
		if(cloaking_device)
			cloaking_device->rdy=-1;
		if(calc::rnd(402)==0)
			hit(1000,loc,mov,NULL);
	}
	else
	{
		if(power_plant && fuel_tank)
		{
			if((power_plant->cap)<(power_plant->item->capacity) && fuel_tank->cap>0)
			{
				fuel_tank->cap-=power_plant->item->power_requirement;
				power_plant->cap+=power_plant->item->power_requirement;
				if(fuel_tank->cap<0)
					fuel_tank->cap=0;
			}
			if((power_plant->cap)>(power_plant->item->capacity))
			{
				fuel_tank->cap+=(power_plant->cap)-(power_plant->item->capacity);
				power_plant->cap=power_plant->item->capacity;
			}
		}
	}
	
	for(int i=0;i<32;i++)
	{
		if(slots[i].item)
		{
			if(slots[i].rdy>0)
				slots[i].rdy--;
			else
				if(slots[i].rdy<0 && (slots[i].item->equipment_type==equip::PHASER || slots[i].item->equipment_type==equip::LAUNCHER || slots[i].item->equipment_type==equip::TRANSPORTER))
					slots[i].rdy=0;
		}
	}

	if(shield_generator && shield_generator->rdy==0 && power_plant && (power_plant->cap)>=shield_generator->item->power_requirement)
	{
		shield_generator->cap+=shield_generator->item->power_requirement;
		power_plant->cap-=shield_generator->item->power_requirement;
		if(shield_generator->cap>shield_generator->item->capacity)
			shield_generator->cap=shield_generator->item->capacity;
	}
	else
	{
		if(shield_generator)
			shield_generator->cap/=2;
	}
	if(cloaking_device)
	{
		if(cloaking_device->rdy==0)
		{
			if(power_plant && (power_plant->cap)>=(cloaking_device->item->power_requirement*mass)/20)
				power_plant->cap-=(cloaking_device->item->power_requirement*mass)/20;
			else
				uncloak();
			if(cloaking_device->cap<cloaking_device->item->capacity)
				cloaking_device->cap++;
		}
		else
		{
			if(cloaking_device->cap>=0)
				cloaking_device->cap=0;
			else
				cloaking_device->cap++;
		}
	}

	if(power_plant && power_plant->cap<0)
		power_plant->cap=0;
	if((!fuel_tank || (fuel_tank && fuel_tank->cap==0)) && !ply && calc::rnd(100)==0)
		delete this;
}

// ============================================================================
// SHIP AI BEHAVIOR SYSTEMS
// ============================================================================
//
// AI Behavior Types:
// - AI_NULL: No AI behavior (player controlled)
// - AI_AUTOPILOT: Navigate to selected planet
// - AI_PATROLLER: Patrol area and engage hostiles
// - AI_INVADER: Aggressive attack behavior
// - AI_CARAVAN: Trade route behavior
// - AI_BUDDY: Follow and assist allied ships
// - AI_FLEET: Coordinated fleet behavior
//

void ship::execute_ai_behavior()
{
	int individual_strobe; //Individual strobe for this ship
	bool run_amortized_code; //Run amortised cost code for this state?
	planet* target_planet; //A planet for use in ai code
	ship* target_ship; //A ship for use in ai code

	individual_strobe=(master_strobe+self*7)%400;
	if(individual_strobe%40==0)
		run_amortized_code=true;
	else
		run_amortized_code=false;

	//Run behaviours for each case of this ship's behaviour state
	switch(aity)
	{
		case AI_AUTOPILOT:
		if(enem)
			follow(enem);
		else if(plnt)
			navigate_to_planet(plnt);
		break;

		case AI_PATROLLER:
		if(enem)
		{
			execute_attack_maneuvers(enem,individual_strobe);
			handle_weapon_targeting(individual_strobe);
		}
		else if(plnt)
			navigate_to_planet(plnt);

		if(run_amortized_code)
		{
			if(!enem)
			{
				shieldsdown();
				enem=find_hostile_target();
			}
			else
			{
				shieldsup();
				if(plnt && !see(plnt))
					enem=NULL;
			}
			if(!plnt || plnt->all!=all || vel.rad<=5)
			{
				target_planet=planet::pick(all);
				if(target_planet && target_planet->typ!=planet::STAR && see(target_planet))
					plnt=target_planet;
			}
		}
		break;

		case AI_INVADER:
		if(enem)
		{
			execute_attack_maneuvers(enem,individual_strobe);
			handle_weapon_targeting(individual_strobe);
		}
		else if(plnt)
			navigate_to_planet(plnt);

		if(run_amortized_code)
		{
			if(plnt)
				cloak();
			if(enem && calc::rnd(10)==0)
				enem=NULL;
			if(!enem)
			{
				shieldsdown();
				target_ship=find_hostile_target();
				if(target_ship)
				{
					enem=target_ship;
					plnt=NULL;
				}
			}
			else
				shieldsup();
			if(!enem && !plnt)
				plnt=planet::find_hostile_planet(all);
		}
		break;

		case AI_CARAVAN:
		if(enem)
		{
			handle_weapon_targeting(individual_strobe);
			if((plnt && individual_strobe<200) || !plnt)
				execute_attack_maneuvers(enem,individual_strobe);
			else
				navigate_to_planet(plnt);
		}
		else if(plnt)
			navigate_to_planet(plnt);

		if(run_amortized_code)
		{
			if(!enem)
			{
				shieldsdown();
				enem=find_hostile_target();
			}
			else
				shieldsup();
			if(!plnt || all->opposes(plnt->all) || vel.rad<=5)
			{
				target_planet=planet::find_allied_planet(all);
				if(target_planet && target_planet->typ!=planet::STAR)
					plnt=target_planet;
			}
		}
		break;

		case AI_BUDDY:
		if(enem)
		{
			execute_attack_maneuvers(enem,individual_strobe);	
			handle_weapon_targeting(individual_strobe);
		}
		else if(frnd)
			follow(frnd);

		if(run_amortized_code)
		{
			if(!enem)
				shieldsdown();
			if(enem)
			{
				shieldsup();
				if(calc::rnd(10)==0)
					enem=NULL;
			}
			if(!enem)
				enem=find_hostile_target();
			if(frnd)
			{
				if(frnd->cloaking_device && frnd->cloaking_device->cap!=0)
					cloak();
				if(enem && !see(frnd))
					enem=NULL;
				if(frnd->frnd)
					frnd=frnd->frnd;
				if(frnd==this)
					frnd=NULL;
			}
			else
				frnd=find_allied_ship();
		}
		break;

		case AI_FLEET:
		if(enem)
		{
			if(shield_generator && shield_generator->cap<shield_generator->item->capacity)
				execute_attack_maneuvers(enem,individual_strobe);	
			else if(frnd)
				follow(frnd);
			handle_weapon_targeting(individual_strobe);
		}
		else if(frnd)
			follow(frnd);

		if(run_amortized_code)
		{
			if(!enem)
				shieldsdown();
			if(enem)
			{
				shieldsup();
				if(calc::rnd(10)==0)
					enem=NULL;
			}
			if(!enem) {
				if(frnd && frnd->enem)
					enem=frnd->enem;
				else
					enem=find_hostile_target();
		}
			if(frnd)
			{
				if(frnd->cloaking_device && frnd->cloaking_device->cap!=0)
					cloak();
				if(enem && !see(frnd))
					enem=NULL;
				if(frnd->frnd)
					frnd=frnd->frnd;
				if(frnd==this)
					frnd=NULL;
			}
			else
				frnd=find_allied_ship();
		}
		break;
	}
}

ship* ship::find_hostile_target()
{
        for(int i=0,j=0;i<ISIZE;i++)
        {
                j=calc::rnd(ISIZE);
                if(ships[j] && ships[j]!=this && all->opposes(ships[j]->all) && see(ships[j]))
                        return ships[j];
        }
        return NULL;
}

ship* ship::find_allied_ship()
{
        for(int i=0,j=0;i<ISIZE;i++)
        {
                j=calc::rnd(ISIZE);
                if(ships[j] && ships[j]!=this && !ships[j]->ply && all==ships[j]->all && see(ships[j]))
                        return ships[j];
        }
        return NULL;
}

void ship::alert_nearby_ships()
{
	for(int i=0;i<ISIZE;i++)
		if(ships[i] && !ships[i]->ply && ships[i]!=this && !ships[i]->enem && ships[i]->see(this))
			ships[i]->enem=this;
}

void ship::handle_weapon_targeting(int str)
{
	if(enem && see(enem))
	{
		shoot(false);
		//Shoot torpedoes if they appear more threatening; i.e. have a greater maximum shield capacity
		if(str>50 && enem->shield_generator && this->shield_generator && ((enem->shield_generator->item->capacity)*2)>(this->shield_generator->item->capacity))
		{
			shoot(true);
		}
	}
}

// ============================================================================
// SHIP EQUIPMENT MANAGEMENT
// ============================================================================

void ship::update_equipment_references()
{
	power_plant=NULL;
	shield_generator=NULL;
	sensor_array=NULL;
	cloaking_device=NULL;
	fuel_tank=NULL;

	for(int i=0;i<32;i++)
	{
		if(slots[i].item)
		{
			switch(slots[i].item->equipment_type)
			{
				case equip::POWER:
				power_plant=&slots[i];
				break;

				case equip::SHIELD:
				shield_generator=&slots[i];
				break;

				case equip::SENSOR:
				sensor_array=&slots[i];
				break;

				case equip::CLOAK:
				cloaking_device=&slots[i];
				break;

				case equip::FUELTANK:
				fuel_tank=&slots[i];
				break;
			}
		}
	}
}

// ============================================================================
// SHIP CLASS - STATIC DATA AND INITIALIZATION
// ============================================================================

ship* ship::ships[ISIZE];
ship* ship::lib[LIBSIZE];
int ship::master_strobe;
