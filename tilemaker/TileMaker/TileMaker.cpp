// TileMaker : Pull in a base set of tiles and overlay all tiles found in the tiles directory. For Mineways.
//
// Step 1: Read in the terrain.png file.
// Step 2: Read all tiles in the tiles directory, get name and size of each. Check name for if it's in the known
// set of tiles.
// Step 3: make the background tile (which contains defaults) the proper size of the largest tile found in "tiles". 
// For example, if a 64x64 tile is found, the background set of tile are all expanded to this size.
// Step 4: overlay new tiles. Write out new tile set as terrain_new.png

#include "rwpng.h"
#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "tiles.h"
#include "tilegrid.h"

#define	VERSION_STRING	L"3.01"

//#define TILE_PATH	L".\\blocks\\"
#define BASE_INPUT_FILENAME			L"terrainBase.png"
#define TILE_PATH	L"blocks"
#define OUTPUT_FILENAME L"terrainExt.png"

typedef struct ChestData {
	int fromX;
	int fromY;
	int sizeX;
	int sizeY;
	int txrX;   // column and row, from upper left, of 64x64 chest tile
	int txrY;
	int toX;
	int toY;
	unsigned int flags;
} ChestData;

static ChestData gNormalChest[] = {
	//  from,    size, to tile,  starting at corner
	{  0,  0,   6,  5,   7, 26,   0, 0,  0x0 },	// MWO_chest_latch
	{ 14,  0,  14, 14,   9,  1,   1, 1,  0x0 },	// MWO_chest_top
	{  0, 14,  14,  4,  10,  1,   1, 2,  0x0 },	// top of MWO_chest_side
	{  0, 33,  14, 10,  10,  1,   1, 6,  0x0 },	// bottom of MWO_chest_side
	{ 14, 14,  14,  4,  11,  1,   1, 2,  0x0 },	// top of MWO_chest_front
	{ 14, 33,  14, 10,  11,  1,   1, 6,  0x0 },	// bottom of MWO_chest_front
};

static ChestData gNormalDoubleChest[] = {
	//  from,    size, to tile,  starting at corner
	{ 14, 14,  15,  4,   9,  2,  1, 2,  0x0 },	// MWO_double_chest_front_left top
	{ 14, 33,  15, 10,   9,  2,  1, 6,  0x0 },	// MWO_double_chest_front_left bottom
	{ 29, 14,  15,  4,  10,  2,  0, 2,  0x0 },	// MWO_double_chest_front_right top
	{ 29, 33,  15, 10,  10,  2,  0, 6,  0x0 },	// MWO_double_chest_front_right bottom
	{ 58, 14,  15,  4,   9,  3,  1, 2,  0x0 },	// MWO_double_chest_back_left top
	{ 58, 33,  15, 10,   9,  3,  1, 6,  0x0 },	// MWO_double_chest_back_left bottom
	{ 73, 14,  15,  4,  10,  3,  0, 2,  0x0 },	// MWO_double_chest_back_right top
	{ 73, 33,  15, 10,  10,  3,  0, 6,  0x0 },	// MWO_double_chest_back_right bottom
	{ 14,  0,  15, 14,   9, 14,  1, 1,  0x0 },	// MWO_double_chest_top_left
	{ 29,  0,  15, 14,  10, 14,  0, 1,  0x0 },	// MWO_double_chest_top_right
};

static ChestData gEnderChest[] = {
	//  from,    size, to tile,  starting at corner
	{  0,  0,   6,  5,   9, 13,   0, 0,  0x0 },	// MWO_ender_chest_latch
	{ 14,  0,  14, 14,  10, 13,   1, 1,  0x0 },	// MWO_ender_chest_top
	{  0, 14,  14,  4,  11, 13,   1, 2,  0x0 },	// top of MWO_ender_chest_side
	{  0, 33,  14, 10,  11, 13,   1, 6,  0x0 },	// bottom of MWO_ender_chest_side
	{ 14, 14,  14,  4,  12, 13,   1, 2,  0x0 },	// top of MWO_ender_chest_front
	{ 14, 33,  14, 10,  12, 13,   1, 6,  0x0 },	// bottom of MWO_ender_chest_front
};


// from: from upper left pixel
// size: resolution; negative means scan backwards, and compute lower right and go from there (for my sanity)
// to tile: in the grid of tiles, where it goes
// starting at corner: point to start in 16x16 tile itself
static ChestData gNormalChest115[] = {
	//  from,    size, to tile,  starting at corner
	{  1,  0,   6,  5,   7, 26,   0, 0,  0x3 },	// MWO_chest_latch
	{ 28,  0,  14, 14,   9,  1,   1, 1,  0x3 },	// MWO_chest_top
	{  0, 15,  14,  4,  10,  1,   1, 2,  0x3 },	// top of MWO_chest_side
	{  0, 33,  14, 10,  10,  1,   1, 6,  0x3 },	// bottom of MWO_chest_side
	{ 42, 15,  14,  4,  11,  1,   1, 2,  0x3 },	// top of MWO_chest_front
	{ 42, 33,  14, 10,  11,  1,   1, 6,  0x3 },	// bottom of MWO_chest_front
};

// Minecraft names these left and right, but in fact they're swapped when rendered,
// with the right chest tile's elements being put on the left, and vice versa.
static ChestData gNormalLeftChest115[] = {
	//  from,    size, to tile,  starting at corner
	{ 43, 15,  15,  4,  10,  2,  0, 2,  0x3 },	// MWO_double_chest_front_left top half
	{ 43, 33,  15, 10,  10,  2,  0, 6,  0x3 },	// MWO_double_chest_front_left bottom half
	{ 14, 15,  15,  4,  10,  3,  0, 2,  0x2 },	// MWO_double_chest_back_left top half - should really swap with RightChest, but nah
	{ 14, 33,  15, 10,  10,  3,  0, 6,  0x2 },	// MWO_double_chest_back_left bottom half - should really swap with RightChest, but nah
	{ 14, 19,  15, 14,  10, 14,  0, 1,  0x2 },	// MWO_double_chest_top_left
};

static ChestData gNormalRightChest115[] = {
	//  from,    size, to tile,  starting at corner
	{ 43, 15,  15,  4,   9,  2,  1, 2,  0x3 },	// MWO_double_chest_front_right top half
	{ 43, 33,  15, 10,   9,  2,  1, 6,  0x3 },	// MWO_double_chest_front_right bottom half
	{ 14, 15,  15,  4,   9,  3,  1, 2,  0x2 },	// MWO_double_chest_back_right top half
	{ 14, 33,  15, 10,   9,  3,  1, 6,  0x2 },	// MWO_double_chest_back_right bottom half
	{ 14, 19,  15, 14,   9, 14,  1, 1,  0x2 },	// MWO_double_chest_top_right - note we don't flip
};

static ChestData gEnderChest115[] = {
	//  from,    size, to tile,  starting at corner
	{  1,  0,   6,  5,   9, 13,   0, 0,  0x3 },	// MWO_ender_chest_latch
	{ 28,  0,  14, 14,  10, 13,   1, 1,  0x3 },	// MWO_ender_chest_top
	{  0, 15,  14,  4,  11, 13,   1, 2,  0x3 },	// top of MWO_ender_chest_side
	{  0, 33,  14, 10,  11, 13,   1, 6,  0x3 },	// bottom of MWO_ender_chest_side
	{ 42, 15,  14,  4,  12, 13,   1, 2,  0x3 },	// top of MWO_ender_chest_front
	{ 42, 33,  14, 10,  12, 13,   1, 6,  0x3 },	// bottom of MWO_ender_chest_front
};

typedef struct Chest {
	const wchar_t* wname;
	int numCopies;	// number of elements we'll copy
	int defaultResX;	// how big the image is in the default set
	int defaultresY;
	ChestData* data;
} Chest;


// note these are NOT const, as they do get modified and used
static Chest gChest114[] = {
	{ L"normal", 6, 64, 64, NULL },
	{ L"normal_double", 10, 128, 64, NULL },
	{ L"ender", 6, 64, 64, NULL }
};

static Chest gChest115[] = {
	{ L"normal", 6, 64, 64, NULL },
	{ L"normal_left", 5, 64, 64, NULL },
	{ L"normal_right", 5, 64, 64, NULL },
	{ L"ender", 6, 64, 64, NULL }
};


static const int gCatChannels[TOTAL_CATEGORIES] = { 4, 3, 3, 1, 1, 1, 3, 3, 3 };
static const LodePNGColorType gCatFormat[TOTAL_CATEGORIES] = { LCT_RGBA, LCT_RGB, LCT_RGB, LCT_GREY, LCT_GREY, LCT_GREY, LCT_RGB, LCT_RGB, LCT_RGB };

static int gErrorCount = 0;
static int gWarningCount = 0;

static wchar_t gErrorString[1000];
// 1000 errors of 100 characters each - sounds sufficient
#define CONCAT_ERROR_LENGTH	(1000*100)
static wchar_t gConcatErrorString[CONCAT_ERROR_LENGTH];

#define INC_AND_TEST_ARG_INDEX( loc )		argLoc++; \
											if (argLoc == argc) { \
												printHelp(); \
												return 1; \
											}

//-------------------------------------------------------------------------
void printHelp();

int shareFileRecords(FileGrid* pfg, wchar_t* tile1, wchar_t* tile2);
int checkFileWidth(FileRecord* pfr, int overlayTileSize, boolean square, boolean isFileGrid, int index, int lavaFlowIndex, int waterFlowIndex);
int trueWidth(int index, int width, int lavaFlowIndex, int waterFlowIndex);

int testFileForPowerOfTwo(int width, int height, const wchar_t* cFileName, bool square);

static void reportReadError(int rc, const wchar_t* filename);
static void saveErrorForEnd();

static void setBlackAlphaPNGTile(int chosenTile, progimage_info* src);
static int setBlackToNearlyBlack(progimage_info* src);
static int copyPNGTile(progimage_info* dst, int channels, unsigned long dst_x, unsigned long dst_y, unsigned long chosenTile, progimage_info* src,
	unsigned long dst_x_lo, unsigned long dst_y_lo, unsigned long dst_x_hi, unsigned long dst_y_hi, unsigned long src_x_lo, unsigned long src_y_lo, unsigned long flags, float zoom);
static void multPNGTileByColor(progimage_info* dst, int dst_x, int dst_y, int* color);
static void getPNGPixel(progimage_info* src, int channels, int col, int row, unsigned char* color);
static void getBrightestPNGPixel(progimage_info* src, int channels, unsigned long col, unsigned long row, unsigned long res, unsigned char* color, int* locc, int* locr);
static int computeVerticalTileOffset(progimage_info* src, int chosenTile);
static int isPNGTileEmpty(progimage_info* dst, int dst_x, int dst_y);
static void makePNGTileEmpty(progimage_info* dst, int dst_x, int dst_y);
static void makeSolidTile(progimage_info* dst, int chosenTile, int solid);

