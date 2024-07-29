#include "nturt/sensors.h"

// glibc includes
#include <errno.h>

// zephyr includes
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

// project includes
#include "nturt/err.h"

LOG_MODULE_REGISTER(nturt_sensors, CONFIG_NTURT_LOG_LEVEL);

/* function definition -------------------------------------------------------*/
int sensor_tol_chan_read(struct sensor_tol *tol, enum sensor_channel chan,
                         struct sensor_value *val) {
  if (err_get_errors() & tol->err) {
    return -ENODEV;
  }

  int ret;
  if ((ret = sensor_sample_fetch_chan(tol->dev, chan)) < 0 ||
      (ret = sensor_channel_get(tol->dev, chan, val)) < 0) {
    if (++tol->fail == tol->tol) {
      err_set_errors(tol->err, true);
      LOG_ERR("Sensor %s failed after %d consecutive tries", tol->dev->name,
              tol->fail);
    }

    return ret;
  } else {
    tol->fail = 0;
    return 0;
  }
}
