<h1 align="center">
    <img width="75%" title="mt32-pi - Baremetal synthesizer system" src="images/mt32-pi-hc-logo-01.png">
</h1>

- A work-in-progress baremetal MIDI synthesizer for the Raspberry Pi 3 or above, based on [Munt], [FluidSynth] and [Circle].

- Turn your Raspberry Pi into a dedicated emulation of the [famous multi-timbre sound module][Roland MT-32] used by countless classic MS-DOS, PC-98 and Sharp X68000 games!

- Add your favorite [SoundFonts][SoundFont] to expand your synthesizer with [General MIDI], [Roland GS], or even [Yamaha XG] support for endless MIDI possibilities.

- Includes General MIDI and Roland GS support out of the box thanks to [GeneralUser GS] by S. Christian Collins.

- No operating system, no complex Linux audio configuration; just super-low latency audio.

- Easy to configure and ready to play from cold-boot in a matter of seconds.

---

## New features

- Encoder button press (once) displays the currently used soundfont file.

- Encoder button press (twice) displays animated pixel pet.

## Planned features

- Make device remembers last used soundfont and will select it automatically upon device reboot, power loss, etc.

## Project status

<img title="mt32-pi running on the Raspberry Pi 3 A+" width="280rem" align="right" src="images/mt32-pi-wht-pet.jpeg">
This repository is a clone of the original where I will be making my own changes and is not affiliated with the original project. This repository is for a specific hardware build of the device by HobbyChop and therefore this readme file will be stripped down to include only what is related to that specific device. 

The goal of this development is focused on features for musicians, and the focus will not be on mister integration.

- [I²S Hi-Fi DAC support][I²S Hi-Fi DACs].
  * This is the recommended audio output method for the best quality audio.

- MIDI input, output and thru via [USB][USB MIDI interfaces], [GPIO][GPIO MIDI interface] MIDI interfaces, or the [serial port].

- [Configuration file] for selecting hardware options and fine tuning.

- [LCD status screen support][LCD and OLED displays] (for MT-32 SysEx messages and status information).

- Simple [physical control surface][control surface] using buttons and rotary encoder.

- Network MIDI support via [RTP-MIDI] and [raw UDP socket].

- [Embedded FTP server][FTP server] for remote access to files.

## Installation

1. Download the latest release from the [Releases] section.

2. Extract contents to a blank [FAT32-formatted SD card][SD card preparation].

3. For MT-32 support, add your MT-32 or CM-32L ROM images to the `roms` directory - you have to provide these for copyright reasons.

4. Add your favorite SoundFonts to the `soundfonts` directory.

5. The `mt32-pi.cfg` file has already been configured for the hardware and the MIDI hat, but feel free to look through the options and make any tweaks.

6. Connect a [USB MIDI interface][USB MIDI interfaces] or [GPIO MIDI circuit][GPIO MIDI interface] to the Pi, and connect some speakers to the headphone jack.

7. Connect your vintage PC's MIDI OUT to the Pi's MIDI IN and (optionally) vice versa.

Device available here: https://hobbychop.etsy.com
