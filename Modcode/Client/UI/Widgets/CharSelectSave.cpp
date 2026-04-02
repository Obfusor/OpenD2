#include "CharSelectSave.hpp"

// Title tables (same as before, simplified)
struct CharacterTitle
{
	const char16_t *maleTitle;
	const char16_t *femaleTitle;
};

#define TITLE_BIGENDER(x, y) {x, y}, {x, y}, {x, y}, {x, y}
#define TITLE_NOGENDER(x) {x, x}, {x, x}, {x, x}, {x, x}
#define TITLE_BIGENDER_EXP(x, y) {x, y}, {x, y}, {x, y}, {x, y}, {x, y}
#define TITLE_NOGENDER_EXP(x) {x, x}, {x, x}, {x, x}, {x, x}, {x, x}

static const CharacterTitle TitleStatus_Classic[] = {
	TITLE_NOGENDER(0),
	TITLE_BIGENDER(u"Sir", u"Dame"),
	TITLE_BIGENDER(u"Lord", u"Lady"),
	TITLE_BIGENDER(u"Baron", u"Baronness")
};

static const CharacterTitle TitleStatus_ClassicHardcore[] = {
	TITLE_NOGENDER(0),
	TITLE_BIGENDER(u"Count", u"Countess"),
	TITLE_BIGENDER(u"Duke", u"Duchess"),
	TITLE_BIGENDER(u"King", u"Queen")
};

static const CharacterTitle TitleStatus_Expansion[] = {
	TITLE_NOGENDER_EXP(0),
	TITLE_NOGENDER_EXP(u"Slayer"),
	TITLE_NOGENDER_EXP(u"Champion"),
	TITLE_BIGENDER_EXP(u"Patriarch", u"Matriarch")
};

static const CharacterTitle TitleStatus_ExpansionHardcore[] = {
	TITLE_NOGENDER_EXP(0),
	TITLE_NOGENDER_EXP(u"Destroyer"),
	TITLE_NOGENDER_EXP(u"Conqueror"),
	TITLE_NOGENDER_EXP(u"Guardian")
};

// Difficulty names based on highest town visited
static const char16_t* GetDifficultyString(const D2SaveHeader& header)
{
	// Check Hell first, then Nightmare, then Normal
	// nTowns[] stores last town ID per difficulty (0 = not visited)
	if (header.nTowns[2] > 0)
		return u"Hell";
	else if (header.nTowns[1] > 0)
		return u"Nightmare";
	else
		return u"Normal";
}

// Format modification time as short date
static void FormatTime(DWORD dwTime, char16_t* out, int outLen)
{
	if (dwTime == 0)
	{
		D2Lib::qstrncpyz(out, u"--", outLen);
		return;
	}

	// D2 save timestamps are Windows FILETIME-style or Unix timestamps
	// For simplicity, just show a relative indicator
	// TODO: parse dwModificationTime properly
	D2Lib::qstrncpyz(out, u"--", outLen);
}

namespace D2Widgets
{
	CharSelectSave::CharSelectSave(const char* characterSave, D2SaveHeader& header)
	{
		bool bClassMale = Client_classMale(header.nCharClass);

		D2Lib::strncpyz(path, characterSave, MAX_D2PATH_ABSOLUTE);
		D2Lib::qmbtowc(charName, sizeof(charName), header.szCharacterName);
		D2Lib::qsnprintf(charClassAndLevel, 32, u"%s %s",
			engine->TBL_FindStringFromIndex(5017),
			Client_className(header.nCharClass));
		D2Lib::qsnprintf(charClassAndLevel, 32, charClassAndLevel, header.nCharLevel);
		saveHeader = header;

		nextInChain = nullptr;
		bIsSelected = false;
		bHasTitle = bIsDeadHardcore = bIsExpansion = bIsHardcore = false;
		memset(charTitle, 0, sizeof(charTitle));

		// Hardcore status
		if (header.nCharStatus & (1 << D2STATUS_HARDCORE))
		{
			bIsHardcore = true;
			if (header.nCharClass & (1 << D2STATUS_DEAD))
				bIsDeadHardcore = true;
		}

		// Expansion status
		if (header.nCharStatus & (1 << D2STATUS_EXPANSION))
			bIsExpansion = true;

		// Title
		if (header.nCharTitle != 0)
		{
			bHasTitle = true;
			const char16_t* title = nullptr;

			if (bIsExpansion)
			{
				if (bIsHardcore)
					title = bClassMale ? TitleStatus_ExpansionHardcore[header.nCharTitle].maleTitle
									   : TitleStatus_ExpansionHardcore[header.nCharTitle].femaleTitle;
				else
					title = bClassMale ? TitleStatus_Expansion[header.nCharTitle].maleTitle
									   : TitleStatus_Expansion[header.nCharTitle].femaleTitle;
			}
			else if (bIsHardcore)
			{
				title = bClassMale ? TitleStatus_ClassicHardcore[header.nCharTitle].maleTitle
								   : TitleStatus_ClassicHardcore[header.nCharTitle].femaleTitle;
			}
			else
			{
				title = bClassMale ? TitleStatus_Classic[header.nCharTitle].maleTitle
								   : TitleStatus_Classic[header.nCharTitle].femaleTitle;
			}

			if (title)
				D2Lib::qstrncpyz(charTitle, title, 32);
		}

		// Difficulty progress
		D2Lib::qstrncpyz(charDifficulty, GetDifficultyString(header), 16);

		// Last played
		FormatTime(header.dwModificationTime, charLastPlayed, 16);
	}

	CharSelectSave::~CharSelectSave()
	{
		if (nextInChain)
			delete nextInChain;
	}

	void CharSelectSave::SetNextInChain(CharSelectSave* next)
	{
		nextInChain = next;
	}

	CharSelectSave* CharSelectSave::GetInChain(int counter)
	{
		if (counter == 0 || nextInChain == nullptr)
			return this;
		return nextInChain->GetInChain(counter - 1);
	}

	void CharSelectSave::Select(int counter)
	{
		if (counter <= 0 || nextInChain == nullptr)
		{
			OnSelected();
			return;
		}
		nextInChain->Select(counter - 1);
	}

	int CharSelectSave::GetDifficultyRank() const
	{
		if (saveHeader.nTowns[2] > 0) return 2; // Hell
		if (saveHeader.nTowns[1] > 0) return 1; // Nightmare
		return 0; // Normal
	}

	void CharSelectSave::OnSelected()
	{
		bIsSelected = true;
	}

	void CharSelectSave::DeselectAllInChain()
	{
		bIsSelected = false;
		if (nextInChain)
			nextInChain->DeselectAllInChain();
	}

	char16_t* CharSelectSave::GetSelectedCharacterName()
	{
		if (bIsSelected)
			return charName;
		if (nextInChain)
			return nextInChain->GetSelectedCharacterName();
		return u"";
	}
}
