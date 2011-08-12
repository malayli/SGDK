/**
 * \file sound.c
 * \brief Audio / Sound stuff
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides advanced sound playback methods through differents Z80 drivers.
 *
 * <b>Z80_DRIVER_PCM</b><br>
 * Single channel 8 bits signed sample driver.<br>
 * It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.<br>
 *<br>
 * <b>Z80_DRIVER_2ADPCM</b><br>
 * 2 channels 4 bits ADPCM sample driver.<br>
 * It can mix up to 2 ADCPM samples at a fixed 22050 Hz Khz rate.<br>
 * Address and size of samples have to be 256 bytes boundary.<br>
 *<br>
 * <b>Z80_DRIVER_4PCM</b><br>
 * 4 channels 8 bits signed sample driver.<br>
 * It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.<br>
 * Address and size of samples have to be 256 bytes boundary.<br>
 * The driver does support "cutoff" when mixing so you can use true 8 bits samples :)<br>
 *<br>
 * <b>Z80_DRIVER_4PCM_ENV</b><br>
 * 4 channels 8 bits signed sample driver with volume support.<br>
 * It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.<br>
 * with volume support (16 levels du to memory limitation).<br>
 * Address and size of samples have to be 256 bytes boundary.<br>
 * The driver does support "cutoff" when mixing so you can use true 8 bits samples :)<br>
 *<br>
 * <b>Z80_DRIVER_MVS</b><br>
 * MVS music player driver.<br>
 *<br>
 * <b>Z80_DRIVER_TFM</b><br>
 * TFM music player driver.<br>
 */

#include "config.h"
#include "types.h"

#include "sound.h"

#include "z80_ctrl.h"
#include "smp_null.h"
#include "smp_null_pcm.h"


// Z80_DRIVER_PCM
// single channel 8 bits signed sample driver
///////////////////////////////////////////////////////////////

/**
 * \brief
 *      Return play status (Single channel PCM player driver).
 *
 * \return
 *      Return non zero if PCM player is currently playing a sample
 */
u8 SND_isPlaying_PCM()
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();

    return ret;
}

/**
 * \brief
 *      Start playing a sample (Single channel PCM player driver).<br>
 *      If a sample was currently playing then it's stopped and the new sample is played instead.
 *
 * \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      SGDK automatically align resource as needed
 * \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      SGDK automatically adjust resource size as needed
 * \param rate
 *      Playback rate :<br>
 *      SOUND_RATE_32000 = 32 Khz (best quality but take lot of rom space)<br>
 *      SOUND_RATE_22050 = 22 Khz<br>
 *      SOUND_RATE_16000 = 16 Khz<br>
 *      SOUND_RATE_13400 = 13.4 Khz<br>
 *      SOUND_RATE_11025 = 11 Khz<br>
 *      SOUND_RATE_8000  = 8 Khz (worst quality but take less rom place)<br>
 * \param pan
 *      Panning :<br>
 *      SOUND_PAN_LEFT   = play on left speaker<br>
 *      SOUND_PAN_RIGHT  = play on right speaker<br>
 *      SOUND_PAN_CENTER = play on both speaker<br>
 * \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_PCM(const u8 *sample, const u32 len, const u8 rate, const u8 pan, const u8 loop)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, 1);

    Z80_requestBus(1);

    // point to Z80 base parameters
    pb = (u8 *) Z80_DRV_PARAMS;

    addr = (u32) sample;
    // sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // sample length (256 bytes aligned)
    pb[2] = len >> 8;
    pb[3] = len >> 16;
    // rate
    pb[4] = rate;
    // pan (left / right / center)
    pb[6] = pan;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // play command
    *pb |= Z80_DRV_COM_PLAY;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;

    // loop flag in status
    if (loop) pb[1] |= Z80_DRV_STAT_PLAYING;
    else pb[1] &= ~Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();
}

/**
 * \brief
 *      Stop playing (Single channel PCM player driver).<br>
 *      No effect if no sample was currently playing.
 */
void SND_stopPlay_PCM()
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, 1);

    Z80_requestBus(1);

    // point to Z80 internal parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x10);

    addr = (u32) smp_null;
    // sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // sample length (256 bytes aligned)
    pb[2] = sizeof(smp_null) >> 8;
    pb[3] = sizeof(smp_null) >> 16;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // remove play and loop status
    pb[0] &= ~Z80_DRV_STAT_PLAYING;
    pb[1] &= ~Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();
}


// Z80_DRIVER_2ADPCM
// 2 channels 4 bits ADPCM sample driver
///////////////////////////////////////////////////////////////
/**
 * \brief
 *      Return play status of specified channel (2 channels ADPCM player driver).
 *
 * \param channel_mask
 *      Channel(s) we want to retrieve play state.<br>
 *      SOUND_PCM_CH1_MSK : channel 1<br>
 *      SOUND_PCM_CH2_MSK : channel 2<br>
 *      <br>
 *      You can combine mask to retrieve state of severals channels at once:<br>
 *      <code>isPlaying_2ADPCM(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 * \return
 *      Return non zero if specified channel(s) is(are) playing.
 */
