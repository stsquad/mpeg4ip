
#ifdef __cplusplus
extern "C" {
#endif 

int RGB2YUV (int x_dim, int y_dim, void *bmp, void *y_out, void *u_out, void *v_out, int flip);
int YUV2YUV (int x_dim, int y_dim, void *yuv, void *y_out, void *u_out, void *v_out, int flip);

#ifdef __cplusplus
}
#endif 