static void copyPNG(progimage_info* dst, progimage_info* src);
static void copyPNGArea(progimage_info* dst, unsigned long dst_x_min, unsigned long dst_y_min, unsigned long size_x, unsigned long size_y, progimage_info* src, int src_x_min, int src_y_min);

static int checkForCutout(progimage_info* dst);
static int convertHeightfieldToXYZ(progimage_info* src, float heightfieldScale);

static bool rotateTileIfHorizontal(progimage_info& tile);
static void rotateTile(progimage_info& tile, int channels);



int wmain(int argc, wchar_t* argv[])
{
	int rc = 0;
	progimage_info basicterrain;
	progimage_info destination;
	progimage_info* destination_ptr = &destination;

	int i, j, catIndex;
	int index, fullIndex;

	int baseTileSize, xTiles, baseYTiles, baseXResolution, baseYResolution;
	int outputTileSize, outputYTiles;
	unsigned long outputXResolution, outputYResolution;

	wchar_t terrainBase[MAX_PATH_AND_FILE];
	wchar_t terrainExtOutputTemplate[MAX_PATH_AND_FILE];
	wchar_t terrainExtOutputRoot[MAX_PATH_AND_FILE];
	wchar_t terrainExtOutput[MAX_PATH_AND_FILE];

#define MAX_INPUT_DIRECTORIES 100
	wchar_t* inputDirectoryList[MAX_INPUT_DIRECTORIES + 1];	// 1 extra, for the null terminator
	int numInputDirectories = 0;

	gConcatErrorString[0] = 0;

	int argLoc = 1;

	int overlayTileSize = 0;
	int overlayChestSize = 0;
	int forcedTileSize = 0;
	int chosenTile = 0;

	int nobase = 0;
	bool useTiles = true;
	int onlyreplace = 0;
	int verbose = 0;
	int checkmissing = 0;
	int alternate = 2;  // always include alternate names; needed for 1.13
	int solid = 0;
	int solidcutout = 0;
	int heightfieldCount = 0;
	float heightfieldScale = 0.5f;
	int normalsCount = 0;

	bool allChests = true;
	bool anyChests = false;

	initializeFileGrid(&gFG);
	initializeChestGrid(&gCG);

	wcscpy_s(terrainBase, MAX_PATH_AND_FILE, BASE_INPUT_FILENAME);
	wcscpy_s(terrainExtOutputTemplate, MAX_PATH_AND_FILE, OUTPUT_FILENAME);

	// usage: [-i terrainBase.png] [-d tiles_directory] [-o terrainExt.png] [-t forceTileSize]
	// single argument is alternate subdirectory other than "tiles"
	while (argLoc < argc)
	{
		if (wcscmp(argv[argLoc], L"-i") == 0)
		{
			INC_AND_TEST_ARG_INDEX(argLoc);
			wcscpy_s(terrainBase, MAX_PATH_AND_FILE, argv[argLoc]);
			if (!isPNGfile(terrainBase)) {
				wprintf(L"***** ERROR: '-i %s' is illegal. You must specify an output file name with '.png' at the end. Aborting.\n", terrainBase);
				// quit!
				return 1;
			}
		}
		// -z is supported for backwards compatibility, but -d should suffice now
		else if (wcscmp(argv[argLoc], L"-d") == 0 || wcscmp(argv[argLoc], L"-z") == 0)
		{
			if (wcscmp(argv[argLoc], L"-z") == 0 ) {
				wprintf(L"Note: the '-z directory' command-line argument is deprecated;\n  use '-d directory' (multiple times, as you like).\n");
			}
			INC_AND_TEST_ARG_INDEX(argLoc);
			inputDirectoryList[numInputDirectories++] = _wcsdup(argv[argLoc]);
			if (numInputDirectories >= MAX_INPUT_DIRECTORIES) {
				wprintf(L"***** ERROR: Sorry, there is a maximum of %d input directories you can specify, you wacky person you.\n", MAX_INPUT_DIRECTORIES);
				return 1;
			}
		}
		else if (wcscmp(argv[argLoc], L"-o") == 0)
		{
			INC_AND_TEST_ARG_INDEX(argLoc);
			wcscpy_s(terrainExtOutputTemplate, MAX_PATH_AND_FILE, argv[argLoc]);
			if (!isPNGfile(terrainExtOutputTemplate)) {
				wprintf(L"***** ERROR: '-o %s' is illegal. You must specify an output file name with '.png' at the end. Aborting.\n", terrainExtOutputTemplate);
				// quit!
				return 1;
			}
		}
		else if (wcscmp(argv[argLoc], L"-t") == 0)
		{
			// force to a given tile size.
			INC_AND_TEST_ARG_INDEX(argLoc);
			swscanf_s(argv[argLoc], L"%d", &forcedTileSize);
		}
		else if (wcscmp(argv[argLoc], L"-c") == 0)
		{
			// choose which tile of multiple tiles to use.
			INC_AND_TEST_ARG_INDEX(argLoc);
			swscanf_s(argv[argLoc], L"%d", &chosenTile);
		}
		else if (wcscmp(argv[argLoc], L"-nb") == 0)
		{
			// no base background image; mostly for debug, to see which tiles we actually have ready.
			nobase = 1;
		}
		else if (wcscmp(argv[argLoc], L"-nt") == 0)
		{
			// no tiles
			useTiles = false;
		}
		else if (wcscmp(argv[argLoc], L"-r") == 0)
		{
			// replace with tiles from directory only those tiles that don't exist (i.e. base terrain wins)
			onlyreplace = 1;
		}
		else if (wcscmp(argv[argLoc], L"-m") == 0)
		{
			// Check for missing tiles, i.e. look for names in tiles.h that do not have a corresponding tile
			// in the tile directory. This lets people know what tiles they need to add.
			checkmissing = 1;
		}
		else if (wcscmp(argv[argLoc], L"-a") == 0)
		{
			// alternate: use names such as "blockIron" when "iron_block" is not found
			//alternate = 1;
			wprintf(L"Note: alternate names are always on now, so -a is no longer needed.\n");
		}
		else if (wcscmp(argv[argLoc], L"-na") == 0)
		{
			// turn alternate names off
			wprintf(L"Note: alternate names are needed for 1.13, so -na is no longer supported.\n");
		}
		else if (wcscmp(argv[argLoc], L"-s") == 0)
		{
			// solid: take the average color of the incoming tile and output this solid color
			solid = 1;
		}
		else if (wcscmp(argv[argLoc], L"-S") == 0)
		{
			// solid cutout: as above, but preserve the cutout transparent areas
			solidcutout = 1;
		}
		else if (wcscmp(argv[argLoc], L"-h") == 0)
		{
			// heightfield scale - 0.5 by default
			INC_AND_TEST_ARG_INDEX(argLoc);
			swscanf_s(argv[argLoc], L"%f", &heightfieldScale);
		}
		else if (wcscmp(argv[argLoc], L"-v") == 0)
		{
			// verbose: tell when normal things happen
			verbose = 1;
		}
		else
		{
			printHelp();
			return 1;
		}
		argLoc++;
	}

	if (verbose)
		wprintf(L"TileMaker version %s\n", VERSION_STRING);

	xTiles = 16;	// this should always be the same for all things
	if (!nobase)
	{
		// read the base terrain file
		rc = readpng(&basicterrain, terrainBase, LCT_RGBA);
		if (rc != 0)
		{
			reportReadError(rc, terrainBase);
			return 1;
		}
		readpng_cleanup(0, &basicterrain);
		if (verbose)
			wprintf(L"The base terrain is '%s'\n", terrainBase);

		baseTileSize = basicterrain.width / xTiles;
		baseYTiles = basicterrain.height / baseTileSize;
	}
	else
	{
		if (verbose)
			wprintf(L"No base terrain file is set.\n");
		// minimums
		baseTileSize = 16;
		baseYTiles = VERTICAL_TILES;
	}
	baseXResolution = xTiles * baseTileSize;
	baseYResolution = baseYTiles * baseTileSize;

	// output should be the size of the output number of tiles
	outputYTiles = VERTICAL_TILES; // used to be baseYTiles - that's no good

#ifdef _DEBUG
	// reality check: make sure no tile in the tiles.h array is used twice (hey, I've made this mistake it in the past)
	for (int tileid = 0; tileid < TOTAL_TILES - 1; tileid++) {
		if ((gTilesTable[tileid].txrX != tileid % 16) || (gTilesTable[tileid].txrY != (int)(tileid / 16))) {
			wprintf(L"INTERNAL WARNING: Tile %d,%d does not have the expected txrX and txrY values\n", tileid % 16, (int)(tileid / 16));
			assert(0);
			gWarningCount++;
		}
		if (wcslen(gTilesTable[tileid].filename) > 0) {
			for (int testtile = tileid + 1; testtile < TOTAL_TILES; testtile++) {
				if (_wcsicmp(gTilesTable[tileid].filename, gTilesTable[testtile].filename) == 0) {
					wprintf(L"INTERNAL WARNING: Tile %d,%d and tile %d,%d have the same file name %wS\n", tileid % 16, (int)(tileid / 16), testtile % 16, (int)(testtile / 16), gTilesTable[tileid].filename);
					assert(0);
					gWarningCount++;
				}
			}
		}
	}
#endif

	// If there is no directory specified, use "blocks"
	if (numInputDirectories == 0) {
		inputDirectoryList[numInputDirectories++] = _wcsdup(TILE_PATH);
	}
	// put a NULL pointer on the end of the list
	inputDirectoryList[numInputDirectories] = NULL;

	// look through tiles in tiles directories, see which exist.
	int filesFound = 0;
	int filesProcessed = 0;
	boolean warnDups = true;
	wchar_t** inputDirectoryPtr = inputDirectoryList;
	while (*inputDirectoryPtr != NULL) {
		// Strategy: does the directory exist?
		// If so, categorize the directory. If it's
		//  "block" or "blocks" - look through it for block names
		//  "chest" or "chests" - look for chest names and fill in
		//  "item" or "items" - look for barrier.png, only
		// If it's none of these, then look through it for directories. Ignore '.' and '..'. Recursively search directories for more directories.
		int fileCount = searchDirectoryForTiles(&gFG, &gCG, *inputDirectoryPtr, wcslen(*inputDirectoryPtr), verbose, alternate, true, warnDups);
		warnDups = false;
		if (fileCount < 0) {
			wsprintf(gErrorString, L"***** ERROR: cannot access the directory '%s' (Windows error code # %d). Ignoring directory.\n", *inputDirectoryPtr, GetLastError());
			saveErrorForEnd();
			gErrorCount++;
		}
		else {
			filesFound += fileCount;
		}
		inputDirectoryPtr++;
	}

	// any data found? Not needed if forcing a tile size (resizing the base texture).
	if ((forcedTileSize == 0) && (gFG.fileCount <= 0 && gCG.chestCount <= 0)) {
		wprintf(L"***** ERROR: no textures were read in for replacing. Nothing to do!\n  Put your new textures in the 'blocks' directory, or use\n  the '-d directory' command line option to say where your new textures are.\n");
		return 1;
	}

	// look for tiles not input?
	if (checkmissing)
	{
		for (i = 0; i < TOTAL_TILES; i++)
		{
			if (!gFG.fr[i].exists)
			{
				// if it starts with "MW" or is the empty string, ignore miss
				if (wcslen(gTilesTable[i].filename) > 0 && wcsncmp(gTilesTable[i].filename, L"MW", 2) != 0) {
					wprintf(L"WARNING: TileMaker needs a tile named '%s.png' that was not replaced.\n", gTilesTable[i].filename);
					gWarningCount++;
				}
			}
		}
	}

	// Find largest tile.
	int lavaFlowIndex = findTileIndex(L"lava_flow", 0);
	int waterFlowIndex = findTileIndex(L"water_flow", 0);
	assert(lavaFlowIndex >= 0 && waterFlowIndex >= 0);
	int size;
	for (catIndex = 0; catIndex < gFG.totalCategories; catIndex++) {
		for (index = 0; index < gFG.totalTiles; index++) {
			fullIndex = catIndex * gFG.totalTiles + index;
			if (gFG.fr[fullIndex].exists) {
				size = checkFileWidth(&gFG.fr[fullIndex], overlayTileSize, true, true, index, lavaFlowIndex, waterFlowIndex);
				if (size == 0) {
					deleteFileFromGrid(&gFG, fullIndex / gFG.totalTiles, fullIndex);
				}
				else {
					overlayTileSize = size;
				}
			}
		}
	}

	// check over chest tiles' power of twos, to see if any are in error
	for (catIndex = 0; catIndex < gCG.totalCategories; catIndex++) {
		for (index = 0; index < gCG.totalTiles; index++) {
			fullIndex = catIndex * gCG.totalTiles + index;
			if (gCG.cr[fullIndex].exists) {
				size = checkFileWidth(&gCG.cr[fullIndex], overlayChestSize, false, false, -1, 0, 0);
				if (size == 0) {
					deleteChestFromGrid(&gCG, fullIndex / gCG.totalTiles, fullIndex);
				}
				else {
					overlayChestSize = size;
				}
			}
		}
	}
	if (verbose) {
		wprintf(L"Largest input image found was %d pixels wide.\n", overlayTileSize);
	}

	// take the larger of the overlay and base tile sizes as the target size
	outputTileSize = (overlayTileSize > baseTileSize) ? overlayTileSize : baseTileSize;

	// however, if there's a forced tile size, use that:
	if (forcedTileSize > 0)
	{
		outputTileSize = forcedTileSize;

		if (verbose)
			wprintf(L"Output texture '%s' is forced to have tiles that are each %d pixels wide.\n", terrainExtOutputTemplate, outputTileSize);
	}
	else {
		wprintf(L"Output texture '%s' will have tiles that are each %d pixels wide.\n", terrainExtOutputTemplate, outputTileSize);
	}

	// warn user of large tiles
	if (outputTileSize > 256) {
		wprintf(L"WARNING: With a texture image size of %d X %d, animation programs such as Blender\n  may have problems with such large textures, unless you export from Mineways\n  by using the 'Export tiles for textures' option. Consider running again,\n  using the '-t tileSize' option, choosing a power of two value less than this,\n  such as '-t 256' or '-t 128'.\n", outputTileSize, outputTileSize);
		gWarningCount++;
	}

	// now add new textures as needed. If sharing goes on, drop the output count.
	filesProcessed -= shareFileRecords(&gFG, L"smooth_stone", L"stone_slab_top");
	filesProcessed -= shareFileRecords(&gFG, L"smooth_stone_slab_side", L"stone_slab_side");

	// should check if the redstone_dust_line* record is horizontal - need to make it vertical TODOTODO

	// if there is a redstone_dust_line0 but not a line1, or vice versa, copy one to the other
	bool lineDuplicated = false;
	int prevProcessed = filesProcessed;
	filesProcessed -= shareFileRecords(&gFG, L"redstone_dust_line0", L"redstone_dust_line1");
	if (prevProcessed != filesProcessed) {
		wprintf(L"NOTE: only one of the tiles for redstone_dust_line0 and redstone_dust_line1 exists, so it is also used for the other.\n");
		lineDuplicated = true;
		gWarningCount++;
	}

	// if there's a grass_block_overlay, then do this equivalency
	index = findTileIndex(L"grass_block_side_overlay", false);
	assert(index >= 0);
	if (gFG.fr[index].exists) {
		// grass_block_side_overlay exists
		// check if grass_block_side does not exist
		if (!gFG.fr[findTileIndex(L"grass_block_side", 0)].exists) {
			prevProcessed = filesProcessed;
			filesProcessed -= shareFileRecords(&gFG, L"grass_block_side", L"dirt");
			if (prevProcessed != filesProcessed) {
				wprintf(L"WARNING: the grass_block_side and dirt textures do not both exist, so dirt is used for grass_block_side.\n");
				gWarningCount++;
			}
		}
	}
	else {
		// grass_block_side_overlay does not exist, so test if grass_block_side does: if so, then replace grass_block_side into grass_block_side_overlay.
		// Mineways itself should handle this substitution properly.
		prevProcessed = filesProcessed;
		filesProcessed -= shareFileRecords(&gFG, L"grass_block_side", L"grass_block_side_overlay");
		if (prevProcessed != filesProcessed) {
			wprintf(L"WARNING: the grass_block_side_overlay texture does not exist, so the grass_block_side texture replaces it.\n");
			gWarningCount++;
		}
	}

	// if there are _n and _normal textures for the same tile, favor the _n textures
	for (index = 0; index < gFG.totalTiles; index++) {
		int fullIndexN = CATEGORY_NORMALS * gFG.totalTiles + index;
		int fullIndexNormals = CATEGORY_NORMALS_LONG * gFG.totalTiles + index;
		// does _normal version exist? Deal with it
		if (gFG.fr[fullIndexNormals].exists) {
			// does _n version exist?
			if (gFG.fr[fullIndexN].exists) {
				deleteFileFromGrid(&gFG, CATEGORY_NORMALS_LONG, fullIndexNormals);
				wprintf(L"DUP WARNING: File '%s' and '%s' specify the same texture, so the second file is ignored.\n", gFG.fr[fullIndexN].fullFilename, gFG.fr[fullIndexNormals].fullFilename);
				gWarningCount++;
			} else {
				// move the _normal to _n
				copyFileRecord(&gFG, CATEGORY_NORMALS, fullIndexN, &gFG.fr[fullIndexNormals]);
				deleteFileFromGrid(&gFG, CATEGORY_NORMALS_LONG, fullIndexNormals);
			}
		}
	}
	// these should all now be cleared out
	assert(gFG.categories[CATEGORY_NORMALS_LONG] == 0);

	// get "root" of output file, i.e., without '.png', for ease of writing the PBR output files
	wcscpy_s(terrainExtOutputRoot, MAX_PATH_AND_FILE, terrainExtOutputTemplate);
	removePNGsuffix(terrainExtOutputRoot);

	// Warn of tiles where the color tile is missing but another was found for the tile type
	for (index = 0; index < gFG.totalTiles; index++) {
		// does color tile exist?
		if (!gFG.fr[index].exists) {
			// does not exist, so see if other non-color tiles do exist
			boolean foundMismatch = false;
			for (int testType = 0; testType < 4; testType++) {
				int compareIndex;
				switch (testType) {
				default:
				case 0:
					compareIndex = CATEGORY_NORMALS * gFG.totalTiles + index;
					break;
				case 1:
					compareIndex = CATEGORY_METALLIC * gFG.totalTiles + index;
					break;
				case 2:
					compareIndex = CATEGORY_EMISSION * gFG.totalTiles + index;
					break;
				case 3:
					compareIndex = CATEGORY_ROUGHNESS * gFG.totalTiles + index;
					break;
				}
				if (gFG.fr[compareIndex].exists) {
					wprintf(L"%sRGB MISSING WARNING: File '%s' exists but there is no corresponding color file.\n",
						(foundMismatch ? L"  and ":L""), gFG.fr[compareIndex].fullFilename);
					gWarningCount++;
					if (!foundMismatch) {
						foundMismatch = true;
						//wprintf(L"    Perhaps the color file is in a TGA file? You'll need to convert it to a PNG file.\n");
					}
				}
			}
		}
	}

	int redstoneDustLine0Index = findTileIndex(L"redstone_dust_line0", 0);
	int redstoneDustLine1Index = findTileIndex(L"redstone_dust_line1", 0);
	bool line0Rotated = false;
	bool line1Rotated = false;
	// write out tiles found
	for (catIndex = 0; catIndex < gFG.totalCategories; catIndex++) {
		// always export RGBA image, and others if there's content
		if ((catIndex == CATEGORY_RGBA || gFG.categories[catIndex] > 0 || gCG.categories[catIndex] > 0) &&
			((catIndex == CATEGORY_RGBA) ||
				(catIndex == CATEGORY_NORMALS) ||	// note that, above, all _normals (LONG) versions have been moved over
				(catIndex == CATEGORY_METALLIC) ||
				(catIndex == CATEGORY_EMISSION) ||
				(catIndex == CATEGORY_ROUGHNESS)))
		{
			// set output file to properly suffixed name
			wcscpy_s(terrainExtOutput, MAX_PATH_AND_FILE, terrainExtOutputRoot);

			// retrieve number of channels, and set hard-wired suffix
			int channels = gCatChannels[catIndex];
			switch (catIndex) {
			default:
				assert(0);
			case CATEGORY_RGBA:
				// no suffix, else: wcscat_s(terrainExtOutput, MAX_PATH_AND_FILE, L"");
				break;
			case CATEGORY_NORMALS:
				wcscat_s(terrainExtOutput, MAX_PATH_AND_FILE, L"_n");
				break;
			case CATEGORY_METALLIC:
				wcscat_s(terrainExtOutput, MAX_PATH_AND_FILE, L"_m");
				break;
			case CATEGORY_EMISSION:
				wcscat_s(terrainExtOutput, MAX_PATH_AND_FILE, L"_e");
				break;
			case CATEGORY_ROUGHNESS:
				wcscat_s(terrainExtOutput, MAX_PATH_AND_FILE, L"_r");
				break;
			}
			// and add ".png"
			wcscat_s(terrainExtOutput, MAX_PATH_AND_FILE, L".png");

			if (verbose)
				wprintf(L"Populating '%s' for output.\n", terrainExtOutput);

			// allocate output image and fill it up
			// already done above, so we don't have to dealloc destination_ptr = new progimage_info();

			outputXResolution = xTiles * outputTileSize;
			outputYResolution = outputYTiles * outputTileSize;

			destination_ptr->width = outputXResolution;
			destination_ptr->height = outputYResolution;

			// test if new image size to be allocated would be larger than 2^32, which is impossible to allocate (and the image would be unusable anyway)
			if (destination_ptr->width > 16384) {
				wprintf(L"***** ERROR: The tile size that is desired, %d X %d, is larger than can be allocated\n  (and likely larger than anything you would ever want to use).\n  Please run again with the '-t tileSize' option, choosing a power of two\n  value less than this, such as 256, 512, or 1024.\n",
					destination_ptr->width / 16, destination_ptr->width / 16);
				// quit!
				return 1;
			}

			if (nobase || (catIndex != CATEGORY_RGBA))
			{
				// for debug and for non-color categories, to see just the tiles placed
				destination_ptr->image_data.resize(outputXResolution * outputYResolution * channels * sizeof(unsigned char), 0x0);
			}
			else
			{
				// copy base texture over - assumes RGBA
				destination_ptr->image_data.resize(outputXResolution * outputYResolution * channels * sizeof(unsigned char), 0x0);
				copyPNG(destination_ptr, &basicterrain);
				if (verbose)
					wprintf(L"Base texture '%s' copied to output file '%s'.\n", terrainBase, terrainExtOutput);
			}

			// copy tiles found over to the output file
			for (index = 0; index < gFG.totalTiles; index++) {
				fullIndex = catIndex * gFG.totalTiles + index;
				if (gFG.fr[fullIndex].exists) {

					// read tile
					wchar_t inputFile[MAX_PATH_AND_FILE];
					wcscpy_s(inputFile, MAX_PATH_AND_FILE, gFG.fr[fullIndex].path);
					wcscat_s(inputFile, MAX_PATH_AND_FILE, gFG.fr[fullIndex].fullFilename);

					progimage_info tile;
					rc = readpng(&tile, inputFile, gCatFormat[catIndex]);
					if (rc != 0)
					{
						reportReadError(rc, inputFile);
						continue;
					}
					else {
						// check if tile has an alpha == 0; if so, it must have SBIT_DECAL or SBIT_CUTOUT_GEOMETRY set
						if (catIndex == CATEGORY_RGBA) {
							// -r option on?
							if (onlyreplace)
							{
								if (channels == 4 && !isPNGTileEmpty(destination_ptr, gTilesTable[index].txrX, gTilesTable[index].txrY))
								{
									wprintf(L"WARNING: Image '%s' was not used because there is already a image put there.\n", 
										gFG.fr[fullIndex].fullFilename);
									gWarningCount++;
									continue;
								}
							}

							if ( !(gTilesTable[index].flags & (SBIT_DECAL | SBIT_CUTOUT_GEOMETRY | SBIT_ALPHA_OVERLAY))) {
								// flag not set, so check for alpha == 0 and that it's not glass, which could be fully transparent in spots from modding
								if (checkForCutout(&tile) && wcsstr(gTilesTable[index].filename,L"glass") == NULL) {
									// if it's a grass_block_side.png file, then this should probably be also converted to a grass_block_side_overlay.png 
									if ((wcsstr(gTilesTable[index].filename, L"grass") != NULL) && (wcsstr(gTilesTable[index].filename, L"_side") != NULL)) {
										wprintf(L"WARNING: File '%s' has texels that are fully transparent, but grass block side\n  images should be in a pair: a fully opaque grass_block_side.png image, and\n  a grayscale grass with alpha cutout image grass_block_side_overlay.png\n", gFG.fr[fullIndex].fullFilename);
									}
									else {
										wprintf(L"WARNING: File '%s' has texels that are fully transparent, but the image is not\n  identified as having cutout geometry, being a decal, or being an overlay.\n", gFG.fr[fullIndex].fullFilename);
									}
									gWarningCount++;
								}
							}
							// If set, the incoming .png's black pixels should be treated as having an alpha of 0.
							// Normally Minecraft textures have alpha set properly, but this is a workaround for those that don't.
							// Not needed for newer textures - they've cleaned up their act.
							if (gTilesTable[index].flags & SBIT_BLACK_ALPHA) {
								setBlackAlphaPNGTile(chosenTile, &tile);
							}
						}
						// Note: defaults assumed:
						// metallic = 0
						// emissive = 0
						// roughness = 1
						// So textures found with all black (or all white, for roughness) will be ignored
						else if (catIndex == CATEGORY_METALLIC || catIndex == CATEGORY_EMISSION) {
							// if an image is entirely black, make it 01 black, so that Mineways will take it seriously.
							// Mineways assumes an image that is all black is not actually set, so ignores it.
							if (channelEqualsValue(&tile, 0, gCatChannels[catIndex], 0, 0)) {
								wprintf(L"WARNING: Image '%s' was not used because it is all black, 0.0, the default value for %s.\n",
									gFG.fr[fullIndex].fullFilename, (catIndex == CATEGORY_METALLIC) ? L"metallic" : L"emissive");
								deleteFileFromGrid(&gFG, catIndex, fullIndex);
								gWarningCount++;
								continue;
							}
						}
						else if (catIndex == CATEGORY_ROUGHNESS) {
							if (channelEqualsValue(&tile, 0, gCatChannels[catIndex], 255, 0)) {
								wprintf(L"WARNING: Image '%s' was not used because it is all white, 1.0, the default value for roughness.\n", 
									gFG.fr[fullIndex].fullFilename);
								deleteFileFromGrid(&gFG, catIndex, fullIndex);
								gWarningCount++;
								continue;
							}
							// if an image is entirely black, make it 01 black, so that Mineways will take it seriously.
							// Mineways assumes an image that is all black is not actually set, so ignores it.
							setBlackToNearlyBlack(&tile);
						}
						else if (catIndex == CATEGORY_NORMALS) {
							// if normal map has all the same value for the blue channel, it's likely a heightfield instead, so ignore it
							// Happens with Kellys and Vanilla RTX.

							// TODOTODO: this does not catch all tiles properly, such as oak leave cutouts. Also, need scaling factor!
							if ((heightfieldCount > normalsCount) || channelEqualsValue(&tile, 2, gCatChannels[catIndex], 0, 1)) {
								if (channelEqualsValue(&tile, 0, gCatChannels[catIndex], 0, 1)) {
									wprintf(L"WARNING: Image '%s' was not used because it seems to be a flat heightfield, no changes detected.\n",
										gFG.fr[fullIndex].fullFilename);
									deleteFileFromGrid(&gFG, catIndex, fullIndex);
									gWarningCount++;
									continue;
								}
								else {
									// convert from heightfield to normals. Should add a "what is the slope? command line argument TODO.
									if (heightfieldCount <= 0) {
										wprintf(L"Image '%s' appears to be a heightfield - TileMaker will convert this one and any others found.\n",
											gFG.fr[fullIndex].fullFilename);
									}
									else if (verbose) {
										wprintf(L"Image '%s' appears to be a heightfield - converting.\n",
											gFG.fr[fullIndex].fullFilename);
									}
									convertHeightfieldToXYZ(&tile, heightfieldScale);
									heightfieldCount++;
								}
							}
							else {
								// likely a normals map
								normalsCount++;
							}
						}

						// Check some special tiles here for possible errors
						if (catIndex == CATEGORY_RGBA) {
							// for some reason, a lot of resource packs make the redstone_dust_line*.png images horizontal, when they are vertical in
							// vanilla Minecraft. Rotate them if found to be that way. We check if there are more alpha>0 (visible) pixels horizontally
							// across the middle of the image than there are vertically down the middle of the image. If so, rotate in place.
							if (index == redstoneDustLine0Index) {
								line0Rotated = rotateTileIfHorizontal(tile);
								if (line0Rotated) {
									wprintf(L"NOTE: Image '%s' was rotated 90 degrees to be in the proper orientation for Mineways.\n",
										gFG.fr[fullIndex].fullFilename);
								}
							} else if (index == redstoneDustLine1Index) {
								line1Rotated = rotateTileIfHorizontal(tile);
								if (line1Rotated && !lineDuplicated) {
									wprintf(L"NOTE: Image '%s' was rotated 90 degrees to be in the proper orientation for Mineways.\n",
										gFG.fr[fullIndex].fullFilename);
								}
							}
						}
						else {
							// these flags are now set
							if (index == redstoneDustLine0Index && line0Rotated) {
								rotateTile(tile, channels);
							} else if (index == redstoneDustLine1Index && line1Rotated) {
								rotateTile(tile, channels);
							}
						}

						if (copyPNGTile(destination_ptr, channels, gTilesTable[index].txrX, gTilesTable[index].txrY, chosenTile, &tile, 0, 0, 16, 16, 0, 0, 0x0, (float)destination_ptr->width / (float)(trueWidth(index, tile.width, lavaFlowIndex, waterFlowIndex) * 16))) {
							// failed to copy, somehow
							assert(0);
							return 1;
						}
//wprintf(L"%s\n", gFG.fr[fullIndex].fullFilename);
						filesProcessed++;
						if (verbose)
							wprintf(L"File '%s' merged.\n", gFG.fr[fullIndex].fullFilename);
					}
					readpng_cleanup(1, &tile);
				}
			}

			////////////////////////
			// Special stuff - shulker boxes, and chests
			if (catIndex == CATEGORY_RGBA) {
				// Compute shulker box sides and bottoms, if not input

				// look through tiles missing: if shulker side and bottom tiles found, note they don't need to be generated;
				// these are not standard at all - shulkers now have their own entitities - but are left in for simplicity.
				// TODO: someday add in shulker box reader, just like chests
				int topIndex = findTileIndex(L"white_shulker_box", 0);
				int startIndex = findTileIndex(L"shulker_side_white", 0);
				int neutralSideIndex = findTileIndex(L"MW_SHULKER_SIDE", 0);
				int neutralBottomIndex = findTileIndex(L"MW_SHULKER_BOTTOM", 0);
				// where do shulker sides start?
				fullIndex = catIndex * gFG.totalTiles + startIndex + index;
				// go through 16 side and bottoms
				for (index = 0; index < 16; index++) {
					boolean sideNeeded = !gFG.fr[fullIndex].exists;
					boolean bottomNeeded = !gFG.fr[fullIndex + 16].exists;	// bottoms follow sides
					if (sideNeeded || bottomNeeded) {
						// Compute shulker box sides and bottoms, if not input

						// Take location 2,2 on the top as the "base color". Multiply by this color, divide by the white color, and then multiply the side and bottom tile by this color. Save.
						unsigned char box_color[4];
						int neutral_color[4], mult_color[4];

						// check that the entries are in tiles.h.
						// Note that we work from the output image file being generated, so we
						// don't actually ever read in any of the 3 images above - they're assumed
						// to be in the output image already (from terrainBase.png).
						assert(topIndex >= 0 && neutralSideIndex >= 0 && neutralBottomIndex >= 0);
						int pick_row = outputTileSize / 2;
						int pick_col = outputTileSize / 2;

						// compute side and bottom color
						// First, find brightest pixel
						if (index == 0) {
							// the white box needs no adjustment
							getBrightestPNGPixel(destination_ptr, channels, gTilesTable[topIndex].txrX * outputTileSize, gTilesTable[topIndex].txrY * outputTileSize, outputTileSize, box_color, &pick_col, &pick_row);
							for (j = 0; j < 4; j++) {
								neutral_color[j] = box_color[j];
								mult_color[j] = 255;
							}
						}
						else {
							getPNGPixel(destination_ptr, channels, gTilesTable[topIndex].txrX * outputTileSize + pick_col, gTilesTable[topIndex].txrY * outputTileSize + pick_row, box_color);
							for (j = 0; j < 4; j++) {
								if (neutral_color[j] > 0) {
									mult_color[j] = (255 * (int)box_color[j] / (int)neutral_color[j]);
								}
								else {
									// avoid division by zero
									mult_color[j] = 0;
								}
							}
						}
						// we now have the multiplier color, so multiply base tile by it
						if (sideNeeded) {
							copyPNGArea(destination_ptr, gTilesTable[topIndex].txrX * outputTileSize, (gTilesTable[topIndex].txrY + 4) * outputTileSize, outputTileSize, outputTileSize,
								destination_ptr, gTilesTable[neutralSideIndex].txrX * outputTileSize, gTilesTable[neutralSideIndex].txrY * outputTileSize);
							multPNGTileByColor(destination_ptr, gTilesTable[topIndex].txrX, gTilesTable[topIndex].txrY + 4, mult_color);
						}
						if (bottomNeeded) {
							copyPNGArea(destination_ptr, gTilesTable[topIndex].txrX * outputTileSize, (gTilesTable[topIndex].txrY + 5) * outputTileSize, outputTileSize, outputTileSize,
								destination_ptr, gTilesTable[neutralBottomIndex].txrX * outputTileSize, gTilesTable[neutralBottomIndex].txrY * outputTileSize);
							multPNGTileByColor(destination_ptr, gTilesTable[topIndex].txrX, gTilesTable[topIndex].txrY + 5, mult_color);
						}
					}
					topIndex++;
					fullIndex++;
				}
			}

			// Note: done for all categories
			// Test if any chest exists for this category
			if ((gCG.chestCount > 0) && (
				gCG.cr[CHEST_NORMAL + catIndex * gCG.totalTiles].exists ||
				gCG.cr[CHEST_NORMAL_DOUBLE + catIndex * gCG.totalTiles].exists ||
				gCG.cr[CHEST_NORMAL_LEFT + catIndex * gCG.totalTiles].exists ||
				gCG.cr[CHEST_NORMAL_RIGHT + catIndex * gCG.totalTiles].exists ||
				gCG.cr[CHEST_ENDER + catIndex * gCG.totalTiles].exists) 
			) {

				// Test if left chest exists. If so, we assume 1.15 content or newer is being used.
				int numChests;
				Chest* chest;
				if ( gCG.cr[CHEST_NORMAL_LEFT + catIndex*gCG.totalTiles].exists) {
					numChests = 4;
					gChest115[0].data = gNormalChest115;
					gChest115[1].data = gNormalLeftChest115;
					gChest115[2].data = gNormalRightChest115;
					gChest115[3].data = gEnderChest115;
					chest = gChest115;
				}
				else {
					numChests = 3;
					gChest114[0].data = gNormalChest;
					gChest114[1].data = gNormalDoubleChest;
					gChest114[2].data = gEnderChest;
					chest = gChest114;
				}

				// Now for the chests, if any. Look for each chest image file, and use bits as found
				for (i = 0; i < numChests; i++) {
					// single chest, double chest, ender chest in \textures\entity\chest
					Chest* pChest = &chest[i];

					//findChestIndex()
					index = -1;
					for (j = 0; j < gCG.totalTiles; j++) {
						if (_wcsicmp(pChest->wname, gChestNames[j]) == 0) {
							index = j;
							break;
						}
					}
					assert(index >= 0);

					if (gCG.cr[index + catIndex * gCG.totalTiles].exists) {

						// read chest and process

						// chests are normally found in \assets\minecraft\textures\entity\chest
						wchar_t chestFile[MAX_PATH_AND_FILE];
						wcscpy_s(chestFile, MAX_PATH_AND_FILE, gCG.cr[index + catIndex * gCG.totalTiles].path);
						wcscat_s(chestFile, MAX_PATH_AND_FILE, gCG.cr[index + catIndex * gCG.totalTiles].fullFilename);

						// note: we really do need to declare this each time, otherwise you get odd leftovers for some reason.
						progimage_info chestImage;
						rc = readpng(&chestImage, chestFile, gCatFormat[catIndex]);
						if (rc != 0)
						{
							// file not found
							reportReadError(rc, chestFile);
							// It's important to note the count is down,
							// as this determines whether anything was done for the category.
							deleteChestFromGrid( &gCG, catIndex, index + catIndex * gCG.totalTiles);
							// try next chest
							continue;
						}
						// chests must be powers of two
						if (testFileForPowerOfTwo(chestImage.width, chestImage.height, chestFile, false)) {
							allChests = false;
							readpng_cleanup(1, &chestImage);
							// It's important to note the count is down,
							// as this determines whether anything was done for the category.
							deleteChestFromGrid(&gCG, catIndex, index + catIndex * gCG.totalTiles);
							continue;
						}

						// if we got this far, at least one chest was found
						anyChests = true;

						// from size figure out scaling factor from chest to terrainExt.png

						// loop through bits to copy
						for (int copyIndex = 0; copyIndex < pChest->numCopies; copyIndex++) {
							// clear tile if it's a new one (don't wipe out previous copies)
							if (catIndex == CATEGORY_RGBA &&
								((copyIndex == 0) ||
									(pChest->data[copyIndex].txrX != pChest->data[copyIndex - 1].txrX) ||
									(pChest->data[copyIndex].txrY != pChest->data[copyIndex - 1].txrY))) {
								makePNGTileEmpty(destination_ptr, pChest->data[copyIndex].txrX, pChest->data[copyIndex].txrY);
							}

							// copy from area to area, scaling as needed
							copyPNGTile(destination_ptr, channels, pChest->data[copyIndex].txrX, pChest->data[copyIndex].txrY, 0,
								&chestImage,
								pChest->data[copyIndex].toX, pChest->data[copyIndex].toY,
								pChest->data[copyIndex].toX + pChest->data[copyIndex].sizeX, pChest->data[copyIndex].toY + pChest->data[copyIndex].sizeY,
								pChest->data[copyIndex].fromX, pChest->data[copyIndex].fromY,
								pChest->data[copyIndex].flags,
								(float)destination_ptr->width / (256.0f * (float)chestImage.width / (float)pChest->defaultResX));	// default is 256 / 64 * 4 or 128 * 2
						}
						filesProcessed++;
//wprintf(L"%s\n", gCG.cr[index + catIndex * gCG.totalTiles].fullFilename);
						if (verbose)
							wprintf(L"Chest file '%s' merged.\n", chestFile);

						// clean up
						readpng_cleanup(1, &chestImage);
					}
				}

				// if solid is desired, blend final result and replace in-place
				if (solid || solidcutout)
				{
					for (i = 0; i < TOTAL_TILES; i++)
					{
						makeSolidTile(destination_ptr, i, solid);
					}
				}
			}

			if (verbose)
				wprintf(L"Opening '%s' for output.\n", terrainExtOutput);

			// write out the result if anything was actually written
			if (catIndex == CATEGORY_RGBA || gFG.categories[catIndex] > 0 || gCG.categories[catIndex] > 0) {
				rc = writepng(destination_ptr, channels, terrainExtOutput);
				if (rc != 0)
				{
					reportReadError(rc, terrainExtOutput);
					// quit
					return 1;
				}
				writepng_cleanup(destination_ptr);
				if (verbose)
					wprintf(L"New texture '%s' created.\n", terrainExtOutput);
			}
			else {
				wprintf(L"WARNING: New texture '%s' was not created, as all input textures were found to be unusable.\n", terrainExtOutput);
				gWarningCount++;
			}
		}
	}

	// was there a mix of heightfields and normal maps?
	if (heightfieldCount > 0 && normalsCount > 0) {
		wprintf(L"WARNING: %d heightfields and %d normal maps detected.\n  TileMaker does its best to convert heightfields to normal maps, but you should double check.\n", heightfieldCount, normalsCount);
		gWarningCount++;
	}

	// warn user that nothing was done
	// 3 is the number of MW_*.png files that come with TileMaker
	if (gFG.fileCount <= 3 && !anyChests) {
		wprintf(L"WARNING: It's likely no real work was done. To use TileMaker, you need to put\n  all the images from your resource pack's 'assets\\minecraft\\textures'\n  block and entity\\chest directories into TileMaker's 'blocks' and\n  'blocks\\chest' directories. See http://mineways.com for more about TileMaker.\n");
		gWarningCount++;
	}
	else if (!allChests) {
		wprintf(L"WARNING: Not all relevant chest images were found in the 'blocks\\chest' directory.\n  TileMaker worked, but you can add chest images if you like. You can provide\n  the images normal.png, normal_left.png, normal_right.png\n  (or normal_double.png for 1.14 and earlier), and ender.png.\n  Copy these texture resources from Minecraft's jar-file\n  'assets\\minecraft\\textures\\entity\\chest' directory to\n  Mineways' subdirectory blocks\\chest.\n");
		gWarningCount++;
	}

	if (gErrorCount)
		wprintf(L"\nERROR SUMMARY:\n%s\n", gConcatErrorString);

	if (gErrorCount)
		wprintf(L"Error count: %d error%S generated.\n", gErrorCount, (gErrorCount == 1) ? " was" : "s were");

	wprintf(L"TileMaker summary: %d relevant PNG files discovered and %d of these were used.\n", filesFound, filesProcessed);
	if (filesFound > filesProcessed) {
		wprintf(L"    This difference of %d files means that some files were found and not used.\n", filesFound - filesProcessed);
		wprintf(L"    Look through the warnings and repair, rename, or delete those files you do not want to use.\n");
	}
	return 0;
}

