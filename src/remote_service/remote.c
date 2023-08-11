
#include "remote.h"


#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME)-1)

static K_SEM_DEFINE(bt_init_ok, 1, 1);

static int8_t data[240] = {0};
//static uint8_t button_value = 5;

enum bt_button_notifications_enabled notifications_enabled;
static struct bt_remote_service_cb remote_callbacks;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_REMOTE_SERV_VAL),
};


/* Declarations */
static ssize_t read_accel_characteristic_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);
void accel_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);



BT_GATT_SERVICE_DEFINE(remote_srv,
BT_GATT_PRIMARY_SERVICE(BT_UUID_REMOTE_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_REMOTE_BUTTON_CHRC,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_accel_characteristic_cb, NULL, NULL),
    BT_GATT_CCC(accel_chrc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);


/* Callbacks */

void MTU_exchange_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
    uint16_t payload_length;
	if (!err) {
		printk("MTU exchange done. "); 
		payload_length=bt_gatt_get_mtu(conn)-3; //3 bytes ATT header
	} else {
		printk("MTU exchange failed (err %" PRIu8 ")", err);
	}
}


void request_mtu_exchange(struct bt_conn *conn)
{	int err;
	static struct bt_gatt_exchange_params exchange_params;
	exchange_params.func = MTU_exchange_cb;

	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		printk("MTU exchange failed (err %d)", err);
	} else {
		printk("MTU exchange pending");
	}

}

/*void update_mtu(struct bt_conn *conn)
{
    int err;
    exchange_params.func = exchange_func;

    err = bt_gatt_exchange_mtu(conn, &exchange_params);
    if (err) {
        printk("bt_gatt_exchange_mtu failed (err %d)", err);
    }
}

static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	printk("MTU exchange %s", att_err == 0 ? "successful" : "failed");
    if (!att_err) {
        uint16_t payload_mtu = bt_gatt_get_mtu(conn) - 3;   // 3 bytes used for Attribute headers.
        printk("New MTU: %d bytes", payload_mtu);
    }
}*/


void on_sent(struct bt_conn *conn, void *user_data)
{
    ARG_UNUSED(user_data);
    printk("Notification sent on connection %p", (void *)conn);
}

static ssize_t read_accel_characteristic_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &data,
				 sizeof(data));
}

void accel_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    printk("Notifications %s", notif_enabled? "enabled":"disabled");

    notifications_enabled = notif_enabled? BT_BUTTON_NOTIFICATIONS_ENABLED:BT_BUTTON_NOTIFICATIONS_DISABLED;
    if (remote_callbacks.notif_changed) {
        remote_callbacks.notif_changed(notifications_enabled);
    }
}


/* Custom functions */

int send_button_notification(struct bt_conn *conn)
{
    int err = 0;

    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &remote_srv.attrs[2];

    params.attr = attr;
    params.data = &data;
    params.len = 240;
    params.func = on_sent;


    err = bt_gatt_notify_cb(conn, &params);

    return err;
}

void set_accel_status(int8_t x_int, int8_t x_dec, int8_t y_int, int8_t y_dec, int8_t z_int, int8_t z_dec, int8_t ppg, int count)
{
    int factor = count * 6;
    data[factor] = x_int;
    data[factor+1] = x_dec;
    data[factor+2] = y_int;
    data[factor+3] = y_dec;
    data[factor+4] = z_int;
    data[factor+5] = z_dec;
    data[factor+6] = ppg;
}

int bluetooth_init(struct bt_conn_cb *bt_cb, struct bt_remote_service_cb *remote_cb)
{
    int err;
    printk("Initializing bluetooth...");

    if (bt_cb == NULL || remote_cb == NULL) {
        return -NRFX_ERROR_NULL;
    }

    bt_conn_cb_register(bt_cb);
    remote_callbacks.notif_changed = remote_cb->notif_changed;

    err = bt_enable(NULL);
    if (err) {
        printk("bt_enable returned %d", err);
        return err;
    }
    
    k_sem_take(&bt_init_ok, K_FOREVER);

    err = bt_le_adv_start(BT_LE_ADV_CONN/*BT_LE_ADV_PARAM(BIT(1),0x00f0, 0x00f0, NULL)*/, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Couldn't start advertising (err = %d)", err);
        return err;
    }



    return err;
}