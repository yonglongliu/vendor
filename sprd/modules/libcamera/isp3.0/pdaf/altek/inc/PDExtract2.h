#ifndef __PDExtract2_H__
#define __PDExtract2_H__

struct altek_pd_size {
	unsigned short total_width;
	unsigned short total_height;
	unsigned short left_width;
	unsigned short left_height;
	unsigned short right_width;
	unsigned short right_height;
};

enum vendor {
	SS,
	SONY
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int altek_extract_getversion(void *version, int size);

extern int altek_extract_type2(unsigned char *tail_data,
							struct altek_pd_size pd_size,
							enum vendor sensor_module,
							unsigned short *pd_left,
							unsigned short *pd_right);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __PDExtract2_H__ */
