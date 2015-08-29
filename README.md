valuescan
=========

Scan a binary file for a value.

Usage
-----

	valuescan [-s start-offset] [-e end-offset] format:value [format:value...] [--] [file...]

### Supported Formats

| Format         | Description                                                   |
| -------------- | ------------------------------------------------------------- |
| text           | simple text                                                   |
| hex            | binary data encoded as a sequence hexadecimal values          |
| i8             | signed 8 bit integer value                                    |
| i16le          | signed 16 bit integer value in little endian encoding         |
| i32le          | signed 32 bit integer value in little endian encoding         |
| i64le          | signed 64 bit integer value in little endian encoding         |
| i16be          | signed 16 bit integer value in big endian encoding            |
| i32be          | signed 32 bit integer value in big endian encoding            |
| i64be          | signed 64 bit integer value in big endian encoding            |
| u8             | unsinged 8 bit integer value                                  |
| u16le          | unsigned 16 bit integer value in little endian encoding       |
| u32le          | unsigned 32 bit integer value in little endian encoding       |
| u64le          | unsigned 64 bit integer value in little endian encoding       |
| u16be          | unsigned 16 bit integer value in big endian encoding          |
| u32be          | unsigned 32 bit integer value in big endian encoding          |
| u64be          | unsigned 64 bit integer value in big endian encoding          |
| f32le          | 32 bit IEC 559 floating point value in little endian encoding |
| f64le          | 64 bit IEC 559 floating point value in little endian encoding |
| f32be          | 32 bit IEC 559 floating point value in big endian encoding    |
| f64be          | 64 bit IEC 559 floating point value in big endian encoding    |
