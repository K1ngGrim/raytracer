// GPU-Renderer mit Metal: Window::RenderGpu() rechnet das Bild mit dem Kernel aus gpu/raytrace.metal
// und schreibt das Ergebnis in Window::pixels, genau wie die CPU-Variante Window::Render().
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../view/window.h"
#include "metal_source.h"   // kMetalSource, wird von CMake aus gpu/raytrace.metal erzeugt

namespace {

// Datenlayout wie in gpu/raytrace.metal
struct GpuObject {
    float p0[4], p1[4], p2[4], n0[4], n1[4], n2[4], color[4], mat[4];
};

struct GpuLight {
    float position[4], color[4];
};

struct Params {
    float camera_center[4];
    float pixel00[4];
    float delta_u[4];
    float delta_v[4];
    uint32_t width, height, samples, max_depth;
    uint32_t num_objects, num_lights, path_tracing, row_start;
    float ray_bias, sky_brightness, exposure, pad;
};

static_assert(sizeof(GpuObject) == 128, "GpuObject must match the Metal struct");
static_assert(sizeof(GpuLight) == 32, "GpuLight must match the Metal struct");
static_assert(sizeof(Params) == 112, "Params must match the Metal struct");

void set4(float (&dst)[4], const Vector3df &v, float w = 0.f) {
    dst[0] = v[0]; dst[1] = v[1]; dst[2] = v[2]; dst[3] = w;
}

}

bool Window::RenderGpu() {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            printf("Error: no Metal device found\n");
            return false;
        }

        // Kernel zur Laufzeit kompilieren (ohne Fast-Math, die grossen Kugeln der Szene brauchen die volle Genauigkeit)
        NSError *error = nil;
        MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        options.fastMathEnabled = NO;
