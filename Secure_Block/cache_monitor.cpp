#include <cache_monitor.h>

ac_int<32, false> timer = 0;
ac_int<8, false> secret_counter = 0;
ac_int<32,false> prev_miss_time = 0;
ac_int <32,false> curr_miss_time = 0;
ac_int <1,false> reset_timer = 0;

void cache_monitor(ac_int <32,false> req_addr, ac_int <32,false> sec_addr, 
	bool &detection_flag, ac_int <8,false> threshold, 
	ac_int <32,false> sample_time, bool cacheMiss)
{
    if(cacheMiss) {
        if (req_addr == sec_addr) {
            secret_counter ++;
            prev_miss_time = curr_miss_time;
            curr_miss_time = timer;
        }
    }
    timer++;
    if (timer > sample_time) {
        secret_counter =0;
        timer =0;
    }

    if(secret_counter>=threshold) {
        detection_flag = 1;
    }
    else {
        detection_flag = 0;
    }
}
