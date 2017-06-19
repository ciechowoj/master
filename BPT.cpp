#include <BPT.hpp>

namespace haste {

template <class Beta>
BPTBase<Beta>::BPTBase(const shared<const Scene>& scene, float lights, float roulette, float beta, size_t num_threads)
    : Technique(scene, num_threads)
    , _roulette(roulette)
    , _roulette_inv(1.0f / roulette)
    , _lights(lights) {
}

template <class Beta>
vec3 BPTBase<Beta>::_traceEye(render_context_t& context, Ray ray) {
    light_path_t light_path;
    vec3 radiance = vec3(0.0f);

    if (_russian_roulette(*context.generator)) {
        return radiance;
    }

    _traceLight(*context.generator, light_path);

    EyeVertex eye[2];
    size_t itr = 0, prv = 1;

    SurfacePoint surface = _camera_surface(context);
    eye[prv].surface = surface;
    eye[prv].omega = -ray.direction;
    eye[prv].throughput = vec3(1.0f) * _roulette_inv;
    eye[prv].finite = 1;
    eye[prv].c = 0;
    eye[prv].C = 0;

    while (true) {
        if (eye[prv].surface.is_camera()) {
            radiance += _connect_eye(context, eye[prv], light_path);
        }
        else {
            radiance += _connect(context, eye[prv], light_path);
        }

        auto bsdf = _scene->sampleBSDF(*context.generator, eye[prv].surface, eye[prv].omega);

        while (true) {
            surface = _scene->intersect(surface, bsdf.omega);

            if (!surface.is_present()) {
                return radiance;
            }

            eye[itr].surface = surface;
            eye[itr].omega = -bsdf.omega;

            auto edge = Edge(eye[prv].surface, eye[itr].surface, eye[itr].omega);

            eye[itr].throughput
                = eye[prv].throughput
                * bsdf.throughput
                * edge.bCosTheta;

            if (l1Norm(eye[itr].throughput) < FLT_EPSILON) {
              return radiance;
            }

            eye[itr].throughput /= bsdf.density;

            eye[prv].finite = min(eye[prv].finite, bsdf.finite);
            eye[itr].finite = bsdf.finite;
            eye[itr].c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

            eye[itr].C
                = (eye[prv].C
                    * Beta::beta(bsdf.densityRev)
                    + eye[prv].c * eye[prv].finite)
                * Beta::beta(edge.bGeometry)
                * eye[itr].c;

            if (surface.is_light()) {
                radiance += _connect_light(eye[itr]);
            }
            else {
                break;
            }
        }

        std::swap(itr, prv);

        if (_russian_roulette(*context.generator)) {
            return radiance;
        }

        eye[prv].throughput *= _roulette_inv;
    }

    return radiance;
}

template <class Beta>
typename BPTBase<Beta>::LightVertex BPTBase<Beta>::_sample_to_vertex(const LightSample& sample) {
    LightVertex vertex;
    vertex.surface = sample.surface;
    vertex.omega = vertex.surface.normal();
    vertex.throughput = sample.radiance() / sample.combined_density() * _roulette_inv;
    vertex.a = sample.kind == light_kind::directional ? 0.0f : 1.0f / Beta::beta(sample.combined_density());
    vertex.A = 0.0f;
    vertex.finite = 1;

    return vertex;
}

template <class Beta>
typename BPTBase<Beta>::LightVertex BPTBase<Beta>::_sample_light(random_generator_t& generator) {
    return _sample_to_vertex(_scene->sampleLight(generator));
}

template <class Beta>
void BPTBase<Beta>::_traceLight(RandomEngine& generator, light_path_t& path) {
    size_t itr = path.size() + 1, prv = path.size();

    if (_russian_roulette(generator)) {
        return;
    }

    LightSample sample = _scene->sampleLight(generator);
    path.emplace_back();
    path[prv] = _sample_to_vertex(sample);

    while (!_russian_roulette(generator)) {
        auto bsdf = _scene->sampleBSDF(generator, path[prv].surface, path[prv].omega);

        auto surface = _scene->intersectMesh(path[prv].surface, bsdf.omega);

        if (!surface.is_present()) {
            break;
        }

        path.emplace_back();

        path[itr].surface = surface;
        path[itr].omega = -bsdf.omega;

        auto edge = Edge(path[prv].surface, path[itr].surface, path[itr].omega);

        path[itr].throughput
            = path[prv].throughput
            * bsdf.throughput
            * edge.bCosTheta
            * _roulette_inv;

        if (l1Norm(path[itr].throughput) < FLT_EPSILON) {
            path.pop_back();
            break;
        }

        path[itr].throughput /= bsdf.density;

        path[prv].finite = min(path[prv].finite, bsdf.finite);
        path[itr].finite = bsdf.finite;

        path[itr].a = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

        path[itr].A
            = (path[prv].A
                * Beta::beta(bsdf.densityRev)
                + path[prv].a * path[prv].finite)
            * Beta::beta(edge.bGeometry)
            * path[itr].a;

        if (bsdf.finite == 0) {
            path[prv] = path[itr];
            path.pop_back();
        }
        else {
            prv = itr;
            ++itr;
        }
    }

    auto bsdf = _scene->sampleBSDF(
        generator,
        path[prv].surface,
        path[prv].omega);

    if (bsdf.finite == 0) {
        path.pop_back();
    }
}

template <class Beta> vec3 BPTBase<Beta>::_connect(
    const LightVertex& light,
    const EyeVertex& eye) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    auto edge = Edge(light.surface, eye.surface, omega);

