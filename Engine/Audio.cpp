#include "Audio.hpp"

#ifdef USE_ALLEGRO5
// Audio stubs for Allegro 5 build (Phase 5 will implement Allegro audio)
namespace Audio
{
	void Init(OpenD2ConfigStrc* openconfig) { (void)openconfig; }
	void Shutdown() {}
	sfx_handle RegisterSound(char* szAudioFile) { (void)szAudioFile; return 0; }
	mus_handle RegisterMusic(char* szAudioFile) { (void)szAudioFile; return 0; }
	void PlaySound(sfx_handle handle, int loops) { (void)handle; (void)loops; }
	void PlayMusic(mus_handle handle, int loops) { (void)handle; (void)loops; }
	void PauseAudio() {}
	void ResumeAudio() {}
	void SetMasterVolume(float volume) { (void)volume; }
	void SetMusicVolume(float volume) { (void)volume; }
	void SetSoundVolume(float volume) { (void)volume; }
}
#else
#include "Audio_SDL.hpp"

namespace Audio
{
	void Init(OpenD2ConfigStrc* openconfig)
	{
		Audio_SDL::Init(openconfig);
	}

	void Shutdown()
	{
		Audio_SDL::Shutdown();
	}

	sfx_handle RegisterSound(char* szAudioFile)
	{
		return Audio_SDL::RegisterSound(szAudioFile);
	}

	mus_handle RegisterMusic(char* szAudioFile)
	{
		return Audio_SDL::RegisterMusic(szAudioFile);
	}

	void PlaySound(sfx_handle handle, int loops)
	{
		return Audio_SDL::PlaySound(handle, loops);
	}

	void PlayMusic(mus_handle music, int loops)
	{
		return Audio_SDL::PlayMusic(music, loops);
	}

	void PauseAudio()
	{
		return Audio_SDL::PauseAudio();
	}

	void ResumeAudio()
	{
		return Audio_SDL::ResumeAudio();
	}

	void SetMasterVolume(float volume)
	{
		Audio_SDL::SetMasterVolume(volume);
	}

	void SetMusicVolume(float volume)
	{
		Audio_SDL::SetMusicVolume(volume);
	}

	void SetSoundVolume(float volume)
	{
		Audio_SDL::SetSoundVolume(volume);
	}
}
#endif // USE_ALLEGRO5
