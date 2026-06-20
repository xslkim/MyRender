#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include "../base/Vector.hpp"
#include "Config.hpp"

// ---------------------------------------------------------------------------
// BloomPass -- C2: HDR bloom applied before tonemapping.
//
// Pyramid approach: downsample bright buffer through 3 levels (1/2, 1/4, 1/8),
// blur each level with a small-radius box filter, then upsample back in stages
// accumulating into a full-res bloom buffer.  This gives a wide, soft glow
// with low per-pixel intensity -- matching Unity URP's dual Kawase style.
// ---------------------------------------------------------------------------
namespace BloomPass {

static void BoxBlurH(const std::vector<float>& src, std::vector<float>& dst,
                     int W, int H, int r)
{
    float inv = 1.0f / (2*r+1);
    for (int y = 0; y < H; ++y) {
        float sR=0,sG=0,sB=0;
        for (int kx=-r; kx<=r; ++kx) {
            int i = (y*W + std::max(0,std::min(W-1,kx)))*3;
            sR+=src[i]; sG+=src[i+1]; sB+=src[i+2];
        }
        for (int x=0; x<W; ++x) {
            int o=(y*W+x)*3;
            dst[o]=sR*inv; dst[o+1]=sG*inv; dst[o+2]=sB*inv;
            int il=(y*W+std::max(0,    x-r  ))*3;
            int ir=(y*W+std::min(W-1,x+r+1))*3;
            sR+=src[ir]-src[il]; sG+=src[ir+1]-src[il+1]; sB+=src[ir+2]-src[il+2];
        }
    }
}

static void BoxBlurV(const std::vector<float>& src, std::vector<float>& dst,
                     int W, int H, int r)
{
    float inv = 1.0f / (2*r+1);
    for (int x=0; x<W; ++x) {
        float sR=0,sG=0,sB=0;
        for (int ky=-r; ky<=r; ++ky) {
            int i=(std::max(0,std::min(H-1,ky))*W+x)*3;
            sR+=src[i]; sG+=src[i+1]; sB+=src[i+2];
        }
        for (int y=0; y<H; ++y) {
            int o=(y*W+x)*3;
            dst[o]=sR*inv; dst[o+1]=sG*inv; dst[o+2]=sB*inv;
            int il=(std::max(0,    y-r  )*W+x)*3;
            int ir=(std::min(H-1,y+r+1)*W+x)*3;
            sR+=src[ir]-src[il]; sG+=src[ir+1]-src[il+1]; sB+=src[ir+2]-src[il+2];
        }
    }
}

// Average 2x2 downsample.
static void Downsample2x(const std::vector<float>& src, std::vector<float>& dst,
                         int sW, int sH, int dW, int dH)
{
    for (int y=0; y<dH; ++y) for (int x=0; x<dW; ++x) {
        int x1=std::min(x*2+1,sW-1), y1=std::min(y*2+1,sH-1);
        int di=(y*dW+x)*3;
        for (int c=0; c<3; ++c) {
            dst[di+c] = (src[(y*2*sW+x*2)*3+c] + src[(y*2*sW+x1)*3+c]
                       + src[(y1*sW+x*2)*3+c]  + src[(y1*sW+x1)*3+c]) * 0.25f;
        }
    }
}

// Bilinear 2x upsample, accumulate (+= scale) into dst[dW*dH*3].
static void Upsample2xAdd(const std::vector<float>& src, std::vector<float>& dst,
                          int sW, int sH, int dW, int dH, float scale)
{
    for (int y=0; y<dH; ++y) for (int x=0; x<dW; ++x) {
        // Map dst pixel centre to src pixel space.
        float fx = (x+0.5f)*0.5f - 0.5f;
        float fy = (y+0.5f)*0.5f - 0.5f;
        int x0=std::max(0,(int)fx), x1=std::min(sW-1,x0+1);
        int y0=std::max(0,(int)fy), y1=std::min(sH-1,y0+1);
        float tx=fx-(int)fx, ty=fy-(int)fy;
        int oi=(y*dW+x)*3;
        for (int c=0; c<3; ++c) {
            float v = (src[(y0*sW+x0)*3+c]*(1-tx)+src[(y0*sW+x1)*3+c]*tx)*(1-ty)
                    + (src[(y1*sW+x0)*3+c]*(1-tx)+src[(y1*sW+x1)*3+c]*tx)*   ty;
            dst[oi+c] += v*scale;
        }
    }
}

// colorBuffer: Render::GetColorBuffer() -- float[W*H*4], linear HDR RGBA.
inline void Apply(float* colorBuffer, int W, int H, float threshold, float intensity)
{
    if (intensity <= 0.0f) return;

    const int N = W*H;
    const float knee = threshold * 0.5f;

    // Step 1: Threshold extract (with soft knee) into full-res bright buffer.
    std::vector<float> bright(N*3, 0.0f);
    for (int i=0; i<N; ++i) {
        float r=colorBuffer[i*4], g=colorBuffer[i*4+1], b=colorBuffer[i*4+2];
        float lum=r*0.2126f+g*0.7152f+b*0.0722f;
        if (lum<=0.0f) continue;
        float rq    = std::max(0.0f, std::min(lum-threshold+knee, 2.0f*knee));
        float soft  = rq*rq*0.25f / (knee+1e-5f);
        float factor= std::max(lum-threshold, soft) / (lum+1e-5f);
        if (factor<=0.0f) continue;
        bright[i*3]=r*factor; bright[i*3+1]=g*factor; bright[i*3+2]=b*factor;
    }

    // Step 2: Build 3-level pyramid: L0=full, L1=1/2, L2=1/4, L3=1/8.
    int W1=W/2, H1=H/2, W2=W/4, H2=H/4, W3=W/8, H3=H/8;

    std::vector<float> L1(W1*H1*3), L1b(W1*H1*3);
    std::vector<float> L2(W2*H2*3), L2b(W2*H2*3);
    std::vector<float> L3(W3*H3*3), L3b(W3*H3*3);
    std::vector<float> tmp(N*3);

    // Downsample + blur each level.
    Downsample2x(bright, L1, W,  H,  W1, H1);
    BoxBlurH(L1, L1b, W1, H1, 5); BoxBlurV(L1b, L1, W1, H1, 5);

    Downsample2x(L1, L2, W1, H1, W2, H2);
    BoxBlurH(L2, L2b, W2, H2, 5); BoxBlurV(L2b, L2, W2, H2, 5);

    Downsample2x(L2, L3, W2, H2, W3, H3);
    BoxBlurH(L3, L3b, W3, H3, 5); BoxBlurV(L3b, L3, W3, H3, 5);

    // Upsample back in stages, accumulate into full-res bloom buffer.
    // Each level contributes equally (weight = 0.25 per level, 4 levels).
    std::vector<float> bloom(N*3, 0.0f);
    // L0: tight halo at full res (blur bright in-place).
    BoxBlurH(bright, tmp, W, H, 3); BoxBlurV(tmp, bloom, W, H, 3);  // accumulate L0

    // L1 -> full
    std::vector<float> up1(N*3, 0.0f);
    Upsample2xAdd(L1, up1, W1, H1, W, H, 1.0f);
    for (int i=0; i<N*3; ++i) bloom[i]+=up1[i];

    // L2 -> L1 size -> full
    std::vector<float> up2(W1*H1*3, 0.0f);
    Upsample2xAdd(L2, up2, W2, H2, W1, H1, 1.0f);
    Upsample2xAdd(up2, bloom, W1, H1, W, H, 1.0f);

    // L3 -> L2 -> L1 -> full
    std::vector<float> up3b(W2*H2*3, 0.0f);
    Upsample2xAdd(L3, up3b, W3, H3, W2, H2, 1.0f);
    std::vector<float> up3a(W1*H1*3, 0.0f);
    Upsample2xAdd(up3b, up3a, W2, H2, W1, H1, 1.0f);
    Upsample2xAdd(up3a, bloom, W1, H1, W, H, 1.0f);

    // Step 3: Additive composite: divide by 4 levels, multiply by intensity.
    const float norm = intensity * 0.25f;
    for (int i=0; i<N; ++i) {
        colorBuffer[i*4  ] += bloom[i*3  ] * norm;
        colorBuffer[i*4+1] += bloom[i*3+1] * norm;
        colorBuffer[i*4+2] += bloom[i*3+2] * norm;
    }
}

} // namespace BloomPass