void printHelp()
{
	wprintf(L"TileMaker version %s\n", VERSION_STRING);
	wprintf(L"usage: TileMaker [-v] [-i terrainBase.png] [-d blocks] [-o terrainExt.png]\n        [-t tileSize] [-c chosenTile] [-nb] [-nt] [-r] [-m] [-s] [-S] [-h #]\n");
	wprintf(L"  -v - verbose, explain everything going on. Default: display only warnings and errors.\n");
	wprintf(L"  -i terrainBase.png - image containing the base set of terrain blocks\n    (includes special chest tiles). Default is 'terrainBase.png'.\n");
	wprintf(L"  -d blocks - directory of block textures to overlay on top of the base.\n    Default directory is 'blocks'. Can be set multiple times to include\n    multiple directories.\n");
	//wprintf(L"  -z zip - optional directory where a texture resource pack has been unzipped.\n");
	wprintf(L"  -o terrainExt.png - the resulting terrain image, used by Mineways. Default is\n    terrainExt.png.\n");
	wprintf(L"  -t tileSize - force a given (power of 2) tile size for the resulting terrainExt.png\n    file, e.g. 32, 128. Useful for zooming or making a 'draft quality'\n    terrainExt.png. If not set, largest tile found is used.\n");
	wprintf(L"  -h # - scale any normalmap heightfields by this value. Default is 0.5.\n");
	wprintf(L"  -c chosenTile - for tiles with multiple versions in a vertical strip,\n     (e.g. water, lava, portal), choose which tile to use. 0 means topmost, 1 second from top, 2 etc.;\n    -1 bottommost, -2 next to bottom.\n");
	wprintf(L"  -nb - no base; the base texture terrainBase.png is not read. This option is\n    good for seeing what images are in the blocks directory, as these are\n    what get put into terrainExt.png.\n");
	wprintf(L"  -nt - no tile directory; don't read in any images in the 'blocks' directory,\n    just the base texture is read in.\n");
	wprintf(L"  -r - replace (from the 'blocks' directories) only those tiles not in the base\n    texture. This is a way of extending a base texture to new versions of Mineways.\n");
	wprintf(L"  -m - to report all missing tiles, ones that Mineways uses but were not in the\n    tiles directory.\n");
	wprintf(L"  -s - take the average color of the incoming tile and output this solid color.\n");
	wprintf(L"  -S - as above, but preserve the cutout transparent areas.\n");
}

