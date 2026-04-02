#pragma once
#include "../Shared/D2Shared.hpp"

// Input event pumping
namespace IN
{
	void PumpEvents(OpenD2ConfigStrc* pOpenConfig);

#ifdef USE_ALLEGRO5
	void StartTextEditing();
	void StopTextEditing();
#endif
}
