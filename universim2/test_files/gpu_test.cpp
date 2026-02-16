#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 300
#include <CL/opencl.hpp>
#include <chrono>
#include <iostream>
#include <vector>

const char *kernelSource = R"(
__kernel void vec_add(__global const float* A,
                      __global const float* B,
                      __global float* C)
{
    int id = get_global_id(0);
    C[id] = A[id] + B[id];
}
)";

int main() {
  try {
    // 1️⃣ Get all platforms
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.empty()) {
      std::cerr << "No OpenCL platforms found.\n";
      return 1;
    }

    // 2️⃣ Pick first platform with a GPU
    cl::Platform selectedPlatform;
    cl::Device selectedDevice;

    bool found = false;

    for (auto &platform : platforms) {
      std::vector<cl::Device> devices;
      platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

      if (!devices.empty()) {
        selectedPlatform = platform;
        selectedDevice = devices[0];
        found = true;
        break;
      }
    }

    // Fallback to CPU if no GPU
    if (!found) {
      for (auto &platform : platforms) {
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
        if (!devices.empty()) {
          selectedPlatform = platform;
          selectedDevice = devices[0];
          found = true;
          break;
        }
      }
    }

    if (!found) {
      std::cerr << "No suitable OpenCL device found.\n";
      return 1;
    }

    std::cout << "Using platform: "
              << selectedPlatform.getInfo<CL_PLATFORM_NAME>() << "\n";
    std::cout << "Using device: " << selectedDevice.getInfo<CL_DEVICE_NAME>()
              << "\n";

    // 3️⃣ Create context and queue
    cl::Context context(selectedDevice);
    cl::CommandQueue queue(context, selectedDevice);

    // 4️⃣ Prepare data
    const int N = 400000000;
    std::vector<float> A(N), B(N), C(N);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
      A[i] = i;
      B[i] = i * 2;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Preparing buffers took: " << elapsed.count() << " seconds\n";

    cl::Buffer bufferA(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                       sizeof(float) * N, A.data());
    cl::Buffer bufferB(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                       sizeof(float) * N, B.data());
    cl::Buffer bufferC(context, CL_MEM_WRITE_ONLY, sizeof(float) * N);

    // 5️⃣ Build program
    cl::Program program(context, kernelSource);
    program.build({selectedDevice});

    cl::Kernel kernel(program, "vec_add");
    kernel.setArg(0, bufferA);
    kernel.setArg(1, bufferB);
    kernel.setArg(2, bufferC);

    start = std::chrono::high_resolution_clock::now();
    // 6️⃣ Run kernel
    queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(N),
                               cl::NullRange);
    end = std::chrono::high_resolution_clock::now();
    elapsed = end - start;
    std::cout << "Enqueue NDRangeKernel took: " << elapsed.count()
              << " seconds\n";
    start = std::chrono::high_resolution_clock::now();

    queue.enqueueReadBuffer(bufferC, CL_TRUE, 0, sizeof(float) * N, C.data());
    elapsed = end - start;
    std::cout << "Enqueue Read buffer took: " << elapsed.count()
              << " seconds\n";

    // 7️⃣ Print result
    for (int i = 0; i < N; ++i)
      if (i % (N / 10) == 0) {
        std::cout << A[i] << " + " << B[i] << " = " << C[i] << "\n";
      }

  } catch (cl::Error &err) {
    std::cerr << "OpenCL error: " << err.what() << " (" << err.err() << ")\n";
    return 1;
  }

  return 0;
}