// Shares textures found, as possible. If both or neither exist, nothing to do.
// If only one exists, copy it to the other.
int shareFileRecords(FileGrid* pfg, wchar_t* tile1, wchar_t* tile2)
{
	int index1 = findTileIndex(tile1, false);
	int index2 = findTileIndex(tile2, false);

	if (index1 < 0) {
		wprintf(L"INTERNAL WARNING: shareFileRecords cannot find tile name '%s'.\n", tile1);
		assert(0);
		gWarningCount++;
		return 0;
	}
	if (index2 < 0) {
		wprintf(L"INTERNAL WARNING: shareFileRecords cannot find tile name '%s'.\n", tile2);
		assert(0);
		gWarningCount++;
		return 0;
	}

	int shareCount = 0;
	for (int category = 0; category < pfg->totalCategories; category++) {
		int fullIndex1 = category * pfg->totalTiles + index1;
		int fullIndex2 = category * pfg->totalTiles + index2;
		if (pfg->fr[fullIndex1].exists) {
			// first exists, does second?
			if (!pfg->fr[fullIndex2].exists) {
				// copy first to second
				copyFileRecord(pfg, category, fullIndex2, &pfg->fr[fullIndex1]);
				shareCount++;
			}
		}
		else {
			// first does not exist, does second?
			if (pfg->fr[fullIndex2].exists) {
				// copy second to first
				copyFileRecord(pfg, category, fullIndex1, &pfg->fr[fullIndex2]);
				shareCount++;
			}
		}
	}
	return shareCount;
}

