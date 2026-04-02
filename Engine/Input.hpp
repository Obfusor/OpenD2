#pragma once
#include "../Shared/D2Shared.hpp"

// Input event pumping
namespace IN
{
	void PumpEvents(OpenD2ConfigStrc* pOpenConfig);
	void StartTextEditing();
	void StopTextEditing();
}