u8 SND_isPlaying_2ADPCM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

/**
 * \brief
 *      Start playing a sample on specified channel (2 channels ADPCM player driver).<br>
 *      If a sample was currently playing on this channel then it's stopped and the new sample is played instead.
 *
 * \param sample
 *      Sample address, should be 128 bytes boundary aligned<br>
 *      SGDK automatically align resource as needed
 * \param len
 *      Size of sample in bytes, should be a multiple of 128<br>
 *      SGDK automatically adjust resource size as needed
 * \param channel
 *      Channel where we want to play sample.<br>
 *      SOUND_PCM_CH1     : channel 1<br>
 *      SOUND_PCM_CH2     : channel 2<br>
 * \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_2ADPCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // get status
    status = *pb;

    // auto channel ?
    if (channel == SOUND_PCM_CH_AUTO)
    {
        // scan for first free channel
        ch = 0;

        while ((ch < 2) && (status & (Z80_DRV_STAT_PLAYING << ch))) ch++;

        // if all channel busy we use the first
        if (ch == 2) ch = 0;
    }
    // we use specified channel
    else ch = channel;

    // point to Z80 base parameters
    pb = (u8 *) (Z80_DRV_PARAMS + (ch * 4));

    addr = (u32) sample;
    // sample address (128 bytes aligned)
    pb[0] = addr >> 7;
    pb[1] = addr >> 15;
    // sample length (128 bytes aligned)
    pb[2] = len >> 7;
    pb[3] = len >> 15;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // play command
    *pb |= (Z80_DRV_COM_PLAY << ch);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;

    // loop flag in status
    if (loop) pb[1] |= (Z80_DRV_STAT_PLAYING << ch);
    else pb[1] &= ~(Z80_DRV_STAT_PLAYING << ch);

    Z80_releaseBus();
}

/**
 * \brief
 *      Stop playing the specified channel (2 channels ADPCM player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 * \param channel
 *      Channel we want to stop.<br>
 *      SOUND_PCM_CH1     : channel 1<br>
 *      SOUND_PCM_CH2     : channel 2<br>
 */
void SND_stopPlay_2ADPCM(const u16 channel)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, 1);

    Z80_requestBus(1);

    // point to Z80 internal parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x10 + (channel * 4));

    addr = (u32) smp_null_pcm;
    // sample address (128 bytes aligned)
    pb[0] = addr >> 7;
    pb[1] = addr >> 15;
    // sample length (128 bytes aligned)
    pb[2] = sizeof(smp_null_pcm) >> 7;
    pb[3] = sizeof(smp_null_pcm) >> 15;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // remove play and loop status
    pb[0] &= ~(Z80_DRV_STAT_PLAYING << channel);
    pb[1] &= ~(Z80_DRV_STAT_PLAYING << channel);

    Z80_releaseBus();
}


// Z80_DRIVER_4PCM
// 4 channels 8 bits signed sample driver
///////////////////////////////////////////////////////////////
/**
 * \brief
 *      Return play status of specified channel (4 channels PCM player driver).
 *
 * \param channel_mask
 *      Channel(s) we want to retrieve play state.<br>
 *      SOUND_PCM_CH1_MSK : channel 1<br>
 *      SOUND_PCM_CH2_MSK : channel 2<br>
 *      SOUND_PCM_CH3_MSK : channel 3<br>
 *      SOUND_PCM_CH4_MSK : channel 4<br>
 *      <br>
 *      You can combine mask to retrieve state of severals channels at once:<br>
 *      <code>isPlaying_2ADPCM(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 * \return
 *      Return non zero if specified channel(s) is(are) playing.
 */
u8 SND_isPlaying_4PCM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

