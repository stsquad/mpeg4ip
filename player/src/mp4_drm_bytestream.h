/**	\file mp4_drm_bytestream.h

  	\author Danijel Kopcinovic (danijel.kopcinovic@adnecto.net)
*/

#ifndef MP4_DRM_BYTESTREAM_H
#define MP4_DRM_BYTESTREAM_H

#include "mp4_bytestream.h"

/*! Access protected video stream.
*/
class CMp4DRMVideoByteStream: public CMp4VideoByteStream {
public:
  /*! \brief  Constructor.
  */
  CMp4DRMVideoByteStream(CMp4File *parent, MP4TrackId track);

  /*! \brief  Destructor.
  */
  ~CMp4DRMVideoByteStream();

  bool start_next_frame(uint8_t **buffer, uint32_t *buflen,	frame_timestamp_t *pts,
    void **ud);

private:
  uint8_t* dBuffer;
  uint32_t dBuflen;
};

/*! Access protected audio stream.
*/
class CMp4DRMAudioByteStream: public CMp4AudioByteStream {
public:
  /*! \brief  Constructor.
  */
  CMp4DRMAudioByteStream(CMp4File *parent, MP4TrackId track);

  /*! \brief  Destructor.
  */
  ~CMp4DRMAudioByteStream();

  bool start_next_frame(uint8_t **buffer, uint32_t *buflen,	frame_timestamp_t *pts,
    void **ud);

private:
  uint8_t* dBuffer;
  uint32_t dBuflen;
};

#endif // MP4_DRM_BYTESTREAM_H
