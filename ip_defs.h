#define LOCAL_TB 1

#ifdef AMAZON_EC2

#define CL_ADDR "52.8.21.243"
#define M1_ADDR "52.6.55.195"
#define M2_ADDR "52.24.102.104"
#define SE_ADDR "52.5.27.99"

#endif

#ifdef LOCAL_TB

#define CL_ADDR "10.0.2.2"
#define M1_ADDR "10.0.2.1"
#define M2_ADDR "10.0.4.1"
#define SE_ADDR "10.0.3.2"

#endif
