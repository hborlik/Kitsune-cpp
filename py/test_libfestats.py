import libfestats
import ctypes as ct

if __name__ == "__main__":
    inc_stat = libfestats.inc_stat(ct.c_ulonglong(0), (1, 2, 3, 4, 5), (1, 2, 3, 4, 5), (1, 2, 3, 4, 5), False)
    print(inc_stat.w)
    rv = libfestats.update_and_process_decay(10, 1000, ct.byref(inc_stat))
    print(rv)
    for i in range(len(inc_stat.w)):
        print(inc_stat.w[i])