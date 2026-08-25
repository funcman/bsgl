/*
** Buggy-Mushroom's Spore Game Library
** Copyright (c) 2008-2026 Buggy-Mushroom Studio
**
** Audio functions (SDL_mixer 3 frontend): Effect / Channel / Stream / Music
*/

#include "bsgl_impl.h"

#include <SDL3_mixer/SDL_mixer.h>
#include <unordered_map>

#define AUDIO_RESTYPE_EFFECT  0
#define AUDIO_RESTYPE_STREAM  1
#define AUDIO_RESTYPE_MUSIC   2

struct AudioResource {
    MIX_Audio*  audio;
    int         type;
    int         def_vol;        // 0-100
    int         def_pan;        // -100..100
    float       def_pitch;
    DWORD       channel;        // primary channel handle (stream/music), 0 if none
    int         length_ms;      // -1 unknown
};

struct AudioChannel {
    MIX_Track*  track;
    DWORD       res_id;
    bool        persistent;     // primary channel of a stream/music resource
    int         vol;            // 0-100
    int         pan;            // -100..100
    float       pitch;
    // linear slide state
    bool        sliding;
    Uint64      slide_start_ms;
    Uint64      slide_end_ms;
    int         vol0, vol1;
    int         pan0, pan1;
    float       pitch0, pitch1;
};

struct AudioState {
    MIX_Mixer*  mixer;
    std::unordered_map<DWORD, AudioResource>  resources;
    std::unordered_map<DWORD, AudioChannel>   channels;
    DWORD       next_id;

    AudioState() : mixer(nullptr), next_id(1) {}
};

static float _GainFromVol(int vol) {
    if (vol < 0)   vol = 0;
    if (vol > 100) vol = 100;
    return vol / 100.0f;
}

static void _ApplyChannelParams(AudioChannel* chn) {
    MIX_SetTrackGain(chn->track, _GainFromVol(chn->vol));
    MIX_StereoGains gains;
    int pan = chn->pan;
    if (pan < -100) pan = -100;
    if (pan >  100) pan =  100;
    gains.left  = (100 - pan) / 100.0f;
    gains.right = (pan + 100) / 100.0f;
    MIX_SetTrackStereo(chn->track, &gains);
    MIX_SetTrackFrequencyRatio(chn->track, chn->pitch > 0.0f ? chn->pitch : 1.0f);
}

static int _AudioDurationMS(MIX_Audio* audio) {
    if (!audio) {
        return -1;
    }
    Sint64 duration = MIX_GetAudioDuration(audio);
    if (duration < 0) {
        return -1;
    }
    return (int)MIX_AudioFramesToMS(audio, duration);
}

void BSGL_Impl::_AudioInit() {
    if (audio) {
        return;
    }
    audio = new AudioState();

    if (!MIX_Init()) {
        _PostError("MIX_Init failed: %s", SDL_GetError());
        return;
    }

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;

    audio->mixer = MIX_CreateMixer(&spec);
    if (!audio->mixer) {
        _PostError("Can't create the audio mixer: %s", SDL_GetError());
    }
}

void BSGL_Impl::_AudioDone() {
    if (!audio) {
        return;
    }
    for (auto& it : audio->channels) {
        if (it.second.track) {
            MIX_DestroyTrack(it.second.track);
        }
    }
    audio->channels.clear();
    for (auto& it : audio->resources) {
        if (it.second.audio) {
            MIX_DestroyAudio(it.second.audio);
        }
    }
    audio->resources.clear();
    if (audio->mixer) {
        MIX_DestroyMixer(audio->mixer);
    }
    MIX_Quit();
    delete audio;
    audio = nullptr;
}

