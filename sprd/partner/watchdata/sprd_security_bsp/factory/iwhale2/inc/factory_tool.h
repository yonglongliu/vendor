#ifndef __FACTORY_TOOL_H
#define __FACTORY_TOOL_H

extern void sprd_tee_debug(const char *tag, const char *fmt, ...);


void ft_sprd_rotpk0_print(void);
 
/*
 *rotpk0 program
 */
int ft_sprd_rotpk0_program(void);

/*
 *get life cycle
 * */
int ft_sprd_get_lcs(unsigned int *pLcs);

/*
 * sprd write simlock
 * */
int ft_sprd_simlock_write(unsigned int block_offset, unsigned int writeData);

/*
 *sprd kce program
 */
int ft_sprd_kce_program(unsigned int* pKceData);

#endif
