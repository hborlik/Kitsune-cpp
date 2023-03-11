from ctypes import *

libfestats = CDLL("/home/hborlik/Documents/homework/CSC522/project/Kitsune-cpp/build/eBPF_XDP/libfestats.so")
print(libfestats)
print(libfestats.stat_update_and_process_decay)
print(libfestats.stat_parse_and_hash)

N_INC_STATS = 5
# struct inc_stat {
#     __u64   last_t;     // timestamp  for last inserted measurement nanoseconds
# 	  int32_t CF1		[N_INC_STATS];
#     int32_t CF2		[N_INC_STATS];
#     int32_t w		    [N_INC_STATS];
#     bool  isTypeDiff;
# };
class inc_stat(Structure):
    _fields_ = [
            ('last_t', c_ulonglong),
            ('CF1', c_int32 * N_INC_STATS),
            ('CF2', c_int32 * N_INC_STATS),
            ('w', c_int32 * N_INC_STATS),
            ('isTypeDiff', c_bool),
        ]

# struct address_hash {
# 	__u64 vals[1]; // [ADDRESS_WILDCARD]
# };
class address_hash(Structure):
    _fields_ = [
        ('vals', c_ulonglong * 1),
    ]

# struct hash {
# 	struct address_hash src;
# 	struct address_hash dst;
# 	__u64 src_port;
# 	__u64 dst_port;
# 	__u32 mix_hash;
# };
class hash(Structure):
    _fields_ = [
            ('src', address_hash),
            ('dst', address_hash),
            ('src_port', c_ulonglong),
            ('dst_port', c_ulonglong),
            ('mix_hash', c_int32),
        ]

# int stat_update_and_process_decay(__u64 timestamp, int32_t fx_value, struct inc_stat *stat)
libfestats.stat_update_and_process_decay.restype = c_int
libfestats.stat_update_and_process_decay.argtypes = [c_ulonglong, c_int, POINTER(inc_stat)]

# int stat_parse_and_hash(void *data_end, void *data, struct hash *ph)
libfestats.stat_parse_and_hash.restype = c_int
libfestats.stat_parse_and_hash.argtypes = [c_void_p, c_void_p, POINTER(hash)]