/**
 * \brief
 *      Start playing a sample on specified channel (4 channels PCM player driver).<br>
 *      If a sample was currently playing on this channel then it's stopped and the new sample is played instead.
 *
 * \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      SGDK automatically align resource as needed
 * \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      SGDK automatically adjust resource size as needed
 * \param channel
 *      Channel where we want to play sample.<br>
 *      SOUND_PCM_CH1     : channel 1<br>
 *      SOUND_PCM_CH2     : channel 2<br>
 *      SOUND_PCM_CH3     : channel 3<br>
 *      SOUND_PCM_CH4     : channel 4<br>
 * \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_4PCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // get status
    status = *pb;

    // auto channel ?
    if (channel == SOUND_PCM_CH_AUTO)
    {
        // scan for first free channel
        ch = 0;

        while ((ch < 4) && (status & (Z80_DRV_STAT_PLAYING << ch))) ch++;

        // if all channel busy we use the first
        if (ch == 4) ch = 0;
    }
    // we use specified channel
    else ch = channel;

    // point to Z80 base parameters
    pb = (u8 *) (Z80_DRV_PARAMS + (ch * 4));

    addr = (u32) sample;
    // sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // sample length (256 bytes aligned)
    pb[2] = len >> 8;
    pb[3] = len >> 16;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // play command
    *pb |= (Z80_DRV_COM_PLAY << ch);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;

    // loop flag in status
    if (loop) pb[1] |= (Z80_DRV_STAT_PLAYING << ch);
    else pb[1] &= ~(Z80_DRV_STAT_PLAYING << ch);

    Z80_releaseBus();
}

/**
 * \brief
 *      Stop playing the specified channel (4 channels PCM player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 * \param channel
 *      Channel we want to stop.<br>
 *      SOUND_PCM_CH1     : channel 1<br>
 *      SOUND_PCM_CH2     : channel 2<br>
 *      SOUND_PCM_CH3     : channel 3<br>
 *      SOUND_PCM_CH4     : channel 4<br>
 */
void SND_stopPlay_4PCM(const u16 channel)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, 1);

    Z80_requestBus(1);

    // point to Z80 internal parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x10 + (channel * 4));

    addr = (u32) smp_null;
    // sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // sample length (256 bytes aligned)
    pb[2] = sizeof(smp_null) >> 8;
    pb[3] = sizeof(smp_null) >> 16;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // remove play and loop status
    pb[0] &= ~(Z80_DRV_STAT_PLAYING << channel);
    pb[1] &= ~(Z80_DRV_STAT_PLAYING << channel);

    Z80_releaseBus();
}


// Z80_DRIVER_4PCM_ENV
// 4 channels 8 bits signed sample driver with volume support
///////////////////////////////////////////////////////////////
/**
 * \brief
 *      Return play status of specified channel (4 channels PCM ENV player driver).
 *
 * \param channel_mask
 *      Channel(s) we want to retrieve play state.<br>
 *      SOUND_PCM_CH1_MSK : channel 1<br>
 *      SOUND_PCM_CH2_MSK : channel 2<br>
 *      SOUND_PCM_CH3_MSK : channel 3<br>
 *      SOUND_PCM_CH4_MSK : channel 4<br>
 *      <br>
 *      You can combine mask to retrieve state of severals channels at once:<br>
 *      <code>isPlaying_2ADPCM(SOUND_PCM_CH1_MSK | SOUND_PCM_CH2_MSK)</code><br>
 *      will actually return play state for channel 1 and channel 2.
 *
 * \return
 *      Return non zero if specified channel(s) is(are) playing.
 */
u8 SND_isPlaying_4PCM_ENV(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

/**
 * \brief
 *      Start playing a sample on specified channel (4 channels PCM ENV player driver).<br>
 *      If a sample was currently playing on this channel then it's stopped and the new sample is played instead.
 *
 * \param sample
 *      Sample address, should be 256 bytes boundary aligned<br>
 *      SGDK automatically align resource as needed
 * \param len
 *      Size of sample in bytes, should be a multiple of 256<br>
 *      SGDK automatically adjust resource size as needed
 * \param channel
 *      Channel where we want to play sample.<br>
 *      SOUND_PCM_CH1     : channel 1<br>
 *      SOUND_PCM_CH2     : channel 2<br>
 *      SOUND_PCM_CH3     : channel 3<br>
 *      SOUND_PCM_CH4     : channel 4<br>
 * \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_4PCM_ENV(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // get status
    status = *pb;

    // auto channel ?
    if (channel == SOUND_PCM_CH_AUTO)
    {
        // scan for first free channel
        ch = 0;

        while ((ch < 4) && (status & (Z80_DRV_STAT_PLAYING << ch))) ch++;

        // if all channel busy we use the first
        if (ch == 4) ch = 0;
    }
    // we use specified channel
    else ch = channel;

    // point to Z80 base parameters
    pb = (u8 *) (Z80_DRV_PARAMS + (ch * 4));

    addr = (u32) sample;
    // sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // sample length (256 bytes aligned)
    pb[2] = len >> 8;
    pb[3] = len >> 16;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // play command
    *pb |= (Z80_DRV_COM_PLAY << ch);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;

    // loop flag in status
    if (loop) pb[1] |= (Z80_DRV_STAT_PLAYING << ch);
    else pb[1] &= ~(Z80_DRV_STAT_PLAYING << ch);

    Z80_releaseBus();
}

/**
 * \brief
 *      Stop playing the specified channel (4 channels PCM ENV player driver).<br>
 *      No effect if no sample was currently playing on this channel.
 *
 * \param channel
 *      Channel we want to stop.<br>
 *      SOUND_PCM_CH1     : channel 1<br>
 *      SOUND_PCM_CH2     : channel 2<br>
 *      SOUND_PCM_CH3     : channel 3<br>
 *      SOUND_PCM_CH4     : channel 4<br>
 */
void SND_stopPlay_4PCM_ENV(const u16 channel)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

    // point to Z80 internal parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x10 + (channel * 4));

    addr = (u32) smp_null;
    // sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // sample length (256 bytes aligned)
    pb[2] = sizeof(smp_null) >> 8;
    pb[3] = sizeof(smp_null) >> 16;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // remove play and loop status
    pb[0] &= ~(Z80_DRV_STAT_PLAYING << channel);
    pb[1] &= ~(Z80_DRV_STAT_PLAYING << channel);

    Z80_releaseBus();
}

