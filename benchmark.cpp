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
#include <HashGrid3D.hpp>
#include <streamops.hpp>

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

    void setPosition(const vec3& position) {
        _position = position;
    }

    bool is_light() const {
        return false;
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

    void setPosition(const vec3& position) {
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

vector<vec3> makeModelTestCase(size_t n, size_t seed, string model) {
    auto triangles = loadTriangles(model);

    vector<float> areaSums(triangles.size());

    areaSums[0] = triangles[0].area();

    for (size_t i = 1; i < triangles.size(); ++i) {
        areaSums[i] = areaSums[i - 1] + triangles[i].area();
    }

    mt19937 generator(seed);
    uniform_real_distribution<float> distribution(0.f, areaSums.back());

    uniform_real_distribution<float> uniform(0.f, 1.f);

    vector<vec3> result(n);

    for (size_t i = 0; i < n; ++i) {
        auto index = lower_bound(
            areaSums.begin(),
            areaSums.end(),
            distribution(generator)) - areaSums.begin();

        float u = uniform(generator);
        float v = uniform(generator);

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

template <class T> vector<vec3> extractPositions(const vector<T>& data, size_t size) {
    vector<vec3> result(data.size());

    for (size_t i = 0; i < size; ++i) {
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

vector<vector<vec3>> solveKDTree(
    const vector<vec3>& data,
    const vector<vec3>& queries,
    float radius)
{
    auto testData = injectPositions<TestStruct>(data);

    v3::HashGrid3D<TestStruct> kdtree(&testData, radius);
    vector<vector<vec3>> result(queries.size());
    vector<TestStruct> testResult(100000);

    for (size_t iQuery = 0; iQuery < queries.size(); ++iQuery) {
        size_t size = kdtree.rQuery(&testResult[0], queries[iQuery], radius);

        result[iQuery] = extractPositions(testResult, size);
        result[iQuery].resize(size);
        result[iQuery].shrink_to_fit();

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

vector<vec3> wiggle(const vector<vec3>& photons, size_t seed, size_t num_queries, float radius) {
    vector<vec3> result;

    mt19937 generator(seed);
    uniform_real_distribution<float> fdistr(-1.5f * radius, 1.5f * radius);
    uniform_int_distribution<size_t> idistr(0, photons.size() - 1);

    for (size_t i = 0; i < num_queries; ++i) {
        result.push_back(photons[idistr(generator)] + vec3(
            fdistr(generator),
            fdistr(generator),
            fdistr(generator)));
    }

    return result;
}

void prepareModelTestCase(
    ofstream& stream,
    size_t numData,
    size_t numQueries,
    float radius,
    string model)
{
    auto data = makeModelTestCase(numData, 31415, model);
    auto queries = makeModelTestCase(numQueries, 51413, model);
    // auto queries = wiggle(data, 51413, numQueries, radius);
    // auto result = solveNaive(data, queries, radius);
    auto result = solveKDTree(data, queries, radius);

    saveTestCase(stream, data, queries, result, radius);
}

void prepareModelTestCase(
    string path,
    size_t numData,
    size_t numQueries,
    float radius,
    string model)
{
    ofstream stream(path, ofstream::binary);
    prepareModelTestCase(stream, numData, numQueries, radius, model);
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

template <class T> void runQueries(
    const T& points,
    const vector<vec3>& queries,
    vector<vector<TestStruct>>& result,
    float radius)
{
    for (size_t i = 0; i < queries.size(); ++i) {
        size_t size = points.rQuery(
            result[i].data(),
            queries[i],
            radius);

        result[i].resize(size);
    }
}

template <template <class> class T> void run_test_case(string name, ifstream& stream) {
    vector<vec3> data, queries;
    vector<vector<vec3>> result;
    float radius = 0.0f;
    loadTestCase(stream, data, queries, result, radius);

    auto testData = injectPositions<TestStruct>(data);
    vector<vector<TestStruct>> testResult(result.size());

    for (size_t i = 0; i < testResult.size(); ++i) {
        testResult[i].resize(result[i].size() * 2);
    }

    auto start = chrono::high_resolution_clock::now();

    T<TestStruct> accel_struct(&testData, radius);

    // v2::KDTree3D<TestStruct> kdtree(testData);
    // v3::HashGrid3D<TestStruct> grid(testData, radius);

    auto build = chrono::high_resolution_clock::now();

    // BENCHMARKED CALL
    runQueries(accel_struct, queries, testResult, radius);

    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double> buildTime = build - start;
    chrono::duration<double> queriesTime = end - build;
    chrono::duration<double> totalTime = end - start;

    cout
        << setw(14) << name
        << setw(16) << data.size()
        << setw(16) << queries.size()
        << setw(16) << radius;

    cout
        << setw(12) << setprecision(4) << fixed << buildTime.count() << "s"
        << setw(12) << setprecision(4) << fixed << queriesTime.count() / queries.size() * 1000 << "ms"
        << setw(12) << setprecision(4) << fixed << totalTime.count() << "s" << endl;

    /*for (size_t i = 0; i < result.size(); ++i) {
        auto actual = extractPositions(testResult[i]);

        sort(
            actual.begin(),
            actual.end(),
            [&](const vec3& a, const vec3& b) -> bool {
                return a.z != b.z ? a.z < b.z : (a.y != b.y ? a.y < b.y : a.x < b.x);
            });

        sort(
            result[i].begin(),
            result[i].end(),
            [&](const vec3& a, const vec3& b) -> bool {
                return a.z != b.z ? a.z < b.z : (a.y != b.y ? a.y < b.y : a.x < b.x);
            });

        if (!equal(result[i], actual)) {
            cout << "Test failed (query = " << i << "):\n";
            cout << "Expected:\n";
            cout << result[i] << "\n";
            cout << "Actual:\n";
            cout << actual << "\n";

            return;
        }
    }*/

    // cout << "SUCCESS" << endl;
}

template <template <class> class T> void run_test_case(string path) {
    ifstream stream(path, ifstream::binary);
    run_test_case<T>("<current>", stream);
}

void run_comparison(string path) {
    cout << path << "\n";
    // ifstream stream0(path, ifstream::binary);
    // run_test_case<v1::KDTree3D>("v1::KDTree3D", stream0);
    // ifstream stream1(path, ifstream::binary);
    // run_test_case<v2::KDTree3D>("v2::KDTree3D", stream1);
    ifstream stream2(path, ifstream::binary);
    run_test_case<v2::HashGrid3D>("v2::HashGrid3D", stream2);
    ifstream stream3(path, ifstream::binary);
    run_test_case<v3::HashGrid3D>("v3::HashGrid3D", stream3);
}

void test_case_header() {
    cout << "          NAME      NUM POINTS     NUM QUERIES          RADIUS        BUILD    QUERY            TOTAL" << endl;
}

int main(int argc, char **argv) {

    // prepareModelTestCase("test_case_A.dat", 1, 2000, 0.1f, "models/TestCase9.blend");
    // prepareModelTestCase("test_case_B.dat", 2, 2000, 0.1f, "models/TestCase9.blend");
    // prepareModelTestCase("test_case_C.dat", 3, 2000, 0.1f, "models/TestCase9.blend");
    // prepareModelTestCase("test_case_D.dat", 4, 2000, 0.1f, "models/TestCase9.blend");
    // prepareModelTestCase("test_case_E.dat", 5, 2000, 0.1f, "models/TestCase9.blend");
    // prepareModelTestCase("test_case_F.dat", 500000, 2000, 1.0f, "models/TestCase9.blend");
    // prepareModelTestCase("test_case_G.dat", 500000, 2000, 2.0f, "models/TestCase9.blend");
    // prepareModelTestCase("test_case_3.dat", 500000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");

    // prepareModelTestCase("Bearings.dat", 500000, 2000, 0.02f, "models/Bearings.blend");

    // prepareModelTestCase("test_data/bearings1M0_01.case", 1000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings1M0_1.case", 1000000, 2000, 0.1f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/cornell1M0_01.case", 1000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell1M0_1.case", 1000000, 2000, 0.1f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/sponza1M0_01.case", 1000000, 2000, 0.01f, "models/DabrovicSponza.blend");
    // prepareModelTestCase("test_data/sponza1M0_1.case", 1000000, 2000, 0.1f, "models/DabrovicSponza.blend");

    // prepareModelTestCase("test_data/cornell100k0_1.case", 100000, 2000, 0.1f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell200k0_1.case", 200000, 2000, 0.1f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell300k0_1.case", 300000, 2000, 0.1f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell400k0_1.case", 400000, 2000, 0.1f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell500k0_1.case", 500000, 2000, 0.1f, "models/CornellBoxDiffuse.blend");

    // prepareModelTestCase("test_data/bearings1M0_01.case", 1000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings2M0_01.case", 2000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings3M0_01.case", 3000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings4M0_01.case", 4000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings5M0_01.case", 5000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings6M0_01.case", 6000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings7M0_01.case", 7000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings8M0_01.case", 8000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings9M0_01.case", 9000000, 2000, 0.01f, "models/Bearings.blend");
    // prepareModelTestCase("test_data/bearings10M0_01.case", 10000000, 2000, 0.01f, "models/Bearings.blend");

    // prepareModelTestCase("test_data/cornell1M0_01.case", 1000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell2M0_01.case", 2000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell3M0_01.case", 3000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell4M0_01.case", 4000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell5M0_01.case", 5000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell6M0_01.case", 6000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell7M0_01.case", 7000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell8M0_01.case", 8000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");
    // prepareModelTestCase("test_data/cornell9M0_01.case", 9000000, 2000, 0.01f, "models/CornellBoxDiffuse.blend");

    test_case_header();

    //run_comparison("test_data/bearings1M0_01.case");
    //run_comparison("test_data/cornell1M0_01.case");

    run_comparison("test_data/bearings1M0_1.case");
    run_comparison("test_data/cornell1M0_1.case");
    run_comparison("test_data/sponza1M0_01.case");
    run_comparison("test_data/sponza1M0_1.case");

    run_comparison("test_data/cornell100k0_1.case");
    run_comparison("test_data/cornell200k0_1.case");
    run_comparison("test_data/cornell300k0_1.case");
    run_comparison("test_data/cornell400k0_1.case");
    run_comparison("test_data/cornell500k0_1.case");

    run_comparison("test_data/cornell1M0_01.case");
    run_comparison("test_data/cornell2M0_01.case");
    run_comparison("test_data/cornell3M0_01.case");
    run_comparison("test_data/cornell4M0_01.case");
    run_comparison("test_data/cornell5M0_01.case");
    run_comparison("test_data/cornell6M0_01.case");
    run_comparison("test_data/cornell7M0_01.case");
    run_comparison("test_data/cornell8M0_01.case");
    run_comparison("test_data/cornell9M0_01.case");
    run_comparison("test_data/cornell10M0_01.case");

    run_comparison("test_data/bearings1M0_01.case");
    run_comparison("test_data/bearings2M0_01.case");
    run_comparison("test_data/bearings3M0_01.case");
    run_comparison("test_data/bearings4M0_01.case");
    run_comparison("test_data/bearings5M0_01.case");
    run_comparison("test_data/bearings6M0_01.case");
    run_comparison("test_data/bearings7M0_01.case");
    run_comparison("test_data/bearings8M0_01.case");
    run_comparison("test_data/bearings9M0_01.case");
    run_comparison("test_data/bearings10M0_01.case");

    return 0;
}


// v1::KDTree3D           7000000            2000          0.0100     19.7090s      0.3613ms     20.4316s
// v2::KDTree3D           7000000            2000          0.0100     15.2202s      0.0979ms     15.4159s
// v2::HashGrid3D         7000000            2000          0.0100      4.2575s      0.0772ms      4.4119s
// v3::HashGrid3D         7000000            2000          0.0100      5.4795s      0.0776ms      5.6346s
