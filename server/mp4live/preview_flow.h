

#ifndef __PREVIEW_FLOW_H__
#define __PREVIEW_FLOW_H__ 1

class CPreviewAVMediaFlow : public CAVMediaFlow {
 public:
  CPreviewAVMediaFlow(CLiveConfig *pConfig = NULL) :
    CAVMediaFlow(pConfig) {
    m_videoPreview = NULL;
  };

  virtual ~CPreviewAVMediaFlow() {
    Stop();
  }

  void Start(void);

  void StartVideoPreview(void);
  void StopVideoPreview(void);

 protected:
  CMediaSink*		m_videoPreview;
};

#endif

