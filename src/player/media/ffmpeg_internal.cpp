#include <player/media/ffmpeg_internal.h>

#ifdef HAVE_VDPAU
#include <player/media/hwaccels/ffmpeg_vdpau.h>
#endif
#if HAVE_DXVA2_LIB
#include <player/media/hwaccels/ffmpeg_dxva2.h>
#endif
#if CONFIG_VAAPI
#include <player/media/hwaccels/ffmpeg_vaapi.h>
#endif
#if CONFIG_CUVID
#include <player/media/hwaccels/ffmpeg_cuvid.h>
#endif
#if CONFIG_VIDEOTOOLBOX
#include <player/media/hwaccels/ffmpeg_videotoolbox.h>
#endif

namespace fastoplayer {
namespace media {

AVBufferRef* hw_device_ctx = NULL;

const HWAccel hwaccels[] = {
#if CONFIG_VIDEOTOOLBOX
    {"videotoolbox", videotoolbox_init, videotoolbox_uninit, HWACCEL_VIDEOTOOLBOX, AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
     AV_PIX_FMT_VIDEOTOOLBOX},
#endif
#if CONFIG_LIBMFX
    {"qsv", qsv_init, qsv_uninit, HWACCEL_QSV, AV_HWDEVICE_TYPE_QSV, AV_PIX_FMT_QSV},
#endif
#if CONFIG_CUVID
    {"cuvid", cuvid_init, cuvid_uninit, HWACCEL_CUVID, AV_HWDEVICE_TYPE_CUDA, AV_PIX_FMT_CUDA},
#endif
    HWAccel()};

size_t hwaccel_count() {
  return SIZEOFMASS(hwaccels) - 1;
}

const HWAccel* get_hwaccel(enum AVPixelFormat pix_fmt) {
  for (size_t i = 0; i < hwaccel_count(); i++) {
    if (hwaccels[i].pix_fmt == pix_fmt) {
      return &hwaccels[i];
    }
  }
  return NULL;
}

}  // namespace media
}  // namespace fastoplayer
