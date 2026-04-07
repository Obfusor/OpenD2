#include "D2Common.hpp"
#include "TxtParser.hpp"
#include <cstddef>

D2COMMONAPI D2DataTablesStrc *sgptDataTables;

static D2DataTablesStrc gDataTables{0};

////////////////////////////////////////
//
//	TXT Column Definitions
//	Map TXT column headers to struct field offsets.

// levels.txt -> D2LevelDefBin (leveldefs is derived from the same file)
static const TxtColumnDef g_LevelDefColumns[] = {
	{"Quest",			TXT_DWORD,	offsetof(D2LevelDefBin, dwQuestFlag),	0},
	{"Layer",			TXT_DWORD,	offsetof(D2LevelDefBin, dwLayer),		0},
	{"SizeX",			TXT_DWORD,	offsetof(D2LevelDefBin, dwSizeX[0]),	0},
	{"SizeY",			TXT_DWORD,	offsetof(D2LevelDefBin, dwSizeY[0]),	0},
	{"OffsetX",			TXT_DWORD,	offsetof(D2LevelDefBin, dwOffsetX),		0},
	{"OffsetY",			TXT_DWORD,	offsetof(D2LevelDefBin, dwOffsetY),		0},
	{"Depend",			TXT_DWORD,	offsetof(D2LevelDefBin, dwDepend),		0},
	{"DrlgType",		TXT_DWORD,	offsetof(D2LevelDefBin, dwDrlgType),	0},
	{"LevelType",		TXT_DWORD,	offsetof(D2LevelDefBin, dwLevelType),	0},
	{"SubType",			TXT_DWORD,	offsetof(D2LevelDefBin, dwSubType),		0},
	{"SubTheme",		TXT_DWORD,	offsetof(D2LevelDefBin, dwSubTheme),	0},
	{"SubWaypoint",		TXT_DWORD,	offsetof(D2LevelDefBin, dwSubWaypoint),	0},
	{"SubShrine",		TXT_DWORD,	offsetof(D2LevelDefBin, dwSubShrine),	0},
	{"Vis0",			TXT_INT,	offsetof(D2LevelDefBin, nVis[0]),		0},
	{"Vis1",			TXT_INT,	offsetof(D2LevelDefBin, nVis[1]),		0},
	{"Vis2",			TXT_INT,	offsetof(D2LevelDefBin, nVis[2]),		0},
	{"Vis3",			TXT_INT,	offsetof(D2LevelDefBin, nVis[3]),		0},
	{"Vis4",			TXT_INT,	offsetof(D2LevelDefBin, nVis[4]),		0},
	{"Vis5",			TXT_INT,	offsetof(D2LevelDefBin, nVis[5]),		0},
	{"Vis6",			TXT_INT,	offsetof(D2LevelDefBin, nVis[6]),		0},
	{"Vis7",			TXT_INT,	offsetof(D2LevelDefBin, nVis[7]),		0},
	{"Warp0",			TXT_INT,	offsetof(D2LevelDefBin, nWarp[0]),		0},
	{"Warp1",			TXT_INT,	offsetof(D2LevelDefBin, nWarp[1]),		0},
	{"Warp2",			TXT_INT,	offsetof(D2LevelDefBin, nWarp[2]),		0},
	{"Warp3",			TXT_INT,	offsetof(D2LevelDefBin, nWarp[3]),		0},
	{"Warp4",			TXT_INT,	offsetof(D2LevelDefBin, nWarp[4]),		0},
	{"Warp5",			TXT_INT,	offsetof(D2LevelDefBin, nWarp[5]),		0},
	{"Warp6",			TXT_INT,	offsetof(D2LevelDefBin, nWarp[6]),		0},
	{"Warp7",			TXT_INT,	offsetof(D2LevelDefBin, nWarp[7]),		0},
	{"Intensity",		TXT_BYTE,	offsetof(D2LevelDefBin, nIntensity),	0},
	{"Red",				TXT_BYTE,	offsetof(D2LevelDefBin, nRed),			0},
	{"Green",			TXT_BYTE,	offsetof(D2LevelDefBin, nGreen),		0},
	{"Blue",			TXT_BYTE,	offsetof(D2LevelDefBin, nBlue),			0},
	{"Portal",			TXT_DWORD,	offsetof(D2LevelDefBin, dwPortal),		0},
	{"Position",		TXT_DWORD,	offsetof(D2LevelDefBin, dwPosition),	0},
	{"SaveMonsters",	TXT_DWORD,	offsetof(D2LevelDefBin, dwSaveMonsters),0},
	{"LOSDraw",			TXT_DWORD,	offsetof(D2LevelDefBin, dwLOSDraw),		0},
	TXT_END
};

