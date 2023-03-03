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
    printf("U * V (%f) : %f\n", u * v, s15p16_to_double(mul_s15p16(fp_u, fp_v)));
    printf("U / V (%f) : %f\n", u / v, s15p16_to_double(div_s15p16(fp_u, fp_v)));

    printf("U ^ V (%f) : %f\n", pow(u, v), s15p16_to_double(pow_s15p16(fp_u, fp_v)));
    printf("log2_s15p16 V (%f) : %f\n", log2(v), s15p16_to_double(log2_s15p16(fp_v)));
    printf("fixed_log2 V (%f) : %f\n", log2(v), s15p16_to_double(fixed_log2(fp_v)));

    float a = 2;
    int32_t fp_a = double_to_s15p16(a);
    for (int i = 0; i < 100; i++) {
        float actual = pow(a, i / 100.0);
        int32_t fp_i = double_to_s15p16(i / 100.0);
        float estimated = s15p16_to_double(fixed_pow_s15p16(fp_a, fp_i));
        printf("fixed_pow %f ** %f (%f) : %f, error: %f\n", a, (float)i / 100.0, actual, estimated, actual-estimated);
    }

    printf("W: %f\n", w);


    int32_t fx_ns = int_to_s15p16(1000000000);
    printf("Large values do not work!\n");
    printf("1000000000: raw %i, converted back %f\n", fx_ns, s15p16_to_double(fx_ns));
}