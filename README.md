valuescan
=========

Scan files for binary values.

Usage
-----

	Usage: valuescan [options] format:value[,format:value...]... [--] [file...]
	
	BLOB FORAMTS:
	
	        text .... either simple string [-+\._:/\\%a-zA-Z0-9]* or C-like quoted string
	        hex ..... hex encoded binary
	        file .... read needle from given file (filename is encoded like text)
	
	NUMBER FORMATS:
	
	        Format | Type    | Bits |   Sign   |  Byte Order
	        ------ | ------- | ---- | -------- | --------------
	        i8     | integer |    8 | signed   |     ----
	        u8     | integer |    8 | unsigned |     ----
	        i16le  | integer |   16 | signed   | little endian
	        u16le  | integer |   16 | unsigned | little endian
	        i32le  | integer |   32 | signed   | little endian
	        u32le  | integer |   32 | unsigned | little endian
	        i64le  | integer |   64 | signed   | little endian
	        u64le  | integer |   64 | unsigned | little endian
	        i16be  | integer |   16 | signed   | big endian
	        u16be  | integer |   16 | unsigned | big endian
	        i32be  | integer |   32 | signed   | big endian
	        u32be  | integer |   32 | unsigned | big endian
	        i64be  | integer |   64 | signed   | big endian
	        u64be  | integer |   64 | unsigned | big endian
	        f32le  | float   |   32 | signed   | little endian
	        f64le  | float   |   64 | signed   | little endian
	        f32be  | float   |   32 | signed   | big endian
	        f64be  | float   |   64 | signed   | big endian
	
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
	
	EXAMPLES:
	
	        Find a 1024x1024 or 2048x2048 square:
	
	                valuescan u32le:1024,u32le:1024 u32le:2048,u32le:2048 -- file.bin
	
	Report bugs to: https://github.com/panzi/valuescan/issues

**Note:** The floating point stuff needs testing.