// levels.txt -> D2LevelsTxt
static const TxtColumnDef g_LevelsTxtColumns[] = {
	{"Id",				TXT_WORD,	offsetof(D2LevelsTxt, wLevelNo),		0},
	{"Pal",				TXT_BYTE,	offsetof(D2LevelsTxt, nPal),			0},
	{"Act",				TXT_BYTE,	offsetof(D2LevelsTxt, nAct),			0},
	{"Rain",			TXT_BYTE,	offsetof(D2LevelsTxt, nRain),			0},
	{"Mud",				TXT_BYTE,	offsetof(D2LevelsTxt, nMud),			0},
	{"NoPer",			TXT_BYTE,	offsetof(D2LevelsTxt, nNoPer),			0},
	{"IsInside",		TXT_BYTE,	offsetof(D2LevelsTxt, nIsInside),		0},
	{"DrawEdges",		TXT_BYTE,	offsetof(D2LevelsTxt, nDrawEdges),		0},
	{"WarpDist",		TXT_DWORD,	offsetof(D2LevelsTxt, dwWarpDist),		0},
	{"MonLvl1",			TXT_WORD,	offsetof(D2LevelsTxt, wMonLvl[0]),		0},
	{"MonLvl2",			TXT_WORD,	offsetof(D2LevelsTxt, wMonLvl[1]),		0},
	{"MonLvl3",			TXT_WORD,	offsetof(D2LevelsTxt, wMonLvl[2]),		0},
	{"Quest",			TXT_BYTE,	offsetof(D2LevelsTxt, nQuest),			0},
	{"Waypoint",		TXT_BYTE,	offsetof(D2LevelsTxt, nWaypoint),		0},
	{"LevelName",		TXT_STRING,	offsetof(D2LevelsTxt, szLevelName),		40},
	{"LevelWarp",		TXT_STRING,	offsetof(D2LevelsTxt, szLevelWarp),		40},
	{"EntryFile",		TXT_STRING,	offsetof(D2LevelsTxt, szEntryFile),		40},
	{"Themes",			TXT_DWORD,	offsetof(D2LevelsTxt, dwThemes),		0},
	{"SoundEnv",		TXT_DWORD,	offsetof(D2LevelsTxt, dwSoundEnv),		0},
	{"FloorFilter",		TXT_DWORD,	offsetof(D2LevelsTxt, dwFloorFilter),	0},
	{"BlankScreen",		TXT_DWORD,	offsetof(D2LevelsTxt, dwBlankScreen),	0},
	TXT_END
};

// lvlprest.txt -> D2LvlPrestTxt
static const TxtColumnDef g_LvlPrestColumns[] = {
	{"Def",				TXT_DWORD,	offsetof(D2LvlPrestTxt, dwDef),			0},
	{"LevelId",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwLevelId),		0},
	{"Populate",		TXT_DWORD,	offsetof(D2LvlPrestTxt, dwPopulate),	0},
	{"Logicals",		TXT_DWORD,	offsetof(D2LvlPrestTxt, dwLogicals),	0},
	{"Outdoors",		TXT_DWORD,	offsetof(D2LvlPrestTxt, dwOutdoors),	0},
	{"Animate",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwAnimate),		0},
	{"KillEdge",		TXT_DWORD,	offsetof(D2LvlPrestTxt, dwKillEdge),	0},
	{"FillBlanks",		TXT_DWORD,	offsetof(D2LvlPrestTxt, dwFillBlanks),	0},
	{"SizeX",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwSizeX),		0},
	{"SizeY",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwSizeY),		0},
	{"AutoMap",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwAutoMap),		0},
	{"Scan",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwScan),		0},
	{"Pops",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwPops),		0},
	{"PopPad",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwPopPad),		0},
	{"Files",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwFiles),		0},
	{"File1",			TXT_STRING,	offsetof(D2LvlPrestTxt, szFile[0]),		60},
	{"File2",			TXT_STRING,	offsetof(D2LvlPrestTxt, szFile[1]),		60},
	{"File3",			TXT_STRING,	offsetof(D2LvlPrestTxt, szFile[2]),		60},
	{"File4",			TXT_STRING,	offsetof(D2LvlPrestTxt, szFile[3]),		60},
	{"File5",			TXT_STRING,	offsetof(D2LvlPrestTxt, szFile[4]),		60},
	{"File6",			TXT_STRING,	offsetof(D2LvlPrestTxt, szFile[5]),		60},
	{"Dt1Mask",			TXT_DWORD,	offsetof(D2LvlPrestTxt, dwDt1Mask),		0},
	TXT_END
};

