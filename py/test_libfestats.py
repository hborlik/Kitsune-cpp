import libfestats
import ctypes as ct

if __name__ == "__main__":
    five_int = ct.c_int32 * libfestats.N_INC_STATS
    CF1 = five_int(*[libfestats.ftos15p16(x) for x in (10, 10, 10, 10, 10)])
    CF2 = five_int(*[libfestats.ftos15p16(x) for x in (1, 2, 3, 4, 5)])
    w = five_int(*[x for x in (1, 1, 1, 1, 1)])
    for i in CF1: print(i, end=" ")
    print("")

    inc_stat = libfestats.inc_stat(ct.c_ulonglong(0), CF1, CF2, w, False)
    print(inc_stat.w)
    for i in range(40):
        rv = libfestats.update_and_process_decay(1000000*i, libfestats.ftos15p16(3), ct.byref(inc_stat))

        # print CF1 and timestamp
        print("time: {0} ".format(inc_stat.last_t), end=" ")
        for ii in inc_stat.CF1:
            print("{:.7f}".format(libfestats.s15p16tof(ii)), end=" ")
        print("")