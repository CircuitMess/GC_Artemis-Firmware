#include <freertos/FreeRTOS.h>
#include "I2C.h"
#include <driver/i2c.h>
#include <vector>
#include <cstring>

I2C::I2C(i2c_port_t port, gpio_num_t sda, gpio_num_t scl) : port(port){
	i2c_config_t cfg = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = sda,
		.scl_io_num = scl,
		.sda_pullup_en = false,
		.scl_pullup_en = false
	};
	cfg.master.clk_speed = 400000;

	ESP_ERROR_CHECK(i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0));
	ESP_ERROR_CHECK(i2c_param_config(port, &cfg));

}

I2C::~I2C(){
	ESP_ERROR_CHECK(i2c_driver_delete(port));
}

void I2C::scan(TickType_t timeout){
	printf("I2C scan:\n");
	for(int i = 0; i < 127; i++){
		if(probe(i, timeout) == ESP_OK){
			printf("Found device on addr 0x%x\n", i);
		}
	}
	printf("Scan done.\n");
}

esp_err_t I2C::probe(uint8_t addr, TickType_t timeout){
	std::lock_guard lock(mut);

	auto cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
	i2c_master_stop(cmd);
	auto status = i2c_master_cmd_begin(port, cmd, timeout);
	i2c_cmd_link_delete(cmd);
	return status;
}

esp_err_t I2C::write(uint8_t addr, const uint8_t* data, size_t size, TickType_t wait){
	std::lock_guard lock(mut);
	return i2c_master_write_to_device(port, addr, data, size, wait);
}

esp_err_t I2C::write(uint8_t addr, uint8_t data, TickType_t wait){
	return write(addr, &data, 1, wait);
}

esp_err_t I2C::write(uint8_t addr, std::initializer_list<uint8_t> data, TickType_t wait){
	std::vector<uint8_t> buf(data);
	return write(addr, buf.data(), buf.size(), wait);
}

esp_err_t I2C::read(uint8_t addr, uint8_t* data, size_t size, TickType_t wait){
	std::lock_guard lock(mut);
	return i2c_master_read_from_device(port, addr, data, size, wait);
}

esp_err_t I2C::read(uint8_t addr, uint8_t& data, TickType_t wait){
	return read(addr, &data, 1, wait);
}

esp_err_t I2C::write_read(uint8_t addr, const uint8_t* wbuf, size_t wsize, uint8_t* rbuf, size_t rsize, TickType_t wait){
	std::lock_guard lock(mut);
	return i2c_master_write_read_device(port, addr, wbuf, wsize, rbuf, rsize, wait);
}

esp_err_t I2C::write_read(uint8_t addr, uint8_t wdata, uint8_t* rbuf, size_t rsize, TickType_t wait){
	return write_read(addr, &wdata, 1, rbuf, rsize, wait);
}

esp_err_t I2C::write_read(uint8_t addr, const uint8_t* wbuf, size_t wsize, uint8_t& rdata, TickType_t wait){
	return write_read(addr, wbuf, wsize, &rdata, 1, wait);
}

esp_err_t I2C::write_read(uint8_t addr, uint8_t wdata, uint8_t& rdata, TickType_t wait){
	return write_read(addr, &wdata, 1, &rdata, 1, wait);
}

esp_err_t I2C::writeReg(uint8_t addr, uint8_t reg, const uint8_t* data, size_t size, TickType_t wait){
	std::vector<uint8_t> buf(size+1);
	buf[0] = reg;
	memcpy(buf.data()+1, data, size);
	return write(addr, buf.data(), buf.size(), wait);
}

esp_err_t I2C::writeReg(uint8_t addr, uint8_t reg, uint8_t data, TickType_t wait){
	return writeReg(addr, reg, &data, 1, wait);
}

esp_err_t I2C::writeReg(uint8_t addr, uint8_t reg, std::initializer_list<uint8_t> data, TickType_t wait){
	std::vector<uint8_t> buf(data);
	return writeReg(addr, reg, buf.data(), buf.size(), wait);
}

esp_err_t I2C::readReg(uint8_t addr, uint8_t reg, uint8_t* data, size_t size, TickType_t wait){
	return write_read(addr, reg, data, size, wait);
}

esp_err_t I2C::readReg(uint8_t addr, uint8_t reg, uint8_t& data, TickType_t wait){
	return readReg(addr, reg, &data, 1, wait);
}