// lvltypes.txt -> D2LvlTypesTxt
static const TxtColumnDef g_LvlTypesColumns[] = {
	{"File 1",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[0]),		60},
	{"File 2",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[1]),		60},
	{"File 3",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[2]),		60},
	{"File 4",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[3]),		60},
	{"File 5",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[4]),		60},
	{"File 6",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[5]),		60},
	{"File 7",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[6]),		60},
	{"File 8",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[7]),		60},
	{"File 9",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[8]),		60},
	{"File 10",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[9]),		60},
	{"File 11",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[10]),	60},
	{"File 12",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[11]),	60},
	{"File 13",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[12]),	60},
	{"File 14",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[13]),	60},
	{"File 15",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[14]),	60},
	{"File 16",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[15]),	60},
	{"File 17",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[16]),	60},
	{"File 18",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[17]),	60},
	{"File 19",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[18]),	60},
	{"File 20",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[19]),	60},
	{"File 21",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[20]),	60},
	{"File 22",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[21]),	60},
	{"File 23",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[22]),	60},
	{"File 24",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[23]),	60},
	{"File 25",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[24]),	60},
	{"File 26",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[25]),	60},
	{"File 27",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[26]),	60},
	{"File 28",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[27]),	60},
	{"File 29",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[28]),	60},
	{"File 30",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[29]),	60},
	{"File 31",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[30]),	60},
	{"File 32",			TXT_STRING,	offsetof(D2LvlTypesTxt, szFile[31]),	60},
	{"Act",				TXT_DWORD,	offsetof(D2LvlTypesTxt, dwAct),			0},
	TXT_END
};

////////////////////////////////////////
//
//	Functions

/*
 *	Compiles a .txt into a .bin file
 *	@author	eezstreet
 */
void BIN_Compile(const char *szTextName, D2TxtColumnStrc *pColumns, int *nNumberColumns)
{
}

/*
 *	Reads a .bin file
 *	@author	eezstreet
 */
bool BIN_Read(char *szBinName, void **pDestinationData, size_t *pFileSize, DWORD *pNumRecords)
{
	D2MPQArchive *pArchive = nullptr;
	fs_handle f;
	*pFileSize = engine->FS_Open(szBinName, &f, FS_READ, true);

	if (f == INVALID_HANDLE)
	{
		return false;
	}

	Log_ErrorAssertReturn(*pFileSize != 0, false);

	*pDestinationData = malloc(*pFileSize);
	engine->FS_Read(f, *pDestinationData, *pFileSize, 1);
	engine->FS_CloseFile(f);

	// The first 4 bytes of a BIN file contain the record count.
	// Read it, then shift the data to skip the header.
	if (pNumRecords != nullptr)
	{
		*pNumRecords = *(DWORD *)(*pDestinationData);
	}

	void *offset = (void *)(((BYTE *)*pDestinationData) + 4);
	*pFileSize -= sizeof(DWORD);
	memmove(*pDestinationData, offset, *pFileSize);

	return true;
}

/*
 *	Loads a specific data table
 *	@author	eezstreet
 */
