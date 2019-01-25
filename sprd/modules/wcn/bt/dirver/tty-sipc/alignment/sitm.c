/*
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
#include <linux/kfifo.h>
#include "sitm.h"


static const uint8_t preamble_sizes[] = {
	HCI_COMMAND_PREAMBLE_SIZE,
	HCI_ACL_PREAMBLE_SIZE,
	HCI_SCO_PREAMBLE_SIZE,
	HCI_EVENT_PREAMBLE_SIZE
};

static struct packet_receive_data_t *receive_data = NULL;

int sitm_ini(void) {
	receive_data = kmalloc(sizeof(struct packet_receive_data_t), GFP_KERNEL);
	memset(receive_data, 0, sizeof(struct packet_receive_data_t));
	if(kfifo_alloc(&receive_data->fifo, HCI_HAL_SERIAL_BUFFER_SIZE, GFP_KERNEL)) {
		pr_err("no memory for sitm ring buf");
	}
	return 0;
}

int sitm_cleanup(void) {
	if(receive_data == NULL) {
		pr_err("bt sitm_cleanup fail: receive_data NULL");
		return -1;
	}
	kfifo_free(&receive_data->fifo);
	kfree(receive_data);
	receive_data = NULL;
	return 0;
}



static int data_ready(uint8_t *buf, uint32_t count)
{
	int ret = kfifo_out(&receive_data->fifo, buf, count);
	//pr_err("MYLOG read count: %d, byte: 0x%02x, ret: %d\n", count, buf[0], ret);
	return ret;
}

void parse_frame(data_ready_cb data_ready, frame_complete_cb frame_complete) {
	uint8_t byte;
	size_t buffer_size, bytes_read;
	while (data_ready(&byte, 1) == 1) {
		switch (receive_data->state) {
			case BRAND_NEW:
				if (byte > DATA_TYPE_EVENT || byte < DATA_TYPE_COMMAND) {
					pr_err("unknown head: 0x%02x\n", byte);
					break;
				}
				receive_data->type = byte;
				receive_data->bytes_remaining = preamble_sizes[PACKET_TYPE_TO_INDEX(receive_data->type)] + 1;
				memset(receive_data->preamble, 0, PREAMBLE_BUFFER_SIZE);
				receive_data->index = 0;
				receive_data->state = PREAMBLE;
			case PREAMBLE:
				receive_data->preamble[receive_data->index] = byte;
				receive_data->index++;
				receive_data->bytes_remaining--;

				if (receive_data->bytes_remaining == 0) {
					receive_data->bytes_remaining = (receive_data->type == DATA_TYPE_ACL) ? RETRIEVE_ACL_LENGTH(receive_data->preamble) : byte;
					buffer_size = receive_data->index + receive_data->bytes_remaining;

					if (!receive_data->buffer) {
						pr_err("%s error getting buffer for incoming packet of type %d and size %zd\n", __func__, receive_data->type, buffer_size);
						receive_data->state = receive_data->bytes_remaining == 0 ? BRAND_NEW : IGNORE;
						break;
					}
					memcpy(receive_data->buffer, receive_data->preamble, receive_data->index);
					receive_data->state = receive_data->bytes_remaining > 0 ? BODY : FINISHED;
				}
				break;
			case BODY:
				receive_data->buffer[receive_data->index] = byte;
				receive_data->index++;
				receive_data->bytes_remaining--;
				bytes_read = data_ready((receive_data->buffer + receive_data->index), receive_data->bytes_remaining);
				receive_data->index += bytes_read;
				receive_data->bytes_remaining -= bytes_read;
				receive_data->state = receive_data->bytes_remaining == 0 ? FINISHED : receive_data->state;
				break;
			case IGNORE:
				pr_err("PARSE IGNORE\n");
				receive_data->bytes_remaining--;
				if (receive_data->bytes_remaining == 0) {
					receive_data->state = BRAND_NEW;
					return;
				}
				break;
			case FINISHED:
				pr_err("%s the state machine should not have been left in the finished state.\n", __func__);
				break;
			default:
			pr_err("PARSE DEFAULT\n");
			break;
		}

		if (receive_data->state == FINISHED) {
			if (receive_data->type == DATA_TYPE_COMMAND
				|| receive_data->type == DATA_TYPE_ACL) {
				uint32_t tail = BYTE_ALIGNMENT - ((receive_data->index + BYTE_ALIGNMENT) % BYTE_ALIGNMENT);
				while (tail--) {
					receive_data->buffer[receive_data->index++] = 0;
				}
			}
			frame_complete(receive_data->buffer, receive_data->index);
			receive_data->state = BRAND_NEW;
		}
	}
}

int sitm_write(const uint8_t *buf, int count, frame_complete_cb frame_complete)
{
	int ret;
	//pr_err("MYLOG sitm_write in\n");
	if (!receive_data) {
		pr_err("hci fifo no memory\n");
		return count;
	}

	ret = kfifo_avail(&receive_data->fifo);
	if (ret == 0) {
		pr_err("hci fifo no memory\n");
		return ret;
	} else if (ret < count) {
		pr_err("hci fifo low memory\n");
		count = ret;
	}
	//pr_err("MYLOG kfifo_avail: %d\n", ret);

	kfifo_in(&receive_data->fifo, buf, count);
	parse_frame(data_ready, frame_complete);
	return count;
}


