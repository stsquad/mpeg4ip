
#include "text.h"
#include "text_plugin.h"
#include "mpeg4ip_utils.h"
#include "our_config_file.h"
#include "text_href.h"

CPlainTextRender::CPlainTextRender (void *n) : CTextRenderer() 
{
  m_buffer_list = NULL;
}

void CPlainTextRender::load_frame (uint32_t fill_index, 
				   uint32_t disp_type,
				   void *disp_struct)
{
  if (disp_type != TEXT_DISPLAY_TYPE_PLAIN) {
    message(LOG_ERR, "text", "Expecting type %u got type %u in text render", 
			 TEXT_DISPLAY_TYPE_PLAIN, disp_type);
  }
  plain_display_structure_t *pd = (plain_display_structure_t *)disp_struct;
  m_buffer_list[fill_index] = (char *)strdup(pd->display_string);
}

void CPlainTextRender::render (uint32_t play_index)
{
  message(LOG_INFO, "text", "%s", m_buffer_list[play_index]);
  free(m_buffer_list[play_index]);
  m_buffer_list[play_index] = NULL;
}


CTimedTextSync::CTimedTextSync (CPlayerSession *psptr) : CTimedSync("timed text", psptr)
{
  m_render_device = NULL;
}

CTimedTextSync::~CTimedTextSync (void)
{
  // remove any secondard functions
  delete m_render_device;
}

int CTimedTextSync::initialize (const char *name)
{
  if (m_initialized == false) {
    if (m_config_set) { 
      // do initialization
      m_initialized = true;
    } else {
      return 0;
    }
  }
  return 1;
}

bool CTimedTextSync::is_ready (uint64_t &disptime)
{
  disptime = m_play_this_at[m_play_index];

  return m_buffer_filled[m_play_index];
}

void CTimedTextSync::configure (uint32_t display_type, 
				void *display_config)
{
  if (m_config_set == false) {
    m_display_type = display_type;
    switch (m_display_type) {
    case TEXT_DISPLAY_TYPE_PLAIN:
      m_render_device = new CPlainTextRender(display_config);
      break;
    case TEXT_DISPLAY_TYPE_HREF:
      m_render_device = new CHrefTextRenderer(m_psptr, display_config);
    }
    initialize_indexes(m_render_device->initialize());
    m_config_set = true;
  }
}

void CTimedTextSync::render (uint32_t play_index)
{
  m_render_device->render(play_index);
}

void CTimedTextSync::have_frame (uint64_t ts, uint32_t display_type, 
				 void *disp_struct)
{

  if (have_buffer_to_fill() == false) {
    return;
  }

  m_render_device->load_frame(m_fill_index, display_type, disp_struct);
  m_play_this_at[m_fill_index] = ts;

  // call with fill index;
  increment_fill_index();
  save_last_filled_time(ts);
  m_psptr->wake_sync_thread();
}

static void c_text_configure (void *ifptr, uint32_t display_type, 
			      void *display_config)
{
  CTimedTextSync *pText = (CTimedTextSync *)ifptr;

  pText->configure(display_type, display_config);
}

static void c_text_have_frame (void *ifptr, 
			       uint64_t display_time,
			       uint32_t display_type,
			       void *display_structure)
{
  CTimedTextSync *pText = (CTimedTextSync *)ifptr;

  pText->have_frame(display_time, display_type, display_structure);
}

static text_vft_t text_vft = 
{
  message,
  c_text_configure, 
  c_text_have_frame, 
  NULL,
};

text_vft_t *get_text_vft (void)
{
  text_vft.pConfig = &config;
  return &text_vft;
}

CTimedTextSync *create_text_sync (CPlayerSession *psptr)
{
  return new CTimedTextSync(psptr);
}