static int DataTables_Load(const char *szDataTableName, void **pDestinationData,
						   D2TxtLinkStrc **pDestinationLink, size_t dwRowSize)
{
	char szPath[MAX_D2PATH]{0};
	size_t dwFileSize = 0;
	DWORD dwBinRecordCount = 0;

	// Try and load the BIN file first
	if (!gpConfig->bTXT)
	{
		snprintf(szPath, MAX_D2PATH, "%s%s.bin", D2DATATABLES_DIR, szDataTableName);

		if (BIN_Read(szPath, pDestinationData, &dwFileSize, &dwBinRecordCount))
		{
			// Use the record count from the BIN header (first 4 bytes).
			// Fall back to size-based calculation only if header says 0.
			int nRecords;
			if (dwBinRecordCount > 0)
			{
				nRecords = (int)dwBinRecordCount;
			}
			else
			{
				nRecords = (int)(dwFileSize / dwRowSize);
			}

			engine->Print(PRIORITY_MESSAGE,
						  "DataTables: %s -> %d bytes, %d records (row=%d, binCount=%d)",
						  szDataTableName, (int)dwFileSize, nRecords, (int)dwRowSize, (int)dwBinRecordCount);
			return nRecords;
		}
	}

	// If that doesn't work (or we have -txt mode enabled) we need to compile the BIN itself from .txt
	snprintf(szPath, MAX_D2PATH, "%s%s.txt", D2DATATABLES_DIR, szDataTableName);
	return 0;
}

/*
 *	Load all of the data tables
 *	@author	eezstreet
 */