int checkFileWidth(FileRecord *pfr, int overlayTileSize, boolean square, boolean isFileGrid, int index, int lavaFlowIndex, int waterFlowIndex) {
	// check that width and height make sense.
	wchar_t inputFile[MAX_PATH_AND_FILE];
	wcscpy_s(inputFile, MAX_PATH_AND_FILE, pfr->path);
	wcscat_s(inputFile, MAX_PATH_AND_FILE, pfr->fullFilename);

	// read tile header
	progimage_info tile;
	LodePNGColorType colortype;
	int rc = readpngheader(&tile, inputFile, colortype);
	if (rc != 0)
	{
		reportReadError(rc, inputFile);
		return overlayTileSize;	// no change
	}

	if (testFileForPowerOfTwo(tile.width, tile.height, pfr->fullFilename, square)) {
		// error - should delete this tile as it's unusable
		return 0;
	}
	else {
		// usable width

		// Check when this is a file grid.
		// For water_flow and lava_flow, the image width is twice normal, so halve it for the width we actually use.
		if (isFileGrid) {
			tile.width = trueWidth(index, tile.width, lavaFlowIndex, waterFlowIndex);
		}

		if (overlayTileSize < tile.width)
		{
			overlayTileSize = tile.width;
		}
	}
	return overlayTileSize;
}

