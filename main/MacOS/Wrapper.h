#pragma once

#import <Metal/Metal.h>
#import <CoreGraphics/CoreGraphics.h>

@interface Wrapper : NSObject

- (id<MTLDevice>)   getDevice;
- (void) nextDrawable: (id<MTLDrawable>)drawable
                texture: (id<MTLTexture>)drawableTexture
                width: (uint32_t)width
               height: (uint32_t)height;

- (void) render;
- (void) update: (CGFloat)time;

@property (strong) id<MTLDevice>        device;
@property (strong) id<MTLDrawable>      drawable;
@property (strong) id<MTLTexture>       swapChainTexture;

@end
