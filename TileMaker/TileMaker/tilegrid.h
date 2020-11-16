// tilegrid.h - grid of image texture filenames found, and related methods

#pragma once

#define MAX_PATH_AND_FILE (2*MAX_PATH)


#define	CHANNEL_RED		0
#define	CHANNEL_GREEN	1
#define	CHANNEL_BLUE	2

#define TOTAL_CATEGORIES		9
#define	CATEGORY_RGBA			0
#define	CATEGORY_NORMALS		1
#define	CATEGORY_NORMALS_LONG	2
#define	CATEGORY_METALLIC		3
#define	CATEGORY_EMISSION		4
#define	CATEGORY_ROUGHNESS		5
#define CATEGORY_SPECULAR		6
#define CATEGORY_MER			7
#define CATEGORY_SYNTHETIC		8

#define	FILE_NOT_FOUND			0
#define FILE_FOUND				1
#define FILE_FOUND_AND_IGNORED	2

static const wchar_t* gCatSuffixes[TOTAL_CATEGORIES] = { L"", L"_n", L"_normal", L"_m", L"_e", L"_r", L"_s", L"_mer", L"_y" };
static const char* gCatStrSuffixes[TOTAL_CATEGORIES] = { "", "_n", "_normal", "_m", "_e", "_r", "_s", "_mer", "_y" };

typedef struct FileRecord {
	wchar_t* rootName;
	wchar_t* fullFilename;
	wchar_t* path;
	boolean exists;
} FileRecord;

typedef struct FileGrid {
	int fileCount;
	int totalCategories;
	int totalTiles;
	int categories[TOTAL_CATEGORIES];	// number of files found in a category
	FileRecord fr[TOTAL_CATEGORIES * TOTAL_TILES];
} FileGrid;

static FileGrid gFG;


#define	TOTAL_CHEST_TILES	5
#define CHEST_NORMAL		0
#define CHEST_NORMAL_DOUBLE	1
#define CHEST_NORMAL_LEFT	2
#define CHEST_NORMAL_RIGHT	3
#define CHEST_ENDER			4
static const wchar_t* gChestNames[] = { L"normal", L"normal_double", L"normal_left", L"normal_right", L"ender" };

typedef struct ChestGrid {
	int chestCount;
	int totalCategories;
	int totalTiles;
	int categories[TOTAL_CATEGORIES];
	FileRecord cr[TOTAL_CATEGORIES * TOTAL_CHEST_TILES];
} ChestGrid;

static ChestGrid gCG;

void initializeFileGrid(FileGrid* pfg);
void initializeChestGrid(ChestGrid* pcg);
void addBackslashIfNeeded(wchar_t* dir);
int searchDirectoryForTiles(FileGrid* pfg, ChestGrid* pcg, const wchar_t* tilePath, int verbose, int alternate, boolean topmost);
bool dirExists(const wchar_t* path);
int checkTilesInDirectory(FileGrid* pfg, const wchar_t* tilePath, int verbose, int alternate);
int testIfTileExists(FileGrid* pfg, const wchar_t* tilePath, const wchar_t* origTileName, int verbose, int alternate, boolean warnDNE);
int testIfChestFile(ChestGrid* pcg, const wchar_t* tilePath, const wchar_t* origTileName, int verbose);
boolean removePNGsuffix(wchar_t* name);
boolean isPNGfile(wchar_t* name);
int stripTypeSuffix(wchar_t* tileName, const wchar_t** suffixes, int numSuffixes);
int findTileIndex(const wchar_t* tileName, int alternate);
void copyFileRecord(FileGrid* pfg, int category, int fullIndex, FileRecord* srcFR);
void deleteFileFromGrid(FileGrid* pfg, int category, int fullIndex);
void deleteChestFromGrid(ChestGrid* pcg, int category, int fullIndex);