/**
 * \brief
 *      Change envelop / volume of specified channel (4 channels PCM ENV player driver).
 *
 * \param channel
 *      Channel we want to set envelop.<br>
 *      SOUND_PCM_CH1     : channel 1<br>
 *      SOUND_PCM_CH2     : channel 2<br>
 *      SOUND_PCM_CH3     : channel 3<br>
 *      SOUND_PCM_CH4     : channel 4<br>
 * \param volume
 *      Volume to set : 16 possible level from 0 (minimum) to 15 (maximum).
 */
void SND_setVolume_4PCM_ENV(const u16 channel, const u8 volume)
{
    vu8 *pb;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // set volume (16 levels)
    *pb = volume & 0x0F;

    Z80_releaseBus();
}

/**
 * \brief
 *      Return envelop / volume level of specified channel (4 channels PCM ENV player driver).
 *
 * \param channel
 *      Channel we want to retrieve envelop level.<br>
 *      SOUND_PCM_CH1     : channel 1<br>
 *      SOUND_PCM_CH2     : channel 2<br>
 *      SOUND_PCM_CH3     : channel 3<br>
 *      SOUND_PCM_CH4     : channel 4<br>
 * \return
 *      Envelop of specified channel.<br>
 *      The returned value is comprised between 0 (quiet) to 15 (loud).
 */
u8 SND_getVolume_4PCM_ENV(const u16 channel)
{
    vu8 *pb;
    u8 volume;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // get volume (16 levels)
    volume = *pb & 0x0F;

    Z80_releaseBus();

    return volume;
}


// Z80_DRIVER_MVS
// MVS Tracker driver
///////////////////////////////////////////////////////////////
/**
 * \brief
 *      Return play status (MVS music player driver).
 *
 * \return
 *      Return non zero if MVS player is currently playing.
 */
u8 SND_isPlaying_MVS()
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(1);

    // point to Z80 play status for MVS
    pb = (u8 *) 0xA0151D;

    // status
    // 0 :silence 1: loop play 2: play once
    ret = *pb & 3;

    Z80_releaseBus();

    return ret;
}

/**
 * \brief
 *      Start playing the specified MVS track (MVS music player driver).
 *
 * \param song
 *      MVS track address.
 * \param loop
 *      Loop flag.<br>
 *      If non zero then the sample will be played in loop (else sample is played only once).
 */
void SND_startPlay_MVS(const u8 *song, const u8 loop)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(1);

    addr = (u32) song;

    // point to Z80 base parameters for MVS
    pb = (u8 *) 0xA0151A;

    // song address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;

    // command
    if (loop)
        pb[0x03] = SOUND_MVS_LOOP;
    else
        pb[0x03] = SOUND_MVS_ONCE;

    Z80_releaseBus();
}

/**
 * \brief
 *      Stop playing music (MVS music player driver).
 */
void SND_stopPlay_MVS()
{
    vu8 *pb;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(1);

    // point to Z80 command for MVS
    pb = (u8 *) 0xA0151D;
    // stop command for MVS
    *pb = SOUND_MVS_SILENCE;

    Z80_releaseBus();
}

// Z80_DRIVER_TFM
// TFM Tracker driver
///////////////////////////////////////////////////////////////
/**
 * \brief
 *      Start playing the specified TFM track (TFM music player driver).
 *
 * \param song
 *      TFM track address.
 */
void SND_startPlay_TFM(const u8 *song)
{
    vu8 *pb;
    u32 addr;

    Z80_requestBus(1);

    addr = (u32) song;

    // point to Z80 base parameters for TFM
    pb = (u8 *) 0xA01FFC;

    // song address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;
    pb[0x00] = addr >> 24;

    Z80_releaseBus();

    // load the driver efter we set the song adress
    Z80_loadDriver(Z80_DRIVER_TFM, 0);

    // reset Z80 (in case driver was already loaded)
    Z80_startReset();
    Z80_endReset();
}
