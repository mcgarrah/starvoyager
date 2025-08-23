# StarVoyager Developer Notes

Technical information for developers working on the StarVoyager codebase.

## Coordinate System

### Game Convention (Compass/Navigation)
- **0° = North** (positive Y direction)
- **90° = East** (positive X direction)  
- **180° = South** (negative Y direction)
- **270° = West** (negative X direction)
- Angles increase **clockwise**

### Mathematical Convention (Standard)
- **0° = East** (positive X direction)
- **90° = North** (positive Y direction)
- Angles increase **counter-clockwise**

### Implementation
The conversion is handled in `calc.h` by the `tovect()` function:
```cpp
// Converts game angle to mathematical angle by subtracting 90°
cang = ((ang - 90) * M_PI) / 180;
```

### Testing Impact
When writing tests involving angles or directions:
1. **Use compass convention**: 0° points North, not East
2. **Clockwise rotation**: Increasing angles rotate clockwise
3. **Vector calculations**: Account for the 90° offset in tovect()

## Database Format

### File Structure
StarVoyager uses an object-oriented file format, not a simple key-value store:

```
SV0040                    // Magic number header
@ObjectName1              // Object declaration
key1=value1               // Key-value pairs within object
key2=value2
@ObjectName2              // Next object declaration  
key3=value3
key4=value4
```

### API Usage Pattern

**Writing Data:**
```cpp
FILE* file = fopen("data.dat", "wb");
database::openwriter(file);
database::putobject("PlayerData");     // REQUIRED: Create object first
database::putvalue("level", 5);
database::putvalue("name", "Player1");
database::closewriter();              // Closes file automatically
```

**Reading Data:**
```cpp
FILE* file = fopen("data.dat", "rb");
database::openreader(file);
database::switchobj("PlayerData");    // REQUIRED: Switch to object context
long level = database::getvalue("level");
char name[64];
database::getvalue("name", name);
database::closereader();              // Closes file automatically
```

### Critical Requirements
1. **Object Context Required**: All `putvalue()`/`getvalue()` operations must occur within an object context
2. **File-Based Storage**: In-memory operations don't persist - must use file I/O
3. **Automatic File Management**: `closewriter()`/`closereader()` call `fclose()` internally
4. **Magic Number**: Files start with "SV0040" header

### Common Mistakes
- ❌ Calling `putvalue()` without `putobject()` first
- ❌ Calling `getvalue()` without `switchobj()` first  
- ❌ Calling `fclose()` after `closewriter()`/`closereader()` (causes double free)
- ❌ Expecting in-memory operations to persist without file I/O

## Memory Management

### Database API
- `database::openwriter(FILE*)` - Takes ownership of file handle
- `database::closewriter()` - Calls `fclose()` internally and sets `owrt=NULL`
- `database::openreader(FILE*)` - Takes ownership of file handle  
- `database::closereader()` - Calls `fclose()` internally

### Double Free Prevention
Never call `fclose()` after database close operations:
```cpp
// WRONG - causes double free
database::closewriter();
fclose(file);

// CORRECT
database::closewriter(); // This closes the file
```

## Testing Guidelines

### Database Tests
Always test database operations with complete write→read cycles using temporary files.

### Coordinate Tests  
Use compass convention (0°=North) when testing angles and directions.

### Memory Tests
Use GDB with malloc checking for memory issue detection:
```bash
make gdb-debug  # Enhanced debugging with MALLOC_CHECK_=2
```

## Graphics System Architecture

### Drawing Operation Tracking

The graphics system uses a "dirty rectangle" tracking mechanism with a hardcoded limit:

**Buffer Size**: `MAX_DRAW_OPERATIONS = 1024`

**Purpose**: Double-buffered graphics with cleanup tracking
- `dtyp[1024]` - Stores type of each drawing operation (pixel, line, rectangle, clip)
- `dpos[1024]` - Stores position/dimensions of each drawing operation
- `nd` - Counter tracking number of operations stored

**Workflow**:
1. **Draw Phase**: Each operation (text, boxes, lines, pixels) recorded in arrays
2. **Display Phase**: `blit()` commits everything to screen
3. **Cleanup Phase**: `clean()` erases all recorded operations by drawing black over them
4. **Repeat**: Next frame starts clean

**Why 1024?**
- **Performance**: Limits memory usage to ~24KB (1024 × 24 bytes per operation)
- **Capacity**: Allows tracking up to 1024 drawing operations per frame
- **Era Constraint**: Written for older hardware where memory was more constrained

**Potential Issues**:
- **Silent Overflow**: Operations beyond 1024 per frame are silently ignored
- **No Dynamic Sizing**: Fixed buffer regardless of actual usage
- **Memory Waste**: Always allocates full 1024 slots even if using few

## Code Coverage Analysis

### Coverage Reports
StarVoyager supports comprehensive code coverage analysis using gcov and lcov:

**HTML Coverage Report:**
```bash
make coverage
# Generates coverage_report/index.html - interactive web-based report
```

**Text Coverage Report:**
```bash
make coverage-text
# Generates coverage_report.txt - plain text summary
```

### Coverage Process
1. **Clean Build**: Removes existing objects and coverage data
2. **Instrumented Compilation**: Builds with `--coverage` flags and `-lgcov` linking
3. **Test Execution**: Runs complete test suite via `run_tests.sh`
4. **Data Collection**: Uses `lcov` to capture coverage information
5. **Report Generation**: Creates HTML and/or text reports

### Coverage Metrics
- **Line Coverage**: Percentage of executable lines tested
- **Function Coverage**: Percentage of functions called during tests
- **File-by-File Breakdown**: Detailed coverage per source file

### Interpreting Results
**Well-tested modules** (>70% line coverage):
- `database.cc` - File I/O and data persistence
- `calc.h` - Mathematical utilities
- `interface.cc` - User interface components

**Needs attention** (<30% line coverage):
- `server.cc` - Network server functionality
- `ship.cc` - Core game logic
- `sockhelper.cc` - Network utilities

### Coverage Files
- `coverage.info` - Raw lcov data file
- `coverage_report/` - HTML report directory
- `coverage_report.txt` - Text summary file
- `*.gcda` - Runtime coverage data (auto-generated)
- `*.gcno` - Compile-time coverage notes (auto-generated)