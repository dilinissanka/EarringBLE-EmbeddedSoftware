
// These snippets are intended to help following the Webinar: Developing Bluetooth Low Energy products using nRF Connect SDK
---------------------------

// The following snippet belongs to remote.h:
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>


-------------------------------
// The following snippet belongs to remote.c:

int bluetooth_init(void)
{
    LOG_INF("Initializing Bluetooth");

    int err;

    err = bt_enable(bt_ready);
	if (err) {
		LOG_ERR("bt_enable returned %d", err);
	}

    return err;
}
----------------------------
// The following snippet belongs to remote.c:
void bt_ready(int err)
{
    if (err) {
        LOG_ERR("bt_enable returned %d", err);
    }
    
}

-----------------------------
// The following snippet belongs to remote.c:

static K_SEM_DEFINE(bt_init_ok, 1, 1);
------------------------------
// The following snippet belongs to remote.c:
void bt_ready(int err)
{
    if (err) {
        LOG_ERR("bt_ready err: %d", err);
    }
    k_sem_give(&bt_init_ok);
}

-------------------------
// The following snippet belongs to remote.c:
bluetooth_init()
{
	k_sem_take(&bt_init_ok, K_FOREVER);
	
}

----------------------------
// The following snippet belongs to prj.conf:

# Configure Bluetooth
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DEVICE_NAME="Remote_controller"
CONFIG_BT_DEVICE_APPEARANCE=0
CONFIG_BT_MAX_CONN=1
CONFIG_BT_LL_SOFTDEVICE=y

CONFIG_ASSERT=y
----------------------------
// The following snippet belongs to remote.h:
/** @brief UUID of the Remote Service. **/
#define BT_UUID_REMOTE_SERV_VAL \
	BT_UUID_128_ENCODE(0xe9ea0001, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)

#define BT_UUID_REMOTE_SERVICE  BT_UUID_DECLARE_128(BT_UUID_REMOTE_SERV_VAL)


------------------------------
// The following snippet belongs to remote.c:


#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME)-1)


static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_REMOTE_SERV_VAL),
};
----------------------------------
// The following snippet belongs to remote.c -> bluetooth_init()

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err){
        LOG_ERR("couldn't start advertising (err = %d", err);
        return err;
    }
	
------------------------------------
// The following snippet belongs to main.c

void on_connected(struct bt_conn *conn, uint8_t err);
void on_disconnected(struct bt_conn *conn, uint8_t reason);

static struct bt_conn *current_conn;

struct bt_conn_cb bluetooth_callbacks = {
	.connected 		= on_connected,
	.disconnected 	= on_disconnected,
};

void on_connected(struct bt_conn *conn, uint8_t err)
{
	if(err) {
		LOG_ERR("connection err: %d", err);
		return;
	}
	LOG_INF("Connected.");
	current_conn = bt_conn_ref(conn);
	dk_set_led_on(CONN_STATUS_LED);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason: %d)", reason);
	dk_set_led_off(CONN_STATUS_LED);
	if(current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
}
---------------------------------------
// The following snippet belongs to main():
err = bluetooth_init(&bluetooth_callbacks);

---------------------------------
// The following snippet belongs to remote.h:
int bluetooth_init(struct bt_conn_cb *bt_cb);

-----------------------------
// The following snippet belongs to remote.c:

int bluetooth_init(struct bt_conn_cb *bt_cb)
{
    LOG_INF("Initializing Bluetooth");

    int err;

    if (bt_cb == NULL)
    {
        return -NRFX_ERROR_NULL;
    }
    bt_conn_cb_register(bt_cb);
	...

-----------------------
// The following snippet belongs to remote.c:

BT_GATT_SERVICE_DEFINE(remote_srv,
BT_GATT_PRIMARY_SERVICE(BT_UUID_REMOTE_SERVICE),
);

----------------------------
// The following snippet belongs to remote.h:

/** @brief UUID of the Button Characteristic. **/
#define BT_UUID_REMOTE_BUTTON_CHRC_VAL \
	BT_UUID_128_ENCODE(0xe9ea0002, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)
	
#define BT_UUID_REMOTE_BUTTON_CHRC 	BT_UUID_DECLARE_128(BT_UUID_REMOTE_BUTTON_CHRC_VAL)

--------------------------
// The following snippet belongs to remote.c:
static uint8_t button_value = 0;

static ssize_t read_button_characteristic_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);

BT_GATT_SERVICE_DEFINE(remote_srv,
BT_GATT_PRIMARY_SERVICE(BT_UUID_REMOTE_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_REMOTE_BUTTON_CHRC,
                    BT_GATT_CHRC_READ,
                    BT_GATT_PERM_READ,
                    read_button_characteristic_cb, NULL, NULL),

);


static ssize_t read_button_characteristic_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &button_value,
				 sizeof(button_value));
}


---------------------------
// The following snippet belongs to remote.c:

void set_button_value(uint8_t btn_value)
{
    button_value = btn_value;
}

--------------------------------
// The following snippet belongs to remote.h:

void set_button_value(uint8_t btn_value);

------------------------------
// The following snippet belongs to main.c -> button_handler():

set_button_value(button_pressed);

-----------------------------
// The following snippet belongs to remote.c:

void button_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

