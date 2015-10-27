#include <stdlib.h>
#include <CL/cl.h>
#include <stdio.h>
#include <math.h>

typedef struct mt_struct_s {
    cl_uint aaa;
    cl_int mm, nn, rr, ww;
    cl_uint wmask, umask, lmask;
    cl_int shift0, shift1, shiftB, shiftC;
    cl_uint maskB, maskC;
} mt_struct;

mt_struct mts[] = { /* Dynamic Creater로 생성한 파라미터 */
    {-822663744,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-1514742400,-2785280},
    {-189104639,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-1263215232,-2785280},
    {-1162865726,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-1242112640,-2686976},
    {-1028775805,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-688432512,-2785280},
    {-1154537980,8,17,23,32,-1,-8388608,8388607,12,18,7,15,1702190464,-2785280},
    {-489740155,8,17,23,32,-1,-8388608,8388607,12,18,7,15,938827392,-34275328},
    {-2145422714,8,17,23,32,-1,-8388608,8388607,12,18,7,15,1722112896,-2818048},
    {-1611140857,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-173220480,-2785280},
    {-1514624952,8,17,23,32,-1,-8388608,8388607,12,18,7,15,1970625920,-2785280},
    {-250871863,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-605200512,-45776896},
    {-504393846,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-1535844992,-2818048},
    {-96493045,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-608739712,-36339712},
    {-291834292,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-151201152,-1736704},
    {-171838771,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-411738496,-36372480},
    {-588451954,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-1494951296,-2785280},
    {-72754417,8,17,23,32,-1,-8388608,8388607,12,18,7,15,1727880064,-688128},
    {-1086664688,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-1230685312,-8552448},
    {-558849839,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-1946722688,-34242560},
    {-1723207278,8,17,23,32,-1,-8388608,8388607,12,18,7,15,2000501632,-1114112},
    {-1779704749,8,17,23,32,-1,-8388608,8388607,12,18,7,15,1004886656,-2785280},
    {-2061554476,8,17,23,32,-1,-8388608,8388607,12,18,7,15,1958042496,-2785280},
    {-1524638763,8,17,23,32,-1,-8388608,8388607,12,18,7,15,653090688,-34177024},
    {-474740330,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-1263215232,-3833856},
    {-1001052777,8,17,23,32,-1,-8388608,8388607,12,18,7,15,921000576,-196608},
    {-1331085928,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-453715328,-2785280},
    {-883314023,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-184755584,-2785280},
    {-692780774,8,17,23,32,-1,-8388608,8388607,12,18,7,15,-638264704,-2785280},
    {-1009388965,8,17,23,32,-1,-8388608,8388607,12,18,7,15,1001217664,-36339712},
    {-690295716,8,17,23,32,-1,-8388608,8388607,12,18,7,15,922049152,-1769472},
    {-1787515619,8,17,23,32,-1,-8388608,8388607,12,18,7,15,1970625920,-688128},
    {-1172830434,8,17,23,32,-1,-8388608,8388607,12,18,7,15,720452480,-2654208},
    {-746112289,8,17,23,32,-1,-8388608,8388607,12,18,7,15,2000509824,-2785280},};

#define MAX_SOURCE_SIZE (0x100000)

