#include "Audio.hpp"
#include "Allegro5.hpp"
#include "FileSystem.hpp"
#include "Logging.hpp"
#include <cstring>

#define MAX_SOUNDS 256
#define MAX_MUSIC  16

namespace Audio
{
	static bool bInitialized = false;
	static ALLEGRO_SAMPLE *gpSamples[MAX_SOUNDS] = {0};
	static ALLEGRO_SAMPLE_ID gpSampleIDs[MAX_SOUNDS] = {0};
	static int gnNumSamples = 0;

	static ALLEGRO_AUDIO_STREAM *gpMusic[MAX_MUSIC] = {0};
	static int gnNumMusic = 0;
	static int gnCurrentMusic = -1;

	static ALLEGRO_MIXER *gpMixer = nullptr;
	static ALLEGRO_VOICE *gpVoice = nullptr;

	static float gMasterVolume = 1.0f;
	static float gMusicVolume = 0.7f;
	static float gSoundVolume = 1.0f;

	void Init(OpenD2ConfigStrc *openconfig)
	{
		(void)openconfig;

		if (!al_install_audio())
		{
			Log::Print(PRIORITY_MESSAGE, "Audio: Failed to install audio");
			return;
		}

		if (!al_init_acodec_addon())
		{
			Log::Print(PRIORITY_MESSAGE, "Audio: Failed to init acodec addon");
			return;
		}

		// Reserve sample instances for sound effects
		al_reserve_samples(16);

		gpVoice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2);
		if (!gpVoice)
		{
			Log::Print(PRIORITY_MESSAGE, "Audio: Failed to create voice");
			return;
		}

		gpMixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
		if (!gpMixer)
		{
			Log::Print(PRIORITY_MESSAGE, "Audio: Failed to create mixer");
			al_destroy_voice(gpVoice);
			gpVoice = nullptr;
			return;
		}

		al_attach_mixer_to_voice(gpMixer, gpVoice);
		al_set_default_mixer(gpMixer);

		bInitialized = true;
		Log::Print(PRIORITY_MESSAGE, "Audio: Initialized (Allegro audio)");
	}

	void Shutdown()
	{
		if (!bInitialized)
			return;

		// Stop and free music
		for (int i = 0; i < gnNumMusic; i++)
		{
			if (gpMusic[i])
			{
				al_destroy_audio_stream(gpMusic[i]);
				gpMusic[i] = nullptr;
			}
		}

		// Free samples
		for (int i = 0; i < gnNumSamples; i++)
		{
			if (gpSamples[i])
			{
				al_destroy_sample(gpSamples[i]);
				gpSamples[i] = nullptr;
			}
		}

		if (gpMixer)
		{
			al_destroy_mixer(gpMixer);
			gpMixer = nullptr;
		}

		if (gpVoice)
		{
			al_destroy_voice(gpVoice);
			gpVoice = nullptr;
		}

		al_uninstall_audio();
		bInitialized = false;
		Log::Print(PRIORITY_MESSAGE, "Audio: Shutdown");
	}

	sfx_handle RegisterSound(char *szAudioFile)
	{
		if (!bInitialized || !szAudioFile || gnNumSamples >= MAX_SOUNDS)
			return INVALID_HANDLE;

		// Try loading the file directly via Allegro (searches working directory)
		ALLEGRO_SAMPLE *sample = al_load_sample(szAudioFile);
		if (!sample)
		{
			// Try with basepath prepended
			// Sound files might be at basepath + audioFile
			return INVALID_HANDLE;
		}

		sfx_handle h = gnNumSamples;
		gpSamples[gnNumSamples] = sample;
		gnNumSamples++;
		return h;
	}

	mus_handle RegisterMusic(char *szAudioFile)
	{
		if (!bInitialized || !szAudioFile || gnNumMusic >= MAX_MUSIC)
			return INVALID_HANDLE;

		ALLEGRO_AUDIO_STREAM *stream = al_load_audio_stream(szAudioFile, 4, 2048);
		if (!stream)
			return INVALID_HANDLE;

		al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_LOOP);
		al_set_audio_stream_gain(stream, gMusicVolume * gMasterVolume);

		// Don't auto-play, attach to mixer but pause
		al_attach_audio_stream_to_mixer(stream, gpMixer);
		al_set_audio_stream_playing(stream, false);

		mus_handle h = gnNumMusic;
		gpMusic[gnNumMusic] = stream;
		gnNumMusic++;
		return h;
	}

	void PlaySound(sfx_handle handle, int loops)
	{
		if (!bInitialized || handle == INVALID_HANDLE || handle >= (sfx_handle)gnNumSamples)
			return;

		if (!gpSamples[handle])
			return;

		al_play_sample(gpSamples[handle], gSoundVolume * gMasterVolume, 0.0f, 1.0f,
			(loops == 0) ? ALLEGRO_PLAYMODE_ONCE : ALLEGRO_PLAYMODE_LOOP,
			&gpSampleIDs[handle]);
	}

	void PlayMusic(mus_handle handle, int loops)
	{
		if (!bInitialized || handle == INVALID_HANDLE || handle >= (mus_handle)gnNumMusic)
			return;

		// Stop current music
		if (gnCurrentMusic >= 0 && gnCurrentMusic < gnNumMusic && gpMusic[gnCurrentMusic])
		{
			al_set_audio_stream_playing(gpMusic[gnCurrentMusic], false);
		}

		if (!gpMusic[handle])
			return;

		al_set_audio_stream_playmode(gpMusic[handle],
			(loops == 0) ? ALLEGRO_PLAYMODE_ONCE : ALLEGRO_PLAYMODE_LOOP);
		al_set_audio_stream_gain(gpMusic[handle], gMusicVolume * gMasterVolume);
		al_set_audio_stream_playing(gpMusic[handle], true);
		gnCurrentMusic = (int)handle;
	}

	void PauseAudio()
	{
		if (!bInitialized || !gpMixer)
			return;
		// Pause the mixer to pause everything
		al_set_mixer_playing(gpMixer, false);
	}

	void ResumeAudio()
	{
		if (!bInitialized || !gpMixer)
			return;
		al_set_mixer_playing(gpMixer, true);
	}

	void SetMasterVolume(float volume)
	{
		gMasterVolume = volume;
		if (gpMixer)
			al_set_mixer_gain(gpMixer, volume);
	}

	void SetMusicVolume(float volume)
	{
		gMusicVolume = volume;
		if (gnCurrentMusic >= 0 && gnCurrentMusic < gnNumMusic && gpMusic[gnCurrentMusic])
		{
			al_set_audio_stream_gain(gpMusic[gnCurrentMusic], volume * gMasterVolume);
		}
	}

	void SetSoundVolume(float volume)
	{
		gSoundVolume = volume;
	}
}