void BSGL_Impl::_AudioTick() {
    if (!audio || !audio->mixer) {
        return;
    }
    Uint64 now = SDL_GetTicks();
    for (auto it = audio->channels.begin(); it != audio->channels.end();) {
        AudioChannel* chn = &it->second;
        if (!chn->track) {
            it = audio->channels.erase(it);
            continue;
        }
        if (chn->sliding) {
            float t = 1.0f;
            if (now < chn->slide_end_ms) {
                Uint64 span = chn->slide_end_ms - chn->slide_start_ms;
                t = span ? (float)(now - chn->slide_start_ms) / (float)span : 1.0f;
            }
            chn->vol   = (int)(chn->vol0   + (chn->vol1   - chn->vol0)   * t);
            chn->pan   = (int)(chn->pan0   + (chn->pan1   - chn->pan0)   * t);
            chn->pitch =        chn->pitch0 + (chn->pitch1 - chn->pitch0) * t;
            _ApplyChannelParams(chn);
            if (t >= 1.0f) {
                chn->sliding = false;
            }
        }
        // effect channels are pooled per play and reclaimed when finished
        if (!chn->persistent && !MIX_TrackPlaying(chn->track)) {
            MIX_DestroyTrack(chn->track);
            it = audio->channels.erase(it);
            continue;
        }
        ++it;
    }
}

static DWORD _LoadAudioResource(BSGL_Impl* impl, const char* filename,
                                DWORD size, int type) {
    impl->_AudioInit();
    if (!impl->audio || !impl->audio->mixer) {
        impl->_PostError("Audio is not initialized.");
        return 0;
    }

    MIX_Audio* mixaudio = nullptr;
    if (size > 0) {
        SDL_IOStream* io = SDL_IOFromConstMem(filename, size);
        if (io) {
            mixaudio = MIX_LoadAudio_IO(impl->audio->mixer, io, type != AUDIO_RESTYPE_EFFECT, true);
        }
    } else {
        mixaudio = MIX_LoadAudio(impl->audio->mixer, filename, type != AUDIO_RESTYPE_EFFECT);
    }
    if (!mixaudio) {
        impl->_PostError("Can't load audio %s: %s", filename, SDL_GetError());
        return 0;
    }

    AudioResource res;
    res.audio = mixaudio;
    res.type = type;
    res.def_vol = 100;
    res.def_pan = 0;
    res.def_pitch = 1.0f;
    res.channel = 0;
    res.length_ms = _AudioDurationMS(mixaudio);

    DWORD id = impl->audio->next_id++;
    impl->audio->resources[id] = res;
    return id;
}

static AudioChannel* _StartChannel(AudioState* state, DWORD res_id,
                                   AudioResource* res, DWORD chn_id, int loops) {
    if (!res->audio) {
        return nullptr;
    }

    // reuse the primary channel of a stream/music resource
    AudioChannel* chn = nullptr;
    if (res->channel) {
        auto it = state->channels.find(res->channel);
        if (it != state->channels.end()) {
            chn = &it->second;
        }
    }
    if (!chn) {
        AudioChannel newchn;
        newchn.track = MIX_CreateTrack(state->mixer);
        if (!newchn.track) {
            return nullptr;
        }
        newchn.res_id = res_id;
        newchn.persistent = (res->type != AUDIO_RESTYPE_EFFECT);
        newchn.vol = res->def_vol;
        newchn.pan = res->def_pan;
        newchn.pitch = res->def_pitch;
        newchn.sliding = false;
        state->channels[chn_id] = newchn;
        chn = &state->channels[chn_id];
        if (newchn.persistent) {
            res->channel = chn_id;
        }
    }

    MIX_SetTrackAudio(chn->track, res->audio);
    chn->vol = res->def_vol;
    chn->pan = res->def_pan;
    chn->pitch = res->def_pitch;
    chn->sliding = false;
    _ApplyChannelParams(chn);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops);
    bool ok = MIX_PlayTrack(chn->track, props);
    SDL_DestroyProperties(props);
    if (!ok) {
        return nullptr;
    }
    return chn;
}

static AudioChannel* _FindChannel(AudioState* state, HCHANNEL chn) {
    auto it = state->channels.find(chn);
    if (it == state->channels.end()) {
        return nullptr;
    }
    return &it->second;
}

static AudioResource* _FindResource(AudioState* state, DWORD id) {
    auto it = state->resources.find(id);
    if (it == state->resources.end()) {
        return nullptr;
    }
    return &it->second;
}

