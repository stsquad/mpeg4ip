#include <string.h>
#include <stdio.h>
#include "live_apis.h"

static const char *capture_devices[] = { 
  "Capture 1", 
  "Capture 2", 
  "Capture 3",
};

static const char *cap_input0[] = {
  "Composite",
  "S-Video",
  "Aux",
};
static const char *cap_input1[] = {
  "DV-1394",
  "S-Video",
  "Comb",
};

static const char *video_encoder_names[] =
{
  "MPEG4",
};

static size_t capture_index = 0;
static size_t capture_input_index = 0;
static size_t height = 320, width = 240, fps = 30;
static int crop_enabled = 0;
static size_t crop_height = 320, crop_width = 240;
static size_t video_encoder_index = 0;
static size_t video_encoder_kbps = 300;

void set_capture_device (size_t index)
{
  capture_index = index;
}

const char *get_capture_device (void)
{
  return (capture_devices[capture_index]);
}

const char **get_capture_devices(size_t &max, size_t &current_index)
{
  max = 3;
  current_index = capture_index;
  return (capture_devices);
}

size_t get_capture_device_index (void)
{
  return (capture_index);
}
void set_video_capture_device (size_t index)
{
  capture_index = index;
}

const char *get_video_capture_input (void)
{
  switch (capture_index) {
  case 0:
    return cap_input0[capture_input_index];
  case 1:
    return cap_input1[capture_input_index];
  default:
    break;
  }
  return (cap_input0[capture_input_index]);
}

const char **get_video_capture_inputs (size_t capture_device, 
				       size_t &max, 
				       size_t &current_index)
{
  const char **ret = NULL;
  switch (capture_device) {
  case 0:
    ret = cap_input0;
    max = 3;
    current_index = capture_input_index;
    break;
  case 1:
    ret = cap_input1;
    max = 3;
    current_index = capture_input_index;
    break;
  case 2:
    ret = cap_input0;
    max = 2;
    current_index = capture_input_index;
    break;
  }

  return (ret);
}

size_t get_video_capture_input_index (void)
{
  return (capture_input_index);
}

  
void set_video_capture_inputs (size_t index) 
{
  capture_input_index = index;
}

size_t get_video_height (void)
{
  return (height);
}

size_t get_video_width (void)
{
  return (width);
}

size_t get_video_frames_per_second (void)
{
  return (fps);
}


int get_video_crop_enabled (void)
{
  return (crop_enabled);
}

size_t get_video_crop_height (void)
{
  return (crop_height);
}

size_t get_video_crop_width (void)
{
  return (crop_width);
}

int check_video_parameters (size_t height,
			    size_t width,
			    int crop_enabled,
			    size_t crop_height,
			    size_t crop_width,
			    size_t fps,
			    size_t video_encoder_index,
			    size_t video_encoder_kbps,
			    const char **errmsg)
{
  return (1);
}

int set_video_parameters (size_t h,
			  size_t w,
			  size_t crop_e,
			  size_t crop_h,
			  size_t crop_w,
			  size_t f,
			  size_t encoder_ix,
			  size_t encoder_kbps)
{
  height = h;
  width = w;
  crop_enabled = crop_e;
  crop_height = crop_h;
  crop_width = crop_w;
  fps = f;
  video_encoder_index = encoder_ix;
  video_encoder_kbps = encoder_kbps;
  return (1);
}


const char *get_video_encoder_type (void)
{
  return (video_encoder_names[video_encoder_index]);
}

const char **get_video_encoder_types (size_t &max, size_t &current_index)
{
  max = 1;
  current_index = video_encoder_index;
  return (video_encoder_names);
}

size_t get_video_encoder_index (void)
{
  return video_encoder_index;
}

size_t get_video_encoder_kbps (void)
{
  return video_encoder_kbps;
}


char *broadcast_addr;
uint16_t video_port, audio_port;

const char *get_broadcast_address (void)
{
  return broadcast_addr;
}

uint16_t get_broadcast_video_port (void)
{
  return video_port;
}

uint16_t get_broadcast_audio_port (void)
{
  return audio_port;
}

int set_broadcast_parameters (const char *addr,
			      uint16_t vport,
			      uint16_t aport,
			      const char **errmsg)
{
  audio_port = aport;
  video_port = vport;
  if (broadcast_addr != NULL) 
    free(broadcast_addr);
  broadcast_addr = strdup(addr);
  return (1);
}

