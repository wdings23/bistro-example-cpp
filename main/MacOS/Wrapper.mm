#include "Wrapper.h"
#include "App.h"
#include <memory>


static Wrapper* _instance;
static std::unique_ptr<CApp>    _app;

@interface Wrapper()
@property id<MTLLibrary>    defaultLibrary;

@end

@implementation Wrapper

/*
**
*/
- (id) init
{
    self = [super init];
    _instance = self;
    
    _app = std::make_unique<CApp>();
    
    CApp::AppDescriptor appDesc = {};
    _app->init(appDesc);
    
    return self;
}

/*
**
*/
- (void) render
{
    _app->render();
}

/*
**
*/
- (void) update: (CGFloat)time
{
    _app->update(time);
}

/*
**
*/
- (void) nextDrawable: 
    (id<MTLDrawable>)drawable
    texture: (id<MTLTexture>)drawableTexture
    width: (uint32_t)width
    height: (uint32_t)height
{
    self.drawable = drawable;
    self.swapChainTexture = drawableTexture;
    _app->nextDrawable(
        self.drawable,
        self.swapChainTexture,
        width,
        height);
}

/*
**
*/
- (id<MTLDevice>) getDevice
{
    return (__bridge id)_app->getNativeDevice();
}

@end
