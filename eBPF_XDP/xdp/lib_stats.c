#include <xdp_stats_kern_user.h>


int stat_update_and_process_decay(__u64 timestamp, int32_t fx_value, struct inc_stat *stat) {
    return update_and_process_decay(timestamp, fx_value, stat);
}

int stat_parse_and_hash(void *data_end, void *data, struct hash *ph) {
    return parse_and_hash(data_end, data, ph);
}