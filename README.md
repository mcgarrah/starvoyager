# Star Voyager

A Frontier/Elite-style space combat and exploration game set in the Star Trek universe, featuring 2D arcade-style gameplay with SDL graphics.

## Goal

I would like to update this code base to be a bit more secure and see about upgrading to more current libraries. A cross-platform Win32 or MacOS build would also be fun to have. Win32 would open up a lot of people to the game.

I used [Johnny A. Solbu Github](https://github.com/solbu/starvoyager) with all the [Debian patches](https://packages.debian.org/sid/games/starvoyager) as a starting point. Thanks.

## Features

### Core Gameplay

- **Space Combat**: Real-time battles with phasers, disruptors, and photon torpedoes
- **Ship Management**: Control shields, energy, fuel, and hull integrity
- **Exploration**: Navigate through a procedurally populated universe
- **Trading**: Buy fuel, ammunition, and equipment from inhabited planets
- **Multiple Ship Types**: Various Federation, Klingon, and other alliance vessels
- **Warp Travel**: Sublight and warp speed navigation
- **Cloaking Technology**: Stealth capabilities for compatible ships

### Multiplayer Support

- **Network Play**: Up to 32 players in shared universe
- **Dedicated Server**: Standalone server mode (`-s` flag)
- **Player Persistence**: Save/load player progress with password protection
- **Real-time Chat**: In-game communication system

### Advanced Features

- **Alliance System**: Choose from multiple factions (Federation, Klingon, etc.)
- **AI Opponents**: Sophisticated enemy behavior and fleet tactics
- **Planetary Systems**: Inhabited worlds with trading and refueling
- **Equipment Upgrades**: Enhance your ship's capabilities
- **Damage System**: Realistic battle damage and repair mechanics
- **Sensor Systems**: Long-range scanners with zoom capabilities

## Installation

### Dependencies

```bash
# Debian/Ubuntu
sudo apt install libsdl2-dev libsdl-net1.2-dev libsdl1.2-compat-dev

# For code coverage analysis
sudo apt install lcov

# For debugging
sudo apt install gdb
```

### Build from Source

```bash
git clone https://github.com/mcgarrah/starvoyager
cd starvoyager
make
sudo make install
```

### Run the Game

```bash
/usr/games/starvoyager
```

## Usage

### Starting a Game

1. Run `starvoyager`
2. Press `1` for local game
3. Enter player name
4. Choose alliance (Federation, Klingon, etc.)
5. Set password when prompted

### Command Line Options

- `-v` - Print version and exit
- `-h` - Show help
- `-x` - Run in extra-large window
- `-f` - Fullscreen mode
- `-s` - Run as standalone server

### Multiplayer

1. Start server: `starvoyager -s`
2. Connect: Press `3` at main menu, enter server IP

## Controls

### Movement

- **Arrow Keys** - Accelerate and turn
- **Shift + Arrows** - Warp speed transition
- **Shift + Down** - Reverse thrust

### Combat

- **Space** - Fire energy weapons (phasers/disruptors)
- **Z** - Fire torpedoes

### Targeting

- **T** - Cycle through nearby ships
- **E** - Target enemy ships only
- **P** - Target planets
- **Shift + T/E/P** - Reverse cycle

### Interface

- **F1** - Ship status
- **F2** - Internal systems (shields, cloaking)
- **F3** - Scanner controls
- **F4** - Hail target
- **F5** - Chat (multiplayer)
- **F6** - Player information
- **F7** - Game options

### Display

- **-/+** - Zoom sensor display
- **/** - Zoom main view
- **Q** - Quit game

## Gameplay Guide

### Ship Systems

- **Shields** - Toggle on/off (drains energy, blocks transporters)
- **Energy** - Powers weapons, shields, and movement
- **Hull** - Ship integrity (repairs available at friendly planets)
- **Fuel** - Required for power generation

### Combat Tips

- Energy weapons auto-target selected enemy
- Torpedoes require line-of-sight and range
- Shields drain energy but provide protection
- Cloaking makes you harder to detect but uses power

### Economy

- Earn credits by destroying enemy vessels
- Purchase fuel, ammunition, and upgrades at planets
- Hail inhabited planets to access services
- Save progress by beaming down to allied planets

### Navigation

- Use sensors to navigate large distances
- Warp speed for interstellar travel
- Mass lock prevents warping near planets
- Target planets for navigation assistance

## Technical Information

### System Requirements

- SDL 1.2 or 2.0
- SDL_net library
- Linux/Unix (primary), Windows/macOS (theoretical)
- Minimum 600x400 display

### File Structure

- `data/gfx/` - Game graphics and sprites
- `data/snd/` - Sound effects
- `data/*.svd` - Game databases (ships, equipment, alliances)

### Network Protocol

- TCP-based multiplayer
- Optimized for modem connections
- Adaptive client framerate
- Bandwidth-efficient updates

## Development

### VS Code Debugging Setup

The project includes a complete VS Code debugging configuration in the `.vscode/` directory:

#### Files Included

- `launch.json` - Debug configurations for client and server modes
- `tasks.json` - Build, clean, and rebuild tasks
- `c_cpp_properties.json` - IntelliSense configuration for SDL development
- `settings.json` - Makefile tools integration

#### Debug Configurations

- **Debug StarVoyager** - Main game debugging
- **Debug StarVoyager (Server)** - Server mode with `-s` flag

#### Usage

1. Open project in VS Code
2. Set breakpoints by clicking line numbers
3. Press `F5` to start debugging
4. Choose configuration from dropdown
5. Use `F10` (step over), `F11` (step into), `F5` (continue)

#### Features

- Automatic build before debugging
- SDL graphics compatibility
- Pretty-printing for variables
- IntelliSense code completion
- Multiple debug configurations

### Code Coverage Analysis

Generate code coverage reports to validate test effectiveness:

```bash
# Generate coverage report
make coverage

# View report
open coverage_report/index.html
```

#### VS Code Integration

1. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on macOS)
2. Type "Tasks: Run Task"
3. Select "Generate Coverage Report"
4. View results in `coverage_report/index.html`

### Version History

- **0.5.0** - Current version with Debian patches and build fixes
- **0.4.x** - Network improvements and bug fixes
- **0.3.x** - Multiplayer support added
- **0.2.x** - Sound effects and AI improvements
- **0.1.x** - Core gameplay implementation

### License

Licensed under LGPL - see LICENSE file for details.

### Contributing

Original author: Richard Thrippleton (ret28@cam.ac.uk)

Enthusiastic coder: Michael McGarrah (mcgarrah@gmail.com)

## Troubleshooting

### Common Issues

- **Compilation errors**: Ensure all SDL dependencies are installed
- **Crash on startup**: Check data files are in correct location
- **Network connection fails**: Verify server is running and accessible
- **Performance issues**: Try `-x` flag for larger window, check framerate

### Debug Information

- Crash logs saved to `~/.starvoyager/exitlog`
- Use `-Wall -ggdb3` compile flags for debugging
- Report bugs with detailed system information

## Credits

**Original Author**: Richard Thrippleton

**Past Maintainer**: Johnny A. Solbu

**Current Maintainer**: Michael McGarrah

**Engine**: SDL (Simple DirectMedia Layer)  

**Theme**: Star Trek Universe  

**Inspired by**: Elite, Frontier series
