#include <runtime_assert>
#include <RayIsect.hpp>

namespace haste {

inline void setShadow(
    RTCScene scene,
    unsigned geomId,
    const Geometry& geometry)
{
    rtcSetMask(
        scene,
        geomId,
        geometry.castShadow()
            ? RayIsect::occluderMask()
            : RayIsect::lightMask());
}

const unsigned newMesh(RTCScene scene, const Geometry& geometry) {
    if (geometry.usesTriangles()) {
        unsigned geomId = rtcNewTriangleMesh(
            scene,
            RTC_GEOMETRY_STATIC,
            geometry.numTriangles(),
            geometry.numVertices(),
            1);

        int* indices = (int*)rtcMapBuffer(scene, geomId, RTC_INDEX_BUFFER);
        vec4* vertices = (vec4*)rtcMapBuffer(scene, geomId, RTC_VERTEX_BUFFER);

        geometry.updateBuffers(indices, vertices);

        rtcUnmapBuffer(scene, geomId, RTC_INDEX_BUFFER);
        rtcUnmapBuffer(scene, geomId, RTC_VERTEX_BUFFER);

        setShadow(scene, geomId, geometry);

        return geomId;
    }
    else {
        runtime_assert(geometry.usesQuads());

        unsigned geomId = rtcNewQuadMesh(
            scene,
            RTC_GEOMETRY_STATIC,
            geometry.numQuads(),
            geometry.numVertices(),
            1);

        int* indices = (int*)rtcMapBuffer(scene, geomId, RTC_INDEX_BUFFER);
        vec4* vertices = (vec4*)rtcMapBuffer(scene, geomId, RTC_VERTEX_BUFFER);

        geometry.updateBuffers(indices, vertices);

        rtcUnmapBuffer(scene, geomId, RTC_INDEX_BUFFER);
        rtcUnmapBuffer(scene, geomId, RTC_VERTEX_BUFFER);

        setShadow(scene, geomId, geometry);

        return geomId;
    }
}

}
