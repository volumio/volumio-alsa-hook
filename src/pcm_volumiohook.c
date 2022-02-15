/*
 *  PCM - volumio hook plugin
 *
 *  Copyright (c) 2022 by Timothy Ward <timothyjward@apache.org>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <limits.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>

typedef struct snd_pcm_volumiohook {
    snd_pcm_extplug_t ext;
    char* hw_params_command;
    char* prepare_command;
    char* hw_free_command;
    char debug;
} snd_pcm_volumiohook_t;

int _snd_pcm_volumiohook_parameter(snd_pcm_volumiohook_t *volumio, char* dst, int dst_ptr,
		char* template, int template_idx) {
	int result = 1000;
	switch(template[template_idx]) {
		case 'c':
			result = snprintf(dst + dst_ptr, 999 - dst_ptr, "%d", volumio->ext.channels);
			break;
		case 'r':
			result = snprintf(dst + dst_ptr, 999 - dst_ptr, "%d", volumio->ext.rate);
			break;
		case 'f':
			result = snprintf(dst + dst_ptr, 999 - dst_ptr, "%s", snd_pcm_format_name(volumio->ext.format));
			break;
		case 'd':
			result = snprintf(dst + dst_ptr, 999 - dst_ptr, "%d", snd_pcm_format_physical_width(volumio->ext.format));
			break;
		default:
			SNDERR("PCM %s Unable to populate template %s. Error at index %d",
					snd_pcm_name(volumio->ext.pcm), template, template_idx);
			break;
	}
	return result;
}

char* _snd_pcm_volumiohook_create_cmd(snd_pcm_volumiohook_t *volumio, char* template) {
	char* result = 0;

	if(template) {

		result = calloc(1000, sizeof(char));

		int result_ptr = 0;

		for(int i = 0; result_ptr < 999; i++) {
			switch(template[i]) {
				case '%': {
					result_ptr += _snd_pcm_volumiohook_parameter(volumio, result, result_ptr, template, ++i);
					break;
				}
				case 0:
					if(volumio->debug > 1) {
						SNDERR("PCM %s has created command %s from template %s",
								snd_pcm_name(volumio->ext.pcm), result, template);
					}
					return result;
				default:
					result[result_ptr++] = template[i];
					break;
			}
		}

		free(result);
		result = 0;

		if(volumio->debug > 1) {
			SNDERR("PCM %s has failed to create a command from template %s",
					snd_pcm_name(volumio->ext.pcm), template);
		}
	}

	return result;
}

int _snd_pcm_volumiohook_execute(snd_pcm_volumiohook_t *volumio, char* cmd) {
	int result = 0;
	if(cmd) {
		result = system(cmd);
		if(result == -1) {
			SNDERR("The PCM %s failed to run command %s", snd_pcm_name(volumio->ext.pcm), cmd);
		} else if (result != 0) {
			result = 0;
			if(volumio->debug > 0) {
				SNDERR("The PCM %s got a non zero return %d from command %s", snd_pcm_name(volumio->ext.pcm), result, cmd);
			}
		}
		free(cmd);
		cmd = 0;
	}
	return 0;
}

int snd_pcm_volumiohook_hw_params(snd_pcm_extplug_t *ext, snd_pcm_hw_params_t *params) {
	snd_pcm_volumiohook_t *volumio = ext->private_data;

	_snd_pcm_volumiohook_execute(volumio, _snd_pcm_volumiohook_create_cmd(volumio, volumio->hw_params_command));

	return 0;
}

int snd_pcm_volumiohook_init(snd_pcm_extplug_t *ext) {
	snd_pcm_volumiohook_t *volumio = ext->private_data;

	_snd_pcm_volumiohook_execute(volumio, _snd_pcm_volumiohook_create_cmd(volumio, volumio->prepare_command));

	return 0;
}

int snd_pcm_volumiohook_hw_free(snd_pcm_extplug_t *ext) {
	snd_pcm_volumiohook_t *volumio = ext->private_data;

	_snd_pcm_volumiohook_execute(volumio, _snd_pcm_volumiohook_create_cmd(volumio, volumio->hw_free_command));

	return 0;
}

int snd_pcm_volumiohook_close(snd_pcm_extplug_t *ext) {
	snd_pcm_volumiohook_t *volumio = ext->private_data;

	int err = 0;

	if(volumio->debug)
		SNDERR("PCM close called for %s", snd_pcm_name(ext->pcm));

	if (volumio->hw_params_command != NULL) {
		free(volumio->hw_params_command);
		volumio->hw_params_command = NULL;
	}
	if (volumio->prepare_command != NULL) {
		free(volumio->prepare_command);
		volumio->prepare_command = NULL;
	}
	if (volumio->hw_free_command != NULL) {
		free(volumio->hw_free_command);
		volumio->hw_free_command = NULL;
	}

	free(volumio);

	return err;
}

snd_pcm_sframes_t snd_pcm_volumiohook_transfer(snd_pcm_extplug_t *ext, const snd_pcm_channel_area_t *dst_areas,
		snd_pcm_uframes_t dst_offset, const snd_pcm_channel_area_t *src_areas,
		snd_pcm_uframes_t src_offset, snd_pcm_uframes_t size) {

	int err = snd_pcm_areas_copy(dst_areas, dst_offset, src_areas, src_offset, ext->channels,
			size, ext->format);

	return err == 0 ? size : err;
}

static const snd_pcm_extplug_callback_t volumio_hook_callback = {
	.hw_params = snd_pcm_volumiohook_hw_params,
	.init = snd_pcm_volumiohook_init,
	.hw_free = snd_pcm_volumiohook_hw_free,
	.close = snd_pcm_volumiohook_close,
	.transfer = snd_pcm_volumiohook_transfer,
};

SND_PCM_PLUGIN_DEFINE_FUNC(volumiohook)
{
    snd_config_iterator_t i, next;
    snd_config_t *slave = NULL;
    const char *hw_params_command = 0, *prepare_command = 0, *hw_free_command = 0;
    long debug = 0;
    snd_pcm_volumiohook_t *volumio;
    int err;

    snd_config_for_each(i, next, conf) {
        snd_config_t *n = snd_config_iterator_entry(i);
        const char *id;
        if (snd_config_get_id(n, &id) < 0)
            continue;
        if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0)
            continue;
        if (strcmp(id, "slave") == 0) {
            slave = n;
            continue;
        }
        if (strcmp(id, "debug") == 0) {
			if (snd_config_get_integer(n, &debug) < 0) {
				SNDERR("Invalid type for %s", id);
				err = -EINVAL;
				goto error;
			}
			continue;
		}
        if (strcmp(id, "hw_params_command") == 0) {
        	if (snd_config_get_string(n, &hw_params_command) < 0) {
				SNDERR("Invalid type for %s", id);
				err = -EINVAL;
				goto error;
			}
			continue;
        }
        if (strcmp(id, "prepare_command") == 0) {
        	if (snd_config_get_string(n, &prepare_command) < 0) {
				SNDERR("Invalid type for %s", id);
				err = -EINVAL;
				goto error;
			}
			continue;
        }
        if (strcmp(id, "hw_free_command") == 0) {
        	if (snd_config_get_string(n, &hw_free_command) < 0) {
				SNDERR("Invalid type for %s", id);
				err = -EINVAL;
				goto error;
			}
			continue;
        }
        SNDERR("Unknown field %s", id);
        return -EINVAL;
    }

    if (! slave) {
        SNDERR("No slave defined for %s", name);
        return -EINVAL;
    }

    volumio = calloc(1, sizeof(*volumio));
    if (volumio == NULL)
        return -ENOMEM;

    volumio->debug = debug <= 0 ? 0 : debug >= 127 ? 127 : debug;

    if(hw_params_command) {
		volumio->hw_params_command = strdup(hw_params_command);
		if (volumio->hw_params_command == NULL) {
			SNDERR("cannot allocate");
			err = -ENOMEM;
			goto error;
		}
    }
    if(prepare_command) {
		volumio->hw_params_command = strdup(prepare_command);
		if (volumio->prepare_command == NULL) {
			SNDERR("cannot allocate");
			err = -ENOMEM;
			goto error;
		}
    }
    if(hw_free_command) {
		volumio->hw_free_command = strdup(hw_free_command);
		if (volumio->hw_free_command == NULL) {
			SNDERR("cannot allocate");
			err = -ENOMEM;
			goto error;
		}
    }


    volumio->ext.version = SND_PCM_EXTPLUG_VERSION;
    volumio->ext.name = "Volumio Hook Plugin";
    volumio->ext.callback = &volumio_hook_callback;
    volumio->ext.private_data = volumio;

    err = snd_pcm_extplug_create(&volumio->ext, name, root, slave, stream, mode);
    if (err < 0)
    		goto error;

    *pcmp = volumio->ext.pcm;
    return 0;

error:
	if(volumio) {
		if (volumio->hw_params_command != NULL) {
			free(volumio->hw_params_command);
			volumio->hw_params_command = NULL;
		}
		if (volumio->prepare_command != NULL) {
			free(volumio->prepare_command);
			volumio->prepare_command = NULL;
		}
		if (volumio->hw_free_command != NULL) {
			free(volumio->hw_free_command);
			volumio->hw_free_command = NULL;
		}

		if(volumio->ext.pcm)
			snd_pcm_extplug_delete(&volumio->ext);
		else
			free(volumio);
	}
	return err;
}

SND_PCM_PLUGIN_SYMBOL(volumiohook);