static void _Slide(AudioChannel* chn, DWORD time, int vol, int pan, float pitch) {
    if (!chn || !chn->track) {
        return;
    }
    // out-of-range values mean "leave unchanged" (HGE-style slide parameters)
    if (vol < 0 || vol > 100)      vol = chn->vol;
    if (pan < -100 || pan > 100)   pan = chn->pan;
    if (pitch <= 0.0f)             pitch = chn->pitch;
    chn->vol0 = chn->vol;      chn->vol1 = vol;
    chn->pan0 = chn->pan;      chn->pan1 = pan;
    chn->pitch0 = chn->pitch;  chn->pitch1 = pitch;
    Uint64 now = SDL_GetTicks();
    chn->slide_start_ms = now;
    chn->slide_end_ms = now + time;
    chn->sliding = (time > 0);
    if (!chn->sliding) {
        chn->vol = vol;
        chn->pan = pan;
        chn->pitch = pitch;
        _ApplyChannelParams(chn);
    }
}

//--- Effects -------------------------------------------------------------

HEFFECT CALL BSGL_Impl::Effect_Load(const char* filename, DWORD size) {
    return _LoadAudioResource(this, filename, size, AUDIO_RESTYPE_EFFECT);
}

void CALL BSGL_Impl::Effect_Free(HEFFECT eff) {
    if (!audio) {
        return;
    }
    auto it = audio->resources.find(eff);
    if (it == audio->resources.end()) {
        return;
    }
    AudioResource& res = it->second;
    if (res.channel) {
        auto cit = audio->channels.find(res.channel);
        if (cit != audio->channels.end()) {
            if (cit->second.track) {
                MIX_DestroyTrack(cit->second.track);
            }
            audio->channels.erase(cit);
        }
    }
    if (res.audio) {
        MIX_DestroyAudio(res.audio);
    }
    audio->resources.erase(it);
}

HCHANNEL CALL BSGL_Impl::Effect_Play(HEFFECT eff) {
    if (!audio || !audio->mixer) {
        return 0;
    }
    AudioResource* res = _FindResource(audio, eff);
    if (!res || res->type != AUDIO_RESTYPE_EFFECT) {
        return 0;
    }
    DWORD chn_id = audio->next_id++;
    if (!_StartChannel(audio, eff, res, chn_id, 0)) {
        return 0;
    }
    return chn_id;
}

void CALL BSGL_Impl::Effect_SetVol(HEFFECT eff, int vol) {
    if (!audio) {
        return;
    }
    AudioResource* res = _FindResource(audio, eff);
    if (res) {
        res->def_vol = vol;
    }
}

void CALL BSGL_Impl::Effect_SetPan(HEFFECT eff, int pan) {
    if (!audio) {
        return;
    }
    AudioResource* res = _FindResource(audio, eff);
    if (res) {
        res->def_pan = pan;
    }
}

void CALL BSGL_Impl::Effect_SetPitch(HEFFECT eff, float pitch) {
    if (!audio) {
        return;
    }
    AudioResource* res = _FindResource(audio, eff);
    if (res) {
        res->def_pitch = pitch;
    }
}

//--- Channels ------------------------------------------------------------

void CALL BSGL_Impl::Channel_SetPanning(HCHANNEL chn, int pan) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _FindChannel(audio, chn);
    if (c) {
        c->pan = pan;
        c->sliding = false;
        _ApplyChannelParams(c);
    }
}

void CALL BSGL_Impl::Channel_SetVolume(HCHANNEL chn, int vol) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _FindChannel(audio, chn);
    if (c) {
        c->vol = vol;
        c->sliding = false;
        _ApplyChannelParams(c);
    }
}

void CALL BSGL_Impl::Channel_SetPitch(HCHANNEL chn, float pitch) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _FindChannel(audio, chn);
    if (c) {
        c->pitch = pitch;
        c->sliding = false;
        _ApplyChannelParams(c);
    }
}

void CALL BSGL_Impl::Channel_SlideTo(HCHANNEL chn, DWORD time, int vol, int pan, float pitch) {
    if (!audio) {
        return;
    }
    _Slide(_FindChannel(audio, chn), time, vol, pan, pitch);
}

void CALL BSGL_Impl::Channel_FadeTo(HCHANNEL chn, DWORD time, int vol) {
    if (!audio) {
        return;
    }
    _Slide(_FindChannel(audio, chn), time, vol, -101, -1.0f);
}

