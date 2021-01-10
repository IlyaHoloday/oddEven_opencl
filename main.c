#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef __APPLE__
# include <OpenCL/opencl.h>      									 // для компьютеров на MacOsX
#else
#include <CL/cl.h>              									 // для компьютеров на Win\Linux указывайте путь к файлу cl.h
#endif
#define MAX_SRC_SIZE (0x10000)                                      // максимальный размер исходного кода кернеля

int 						main(void)
{
	int         			i;
	const int   			arr_size = 10000;                         // размер наших массивов
	int         			*A;
	int      				fd;
	char     				*src_str;                            	 // сюда будет записан исходный код кернеля
	size_t   				src_size;
	char					vendor[1024];
    char					device_name[1024];
	cl_uint					num_cores;

	cl_platform_id        	platform_id = NULL;    					 // обратите внимание на типы данных
	cl_device_id          	device_id = NULL;
	cl_uint              	ret_num_devices;
	cl_uint              	ret_num_platforms;
	cl_int                	ret;                   					 // сюда будут записываться сообщения об ошибках

	cl_context              context;
	cl_command_queue        command_queue;

	cl_mem         			a_mem_obj;           					 

	cl_program        		program;       							 // сюда будет записанна наша программа
	cl_kernel         		kernel;        							 // сюда будет записан наш кернель
	cl_kernel         		kernel_2;

	size_t          		NDRange;       							 // здесь мы указываем размер вычислительной сетки
	size_t           		work_size;     							 // размер work-group




	A = (int *)malloc(sizeof(int) * arr_size);						// выделяем место под массивы А и В
	for(i = 0; i < arr_size; i++)                      				// наполняем массивы данными для отправки в кернель OpenCL
	{
		A[i] = 10000 - i;
	}

	clock_t begin = clock();
	src_str = (char *)malloc(MAX_SRC_SIZE);       					// выделяем память для исходного кода
	fd = open("OddEven.cl", "r");
	if (fd <= 0)
	{
		fprintf(stderr, "Empty File :(\n");
		exit(1);
	}
	src_size = read(fd, src_str, MAX_SRC_SIZE);       				// записываем исходный код в src_str
	close(fd);


	ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_CPU, 1, &device_id, &ret_num_devices);
	clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, sizeof(vendor), vendor, NULL);
	clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
	clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(num_cores), &num_cores, NULL);

	//printf("-------------------------------------------------\n");
	//printf("Vendor: %s\n", vendor);
	//printf("Name: %s\n", device_name);
	//printf("-------------------------------------------------\n");


	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
	command_queue = clCreateCommandQueue(context, device_id, 0, &ret);


	a_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
							   arr_size * sizeof(cl_int), NULL, &ret);


	ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,    			// записываем массив А
							   arr_size * sizeof(int), A, 0, NULL, NULL);


	program = clCreateProgramWithSource(context, 1,
										(const char **)&src_str, (const size_t *)&src_size, &ret);   // создаём программу из исходного кода
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);                					 // собираем программу (онлайн компиляция)
	kernel = clCreateKernel(program, "odd", &ret);                            					 // создаём кернель
	kernel_2 = clCreateKernel(program, "even", &ret);

	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);  			// объект А
	ret = clSetKernelArg(kernel_2, 0, sizeof(cl_mem), (void*)&a_mem_obj);//  			// объект В


	NDRange = arr_size;
	work_size = 256;                 												// NDRange должен быть кратен размеру work-group
	for (int i = 0; i < arr_size/2; i++)
	{
		ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
			&NDRange, &work_size, 0, NULL, NULL);

		ret = clEnqueueNDRangeKernel(command_queue, kernel_2, 1, NULL,
			&NDRange, &work_size, 0, NULL, NULL);
	}
	



	ret = clEnqueueReadBuffer(command_queue, a_mem_obj, CL_TRUE, 0,  				// записываем ответы
		arr_size * sizeof(int), A, 0, NULL, NULL);


	//for(i = 0; i < arr_size; i++)
	//	printf("%d\n", A[i]);


	ret = clFlush(command_queue);                   // отчищаем очередь команд
	ret = clFinish(command_queue);                  // завершаем выполнение всех команд в очереди
	ret = clReleaseKernel(kernel);                  // удаляем кернель
	ret = clReleaseProgram(program);                // удаляем программу OpenCL
	ret = clReleaseMemObject(a_mem_obj);            // отчищаем OpenCL буфер А
	ret = clReleaseCommandQueue(command_queue);     // удаляем очередь команд
	ret = clReleaseContext(context);                // удаляем контекст OpenCL
	free(A);                                        // удаляем локальный буфер А
	clock_t end = clock();
	printf("Elapsed: %f seconds\n", (double)(end - begin) / CLOCKS_PER_SEC);
	
	return(0);
}
