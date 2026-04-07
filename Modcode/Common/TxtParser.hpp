#pragma once
#include "../../Shared/D2Common_Shared.hpp"
#include <cstddef>

/*
 *	Generic tab-delimited TXT data table parser.
 *	Parses D2's .txt data files (tab-separated, first row = headers)
 *	and populates C struct arrays by mapping column names to struct field offsets.
 */

enum TxtFieldType
{
	TXT_DWORD,		// 4-byte unsigned integer
	TXT_WORD,		// 2-byte unsigned integer
	TXT_BYTE,		// 1-byte unsigned integer
	TXT_INT,		// 4-byte signed integer
	TXT_SHORT,		// 2-byte signed integer
	TXT_STRING,		// Fixed-size char array (size in dwExtra)
};

struct TxtColumnDef
{
	const char *szColumnName;	// Column header name in the TXT file
	TxtFieldType fieldType;		// Data type
	size_t dwOffset;			// offsetof() into the target struct
	size_t dwExtra;				// For TXT_STRING: max string length
};

// Sentinel value to mark end of column definition array
#define TXT_END { nullptr, TXT_DWORD, 0, 0 }

/*
 *	Parse a tab-delimited TXT file and populate an array of structs.
 *
 *	@param szFilePath		Path to the TXT file (relative, searched via FS)
 *	@param columns			Array of TxtColumnDef terminated by TXT_END sentinel
 *	@param dwRowSize		sizeof(TargetStruct) — size of each output record
 *	@param ppOutData		Receives malloc'd array of structs (caller must free)
 *	@return					Number of records parsed, or 0 on failure
 */
int TXT_ParseFile(const char *szFilePath, const TxtColumnDef *columns,
				  size_t dwRowSize, void **ppOutData);
