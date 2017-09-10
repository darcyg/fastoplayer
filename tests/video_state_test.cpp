#include "player/media/video_state.h"

int main(int argc, char** argv) {
  using namespace fastoplayer;
  media::VideoState* vs =
      new media::VideoState(fastoplayer::stream_id(), common::uri::Url(), media::AppOptions(), media::ComplexOptions());
  delete vs;
  return 0;
}
