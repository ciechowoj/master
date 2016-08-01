#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>
#include <loader.hpp>
#include <KDTree3D.hpp>


using namespace std;
using namespace glm;
using namespace haste;

struct TestStruct
{
    vec3 _position;
    vec3 matrix[3];
    vec3 padding[16];

    const vec3& position() const {
        return _position;
    }

    vec3& setPosition(const vec3& position) {
        _position = position;
    }

    float& operator[](size_t index)
    {
        return _position[index];
    }

    float operator[](size_t index) const
    {
        return _position[index];
    }
};

struct TestPosition
{
    vec3 _position;

    const vec3& position() const {
        return _position;
    }

    vec3& setPosition(const vec3& position) {
        _position = position;
    }
};

template <class Stream> Stream& operator<<(
    Stream& stream,
    const TestStruct& value)
{
    auto position = value.position();

    return stream << "["
        << position.x << ", "
        << position.y << ", "
        << position.z << "]";
}

template <class Stream> Stream& operator<<(
    Stream& stream,
    const vector<TestStruct>& value)
{
    stream << "[";

    if (value.size() != 0)
        stream << value[0];

    for (size_t i = 1; i < value.size(); ++i)
        stream << ", " << value[i];

    stream << "]";

    return stream;
}

template <class Stream> Stream& operator<<(
    Stream& stream,
    const vector<vec3>& value)
{
    stream << "[";

    if (value.size() != 0)
        stream << value[0];

    for (size_t i = 1; i < value.size(); ++i)
        stream << ", " << value[i];

    stream << "]";

    return stream;
}

template <class T> vector<T> makeUniformTestCase(size_t n, size_t seed, float half) {
    mt19937 generator(seed);
    uniform_real_distribution<float> distribution(-half, half);

    vector<T> result(n);

    for (size_t i = 0; i < n; ++i) {
        vec3 position = vec3(
            distribution(generator),
            distribution(generator),
            distribution(generator));

        result[i].setPosition(position);
    }

    return result;
}

vector<vec3> makeCornellTestCase(size_t n, size_t seed) {
    auto triangles = loadTriangles("models/CornellBoxDiffuse.blend");

    vector<float> areaSums(triangles.size());

    areaSums[0] = triangles[0].area();

    for (size_t i = 1; i < triangles.size(); ++i) {
        areaSums[i] = areaSums[i - 1] + triangles[i].area();
    }

    mt19937 generator(seed);
    uniform_real_distribution<float> distribution(0.f, areaSums.back());

    vector<vec3> result(n);

    for (size_t i = 0; i < n; ++i) {
        auto index = lower_bound(
            areaSums.begin(),
            areaSums.end(),
            distribution(generator)) - areaSums.begin();

        float u = distribution(generator);
        float v = distribution(generator);

        if (u + v > 1) {
            u = 1.f - u;
            v = 1.f - v;
        }

        result[i] = triangles[index].interp(u, v);
    }

    return result;
}

template <class T> float boundingCubeHalf(const vector<T>& cloud) {
    float result = 0;

    for (size_t i = 0; i < cloud.size(); ++i) {
        auto position = cloud[i].position();
        result = std::max<float>(result, abs(position.x));
        result = std::max<float>(result, abs(position.y));
        result = std::max<float>(result, abs(position.z));
    }

    return result;
}

template <class T> vector<vec3> extractPositions(const vector<T>& data) {
    vector<vec3> result(data.size());

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = data[i].position();
    }

    return result;
}

template <class T> vector<T> injectPositions(const vector<vec3>& data) {
    vector<T> result(data.size());

    for (size_t i = 0; i < result.size(); ++i) {
        result[i].setPosition(data[i]);
    }

    return result;
}

vector<vector<vec3>> solveNaive(
    const vector<vec3>& data,
    const vector<vec3>& queries,
    float radius)
{
    vector<vector<vec3>> result(queries.size());

    for (size_t iQuery = 0; iQuery < queries.size(); ++iQuery) {
        for (size_t iCloud = 0; iCloud < data.size(); ++iCloud) {
            if (distance(queries[iQuery], data[iCloud]) < radius) {
                result[iQuery].push_back(data[iCloud]);
            }

            sort(result[iQuery].begin(),
                result[iQuery].end(),
                [&](const vec3& a, const vec3& b) -> bool {
                auto distA = distance(a, queries[iQuery]);
                auto distB = distance(b, queries[iQuery]);

                return distB == distA ? a.x > b.x : distB < distA;
            });
        }

        std::cout << "query: " << iQuery << " (" << result[iQuery].size() << ")" << endl;
    }

    return result;
}

void save(ofstream& stream, const vector<vec3>& data) {
    uint32_t size = static_cast<uint32_t>(data.size());
    stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
    stream.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(data[0]));
}

