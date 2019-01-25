/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/fb.h>
#include <linux/kd.h>

#include <time.h>
#include <../factorytest.h>

#include "graphics.h"
int tp_flag1 = 0;
int s_rotate_type = ROTATE_UNKNOWN;

/* SPRD: add for support rotate @{ */
#include <utils/Log.h>
#include "cutils/properties.h"
/* @} */

static minui_backend* gr_backend = NULL;
extern int freetype_init();
extern char* freetype(unsigned long, struct FontGraph *);

/* SPRD: add for support rotate @{ */
void gr_rotate_180(GRSurface *dstSurf, GRSurface *srcSurf);
int gr_initRotate();

/* @} */
static int overscan_percent = OVERSCAN_PERCENT;
static int overscan_offset_x = 0;
static int overscan_offset_y = 0;

static unsigned char gr_current_r = 255;
static unsigned char gr_current_g = 255;
static unsigned char gr_current_b = 255;
static unsigned char gr_current_a = 255;

static GRSurface* gr_draw = NULL;
struct utf8_table
{
	int cmask;
	int cval;
	int shift;
	long lmask;
	long lval;
};

static struct utf8_table utf8_table[6] =
{
	{0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
	{0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
	{0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
	{0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
	{0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
	{0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
};

static bool outside(int x, int y)
{
    return x < 0 || x >= gr_draw->width || y < 0 || y >= gr_draw->height;
}

static void text_blend(unsigned char* src_p, int src_row_bytes, unsigned char* dst_p, int dst_row_bytes, int width, int height)
{
    int j, i;
    for (j = 0; j < height; ++j)
    {
        unsigned char* sx = src_p;
        unsigned char* px = dst_p;
        for (i = 0; i < width; ++i)
        {
            unsigned char a = *sx++;
            if (gr_current_a < 255) a = ((int)a * gr_current_a) / 255;
            if (a == 255)
            {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;
            }
            else if (a > 0)
            {
                *px = (*px * (255 - a) + gr_current_r * a) / 255;
                ++px;
                *px = (*px * (255 - a) + gr_current_g * a) / 255;
                ++px;
                *px = (*px * (255 - a) + gr_current_b * a) / 255;
                ++px;
                ++px;
            }
            else
            {
                px += 4;
            }
        }
        src_p += src_row_bytes;
        dst_p += dst_row_bytes;
    }
}

int utf8towc(wchar_t *p, const char *s, int n)
{
    wchar_t l;
    int c0, c, nc;
    struct utf8_table *t;
    nc = 0;
    c0 = *s;
    l = c0;
    for (t = utf8_table; t->cmask; t++)
    {
        nc++;
        if ((c0 & t->cmask) == t->cval)
        {
            l &= t->lmask;
            if (l < (wchar_t)t->lval)
                return -nc;
            *p = l;
            return nc;
        }
        if (n <= nc)
            return 0;
        s++;
        c = (*s ^ 0x80) & 0xFF;
        if (c & 0xC0)
            return -nc;
        l = (l << 6) | c;
    }
    return -nc;
}

char* gr_text(int x, int y, const char *s)
{
    unsigned bytes;
    unsigned char* dst_p;
    wchar_t unicode;
    FontGraph graph;

    x += overscan_offset_x;
    y += overscan_offset_y;
    while(*s)
    {
        if(*(unsigned char*)(s) <= 0x20)
        {
            s++;
            x += CHAR_SIZE/2;
            continue;
        }
        bytes = utf8towc(&unicode, s, strlen(s));
        if(bytes <= 0)
            break;
        s += bytes;

        freetype((unsigned long)unicode, &graph);
        if(graph.top >= 0 && graph.left >= 0 && graph.width >0 && graph.height > 0)
        {
            y -= graph.top;
            x += graph.left;

            if(x < 0 || y < 0 ||x > gr_fb_width() || y > gr_fb_height())
                LOGD("char position: x = %d, y = %d, width = %d, height = %d", x, y, graph.width, graph.height);

            dst_p = gr_draw->data + y * gr_draw->row_bytes + x * gr_draw->pixel_bytes;

            x += graph.width;
            y += graph.top;

            if(x >= gr_fb_width())
                return (s-bytes);

            text_blend(graph.map, graph.width, dst_p, gr_draw->row_bytes, graph.width, graph.height);
            free(graph.map);
        }
        else
        {
            LOGE("s=%s, (%d, %d, %d, %d)",s,graph.top, graph.left, graph.width, graph.height);
        }

    }

    return NULL;
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    gr_current_r = r;
    gr_current_g = g;
    gr_current_b = b;
    gr_current_a = a;
}

void gr_clear()
{
	int y, x;

    if (gr_current_r == gr_current_g && gr_current_r == gr_current_b)
    {
        memset(gr_draw->data, gr_current_r, gr_draw->height * gr_draw->row_bytes);
    }
    else
    {
        unsigned char* px = gr_draw->data;
        for (y = 0; y < gr_draw->height; ++y)
        {
            for (x = 0; x < gr_draw->width; ++x)
            {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;
            }
            px += gr_draw->row_bytes - (gr_draw->width * gr_draw->pixel_bytes);
        }
    }
}

void gr_fill(int x1, int y1, int x2, int y2)
{
	x1 += overscan_offset_x;
	y1 += overscan_offset_y;

	x2 += overscan_offset_x;
	y2 += overscan_offset_y;

	if (outside(x1, y1) || outside(x2-1, y2-1))
		return;

	unsigned char* p = gr_draw->data + y1 * gr_draw->row_bytes + x1 * gr_draw->pixel_bytes;
	if (gr_current_a == 255)
	{
		int x, y;
		for (y = y1; y < y2; ++y)
		{
			unsigned char* px = p;
			for (x = x1; x < x2; ++x)
			{
				*px++ = gr_current_r;
				*px++ = gr_current_g;
				*px++ = gr_current_b;
				px++;
			}
			p += gr_draw->row_bytes;
		}
	}
	else if (gr_current_a > 0)
	{
		int x, y;
		for (y = y1; y < y2; ++y)
		{
			unsigned char* px = p;
			for (x = x1; x < x2; ++x)
			{
				*px = (*px * (255-gr_current_a) + gr_current_r * gr_current_a) / 255;
				++px;
				*px = (*px * (255-gr_current_a) + gr_current_g * gr_current_a) / 255;
				++px;
				*px = (*px * (255-gr_current_a) + gr_current_b * gr_current_a) / 255;
				++px;
				++px;
			}
			p += gr_draw->row_bytes;
		}
	}
}

void gr_draw_point(int x, int y)
{
	x += overscan_offset_x;
	y += overscan_offset_y;

	unsigned char *p = gr_draw->data + y * gr_draw->row_bytes + x * gr_draw->pixel_bytes;
	if (gr_current_a == 255)
	{
		*p++ = gr_current_r;
		*p++ = gr_current_g;
		*p++ = gr_current_b;
	}
	else if(gr_current_a > 0)
	{
		*p = ( (*p) * (255-gr_current_a) + gr_current_r * gr_current_a) / 255;
		++p;
		*p = ( (*p) * (255-gr_current_a) + gr_current_g * gr_current_a) / 255;
		++p;
		*p = ( (*p) * (255-gr_current_a) + gr_current_b * gr_current_a) / 255;
	}
}

void gr_blit(GRSurface* source, int sx, int sy, int w, int h, int dx, int dy) {
    if (source == NULL) return;

    if (gr_draw->pixel_bytes != source->pixel_bytes) {
        printf("gr_blit: source has wrong format\n");
        return;
    }

    dx += overscan_offset_x;
    dy += overscan_offset_y;

    if (outside(dx, dy) || outside(dx+w-1, dy+h-1)) return;

    unsigned char* src_p = source->data + sy*source->row_bytes + sx*source->pixel_bytes;
    unsigned char* dst_p = gr_draw->data + dy*gr_draw->row_bytes + dx*gr_draw->pixel_bytes;

    int i;
    for (i = 0; i < h; ++i) {
        memcpy(dst_p, src_p, w * source->pixel_bytes);
        src_p += source->row_bytes;
        dst_p += gr_draw->row_bytes;
    }
}

unsigned int gr_get_width(GRSurface* surface) {
    if (surface == NULL) {
        return 0;
    }
    return surface->width;
}

unsigned int gr_get_height(GRSurface* surface) {
    if (surface == NULL) {
        return 0;
    }
    return surface->height;
}

static void gr_init_font(void)
{
    freetype_init();
}

void gr_tp_flag(int flag)
{
	tp_flag1 = flag;
	return;
}

void gr_flip() {
    unsigned char *tmpbuf = gr_draw->data;

    gr_draw = gr_backend->flip(gr_backend);
    if(!tp_flag1){
        LOGD("gr_flip gr_draw->data len: %d, %d IN", strlen(tmpbuf), __LINE__);
        memcpy(gr_draw->data, tmpbuf, gr_draw->row_bytes*gr_draw->height);
    }
}

int gr_init(void)
{
    gr_init_font();
    gr_initRotate();

    gr_backend = open_adf();
    if (gr_backend) {
        gr_draw = gr_backend->init(gr_backend);
        if (!gr_draw) {
            gr_backend->exit(gr_backend);
        }
    }

    if (!gr_draw) {
        gr_backend = open_fbdev();
        gr_draw = gr_backend->init(gr_backend);
        if (gr_draw == NULL) {
            return -1;
        }
    }
    overscan_offset_x = gr_draw->width * overscan_percent / 100;
    overscan_offset_y = gr_draw->height * overscan_percent / 100;

    return 0;
}

void gr_exit(void)
{
    gr_backend->exit(gr_backend);
}

minui_backend* gr_backend_get(void)
{
	return gr_backend;
}

GRSurface* gr_draw_get(void)
{
	return gr_draw;
}

int gr_pixel_bytes(void)
{
	return gr_draw->pixel_bytes;
}

int gr_fb_width(void)
{
    return gr_draw->width - 2*overscan_offset_x;
}

int gr_fb_height(void)
{
    return gr_draw->height - 2*overscan_offset_y;
}

void gr_fb_blank(bool blank)
{
    gr_backend->blank(gr_backend, blank);
}

//changed begin
void gr_rotate_180(GRSurface *dstSurf, GRSurface *srcSurf)
{
	int height = srcSurf->height;
	int width = srcSurf->width;
	int row_bytes = srcSurf->row_bytes;
	unsigned char* src_p = srcSurf->data;
	unsigned char* dst_p = dstSurf->data + row_bytes*height -1;
	int i, j;
	for (j = 0; j < height; ++j) {
	    unsigned char* sx = src_p;
	    unsigned char* px = dst_p ;
	    for (i = 0; i < width; ++i) {
		*(px-0) = *(sx+3);
		*(px-1) = *(sx+2);
		*(px-2) = *(sx+1);
		*(px-3) = *(sx+0);
		px-=4;
		sx+=4;
	    }
	    src_p += row_bytes;
	    dst_p -= row_bytes;
	}
}
//changed end

int gr_initRotate()
{
   if (s_rotate_type == ROTATE_UNKNOWN) {
        /* SPRD: add for support rotate @{ */
        char rotate_str[PROPERTY_VALUE_MAX+1];
        property_get("ro.sf.hwrotation", rotate_str, "0");
        printf("in %s: ro.sf.hwrotation=%s\n",__func__,rotate_str);
        if (!strcmp(rotate_str, "90")) {
            printf("warning: rotate 90 not support\n");
            s_rotate_type = ROTATE_90;
        } else if (!strcmp(rotate_str, "180")) {
            printf("rotate: rotate 180 support\n");
            s_rotate_type = ROTATE_180;
        } else if (!strcmp(rotate_str, "270")) {
            printf("rotate: rotate 270 not support\n");
            s_rotate_type = ROTATE_270;
        } else if (!strcmp(rotate_str, "0")) {
            printf("rotate: rotate 0 support\n");
            s_rotate_type = ROTATE_0;
        } else {
        }
    }

   return s_rotate_type;
}
