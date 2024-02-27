#ifndef BOARD_H
#define BOARD_H

/*PinOut definition*/
#define PIN_RED              (18)
#define PIN_GREEN            (5)
#define PIN_BLUE             (17)
#define WIFI_RESET_PIN       12
#define SOILPIN              32
#define HUM_CTRL_OUTPUT      25
#define TEMP_CTRL_OUTPUT     33
#define W_VLV_PIN            26
#define LIGHTS               27
#define I2C_MASTER_SDA_IO    21  /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_SCL_IO    22  /*!< GPIO number used for I2C master clock */

/*Perifirals settings*/
#define I2C_MASTER_NUM              I2C_NUM_0        /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          100000UL /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000
#define BH1750_SENSOR_ADDR          0x23        /*!< Slave address of the MPU9250 sensor */
#define HTU21_SENSOR_ADDR           0x40        /*!< Slave address of the MPU9250 sensor */


#endif