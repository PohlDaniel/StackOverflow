


#ifdef SSSS_N_SAMPLES
/**
 * Filter kernel layout is as follows:
 *   - Weights in the RGB channels.
 *   - Offsets in the A channel.
 */
float4 kernel[SSSS_N_SAMPLES]; 
#else
/**
 * Here you have ready-to-use kernels for quickstarters. Three kernels are 
 * readily available, with varying quality.
 * To create new kernels take a look into SSS::calculateKernel, or simply
 * push CTRL+C in the demo to copy the customized kernel into the clipboard.
 *
 * Note: these preset kernels are not used by the demo. They are calculated on
 * the fly depending on the selected values in the interface, by directly using
 * SSS::calculateKernel.
 *
 * Quality ranges from 0 to 2, being 2 the highest quality available.
 * The quality is with respect to 1080p; for 720p Quality=0 suffices.
 */
#define SSSS_QUALITY 1

#if SSSS_QUALITY == 2
#define SSSS_N_SAMPLES 25
float4 kernel[] = {
    float4(0.530605, 0.613514, 0.739601, 0),
    float4(0.000973794, 1.11862e-005, 9.43437e-007, -3),
    float4(0.00333804, 7.85443e-005, 1.2945e-005, -2.52083),
    float4(0.00500364, 0.00020094, 5.28848e-005, -2.08333),
    float4(0.00700976, 0.00049366, 0.000151938, -1.6875),
    float4(0.0094389, 0.00139119, 0.000416598, -1.33333),
    float4(0.0128496, 0.00356329, 0.00132016, -1.02083),
    float4(0.017924, 0.00711691, 0.00347194, -0.75),
    float4(0.0263642, 0.0119715, 0.00684598, -0.520833),
    float4(0.0410172, 0.0199899, 0.0118481, -0.333333),
    float4(0.0493588, 0.0367726, 0.0219485, -0.1875),
    float4(0.0402784, 0.0657244, 0.04631, -0.0833333),
    float4(0.0211412, 0.0459286, 0.0378196, -0.0208333),
    float4(0.0211412, 0.0459286, 0.0378196, 0.0208333),
    float4(0.0402784, 0.0657244, 0.04631, 0.0833333),
    float4(0.0493588, 0.0367726, 0.0219485, 0.1875),
    float4(0.0410172, 0.0199899, 0.0118481, 0.333333),
    float4(0.0263642, 0.0119715, 0.00684598, 0.520833),
    float4(0.017924, 0.00711691, 0.00347194, 0.75),
    float4(0.0128496, 0.00356329, 0.00132016, 1.02083),
    float4(0.0094389, 0.00139119, 0.000416598, 1.33333),
    float4(0.00700976, 0.00049366, 0.000151938, 1.6875),
    float4(0.00500364, 0.00020094, 5.28848e-005, 2.08333),
    float4(0.00333804, 7.85443e-005, 1.2945e-005, 2.52083),
    float4(0.000973794, 1.11862e-005, 9.43437e-007, 3),
};
#elif SSSS_QUALITY == 1
#define SSSS_N_SAMPLES 17
float4 kernel[] = {
    float4(0.536343, 0.624624, 0.748867, 0),
    float4(0.00317394, 0.000134823, 3.77269e-005, -2),
    float4(0.0100386, 0.000914679, 0.000275702, -1.53125),
    float4(0.0144609, 0.00317269, 0.00106399, -1.125),
    float4(0.0216301, 0.00794618, 0.00376991, -0.78125),
    float4(0.0347317, 0.0151085, 0.00871983, -0.5),
    float4(0.0571056, 0.0287432, 0.0172844, -0.28125),
    float4(0.0582416, 0.0659959, 0.0411329, -0.125),
    float4(0.0324462, 0.0656718, 0.0532821, -0.03125),
    float4(0.0324462, 0.0656718, 0.0532821, 0.03125),
    float4(0.0582416, 0.0659959, 0.0411329, 0.125),
    float4(0.0571056, 0.0287432, 0.0172844, 0.28125),
    float4(0.0347317, 0.0151085, 0.00871983, 0.5),
    float4(0.0216301, 0.00794618, 0.00376991, 0.78125),
    float4(0.0144609, 0.00317269, 0.00106399, 1.125),
    float4(0.0100386, 0.000914679, 0.000275702, 1.53125),
    float4(0.00317394, 0.000134823, 3.77269e-005, 2),
};
#elif SSSS_QUALITY == 0
#define SSSS_N_SAMPLES 11
float4 kernel[] = {
    float4(0.560479, 0.669086, 0.784728, 0),
    float4(0.00471691, 0.000184771, 5.07566e-005, -2),
    float4(0.0192831, 0.00282018, 0.00084214, -1.28),
    float4(0.03639, 0.0130999, 0.00643685, -0.72),
    float4(0.0821904, 0.0358608, 0.0209261, -0.32),
    float4(0.0771802, 0.113491, 0.0793803, -0.08),
    float4(0.0771802, 0.113491, 0.0793803, 0.08),
    float4(0.0821904, 0.0358608, 0.0209261, 0.32),
    float4(0.03639, 0.0130999, 0.00643685, 0.72),
    float4(0.0192831, 0.00282018, 0.00084214, 1.28),
    float4(0.00471691, 0.000184771, 5.07565e-005, 2),
};
#else
#error Quality must be one of {0,1,2}
#endif
#endif







