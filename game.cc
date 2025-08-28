/*
	game.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <csignal>
#include <stdio.h>

#include "alliance.h"
#include "calc.h"
#include "camera.h"
#include "client.h"
#include "database.h"
#include "error.h"
#include "frag.h"
#include "game.h"
#include "graphic.h"
#include "interface.h"
#include "os.h"
#include "planet.h"
#include "player.h"
#include "presence.h"
#include "server.h"
#include "ship.h"
#include "ticker.h"

static volatile bool shutdown_requested = false;

static void signal_handler(int sig) {
	shutdown_requested = true;
}

void game::runheadless() //Run as a headless server
{
	ticker lreg(24); //Loop regulator
	int sdly; //Game saving delay

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	
	sdly=0;
	lreg.start();
	try
	{
		server::start(false);
		try
		{
			load();
		}
		catch(error it)
		{
			alliance::maketerritories();
		}
		while(!shutdown_requested)
		{
			server::cycle();
			ship::simulateall();
			frag::simulateall();
			ship::behaveall();
			planet::shipyards();
			lreg.tick();
			if(sdly<=0)
			{
				save();
				sdly=3000;
			}
			sdly--;
		}
		if(shutdown_requested)
			throw error("Server shutdown requested");
	}
	catch(error it)
	{
		server::stop();
		save();
		player::purgeall();
		ship::purgeall();
		planet::purgeall();
		frag::purgeall();
		throw it;
	}
}

void game::runlocal()
{
	ticker lreg(24); //Loop regulator
	int frame_drop_rate; //Frame drop rate
	int frame_counter; //Frame drop counter

	lreg.start();
	frame_counter=0;
	frame_drop_rate=100;
	try
	{
		interface::printtomesg(NULL);
		server::start(true);
		try
		{
			load();
		}
		catch(error it)
		{
			alliance::maketerritories();
		}
		client::connect("127.0.0.1");
		while(true)
		{
			interface::poll();
			presence::controls();
			client::flush();
			server::cycle();

			lreg.tick();

			presence::interpolateall();
			client::process_network_messages();
			camera::update();
			frame_counter+=100;
			if(frame_counter>=frame_drop_rate)
			{
				frame_counter-=frame_drop_rate;
				graphic::clean();
				camera::render();
				interface::render();
				presence::render();
				graphic::blit();
			}
			ship::simulateall();
			frag::simulateall();
			ship::behaveall();
			planet::shipyards();
		}
	}
	catch(error it)
	{
		client::stop();
		server::stop();

		save();

		player::purgeall();
		ship::purgeall();
		planet::purgeall();
		frag::purgeall();

		presence::purgeall();
		camera::turnoff();
		camera::unbind();
		interface::printtomesg(NULL);
		interface::printtomesg(it.str);
	}
}

void game::runclient(char* host)
{
	ticker lreg(25); //Loop regulator
	int frame_drop_rate; //Frame drop rate
	int frame_counter; //Frame drop counter

	lreg.start();
	frame_counter=0;
	frame_drop_rate=100;
	try
	{
		interface::printtomesg(NULL);
		client::connect(host);
		while(true)
		{
			interface::poll();
			presence::controls();
			client::flush();

			lreg.tick();

			presence::interpolateall();
			client::process_network_messages();
			camera::update();
			frame_counter+=100;
			if(frame_counter>=frame_drop_rate)
			{
				frame_counter-=frame_drop_rate;
				graphic::clean();
				camera::render();
				interface::render();
				presence::render();
				graphic::blit();
			}
			if(lreg.afps<23)
				frame_drop_rate++;
			if(lreg.afps>23.9 && frame_drop_rate>100)
				frame_drop_rate--;
		}
	}
	catch(error it)
	{
		client::stop();
		presence::purgeall();
		camera::turnoff();
		camera::unbind();
		interface::printtomesg(NULL);
		interface::printtomesg(it.str);
	}
}

void game::save()
{
	FILE* f = os::openpersonal("universe.svd","w");
	if(!f)
		throw error("Cannot open universe.svd for writing");
	database::openwriter(f);
	planet::saveall();
	ship::saveall();
	frag::saveall();
	player::saveall();
	database::closewriter();
}

void game::load()
{
	FILE* f = os::openpersonal("universe.svd","r");
	if(!f)
		throw error("Cannot open universe.svd for reading");
	// Check if file is empty or too small to be valid
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if(size < 10) // Minimum size for a valid database file
	{
		fclose(f);
		throw error("Universe file is too small or empty");
	}
	database::openreader(f);
	planet::loadall();
	ship::loadall();
	frag::loadall();
	player::loadall();
	database::closereader();
}
