#ifndef _QUANT_MATRIX_H_
#define _QUANT_MATRIX_H_

#include "../portab.h"

uint8_t get_intra_matrix_status(void);
uint8_t get_inter_matrix_status(void);

void set_intra_matrix_status(uint8_t status);
void set_inter_matrix_status(uint8_t status);

uint8_t set_intra_matrix(uint8_t *matrix);
uint8_t set_inter_matrix(uint8_t *matrix);

int16_t *get_intra_matrix(void);
int16_t *get_inter_matrix(void);

uint8_t *get_default_intra_matrix(void);
uint8_t *get_default_inter_matrix(void);

#endif /* _QUANT_MATRIX_H_ */
