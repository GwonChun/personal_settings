#include <stdlib.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <stdio.h>

#define MAX_SOURCE_SIZE (0x100000)

int main()
{
	cl_platform_id platform_id = NULL;
	cl_uint ret_num_platforms;
	cl_device_id device_id = NULL;
	cl_uint ret_num_devices;
	cl_context context = NULL;
	cl_command_queue command_queue = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	size_t kernel_code_size;
	char *kernel_src_str;
	float *result;
	cl_int ret;
	int i;
	FILE *fp;
	size_t r_size;

	cl_mem image, out;
	cl_bool support;
	cl_image_format fmt;

	int num_out = 9;

	clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
	context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);
	result = (float*)malloc(sizeof(cl_float4)*num_out);

	/* 디바이스가 이미지를 지원하는지 확인 */
	clGetDeviceInfo(device_id, CL_DEVICE_IMAGE_SUPPORT, sizeof(support), &support, &r_size);
	if(support != CL_TRUE) {
	   puts("image not supported");
	return 1;
	}

	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
	fp = fopen("image.cl", "r");
	kernel_src_str = (char*)malloc(MAX_SOURCE_SIZE);
	kernel_code_size = fread(kernel_src_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);

	/* 출력 버퍼를 생성 */
	out = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_float4)*num_out, NULL, &ret);

	/* 이미지 오브젝트에 포함될 데이터 포맷을 정의 */
	fmt.image_channel_order = CL_R;
	fmt.image_channel_data_type = CL_FLOAT;

	/* 이미지 오브젝트 생성 */
	image = clCreateImage2D(context, CL_MEM_READ_ONLY, &fmt, 4, 4, 0, 0, NULL);

	/* 이미지 오브젝트를 전송하기 위한 파라미터를 설정 */
	size_t origin[] = {0, 0, 0}; /* 전송될 위치의 좌표 */
	size_t region[] = {4, 4, 1}; /* 전송될 오브젝트의 크기 */

	float data[] = { /* 전송 데이터 */
		10, 20, 30, 40,
		10, 20, 30, 40,
		10, 20, 30, 40,
		10, 20, 30, 40,
	};

	/* 이미지 오브젝트를 디바이스로 전송 */
	clEnqueueWriteImage(command_queue, image, CL_TRUE, origin, region, 4*sizeof(float), 0, data, 0, NULL, NULL);

	/* 커널 프로그램 빌드 */
	program = clCreateProgramWithSource(context, 1, (const char **)&kernel_src_str, (const size_t *)&kernel_code_size, &ret);
	clBuildProgram(program, 1, &device_id, "", NULL, NULL);
	kernel = clCreateKernel(program, "image_test", &ret);

	/* 커널 파라미터를 설정 */
	clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&image);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&out);

	cl_event ev;
	clEnqueueTask(command_queue, kernel, 0, NULL, &ev);

	/* 커널 실행 결과를 얻음 */
	clEnqueueReadBuffer(command_queue, out, CL_TRUE, 0, sizeof(cl_float4)*num_out, result, 0, NULL, NULL);

	for(i = 0; i < num_out; i++) {
		printf("%f, %f, %f, %f\n", result[i*4+0], result[i*4+1], result[i*4+2], result[i*4+3]);
	}

	clReleaseMemObject(out);
	clReleaseMemObject(image);

	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);

	free(kernel_src_str);
	free(result);

	return 0;
}