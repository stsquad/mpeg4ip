int check_cpu_features(void);

typedef void (emmsFunc)();
typedef emmsFunc* emmsFuncPtr;	

extern emmsFuncPtr emms;

emmsFunc emms_c;
emmsFunc emms_mmx;
