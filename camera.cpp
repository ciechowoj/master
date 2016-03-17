#include <thread>
#include <tbb/tbb.h>
#include <camera.hpp>
#include <runtime_assert>
#include <GLFW/glfw3.h>

namespace haste {

Ray shoot(
    const Camera& camera, 
    int x, 
    int y, 
    float winv2, 
    float hinv2, 
    float aspect, 
    float znear) 
{
    float fx = ((float(x) + 0.5f) * winv2 * - 1.f) * aspect;
    float fy = (float(y) + 0.5f) * hinv2 - 1.f;

    return Ray {
        (camera.view * vec4(0.f, 0.f, 0.f, 1.f)).xyz(),
        (camera.view * vec4(normalize(vec3(fx, fy, -znear)), 0.f)).xyz()
    };
}

void _render(
    vector<vec4>& imageData,
    const ImageDesc& imageDesc,
    const Camera& camera,
    std::function<vec3(Ray ray)> trace) 
{
    int xBegin = imageDesc.x;
    int xEnd = xBegin + imageDesc.w;

    int rXBegin = xEnd - 1;
    int rXEnd = xBegin - 1;

    int yBegin = imageDesc.y;
    int yEnd = yBegin + imageDesc.h;

    int width = imageDesc.pitch;
    int height = imageData.size() / width;

    runtime_assert(width * height == imageData.size());
    runtime_assert(0 <= xBegin && xEnd <= width);
    runtime_assert(0 <= yBegin && yEnd <= height);
    
    float znear = 1.f / tan(camera.fovy * 0.5f);
    float aspect = float(width) / float(height);
    
    float winv2 = 2.f / float(width);
    float hinv2 = 2.f / float(height);

    for (int y = yBegin; y < yEnd; ++y) {
        for (int x = xBegin; x < xEnd; ++x) {
            Ray ray = shoot(camera, x, y, winv2, hinv2, aspect, znear);
            vec3 old = imageData[y * width + x].xyz();
            float count = imageData[y * width + x].w;
            imageData[y * width + x] = vec4(old + trace(ray), count + 1.f);
        }

        ++y;

        if (y < yEnd) {
            for (int x = rXBegin; x > rXEnd; --x) {
                Ray ray = shoot(camera, x, y, winv2, hinv2, aspect, znear);
                vec3 old = imageData[y * width + x].xyz();
                float count = imageData[y * width + x].w;
                imageData[y * width + x] = vec4(old + trace(ray), count + 1.f);
            }
        }
    }
}

int findLastBlock(
    const vector<vec4>& imageData,
    size_t pitch) {

    int width = pitch;
    int height = imageData.size() / width;

    int a = 0;
    int b = height;

    float x = imageData[width - 1].w;

    



}

void _renderBlock(
    vector<vec4>& imageData,
    size_t pitch,
    int blockIndex,
    int blockSize,
    int rows,
    int cols,
    const Camera& camera,
    const function<vec3(Ray ray)>& trace) 
{
    int width = pitch;
    int height = imageData.size() / pitch;

    ImageDesc imageDesc;
    imageDesc.x = blockIndex % cols * blockSize;
    imageDesc.y = blockIndex / cols * blockSize;
    imageDesc.w = blockSize;
    imageDesc.h = blockSize;
    imageDesc.pitch = pitch;

    _render(
        imageData,
        imageDesc,
        camera,
        trace);
}


void renderInteractive(
    vector<vec4>& imageData,
    size_t pitch,
    const Camera& camera,
    const function<vec3(Ray ray)>& trace)
{
    const float budget = 0.020;
    double start = glfwGetTime();
    const int block = 128;

    const int width = pitch;
    const int height = imageData.size() / pitch;
    const int rows = (height + block - 1) / block;
    const int cols = (width + block - 1) / block;
    const int numBlocks = rows * cols;

    const unsigned cores = std::thread::hardware_concurrency();

    int itr = 0;

    while (glfwGetTime() < start + budget) {
        tbb::task_group group;

        for (unsigned i = 0; i < cores - 1; ++i) {
            group.run([=, &imageData] { _renderBlock(
                imageData, 
                pitch, 
                itr + i, 
                block, 
                rows,
                cols,
                camera, 
                trace); });
        }

        _renderBlock(
            imageData, 
            pitch, 
            itr + cores - 1, 
            block, 
            rows,
            cols,
            camera, 
            trace);

        group.wait();

        itr = (itr + cores) % numBlocks;
    }
}


}