int trueWidth(int index, int width, int lavaFlowIndex, int waterFlowIndex)
{
	return (index == lavaFlowIndex || index == waterFlowIndex) ? width / 2 : width;
}

int testFileForPowerOfTwo(int width, int height, const wchar_t* cFileName, bool square)
{
	int fail_code = 0;
	if (fmod(log2((float)(width)), 1.0f) != 0.0f) {
		wsprintf(gErrorString, L"***** ERROR: file '%s'\n  has a width of %d that is not a power of two.\n  This will cause copying errors, so TileMaker ignores it.\n  We recommend you remove or resize this file.\n", cFileName, width);
		saveErrorForEnd();
		gErrorCount++;
		fail_code = 1;
	}
	// check if height is not a power of two AND is not a multiple of the width.
	// if not square (i.e., a chest), the height may be half that of the width.
	else if (fmod((float)(height) / (float)width, square ? 1.0f : 0.5f) != 0.0f) {
		wsprintf(gErrorString, L"***** ERROR: file '%s'\n  has a height of %d that is not %s its width of %d.\n  This will cause copying errors, so TileMaker ignores it.\n  We recommend you remove or resize this file.\n", cFileName, height, square ? L"equal to" : L"a multiple of", width);
		saveErrorForEnd();
		gErrorCount++;
		fail_code = 1;
	}
	// not sure I actually need this test - the one above should cover it, I think - but left in, just in case
	if (square && width > height && fail_code == 0) {
		wsprintf(gErrorString, L"***** ERROR: file '%s'\n  has a height of %d that is less than its width of %d.\n  This will cause copying errors, so TileMaker ignores it.\n  We recommend you remove or resize this file.\n", cFileName, height, width);
		saveErrorForEnd();
		gErrorCount++;
		fail_code = 1;
	}
	return fail_code;
}


//====================== statics ==========================

static void reportReadError(int rc, const wchar_t* filename)
{
	switch (rc) {
	case 1:
		wsprintf(gErrorString, L"***** ERROR [%s] is not a PNG file: incorrect signature.\n", filename);
		break;
	case 2:
		wsprintf(gErrorString, L"***** ERROR [%s] has bad IHDR (libpng longjmp).\n", filename);
		break;
	case 4:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - insufficient memory.\n", filename);
		break;
	case 57:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - invalid CRC. Try saving this PNG using some program, e.g., IrfanView.\n", filename);
		break;
	case 63:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - chunk too long.\n", filename);
		break;
	case 78:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - file not found or could not be read.\n", filename);
		break;
	case 79:
		wsprintf(gErrorString, L"***** ERROR [%s] write failed - directory not found. Please create the directory.\n", filename);
		break;
	case 83:
		wsprintf(gErrorString, L"***** ERROR [%s] allocation failed. Image file is too large for your system to handle?\n", filename);
		break;
	default:
		wsprintf(gErrorString, L"***** ERROR [%s] read failed - unknown readpng_init() error.\n", filename);
		break;
	}
	saveErrorForEnd();
	gErrorCount++;

	if (rc != 78 && rc != 79) {
		wsprintf(gErrorString, L"Often this means the PNG file has some small bit of information that TileMaker cannot\n  handle. You might be able to fix this error by opening this PNG file in\n  Irfanview or other viewer and then saving it again. This has been known to clear\n  out any irregularity that TileMaker's somewhat-fragile PNG reader dies on.\n");
	}
	saveErrorForEnd();
}

static void saveErrorForEnd()
{
	wprintf(gErrorString);
	wcscat_s(gConcatErrorString, CONCAT_ERROR_LENGTH, L"  ");
	wcscat_s(gConcatErrorString, CONCAT_ERROR_LENGTH, gErrorString);
}

//================================ Image Manipulation ====================================

// if color is black, set alpha to 0 - meant for RGBA only
static void setBlackAlphaPNGTile(int chosenTile, progimage_info* src)
{
	unsigned long row, col, src_start;
	unsigned char* src_data;
	unsigned long tileSize;

	//tile matches destination tile size - copy
	tileSize = src->width;

	// which tile to use: get the bottommost
	src_start = computeVerticalTileOffset(src, chosenTile);
	src_data = &src->image_data[0] + (src_start * src->width) * 4;

	for (row = 0; row < tileSize; row++)
	{
		for (col = 0; col < tileSize; col++)
		{
			// Treat alpha == 0 as clear - nicer to set to black. This happens with fire,
			// and the flowers and double flowers have junk in the RGB channels where alpha == 0.
			if (src_data[0] == 0 && src_data[1] == 0 && src_data[2] == 0)
			{
				src_data[3] = 0;
			}
			src_data += 4;
		}
	}
}

// meant only for one-channel grayscale
static int setBlackToNearlyBlack(progimage_info* src)
{
	// look at all data: black?
	int row, col;
	unsigned char* src_data = &src->image_data[0];
	for (row = 0; row < src->height; row++)
	{
		for (col = 0; col < src->width; col++)
		{
			if (*src_data++ != 0)
			{
				return 0;
			}
		}
	}
	// survived - it's all black, so set it all to nearly black
	src_data = &src->image_data[0];
	for (row = 0; row < src->height; row++)
	{
		for (col = 0; col < src->width; col++)
		{
			*src_data++ = 1;
		}
	}
	return 1;
};


