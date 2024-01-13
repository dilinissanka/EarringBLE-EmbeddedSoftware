/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>
#include <zephyr/drivers/i2c.h>

#include "remote.h"

#define RUN_STATUS_LED DK_LED1
#define CONN_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 20

bool connected = false;
int count = 0;
int val = -1;
static struct bt_conn *current_conn;

struct device *lis2dw12 = DEVICE_DT_GET_ANY(st_lis2dw12);
//struct device *accel = DEVICE_DT_GET(DT_NODELABEL(i21));
struct device *accel = DEVICE_DT_GET_ANY(st_lis2dw12);
const struct device *const dev = DEVICE_DT_GET_ANY(maxim_max30101);
const struct device *const hdc2010 = DEVICE_DT_GET_ANY(ti_hdc2010);

/* Declarations */
void on_connected(struct bt_conn *conn, uint8_t err);
void on_disconnected(struct bt_conn *conn, uint8_t reason);
void on_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info);
void on_notif_changed(enum bt_button_notifications_enabled status);
void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len);

struct bt_conn_cb bluetooth_callbacks = {
	.connected 		= on_connected,
	.disconnected 	= on_disconnected,
	.le_data_len_updated    = on_le_data_len_updated,
};
struct bt_remote_service_cb remote_callbacks = {
	.notif_changed = on_notif_changed,
    .data_received = on_data_received,
};

/* Callbacks */



void on_connected(struct bt_conn *conn, uint8_t err)
{
	if(err) {
		printk("connection err: %d", err);
		return;
	}
	printk("Connected.");
	current_conn = bt_conn_ref(conn);
	request_mtu_exchange(conn);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason: %d)", reason);
	if(current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
}

void on_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)
{
    uint16_t tx_len     = info->tx_max_len; 
    uint16_t tx_time    = info->tx_max_time;
    uint16_t rx_len     = info->rx_max_len;
    uint16_t rx_time    = info->rx_max_time;
    printk("Data length updated. Length %d/%d bytes, time %d/%d us", tx_len, rx_len, tx_time, rx_time);
}

void on_notif_changed(enum bt_button_notifications_enabled status)
{
	if (status == BT_BUTTON_NOTIFICATIONS_ENABLED) {
		printk("Notifications enabled");
	}
	else {
		printk("Notificatons disabled");
	} 
}

void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	uint8_t temp_str[len+1];
	memcpy(temp_str, data, len);
	temp_str[len] = 0x00;

	printk("Received data on conn %p. Len: %d", (void *)conn, len);
}


void accel_updatev1()
{
	// save it to a buffer, when reach certain length send, 240 max - done
	// measure current & put in shared slides, average current is 1.36 mA
	// change sleep function to app timer, so entire thing goes to sleep --> reduce power consumption
	// on chip step counting
	// modify android studio app
	count++;
	struct sensor_value acc[3];
	struct sensor_value ir;
	sensor_sample_fetch(lis2dw12);
	sensor_channel_get(lis2dw12, SENSOR_CHAN_ACCEL_XYZ, acc);
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_GREEN, &ir);
	set_accel_status((int8_t)acc[0].val1,(int8_t)acc[0].val2, (int8_t)acc[1].val1, (int8_t)acc[1].val2, (int8_t)acc[2].val1, (int8_t)acc[2].val2,(uint16_t)ir.val1 ,count);
	if(count == 29) {
		send_button_notification(current_conn);
	}
}

void ppg()
{
	uint8_t data[200] = {0};
	for(int i = 0; i < 20; i++) {
		//data[i] = 0xFF;
		struct sensor_value acc[3];
	 	struct sensor_value ir;
		struct sensor_value temp;
		sensor_sample_fetch(lis2dw12);
		sensor_channel_get(lis2dw12, SENSOR_CHAN_ACCEL_XYZ, acc);
	 	sensor_sample_fetch(dev);
	 	sensor_channel_get(dev, SENSOR_CHAN_IR, &ir);
		sensor_sample_fetch(hdc2010);
		sensor_channel_get(hdc2010, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	 	uint16_t ppg_data = (uint16_t)ir.val1;
	 	data[i*10] = (ppg_data >> 8) & 0xFF;
	 	data[(i*10) + 1] = ppg_data & 0xFF;
		data[(i*10) + 2] = (uint8_t)(acc[0].val1 & 0xFF);
		data[(i*10) + 3] = (uint8_t)(acc[0].val2 & 0xFF);
		data[(i*10) + 4] = (uint8_t)(acc[1].val1 & 0xFF);
		data[(i*10) + 5] = (uint8_t)(acc[1].val2 & 0xFF);
		data[(i*10) + 6] = (uint8_t)(acc[2].val1 & 0xFF);
		data[(i*10) + 7] = (uint8_t)(acc[2].val2 & 0xFF);

		data[(i*10) + 8] = (uint8_t)(temp.val1 & 0xFF);
		data[(i*10) + 9] = (uint8_t)(temp.val2 & 0xFF);

	}
	set_ppg(data);
	send_button_notification(current_conn);
	//60 bytes
	
}



/* Configurations */

/* main */

void main(void)
{
	int err;
	int blink_status = 0;
	/*printk("Hello World! %s\n", CONFIG_BOARD);
	err = bluetooth_init(&bluetooth_callbacks, &remote_callbacks);
	if (err) {
		printk("bt_enable returned %d", err);
	}*/

	if(!device_is_ready(lis2dw12)) {
		//printk("lis2dw12 device is not ready\n");
		return;
	}

	if (!device_is_ready(hdc2010)) {
		return;
	}


	/*uint8_t buf[2] = {0x2E, 0xD0};
	int ret = i2c_write(accel, buf, 2, 0x19);
	if(ret != 0) {
		return;
	}*/

	if (!device_is_ready(dev)) {
		printf("max30101 device %s is not ready\n", dev->name);
		return;
	}

	printk("Hello World! %s\n", CONFIG_BOARD);
	err = bluetooth_init(&bluetooth_callbacks, &remote_callbacks);
	if (err) {
		printk("bt_enable returned %d", err);
	}

	// send data to register where i enable fifo

	

    /*for(int i = 0; i < 10; i++){
		k_sleep(K_MSEC(1000));
	}*/

	for (;;) {
		
		/*if(count == 29) {
			count = -1;
		}*/
		ppg();
		/*if(count == 30) {
			ppg();
			count = 0;
		}
		count++;*/
		k_sleep(K_MSEC(400));
	}
	//k_timer_stop(&mytimer);
}
