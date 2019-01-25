#ifndef HAL_UTILS_H_
#define HAL_UTILS_H_

int hal_add(int var1);

void hexdump(char *str, const unsigned char *data, int num_write_bytes);

void hexdump_to_file(FILE *filetype, char *str, const unsigned char *data, int num_write_bytes);

#endif /* HAL_UTILS_H_ */