// Give the destination image, the tile location on that destination (multiplied by destination width/16),
// the source image, the upper left and lower right destination pixels, the upper left source location, any flags,
// and the zoom factor for going from source to destination - zoom > 1 means destination is larger, zoom < 1 means source is larger
static int copyPNGTile(progimage_info* dst, int channels, unsigned long dst_x, unsigned long dst_y, unsigned long chosenTile, progimage_info* src,
	unsigned long dst_x_lo, unsigned long dst_y_lo, unsigned long dst_x_hi, unsigned long dst_y_hi, unsigned long src_x_lo, unsigned long src_y_lo, unsigned long flags, float zoom)
{
	unsigned long row, col, src_start;
	int ic;
	unsigned char* dst_data;
	unsigned char color[4];
	unsigned long tileSize, zoomTileSize;
	unsigned long zoomrow, zoomcol, izoom;
	unsigned int sum[4];
	unsigned long zoom2;

	if (zoom == 1.0f) // dst->width == src->width * 16 )
	{
		//tile matches destination tile size - copy
		tileSize = dst->width / 16;

		// 16x16 is assumed, so scale up all our lo and hi values if not the case
		if (tileSize != 16) {
			int rescale = tileSize / 16;
			dst_x_lo *= rescale;
			dst_y_lo *= rescale;
			dst_x_hi *= rescale;
			dst_y_hi *= rescale;
			src_x_lo *= rescale;
			src_y_lo *= rescale;
		}

		assert(dst_y * tileSize < (unsigned long)dst->height);	// destination can't hold tile

		// which tile to use: get the bottommost
		src_start = computeVerticalTileOffset(src, chosenTile);

		for (row = dst_y_lo; row < dst_y_hi; row++)
		{
			dst_data = &dst->image_data[0] + ((dst_y * tileSize + row) * dst->width + dst_x * tileSize + dst_x_lo) * channels;
			for (col = dst_x_lo; col < dst_x_hi; col++)
			{
				// Treat alpha == 0 as clear - nicer to set to black. This happens with fire,
				// and the flowers and double flowers have junk in the RGB channels where alpha == 0.
				// negate column and row, as Minecraft stores these reversed (left and right chest, basically)
				getPNGPixel(src, channels,
					src_x_lo + ((flags & 0x1) ? (dst_x_hi - col - 1) : (col - dst_x_lo)),
					src_start + src_y_lo + ((flags & 0x2) ? (dst_y_hi - row - 1) : (row - dst_y_lo)), color);
				if (channels == 4 && color[3] == 0)
				{
					memset(dst_data, 0, channels);
				}
				else
				{
					memcpy(dst_data, color, channels);
				}
				dst_data += channels;
			}
		}
	}
	else if (zoom > 1.0f) // dst->width > src->width * 16 )
	{
		// magnify
		tileSize = (int)((float)dst->width / zoom) / 16;

		if (tileSize <= 0) {
			wsprintf(gErrorString, L"***** ERROR: somehow, the largest tile size is computed to be %d - this needs to be a positive number.\n", tileSize);
			saveErrorForEnd();
			gErrorCount++;
			return 1;
		}

		// 16x16 is assumed, so scale up all our lo and hi values if not the case
		if (tileSize != 16) {
			float rescale = (float)tileSize / 16;
			dst_x_lo = (int)((float)dst_x_lo * rescale);
			dst_y_lo = (int)((float)dst_y_lo * rescale);
			dst_x_hi = (int)((float)dst_x_hi * rescale);
			dst_y_hi = (int)((float)dst_y_hi * rescale);
			src_x_lo = (int)((float)src_x_lo * rescale);
			src_y_lo = (int)((float)src_y_lo * rescale);
		}

		// could check that zoom factor is an integer (really should be a power of two)
		izoom = (int)zoom;
		zoomTileSize = izoom * tileSize;

		// which tile to use: get the bottommost
		src_start = computeVerticalTileOffset(src, chosenTile);

		for (row = dst_y_lo; row < dst_y_hi; row++)
		{
			for (col = dst_x_lo; col < dst_x_hi; col++)
			{
				// Treat alpha == 0 as clear - nicer to set to black. This happens with fire,
				// and the flowers and double flowers have junk in the RGB channels where alpha == 0.
				getPNGPixel(src, channels,
					src_x_lo + ((flags & 0x1) ? (dst_x_hi - col - 1) : (col - dst_x_lo)),
					src_start + src_y_lo + ((flags & 0x2) ? (dst_y_hi - row - 1) : (row - dst_y_lo)), color);
				if (channels == 4 && color[3] == 0)
				{
					color[0] = color[1] = color[2] = 0;
				}
				for (zoomrow = 0; zoomrow < izoom; zoomrow++)
				{
					dst_data = &dst->image_data[0] + ((dst_y * zoomTileSize + row * izoom + zoomrow) * (unsigned long)dst->width + dst_x * zoomTileSize + col * izoom) * channels;
					for (zoomcol = 0; zoomcol < izoom; zoomcol++)
					{
						memcpy(dst_data, color, channels);
						dst_data += channels;
					}
				}
			}
		}
	}
	else // zoom < 1.0f
	{
		// minify
		tileSize = dst->width / 16;

		// 16x16 is assumed, so scale up all our lo and hi values if not the case
		if (tileSize != 16) {
			int rescale = tileSize / 16;
			dst_x_lo *= rescale;
			dst_y_lo *= rescale;
			dst_x_hi *= rescale;
			dst_y_hi *= rescale;
			src_x_lo *= rescale;
			src_y_lo *= rescale;
		}

		// check that zoom factor is an integer (really should be a power of two)
		izoom = (int)(1.0f / zoom);	// src->width * 16 / dst->width;
		zoom2 = izoom * izoom;

		// which tile to use: get the bottommost
		src_start = computeVerticalTileOffset(src, chosenTile);

		for (row = dst_y_lo; row < dst_y_hi; row++)
		{
			for (col = dst_x_lo; col < dst_x_hi; col++)
			{
				sum[0] = sum[1] = sum[2] = sum[3] = 0;
				for (zoomrow = 0; zoomrow < izoom; zoomrow++)
				{
					for (zoomcol = 0; zoomcol < izoom; zoomcol++)
					{
						// Treat alpha == 0 as clear - nicer to set to black. This happens with fire,
						// and the flowers and double flowers have junk in the RGB channels where alpha == 0.
						getPNGPixel(src, channels, (col + src_x_lo - dst_x_lo) * izoom + zoomcol, (row + src_y_lo - dst_y_lo) * izoom + zoomrow, color);
						if (channels == 4 && color[3] == 0)
						{
							color[0] = color[1] = color[2] = 0;
						}
						for (ic = 0; ic < channels; ic++)
						{
							sum[ic] += (unsigned int)color[ic];
						}
					}
				}
				dst_data = &dst->image_data[0] + ((dst_y * tileSize + row) * dst->width + dst_x * tileSize + col) * channels;
				for (zoomcol = 0; zoomcol < izoom; zoomcol++)
				{
					for (ic = 0; ic < channels; ic++)
					{
						dst_data[ic] = (unsigned char)(sum[ic] / zoom2);
					}
				}
			}
		}
	}
	return 0;
}

// Meant for RGBA only!
static void multPNGTileByColor(progimage_info* dst, int dst_x, int dst_y, int* color)
{
	unsigned long row, col, i;
	unsigned char* dst_data;

	unsigned long tileSize = dst->width / 16;

	for (row = 0; row < tileSize; row++)
	{
		dst_data = &dst->image_data[0] + ((dst_y * tileSize + row) * dst->width + dst_x * tileSize) * 4;
		for (col = 0; col < tileSize; col++)
		{
			for (i = 0; i < 3; i++) {
				int val = color[i] * (int)*dst_data / 255;
				*dst_data++ = (val > 255) ? 255 : (unsigned char)val;
			}
			// ignore alpha - assumed solid, and that we wouldn't want to multiply anyway
			dst_data++;
		}
	}
}

static int computeVerticalTileOffset(progimage_info* src, int chosenTile)
{
	int offset = 0;
	if (chosenTile >= 0)
		offset = src->width * chosenTile;
	else
		offset = src->height + chosenTile * src->width;

	if (offset < 0)
	{
		offset = 0;
	}
	else if (offset >= src->height)
	{
		offset = src->height - src->width;
	}

	return offset;
}

static void getPNGPixel(progimage_info* src, int channels, int col, int row, unsigned char* color)
{
	unsigned char* src_data;

	//if ( ( src->color_type == PNG_COLOR_TYPE_RGB_ALPHA ) || ( src->color_type == PNG_COLOR_TYPE_PALETTE ) || ( src->color_type == PNG_COLOR_TYPE_GRAY_ALPHA ) )
	//if ( src->channels == 4 )
	//{

	// LodePNG does all the work for us, going to RGBA by default:
	src_data = &src->image_data[0] + (row * src->width + col) * channels;
	memcpy(color, src_data, channels);

	//}
	//else if ( ( src->color_type == PNG_COLOR_TYPE_RGB ) || (src->color_type == PNG_COLOR_TYPE_GRAY) )
	//else if ( src->channels == 3 )
	//{
	//	src_data = &src->image_data[0] + ( row * src->width + col ) * 3;
	//	memcpy(color,src_data,3);
	//	color[3] = 255;	// alpha always 1.0
	//}
	//else if ( src->channels == 2 )
	//{
	//	// just a guess
	//	src_data = &src->image_data[0] + ( row * src->width + col ) * 2;
	//	color[0] = color[1] = color[2] = *src_data++;
	//	color[3] = *src_data;
	//}
	//else if ( src->channels == 1 )
	//{
	//	// just a guess
	//	src_data = &src->image_data[0] + ( row * src->width + col );
	//	color[0] = color[1] = color[2] = *src_data;
	//	color[3] = 255;	// alpha always 1.0
	//}
	////else if ( src->color_type == PNG_COLOR_TYPE_GRAY )
	////{
	////	// I'm guessing there's just one channel...
	////	src_data = &src->image_data[0] + row * src->width + col;
	////	color[0] = color[1] = color[2] = *src_data;
	////	color[3] = 255;	// alpha always 1.0
	////}
	//else
	//{
	//	// unknown type
	//	assert(0);
	//}
}

static void getBrightestPNGPixel(progimage_info* src, int channels, unsigned long col, unsigned long row, unsigned long res, unsigned char* color, int* locc, int* locr)
{
	unsigned long r, c;
	int i;
	unsigned char testColor[4];
	int maxSum, testSum;
	maxSum = -1;
	color[3] = 255;
	for (r = 0; r < res; r++) {
		for (c = 0; c < res; c++) {
			getPNGPixel(src, channels, col + c, row + r, testColor);
			testSum = (int)testColor[0] + (int)testColor[1] + (int)testColor[2];
			if (testSum > maxSum) {
				maxSum = testSum;
				*locr = r;
				*locc = c;
				for (i = 0; i < ((channels > 3) ? 3 : channels); i++) {
					color[i] = testColor[i];
				}
			}
		}
	}
}

// meant for RGBA only
static int isPNGTileEmpty(progimage_info* dst, int dst_x, int dst_y)
{
	// look at all data: are all alphas 0?
	unsigned long tileSize = dst->width / 16;
	unsigned char* dst_data;
	unsigned long row, col;

	for (row = 0; row < tileSize; row++)
	{
		dst_data = &dst->image_data[0] + ((dst_y * tileSize + row) * dst->width + dst_x * tileSize) * 4;
		for (col = 0; col < tileSize; col++)
		{
			if (dst_data[3] != 0)
			{
				return 0;
			}
			dst_data += 4;
		}
	}
	return 1;
};

// meant for RGBA only
static void makePNGTileEmpty(progimage_info* dst, int dst_x, int dst_y)
{
	// look at all data: are all alphas 0?
	unsigned long tileSize = dst->width / 16;
	unsigned int* dst_data;
	unsigned long row, col;

	for (row = 0; row < tileSize; row++)
	{
		dst_data = ((unsigned int*)&dst->image_data[0]) + ((dst_y * tileSize + row) * dst->width + dst_x * tileSize);
		for (col = 0; col < tileSize; col++)
		{
			*dst_data++ = 0x0;
		}
	}
};

