#include <iostream>
#include <chrono>
#include <cuda_runtime.h>
#include <curand_kernel.h>

// Constants
constexpr int NUM_PATHS = 1'000'000;
constexpr int NUM_STEPS = 1000;
constexpr float S0 = 100.0f;
constexpr float mu = 0.05f;
constexpr float sigma = 0.2f;
constexpr float T = 1.0f;
constexpr int BLOCK_SIZE = 256;

// CUDA error checking macro
#define CUDA_CHECK(call) \
do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ << ": " \
                  << cudaGetErrorString(err) << std::endl; \
        exit(EXIT_FAILURE); \
    } \
} while (0)

__global__ void gbm_kernel(float* d_paths, float dt, curandState* states) {
    const int path_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (path_idx >= NUM_PATHS) return;

    curandState local_state = states[path_idx];
    float S = S0;
    d_paths[path_idx * (NUM_STEPS + 1)] = S0;

    for (int step = 0; step < NUM_STEPS; step++) {
        float z = curand_normal(&local_state);
        S *= expf((mu - 0.5f * sigma * sigma) * dt + sigma * sqrtf(dt) * z);
        d_paths[path_idx * (NUM_STEPS + 1) + step + 1] = S;
    }
    states[path_idx] = local_state;
}

void run_benchmark() {
    // Allocate device memory
    float* d_paths;
    CUDA_CHECK(cudaMalloc(&d_paths, NUM_PATHS * (NUM_STEPS + 1) * sizeof(float)));

    // Allocate and initialize RNG states
    curandState* d_states;
    CUDA_CHECK(cudaMalloc(&d_states, NUM_PATHS * sizeof(curandState)));

    // Setup RNG
    const int blocks = (NUM_PATHS + BLOCK_SIZE - 1) / BLOCK_SIZE;
    curandSetupKernel<<<blocks, BLOCK_SIZE>>>(d_states, time(nullptr));

    // Run simulation
    const float dt = T / NUM_STEPS;
    auto start = std::chrono::high_resolution_clock::now();

    gbm_kernel<<<blocks, BLOCK_SIZE>>>(d_paths, dt, d_states);
    CUDA_CHECK(cudaDeviceSynchronize());

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // Print results
    std::cout << "Simulated " << NUM_PATHS << " paths with " << NUM_STEPS << " steps each\n";
    std::cout << "Elapsed time: " << elapsed.count() << " seconds\n";
    std::cout << "Paths per second: " << NUM_PATHS / elapsed.count() << "\n";

    // Cleanup
    CUDA_CHECK(cudaFree(d_paths));
    CUDA_CHECK(cudaFree(d_states));
}

__global__ void curandSetupKernel(curandState* states, unsigned long seed) {
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= NUM_PATHS) return;
    curand_init(seed + idx, 0, 0, &states[idx]);
}

int main() {
    std::cout << "CUDA GBM Benchmark\n";
    std::cout << "GPU: " << []() {
        cudaDeviceProp prop;
        CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
        return prop.name;
    }() << "\n";

    run_benchmark();
    return 0;
}