void DataTables_Init()
{
	sgptDataTables = &gDataTables;

	//////////////
	//
	//	Levels

	// Level tables: prefer TXT loading since BIN format varies by D2 version.
	// levels.txt populates both D2LevelsTxt and D2LevelDefBin (same source file).
	sgptDataTables->nLevelsTxtRecordCount =
		TXT_ParseFile(D2DATATABLES_DIR "Levels.txt", g_LevelsTxtColumns,
					  sizeof(D2LevelsTxt), (void **)&sgptDataTables->pLevelsTxt);
	if (sgptDataTables->nLevelsTxtRecordCount == 0)
	{
		sgptDataTables->nLevelsTxtRecordCount =
			DataTables_Load("levels", (void **)&sgptDataTables->pLevelsTxt, nullptr, sizeof(D2LevelsTxt));
	}

	// leveldefs is derived from the same levels.txt columns
	int nLevelDefs = TXT_ParseFile(D2DATATABLES_DIR "Levels.txt", g_LevelDefColumns,
								   sizeof(D2LevelDefBin), (void **)&sgptDataTables->pLevelDefBin);
	if (nLevelDefs == 0)
	{
		DataTables_Load("leveldefs", (void **)&sgptDataTables->pLevelDefBin, nullptr, sizeof(D2LevelDefBin));
	}

	// lvltypes
	sgptDataTables->nLvlTypesTxtRecordCount =
		TXT_ParseFile(D2DATATABLES_DIR "LvlTypes.txt", g_LvlTypesColumns,
					  sizeof(D2LvlTypesTxt), (void **)&sgptDataTables->pLvlTypesTxt);
	if (sgptDataTables->nLvlTypesTxtRecordCount == 0)
	{
		sgptDataTables->nLvlTypesTxtRecordCount =
			DataTables_Load("lvltypes", (void **)&sgptDataTables->pLvlTypesTxt, nullptr, sizeof(D2LvlTypesTxt));
	}
	sgptDataTables->nLvlSubTxtRecordCount =
		DataTables_Load("lvlsub", (void **)&sgptDataTables->pLvlSubTxt, nullptr, sizeof(D2LvlSubTxt));
	sgptDataTables->nLvlWarpTxtRecordCount =
		DataTables_Load("lvlwarp", (void **)&sgptDataTables->pLvlWarpTxt, nullptr, sizeof(D2LvlWarpTxt));
	sgptDataTables->nLvlMazeTxtRecordCount =
		DataTables_Load("lvlmaze", (void **)&sgptDataTables->pLvlMazeTxt, nullptr, sizeof(D2LvlMazeTxt));
	sgptDataTables->nLvlPrestTxtRecordCount =
		TXT_ParseFile(D2DATATABLES_DIR "LvlPrest.txt", g_LvlPrestColumns,
					  sizeof(D2LvlPrestTxt), (void **)&sgptDataTables->pLvlPrestTxt);
	if (sgptDataTables->nLvlPrestTxtRecordCount == 0)
	{
		sgptDataTables->nLvlPrestTxtRecordCount =
			DataTables_Load("lvlprest", (void **)&sgptDataTables->pLvlPrestTxt, nullptr, sizeof(D2LvlPrestTxt));
	}

	//////////////
	//
	//	Items

	sgptDataTables->nItemsTxtRecordCount =
		DataTables_Load("weapons", (void **)&sgptDataTables->pWeapons, nullptr, sizeof(D2ItemsTxt));
	sgptDataTables->nArmorTxtRecordCount =
		DataTables_Load("armor", (void **)&sgptDataTables->pArmor, nullptr, sizeof(D2ItemsTxt));
	sgptDataTables->nMiscTxtRecordCount =
		DataTables_Load("misc", (void **)&sgptDataTables->pMisc, nullptr, sizeof(D2ItemsTxt));

	sgptDataTables->nItemStatCostTxtRecordCount =
		DataTables_Load("itemstatcost", (void **)&sgptDataTables->pItemStatCostTxt, nullptr, sizeof(D2ItemStatCostTxt));

	sgptDataTables->nItemTypesTxtRecordCount =
		DataTables_Load("itemtypes", (void **)&sgptDataTables->pItemTypesTxt, nullptr, sizeof(D2ItemTypesTxt));

	//////////////
	//
	//	Unique and Set Items

	sgptDataTables->nUniqueItemsTxtRecordCount =
		DataTables_Load("uniqueitems", (void **)&sgptDataTables->pUniqueItemsTxt, nullptr, sizeof(D2UniqueItemsTxt));

	sgptDataTables->nSetItemsTxtRecordCount =
		DataTables_Load("setitems", (void **)&sgptDataTables->pSetItemsTxt, nullptr, sizeof(D2SetItemsTxt));

	sgptDataTables->nSetsTxtRecordCount =
		DataTables_Load("sets", (void **)&sgptDataTables->pSetsTxt, nullptr, sizeof(D2SetsTxt));

	//////////////
	//
	//	Skills

	sgptDataTables->nSkillsTxtRecordCount =
		DataTables_Load("skills", (void **)&sgptDataTables->pSkillsTxt, nullptr, sizeof(D2SkillsTxt));

	sgptDataTables->nSkillDescTxtRecordCount =
		DataTables_Load("skilldesc", (void **)&sgptDataTables->pSkillDescTxt, nullptr, sizeof(D2SkillDescTxt));

	//////////////
	//
	//	Character Stats

	sgptDataTables->nCharStatsTxtRecordCount =
		DataTables_Load("charstats", (void **)&sgptDataTables->pCharStatsTxt, nullptr, sizeof(D2CharStatsTxt));

	//////////////
	//
	//	Properties

	sgptDataTables->nPropertiesTxtRecordCount =
		DataTables_Load("properties", (void **)&sgptDataTables->pPropertiesTxt, nullptr, sizeof(D2PropertiesTxt));
}

/*
 *	Delete all of the data tables
 *	@author	eezstreet
 */
void DataTables_Free()
{
	free(sgptDataTables->pLevelsTxt);
	free(sgptDataTables->pLevelDefBin);
	free(sgptDataTables->pLvlTypesTxt);
	free(sgptDataTables->pLvlSubTxt);
	free(sgptDataTables->pLvlWarpTxt);
	free(sgptDataTables->pLvlMazeTxt);
	free(sgptDataTables->pLvlPrestTxt);
	free(sgptDataTables->pWeapons);
	free(sgptDataTables->pArmor);
	free(sgptDataTables->pMisc);
	free(sgptDataTables->pItemStatCostTxt);
	free(sgptDataTables->pItemTypesTxt);
	free(sgptDataTables->pUniqueItemsTxt);
	free(sgptDataTables->pSetItemsTxt);
	free(sgptDataTables->pSetsTxt);
	free(sgptDataTables->pSkillsTxt);
	free(sgptDataTables->pSkillDescTxt);
	free(sgptDataTables->pCharStatsTxt);
	free(sgptDataTables->pPropertiesTxt);
}