BT_GATT_SERVICE_DEFINE(remote_srv,
BT_GATT_PRIMARY_SERVICE(BT_UUID_REMOTE_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_REMOTE_BUTTON_CHRC,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_button_characteristic_cb, NULL, NULL),
    BT_GATT_CCC(button_chrc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void button_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Notifications %s", notif_enabled? "enabled":"disabled");

}

---------------------------------
// The following snippet belongs to remote.h

enum bt_button_notifications_enabled {
	BT_BUTTON_NOTIFICATIONS_ENABLED,
	BT_BUTTON_NOTIFICATIONS_DISABLED,
};

----------------------------------
// The following snippet belongs to remote.c:

enum bt_button_notifications_enabled notifications_enabled;

void button_chrc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Notifications %s", notif_enabled? "enabled":"disabled");

    notifications_enabled = notif_enabled? BT_BUTTON_NOTIFICATIONS_ENABLED:BT_BUTTON_NOTIFICATIONS_DISABLED;    
}

------------------------------------

// The following snippet belongs to remote.h:
struct bt_remote_service_cb {
	void (*notif_changed)(enum bt_button_notifications_enabled status);
};

------------------------------------

// The following snippet belongs to remote.c:

static struct bt_remote_service_cb remote_callbacks;

------------------------------------

// The following snippet belongs to main.c:
struct bt_remote_service_cb remote_callbacks = {
	.notif_changed = on_notif_changed,
};

void on_notif_changed(enum bt_button_notifications_enabled status)
{
	if (status == BT_BUTTON_NOTIFICATIONS_ENABLED) {
		LOG_INF("Notifications enabled");
	}
	else {
		LOG_INF("Notificatons disabled");
	}
}

------------------------------------

// The following snippet belongs to main():
err = bluetooth_init(&connection_callbacks, &remote_callbacks);

------------------------------------

// The following snippet belongs to remote.h:
int bluetooth_init(struct bt_conn_cb *bt_cb, struct bt_remote_service_cb *remote_cb);

------------------------------------

// The following snippet belongs to remote.c:
static struct bt_remote_service_cb remote_callbacks;


int bluetooth_init(struct bt_conn_cb *bt_cb, struct bt_remote_service_cb *remote_cb)
{
    LOG_INF("Initializing Bluetooth");

    int err;

    if (bt_cb == NULL || remote_cb == NULL)
    {
        return -NRFX_ERROR_NULL;
    }
    bt_conn_cb_register(bt_cb);
    remote_callbacks.notif_changed = remote_cb->notif_changed;
	...

------------------------------------

// The following snippet belongs to remote.c

void on_sent(struct bt_conn *conn, void *user_data);

int send_button_notification(struct bt_conn *conn, uint8_t value)
{
    int err = 0;

    struct bt_gatt_notify_params params = {0};
    const struct bt_gatt_attr *attr = &remote_srv.attrs[2];

    params.attr = attr;
    params.data = &value;
    params.len = 1;
    params.func = on_sent;

    err = bt_gatt_notify_cb(conn, &params);

    return err;
}

void on_sent(struct bt_conn *conn, void *user_data)
{
    ARG_UNUSED(user_data);
    LOG_INF("Notification sent on connection %p", (void *)conn);
}

------------------------------------

// The following snippet belongs to remote.h:
int send_button_notification(struct bt_conn *conn, uint8_t value);

------------------------------------

// The following snippet belongs to main.c -> button_handler():
button_handler():
		err = send_button_notification(current_conn, button_pressed);
		if (err) {
			LOG_ERR("couldn't send notification (err: %d)", err);
		}


------------------------------------

// The following snippet belongs to remote.h:

/** @brief UUID of the Message Characteristic. **/
#define BT_UUID_REMOTE_MESSAGE_CHRC_VAL \
	BT_UUID_128_ENCODE(0xe9ea0003, 0xe19b, 0x482d, 0x9293, 0xc7907585fc48)

#define BT_UUID_REMOTE_SERVICE  		BT_UUID_DECLARE_128(BT_UUID_REMOTE_SERV_VAL)
#define BT_UUID_REMOTE_BUTTON_CHRC 		BT_UUID_DECLARE_128(BT_UUID_REMOTE_BUTTON_CHRC_VAL)
#define BT_UUID_REMOTE_MESSAGE_CHRC 	BT_UUID_DECLARE_128(BT_UUID_REMOTE_MESSAGE_CHRC_VAL)

------------------------------------

// The following snippet belongs to remote.c:

static ssize_t on_write(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

BT_GATT_SERVICE_DEFINE(remote_srv,
BT_GATT_PRIMARY_SERVICE(BT_UUID_REMOTE_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_REMOTE_BUTTON_CHRC,
                    BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ,
                    read_button_characteristic_cb, NULL, NULL),
    BT_GATT_CCC(button_chrc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_REMOTE_MESSAGE_CHRC,
                    BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                    NULL, on_write, NULL),
);

static ssize_t on_write(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{
	LOG_INF("Received data, handle %d, conn %p",
		attr->handle, (void *)conn);

	if (remote_callbacks.data_received) {
		remote_callbacks.data_received(conn, buf, len);
    }
	return len;
}


------------------------------------

// The following snippet belongs to remote.h:
struct bt_remote_service_cb {
	void (*notif_changed)(enum bt_button_notifications_enabled status);
	void (*data_received)(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
};

------------------------------------

// The following snippet belongs to main.c:
struct bt_remote_service_cb remote_callbacks = {
	.notif_changed = on_notif_changed,
	.data_received = on_data_received,
};

void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	uint8_t temp_str[len+1];
	memcpy(temp_str, data, len);
	temp_str[len] = 0x00;

	LOG_INF("Received data on conn %p. Len: %d", (void *)conn, len);
	LOG_INF("Data: %s", log_strdup(temp_str));
}