void CALL BSGL_Impl::Channel_Stop(HCHANNEL chn) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _FindChannel(audio, chn);
    if (c && c->track) {
        MIX_StopTrack(c->track, 0);
    }
}

void CALL BSGL_Impl::Channel_Pause(HCHANNEL chn) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _FindChannel(audio, chn);
    if (c && c->track) {
        MIX_PauseTrack(c->track);
    }
}

void CALL BSGL_Impl::Channel_Resume(HCHANNEL chn) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _FindChannel(audio, chn);
    if (c && c->track) {
        MIX_ResumeTrack(c->track);
    }
}

bool CALL BSGL_Impl::Channel_IsPlaying(HCHANNEL chn) {
    if (!audio) {
        return false;
    }
    AudioChannel* c = _FindChannel(audio, chn);
    return (c && c->track) ? MIX_TrackPlaying(c->track) : false;
}

int CALL BSGL_Impl::Channel_GetLength(HCHANNEL chn) {
    if (!audio) {
        return -1;
    }
    AudioChannel* c = _FindChannel(audio, chn);
    if (!c) {
        return -1;
    }
    AudioResource* res = _FindResource(audio, c->res_id);
    return res ? res->length_ms : -1;
}

//--- Streams -------------------------------------------------------------

HSTREAM CALL BSGL_Impl::Stream_Load(const char* filename, DWORD size) {
    return _LoadAudioResource(this, filename, size, AUDIO_RESTYPE_STREAM);
}

void CALL BSGL_Impl::Stream_Free(HSTREAM stream) {
    if (!audio) {
        return;
    }
    auto it = audio->resources.find(stream);
    if (it == audio->resources.end() || it->second.type != AUDIO_RESTYPE_STREAM) {
        return;
    }
    AudioResource& res = it->second;
    if (res.channel) {
        auto cit = audio->channels.find(res.channel);
        if (cit != audio->channels.end()) {
            if (cit->second.track) {
                MIX_DestroyTrack(cit->second.track);
            }
            audio->channels.erase(cit);
        }
    }
    if (res.audio) {
        MIX_DestroyAudio(res.audio);
    }
    audio->resources.erase(it);
}

HCHANNEL CALL BSGL_Impl::Stream_Play(HSTREAM stream) {
    if (!audio || !audio->mixer) {
        return 0;
    }
    AudioResource* res = _FindResource(audio, stream);
    if (!res || res->type != AUDIO_RESTYPE_STREAM) {
        return 0;
    }
    if (!_StartChannel(audio, stream, res, audio->next_id++, 0)) {
        return 0;
    }
    return res->channel;
}

void CALL BSGL_Impl::Stream_SetVol(HSTREAM stream, int vol) {
    if (!audio) {
        return;
    }
    AudioResource* res = _FindResource(audio, stream);
    if (res) {
        res->def_vol = vol;
        if (res->channel) {
            AudioChannel* c = _FindChannel(audio, res->channel);
            if (c) {
                c->vol = vol;
                c->sliding = false;
                _ApplyChannelParams(c);
            }
        }
    }
}

static AudioChannel* _ResourceChannel(AudioState* state, DWORD id) {
    AudioResource* res = _FindResource(state, id);
    if (!res || !res->channel) {
        return nullptr;
    }
    return _FindChannel(state, res->channel);
}

void CALL BSGL_Impl::Stream_Rewind(HSTREAM stream) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _ResourceChannel(audio, stream);
    if (c && c->track) {
        MIX_SetTrackPlaybackPosition(c->track, 0);
    }
}

void CALL BSGL_Impl::Stream_Seek(HSTREAM stream, DWORD position) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _ResourceChannel(audio, stream);
    if (c && c->track) {
        Sint64 frames = MIX_TrackMSToFrames(c->track, position);
        if (frames >= 0) {
            MIX_SetTrackPlaybackPosition(c->track, frames);
        }
    }
}

