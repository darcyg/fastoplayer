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

#pragma once

#include <player/media/video_state_handler.h>

namespace fastoplayer {

class StreamHandler : public media::VideoStateHandler {
 public:
  virtual ~StreamHandler();

  virtual common::Error HandleRequestAudio(media::VideoState* stream,
                                           int64_t wanted_channel_layout,
                                           int wanted_nb_channels,
                                           int wanted_sample_rate,
                                           media::AudioParams* audio_hw_params,
                                           int* audio_buff_size) override = 0;
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) override = 0;

  virtual common::Error HandleRequestVideo(media::VideoState* stream,
                                           int width,
                                           int height,
                                           int av_pixel_format,
                                           AVRational aspect_ratio) override = 0;

 private:
  virtual void HandleFrameResize(media::VideoState* stream,
                                 int width,
                                 int height,
                                 int av_pixel_format,
                                 AVRational aspect_ratio) override final;
  virtual void HandleQuitStream(media::VideoState* stream, int exit_code, common::Error err) override final;
};

}  // namespace fastoplayer