int main()
{
    cl_int num_rand = 4096*256; /* 하나의 생성기가 생성할 난수의 개수 */
    int count_all, i;
    cl_uint num_generator = sizeof(mts)/sizeof(mts[0]); /* 생성기 수 */
    unsigned int num_work_item;
    double pi;
    cl_platform_id platform_id = NULL;
    cl_uint ret_num_platforms;
    cl_device_id device_id = NULL;
    cl_uint ret_num_devices;
    cl_context context = NULL;
    cl_command_queue command_queue = NULL;
    cl_program program = NULL;
    cl_kernel kernel_mt = NULL, kernel_pi = NULL;
    size_t kernel_code_size;
    char *kernel_src_str;
    cl_uint *result;
    cl_int ret;
    FILE *fp;
    cl_mem rand, count;
    size_t global_item_size[3], local_item_size[3];
    cl_mem dev_mts;
    cl_event ev_mt_end, ev_pi_end, ev_copy_end;
    cl_ulong prof_start, prof_mt_end, prof_pi_end, prof_copy_end;
    cl_uint num_compute_unit;

    clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
    context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
    result = (cl_uint*)malloc(sizeof(cl_uint)*num_generator);

    command_queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &ret);

    fp = fopen("mt.cl", "r");
    kernel_src_str = (char*)malloc(MAX_SOURCE_SIZE);
    kernel_code_size = fread(kernel_src_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    /* 출력 버퍼를 생성 */
    rand = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uint)*num_rand*num_generator, NULL, &ret);
    count = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uint)*num_generator, NULL, &ret);

    /* 프로그램 빌드 */
    program = clCreateProgramWithSource(context, 1, (const char **)&kernel_src_str, (const size_t *)&kernel_code_size, &ret);
    clBuildProgram(program, 1, &device_id, "", NULL, NULL);
    kernel_mt = clCreateKernel(program, "genrand", &ret);
    kernel_pi = clCreateKernel(program, "calc_pi", &ret);

    /* 입력 파라미터를 생성 */
    dev_mts = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(mts), NULL, &ret);
    clEnqueueWriteBuffer(command_queue, dev_mts, CL_TRUE, 0, sizeof(mts), mts, 0, NULL, NULL);

    /* 커널 파라미터를 설정 */
    clSetKernelArg(kernel_mt, 0, sizeof(cl_mem), (void*)&rand); /* 난수 값(genrand의 출력) */
    clSetKernelArg(kernel_mt, 1, sizeof(cl_mem), (void*)&dev_mts); /* 생성 파라미터(genrand의 입력) */
    clSetKernelArg(kernel_mt, 2, sizeof(num_rand), &num_rand); /* 생성할 난수의 개수 */
    clSetKernelArg(kernel_mt, 3, sizeof(num_generator), &num_generator); /* 생성할 파라미터 수 */

    clSetKernelArg(kernel_pi, 0, sizeof(cl_mem), (void*)&count); /* 원안에 포함되는 점 수(calc_pi의 출력) */
    clSetKernelArg(kernel_pi, 1, sizeof(cl_mem), (void*)&rand);  /* 난수열(calc_pi의 입력) */
    clSetKernelArg(kernel_pi, 2, sizeof(num_rand), &num_rand);   /* 사용된 난수의 개수 */
    clSetKernelArg(kernel_pi, 3, sizeof(num_generator), &num_generator); /* 생성할 파라미터 수 */

    clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &num_compute_unit, NULL);

    if(num_compute_unit > 32)
       num_compute_unit = 32;

    /* 워크 아이템 갯수 (파라메터의 숫자를 병렬수로 나누어 반올림한 값) */
    num_work_item = (num_generator+(num_compute_unit-1)) / num_compute_unit;

    /* 워크 그룹 갯수, 워크 아이템 갯수를 설정 */
    global_item_size[0] = num_work_item * num_compute_unit; global_item_size[1] = 1; global_item_size[2] = 1;
    local_item_size[0] = num_work_item;  local_item_size[1] = 1;  local_item_size[2] = 1;

    /* 난수열을 생성 */
    clEnqueueNDRangeKernel(command_queue, kernel_mt, 1, NULL, global_item_size, local_item_size, 0, NULL, &ev_mt_end);

    /* pi 계산(디바이스 쪽)  */
    clEnqueueNDRangeKernel(command_queue, kernel_pi, 1, NULL, global_item_size, local_item_size, 0, NULL, &ev_pi_end);

    /* 계산결과 얻기 */
    clEnqueueReadBuffer(command_queue, count, CL_TRUE, 0, sizeof(cl_uint)*num_generator, result, 0, NULL, &ev_copy_end);

    /* pi 전체 계산 */
    count_all = 0;
    for(i = 0; i < (int)num_generator; i++) {
        count_all += result[i];
    }

    pi = ((double)count_all)/(num_rand * num_generator) * 4;
    printf("pi = %f\n", pi);

    /* 처리시간 계산을 위한 프로파일링 데이터 취득 */
    clGetEventProfilingInfo(ev_mt_end, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &prof_start, NULL);
    clGetEventProfilingInfo(ev_mt_end, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &prof_mt_end, NULL);
    clGetEventProfilingInfo(ev_pi_end, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &prof_pi_end, NULL);
    clGetEventProfilingInfo(ev_copy_end, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &prof_copy_end, NULL);

    printf("mt: %f[ms]\n"
           "pi: %f[ms]\n"
           "copy: %f[ms]\n",
           (prof_mt_end - prof_start)/(1000000.0),
           (prof_pi_end - prof_mt_end)/(1000000.0),
           (prof_copy_end - prof_pi_end)/(1000000.0));

    clReleaseEvent(ev_mt_end);
    clReleaseEvent(ev_pi_end);
    clReleaseEvent(ev_copy_end);

    clReleaseMemObject(rand);
    clReleaseMemObject(count);
    clReleaseKernel(kernel_mt);
    clReleaseKernel(kernel_pi);
    clReleaseProgram(program);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);
    free(kernel_src_str);
    free(result);
    return 0;
}