valuescan
=========

Scan files for binary values.

Usage
-----

	usage: valuescan [options] format:value [format:value...] [--] [file...]
	
	OPTIONS:
	        -h, --help                   print this help message
	        -s, --start-offset=OFFSET    start scanning at OFFSET
	        -e, --end-offset=OFFSET      end scanning at OFFSET-1
	        -p, --print-format=FORMAT    use FORMAT for messages
	                  %% ... %
	                  %f ... filename
	                  %o ... offset
	                  %s ... size of matched value
	                  %t ... format:value tuple as provided by user
	                  %v ... value as provided by user
	                  %x ... value as hex (lower case)
	                  %X ... value as hex (upper case)
	        -0, --print0                 separate lines with null bytes

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

**Note:** The floating point stuff needs testing.