// assumes we want to match the source to fit the destination
static void copyPNG(progimage_info* dst, progimage_info* src)
{
	unsigned long row, col, zoomrow, zoomcol;
	unsigned char* dst_data;
	unsigned char* src_data, * src_loc;
	unsigned long zoom, zoom2;
	unsigned int sumR, sumG, sumB, sumA;
	unsigned long numrow, numcol;

	if (dst->width == src->width)
	{
		memcpy(&dst->image_data[0], &src->image_data[0], src->width * src->height * 4);
	}
	else if (dst->width > src->width)
	{
		// magnify

		// check that zoom factor is an integer (really should be a power of two)
		assert((dst->width / src->width) == (float)((int)(dst->width / src->width)));
		zoom = dst->width / src->width;
		assert((unsigned long)dst->height >= src->height * zoom);

		src_data = &src->image_data[0];
		numrow = (unsigned long)src->height;
		numcol = (unsigned long)src->width;
		for (row = 0; row < numrow; row++)
		{
			dst_data = &dst->image_data[0] + row * (unsigned long)dst->width * zoom * 4;
			for (col = 0; col < numcol; col++)
			{
				for (zoomrow = 0; zoomrow < zoom; zoomrow++)
				{
					for (zoomcol = 0; zoomcol < zoom; zoomcol++)
					{
						memcpy(dst_data + (zoomrow * (unsigned long)dst->width + zoomcol) * 4, src_data, 4);
					}
				}
				dst_data += zoom * 4;	// move to next column
				src_data += 4;
			}
		}
	}
	else
	{
		// minify: squish source into destination

		// check that zoom factor is an integer (really should be a power of two)
		assert((src->width / dst->width) == (float)((int)(src->width / dst->width)));
		zoom = src->width / dst->width;
		assert((unsigned long)dst->height * zoom >= (unsigned long)src->height);
		zoom2 = zoom * zoom;

		dst_data = &dst->image_data[0];
		numrow = (unsigned long)dst->height;
		numcol = (unsigned long)dst->width;
		for (row = 0; row < numrow; row++)
		{
			src_data = &src->image_data[0] + row * src->width * zoom * 4;
			for (col = 0; col < numcol; col++)
			{
				sumR = sumG = sumB = sumA = 0;
				for (zoomrow = 0; zoomrow < zoom; zoomrow++)
				{
					for (zoomcol = 0; zoomcol < zoom; zoomcol++)
					{
						src_loc = src_data + (zoomrow * src->width + zoomcol) * 4;
						sumR += (unsigned int)*src_loc++;
						sumG += (unsigned int)*src_loc++;
						sumB += (unsigned int)*src_loc++;
						sumA += (unsigned int)*src_loc++;
					}
				}
				*dst_data++ = (unsigned char)(sumR / zoom2);
				*dst_data++ = (unsigned char)(sumG / zoom2);
				*dst_data++ = (unsigned char)(sumB / zoom2);
				*dst_data++ = (unsigned char)(sumA / zoom2);
				// move to next column
				src_data += zoom * 4;
			}
		}
	}
}

static void copyPNGArea(progimage_info* dst, unsigned long dst_x_min, unsigned long dst_y_min, unsigned long size_x, unsigned long size_y, progimage_info* src, int src_x_min, int src_y_min)
{
	unsigned long row;
	unsigned long dst_offset, src_offset;

	for (row = 0; row < size_y; row++)
	{
		dst_offset = ((dst_y_min + row) * dst->width + dst_x_min) * 4;
		src_offset = ((src_y_min + row) * src->width + src_x_min) * 4;
		memcpy(&dst->image_data[dst_offset], &src->image_data[src_offset], size_x * 4);
	}
}


static void makeSolidTile(progimage_info* dst, int chosenTile, int solid)
{
	unsigned long row, col, dst_offset;
	unsigned char* dst_data;

	unsigned char color[4];
	double dcolor[4];
	double sum_color[3], sum;
	unsigned long tileSize;

	tileSize = dst->width / 16;

	dst_offset = ((chosenTile % 16) * tileSize + (int)(chosenTile / 16) * tileSize * dst->width) * 4;

	sum_color[0] = sum_color[1] = sum_color[2] = sum = 0;

	for (row = 0; row < tileSize; row++)
	{
		dst_data = &dst->image_data[0] + dst_offset + row * dst->width * 4;
		for (col = 0; col < tileSize; col++)
		{
			// linearize; really we should use sRGB conversions, but this is close enough
			dcolor[0] = pow(*dst_data++ / 255.0, 2.2);
			dcolor[1] = pow(*dst_data++ / 255.0, 2.2);
			dcolor[2] = pow(*dst_data++ / 255.0, 2.2);
			dcolor[3] = *dst_data++ / 255.0;
			sum_color[0] += dcolor[0] * dcolor[3];
			sum_color[1] += dcolor[1] * dcolor[3];
			sum_color[2] += dcolor[2] * dcolor[3];
			sum += dcolor[3];
		}
	}
	if (sum > 0) {
		// gamma correct and then unassociate for PNG storage
		color[0] = (unsigned char)(0.5 + 255.0 * pow((sum_color[0] / sum), 1 / 2.2));
		color[1] = (unsigned char)(0.5 + 255.0 * pow((sum_color[1] / sum), 1 / 2.2));
		color[2] = (unsigned char)(0.5 + 255.0 * pow((sum_color[2] / sum), 1 / 2.2));
		color[3] = 255;
		for (row = 0; row < tileSize; row++)
		{
			dst_data = &dst->image_data[0] + dst_offset + row * dst->width * 4;
			for (col = 0; col < tileSize; col++)
			{
				// if we want solid blocks (not cutouts), or we do want solid cutouts
				// and the alpha is not fully transparent, then save new color.
				if (solid || (dst_data[3] == 255)) {
					// solid, or cutout is fully opaque
					*dst_data++ = color[0];
					*dst_data++ = color[1];
					*dst_data++ = color[2];
					*dst_data++ = color[3];
				}
				else if (!solid || (dst_data[3] != 255)) {
					// cutout mode, and partial alpha
					*dst_data++ = color[0];
					*dst_data++ = color[1];
					*dst_data++ = color[2];
					// don't touch alpha, leave it unassociated
					dst_data++;
				}
				else {
					// skip pixel, as it's fully transparent
					dst_data += 4;
				}
			}
		}
	}
}

// does any pixel have an alpha of 0?
static int checkForCutout(progimage_info* dst)
{
	int row, col;
	unsigned char* dst_data = &dst->image_data[0];

	for (row = 0; row < dst->height; row++)
	{
		for (col = 0; col < dst->width; col++)
		{
			if (dst_data[3] == 0)
			{
				return 1;
			}
			dst_data += 4;
		}
	}
	return 0;
};

// assumes 3 channels
// could return largest difference in heights. If 0, then the resulting normal map is flat
static int convertHeightfieldToXYZ(progimage_info* src, float heightfieldScale)
{
	int row, col;
	progimage_info* phf = allocateGrayscaleImage(src);
	copyOneChannel(phf, CHANNEL_RED, src, LCT_RGB);
	unsigned char* phf_data = &phf->image_data[0];
	unsigned char* src_data = &src->image_data[0];

	for (row = 0; row < phf->height; row++)
	{
		int trow = (row + phf->height - 1) % phf->height;
		int brow = (row + phf->height + 1) % phf->height;
		for (col = 0; col < phf->width; col++)
		{
			int lcol = (col + phf->width - 1) % phf->width;
			int rcol = (col + phf->width + 1) % phf->width;
			// Won't swear to this conversion being quite right. From Real-Time Rendering referencing an article.
			float x = heightfieldScale * (phf_data[row * phf->width + lcol] - phf_data[row * phf->width + rcol]) / 255.0f;
			float y = heightfieldScale * (phf_data[trow * phf->width + col] - phf_data[brow * phf->width + col]) / 255.0f;
			float length = (float)sqrt(x * x + y * y + 1.0f);
			// Basically, map from XYZ [-1,1] to RGB.
			*src_data++ = (unsigned char)((1.0f + x / length) * 127.5f);
			*src_data++ = (unsigned char)((1.0f + y / length) * 127.5f);
			*src_data++ = (unsigned char)((1.0f + 1.0 / length) * 127.5f);
		}
	}
	return 1;
}

// 4 channels assumed
static bool rotateTileIfHorizontal(progimage_info &tile)
{
	assert(tile.width == tile.height);
	bool rotated = false;
	// check if tile is vertical or horizontal
	int row, col;
	int vertCount = 0;
	int horizCount = 0;
	// start at middle column of image, at alpha, and scan it for alpha > 0
	unsigned char* data = &tile.image_data[0] + tile.height * 2 + 3;
	for (row = 0; row < tile.height; row++)
	{
		if (*data != 0)
		{
			vertCount++;
		}
		data += 4 * tile.width;
	}
	// start at middle row of image, at alpha, and scan it for alpha > 0
	data = &tile.image_data[0] + tile.width * tile.height * 2 + 3;
	for (col = 0; col < tile.width; col++)
	{
		if (*data != 0)
		{
			horizCount++;
		}
		data += 4;
	}

	if (vertCount < horizCount) {
		rotated = true;
		rotateTile(tile, 4);
	}
	return rotated;
}

static void rotateTile(progimage_info &tile, int channels)
{
	// loop through a quad of the image and round-robin rotate with its 3 other locations
	int row, col, ch;
	// start at middle column of image, at alpha, and scan it for alpha > 0
	for (row = 0; row < tile.height/2; row++)
	{
		unsigned char* data0 = &tile.image_data[row*tile.width*channels]; // upper left corner, going down
		unsigned char* data1 = &tile.image_data[(tile.width - 1 - row) * channels]; // upper right corner, going left
		unsigned char* data2 = &tile.image_data[((tile.height - row) * tile.width - 1) * channels];	// lower right corner, going up
		unsigned char* data3 = &tile.image_data[((tile.height - 1) * tile.width + row) * channels]; // lower left corner, going right
		for (col = 0; col < tile.width/2; col++)
		{
			for (ch = 0; ch < channels; ch++) {
				// copy 0 to tmp, 1 to 0, etc.
				unsigned char tmp = *data0;
				*data0++ = *data1;
				*data1++ = *data2;
				*data2++ = *data3;
				*data3++ = tmp;
			}
			// and move one column/row/column/row inwards, remembering that we've just moved to the next pixel from the above
			//data0 += channels; - <-- that's what we would do if we hadn't incremented the pointer above in the loop
			data1 += (tile.width-1) * channels;
			data2 -= 2*channels;
			data3 -= (tile.width+1) * channels;
		}
	}
}



