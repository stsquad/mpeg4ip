

#ifndef __PREVIEW_FLOW_H__
#define __PREVIEW_FLOW_H__ 1

class CPreviewAVMediaFlow : public CAVMediaFlow {
 public:
  CPreviewAVMediaFlow(CLiveConfig *pConfig = NULL) :
    CAVMediaFlow(pConfig) {
    m_videoPreview = NULL;
    m_PreviewEncoder = NULL;
  };

  virtual ~CPreviewAVMediaFlow() {
    StopVideoPreview();
  };

  void Start(bool startvideo = true, bool start_audio = true,
	     bool starttext = true);

  void Stop(void);
  void StartVideoPreview(void);
  void StopVideoPreview(bool delete_it = true);
  virtual bool ProcessSDLEvents(bool process_quit = true);
  bool doingPreview(void) { return m_videoPreview != NULL; };
  void RestartVideo(void);
 protected:
  void CreatePreview(void);
  void ConnectPreview(bool create_encoder = false);
  void DisconnectPreview(void);
  CMediaSink*		m_videoPreview;
  CVideoEncoder *m_PreviewEncoder;
};

#endif

