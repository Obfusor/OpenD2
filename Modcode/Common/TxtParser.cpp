#include "TxtParser.hpp"
#include "D2Common.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdio>

#define TXT_MAX_COLUMNS 256
#define TXT_MAX_LINE	8192

/*
 *	Map a column index to its TxtColumnDef, or nullptr if not mapped.
 */
static const TxtColumnDef *FindColumnDef(const char *szHeader,
										 const TxtColumnDef *columns)
{
	for (const TxtColumnDef *col = columns; col->szColumnName != nullptr; col++)
	{
		if (D2Lib::stricmp(col->szColumnName, szHeader) == 0)
		{
			return col;
		}
	}
	return nullptr;
}

/*
 *	Write a parsed value into the struct at the given field offset.
 */
static void WriteField(void *pRecord, const TxtColumnDef *col, const char *szValue)
{
	BYTE *dst = (BYTE *)pRecord + col->dwOffset;

	switch (col->fieldType)
	{
	case TXT_DWORD:
		*(DWORD *)dst = (DWORD)atoi(szValue);
		break;
	case TXT_INT:
		*(int *)dst = atoi(szValue);
		break;
	case TXT_WORD:
		*(WORD *)dst = (WORD)atoi(szValue);
		break;
	case TXT_SHORT:
		*(short *)dst = (short)atoi(szValue);
		break;
	case TXT_BYTE:
		*(BYTE *)dst = (BYTE)atoi(szValue);
		break;
	case TXT_STRING:
	{
		size_t maxLen = col->dwExtra;
		if (maxLen == 0)
			maxLen = 60; // default
		strncpy((char *)dst, szValue, maxLen - 1);
		((char *)dst)[maxLen - 1] = '\0';
		break;
	}
	}
}

/*
 *	Count the number of data rows in a text buffer (lines after the header).
 */
static int CountRows(const char *pData, size_t dataSize)
{
	int nRows = 0;
	bool bPastHeader = false;
	bool bLineStart = true;

	for (size_t i = 0; i < dataSize; i++)
	{
		if (bLineStart && pData[i] != '\r' && pData[i] != '\n' && pData[i] != '\0')
		{
			if (bPastHeader)
			{
				nRows++;
			}
			else
			{
				bPastHeader = true; // skip header row
			}
			bLineStart = false;
		}

		if (pData[i] == '\n')
		{
			bLineStart = true;
		}
	}
	return nRows;
}

/*
 *	Parse a tab-delimited TXT file into an array of structs.
 */
int TXT_ParseFile(const char *szFilePath, const TxtColumnDef *columns,
				  size_t dwRowSize, void **ppOutData)
{
	// Open and read the entire file
	engine->Print(PRIORITY_MESSAGE, "TXT: Attempting to open '%s'", szFilePath);
	fs_handle f;
	size_t fileSize = engine->FS_Open(szFilePath, &f, FS_READ, false);
	engine->Print(PRIORITY_MESSAGE, "TXT: FS_Open returned handle=%d size=%d", (int)f, (int)fileSize);
	if (f == INVALID_HANDLE || fileSize == 0)
	{
		engine->Print(PRIORITY_MESSAGE, "TXT: Failed to open '%s'", szFilePath);
		return 0;
	}

	char *pFileData = (char *)malloc(fileSize + 1);
	if (pFileData == nullptr)
	{
		engine->FS_CloseFile(f);
		return 0;
	}

	engine->FS_Read(f, pFileData, fileSize, 1);
	engine->FS_CloseFile(f);
	pFileData[fileSize] = '\0';

	// Strip trailing \r from lines (Windows line endings)
	// We'll handle this per-line instead

	// Count data rows (excluding header)
	int nRows = CountRows(pFileData, fileSize);
	if (nRows <= 0)
	{
		free(pFileData);
		return 0;
	}

	// Allocate output array (zeroed)
	*ppOutData = calloc(nRows, dwRowSize);
	if (*ppOutData == nullptr)
	{
		free(pFileData);
		return 0;
	}

	// Parse header line to build column index mapping
	// columnMap[i] = pointer to TxtColumnDef for column i, or nullptr
	const TxtColumnDef *columnMap[TXT_MAX_COLUMNS] = {nullptr};
	int nColumns = 0;

	char *pLine = pFileData;
	char *pLineEnd = pLine;

	// Find end of header line
	while (*pLineEnd && *pLineEnd != '\r' && *pLineEnd != '\n')
		pLineEnd++;

	// Null-terminate header line
	if (*pLineEnd == '\r' || *pLineEnd == '\n')
	{
		*pLineEnd = '\0';
		pLineEnd++;
		if (*pLineEnd == '\n')
			pLineEnd++; // skip \r\n
	}

	// Split header by tabs
	{
		char *pToken = pLine;
		while (*pToken)
		{
			char *pTab = strchr(pToken, '\t');
			if (pTab)
				*pTab = '\0';

			if (nColumns < TXT_MAX_COLUMNS)
			{
				columnMap[nColumns] = FindColumnDef(pToken, columns);
				nColumns++;
			}

			if (pTab)
				pToken = pTab + 1;
			else
				break;
		}
	}

	// Parse data rows
	pLine = pLineEnd;
	int nRecord = 0;

	while (*pLine && nRecord < nRows)
	{
		// Skip empty lines
		if (*pLine == '\r' || *pLine == '\n')
		{
			pLine++;
			continue;
		}

		// Find end of this line
		pLineEnd = pLine;
		while (*pLineEnd && *pLineEnd != '\r' && *pLineEnd != '\n')
			pLineEnd++;

		char lineEndChar = *pLineEnd;
		*pLineEnd = '\0';

		// Parse columns for this row
		void *pRecord = (BYTE *)(*ppOutData) + (nRecord * dwRowSize);
		int col = 0;
		char *pToken = pLine;

		while (col < nColumns)
		{
			char *pTab = strchr(pToken, '\t');
			if (pTab)
				*pTab = '\0';

			if (columnMap[col] != nullptr && pToken[0] != '\0')
			{
				WriteField(pRecord, columnMap[col], pToken);
			}

			col++;
			if (pTab)
				pToken = pTab + 1;
			else
				break;
		}

		nRecord++;

		// Advance to next line
		pLine = pLineEnd + 1;
		if (lineEndChar == '\r' && *pLine == '\n')
			pLine++;
	}

	free(pFileData);

	engine->Print(PRIORITY_MESSAGE, "TXT: Parsed %s -> %d records (%d columns mapped)",
				  szFilePath, nRecord, nColumns);

	return nRecord;
}
