#ifndef CLOCKSTAR_FIRMWARE_I2C_H
#define CLOCKSTAR_FIRMWARE_I2C_H

#include <freertos/FreeRTOS.h>
#include <hal/i2c_types.h>
#include <hal/gpio_types.h>
#include <esp_err.h>
#include <freertos/portmacro.h>
#include <initializer_list>
#include <mutex>

class I2C {
public:
	I2C(i2c_port_t port, gpio_num_t sda, gpio_num_t scl);
	virtual ~I2C();

	void scan(TickType_t timeout = 5);
	esp_err_t probe(uint8_t addr, TickType_t timeout = 5);

	esp_err_t write(uint8_t addr, const uint8_t* data, size_t size, TickType_t wait = portMAX_DELAY);
	esp_err_t write(uint8_t addr, uint8_t data, TickType_t wait = portMAX_DELAY);
	esp_err_t write(uint8_t addr, std::initializer_list<uint8_t> data, TickType_t wait = portMAX_DELAY);

	esp_err_t read(uint8_t addr, uint8_t* data, size_t size, TickType_t wait = portMAX_DELAY);
	esp_err_t read(uint8_t addr, uint8_t& data, TickType_t wait = portMAX_DELAY);

	esp_err_t write_read(uint8_t addr, const uint8_t* wbuf, size_t wsize, uint8_t* rbuf, size_t rsize, TickType_t wait = portMAX_DELAY);
	esp_err_t write_read(uint8_t addr, uint8_t wdata, uint8_t* rbuf, size_t rsize, TickType_t wait = portMAX_DELAY);
	esp_err_t write_read(uint8_t addr, const uint8_t* wbuf, size_t wsize, uint8_t& rdata, TickType_t wait = portMAX_DELAY);
	esp_err_t write_read(uint8_t addr, uint8_t wdata, uint8_t& rdata, TickType_t wait = portMAX_DELAY);

	esp_err_t writeReg(uint8_t addr, uint8_t reg, const uint8_t* data, size_t size, TickType_t wait = portMAX_DELAY);
	esp_err_t writeReg(uint8_t addr, uint8_t reg, uint8_t data, TickType_t wait = portMAX_DELAY);
	esp_err_t writeReg(uint8_t addr, uint8_t reg, std::initializer_list<uint8_t> data, TickType_t wait = portMAX_DELAY);

	esp_err_t readReg(uint8_t addr, uint8_t reg, uint8_t* data, size_t size, TickType_t wait = portMAX_DELAY);
	esp_err_t readReg(uint8_t addr, uint8_t reg, uint8_t& data, TickType_t wait = portMAX_DELAY);

private:
	const i2c_port_t port;
	std::mutex mut;

};


#endif //CLOCKSTAR_FIRMWARE_I2C_H
