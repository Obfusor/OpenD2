#include "Audio.hpp"

// Audio stubs (Phase 5 will implement Allegro audio)
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
