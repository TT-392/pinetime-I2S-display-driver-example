enum display_byte_type {
    D_CMD,
    D_DAT,
    D_END
};

void I2S_init();

void I2S_add_data(const uint8_t* data, int sizeDiv4, enum display_byte_type type);

void I2S_add_end();

void I2S_start();

void I2S_reset();

int I2S_cleanup();