#pragma clang diagnostic pop
        id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:kMetalSource] options:options error:&error];
        if (!library) {
            printf("Error compiling the Metal kernel: %s\n", error.localizedDescription.UTF8String);
            return false;
        }
        id<MTLFunction> function = [library newFunctionWithName:@"render"];
        id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:function error:&error];
        if (!pipeline) {
            printf("Error creating the Metal pipeline: %s\n", error.localizedDescription.UTF8String);
            return false;
        }

        // Szene in GPU-Puffer packen
        std::vector<GpuObject> objects(std::max<size_t>(1, world->objects.size()));
        for (size_t i = 0; i < world->objects.size(); ++i) {
            const WorldObject &obj = world->objects[i];
            GpuObject &g = objects[i];
            if (obj.is_triangle) {
                set4(g.p0, obj.triangle.get_a());
                set4(g.p1, obj.triangle.get_b());
                set4(g.p2, obj.triangle.get_c());
                set4(g.n0, obj.triangle.get_na());
                set4(g.n1, obj.triangle.get_nb());
                set4(g.n2, obj.triangle.get_nc());
                set4(g.color, obj.material.materialColor, 1.f);
            } else {
                set4(g.p0, obj.sphere.get_center(), obj.sphere.get_radius());
                set4(g.color, obj.material.materialColor, 0.f);
            }
            g.mat[0] = float(obj.material.type);
            g.mat[1] = obj.material.roughness;
            g.mat[2] = obj.material.refractionIndex;
            g.mat[3] = obj.material.emission;
        }

        std::vector<GpuLight> lights(std::max<size_t>(1, world->lights.size()));
        for (size_t i = 0; i < world->lights.size(); ++i) {
            set4(lights[i].position, world->lights[i].center);
            set4(lights[i].color, world->lights[i].lightColor);
        }

        const int w = int(this->width), h = int(this->height);
        const int samples = std::max(1, this->samples_per_pixel);

        // Der Ausgabepuffer hat 4 Byte pro Pixel (0x00RRGGBB). Zu grosse Bilder koennen Metal nicht allokieren.
        const size_t out_bytes = size_t(w) * size_t(h) * sizeof(uint32_t);
        if (out_bytes > device.maxBufferLength) {
            printf("Error: a %dx%d image needs a %.1f GB GPU buffer, but this device allows at most %.1f GB\n",
                   w, h, double(out_bytes) / 1e9, double(device.maxBufferLength) / 1e9);
            return false;
        }

        id<MTLBuffer> object_buffer = [device newBufferWithBytes:objects.data() length:objects.size() * sizeof(GpuObject) options:MTLResourceStorageModeShared];
        id<MTLBuffer> light_buffer = [device newBufferWithBytes:lights.data() length:lights.size() * sizeof(GpuLight) options:MTLResourceStorageModeShared];
        id<MTLBuffer> out_buffer = [device newBufferWithLength:out_bytes options:MTLResourceStorageModeShared];
        id<MTLCommandQueue> queue = [device newCommandQueue];
        if (!object_buffer || !light_buffer || !out_buffer || !queue) {
            printf("Error: could not allocate the GPU buffers (%.1f GB for the image)\n", double(out_bytes) / 1e9);
            return false;
        }

        Params params = {};
        set4(params.camera_center, cam->camera_center);
        set4(params.pixel00, viewport->pixel00_loc);
        set4(params.delta_u, viewport->pixel_delta_u);
        set4(params.delta_v, viewport->pixel_delta_v);
        params.width = uint32_t(w);
        params.height = uint32_t(h);
        params.samples = uint32_t(samples);
        params.max_depth = uint32_t(this->max_depth);
        params.num_objects = uint32_t(world->objects.size());
        params.num_lights = uint32_t(world->lights.size());
        params.path_tracing = cam->path_tracing ? 1u : 0u;
        params.ray_bias = world->ray_bias;
        params.sky_brightness = world->sky_brightness;
        params.exposure = this->exposure;

        printf("DEBUG: Metal device: %s, %zu objects, %zu lights\n", device.name.UTF8String, world->objects.size(), world->lights.size());

        // In Zeilenbloecken rechnen, damit ein einzelner GPU-Aufruf nicht zu lange laeuft (Watchdog) und es einen Fortschritt gibt
        const int rows_per_chunk = std::max(1, int(1.0e6 / (double(w) * samples)));
        const NSUInteger tg_width = pipeline.threadExecutionWidth;
        const NSUInteger tg_height = std::max<NSUInteger>(1, std::min<NSUInteger>(8, pipeline.maxTotalThreadsPerThreadgroup / tg_width));

        for (int row = 0; row < h; row += rows_per_chunk) {
            int rows = std::min(rows_per_chunk, h - row);
            params.row_start = uint32_t(row);

            id<MTLCommandBuffer> command_buffer = [queue commandBuffer];
            id<MTLComputeCommandEncoder> encoder = [command_buffer computeCommandEncoder];
            [encoder setComputePipelineState:pipeline];
            [encoder setBytes:&params length:sizeof(Params) atIndex:0];
            [encoder setBuffer:object_buffer offset:0 atIndex:1];
            [encoder setBuffer:light_buffer offset:0 atIndex:2];
            [encoder setBuffer:out_buffer offset:0 atIndex:3];
            [encoder dispatchThreads:MTLSizeMake(NSUInteger(w), NSUInteger(rows), 1) threadsPerThreadgroup:MTLSizeMake(tg_width, tg_height, 1)];
            [encoder endEncoding];
            [command_buffer commit];
            [command_buffer waitUntilCompleted];

            if (command_buffer.status == MTLCommandBufferStatusError) {
                printf("\nError: Metal command buffer failed: %s\n", command_buffer.error.localizedDescription.UTF8String);
                return false;
            }
            printf("\rRendering (GPU): %3d%%", std::min(h, row + rows) * 100 / h);
            fflush(stdout);
        }
        printf("\n");

        // Der Kernel liefert schon fertige Pixel im Format von Window::pixels
        std::memcpy(this->pixels.data(), out_buffer.contents, out_bytes);
        return true;
    }
}