DWORD CALL BSGL_Impl::Stream_GetPos(HSTREAM stream) {
    if (!audio) {
        return 0;
    }
    AudioChannel* c = _ResourceChannel(audio, stream);
    if (!c || !c->track) {
        return 0;
    }
    Sint64 pos = MIX_GetTrackPlaybackPosition(c->track);
    if (pos < 0) {
        return 0;
    }
    Sint64 ms = MIX_TrackFramesToMS(c->track, pos);
    return ms > 0 ? (DWORD)ms : 0;
}

int CALL BSGL_Impl::Stream_GetLength(HSTREAM stream) {
    if (!audio) {
        return -1;
    }
    AudioResource* res = _FindResource(audio, stream);
    return res ? res->length_ms : -1;
}

//--- Music ---------------------------------------------------------------

HMUSIC CALL BSGL_Impl::Music_Load(const char* filename, DWORD size) {
    return _LoadAudioResource(this, filename, size, AUDIO_RESTYPE_MUSIC);
}

void CALL BSGL_Impl::Music_Free(HMUSIC mus) {
    if (!audio) {
        return;
    }
    auto it = audio->resources.find(mus);
    if (it == audio->resources.end() || it->second.type != AUDIO_RESTYPE_MUSIC) {
        return;
    }
    AudioResource& res = it->second;
    if (res.channel) {
        auto cit = audio->channels.find(res.channel);
        if (cit != audio->channels.end()) {
            if (cit->second.track) {
                MIX_DestroyTrack(cit->second.track);
            }
            audio->channels.erase(cit);
        }
    }
    if (res.audio) {
        MIX_DestroyAudio(res.audio);
    }
    audio->resources.erase(it);
}

HCHANNEL CALL BSGL_Impl::Music_Play(HMUSIC mus) {
    if (!audio || !audio->mixer) {
        return 0;
    }
    AudioResource* res = _FindResource(audio, mus);
    if (!res || res->type != AUDIO_RESTYPE_MUSIC) {
        return 0;
    }
    // module music loops infinitely, as in HGE
    if (!_StartChannel(audio, mus, res, audio->next_id++, -1)) {
        return 0;
    }
    return res->channel;
}

void CALL BSGL_Impl::Music_Stop(HMUSIC mus) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _ResourceChannel(audio, mus);
    if (c && c->track) {
        MIX_StopTrack(c->track, 0);
    }
}

void CALL BSGL_Impl::Music_Pause(HMUSIC mus) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _ResourceChannel(audio, mus);
    if (c && c->track) {
        MIX_PauseTrack(c->track);
    }
}

void CALL BSGL_Impl::Music_Resume(HMUSIC mus) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _ResourceChannel(audio, mus);
    if (c && c->track) {
        MIX_ResumeTrack(c->track);
    }
}

void CALL BSGL_Impl::Music_SetVol(HMUSIC mus, int vol) {
    Stream_SetVol(mus, vol);
}

void CALL BSGL_Impl::Music_SetPan(HMUSIC mus, int pan) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _ResourceChannel(audio, mus);
    if (c && c->track) {
        c->pan = pan;
        _ApplyChannelParams(c);
    }
}

void CALL BSGL_Impl::Music_SetPitch(HMUSIC mus, float pitch) {
    if (!audio) {
        return;
    }
    AudioChannel* c = _ResourceChannel(audio, mus);
    if (c && c->track) {
        c->pitch = pitch;
        _ApplyChannelParams(c);
    }
}

void CALL BSGL_Impl::Music_SlideTo(HMUSIC mus, DWORD time, int vol, int pan, float pitch) {
    if (!audio) {
        return;
    }
    _Slide(_ResourceChannel(audio, mus), time, vol, pan, pitch);
}

void CALL BSGL_Impl::Music_FadeTo(HMUSIC mus, DWORD time, int vol) {
    Music_SlideTo(mus, time, vol, -101, -1.0f);
}

void CALL BSGL_Impl::Music_SetPos(HMUSIC mus, DWORD position) {
    Stream_Seek(mus, position);
}

DWORD CALL BSGL_Impl::Music_GetPos(HMUSIC mus) {
    return Stream_GetPos(mus);
}

int CALL BSGL_Impl::Music_GetLength(HMUSIC mus) {
    return Stream_GetLength(mus);
}
