import ctypes as ct

stats = ct.CDLL("/home/hborlik/Documents/homework/CSC522/project/Kitsune-cpp/build/eBPF_XDP/libfestats.so")

N_INC_STATS = 5
# struct inc_stat {
#     __u64   last_t;     // timestamp  for last inserted measurement nanoseconds
# 	  int32_t CF1		[N_INC_STATS];
#     int32_t CF2		[N_INC_STATS];
#     int32_t w		    [N_INC_STATS];
#     bool  isTypeDiff;
# };
class inc_stat(ct.Structure):
    _fields_ = [
            ('last_t', ct.c_ulonglong),
            ('CF1', ct.c_int32 * N_INC_STATS),
            ('CF2', ct.c_int32 * N_INC_STATS),
            ('w', ct.c_int32 * N_INC_STATS),
            ('isTypeDiff', ct.c_bool),
        ]

# struct address_hash {
# 	__u64 vals[1]; // [ADDRESS_WILDCARD]
# };
class address_hash(ct.Structure):
    _fields_ = [
        ('vals', ct.c_ulonglong * 1),
    ]

# struct hash {
# 	struct address_hash src;
# 	struct address_hash dst;
# 	__u64 src_port;
# 	__u64 dst_port;
# 	__u32 mix_hash;
# };
class hash(ct.Structure):
    _fields_ = [
            ('src', address_hash),
            ('dst', address_hash),
            ('src_port', ct.c_ulonglong),
            ('dst_port', ct.c_ulonglong),
            ('mix_hash', ct.c_int32),
        ]

# int stat_update_and_process_decay(__u64 timestamp, int32_t fx_value, struct inc_stat *stat)
stats.stat_update_and_process_decay.restype = ct.c_int
stats.stat_update_and_process_decay.argtypes = [ct.c_ulonglong, ct.c_int, ct.POINTER(inc_stat)]
update_and_process_decay = stats.stat_update_and_process_decay

# int stat_parse_and_hash(void *data_end, void *data, struct hash *ph)
stats.stat_parse_and_hash.restype = ct.c_int
stats.stat_parse_and_hash.argtypes = [ct.c_void_p, ct.c_void_p, ct.POINTER(hash)]
parse_and_hash = stats.stat_parse_and_hash