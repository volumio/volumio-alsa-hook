# The volumio-alsa-hook repository

This repository contains the Volumio Alsa Hook plugin (known as `volumiohook`). This plugin provides users of ALSA with the ability to call out to external programs at various points in the ALSA lifecycle.

## Quick start

The `volumiohook` plugin can be added to your ALSA configuration very easily. For example:

```
pcm.hook {
  type volumiohook
  slave.pcm <slave>
  hw_params_command <your command>
}
```

## The callbacks

There are the following defined callbacks

 * `hw_params_command` - called when the PCM hardware params are set. This occurs once at PCM setup.
 * `prepare_command` - called when the PCM is prepared. This occurs multiple times. Initially it is called before playback starts, and then whenever the PCM is reset after stopping or an error.
 * `hw_free_command` - called when the PCM hardware is freed. This occurs once when the PCM is being shut down.

## Common rules

There are the following common rules that apply to all hook callbacks

### Parameterised calls

It is common for callbacks to need to know information about the audio stream. The following templates will be expanded when calling your callbacks:

 * %r - this will be replaced with the sample rate
 * %c - this will be replaced with the number of channels
 * %f - this will be replaced with the sample format (e.g. `S16_LE`)
 * %d - this will be replaced with the sample bit depth (e.g. `16`)

### Blocking calls

All hook callbacks are executed to completion and block the callback thread. This allows you to be sure that your callback has finished doing whatever it needs to do before ALSA playback can continue. 

*Note that this model introduces the potential for deadlock.* You must not block waiting for audio in your hook execution.
*Note that this model introduces the potential for zombie processes.* If your callback runs in the background in some way then it will not be terminated by this plugin. Your callback must make sure that it correctly cleans up when the parent dies or is terminated.

### What user runs the hook

This plugin makes no effort to set or change the user when running a hook. Your callback must be able to be run by any user that can play audio. Typically you should allow execution by the `audio` system group.

## A fuller example

This example calls a script `oled-display` to update an oled display with the currently set sample rate, channel count and bit depth for the hw. The display is reset when the hw is released.

```
pcm.hook {
  type volumiohook
  slave.pcm output
  hw_params_command "oled-display 'Now playing %r:%c:%d'"
  hw_free_command "oled-display 'Nothing playing'"
}

pcm.output {
  type hw
  ...
}
```

## Debugging

Debug logging can be enabled by setting `debug 1` in the pcm configuration. Debug logs are written to `stderr`.
