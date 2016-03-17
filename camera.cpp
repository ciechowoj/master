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
    float fx = ((float(x) + 0.5f) * winv2 - 1.f) * aspect;
    float fy = (float(y) + 0.5f) * hinv2 - 1.f;

    return Ray {
        (camera.view * vec4(0.f, 0.f, 0.f, 1.f)).xyz(),
        (camera.view * vec4(normalize(vec3(fx, fy, -znear)), 0.f)).xyz()
    };
}

void render(
    vector<vec4>& imageData,
    const ImageDesc& imageDesc,
    const Camera& camera,
    const std::function<vec3(Ray ray)>& trace)
{
    const int width = imageDesc.pitch;
    const int height = imageData.size() / width;

    const int xBegin = max(0, imageDesc.x);
    const int xEnd = min(xBegin + imageDesc.w, width);

    const int rXBegin = xEnd - 1;
    const int rXEnd = xBegin - 1;

    const int yBegin = max(0, imageDesc.y);
    const int yEnd = min(yBegin + imageDesc.h, height);

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

void render(
    vector<vec4>& imageData,
    size_t pitch,
    const Camera& camera,
    const function<vec3(Ray ray)>& trace)
{
    ImageDesc imageDesc;
    imageDesc.x = 0;
    imageDesc.y = 0;
    imageDesc.w = pitch;
    imageDesc.h = imageData.size() / pitch;
    imageDesc.pitch = pitch;

    render(
        imageData,
        imageDesc,
        camera,
        trace);
}

int findLastBlock(
    const vector<vec4>& imageData,
    size_t pitch,
    size_t block)
{
    if (imageData.front().w == imageData.back().w) {
        return 0;
    }
    else {
        const int width = pitch;
        const int height = imageData.size() / width;
        const int cols = (width + block - 1) / block;
        const int rows = (height + block - 1) / block;

        int a = 0;
        int b = cols * rows;
        float q = imageData.back().a;

        while (a != b) {
            int h = a + (b - a) / 2;

            int x = h % cols * block;
            int y = h / cols * block;

            if (imageData[y * width + x].w > q) {
                a = h + 1;
            }
            else {
                b = h;
            }
        }

        return a;
    }
}

void renderInteractive(
    vector<vec4>& imageData,
    size_t pitch,
    const Camera& camera,
    const function<vec3(Ray ray)>& trace)
{
    const float budget = 0.010;
    double start = glfwGetTime();
    const int block = 128;

    const int width = pitch;
    const int height = imageData.size() / pitch;
    const int rows = (height + block - 1) / block;
    const int cols = (width + block - 1) / block;
    const int numBlocks = rows * cols;

    const unsigned tasks = std::thread::hardware_concurrency() * 4;

    int itr = findLastBlock(imageData, pitch, block);

    auto localRender = [=, &imageData](int index) {
        ImageDesc imageDesc;
        imageDesc.x = index % cols * block;
        imageDesc.y = index / cols * block;
        imageDesc.w = block;
        imageDesc.h = block;
        imageDesc.pitch = pitch;

        render(imageData, imageDesc, camera, trace);
    };

    while (glfwGetTime() < start + budget) {
        tbb::task_group group;

        for (unsigned i = 0; i < tasks; ++i) {
            group.run([=] { localRender(itr + i); });
        }

        localRender(itr + tasks);

        group.wait();

        itr = (itr + tasks + 1) % numBlocks;
    }
}

}
