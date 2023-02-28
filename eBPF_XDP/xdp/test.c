#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <fixed_point.h>


int main() {

    double u = 1.1234567;
    double v = 0.1234567;
    double w = 0.1234567;

    int32_t fp_u = double_to_s15p16(u);
    int32_t fp_v = double_to_s15p16(v);

    printf("U: %f\n", u);
    printf("V: %f\n", v);

    printf("U + V (%f) : %f\n", u + v, s15p16_to_double(fp_u + fp_v));
    printf("U - V (%f) : %f\n", u - v, s15p16_to_double(fp_u - fp_v));
    printf("U * V (%f) : %f\n", u * v, s15p16_to_double(fxmul_s15p16(fp_u, fp_v)));
    printf("U / V (%f) : %f\n", u / v, s15p16_to_double(fxdiv_s15p16(fp_u, fp_v)));

    printf("U ^ V (%f) : %f\n", pow(u, v), s15p16_to_double(fxpow_s15p16(fp_u, fp_v)));

    printf("W: %f\n", w);


    int32_t fx_ns = int_to_s15p16(1000000000);
    printf("Large values do not work!\n");
    printf("1000000000: raw %i, converted back %f\n", fx_ns, s15p16_to_double(fx_ns));
}