    float Ap
        = (light.A * Beta::beta(lightBSDF.densityRev) + light.a * light.finite)
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev);

    float Cp
        = (eye.C * Beta::beta(eyeBSDF.density) + eye.c * eye.finite)
        * Beta::beta(edge.fGeometry * lightBSDF.density);

    float weightInv = Ap + Cp + 1.0f;

    vec3 result = _scene->occluded(eye.surface, light.surface)
        * light.throughput
        * lightBSDF.throughput
        * eye.throughput
        * eyeBSDF.throughput
        * edge.bCosTheta
        * edge.fGeometry;

    return l1Norm(result) < FLT_EPSILON ? vec3(0.0f) : result / weightInv;
}

template <class Beta> vec3 BPTBase<Beta>::_connect_light(const EyeVertex& eye) {
    auto bsdf = _scene->queryBSDF(eye.surface, vec3(0.0f), eye.omega);

    if (l1Norm(bsdf.throughput) < FLT_MIN) {
        return vec3(0.0f);
    }

    auto lsdf = _scene->queryLSDF(eye.surface, eye.omega);

    float Cp
        = (eye.C * Beta::beta(bsdf.density) + eye.c * eye.finite)
        * Beta::beta(lsdf.density);

    float weightInv = Cp + 1.0f;

    return lsdf.radiance
        * eye.throughput
        / weightInv;
}

template <class Beta>
vec3 BPTBase<Beta>::_connect_directional(const EyeVertex& eye, const LightSample& sample) {
    auto isect = _scene->intersect(eye.surface, -sample.normal());

    if (isect.material_id == sample.surface.material_id) {
        auto eyeBSDF = _scene->queryBSDF(eye.surface, -sample.normal(), eye.omega);

        float Cp
            = (eye.C * Beta::beta(eyeBSDF.density) + eye.c * eye.finite)
            * Beta::beta(abs(dot(sample.normal(), eye.surface.normal()))
                / distance2(isect.position(), eye.surface.position()));

        float weightInv = Cp + 1.0f;

        vec3 result = sample.radiance() / sample.light_density * _roulette_inv
            * eye.throughput
            * eyeBSDF.throughput
            * abs(dot(sample.normal(), eye.surface.normal()));

        return l1Norm(result) < FLT_EPSILON ? vec3(0.0f) : result / weightInv;
    }
    else {
        return vec3(0.0f);
    }
}

template <class Beta>
vec3 BPTBase<Beta>::_connect(render_context_t& context, const EyeVertex& eye, const light_path_t& path) {
    vec3 radiance = vec3(0.0f);

    if (!_russian_roulette(*context.generator)) {
        LightSample sample = _scene->sampleLight(*context.generator);

        if (sample.kind == light_kind::area) {
            radiance += _connect(_sample_to_vertex(sample), eye);
        }
        else if (!eye.surface.is_camera()) {
            radiance += _connect_directional(eye, sample);
        }
    }

    for (size_t i = 1; i < path.size(); ++i) {
        radiance += _connect(path[i], eye);
    }

    return radiance;
}

template <class Beta>
vec3 BPTBase<Beta>::_connect_eye(
    render_context_t& context,
    const EyeVertex& eye,
    const light_path_t& path) {
    vec3 radiance = vec3(0.0f);

    for (size_t index = 0; index < path.size(); ++index) {
        vec3 omega = normalize(path[index].surface.position() - eye.surface.position());

        radiance += _accumulate(
            context,
            omega,
            [&] {
                float camera_coefficient = _camera_coefficient(
                    path[index].omega,
                    path[index].surface.gnormal,
                    path[index].surface.normal(),
                    omega,
                    eye.surface.normal());

                return _connect(path[index], eye) * context.focal_factor_y * camera_coefficient;
            });
    }

    return radiance;
}

template <class Beta>
bool BPTBase<Beta>::_russian_roulette(random_generator_t& generator) const {
    return _roulette < generator.sample();
}

BPTb::BPTb(const shared<const Scene>& scene, float lights, float roulette, float beta, size_t num_threads)
    : BPTBase<VariableBeta>(scene, lights, roulette, beta, num_threads)
{
    VariableBeta::init(beta);
}

template class BPTBase<FixedBeta<0>>;
template class BPTBase<FixedBeta<1>>;
template class BPTBase<FixedBeta<2>>;
template class BPTBase<VariableBeta>;

}
