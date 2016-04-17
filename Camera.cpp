#include <thread>
#include <tbb/tbb.h>
#include <GLFW/glfw3.h>

#include <runtime_assert>
#include <Camera.hpp>
#include <utility.hpp>

namespace haste {

mat4 Camera::proj(size_t width, size_t height) const {
    return perspective(
        fovy,
        float(width) / float(height),
        0.1f,
        1000.0f);
}

Ray shoot(
    RandomEngine& source,
    const Camera& camera,
    int x,
    int y,
    float winv2,
    float hinv2,
    float aspect,
    float znear)
{
    auto uniform = sampleUniform2(source);
    float fx = ((float(x) + uniform.a()) * winv2 - 1.f) * aspect;
    float fy = (float(y) + uniform.b()) * hinv2 - 1.f;

    return Ray {
        (camera.view * vec4(0.f, 0.f, 0.f, 1.f)).xyz(),
        (camera.view * vec4(normalize(vec3(fx, fy, -znear)), 0.f)).xyz()
    };
}

size_t render(
    RandomEngine& source,
    size_t width,
    size_t height,
    vec4* image,
    const ImageDesc& imageDesc,
    const Camera& camera,
    const std::function<vec3(RandomEngine& source, Ray ray)>& trace)
{
    UniformSampler sampler;

    const int xBegin = max(0, imageDesc.x);
    const int xEnd = min(xBegin + imageDesc.w, int(width));

    const int rXBegin = xEnd - 1;
    const int rXEnd = xBegin - 1;

    const int yBegin = max(0, imageDesc.y);
    const int yEnd = min(yBegin + imageDesc.h, int(height));

    runtime_assert(0 <= xBegin && xEnd <= width);
    runtime_assert(0 <= yBegin && yEnd <= height);

    float znear = 1.f / tan(camera.fovy * 0.5f);
    float aspect = float(width) / float(height);

    float winv2 = 2.f / float(width);
    float hinv2 = 2.f / float(height);

    for (int y = yBegin; y < yEnd; ++y) {
        for (int x = xBegin; x < xEnd; ++x) {
            Ray ray = shoot(source, camera, x, y, winv2, hinv2, aspect, znear);
            vec3 old = image[y * width + x].xyz();
            float count = image[y * width + x].w;
            image[y * width + x] = vec4(old + trace(source, ray), count + 1.f);
        }

        ++y;

        if (y < yEnd) {
            for (int x = rXBegin; x > rXEnd; --x) {
                Ray ray = shoot(source, camera, x, y, winv2, hinv2, aspect, znear);
                vec3 old = image[y * width + x].xyz();
                float count = image[y * width + x].w;
                image[y * width + x] = vec4(old + trace(source, ray), count + 1.f);
            }
        }
    }

    return (xEnd - xBegin) * (yEnd - yBegin);
}

size_t render(
    RandomEngine& source,
    size_t width,
    size_t height,
    vec4* image,
    const Camera& camera,
    const function<vec3(RandomEngine& source, Ray ray)>& trace)
{
    ImageDesc imageDesc;
    imageDesc.x = 0;
    imageDesc.y = 0;
    imageDesc.w = width;
    imageDesc.h = height;
    imageDesc.pitch = width;

    return render(
        source,
        width,
        height,
        image,
        imageDesc,
        camera,
        trace);
}

int findLastBlock(
    size_t width,
    size_t height,
    vec4* image,
    size_t block)
{
    if (image[0].w == image[width * height - 1].w) {
        return 0;
    }
    else {
        const int cols = (width + block - 1) / block;
        const int rows = (height + block - 1) / block;

        int a = 0;
        int b = cols * rows;
        float q = image[width * height - 1].w;

        while (a != b) {
            int h = a + (b - a) / 2;

            int x = h % cols * block;
            int y = h / cols * block;

            if (image[y * width + x].w > q) {
                a = h + 1;
            }
            else {
                b = h;
            }
        }

        return a;
    }
}

double renderInteractive(
    size_t width,
    size_t height,
    vec4* image,
    const Camera& camera,
    const function<vec3(RandomEngine& source, Ray ray)>& trace)
{
    const float budget = 0.010;
    double start = glfwGetTime();
    const int block = 64;

    const int rows = (height + block - 1) / block;
    const int cols = (width + block - 1) / block;
    const int numBlocks = rows * cols;

    const unsigned tasks = std::thread::hardware_concurrency() - 1;

    int itr = findLastBlock(width, height, image, block);

    tbb::atomic<size_t> numBlocksRendered(0);

    auto localRender = [=, &image, &numBlocksRendered](int index) {
        RandomEngine source;

        ImageDesc imageDesc;
        imageDesc.x = index % cols * block;
        imageDesc.y = index / cols * block;
        imageDesc.w = block;
        imageDesc.h = block;
        imageDesc.pitch = width;

        ++numBlocksRendered;

        render(source, width, height, image, imageDesc, camera, trace);
    };

    while (glfwGetTime() < start + budget) {
        // localRender(itr++);

        tbb::task_group group;

        for (unsigned i = 0; i < tasks; ++i) {
            group.run([=] { localRender(itr + i); });
        }

        localRender(itr + tasks);

        group.wait();

        itr = (itr + tasks + 1) % numBlocks;
    }

    return double((itr + numBlocksRendered) % numBlocks) / numBlocks;
}

size_t renderGammaBoard(
    vector<vec4>& imageData,
    size_t pitch) {

    size_t width = pitch;
    size_t height = imageData.size() / pitch;

    float widthInv = 32.0f / float(width - 64);

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            imageData[y * pitch + x] = vec4(glm::pow(vec3(x / 32) * widthInv, vec3(1.0f)), 1.f);
        }
    }

    return width * height;
}

}
