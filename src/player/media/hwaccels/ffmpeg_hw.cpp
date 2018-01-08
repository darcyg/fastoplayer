/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>

#include <player/media/hwaccels/ffmpeg_hw.h>

#include <player/media/ffmpeg_internal.h>

namespace fastoplayer {
namespace media {

namespace {

int nb_hw_devices;
HWDevice** hw_devices = nullptr;
}  // namespace

static HWDevice* hw_device_get_by_type(enum AVHWDeviceType type) {
  HWDevice* found = NULL;
  for (int i = 0; i < nb_hw_devices; i++) {
    if (hw_devices[i]->type == type) {
      if (found) {
        return NULL;
      }
      found = hw_devices[i];
    }
  }
  return found;
}

HWDevice* hw_device_get_by_name(const char* name) {
  for (int i = 0; i < nb_hw_devices; i++) {
    if (!strcmp(hw_devices[i]->name, name)) {
      return hw_devices[i];
    }
  }
  return NULL;
}

static HWDevice* hw_device_add(void) {
  int err = av_reallocp_array(&hw_devices, nb_hw_devices + 1, sizeof(*hw_devices));
  if (err) {
    nb_hw_devices = 0;
    return NULL;
  }
  hw_devices[nb_hw_devices] = static_cast<HWDevice*>(av_mallocz(sizeof(HWDevice)));
  if (!hw_devices[nb_hw_devices]) {
    return NULL;
  }
  return hw_devices[nb_hw_devices++];
}

static char* hw_device_default_name(enum AVHWDeviceType type) {
  // Make an automatic name of the form "type%d".  We arbitrarily
  // limit at 1000 anonymous devices of the same type - there is
  // probably something else very wrong if you get to this limit.
  const char* type_name = av_hwdevice_get_type_name(type);
  size_t index_pos;
  int index, index_limit = 1000;
  index_pos = strlen(type_name);
  char* name = static_cast<char*>(av_malloc(index_pos + 4));
  if (!name) {
    return NULL;
  }
  for (index = 0; index < index_limit; index++) {
    snprintf(name, index_pos + 4, "%s%d", type_name, index);
    if (!hw_device_get_by_name(name))
      break;
  }
  if (index >= index_limit) {
    av_freep(&name);
    return NULL;
  }
  return name;
}

int hw_device_init_from_string(const char* arg, HWDevice** dev_out) {
  // "type=name:device,key=value,key2=value2"
  // "type:device,key=value,key2=value2"
  // -> av_hwdevice_ctx_create()
  // "type=name@name"
  // "type@name"
  // -> av_hwdevice_ctx_create_derived()

  AVDictionary* options = NULL;
  char *type_name = NULL, *name = NULL, *device = NULL;
  enum AVHWDeviceType type;
  HWDevice *dev, *src;
  AVBufferRef* device_ref = NULL;
  int err;
  const char *errmsg, *p, *q;
  size_t k;

  k = strcspn(arg, ":=@");
  p = arg + k;

  type_name = av_strndup(arg, k);
  if (!type_name) {
    err = AVERROR(ENOMEM);
    goto fail;
  }
  type = av_hwdevice_find_type_by_name(type_name);
  if (type == AV_HWDEVICE_TYPE_NONE) {
    errmsg = "unknown device type";
    goto invalid;
  }

  if (*p == '=') {
    k = strcspn(p + 1, ":@");

    name = av_strndup(p + 1, k);
    if (!name) {
      err = AVERROR(ENOMEM);
      goto fail;
    }
    if (hw_device_get_by_name(name)) {
      errmsg = "named device already exists";
      goto invalid;
    }

    p += 1 + k;
  } else {
    name = hw_device_default_name(type);
    if (!name) {
      err = AVERROR(ENOMEM);
      goto fail;
    }
  }

  if (!*p) {
    // New device with no parameters.
    err = av_hwdevice_ctx_create(&device_ref, type, NULL, NULL, 0);
    if (err < 0)
      goto fail;

  } else if (*p == ':') {
    // New device with some parameters.
    ++p;
    q = strchr(p, ',');
    if (q) {
      device = av_strndup(p, q - p);
      if (!device) {
        err = AVERROR(ENOMEM);
        goto fail;
      }
      err = av_dict_parse_string(&options, q + 1, "=", ",", 0);
      if (err < 0) {
        errmsg = "failed to parse options";
        goto invalid;
      }
    }

    err = av_hwdevice_ctx_create(&device_ref, type, device ? device : p, options, 0);
    if (err < 0)
      goto fail;

  } else if (*p == '@') {
    // Derive from existing device.

    src = hw_device_get_by_name(p + 1);
    if (!src) {
      errmsg = "invalid source device name";
      goto invalid;
    }

    err = av_hwdevice_ctx_create_derived(&device_ref, type, src->device_ref, 0);
    if (err < 0)
      goto fail;
  } else {
    errmsg = "parse error";
    goto invalid;
  }

  dev = hw_device_add();
  if (!dev) {
    err = AVERROR(ENOMEM);
    goto fail;
  }

  dev->name = name;
  dev->type = type;
  dev->device_ref = device_ref;

  if (dev_out)
    *dev_out = dev;

  name = NULL;
  err = 0;
done:
  av_freep(&type_name);
  av_freep(&name);
  av_freep(&device);
  av_dict_free(&options);
  return err;
invalid:
  ERROR_LOG() << "Invalid device specification \"" << arg << "\": " << errmsg;
  err = AVERROR(EINVAL);
  goto done;
fail:
  ERROR_LOG() << "Device creation failed: " << err << ".";
  av_buffer_unref(&device_ref);
  goto done;
}

static int hw_device_init_from_type(enum AVHWDeviceType type, const char* device, HWDevice** dev_out) {
  AVBufferRef* device_ref = NULL;
  HWDevice* dev;
  char* name;
  int err;

  name = hw_device_default_name(type);
  if (!name) {
    err = AVERROR(ENOMEM);
    goto fail;
  }

  err = av_hwdevice_ctx_create(&device_ref, type, device, NULL, 0);
  if (err < 0) {
    ERROR_LOG() << "Device creation failed: " << err << ".";
    goto fail;
  }

  dev = hw_device_add();
  if (!dev) {
    err = AVERROR(ENOMEM);
    goto fail;
  }

  dev->name = name;
  dev->type = type;
  dev->device_ref = device_ref;

  if (dev_out)
    *dev_out = dev;

  return 0;

fail:
  av_freep(&name);
  av_buffer_unref(&device_ref);
  return err;
}

void hw_device_free_all(void) {
  for (int i = 0; i < nb_hw_devices; i++) {
    av_freep(&hw_devices[i]->name);
    av_buffer_unref(&hw_devices[i]->device_ref);
    av_freep(&hw_devices[i]);
  }
  av_freep(&hw_devices);
  nb_hw_devices = 0;
}

static HWDevice* hw_device_match_by_codec(const AVCodec* codec) {
  const AVCodecHWConfig* config;
  HWDevice* dev;
  int i;
  for (i = 0;; i++) {
    config = avcodec_get_hw_config(codec, i);
    if (!config)
      return NULL;
    if (!(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX))
      continue;
    dev = hw_device_get_by_type(config->device_type);
    if (dev)
      return dev;
  }
}

int hw_device_setup_for_decode(AVCodecContext* avctx, AVCodec* codec) {
  InputStream* ist = static_cast<InputStream*>(avctx->opaque);

  const AVCodecHWConfig* config;
  enum AVHWDeviceType type;
  HWDevice* dev = NULL;
  int err, auto_device = 0;

  if (ist->hwaccel_device) {
    dev = hw_device_get_by_name(ist->hwaccel_device);
    if (!dev) {
      if (ist->hwaccel_id == HWACCEL_AUTO) {
        auto_device = 1;
      } else if (ist->hwaccel_id == HWACCEL_GENERIC) {
        type = ist->hwaccel_device_type;
        err = hw_device_init_from_type(type, ist->hwaccel_device, &dev);
      } else {
        // This will be dealt with by API-specific initialisation
        // (using hwaccel_device), so nothing further needed here.
        return 0;
      }
    } else {
      if (ist->hwaccel_id == HWACCEL_AUTO) {
        ist->hwaccel_device_type = dev->type;
      } else if (ist->hwaccel_device_type != dev->type) {
        ERROR_LOG() << "Invalid hwaccel device "
                       "specified for decoder: device "
                    << dev->name << " of type " << av_hwdevice_get_type_name(dev->type)
                    << " is not "
                       "usable with hwaccel "
                    << av_hwdevice_get_type_name(ist->hwaccel_device_type) << ".";

        return AVERROR(EINVAL);
      }
    }
  } else {
    if (ist->hwaccel_id == HWACCEL_AUTO) {
      auto_device = 1;
    } else if (ist->hwaccel_id == HWACCEL_GENERIC) {
      type = ist->hwaccel_device_type;
      dev = hw_device_get_by_type(type);
      if (!dev)
        err = hw_device_init_from_type(type, NULL, &dev);
    } else {
      dev = hw_device_match_by_codec(codec);
      if (!dev) {
        // No device for this codec, but not using generic hwaccel
        // and therefore may well not need one - ignore.
        return 0;
      }
    }
  }

  if (auto_device) {
    int i;
    if (!avcodec_get_hw_config(codec, 0)) {
      // Decoder does not support any hardware devices.
      return 0;
    }
    for (i = 0; !dev; i++) {
      config = avcodec_get_hw_config(codec, i);
      if (!config)
        break;
      type = config->device_type;
      dev = hw_device_get_by_type(type);
      if (dev) {
        INFO_LOG() << "Using auto  hwaccel type " << av_hwdevice_get_type_name(type) << " with existing device"
                   << dev->name << ".";
      }
    }
    for (i = 0; !dev; i++) {
      config = avcodec_get_hw_config(codec, i);
      if (!config)
        break;
      type = config->device_type;
      // Try to make a new device of this type.
      err = hw_device_init_from_type(type, ist->hwaccel_device, &dev);
      if (err < 0) {
        // Can't make a device of this type.
        continue;
      }
      if (ist->hwaccel_device) {
        INFO_LOG() << "Using auto hwaccel type " << av_hwdevice_get_type_name(type) << " with new device created from "
                   << ist->hwaccel_device << ".";
      } else {
        INFO_LOG() << "Using auto "
                      "hwaccel type "
                   << av_hwdevice_get_type_name(type) << " with new default device.";
      }
    }
    if (dev) {
      ist->hwaccel_device_type = type;
    } else {
      INFO_LOG() << "Auto hwaccel disabled: no device found.";
      ist->hwaccel_id = HWACCEL_NONE;
      return 0;
    }
  }

  if (!dev) {
    ERROR_LOG() << "No device available "
                   "for decoder: device type "
                << av_hwdevice_get_type_name(type) << " needed for codec " << codec->name << ".";
    return err;
  }

  avctx->hw_device_ctx = av_buffer_ref(dev->device_ref);
  if (!avctx->hw_device_ctx) {
    return AVERROR(ENOMEM);
  }

  return 0;
}

static int hwaccel_retrieve_data(AVCodecContext* avctx, AVFrame* input) {
  InputStream* ist = static_cast<InputStream*>(avctx->opaque);
  AVFrame* output = NULL;
  enum AVPixelFormat output_format = ist->hwaccel_output_format;
  int err;

  if (input->format == output_format) {
    // Nothing to do.
    return 0;
  }

  output = av_frame_alloc();
  if (!output) {
    return AVERROR(ENOMEM);
  }

  output->format = output_format;

  err = av_hwframe_transfer_data(output, input, 0);
  if (err < 0) {
    ERROR_LOG() << "Failed to transfer data to output frame: " << err << ".";
    goto fail;
  }

  err = av_frame_copy_props(output, input);
  if (err < 0) {
    av_frame_unref(output);
    goto fail;
  }

  av_frame_unref(input);
  av_frame_move_ref(input, output);
  av_frame_free(&output);

  return 0;

fail:
  av_frame_free(&output);
  return err;
}

int hwaccel_decode_init(AVCodecContext* avctx) {
  InputStream* ist = static_cast<InputStream*>(avctx->opaque);
  ist->hwaccel_retrieve_data = &hwaccel_retrieve_data;
  return 0;
}

}  // namespace media
}  // namespace fastoplayer