void save(ofstream& stream, const vector<vector<vec3>>& data) {
    uint32_t size = static_cast<uint32_t>(data.size());
    stream.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (size_t i = 0; i < size; ++i)
        save(stream, data[i]);
}

void saveTestCase(
    ofstream& stream,
    const vector<vec3>& data,
    const vector<vec3>& query,
    const vector<vector<vec3>>& result,
    float radius)
{
    save(stream, data);
    save(stream, query);
    save(stream, result);

    stream.write(
        reinterpret_cast<char*>(&radius),
        sizeof(radius));
}

void load(ifstream& stream, vector<vec3>& data) {
    uint32_t size = 0;
    stream.read(reinterpret_cast<char*>(&size), sizeof(size));
    data.resize(size);
    stream.read(reinterpret_cast<char*>(&data[0]), data.size() * sizeof(data[0]));
}

void load(ifstream& stream, vector<vector<vec3>>& data) {
    uint32_t size = 0;
    stream.read(reinterpret_cast<char*>(&size), sizeof(size));
    data.resize(size);

    for (size_t i = 0; i < size; ++i)
        load(stream, data[i]);
}

void loadTestCase(
    ifstream& stream,
    vector<vec3>& data,
    vector<vec3>& queries,
    vector<vector<vec3>>& result,
    float& radius)
{
    load(stream, data);
    load(stream, queries);
    load(stream, result);

    stream.read(
        reinterpret_cast<char*>(&radius),
        sizeof(radius));
}

void prepareCornellTestCase(
    ofstream& stream,
    size_t numData,
    size_t numQueries,
    float radius)
{
    auto data = makeCornellTestCase(numData, 31415);
    auto queries = makeCornellTestCase(numQueries, 51413);
    auto result = solveNaive(data, queries, radius);

    saveTestCase(stream, data, queries, result, radius);
}

void prepareCornellTestCase(
    string path,
    size_t numData,
    size_t numQueries,
    float radius)
{
    ofstream stream(path, ofstream::binary);
    prepareCornellTestCase(stream, numData, numQueries, radius);
}

template <class T> bool equal(const vector<T>& a, const vector<T>& b)
{
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

void runQueries(
    const KDTree3D<TestStruct>& kdtree,
    const vector<vec3>& queries,
    vector<vector<TestStruct>>& result,
    float radius)
{
    for (size_t i = 0; i < queries.size(); ++i) {
        size_t size = kdtree.query_k(
            result[i].data(),
            queries[i],
            result[i].size(),
            radius);

        result[i].resize(size);
    }
}

void run_test_case(ifstream& stream) {
    vector<vec3> data, queries;
    vector<vector<vec3>> result;
    float radius = 0.0f;
    loadTestCase(stream, data, queries, result, radius);

    cout
        << setw(16) << data.size()
        << setw(16) << queries.size()
        << setw(16) << radius;

    auto testData = injectPositions<TestStruct>(data);
    vector<vector<TestStruct>> testResult(result.size());

    for (size_t i = 0; i < testResult.size(); ++i) {
        testResult[i].resize(result[i].size());
    }

    auto start = chrono::high_resolution_clock::now();

    KDTree3D<TestStruct> kdtree(testData);

    auto build = chrono::high_resolution_clock::now();

    runQueries(kdtree, queries, testResult, radius);

    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> buildTime = build - start;
    chrono::duration<double> queriesTime = end - build;
    chrono::duration<double> totalTime = end - start;

    cout
        << setw(12) << buildTime.count() << "s"
        << setw(12) << queriesTime.count() << "s"
        << setw(12) << totalTime.count() << "s" << endl;

    for (size_t i = 0; i < result.size(); ++i) {
        auto actual = extractPositions(testResult[i]);

        sort(
            actual.begin(),
            actual.end(),
            [&](const vec3& a, const vec3& b) -> bool {
                auto distA = distance(a, queries[i]);
                auto distB = distance(b, queries[i]);

                return distB == distA ? a.x > b.x : distB < distA;
            });

        if (!equal(result[i], actual)) {
            cout << "Test failed:\n";
            cout << "Expected:\n";
            cout << result[i] << "\n";
            cout << "Actual:\n";
            cout << actual << "\n";

            return;
        }
    }

    cout << "SUCCESS" << endl;
}

void run_test_case(string path) {
    ifstream stream(path, ifstream::binary);
    run_test_case(stream);
}

void test_case_header() {
    cout << "      NUM POINTS     NUM QUERIES          RADIUS        BUILD        QUERY        TOTAL" << endl;
}

int main(int argc, char **argv) {

    // prepareCornellTestCase("test_case_2.dat", 100000, 1000, 1.f);

    test_case_header();
    run_test_case("test_case_2.dat");


    return 0;
}
