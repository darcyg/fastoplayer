/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

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

#include "simple_player.h"

namespace fastoplayer {

SimplePlayer::SimplePlayer(const PlayerOptions& options) : ISimplePlayer(options, RELATIVE_SOURCE_DIR), stream_url_() {}

std::string SimplePlayer::GetCurrentUrlName() const {
  return stream_url_.GetUrl();
}

void SimplePlayer::SetUrlLocation(media::stream_id sid,
                                  const common::uri::Url& uri,
                                  media::AppOptions opt,
                                  media::ComplexOptions copt) {
  stream_url_ = uri;
  ISimplePlayer::SetUrlLocation(sid, uri, opt, copt);
}

}  // namespace fastoplayer
